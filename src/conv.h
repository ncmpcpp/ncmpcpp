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

#ifndef _MISC_H
#define _MISC_H

#include <string>

#include "window.h"
#include "song.h"

void ToLower(std::string &);

int StrToInt(const std::string &);
long StrToLong(const std::string &);

std::string IntoStr(int);

std::string IntoStr(mpd_TagItems);

std::string IntoStr(NCurses::Color);

NCurses::Color IntoColor(const std::string &);

mpd_TagItems IntoTagItem(char);

#ifdef HAVE_TAGLIB_H
MPD::Song::SetFunction IntoSetFunction(mpd_TagItems);
#endif // HAVE_TAGLIB_H

void EscapeUnallowedChars(std::string &);

void EscapeHtml(std::string &s);

void Trim(std::string &s);

#endif

