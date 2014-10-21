/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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
#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "song_info.h"
#include "playlist.h"
#include "global.h"
#include "tag_editor.h"
#include "utility/string.h"
#include "utility/type_conversions.h"

using Global::myScreen;

namespace {//

const wchar_t *toColumnName(char c)
{
	switch (c)
	{
		case 'l':
			return L"Time";
		case 'f':
			return L"Filename";
		case 'D':
			return L"Directory";
		case 'a':
			return L"Artist";
		case 'A':
			return L"Album Artist";
		case 't':
			return L"Title";
		case 'b':
			return L"Album";
		case 'y':
			return L"Date";
		case 'n': case 'N':
			return L"Track";
		case 'g':
			return L"Genre";
		case 'c':
			return L"Composer";
		case 'p':
			return L"Performer";
		case 'd':
			return L"Disc";
		case 'C':
			return L"Comment";
		case 'P':
			return L"Priority";
		default:
			return L"?";
	}
}

template <typename T>
void setProperties(NC::Menu<T> &menu, const MPD::Song &s, const ProxySongList &pl, bool &separate_albums,
                   bool &is_now_playing, bool &is_selected, bool &discard_colors)
{
	size_t drawn_pos = menu.drawn() - menu.begin();
	separate_albums = false;
	if (Config.playlist_separate_albums)
	{
		size_t next_pos = drawn_pos+1;
		auto next = next_pos < pl.size() ? pl.getSong(next_pos) : 0;
		if (next && next->getAlbum() != s.getAlbum())
			separate_albums = true;
	}
	if (separate_albums)
	{
		menu << NC::Format::Underline;
		mvwhline(menu.raw(), menu.getY(), 0, KEY_SPACE, menu.getWidth());
	}
	
	is_selected = menu.drawn()->isSelected();
	discard_colors = Config.discard_colors_if_item_is_selected && is_selected;
	
	int song_pos = menu.isFiltered() ? s.getPosition() : drawn_pos;
	is_now_playing = Status::State::player() != MPD::psStop && myPlaylist->isActiveWindow(menu)
	              && song_pos == Status::State::currentSongPosition();
	if (is_now_playing)
		menu << Config.now_playing_prefix;
}

template <typename T>
void showSongs(NC::Menu<T> &menu, const MPD::Song &s, const ProxySongList &pl, const std::string &format)
{
	bool separate_albums, is_now_playing, is_selected, discard_colors;
	setProperties(menu, s, pl, separate_albums, is_now_playing, is_selected, discard_colors);
	
	size_t y = menu.getY();
	std::string line = Charset::utf8ToLocale(s.toString(format, Config.tags_separator, "$"));
	for (auto it = line.begin(); it != line.end(); ++it)
	{
		if (*it == '$')
		{
			++it;
			if (it == line.end()) // end of format
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
				NC::Buffer buf;
				buf << " ";
				stringToBuffer(++it, line.end(), buf);
				if (discard_colors)
					buf.removeProperties();
				size_t x_off = menu.getWidth() - wideLength(ToWString(buf.str()));
				if (is_now_playing)
					x_off -= Config.now_playing_suffix_length;
				if (is_selected)
					x_off -= Config.selected_item_suffix_length;
				menu << NC::XY(x_off, y) << buf;
				break;
			}
			else // not a color nor right align, just a random character
				menu << *--it;
		}
		else if (*it == MPD::Song::FormatEscapeCharacter)
		{
			++it;
			// treat '$' as a normal character if song format escape char is prepended to it
			if (it == line.end() || *it != '$')
				--it;
			menu << *it;
		}
		else
			menu << *it;
	}
	if (is_now_playing)
		menu << Config.now_playing_suffix;
	if (separate_albums)
		menu << NC::Format::NoUnderline;
}

template <typename T>
void showSongsInColumns(NC::Menu<T> &menu, const MPD::Song &s, const ProxySongList &pl)
{
	if (Config.columns.empty())
		return;
	
	bool separate_albums, is_now_playing, is_selected, discard_colors;
	setProperties(menu, s, pl, separate_albums, is_now_playing, is_selected, discard_colors);
	
	int width;
	int y = menu.getY();
	int remained_width = menu.getWidth();
	std::vector<Column>::const_iterator it, last = Config.columns.end() - 1;
	for (it = Config.columns.begin(); it != Config.columns.end(); ++it)
	{
		// check current X coordinate
		int x = menu.getX();
		// column has relative width and all after it have fixed width,
		// so stretch it so it fills whole screen along with these after.
		if (it->stretch_limit >= 0) // (*)
			width = remained_width - it->stretch_limit;
		else
			width = it->fixed ? it->width : it->width * menu.getWidth() * 0.01;
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
				menu.goToXY(width, y);
				menu << ' ';
				continue;
			}
			width -= offset;
			remained_width -= offset;
		}
		
		// if column doesn't fit into screen, discard it and any other after it.
		if (remained_width-width < 0 || width < 0 /* this one may come from (*) */)
			break;
		
		std::wstring tag;
		for (size_t i = 0; i < it->type.length(); ++i)
		{
			MPD::Song::GetFunction get = charToGetFunction(it->type[i]);
			assert(get);
			tag = ToWString(Charset::utf8ToLocale(s.getTags(get, Config.tags_separator)));
			if (!tag.empty())
				break;
		}
		if (tag.empty() && it->display_empty_tag)
			tag = ToWString(Config.empty_tag);
		wideCut(tag, width);
		
		if (!discard_colors && it->color != NC::Color::Default)
			menu << it->color;
		
		int x_off = 0;
		// if column uses right alignment, calculate proper offset.
		// otherwise just assume offset is 0, ie. we start from the left.
		if (it->right_alignment)
			x_off = std::max(0, width - int(wideLength(tag)));
		
		whline(menu.raw(), KEY_SPACE, width);
		menu.goToXY(x + x_off, y);
		menu << tag;
		menu.goToXY(x + width, y);
		if (it != last)
		{
			// add missing width's part and restore the value.
			menu << ' ';
			remained_width -= width+1;
		}
		
		if (!discard_colors && it->color != NC::Color::Default)
			menu << NC::Color::End;
	}
	
	// here comes the shitty part, second chapter. here we apply
	// now playing suffix or/and make room for selected suffix
	// (as it will be applied in Menu::Refresh when this function
	// returns there).
	if (is_now_playing)
	{
		int np_x = menu.getWidth() - Config.now_playing_suffix_length;
		if (is_selected)
			np_x -= Config.selected_item_suffix_length;
		menu.goToXY(np_x, y);
		menu << Config.now_playing_suffix;
	}
	if (is_selected)
		menu.goToXY(menu.getWidth() - Config.selected_item_suffix_length, y);
	
	if (separate_albums)
		menu << NC::Format::NoUnderline;
}

}

std::string Display::Columns(size_t list_width)
{
	std::string result;
	if (Config.columns.empty())
		return result;
	
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
		
		std::wstring name;
		if (it->name.empty())
		{
			size_t j = 0;
			while (true)
			{
				name += toColumnName(it->type[j]);
				++j;
				if (j < it->type.length())
					name += '/';
				else
					break;
			}
		}
		else
			name = it->name;
		wideCut(name, width);
		
		int x_off = std::max(0, width - int(wideLength(name)));
		if (it->right_alignment)
		{
			result += std::string(x_off, KEY_SPACE);
			result += Charset::utf8ToLocale(ToString(name));
		}
		else
		{
			result += Charset::utf8ToLocale(ToString(name));
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

void Display::SongsInColumns(NC::Menu< MPD::Song >& menu, const ProxySongList &pl)
{
	showSongsInColumns(menu, menu.drawn()->value(), pl);
}

void Display::Songs(NC::Menu< MPD::Song >& menu, const ProxySongList &pl, const std::string &format)
{
	showSongs(menu, menu.drawn()->value(), pl, format);
}

#ifdef HAVE_TAGLIB_H
void Display::Tags(NC::Menu<MPD::MutableSong> &menu)
{
	const MPD::MutableSong &s = menu.drawn()->value();
	if (s.isModified())
		menu << Config.modified_item_prefix;
	size_t i = myTagEditor->TagTypes->choice();
	if (i < 11)
	{
		ShowTag(menu, Charset::utf8ToLocale(s.getTags(SongInfo::Tags[i].Get, Config.tags_separator)));
	}
	else if (i == 12)
	{
		if (s.getNewName().empty())
			menu << Charset::utf8ToLocale(s.getName());
		else
			menu << Charset::utf8ToLocale(s.getName())
			     << Config.color2 << " -> " << NC::Color::End
			     << Charset::utf8ToLocale(s.getNewName());
	}
}
#endif // HAVE_TAGLIB_H

void Display::Items(NC::Menu<MPD::Item> &menu, const ProxySongList &pl)
{
	const MPD::Item &item = menu.drawn()->value();
	switch (item.type)
	{
		case MPD::itDirectory:
			menu << "["
			     << Charset::utf8ToLocale(getBasename(item.name))
			     << "]";
			break;
		case MPD::itSong:
			switch (Config.browser_display_mode)
			{
				case DisplayMode::Classic:
					showSongs(menu, *item.song, pl, Config.song_list_format);
					break;
				case DisplayMode::Columns:
					showSongsInColumns(menu, *item.song, pl);
					break;
			}
			break;
		case MPD::itPlaylist:
			menu << Config.browser_playlist_prefix
			     << Charset::utf8ToLocale(getBasename(item.name));
			break;
	}
}

void Display::SEItems(NC::Menu<SEItem> &menu, const ProxySongList &pl)
{
	const SEItem &si = menu.drawn()->value();
	if (si.isSong())
	{
		switch (Config.search_engine_display_mode)
		{
			case DisplayMode::Classic:
				showSongs(menu, si.song(), pl, Config.song_list_format);
				break;
			case DisplayMode::Columns:
				showSongsInColumns(menu, si.song(), pl);
				break;
		}
	}
	else
		menu << si.buffer();
}
