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

bool CaseInsensitiveStringComparison::hasTheWord(const char *s) const
{
	return (s[0] == 't' || s[0] == 'T')
	&&     (s[1] == 'h' || s[1] == 'H')
	&&     (s[2] == 'e' || s[2] == 'E')
	&&     (s[3] == ' ');
}

int CaseInsensitiveStringComparison::operator()(const char *a, const char *b) const
{
	if (m_ignore_the)
	{
		if (hasTheWord(a))
			a += 4;
		if (hasTheWord(b))
			b += 4;
	}
	int dist;
	while (!(dist = tolower(*a)-tolower(*b)) && *b)
		++a, ++b;
	return dist;
}

CaseInsensitiveSorting::CaseInsensitiveSorting(): cmp(Config.ignore_leading_the) { }

bool CaseInsensitiveSorting::operator()(const MPD::Item &a, const MPD::Item &b) const
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
		}
	}
	else
		result = a.type < b.type;
	return result;
}
