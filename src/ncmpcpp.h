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

#ifndef HAVE_NCMPCPP_H
#define HAVE_NCMPCPP_H

#include "window.h"
#include "menu.h"
#include "scrollpad.h"
#include "misc.h"

typedef std::pair<string, string> StringPair;
using std::make_pair;

enum NcmpcppScreen
{
	csHelp,
	csPlaylist,
	csBrowser,
	csTinyTagEditor,
	csInfo,
	csSearcher,
	csLibrary,
	csLyrics,
	csPlaylistEditor,
	csTagEditor,
	csOther
};

const int ncmpcpp_window_timeout = 500;

const string home_folder = getenv("HOME") ? getenv("HOME") : "";

#endif

