/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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
#include "format_impl.h"
#include "utility/string.h"

namespace {

bool hasTheWord(const std::string &s)
{
	return s.length() >= 4
	&&     (s[0] == 't' || s[0] == 'T')
	&&     (s[1] == 'h' || s[1] == 'H')
	&&     (s[2] == 'e' || s[2] == 'E')
	&&     (s[3] == ' ');
}

}

int LocaleStringComparison::compare(const char *a, size_t a_len, const char *b, size_t b_len) const
{
	size_t ac_off = 0, bc_off = 0;
	if (m_ignore_the)
	{
		if (hasTheWord(a))
			ac_off += 4;
		if (hasTheWord(b))
			bc_off += 4;
	}
	return std::use_facet<std::collate<char>>(m_locale).compare(
		a+ac_off, a+a_len, b+bc_off, b+b_len
	);
}

bool LocaleBasedItemSorting::operator()(const MPD::Item &a, const MPD::Item &b) const
{
	bool result = false;
	if (a.type() == b.type())
	{
		switch (m_sort_mode)
		{
			case SortMode::Name:
				switch (a.type())
				{
					case MPD::Item::Type::Directory:
						result = m_cmp(a.directory().path(), b.directory().path());
						break;
					case MPD::Item::Type::Playlist:
						result = m_cmp(a.playlist().path(), b.playlist().path());
						break;
					case MPD::Item::Type::Song:
						result = m_cmp(a.song(), b.song());
						break;
				}
				break;
			case SortMode::CustomFormat:
				switch (a.type())
				{
					case MPD::Item::Type::Directory:
						result = m_cmp(a.directory().path(), b.directory().path());
						break;
					case MPD::Item::Type::Playlist:
						result = m_cmp(a.playlist().path(), b.playlist().path());
						break;
					case MPD::Item::Type::Song:
						result = m_cmp(Format::stringify<char>(Config.browser_sort_format, &a.song()),
						               Format::stringify<char>(Config.browser_sort_format, &b.song()));
						break;
				}
				break;
			case SortMode::ModificationTime:
				switch (a.type())
				{
					case MPD::Item::Type::Directory:
						result = a.directory().lastModified() > b.directory().lastModified();
						break;
					case MPD::Item::Type::Playlist:
						result = a.playlist().lastModified() > b.playlist().lastModified();
						break;
					case MPD::Item::Type::Song:
						result = a.song().getMTime() > b.song().getMTime();
						break;
				}
				break;
			case SortMode::NoOp:
				throw std::logic_error("can't sort with NoOp sorting mode");
		}
	}
	else
		result = a.type() < b.type();
	return result;
}
