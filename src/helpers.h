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

#ifndef _HELPERS_H
#define _HELPERS_H

#include "mpdpp.h"
#include "ncmpcpp.h"
#include "settings.h"

bool ConnectToMPD();
void ParseArgv(int, char **);

class CaseInsensitiveSorting
{
	public:
		bool operator()(std::string, std::string);
		bool operator()(MPD::Song *, MPD::Song *);
		bool operator()(const MPD::Item &, const MPD::Item &);
};

bool SortSongsByTrack(MPD::Song *, MPD::Song *);

void UpdateSongList(Menu<MPD::Song> *);

bool Keypressed(int, const int *);

std::string FindSharedDir(const std::string &, const std::string &);

void GetInfo(MPD::Song &, Scrollpad &);

std::string GetLineValue(std::string &, char = '"', char = '"', bool = 0);

Window &Statusbar();

const Buffer &ShowTag(const std::string &);
const basic_buffer<my_char_t> &ShowTagInInfoScreen(const std::string &);

void Scroller(Window &, const std::string &, size_t, size_t &);

#endif

