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

#ifndef _UTILITY_COMPARATORS
#define _UTILITY_COMPARATORS

#include <string>
#include "mpdpp.h"

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
	
	bool operator()(const MPD::Song &a, const MPD::Song &b)
	{
		return cmp(a.getName(), b.getName()) < 0;
	}
	
	template <typename A, typename B> bool operator()(const std::pair<A, B> &a, const std::pair<A, B> &b)
	{
		return cmp(a.first, b.first) < 0;
	}
	
	bool operator()(const MPD::Item &a, const MPD::Item &b);
};

#endif // _UTILITY_COMPARATORS