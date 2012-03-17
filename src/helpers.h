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
		int operator()(const std::string &a, const std::string &b);
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
		if (*it == '$')
		{
			if (++it == s.end())
			{
				buf << '$';
				break;
			}
			else if (isdigit(*it))
			{
				buf << Color(*it-'0');
			}
			else
			{
				switch (*it)
				{
					case 'b':
						buf << fmtBold;
						break;
					case 'u':
						buf << fmtUnderline;
						break;
					case 'a':
						buf << fmtAltCharset;
						break;
					case 'r':
						buf << fmtReverse;
						break;
					case '/':
						if (++it == s.end())
						{
							buf << "$/";
							break;
						}
						switch (*it)
						{
							case 'b':
								buf << fmtBoldEnd;
								break;
							case 'u':
								buf << fmtUnderlineEnd;
								break;
							case 'a':
								buf << fmtAltCharsetEnd;
								break;
							case 'r':
								buf << fmtReverseEnd;
								break;
							default:
								buf << '$' << *--it;
								break;
						}
						break;
					default:
						buf << *--it;
						break;
				}
			}
		}
		else if (*it == MPD::Song::FormatEscapeCharacter)
		{
			// treat '$' as a normal character if song format escape char is prepended to it
			if (++it == s.end() || *it != '$')
				--it;
			buf << *it;
		}
		else
			buf << *it;
	}
}

template <typename T> void ShowTime(T &buf, size_t length, bool short_names)
{
	const unsigned MINUTE = 60;
	const unsigned HOUR = 60*MINUTE;
	const unsigned DAY = 24*HOUR;
	const unsigned YEAR = 365*DAY;
	
	unsigned years = length/YEAR;
	if (years)
	{
		buf << years << (short_names ? "y" : (years == 1 ? " year" : " years"));
		length -= years*YEAR;
		if (length)
			buf << ", ";
	}
	unsigned days = length/DAY;
	if (days)
	{
		buf << days << (short_names ? "d" : (days == 1 ? " day" : " days"));
		length -= days*DAY;
		if (length)
			buf << ", ";
	}
	unsigned hours = length/HOUR;
	if (hours)
	{
		buf << hours << (short_names ? "h" : (hours == 1 ? " hour" : " hours"));
		length -= hours*HOUR;
		if (length)
			buf << ", ";
	}
	unsigned minutes = length/MINUTE;
	if (minutes)
	{
		buf << minutes << (short_names ? "m" : (minutes == 1 ? " minute" : " minutes"));
		length -= minutes*MINUTE;
		if (length)
			buf << ", ";
	}
	if (length)
		buf << length << (short_names ? "s" : (length == 1 ? " second" : " seconds"));
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

std::string Timestamp(time_t t);

void UpdateSongList(Menu<MPD::Song> *);

#ifdef HAVE_TAGLIB_H
std::string FindSharedDir(Menu<MPD::Song> *);
std::string FindSharedDir(const MPD::SongList &);
#endif // HAVE_TAGLIB_H
std::string FindSharedDir(const std::string &, const std::string &);
std::string ExtractTopName(const std::string &);
std::string PathGoDownOneLevel(const std::string &path);

std::string GetLineValue(std::string &, char = '"', char = '"', bool = 0);

std::basic_string<my_char_t> Scroller(const std::basic_string<my_char_t> &str, size_t &pos, size_t width);

bool SwitchToNextColumn(BasicScreen *);
bool SwitchToPrevColumn(BasicScreen *);

#endif

