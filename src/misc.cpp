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

#include <algorithm>
#include <sstream>

#include "misc.h"

/*int Abs(int num)
{
	return (num < 0 ? -num : num);
}*/

void ToLower(std::string &s)
{
	transform(s.begin(), s.end(), s.begin(), tolower);
}

int StrToInt(std::string str)
{
	return atoi(str.c_str());
}

std::string IntoStr(int l)
{
	std::ostringstream ss;
	ss << l;
	return ss.str();
}

/*string IntoStr(double l, int precision)
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(precision) << l;
	return ss.str();
}*/

