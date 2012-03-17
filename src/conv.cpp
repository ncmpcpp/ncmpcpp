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

#include <algorithm>
#include <sstream>

#include "conv.h"

int StrToInt(const std::string &str)
{
	return atoi(str.c_str());
}

long StrToLong(const std::string &str)
{
	return atol(str.c_str());
}

std::string IntoStr(int l)
{
	std::ostringstream ss;
	ss << l;
	return ss.str();
}

std::string IntoStr(mpd_tag_type tag) // this is only for left column's title in media library
{
	switch (tag)
	{
		case MPD_TAG_ARTIST:
			return "Artist";
		case MPD_TAG_ALBUM:
			return "Album";
		case MPD_TAG_ALBUM_ARTIST:
			return "Album Artist";
		case MPD_TAG_TITLE:
			return "Title";
		case MPD_TAG_TRACK:
			return "Track";
		case MPD_TAG_GENRE:
			return "Genre";
		case MPD_TAG_DATE:
			return "Year";
		case MPD_TAG_COMPOSER:
			return "Composer";
		case MPD_TAG_PERFORMER:
			return "Performer";
		case MPD_TAG_COMMENT:
			return "Comment";
		case MPD_TAG_DISC:
			return "Disc";
		default:
			return "";
	}
}

std::string IntoStr(NCurses::Color color)
{
	std::string result;
	
	if (color == NCurses::clDefault)
		result = "default";
	else if (color == NCurses::clBlack)
		result = "black";
	else if (color == NCurses::clRed)
		result = "red";
	else if (color == NCurses::clGreen)
		result = "green";
	else if (color == NCurses::clYellow)
		result = "yellow";
	else if (color == NCurses::clBlue)
		result = "blue";
	else if (color == NCurses::clMagenta)
		result = "magenta";
	else if (color == NCurses::clCyan)
		result = "cyan";
	else if (color == NCurses::clWhite)
		result = "white";
	
	return result;
}

NCurses::Color IntoColor(const std::string &color)
{
	NCurses::Color result = NCurses::clDefault;
	
	if (color == "black")
		result = NCurses::clBlack;
	else if (color == "red")
		result = NCurses::clRed;
	else if (color == "green")
		result = NCurses::clGreen;
	else if (color == "yellow")
		result = NCurses::clYellow;
	else if (color == "blue")
		result = NCurses::clBlue;
	else if (color == "magenta")
		result = NCurses::clMagenta;
	else if (color == "cyan")
		result = NCurses::clCyan;
	else if (color == "white")
		result = NCurses::clWhite;
	
	return result;
}

mpd_tag_type IntoTagItem(char c)
{
	switch (c)
	{
		case 'a':
			return MPD_TAG_ARTIST;
		case 'A':
			return MPD_TAG_ALBUM_ARTIST;
		case 'b':
			return MPD_TAG_ALBUM;
		case 'y':
			return MPD_TAG_DATE;
		case 'g':
			return MPD_TAG_GENRE;
		case 'c':
			return MPD_TAG_COMPOSER;
		case 'p':
			return MPD_TAG_PERFORMER;
		default:
			return MPD_TAG_ARTIST;
	}
}

MPD::Song::GetFunction toGetFunction(char c)
{
	switch (c)
	{
		case 'l':
			return &MPD::Song::GetLength;
		case 'D':
			return &MPD::Song::GetDirectory;
		case 'f':
			return &MPD::Song::GetName;
		case 'a':
			return &MPD::Song::GetArtist;
		case 'A':
			return &MPD::Song::GetAlbumArtist;
		case 'b':
			return &MPD::Song::GetAlbum;
		case 'y':
			return &MPD::Song::GetDate;
		case 'n':
			return &MPD::Song::GetTrackNumber;
		case 'N':
			return &MPD::Song::GetTrack;
		case 'g':
			return &MPD::Song::GetGenre;
		case 'c':
			return &MPD::Song::GetComposer;
		case 'p':
			return &MPD::Song::GetPerformer;
		case 'd':
			return &MPD::Song::GetDisc;
		case 'C':
			return &MPD::Song::GetComment;
		case 't':
			return &MPD::Song::GetTitle;
		default:
			return 0;
	}
}

#ifdef HAVE_TAGLIB_H
MPD::Song::SetFunction IntoSetFunction(mpd_tag_type tag)
{
	switch (tag)
	{
		case MPD_TAG_ARTIST:
			return &MPD::Song::SetArtist;
		case MPD_TAG_ALBUM:
			return &MPD::Song::SetAlbum;
		case MPD_TAG_ALBUM_ARTIST:
			return &MPD::Song::SetAlbumArtist;
		case MPD_TAG_TITLE:
			return &MPD::Song::SetTitle;
		case MPD_TAG_TRACK:
			return &MPD::Song::SetTrack;
		case MPD_TAG_GENRE:
			return &MPD::Song::SetGenre;
		case MPD_TAG_DATE:
			return &MPD::Song::SetDate;
		case MPD_TAG_COMPOSER:
			return &MPD::Song::SetComposer;
		case MPD_TAG_PERFORMER:
			return &MPD::Song::SetPerformer;
		case MPD_TAG_COMMENT:
			return &MPD::Song::SetComment;
		case MPD_TAG_DISC:
			return &MPD::Song::SetDisc;
		default:
			return 0;
	}
}
#endif // HAVE_TAGLIB_H

std::string Shorten(const std::basic_string<my_char_t> &s, size_t max_length)
{
	if (s.length() <= max_length)
		return TO_STRING(s);
	if (max_length < 2)
		return "";
	std::basic_string<my_char_t> result(s, 0, max_length/2-!(max_length%2));
	result += U("..");
	result += s.substr(s.length()-max_length/2+1);
	return TO_STRING(result);
}

void EscapeUnallowedChars(std::string &s)
{
	static const std::string unallowed_chars = "\"*/:<>?\\|";
	
	for (std::string::const_iterator it = unallowed_chars.begin(); it != unallowed_chars.end(); ++it)
	{
		for (size_t i = 0; i < s.length(); ++i)
		{
			if (s[i] == *it)
			{
				s.erase(s.begin()+i);
				i--;
			}
		}
	}
}

std::string unescapeHtmlUtf8(const std::string &data)
{
	std::string result;
	for (size_t i = 0, j; i < data.length(); ++i)
	{
		if (data[i] == '&' && data[i+1] == '#' && (j = data.find(';', i)) != std::string::npos)
		{
			int n = atoi(&data.c_str()[i+2]);
			if (n >= 0x800)
			{
				result += (0xe0 | ((n >> 12) & 0x0f));
				result += (0x80 | ((n >> 6) & 0x3f));
				result += (0x80 | (n & 0x3f));
			}
			else if (n >= 0x80)
			{
				result += (0xc0 | ((n >> 6) & 0x1f));
				result += (0x80 | (n & 0x3f));
			}
			else
				result += n;
			i = j;
		}
		else
			result += data[i];
	}
	return result;
}

void StripHtmlTags(std::string &s)
{
	bool erase = 0;
	for (size_t i = s.find("<"); i != std::string::npos; i = s.find("<"))
	{
		size_t j = s.find(">", i)+1;
		s.replace(i, j-i, "");
	}
	Replace(s, "&#039;", "'");
	Replace(s, "&amp;", "&");
	Replace(s, "&quot;", "\"");
	Replace(s, "&nbsp;", " ");
	for (size_t i = 0; i < s.length(); ++i)
	{
		if (erase)
		{
			s.erase(s.begin()+i);
			erase = 0;
		}
		if (s[i] == 13) // ascii code for windows line ending, get rid of this shit
		{
			s[i] = '\n';
			erase = 1;
		}
		else if (s[i] == '\t')
			s[i] = ' ';
	}
}

void Trim(std::string &s)
{
	if (s.empty())
		return;
	
	size_t b = 0;
	size_t e = s.length()-1;
	
	while (s[e] == ' ' || s[e] == '\n')
		--e;
	++e;
	if (e != s.length())
		s.resize(e);
	
	while (s[b] == ' ' || s[b] == '\n')
		++b;
	if (b != 0)
		 s = s.substr(b);
}

