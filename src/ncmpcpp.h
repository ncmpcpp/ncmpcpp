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

#ifndef HAVE_NCMPCPP_H
#define HAVE_NCMPCPP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef UTF8_ENABLED
# define UNICODE 1
#else
# define UNICODE 0
#endif

#include <clocale>
#include <ctime>
#include <algorithm>
#include <map>

#include "window.h"
#include "menu.h"
#include "scrollpad.h"
#include "misc.h"

typedef std::pair<string, string> StringPair;

enum NcmpcppScreen
{
	csHelp,
	csPlaylist,
	csBrowser,
	csTagEditor,
	csInfo,
	csSearcher,
	csLibrary,
	csLyrics,
	csPlaylistEditor,
	csAlbumEditor
};

const int ncmpcpp_window_timeout = 500;

const string home_folder = getenv("HOME");
const string TERMINAL_TYPE = getenv("TERM");

const string search_mode_normal = "Match if tag contains searched phrase";
const string search_mode_strict = "Match only if both values are the same";

#endif

