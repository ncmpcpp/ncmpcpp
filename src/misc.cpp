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

namespace
{
	const std::string unallowed_chars = "\"*/:<>?\\|";
}

void ToLower(std::string &s)
{
	transform(s.begin(), s.end(), s.begin(), tolower);
}

int StrToInt(const std::string &str)
{
	return atoi(str.c_str());
}

std::string IntoStr(int l)
{
	std::ostringstream ss;
	ss << l;
	return ss.str();
}

std::string IntoStr(mpd_TagItems tag) // this is only for left column's title in media library
{
	switch (tag)
	{
		case MPD_TAG_ITEM_ARTIST:
			return "Artist";
		case MPD_TAG_ITEM_ALBUM:
			return "Album";
		case MPD_TAG_ITEM_TITLE:
			return "Title";
		case MPD_TAG_ITEM_TRACK:
			return "Track";
		case MPD_TAG_ITEM_GENRE:
			return "Genre";
		case MPD_TAG_ITEM_DATE:
			return "Year";
		case MPD_TAG_ITEM_COMPOSER:
			return "Composer";
		case MPD_TAG_ITEM_PERFORMER:
			return "Performer";
		case MPD_TAG_ITEM_COMMENT:
			return "Comment";
		case MPD_TAG_ITEM_DISC:
			return "Disc";
		case MPD_TAG_ITEM_FILENAME:
			return "Filename";
		default:
			return "";
	}
}

std::string IntoStr(Color color)
{
	std::string result;
	
	if (color == clDefault)
		result = "default";
	else if (color == clBlack)
		result = "black";
	else if (color == clRed)
		result = "red";
	else if (color == clGreen)
		result = "green";
	else if (color == clYellow)
		result = "yellow";
	else if (color == clBlue)
		result = "blue";
	else if (color == clMagenta)
		result = "magenta";
	else if (color == clCyan)
		result = "cyan";
	else if (color == clWhite)
		result = "white";
	
	return result;
}

Color IntoColor(const std::string &color)
{
	Color result = clDefault;
	
	if (color == "black")
		result = clBlack;
	else if (color == "red")
		result = clRed;
	else if (color == "green")
		result = clGreen;
	else if (color == "yellow")
		result = clYellow;
	else if (color == "blue")
		result = clBlue;
	else if (color == "magenta")
		result = clMagenta;
	else if (color == "cyan")
		result = clCyan;
	else if (color == "white")
		result = clWhite;
	
	return result;
}

mpd_TagItems IntoTagItem(char c)
{
	switch (c)
	{
		case 'a':
			return MPD_TAG_ITEM_ARTIST;
		case 'y':
			return MPD_TAG_ITEM_DATE;
		case 'g':
			return MPD_TAG_ITEM_GENRE;
		case 'c':
			return MPD_TAG_ITEM_COMPOSER;
		case 'p':
			return MPD_TAG_ITEM_PERFORMER;
		default:
			return MPD_TAG_ITEM_ARTIST;
	}
}

void EscapeUnallowedChars(std::string &s)
{
	for (std::string::const_iterator it = unallowed_chars.begin(); it != unallowed_chars.end(); it++)
	{
		for (size_t i = 0; i < s.length(); i++)
		{
			if (s[i] == *it)
			{
				s.erase(s.begin()+i);
				i--;
			}
		}
	}
}

