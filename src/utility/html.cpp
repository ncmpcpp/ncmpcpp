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

#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include "utility/html.h"

std::string unescapeHtmlUtf8(const std::string &data)
{
	std::string result;
	for (size_t i = 0, j; i < data.length(); ++i)
	{
		if (data[i] == '&' && data[i+1] == '#' && (j = data.find(';', i)) != std::string::npos)
		{
			int n = atoi(&data.c_str()[i+2]);
			if (n >= 0x800)
			{
				result += (0xe0 | ((n >> 12) & 0x0f));
				result += (0x80 | ((n >> 6) & 0x3f));
				result += (0x80 | (n & 0x3f));
			}
			else if (n >= 0x80)
			{
				result += (0xc0 | ((n >> 6) & 0x1f));
				result += (0x80 | (n & 0x3f));
			}
			else
				result += n;
			i = j;
		}
		else
			result += data[i];
	}
	return result;
}

void unescapeHtmlEntities(std::string &s)
{
	// well, at least some of them.
	boost::replace_all(s, "&amp;", "&");
	boost::replace_all(s, "&gt;", ">");
	boost::replace_all(s, "&lt;", "<");
	boost::replace_all(s, "&nbsp;", " ");
	boost::replace_all(s, "&quot;", "\"");
	boost::replace_all(s, "&ndash;", "–");
	boost::replace_all(s, "&mdash;", "—");
}

void stripHtmlTags(std::string &s)
{
	// Erase newlines so they don't duplicate with HTML ones.
	s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
				return c == '\n' || c == '\r';
			}), s.end());

	bool is_newline;
	for (size_t i = s.find("<"); i != std::string::npos; i = s.find("<"))
	{
		size_t j = s.find(">", i);
		if (j != std::string::npos)
		{
			++j;
			is_newline
				=  s.compare(i, std::min<size_t>(3, j-i), "<p ") == 0
				|| s.compare(i, j-i, "<p>") == 0
				|| s.compare(i, j-i, "</p>") == 0
				|| s.compare(i, j-i, "<br>") == 0
				|| s.compare(i, j-i, "<br/>") == 0
				|| s.compare(i, std::min<size_t>(4, j-i), "<br ") == 0;
			if (is_newline)
				s.replace(i, j-i, "\n");
			else
				s.replace(i, j-i, "");
		}
		else
			break;
	}
	unescapeHtmlEntities(s);
}
