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

#include "display.h"
#include "helpers.h"
#include "song_info.h"
#include "playlist.h"
#include "global.h"

using Global::myScreen;

namespace
{
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
		Window::Cut(name, width);
		
		int x_off = std::max(0, width - int(Window::Length(name)));
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

void Display::SongsInColumns(const MPD::Song &s, void *data, Menu<MPD::Song> *menu)
{
	if (!s.Localized())
		const_cast<MPD::Song *>(&s)->Localize();
	
	if (Config.columns.empty())
		return;
	
	assert(data);
	bool separate_albums = false;
	if (Config.playlist_separate_albums && menu->CurrentlyDrawedPosition()+1 < menu->Size())
	{
		MPD::Song *next = static_cast<ScreenFormat *>(data)->screen->GetSong(menu->CurrentlyDrawedPosition()+1);
		if (next && next->GetAlbum() != s.GetAlbum())
			separate_albums = true;
	}
	if (separate_albums)
		*menu << fmtUnderline;
	
	int song_pos = menu->isFiltered() ? s.GetPosition() : menu->CurrentlyDrawedPosition();
	bool is_now_playing = menu == myPlaylist->Items && song_pos == myPlaylist->NowPlaying;
	bool is_selected = menu->isSelected(menu->CurrentlyDrawedPosition());
	bool discard_colors = Config.discard_colors_if_item_is_selected && is_selected;
	
	if (is_now_playing)
		*menu << Config.now_playing_prefix;
	
	int width;
	int y = menu->Y();
	int remained_width = menu->GetWidth();
	std::vector<Column>::const_iterator it, last = Config.columns.end() - 1;
	for (it = Config.columns.begin(); it != Config.columns.end(); ++it)
	{
		// check current X coordinate
		int x = menu->X();
		// column has relative width and all after it have fixed width,
		// so stretch it so it fills whole screen along with these after.
		if (it->stretch_limit >= 0) // (*)
			width = remained_width - it->stretch_limit;
		else
			width = it->fixed ? it->width : it->width * menu->GetWidth() * 0.01;
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
				menu->GotoXY(width, y);
				*menu << ' ';
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
			MPD::Song::GetFunction get = toGetFunction(it->type[i]);
			tag = TO_WSTRING(get ? s.GetTags(get) : "");
			if (!tag.empty())
				break;
		}
		if (tag.empty() && it->display_empty_tag)
			tag = TO_WSTRING(Config.empty_tag);
		Window::Cut(tag, width);
		
		if (!discard_colors && it->color != clDefault)
			*menu << it->color;
		
		int x_off = 0;
		// if column uses right alignment, calculate proper offset.
		// otherwise just assume offset is 0, ie. we start from the left.
		if (it->right_alignment)
			x_off = std::max(0, width - int(Window::Length(tag)));
		
		whline(menu->Raw(), KEY_SPACE, width);
		menu->GotoXY(x + x_off, y);
		*menu << tag;
		menu->GotoXY(x + width, y);
		if (it != last)
		{
			// add missing width's part and restore the value.
			*menu << ' ';
			remained_width -= width+1;
		}
		
		if (!discard_colors && it->color != clDefault)
			*menu << clEnd;
	}
	
	// here comes the shitty part, second chapter. here we apply
	// now playing suffix or/and make room for selected suffix
	// (as it will be applied in Menu::Refresh when this function
	// returns there).
	if (is_now_playing)
	{
		int np_x = menu->GetWidth() - Config.now_playing_suffix_length;
		if (is_selected)
			np_x -= Config.selected_item_suffix_length;
		menu->GotoXY(np_x, y);
		*menu << Config.now_playing_suffix;
	}
	if (is_selected)
		menu->GotoXY(menu->GetWidth() - Config.selected_item_suffix_length, y);
	
	if (separate_albums)
		*menu << fmtUnderlineEnd;
}

void Display::Songs(const MPD::Song &s, void *data, Menu<MPD::Song> *menu)
{
	if (!s.Localized())
		const_cast<MPD::Song *>(&s)->Localize();
	
	bool is_now_playing = menu == myPlaylist->Items && (menu->isFiltered() ? s.GetPosition() : menu->CurrentlyDrawedPosition()) == size_t(myPlaylist->NowPlaying);
	if (is_now_playing)
		*menu << Config.now_playing_prefix;
	
	assert(data);
	bool separate_albums = false;
	if (Config.playlist_separate_albums && menu->CurrentlyDrawedPosition()+1 < menu->Size())
	{
		MPD::Song *next = static_cast<ScreenFormat *>(data)->screen->GetSong(menu->CurrentlyDrawedPosition()+1);
		if (next && next->GetAlbum() != s.GetAlbum())
			separate_albums = true;
	}
	if (separate_albums)
	{
		*menu << fmtUnderline;
		mvwhline(menu->Raw(), menu->Y(), 0, ' ', menu->GetWidth());
	}
	
	bool discard_colors = Config.discard_colors_if_item_is_selected && menu->isSelected(menu->CurrentlyDrawedPosition());
	
	std::string line = s.toString(*static_cast<ScreenFormat *>(data)->format, "$");
	for (std::string::const_iterator it = line.begin(); it != line.end(); ++it)
	{
		if (*it == '$')
		{
			if (++it == line.end()) // end of format
			{
				*menu << '$';
				break;
			}
			else if (isdigit(*it)) // color
			{
				if (!discard_colors)
					*menu << Color(*it-'0');
			}
			else if (*it == 'R') // right align
			{
				basic_buffer<my_char_t> buf;
				buf << U(" ");
				String2Buffer(TO_WSTRING(line.substr(it-line.begin()+1)), buf);
				if (discard_colors)
					buf.RemoveFormatting();
				if (is_now_playing)
					buf << Config.now_playing_suffix;
				*menu << XY(menu->GetWidth()-buf.Str().length()-(menu->isSelected(menu->CurrentlyDrawedPosition()) ? Config.selected_item_suffix_length : 0), menu->Y()) << buf;
				if (separate_albums)
					*menu << fmtUnderlineEnd;
				return;
			}
			else // not a color nor right align, just a random character
				*menu << *--it;
		}
		else if (*it == MPD::Song::FormatEscapeCharacter)
		{
			// treat '$' as a normal character if song format escape char is prepended to it
			if (++it == line.end() || *it != '$')
				--it;
			*menu << *it;
		}
		else
			*menu << *it;
	}
	if (is_now_playing)
		*menu << Config.now_playing_suffix;
	if (separate_albums)
		*menu << fmtUnderlineEnd;
}

void Display::Tags(const MPD::Song &s, void *data, Menu<MPD::Song> *menu)
{
	size_t i = static_cast<Menu<std::string> *>(data)->Choice();
	if (i < 11)
	{
		ShowTag(*menu, s.GetTags(SongInfo::Tags[i].Get));
	}
	else if (i == 12)
	{
		if (s.GetNewName().empty())
			*menu << s.GetName();
		else
			*menu << s.GetName() << Config.color2 << " -> " << clEnd << s.GetNewName();
	}
}

void Display::Items(const MPD::Item &item, void *data, Menu<MPD::Item> *menu)
{
	switch (item.type)
	{
		case MPD::itDirectory:
		{
			if (item.song)
			{
				*menu << "[..]";
				return;
			}
			*menu << "[" << ExtractTopName(item.name) << "]";
			return;
		}
		case MPD::itSong:
			if (!Config.columns_in_browser)
				Display::Songs(*item.song, data, reinterpret_cast<Menu<MPD::Song> *>(menu));
			else
				Display::SongsInColumns(*item.song, data, reinterpret_cast<Menu<MPD::Song> *>(menu));
			return;
		case MPD::itPlaylist:
			*menu << Config.browser_playlist_prefix << ExtractTopName(item.name);
			return;
		default:
			return;
	}
}

void Display::SearchEngine(const std::pair<Buffer *, MPD::Song *> &pair, void *data, Menu< std::pair<Buffer *, MPD::Song *> > *menu)
{
	if (pair.second)
	{
		if (!Config.columns_in_search_engine)
			Display::Songs(*pair.second, data, reinterpret_cast<Menu<MPD::Song> *>(menu));
		else
			Display::SongsInColumns(*pair.second, data, reinterpret_cast<Menu<MPD::Song> *>(menu));
	}
	
	else
		*menu << *pair.first;
}

