/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
 *   electricityispower@gmail.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <cassert>

#include "browser.h"
#include "display.h"
#include "helpers.h"
#include "song_info.h"
#include "playlist.h"
#include "global.h"
#include "tag_editor.h"
#include "utility/type_conversions.h"

using Global::myScreen;

namespace {//

const my_char_t *toColumnName(char c)
{
	switch (c)
	{
		case 'l':
			return U("Time");
		case 'f':
			return U("Filename");
		case 'D':
			return U("Directory");
		case 'a':
			return U("Artist");
		case 'A':
			return U("Album Artist");
		case 't':
			return U("Title");
		case 'b':
			return U("Album");
		case 'y':
			return U("Date");
		case 'n': case 'N':
			return U("Track");
		case 'g':
			return U("Genre");
		case 'c':
			return U("Composer");
		case 'p':
			return U("Performer");
		case 'd':
			return U("Disc");
		case 'C':
			return U("Comment");
		case 'P':
			return U("Priority");
		default:
			return U("?");
	}
}

template <typename T>
void setProperties(NC::Menu<T> &menu, const MPD::Song &s, HasSongs &screen, bool &separate_albums,
                   bool &is_now_playing, bool &is_selected, bool &discard_colors)
{
	separate_albums = false;
	if (Config.playlist_separate_albums)
	{
		auto pl = screen.getProxySongList();
		assert(pl);
		auto next = pl->getSong(menu.DrawnPosition()+1);
		if (next && next->getAlbum() != s.getAlbum())
			separate_albums = true;
	}
	if (separate_albums)
		menu << NC::fmtUnderline;
	
	int song_pos = menu.isFiltered() ? s.getPosition() : menu.DrawnPosition();
	is_now_playing = static_cast<void *>(&menu) == myPlaylist->Items
	              && song_pos == myPlaylist->NowPlaying;
	is_selected = menu.Drawn().isSelected();
	discard_colors = Config.discard_colors_if_item_is_selected && is_selected;
}

template <typename T>
void showSongs(NC::Menu<T> &menu, const MPD::Song &s, HasSongs &screen, const std::string &format)
{
	bool separate_albums, is_now_playing, is_selected, discard_colors;
	setProperties(menu, s, screen, separate_albums, is_now_playing, is_selected, discard_colors);
	
	std::string line = s.toString(format, "$");
	for (auto it = line.begin(); it != line.end(); ++it)
	{
		if (*it == '$')
		{
			if (++it == line.end()) // end of format
			{
				menu << '$';
				break;
			}
			else if (isdigit(*it)) // color
			{
				if (!discard_colors)
					menu << NC::Color(*it-'0');
			}
			else if (*it == 'R') // right align
			{
				NC::basic_buffer<my_char_t> buf;
				buf << U(" ");
				String2Buffer(TO_WSTRING(line.substr(it-line.begin()+1)), buf);
				if (discard_colors)
					buf.RemoveFormatting();
				if (is_now_playing)
					buf << Config.now_playing_suffix;
				menu << NC::XY(menu.GetWidth()-buf.Str().length()-(is_selected ? Config.selected_item_suffix_length : 0), menu.Y()) << buf;
				if (separate_albums)
					menu << NC::fmtUnderlineEnd;
				return;
			}
			else // not a color nor right align, just a random character
				menu << *--it;
		}
		else if (*it == MPD::Song::FormatEscapeCharacter)
		{
			// treat '$' as a normal character if song format escape char is prepended to it
			if (++it == line.end() || *it != '$')
				--it;
			menu << *it;
		}
		else
			menu << *it;
	}
	if (is_now_playing)
		menu << Config.now_playing_suffix;
	if (separate_albums)
		menu << NC::fmtUnderlineEnd;
}

template <typename T>
void showSongsInColumns(NC::Menu<T> &menu, const MPD::Song &s, HasSongs &screen)
{
	if (Config.columns.empty())
		return;
	
	bool separate_albums, is_now_playing, is_selected, discard_colors;
	setProperties(menu, s, screen, separate_albums, is_now_playing, is_selected, discard_colors);
	
	if (is_now_playing)
		menu << Config.now_playing_prefix;
	
	int width;
	int y = menu.Y();
	int remained_width = menu.GetWidth();
	std::vector<Column>::const_iterator it, last = Config.columns.end() - 1;
	for (it = Config.columns.begin(); it != Config.columns.end(); ++it)
	{
		// check current X coordinate
		int x = menu.X();
		// column has relative width and all after it have fixed width,
		// so stretch it so it fills whole screen along with these after.
		if (it->stretch_limit >= 0) // (*)
			width = remained_width - it->stretch_limit;
		else
			width = it->fixed ? it->width : it->width * menu.GetWidth() * 0.01;
		// columns with relative width may shrink to 0, omit them
		if (width == 0)
			continue;
		// if column is not last, we need to have spacing between it
		// and next column, so we substract it now and restore later.
		if (it != last)
			--width;
		
		if (it == Config.columns.begin() && (is_now_playing || is_selected))
		{
			// here comes the shitty part. if we applied now playing or selected
			// prefix, first column's width needs to be properly modified, so
			// next column is not affected by them. if prefixes fit, we just
			// subtract their width from allowed column's width. if they don't,
			// then we pretend that they do, but we adjust current cursor position
			// so part of them will be overwritten by next column.
			int offset = 0;
			if (is_now_playing)
				offset += Config.now_playing_prefix_length;
			if (is_selected)
				offset += Config.selected_item_prefix_length;
			if (width-offset < 0)
			{
				remained_width -= width + 1;
				menu.GotoXY(width, y);
				menu << ' ';
				continue;
			}
			width -= offset;
			remained_width -= offset;
		}
		
		// if column doesn't fit into screen, discard it and any other after it.
		if (remained_width-width < 0 || width < 0 /* this one may come from (*) */)
			break;
		
		std::basic_string<my_char_t> tag;
		for (size_t i = 0; i < it->type.length(); ++i)
		{
			MPD::Song::GetFunction get = charToGetFunction(it->type[i]);
			tag = TO_WSTRING(get ? s.getTags(get) : "");
			if (!tag.empty())
				break;
		}
		if (tag.empty() && it->display_empty_tag)
			tag = TO_WSTRING(Config.empty_tag);
		NC::Window::Cut(tag, width);
		
		if (!discard_colors && it->color != NC::clDefault)
			menu << it->color;
		
		int x_off = 0;
		// if column uses right alignment, calculate proper offset.
		// otherwise just assume offset is 0, ie. we start from the left.
		if (it->right_alignment)
			x_off = std::max(0, width - int(NC::Window::Length(tag)));
		
		whline(menu.Raw(), KEY_SPACE, width);
		menu.GotoXY(x + x_off, y);
		menu << tag;
		menu.GotoXY(x + width, y);
		if (it != last)
		{
			// add missing width's part and restore the value.
			menu << ' ';
			remained_width -= width+1;
		}
		
		if (!discard_colors && it->color != NC::clDefault)
			menu << NC::clEnd;
	}
	
	// here comes the shitty part, second chapter. here we apply
	// now playing suffix or/and make room for selected suffix
	// (as it will be applied in Menu::Refresh when this function
	// returns there).
	if (is_now_playing)
	{
		int np_x = menu.GetWidth() - Config.now_playing_suffix_length;
		if (is_selected)
			np_x -= Config.selected_item_suffix_length;
		menu.GotoXY(np_x, y);
		menu << Config.now_playing_suffix;
	}
	if (is_selected)
		menu.GotoXY(menu.GetWidth() - Config.selected_item_suffix_length, y);
	
	if (separate_albums)
		menu << NC::fmtUnderlineEnd;
}

}

std::string Display::Columns(size_t list_width)
{
	if (Config.columns.empty())
		return "";
	
	std::string result;
	
	int width;
	int remained_width = list_width;
	std::vector<Column>::const_iterator it, last = Config.columns.end() - 1;
	for (it = Config.columns.begin(); it != Config.columns.end(); ++it)
	{
		// column has relative width and all after it have fixed width,
		// so stretch it so it fills whole screen along with these after.
		if (it->stretch_limit >= 0) // (*)
			width = remained_width - it->stretch_limit;
		else
			width = it->fixed ? it->width : it->width * list_width * 0.01;
		// columns with relative width may shrink to 0, omit them
		if (width == 0)
			continue;
		// if column is not last, we need to have spacing between it
		// and next column, so we substract it now and restore later.
		if (it != last)
			--width;
		
		// if column doesn't fit into screen, discard it and any other after it.
		if (remained_width-width < 0 || width < 0 /* this one may come from (*) */)
			break;
		
		std::basic_string<my_char_t> name;
		if (it->name.empty())
		{
			for (size_t j = 0; j < it->type.length(); ++j)
			{
				name += toColumnName(it->type[j]);
				name += '/';
			}
			name.resize(name.length()-1);
		}
		else
			name = it->name;
		NC::Window::Cut(name, width);
		
		int x_off = std::max(0, width - int(NC::Window::Length(name)));
		if (it->right_alignment)
		{
			result += std::string(x_off, KEY_SPACE);
			result += TO_STRING(name);
		}
		else
		{
			result += TO_STRING(name);
			result += std::string(x_off, KEY_SPACE);
		}
		
		if (it != last)
		{
			// add missing width's part and restore the value.
			remained_width -= width+1;
			result += ' ';
		}
	}
	
	return result;
}

void Display::SongsInColumns(NC::Menu<MPD::Song> &menu, HasSongs &screen)
{
	showSongsInColumns(menu, menu.Drawn().value(), screen);
}

void Display::Songs(NC::Menu<MPD::Song> &menu, HasSongs &screen, const std::string &format)
{
	showSongs(menu, menu.Drawn().value(), screen, format);
}

void Display::Tags(NC::Menu<MPD::MutableSong> &menu)
{
	const MPD::MutableSong &s = menu.Drawn().value();
	size_t i = myTagEditor->TagTypes->Choice();
	if (i < 11)
	{
		ShowTag(menu, s.getTags(SongInfo::Tags[i].Get));
	}
	else if (i == 12)
	{
		if (s.getNewURI().empty())
			menu << s.getName();
		else
			menu << s.getName() << Config.color2 << " -> " << NC::clEnd << s.getNewURI();
	}
}

void Display::Outputs(NC::Menu<MPD::Output> &menu)
{
	menu << menu.Drawn().value().name();
}

void Display::Items(NC::Menu<MPD::Item> &menu)
{
	const MPD::Item &item = menu.Drawn().value();
	switch (item.type)
	{
		case MPD::itDirectory:
			menu << "[" << getBasename(item.name) << "]";
			break;
		case MPD::itSong:
			if (!Config.columns_in_browser)
				showSongs(menu, *item.song, *myBrowser, Config.song_list_format);
			else
				showSongsInColumns(menu, *item.song, *myBrowser);
			break;
		case MPD::itPlaylist:
			menu << Config.browser_playlist_prefix << getBasename(item.name);
			break;
	}
}

void Display::SearchEngine(NC::Menu<SEItem> &menu)
{
	const SEItem &si = menu.Drawn().value();
	if (si.isSong())
	{
		if (!Config.columns_in_search_engine)
			showSongs(menu, si.song(), *mySearcher, Config.song_list_format);
		else
			showSongsInColumns(menu, si.song(), *mySearcher);
	}
	else
		menu << si.buffer();
}
