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

#include <boost/locale/encoding.hpp>
#include "charset.h"
#include "settings.h"

namespace Charset {//

std::string toUtf8From(std::string s, const char *charset)
{
	return boost::locale::conv::to_utf<char>(s, charset);
}

std::string fromUtf8To(std::string s, const char *charset)
{
	return boost::locale::conv::to_utf<char>(s, charset);
}

std::string utf8ToLocale(std::string s)
{
	return Config.system_encoding.empty()
	? s
	: boost::locale::conv::from_utf<char>(s, Config.system_encoding);
}

std::string localeToUtf8(std::string s)
{
	return Config.system_encoding.empty()
	? s
	: boost::locale::conv::to_utf<char>(s, Config.system_encoding);
}

}
