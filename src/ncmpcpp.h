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
const bool UNICODE = 1;
# define ncmpcpp_string_t wstring
# define NCMPCPP_TO_WSTRING(x) ToWString(x)
#else
const bool UNICODE = 0;
# define ncmpcpp_string_t string
# define NCMPCPP_TO_WSTRING(x) (x)
#endif

#define KEY_TAB 9
#define ENTER 10
#define KEY_SPACE 32

#ifdef HAVE_TAGLIB_H
# include "fileref.h"
# include "tag.h"
#endif

#include "libmpd/libmpd.h"

#include <clocale>
#include <ctime>
#include <unistd.h>
#include <vector>

#include "window.h"
#include "menu.h"
#include "scrollpad.h"
#include "misc.h"

enum NcmpcppScreen { csHelp, csPlaylist, csBrowser, csTagEditor, csSearcher, csLibrary, csLyrics };

const int ncmpcpp_window_timeout = 500;
const int search_engine_static_option = 17;

const string home_folder = getenv("HOME");
const string TERMINAL_TYPE = getenv("TERM");

const string search_mode_one = "Match if tag contains searched phrase";
const string search_mode_two = "Match only if both values are the same";

#endif

