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

#include <string>
#include "string.h"

#ifndef _UTILITY_NUMERIC_CONVERSIONS_H
#define _UTILITY_NUMERIC_CONVERSIONS_H

template <typename R> struct intTo { };
template <> struct intTo<std::string> {
	static std::string apply(int n) {
		return print<32, std::string>::apply("%d", n);
	}
};
template <> struct intTo<std::wstring> {
	static std::wstring apply(int n) {
		return print<32, std::wstring>::apply(L"%d", n);
	}
};

template <typename R> struct longIntTo { };
template <> struct longIntTo<std::string> {
	static std::string apply(long int n) {
		return print<32, std::string>::apply("%ld", n);
	}
};
template <> struct longIntTo<std::wstring> {
	static std::wstring apply(long int n) {
		return print<32, std::wstring>::apply(L"%ld", n);
	}
};

template <typename R> struct unsignedIntTo { };
template <> struct unsignedIntTo<std::string> {
	static std::string apply(unsigned int n) {
		return print<32, std::string>::apply("%u", n);
	}
};
template <> struct unsignedIntTo<std::wstring> {
	static std::wstring apply(unsigned int n) {
		return print<32, std::wstring>::apply(L"%u", n);
	}
};

template <typename R> struct unsignedLongIntTo { };
template <> struct unsignedLongIntTo<std::string> {
	static std::string apply(unsigned long int n) {
		return print<32, std::string>::apply("%lu", n);
	}
};
template <> struct unsignedLongIntTo<std::wstring> {
	static std::wstring apply(unsigned long int n) {
		return print<32, std::wstring>::apply(L"%lu", n);
	}
};

#endif // _UTILITY_NUMERIC_CONVERSIONS_H
