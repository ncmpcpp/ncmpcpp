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

#ifndef _UTILITY_STRING
#define _UTILITY_STRING

#include <string>
#include <vector>

std::vector<std::string> split(const std::string &s, const std::string &delimiter);
void replace(std::string &s, const std::string &from, const std::string &to);

void lowercase(std::string &s);
void lowercase(std::wstring &s);

void trim(std::string &s);

std::string getBasename(const std::string &path);
std::string getParentDirectory(const std::string &path);
std::string getSharedDirectory(const std::string &dir1, const std::string &dir2);

std::string getEnclosedString(const std::string &s, char a, char b, size_t *pos);

bool isInteger(const char *s);

#endif // _UTILITY_STRING
