/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include "charset.h"

#if defined(SUPPORTED_LOCALES) && defined(HAVE_ICONV_H)

#include <iconv.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "locale.h"
#include "str_pool.h"

namespace
{
	char *locale_charset = 0;

	inline bool char_non_ascii(char ch)
	{
		return (ch & 0x80) != 0;
	}
	
	bool has_non_ascii_chars(const char *s)
	{
		for (; s; s++)
			if (char_non_ascii(*s))
				return true;
		return false;
	}
	
	bool has_non_ascii_chars(const std::string &s)
	{
		for (std::string::const_iterator it = s.begin(); it != s.end(); it++)
			if (char_non_ascii(*it))
				return true;
		return false;
	}
	
	void charset_convert(const char *from, const char *to, char *&inbuf, size_t len = 0)
	{
		if (!inbuf || !from || !to)
			return;
		
		iconv_t cd = iconv_open(to, from);
		
		if (cd == (iconv_t)-1)
			return;
		
		if (!len)
			len = strlen(inbuf);
		size_t buflen = len*6+1;
		char *outbuf = new char[buflen];
		memset(outbuf, 0, sizeof(char)*buflen);
		char *outstart = outbuf;
		char *instart = inbuf;
		
		if (iconv(cd, &inbuf, &len, &outbuf, &buflen) == (size_t)-1)
		{
			delete [] outstart;
			iconv_close(cd);
			return;
		}
		iconv_close(cd);
		
		str_pool_put(instart);
		inbuf = str_pool_get(outstart);
		delete [] outstart;
	}
}

void init_current_locale()
{
	if (!setlocale(LC_CTYPE, ""))
		return;
	std::string envlocale = setlocale(LC_CTYPE, "");
	if (envlocale.empty() || envlocale == "C")
		return;
	std::ifstream f(SUPPORTED_LOCALES);
	if (!f.is_open())
	{
		std::cerr << "ncmpcpp: cannot open file "SUPPORTED_LOCALES"!\n";
		return;
	}
	envlocale += " ";
	std::string line;
	while (!f.eof())
	{
		getline(f, line);
		if (line.find(envlocale) != std::string::npos)
		{
			try
			{
				std::string charset = line.substr(line.find(" ")+1);
				if (charset == "UTF-8"
				||  charset == "utf-8"
				||  charset == "utf8")
				{
					f.close();
					return;
				}
				locale_charset = strdup((charset + "//TRANSLIT").c_str());
			}
			catch (std::out_of_range)
			{
				f.close();
				return;
			}
			break;
		}
	}
	f.close();
}

void utf_to_locale(std::string &s)
{
	if (s.empty() || !locale_charset || !has_non_ascii_chars(s))
		return;
	char *tmp = str_pool_get(s.c_str());
	charset_convert("utf8", locale_charset, tmp, s.length());
	s = tmp;
	str_pool_put(tmp);
}

std::string utf_to_locale_cpy(const std::string &s)
{
	std::string result = s;
	utf_to_locale(result);
	return result;
}

void locale_to_utf(std::string &s)
{
	if (s.empty() || !locale_charset || !has_non_ascii_chars(s))
		return;
	char *tmp = str_pool_get(s.c_str());
	charset_convert(locale_charset, "utf8", tmp, s.length());
	s = tmp;
	str_pool_put(tmp);
}

std::string locale_to_utf_cpy(const std::string &s)
{
	std::string result = s;
	locale_to_utf(result);
	return result;
}

void str_pool_utf_to_locale(char *&s)
{
	if (!s || !locale_charset || !has_non_ascii_chars(s))
		return;
	charset_convert("utf8", locale_charset, s);
}

void str_pool_locale_to_utf(char *&s)
{
	if (!s || !locale_charset || !has_non_ascii_chars(s))
		return;
	charset_convert(locale_charset, "utf8", s);
}

#endif // SUPPORTED_LOCALES && HAVE_ICONV_H

