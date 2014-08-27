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

#include <cassert>
#include <cwctype>
#include <algorithm>
#include "utility/string.h"

std::string getBasename(const std::string &path)
{
	size_t slash = path.rfind("/");
	if (slash == std::string::npos)
		return path;
	else
		return path.substr(slash+1);
}

std::string getParentDirectory(const std::string &path)
{
	size_t slash = path.rfind('/');
	if (slash == std::string::npos)
		return "/";
	else
		return path.substr(0, slash);
}

std::string getSharedDirectory(const std::string &dir1, const std::string &dir2)
{
	size_t i = 0;
	size_t min_len = std::min(dir1.length(), dir2.length());
	while (i < min_len && !dir1.compare(i, 1, dir2, i, 1))
		++i;
	i = dir1.rfind("/", i);
	if (i == std::string::npos)
		return "/";
	else
		return dir1.substr(0, i);
}

std::string getEnclosedString(const std::string &s, char a, char b, size_t *pos)
{
	std::string result;
	size_t i = s.find(a, pos ? *pos : 0);
	if (i != std::string::npos)
	{
		++i;
		while (i < s.length() && s[i] != b)
		{
			if (s[i] == '\\' && i+1 < s.length() && (s[i+1] == '\\' || s[i+1] == b))
				result += s[++i];
			else
				result += s[i];
			++i;
		}
		// we want to set pos to char after b if possible
		if (i < s.length())
			++i;
		else // we reached end of string and didn't encounter closing char
			result.clear();
	}
	if (pos != 0)
		*pos = i;
	return result;
}

void removeInvalidCharsFromFilename(std::string &filename, bool win32_compatible)
{
	const char *unallowed_chars = win32_compatible ? "\"*/:<>?\\|" : "/";
	for (const char *c = unallowed_chars; *c; ++c)
	{
		for (size_t i = 0; i < filename.length(); ++i)
		{
			if (filename[i] == *c)
			{
				filename.erase(filename.begin()+i);
				--i;
			}
		}
	}
}
