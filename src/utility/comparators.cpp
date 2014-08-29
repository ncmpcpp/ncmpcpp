/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#include <locale>
#include "comparators.h"
#include "utility/string.h"

namespace {//

bool hasTheWord(const std::string &s)
{
	return s.length() >= 4
	&&     (s[0] == 't' || s[0] == 'T')
	&&     (s[1] == 'h' || s[1] == 'H')
	&&     (s[2] == 'e' || s[2] == 'E')
	&&     (s[3] == ' ');
}

}

int LocaleStringComparison::operator()(const std::string &a, const std::string &b) const
{
	const char *ac = a.c_str();
	const char *bc = b.c_str();
	size_t ac_off = 0, bc_off = 0;
	if (m_ignore_the)
	{
		if (hasTheWord(a))
			ac_off += 4;
		if (hasTheWord(b))
			bc_off += 4;
	}
	return std::use_facet<std::collate<char>>(m_locale).compare(
		ac+ac_off, ac+a.length(), bc+bc_off, bc+b.length()
	);
}

bool LocaleBasedItemSorting::operator()(const MPD::Item &a, const MPD::Item &b) const
{
	bool result = false;
	if (a.type == b.type)
	{
		switch (a.type)
		{
			case MPD::itDirectory:
				result = m_cmp(getBasename(a.name), getBasename(b.name));
				break;
			case MPD::itPlaylist:
				result = m_cmp(a.name, b.name);
				break;
			case MPD::itSong:
				switch (m_sort_mode)
				{
					case SortMode::Name:
						result = m_cmp(*a.song, *b.song);
						break;
					case SortMode::ModificationTime:
						result = a.song->getMTime() > b.song->getMTime();
						break;
					case SortMode::CustomFormat:
						result = m_cmp(a.song->toString(Config.browser_sort_format, Config.tags_separator),
						               b.song->toString(Config.browser_sort_format, Config.tags_separator));
						break;
					case SortMode::NoOp:
						throw std::logic_error("can't sort with NoOp sorting mode");
				}
				break;
		}
	}
	else
		result = a.type < b.type;
	return result;
}
