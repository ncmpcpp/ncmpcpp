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
				return U("Year");
			case 'n':
			case 'N':
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
			default:
				return U("?");
		}
	}
}

std::string Display::Columns(size_t list_width)
{
	if (Config.columns.empty())
		return "";
	
	std::basic_string<my_char_t> result;
	size_t where = 0;
	int width;
	
	std::vector<Column>::const_iterator next2last;
	bool last_fixed = Config.columns.back().fixed;
	if (Config.columns.size() > 1)
		next2last = Config.columns.end()-2;
	
	for (std::vector<Column>::const_iterator it = Config.columns.begin(); it != Config.columns.end(); ++it)
	{
		if (it == Config.columns.end()-1)
			width = list_width-where;
		else if (last_fixed && it == next2last)
			width = list_width-where-1-(++next2last)->width;
		else
			width = it->width*(it->fixed ? 1 : list_width/100.0);
		
		std::basic_string<my_char_t> tag;
		if (it->type.length() >= 1 && it->name.empty())
		{
			for (size_t j = 0; j < it->type.length(); ++j)
			{
				tag += toColumnName(it->type[j]);
				tag += '/';
			}
			tag.resize(tag.length()-1);
		}
		else
			tag = it->name;
		if (it->right_alignment)
		{
			long i = width-tag.length()-(it != Config.columns.begin());
			if (i > 0)
				result.resize(result.length()+i, ' ');
		}
		
		where += width;
		result += tag;
		
		if (result.length() > where)
		{
			result.resize(where);
			result += ' ';
		}
		else
			result.resize(std::min(where+1, size_t(COLS)), ' ');
	}
	result.resize(list_width);
	return TO_STRING(result);
}

void Display::SongsInColumns(const MPD::Song &s, void *data, Menu<MPD::Song> *menu)
{
	if (!s.Localized())
		const_cast<MPD::Song *>(&s)->Localize();
	
	/// FIXME: This function is pure mess, it needs to be
	/// rewritten and unified with Display::Columns() a bit.
	
	bool is_now_playing = menu == myPlaylist->Items && (menu->isFiltered() ? s.GetPosition() : menu->CurrentlyDrawedPosition()) == size_t(myPlaylist->NowPlaying);
	if (is_now_playing)
		*menu << Config.now_playing_prefix;
	
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
	
	std::vector<Column>::const_iterator next2last, last, it;
	size_t where = 0;
	int width;
	
	bool last_fixed = Config.columns.back().fixed;
	if (Config.columns.size() > 1)
		next2last = Config.columns.end()-2;
	last = Config.columns.end()-1;
	
	bool discard_colors = Config.discard_colors_if_item_is_selected && menu->isSelected(menu->CurrentlyDrawedPosition());
	
	for (it = Config.columns.begin(); it != Config.columns.end(); ++it)
	{
		if (where)
		{
			menu->GotoXY(where, menu->Y());
			*menu << ' ';
			if (!discard_colors && (it-1)->color != clDefault)
				*menu << clEnd;
		}
		
		if (it == Config.columns.end()-1)
			width = menu->GetWidth()-where;
		else if (last_fixed && it == next2last)
			width = menu->GetWidth()-where-1-(++next2last)->width;
		else
			width = it->width*(it->fixed ? 1 : menu->GetWidth()/100.0);
		
		MPD::Song::GetFunction get = 0;
		
		std::string tag;
		for (size_t i = 0; i < it->type.length(); ++i)
		{
			get = toGetFunction(it->type[i]);
			tag = get ? s.GetTags(get) : "";
			if (!tag.empty())
				break;
		}
		if (!discard_colors && it->color != clDefault)
			*menu << it->color;
		whline(menu->Raw(), 32, menu->GetWidth()-where);
		
		// last column might need to be shrinked to make space for np/sel suffixes
		if (it == last)
		{
			if (menu->isSelected(menu->CurrentlyDrawedPosition()))
				width -= Config.selected_item_suffix_length;
			if (is_now_playing)
				width -= Config.now_playing_suffix_length;
		}
		
		if (it->right_alignment)
		{
			if (width > 0 && (!tag.empty() || it->display_empty_tag))
			{
				int x, y;
				menu->GetXY(x, y);
				std::basic_string<my_char_t> wtag = TO_WSTRING(tag.empty() ? Config.empty_tag : tag).substr(0, width-!!x);
				*menu << XY(x+width-Window::Length(wtag)-!!x, y) << wtag;
			}
		}
		else
		{
			if (it == last)
			{
				if (width > 0)
				{
					std::basic_string<my_char_t> str;
					if (!tag.empty())
						str = TO_WSTRING(tag).substr(0, width-1);
					else if (it->display_empty_tag)
						str = TO_WSTRING(Config.empty_tag).substr(0, width-1);
					*menu << str;
				}
			}
			else
			{
				if (!tag.empty())
					*menu << tag;
				else if (it->display_empty_tag)
					*menu << Config.empty_tag;
			}
		}
		where += width;
	}
	if (!discard_colors && (--it)->color != clDefault)
		*menu << clEnd;
	if (is_now_playing)
		*menu << Config.now_playing_suffix;
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

