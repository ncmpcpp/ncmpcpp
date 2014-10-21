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

#include <boost/locale/encoding.hpp>
#include <cassert>
#include "utility/wide_string.h"

std::string ToString(std::wstring ws)
{
	return boost::locale::conv::utf_to_utf<char>(std::move(ws));
}

std::wstring ToWString(std::string s)
{
	return boost::locale::conv::utf_to_utf<wchar_t>(std::move(s));
}

size_t wideLength(const std::wstring &ws)
{
	size_t result = 0;
	for (const auto &wc : ws)
	{
		int len = wcwidth(wc);
		if (len < 0)
			++result;
		else
			result += len;
	}
	return result;
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

