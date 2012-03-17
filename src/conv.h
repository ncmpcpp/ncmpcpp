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

#ifndef _CONV_H
#define _CONV_H

#include <cstring>
#include <string>

#include "window.h"
#include "song.h"

template <size_t N> inline size_t static_strlen(const char (&)[N])
{
	return N-1;
}

template <size_t N> void Replace(std::string &s, const char (&from)[N], const char *to)
{
	size_t to_len = strlen(to);
	for (size_t i = 0; (i = s.find(from, i)) != std::string::npos; i += to_len)
		s.replace(i, N-1, to);
}

int StrToInt(const std::string &);
long StrToLong(const std::string &);

std::string IntoStr(int);

std::string IntoStr(mpd_tag_type);

std::string IntoStr(NCurses::Color);

NCurses::Color IntoColor(const std::string &);

mpd_tag_type IntoTagItem(char);

MPD::Song::GetFunction toGetFunction(char c);

#ifdef HAVE_TAGLIB_H
MPD::Song::SetFunction IntoSetFunction(mpd_tag_type);
#endif // HAVE_TAGLIB_H

std::string Shorten(const std::basic_string<my_char_t> &s, size_t max_length);

void EscapeUnallowedChars(std::string &);

std::string unescapeHtmlUtf8(const std::string &data);

void StripHtmlTags(std::string &s);

void Trim(std::string &s);

#endif

