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

#include <algorithm>

#include "helpers.h"
#include "playlist.h"
#include "statusbar.h"

bool addSongToPlaylist(const MPD::Song &s, bool play, int position)
{
	bool result = false;
	if (Config.space_add_mode == SpaceAddMode::AddRemove && myPlaylist->checkForSong(s))
	{
		auto &w = myPlaylist->main();
		if (play)
		{
			auto song = std::find(w.beginV(), w.endV(), s);
			assert(song != w.endV());
			Mpd.PlayID(song->getID());
			result = true;
		}
		else
		{
			Mpd.StartCommandsList();
			for (auto it = w.rbeginV(); it != w.rendV(); ++it)
				if (*it == s)
					Mpd.Delete(it->getPosition());
			Mpd.CommitCommandsList();
			// we return false in this case
		}
	}
	else
	{
		int id = Mpd.AddSong(s, position);
		if (id >= 0)
		{
			Statusbar::printf("Added to playlist: %s",
				s.toString(Config.song_status_format_no_colors, Config.tags_separator)
			);
			if (play)
				Mpd.PlayID(id);
			result = true;
		}
	}
	return result;
}

std::string Timestamp(time_t t)
{
	char result[32];
#	ifdef WIN32
	result[strftime(result, 31, "%x %X", localtime(&t))] = 0;
#	else
	tm info;
	result[strftime(result, 31, "%x %X", localtime_r(&t, &info))] = 0;
#	endif // WIN32
	return result;
}

void markSongsInPlaylist(ProxySongList pl)
{
	size_t list_size = pl.size();
	for (size_t i = 0; i < list_size; ++i)
		if (auto s = pl.getSong(i))
			pl.setBold(i, myPlaylist->checkForSong(*s));
}

std::wstring Scroller(const std::wstring &str, size_t &pos, size_t width)
{
	std::wstring s(str);
	if (!Config.header_text_scrolling)
		return s;
	std::wstring result;
	size_t len = wideLength(s);
	
	if (len > width)
	{
		s += L" ** ";
		len = 0;
		auto b = s.begin(), e = s.end();
		for (auto it = b+pos; it < e && len < width; ++it)
		{
			if ((len += wcwidth(*it)) > width)
				break;
			result += *it;
		}
		if (++pos >= s.length())
			pos = 0;
		for (; len < width; ++b)
		{
			if ((len += wcwidth(*b)) > width)
				break;
			result += *b;
		}
	}
	else
		result = s;
	return result;
}

void writeCyclicBuffer(const NC::WBuffer &buf, NC::Window &w, size_t &start_pos,
                       size_t width, const std::wstring &separator)
{
	const auto &s = buf.str();
	size_t len = wideLength(s);
	if (len > width)
	{
		len = 0;
		const auto &ps = buf.properties();
		auto p = ps.begin();
		
		// load attributes from before starting pos
		for (; p != ps.end() && p->position() < start_pos; ++p)
			w << *p;
		
		auto write_buffer = [&](size_t start) {
			for (size_t i = start; i < s.length() && len < width; ++i)
			{
				for (; p != ps.end() && p->position() == i; ++p)
					w << *p;
				len += wcwidth(s[i]);
				if (len > width)
					break;
				w << s[i];
			}
			for (; p != ps.end(); ++p)
				w << *p;
			p = ps.begin();
		};
		
		write_buffer(start_pos);
		size_t i = 0;
		if (start_pos > s.length())
			i = start_pos - s.length();
		for (; i < separator.length() && len < width; ++i)
		{
			len += wcwidth(separator[i]);
			if (len > width)
				break;
			w << separator[i];
		}
		write_buffer(0);
		
		++start_pos;
		if (start_pos >= s.length() + separator.length())
			start_pos = 0;
	}
	else
		w << buf;
}
