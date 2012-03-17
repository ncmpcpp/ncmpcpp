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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ICONV_H

#include <string>

void iconv_convert_from_to(const char *from, const char *to, std::string &s);

void utf_to_locale(std::string &);
void locale_to_utf(std::string &);

std::string utf_to_locale_cpy(const std::string &s);
std::string locale_to_utf_cpy(const std::string &s);

void utf_to_locale(const char *&, bool);
void locale_to_utf(const char *&, bool);

#else

#define iconv_convert_from_to(x, y, z);

#define utf_to_locale(x);
#define locale_to_utf(x);

#define utf_to_locale_cpy(x) (x)
#define locale_to_utf_cpy(x) (x)

#endif // HAVE_ICONV_H

#endif

