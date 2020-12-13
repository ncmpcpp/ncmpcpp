/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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
#include <boost/range/adaptor/reversed.hpp>
#include <time.h>

#include "enums.h"
#include "helpers.h"
#include "format_impl.h"
#include "screens/playlist.h"
#include "statusbar.h"
#include "utility/functional.h"

const MPD::Song *currentSong(const BaseScreen *screen)
{
	const MPD::Song *ptr = nullptr;
	const auto *list = dynamic_cast<const SongList *>(screen->activeWindow());
	if (list != nullptr)
	{
		const auto it = list->currentS();
		if (it != list->endS())
			ptr = it->song();
	}
	return ptr;
}

MPD::SongIterator getDatabaseIterator(MPD::Connection &mpd)
{
	MPD::SongIterator result;
	try
	{
		result = mpd.GetDirectoryRecursive("/");
	}
	catch (MPD::ClientError &e)
	{
		if (e.code() == MPD_ERROR_CLOSED)
		{
			// If we can't get the database, display appropriate
			// error message and reconnect with the MPD server.
			Statusbar::print("Unable to fetch the data, increase max_output_buffer_size in your MPD configuration file");
			mpd.Disconnect();
			mpd.Connect();
		}
		else
			throw;
	}
	catch (MPD::ServerError &e)
	{
		// mopidy blacklists 'listallinfo' command by default and throws server
		// error when it receives it. Work around that to prevent ncmpcpp from
		// continuously retrying to send the command and looping.
		if (strstr(e.what(), "listallinfo") != nullptr
		    && strstr(e.what(), "disabled") != nullptr)
			Statusbar::print("Unable to fetch the data, server refused to process 'listallinfo' command");
		else
			throw;
	}
	return result;
}

void removeSongFromPlaylist(const SongMenu &playlist, const MPD::Song &s)
{
	Mpd.StartCommandsList();
	for (auto &item : boost::adaptors::reverse(playlist))
		if (item.value() == s)
			Mpd.Delete(item.value().getPosition());
	Mpd.CommitCommandsList();
}

bool addSongToPlaylist(const MPD::Song &s, bool play, int position)
{
	bool result = false;
	if (Config.space_add_mode == SpaceAddMode::AddRemove
	&&  myPlaylist->checkForSong(s)
	   )
	{
		result = true;
		if (play)
		{
			const auto begin = myPlaylist->main().beginV(), end = myPlaylist->main().endV();
			auto it = find_map_first(begin, end, s, [](const MPD::Song &found) {
				Mpd.PlayID(found.getID());
			});
			assert(it != end);
		}
		else
			removeSongFromPlaylist(myPlaylist->main(), s);
		return result;
	}
	int id = Mpd.AddSong(s, position);
	if (id >= 0)
	{
		Statusbar::printf("Added to playlist: %s",
			Format::stringify<char>(Config.song_status_format, &s)
		);
		if (play)
			Mpd.PlayID(id);
		result = true;
	}
	return result;
}

std::string timeFormat(const char *format, time_t t)
{
	char result[32];
	tm tinfo;
	localtime_r(&t, &tinfo);
	strftime(result, sizeof(result), format, &tinfo);
	return result;
}

std::string Timestamp(time_t t)
{
	char result[32];
	tm info;
	result[strftime(result, 31, "%x %X", localtime_r(&t, &info))] = 0;
	return result;
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
		for (; p != ps.end() && p->first < start_pos; ++p)
			w << p->second;
		
		auto write_buffer = [&](size_t start) {
			for (size_t i = start; i < s.length() && len < width; ++i)
			{
				for (; p != ps.end() && p->first == i; ++p)
					w << p->second;
				len += wcwidth(s[i]);
				if (len > width)
					break;
				w << s[i];
			}
			for (; p != ps.end(); ++p)
				w << p->second;
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
