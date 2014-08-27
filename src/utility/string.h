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

std::string getBasename(const std::string &path);
std::string getParentDirectory(const std::string &path);
std::string getSharedDirectory(const std::string &dir1, const std::string &dir2);

std::string getEnclosedString(const std::string &s, char a, char b, size_t *pos);

void removeInvalidCharsFromFilename(std::string &filename, bool win32_compatible);

#endif // NCMPCPP_UTILITY_STRING_H
