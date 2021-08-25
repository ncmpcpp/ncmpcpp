/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#ifndef NCMPCPP_CHARSET_H
#define NCMPCPP_CHARSET_H

#include <locale>
#include <string>

namespace Charset {

std::locale internalLocale();

std::string toUtf8From(const std::string &s, const char *charset);
std::string fromUtf8To(const std::string &s, const char *charset);

std::string utf8ToLocale(const std::string &s);
std::string utf8ToLocale(std::string &&s);
std::string localeToUtf8(const std::string &s);
std::string localeToUtf8(std::string &&s);

}

#endif // NCMPCPP_CHARSET_H
