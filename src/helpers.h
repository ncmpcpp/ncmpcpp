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

#ifndef _HELPERS_H
#define _HELPERS_H

#include "misc.h"
#include "mpdpp.h"
#include "ncmpcpp.h"
#include "settings.h"
#include "status.h"

bool ConnectToMPD();
void ParseArgv(int, char **);

class CaseInsensitiveSorting
{
	public:
		bool operator()(std::string, std::string);
		bool operator()(MPD::Song *, MPD::Song *);
		bool operator()(const MPD::Item &, const MPD::Item &);
		
		template <typename A, typename B> bool operator()(const std::pair<A, B> &a, const std::pair<A, B> &b)
		{
			std::string aa = a.first;
			std::string bb = b.first;
			ToLower(aa);
			ToLower(bb);
			return aa < bb;
		}
};

template <typename A, typename B> std::string StringPairToString(const std::pair<A, B> &pair, void *)
{
	return pair.first;
}

template <typename C> void String2Buffer(const std::basic_string<C> &s, basic_buffer<C> &buf)
{
	for (typename std::basic_string<C>::const_iterator it = s.begin(); it != s.end(); ++it)
	{
		if (*it != '$')
			buf << *it;
		else if (isdigit(*++it))
			buf << Color(*it-'0');
		else
		{
			switch (*it)
			{
				case 'b':
					buf << fmtBold;
					break;
				case 'a':
					buf << fmtAltCharset;
					break;
				case 'r':
					buf << fmtReverse;
					break;
				case '/':
					switch (*++it)
					{
						case 'b':
							buf << fmtBoldEnd;
							break;
						case 'a':
							buf << fmtAltCharsetEnd;
							break;
						case 'r':
							buf << fmtReverseEnd;
							break;
					}
					break;
				default:
					buf << *it;
					break;
			}
		}
	}
}

inline bool Keypressed(int in, const int *key)
{
	return in == key[0] || in == key[1];
}

void UpdateSongList(Menu<MPD::Song> *);

#ifdef HAVE_TAGLIB_H
std::string FindSharedDir(Menu<MPD::Song> *);
std::string FindSharedDir(const MPD::SongList &);
#endif // HAVE_TAGLIB_H
std::string FindSharedDir(const std::string &, const std::string &);

std::string GetLineValue(std::string &, char = '"', char = '"', bool = 0);

void RemoveTheWord(std::string &s);
std::string ExtractTopDirectory(const std::string &);

Buffer ShowTag(const std::string &);

#ifdef _UTF8
std::basic_string<my_char_t> Scroller(const std::string &, size_t, size_t &);
#endif // _UTF8
std::basic_string<my_char_t> Scroller(const std::basic_string<my_char_t> &, size_t, size_t &);

#ifdef HAVE_CURL_CURL_H
size_t write_data(char *, size_t, size_t, void *);
#endif

#endif

