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

#ifndef _CHARSET_H
#define _CHARSET_H

#include "config.h"
#include <string>

namespace IConv {//

#ifdef HAVE_ICONV_H

void convertFromTo(const char *from, const char *to, std::string &s);

std::string utf8ToLocale(std::string s);
std::string localeToUtf8(std::string s);

void utf8ToLocale_(std::string &s);
void localeToUtf8_(std::string &s);

#else

inline void convertFromTo(const char *, const char *, std::string &) { }

inline std::string utf8ToLocale(std::string s) { return s; }
inline std::string localeToUtf8(std::string s) { return s; }

inline void utf8ToLocale_(std::string &) { }
inline void localeToUtf8_(std::string &) { }

#endif // HAVE_ICONV_H

}

#endif

