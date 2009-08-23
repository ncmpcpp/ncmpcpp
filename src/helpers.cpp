/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include "charset.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "status.h"
#include "tag_editor.h"

using namespace MPD;

bool ConnectToMPD()
{
	if (!Mpd.Connect())
	{
		std::cout << "Cannot connect to mpd: " << Mpd.GetErrorMessage() << std::endl;
		return false;
	}
	return true;
}

void ParseArgv(int argc, char **argv)
{
	bool quit = 0;
	
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
			std::cout << "ncmpcpp version: " << VERSION << std::endl
			<< "built with support for:"
#			ifdef HAVE_CURL_CURL_H
			<< " curl"
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
			<< "  -?, --help                show this help message\n"
			<< "  -v, --version             display version information\n\n"
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
			exit(0);
		
		if (!strcmp(argv[i], "play"))
		{
			Mpd.Play();
			quit = 1;
		}
		else if (!strcmp(argv[i], "pause"))
		{
			Mpd.Execute("pause \"1\"\n");
			quit = 1;
		}
		else if (!strcmp(argv[i], "toggle"))
		{
			Mpd.UpdateStatus();
			switch (Mpd.GetState())
			{
				case psPause:
				case psPlay:
					Mpd.Pause();
					break;
				case psStop:
					Mpd.Play();
					break;
				default:
					break;
			}
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
				exit(0);
			}
			if (i != argc)
				Mpd.SetVolume(Mpd.GetVolume()+atoi(argv[i]));
			quit = 1;
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

bool CaseInsensitiveSorting::operator()(std::string a, std::string b)
{
	ToLower(a);
	ToLower(b);
	if (Config.ignore_leading_the)
	{
		RemoveTheWord(a);
		RemoveTheWord(b);
	}
	return a < b;
}

bool CaseInsensitiveSorting::operator()(Song *sa, Song *sb)
{
	std::string a = sa->GetName();
	std::string b = sb->GetName();
	ToLower(a);
	ToLower(b);
	if (Config.ignore_leading_the)
	{
		RemoveTheWord(a);
		RemoveTheWord(b);
	}
	return a < b;
}

bool CaseInsensitiveSorting::operator()(const Item &a, const Item &b)
{
	if (a.type == b.type)
	{
		switch (a.type)
		{
			case itDirectory:
				return operator()(ExtractTopDirectory(a.name), ExtractTopDirectory(b.name));
			case itPlaylist:
				return operator()(a.name, b.name);
			case itSong:
				return operator()(a.song, b.song);
			default: // there's no other type, just silence compiler.
				return 0;
		}
	}
	else
		return a.type < b.type;
}

void UpdateSongList(Menu<Song> *menu)
{
	bool bold = 0;
	for (size_t i = 0; i < menu->Size(); ++i)
	{
		for (size_t j = 0; j < myPlaylist->Main()->Size(); ++j)
		{
			if (myPlaylist->Main()->at(j).GetHash() == menu->at(i).GetHash())
			{
				bold = 1;
				break;
			}
		}
		menu->BoldOption(i, bold);
		bold = 0;
	}
	menu->Refresh();
}

#ifdef HAVE_TAGLIB_H
std::string FindSharedDir(Menu<Song> *menu)
{
	SongList list;
	for (size_t i = 0; i < menu->Size(); ++i)
		list.push_back(&menu->at(i));
	return FindSharedDir(list);
}

std::string FindSharedDir(const SongList &v)
{
	std::string result;
	if (!v.empty())
	{
		result = v.front()->GetFile();
		for (SongList::const_iterator it = v.begin()+1; it != v.end(); ++it)
		{
			int i = 1;
			while (result.substr(0, i) == (*it)->GetFile().substr(0, i))
				i++;
			result = result.substr(0, i);
		}
		size_t slash = result.rfind("/");
		result = slash != std::string::npos ? result.substr(0, slash) : "/";
	}
	return result;
}
#endif // HAVE_TAGLIB_H

std::string FindSharedDir(const std::string &one, const std::string &two)
{
	if (one == two)
		return one;
	std::string result;
	size_t i = 1;
	while (one.substr(0, i) == two.substr(0, i))
		i++;
	result = one.substr(0, i);
	i = result.rfind("/");
	return i != std::string::npos ? result.substr(0, i) : "/";
}

std::string GetLineValue(std::string &line, char a, char b, bool once)
{
	int pos[2] = { -1, -1 };
	size_t i;
	for (i = line.find(a); i != std::string::npos && pos[1] < 0; i = line.find(b, i))
	{
		if (i && line[i-1] == '\\')
		{
			i++;
			continue;
		}
		if (once)
			line[i] = 0;
		pos[pos[0] >= 0] = i++;
	}
	pos[0]++;
	std::string result = pos[0] >= 0 && pos[1] >= 0 ? line.substr(pos[0], pos[1]-pos[0]) : "";
	for (i = result.find("\\\""); i != std::string::npos; i = result.find("\\\""))
		result.replace(i, 2, "\"");
	return result;
}

void RemoveTheWord(std::string &s)
{
	size_t the_pos = s.find("the ");
	if (the_pos == 0 && the_pos != std::string::npos)
		s = s.substr(4);
}

std::string ExtractTopDirectory(const std::string &s)
{
	size_t slash = s.rfind("/");
	return slash != std::string::npos ? s.substr(++slash) : s;
}

Buffer ShowTag(const std::string &tag)
{
	Buffer result;
	if (tag.empty())
		result << Config.empty_tags_color << Config.empty_tag << clEnd;
	else
		result << tag;
	return result;
}

#ifdef _UTF8
std::basic_string<my_char_t> Scroller(const std::string &str, size_t width, size_t &pos)
{
	return Scroller(TO_WSTRING(str), width, pos);
}
#endif // _UTF8

std::basic_string<my_char_t> Scroller(const std::basic_string<my_char_t> &str, size_t width, size_t &pos)
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
#			ifdef _UTF8
			len += wcwidth(*it);
#			else
			len++;
#			endif
			result += *it;
		}
		if (++pos >= s.length())
			pos = 0;
		for (; len < width; ++b)
		{
#			ifdef _UTF8
			len += wcwidth(*b);
#			else
			len++;
#			endif
			result += *b;
		}
	}
	else
		result = s;
	return result;
}

#ifdef HAVE_CURL_CURL_H
size_t write_data(char *buffer, size_t size, size_t nmemb, void *data)
{
	size_t result = size*nmemb;
	static_cast<std::string *>(data)->append(buffer, result);
	return result;
}
#endif // HAVE_CURL_CURL_H

