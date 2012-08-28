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

#include <cstdio>
#include <cwchar>
#include <string>

#ifndef _NUMERIC_CONVERSIONS_H
#define _NUMERIC_CONVERSIONS_H

template <typename R> struct intTo { };
template <> struct intTo<std::string> {
	static std::string apply(int n) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", n);
		return buf;
	}
};
template <> struct intTo<std::wstring> {
	static std::wstring apply(int n) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%d", n);
		return buf;
	}
};

template <typename R> struct longIntTo { };
template <> struct longIntTo<std::string> {
	static std::string apply(long int n) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%ld", n);
		return buf;
	}
};
template <> struct longIntTo<std::wstring> {
	static std::wstring apply(long int n) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%ld", n);
		return buf;
	}
};

template <typename R> struct unsignedIntTo { };
template <> struct unsignedIntTo<std::string> {
	static std::string apply(unsigned int n) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%u", n);
		return buf;
	}
};
template <> struct unsignedIntTo<std::wstring> {
	static std::wstring apply(unsigned int n) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%u", n);
		return buf;
	}
};

template <typename R> struct unsignedLongIntTo { };
template <> struct unsignedLongIntTo<std::string> {
	static std::string apply(unsigned long int n) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%lu", n);
		return buf;
	}
};
template <> struct unsignedLongIntTo<std::wstring> {
	static std::wstring apply(unsigned long int n) {
		wchar_t buf[32];
		swprintf(buf, sizeof(buf)/sizeof(wchar_t), L"%lu", n);
		return buf;
	}
};

#endif // _NUMERIC_CONVERSIONS_H
