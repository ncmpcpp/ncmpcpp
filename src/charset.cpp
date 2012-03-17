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

#include "charset.h"

#ifdef HAVE_ICONV_H

#include <iconv.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "settings.h"

namespace
{
	bool is_utf8(const char *s)
	{
		for (; *s; ++s)
		{
			if (*s & 0x80) // 1xxxxxxx
			{
				char c = 0x40;
				unsigned i = 0;
				while (c & *s)
					++i, c >>= 1;
				if (i < 1 || i > 3) // not 110xxxxx, 1110xxxx, 11110xxx
					return false;
				for (unsigned j = 0; j < i; ++j)
					if (!*++s || !(*s & 0x80) || *s & 0x40) // 10xxxxxx
						return false;
			}
		}
		return true;
	}
	
	bool has_non_ascii_chars(const char *s)
	{
		for (; *s; ++s)
			if (*s & 0x80)
				return true;
		return false;
	}
	
	void charset_convert(const char *from, const char *to, const char *&inbuf, bool delete_old, size_t len = 0)
	{
		if (!inbuf || !from || !to)
			return;
		
		iconv_t cd = iconv_open(to, from);
		
		if (cd == iconv_t(-1))
			return;
		
		if (!len)
			len = strlen(inbuf);
		size_t buflen = len*6+1;
		char *outbuf = new char[buflen];
		char *outstart = outbuf;
		const char *instart = inbuf;
		
		if (iconv(cd, const_cast<ICONV_CONST char **>(&inbuf), &len, &outbuf, &buflen) == size_t(-1))
		{
			inbuf = instart;
			delete [] outstart;
			iconv_close(cd);
			return;
		}
		iconv_close(cd);
		*outbuf = 0;
		if (delete_old)
			free(const_cast<char *>(instart));
		inbuf = outstart;
	}
}

void iconv_convert_from_to(const char *from, const char *to, std::string &s)
{
	const char *tmp = strdup(s.c_str());
	charset_convert(from, to, tmp, true, s.length());
	s = tmp;
	free(const_cast<char *>(tmp));
}

void utf_to_locale(std::string &s)
{
	if (s.empty() || Config.system_encoding.empty() || !has_non_ascii_chars(s.c_str()))
		return;
	const char *tmp = strdup(s.c_str());
	charset_convert("utf-8", Config.system_encoding.c_str(), tmp, 1, s.length());
	s = tmp;
	free(const_cast<char *>(tmp));
}

std::string utf_to_locale_cpy(const std::string &s)
{
	std::string result = s;
	utf_to_locale(result);
	return result;
}

void locale_to_utf(std::string &s)
{
	if (s.empty() || Config.system_encoding.empty() || !has_non_ascii_chars(s.c_str()) || is_utf8(s.c_str()))
		return;
	const char *tmp = strdup(s.c_str());
	charset_convert(Config.system_encoding.c_str(), "utf-8", tmp, 1, s.length());
	s = tmp;
	free(const_cast<char *>(tmp));
}

std::string locale_to_utf_cpy(const std::string &s)
{
	std::string result = s;
	locale_to_utf(result);
	return result;
}

void utf_to_locale(const char *&s, bool delete_old)
{
	if (!s || Config.system_encoding.empty() || !has_non_ascii_chars(s))
		return;
	charset_convert("utf-8", Config.system_encoding.c_str(), s, delete_old);
}

void locale_to_utf(const char *&s, bool delete_old)
{
	if (!s || Config.system_encoding.empty() || !has_non_ascii_chars(s) || is_utf8(s))
		return;
	charset_convert(Config.system_encoding.c_str(), "utf-8", s, delete_old);
}

#endif // HAVE_ICONV_H

