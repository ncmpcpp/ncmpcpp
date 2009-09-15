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

#include "conv.h"
#include "mpdpp.h"
#include "ncmpcpp.h"
#include "settings.h"
#include "status.h"

bool ConnectToMPD();
void ParseArgv(int, char **);

class CaseInsensitiveStringComparison
{
	bool hasTheWord(const std::string &s)
	{
		return  (s.length() > 3)
		&&	(s[0] == 't' || s[0] == 'T')
		&&	(s[1] == 'h' || s[1] == 'H')
		&&	(s[2] == 'e' || s[2] == 'E')
		&&	(s[3] == ' ');
	}
	
	public:
		int operator()(const std::string &a, const std::string &b)
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
};

class CaseInsensitiveSorting
{
	CaseInsensitiveStringComparison cmp;
	
	public:
		bool operator()(const std::string &a, const std::string &b)
		{
			return cmp(a, b) < 0;
		}
		
		bool operator()(MPD::Song *a, MPD::Song *b)
		{
			return cmp(a->GetName(), b->GetName()) < 0;
		}
		
		template <typename A, typename B> bool operator()(const std::pair<A, B> &a, const std::pair<A, B> &b)
		{
			return cmp(a.first, b.first) < 0;
		}
		
		bool operator()(const MPD::Item &, const MPD::Item &);
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

template <typename T> void ShowTag(T &buf, const std::string &tag)
{
	if (tag.empty())
		buf << Config.empty_tags_color << Config.empty_tag << clEnd;
	else
		buf << tag;
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
std::string ExtractTopDirectory(const std::string &);

std::string GetLineValue(std::string &, char = '"', char = '"', bool = 0);

std::basic_string<my_char_t> Scroller(const std::basic_string<my_char_t> &str, size_t &pos, size_t width);

#ifdef HAVE_CURL_CURL_H
size_t write_data(char *, size_t, size_t, void *);
#endif

#endif

