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

#ifndef NCMPCPP_UTILITY_STRING_H
#define NCMPCPP_UTILITY_STRING_H

#include <cstdarg>
#include <locale>
#include <string>
#include <vector>
#include "gcc.h"

template <size_t N> size_t const_strlen(const char (&)[N]) {
	return N-1;
}

// it's present in boost for std::string, but we want more general version.
template <typename StringT, typename CollectionT>
StringT join(const CollectionT &collection, const StringT &separator)
{
	StringT result;
	auto first = std::begin(collection), last = std::end(collection);
	if (first != last)
	{
		while (true)
		{
			result += *first;
			++first;
			if (first != last)
				result += separator;
			else
				break;
		}
	}
	return result;
}

std::string getBasename(const std::string &path);
std::string getParentDirectory(std::string path);
std::string getSharedDirectory(const std::string &dir1, const std::string &dir2);

std::string getEnclosedString(const std::string &s, char a, char b, size_t *pos);

void removeInvalidCharsFromFilename(std::string &filename, bool win32_compatible);

#endif // NCMPCPP_UTILITY_STRING_H
