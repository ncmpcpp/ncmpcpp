/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "misc.h"

using std::stringstream;

int Abs(int num)
{
	return (num < 0 ? -num : num);
}

int StrToInt(string str)
{
	return atoi(str.c_str());
}

string IntoStr(int liczba)
{
	stringstream ss;
	ss << liczba;
	return ss.str();
}

string IntoStr(double liczba, int precision)
{
	stringstream ss;
	ss << std::fixed << std::setprecision(precision) << liczba;
	return ss.str();
}

