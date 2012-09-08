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

#include "utility/wide_string.h"

std::string ToString(const std::wstring &ws)
{
	std::string result;
	char s[MB_CUR_MAX];
	for (size_t i = 0; i < ws.length(); ++i)
	{
		int n = wcrtomb(s, ws[i], 0);
		if (n > 0)
			result.append(s, n);
	}
	return result;
}

std::wstring ToWString(const std::string &s)
{
	std::wstring result;
	wchar_t *ws = new wchar_t[s.length()];
	const char *c_s = s.c_str();
	int n = mbsrtowcs(ws, &c_s, s.length(), 0);
	if (n > 0)
		result.append(ws, n);
	delete [] ws;
	return result;
}

size_t wideLength(const std::wstring &ws)
{
	int len = wcswidth(ws.c_str(), -1);
	if (len < 0)
		return ws.length();
	else
		return len;
}

void wideCut(std::wstring &ws, size_t max_length)
{
	size_t i = 0;
	int remained_len = max_length;
	for (; i < ws.length(); ++i)
	{
		remained_len -= wcwidth(ws[i]);
		if (remained_len < 0)
		{
			ws.resize(i);
			break;
		}
	}
}

std::wstring wideShorten(const std::wstring &ws, size_t max_length)
{
	std::wstring result;
	if (wideLength(ws) > max_length)
	{
		const size_t half_max = max_length/2 - 1;
		size_t len = 0;
		// get beginning of string
		for (auto it = ws.begin(); it != ws.end(); ++it)
		{
			len += wcwidth(*it);
			if (len > half_max)
				break;
			result += *it;
		}
		len = 0;
		std::wstring end;
		// get end of string in reverse order
		for (auto it = ws.rbegin(); it != ws.rend(); ++it)
		{
			len += wcwidth(*it);
			if (len > half_max)
				break;
			end += *it;
		}
		// apply end of string to its beginning
		result += L"..";
		result.append(end.rbegin(), end.rend());
	}
	else
		result = ws;
	return result;
}

