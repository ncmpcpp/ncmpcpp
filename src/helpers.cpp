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

#include <cstring>
#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "clock.h"
#include "charset.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "status.h"
#include "tag_editor.h"
#include "help.h"
#include "playlist_editor.h"
#include "browser.h"
#include "media_library.h"
#include "search_engine.h"
#include "outputs.h"
#include "visualizer.h"

bool ConnectToMPD()
{
	if (!Mpd.Connect())
	{
		std::cout << "Couldn't connect to MPD (host = " << Mpd.GetHostname() << ", port = " << Mpd.GetPort() << "): " << Mpd.GetErrorMessage() << std::endl;
		return false;
	}
	return true;
}

void ParseArgv(int argc, char **argv)
{
	bool quit = 0;
	std::string now_playing_format = "{{{(%l) }{{%a - }%t}}|{%f}}";
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--host"))
		{
			if (++i >= argc)
				exit(0);
			Mpd.SetHostname(argv[i]);
			continue;
		}
		if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port"))
		{
			if (++i >= argc)
				exit(0);
			Mpd.SetPort(atoi(argv[i]));
			continue;
		}
		else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
		{
			std::cout << "ncmpcpp version: " << VERSION << "\n\n"
			<< "optional screens compiled-in:\n"
#			ifdef HAVE_TAGLIB_H
			<< " - tag editor\n"
			<< " - tiny tag editor\n"
#			endif
#			ifdef HAVE_CURL_CURL_H
			<< " - artist info\n"
#			endif
#			ifdef ENABLE_OUTPUTS
			<< " - outputs\n"
#			endif
#			ifdef ENABLE_VISUALIZER
			<< " - visualizer\n"
#			endif
#			ifdef ENABLE_CLOCK
			<< " - clock\n"
#			endif
			<< "\nencoding detection: "
#			ifdef HAVE_LANGINFO_H
			<< "enabled"
#			else
			<< "disabled"
#			endif // HAVE_LANGINFO_H
			<< "\nbuilt with support for:"
#			ifdef HAVE_CURL_CURL_H
			<< " curl"
#			endif
#			ifdef HAVE_ICONV_H
			<< " iconv"
#			endif
#			ifdef HAVE_FFTW3_H
			<< " fftw"
#			endif
#			ifdef USE_PDCURSES
			<< " pdcurses"
#			else
			<< " ncurses"
#			endif
#			ifdef HAVE_TAGLIB_H
			<< " taglib"
#			endif
#			ifdef _UTF8
			<< " unicode"
#			endif
			<< std::endl;
			exit(0);
		}
		else if (!strcmp(argv[i], "-?") || !strcmp(argv[i], "--help"))
		{
			std::cout
			<< "Usage: ncmpcpp [OPTION]...\n"
			<< "  -h, --host                connect to server at host [localhost]\n"
			<< "  -p, --port                connect to server at port [6600]\n"
			<< "  -c, --config              use alternative configuration file\n"
			<< "  -s, --screen <name>       specify the startup screen\n"
			<< "  -?, --help                show this help message\n"
			<< "  -v, --version             display version information\n"
			<< "  --now-playing             display now playing song [" << now_playing_format << "]\n"
			<< "\n"
			<< "  play                      start playing\n"
			<< "  pause                     pause the currently playing song\n"
			<< "  toggle                    toggle play/pause mode\n"
			<< "  stop                      stop playing\n"
			<< "  next                      play the next song\n"
			<< "  prev                      play the previous song\n"
			<< "  volume [+-]<num>          adjusts volume by [+-]<num>\n"
			;
			exit(0);
		}
		
		if (!ConnectToMPD())
			exit(1);
		
		if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--screen"))
		{
			if (++i == argc) {
				std::cout << "ncmpcpp: no screen specified" << std::endl;
				exit(0);
			}
			if (!strcmp(argv[i], "help"))
				Config.startup_screen = myHelp;
			else if (!strcmp(argv[i], "playlist"))
				Config.startup_screen = myPlaylist;
			else if (!strcmp(argv[i], "browser"))
				Config.startup_screen = myBrowser;
			else if (!strcmp(argv[i], "search-engine"))
				Config.startup_screen = mySearcher;
			else if (!strcmp(argv[i], "media-library"))
				Config.startup_screen = myLibrary;
			else if (!strcmp(argv[i], "playlist-editor"))
				Config.startup_screen = myPlaylistEditor;
#			ifdef HAVE_TAGLIB_H
			else if (!strcmp(argv[i], "tag-editor"))
				Config.startup_screen = myTagEditor;
#			endif // HAVE_TAGLIB_H
#			ifdef ENABLE_OUTPUTS
			else if (!strcmp(argv[i], "outputs"))
				Config.startup_screen = myOutputs;
#			endif // ENABLE_OUTPUTS
#			ifdef ENABLE_VISUALIZER
			else if (!strcmp(argv[i], "visualizer"))
				Config.startup_screen = myVisualizer;
#			endif // ENABLE_VISUALIZER
#			ifdef ENABLE_CLOCK
			else if (!strcmp(argv[i], "clock"))
				Config.startup_screen = myClock;
#			endif // ENABLE_CLOCK
			else {
				std::cout << "ncmpcpp: invalid screen: " << argv[i] << std::endl;
				exit(0);
			}
		}
		else if (!strcmp(argv[i], "--now-playing"))
		{
			Mpd.UpdateStatus();
			if (!Mpd.GetErrorMessage().empty())
			{
				std::cout << "Error: " << Mpd.GetErrorMessage() << std::endl;
				exit(1);
			}
			if (Mpd.isPlaying())
			{
				if (argc > ++i)
				{
					if (MPD::Song::isFormatOk("now-playing format", argv[i]))
					{
						// apply additional pair of braces
						now_playing_format = "{";
						now_playing_format += argv[i];
						now_playing_format += "}";
						Replace(now_playing_format, "\\n", "\n");
						Replace(now_playing_format, "\\t", "\t");
					}
				}
				std::cout << utf_to_locale_cpy(Mpd.GetCurrentSong().toString(now_playing_format)) << "\n";
			}
			exit(0);
		}
		else if (!strcmp(argv[i], "play"))
		{
			Mpd.Play();
			quit = 1;
		}
		else if (!strcmp(argv[i], "pause"))
		{
			Mpd.Pause(1);
			quit = 1;
		}
		else if (!strcmp(argv[i], "toggle"))
		{
			Mpd.UpdateStatus();
			if (!Mpd.GetErrorMessage().empty())
			{
				std::cout << "Error: " << Mpd.GetErrorMessage() << std::endl;
				exit(1);
			}
			Mpd.Toggle();
			quit = 1;
		}
		else if (!strcmp(argv[i], "stop"))
		{
			Mpd.Stop();
			quit = 1;
		}
		else if (!strcmp(argv[i], "next"))
		{
			Mpd.Next();
			quit = 1;
		}
		else if (!strcmp(argv[i], "prev"))
		{
			Mpd.Prev();
			quit = 1;
		}
		else if (!strcmp(argv[i], "volume"))
		{
			i++;
			Mpd.UpdateStatus();
			if (!Mpd.GetErrorMessage().empty())
			{
				std::cout << "Error: " << Mpd.GetErrorMessage() << std::endl;
				exit(1);
			}
			if (i != argc)
				Mpd.SetVolume(Mpd.GetVolume()+atoi(argv[i]));
			quit = 1;
		}
		else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--config"))
		{
			// this is used in NcmpcppConfig::CheckForCommandLineConfigFilePath, ignoring here.
			++i;
		}
		else
		{
			std::cout << "ncmpcpp: invalid option: " << argv[i] << std::endl;
			exit(0);
		}
		if (!Mpd.GetErrorMessage().empty())
		{
			std::cout << "Error: " << Mpd.GetErrorMessage() << std::endl;
			exit(0);
		}
	}
	if (quit)
		exit(0);
}

int CaseInsensitiveStringComparison::operator()(const std::string &a, const std::string &b)
{
	const char *i = a.c_str();
	const char *j = b.c_str();
	if (Config.ignore_leading_the)
	{
		if (hasTheWord(a))
			i += 4;
		if (hasTheWord(b))
			j += 4;
	}
	int dist;
	while (!(dist = tolower(*i)-tolower(*j)) && *j)
		++i, ++j;
	return dist;
}

bool CaseInsensitiveSorting::operator()(const MPD::Item &a, const MPD::Item &b)
{
	if (a.type == b.type)
	{
		switch (a.type)
		{
			case MPD::itDirectory:
				return cmp(ExtractTopName(a.name), ExtractTopName(b.name)) < 0;
			case MPD::itPlaylist:
				return cmp(a.name, b.name) < 0;
			case MPD::itSong:
				return Config.browser_sort_by_mtime
						? a.song->GetMTime() > b.song->GetMTime()
						: operator()(a.song, b.song);
			default: // there's no other type, just silence compiler.
				return 0;
		}
	}
	else
		return a.type < b.type;
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

void UpdateSongList(Menu<MPD::Song> *menu)
{
	bool bold = 0;
	for (size_t i = 0; i < menu->Size(); ++i)
	{
		for (size_t j = 0; j < myPlaylist->Items->Size(); ++j)
		{
			if (myPlaylist->Items->at(j).GetHash() == menu->at(i).GetHash())
			{
				bold = 1;
				break;
			}
		}
		menu->Bold(i, bold);
		bold = 0;
	}
	menu->Refresh();
}

#ifdef HAVE_TAGLIB_H
std::string FindSharedDir(Menu<MPD::Song> *menu)
{
	MPD::SongList list;
	for (size_t i = 0; i < menu->Size(); ++i)
		list.push_back(&(*menu)[i]);
	return FindSharedDir(list);
}

std::string FindSharedDir(const MPD::SongList &v)
{
	if (v.empty()) // this should never happen, but in case...
		FatalError("empty SongList passed to FindSharedDir(const SongList &)!");
	size_t i = -1;
	std::string first = v.front()->GetDirectory();
	for (MPD::SongList::const_iterator it = ++v.begin(); it != v.end(); ++it)
	{
		size_t j = 0;
		std::string dir = (*it)->GetDirectory();
		size_t length = std::min(first.length(), dir.length());
		while (!first.compare(j, 1, dir, j, 1) && j < length && j < i)
			++j;
		i = j;
	}
	return i ? first.substr(0, i) : "/";
}
#endif // HAVE_TAGLIB_H

std::string FindSharedDir(const std::string &one, const std::string &two)
{
	if (one == two)
		return one;
	size_t i = 0;
	while (!one.compare(i, 1, two, i, 1))
		++i;
	i = one.rfind("/", i);
	return i != std::string::npos ? one.substr(0, i) : "/";
}

std::string GetLineValue(std::string &line, char a, char b, bool once)
{
	int pos[2] = { -1, -1 };
	char x = a;
	size_t i = 0;
	while ((i = line.find(x, i)) != std::string::npos && pos[1] < 0)
	{
		if (i && line[i-1] == '\\')
		{
			i++;
			continue;
		}
		if (once)
			line[i] = 0;
		pos[pos[0] >= 0] = i++;
		if (x == a)
			x = b;
	}
	++pos[0];
	std::string result = pos[0] >= 0 && pos[1] >= 0 ? line.substr(pos[0], pos[1]-pos[0]) : "";
	
	// replace \a and \b with a and b respectively
	char r1[] = "\\ ", r2[] = " ";
	r1[1] = r2[0] = a;
	Replace(result, r1, r2);
	if (a != b)
	{
		r1[1] = r2[0] = b;
		Replace(result, r1, r2);
	}
	
	return result;
}

std::string ExtractTopName(const std::string &s)
{
	size_t slash = s.rfind("/");
	return slash != std::string::npos ? s.substr(++slash) : s;
}

std::string PathGoDownOneLevel(const std::string &path)
{
	size_t i = path.rfind('/');
	return i == std::string::npos ? "/" : path.substr(0, i);
}

std::basic_string<my_char_t> Scroller(const std::basic_string<my_char_t> &str, size_t &pos, size_t width)
{
	std::basic_string<my_char_t> s(str);
	if (!Config.header_text_scrolling)
		return s;
	std::basic_string<my_char_t> result;
	size_t len = Window::Length(s);
	
	if (len > width)
	{
		s += U(" ** ");
		len = 0;
		std::basic_string<my_char_t>::const_iterator b = s.begin(), e = s.end();
		for (std::basic_string<my_char_t>::const_iterator it = b+pos; it < e && len < width; ++it)
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

bool SwitchToNextColumn(BasicScreen *screen)
{
	if (screen == myLibrary)
		return myLibrary->NextColumn();
	else if (screen == myPlaylistEditor)
		return myPlaylistEditor->NextColumn();
#	ifdef HAVE_TAGLIB_H
	else if (screen == myTagEditor)
		return myTagEditor->NextColumn();
#	endif // HAVE_TAGLIB_H
	return false;
}

bool SwitchToPrevColumn(BasicScreen *screen)
{
	if (screen == myLibrary)
		return myLibrary->PrevColumn();
	else if (screen == myPlaylistEditor)
		return myPlaylistEditor->PrevColumn();
#	ifdef HAVE_TAGLIB_H
	else if (screen == myTagEditor)
		return myTagEditor->PrevColumn();
#	endif // HAVE_TAGLIB_H
	return false;
}
