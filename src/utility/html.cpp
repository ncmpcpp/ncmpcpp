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

#include <boost/algorithm/string/replace.hpp>
#include "utility/html.h"
//#include "utility/string.h"

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

void stripHtmlTags(std::string &s)
{
	bool erase = 0;
	for (size_t i = s.find("<"); i != std::string::npos; i = s.find("<"))
	{
		size_t j = s.find(">", i)+1;
		s.replace(i, j-i, "");
	}
	boost::replace_all(s, "&#039;", "'");
	boost::replace_all(s, "&amp;", "&");
	boost::replace_all(s, "&quot;", "\"");
	boost::replace_all(s, "&nbsp;", " ");
	for (size_t i = 0; i < s.length(); ++i)
	{
		if (erase)
		{
			s.erase(s.begin()+i);
			erase = 0;
		}
		if (s[i] == 13) // ascii code for windows line ending, get rid of this shit
		{
			s[i] = '\n';
			erase = 1;
		}
		else if (s[i] == '\t')
			s[i] = ' ';
	}
}