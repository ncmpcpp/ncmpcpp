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

#include "comparators.h"
#include "settings.h"

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
	bool result = false;
	if (a.type == b.type)
	{
		switch (a.type)
		{
			case MPD::itDirectory:
				result = cmp(getBasename(a.name), getBasename(b.name)) < 0;
				break;
			case MPD::itPlaylist:
				result = cmp(a.name, b.name) < 0;
				break;
			case MPD::itSong:
				switch (Config.browser_sort_mode)
				{
					case smName:
						result = operator()(*a.song, *b.song);
						break;
					case smMTime:
						result = a.song->getMTime() > b.song->getMTime();
						break;
					case smCustomFormat:
						result = cmp(a.song->toString(Config.browser_sort_format), b.song->toString(Config.browser_sort_format)) < 0;
						break;
				}
				break;
			default: // there is no other option, silence compiler
				assert(false);
		}
	}
	else
		result = a.type < b.type;
	return result;
}
