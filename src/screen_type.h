/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_SCREEN_TYPE_H
#define NCMPCPP_SCREEN_TYPE_H

#include <string>
#include "config.h"

// forward declaration
struct BaseScreen;

enum class ScreenType {
	Browser,
#	ifdef ENABLE_CLOCK
	Clock,
#	endif // ENABLE_CLOCK
	Help,
#	ifdef HAVE_CURL_CURL_H
	Lastfm,
#	endif // HAVE_CURL_CURL_H
	Lyrics,
	MediaLibrary,
#	ifdef ENABLE_OUTPUTS
	Outputs,
#	endif // ENABLE_OUTPUTS
	Playlist,
	PlaylistEditor,
	SearchEngine,
	SelectedItemsAdder,
	ServerInfo,
	SongInfo,
	SortPlaylistDialog,
#	ifdef HAVE_TAGLIB_H
	TagEditor,
	TinyTagEditor,
#	endif // HAVE_TAGLIB_H
	Unknown,
#	ifdef ENABLE_VISUALIZER
	Visualizer,
#	endif // ENABLE_VISUALIZER
};

ScreenType stringtoStartupScreenType(const std::string &s);
ScreenType stringToScreenType(const std::string &s);

BaseScreen *toScreen(ScreenType st);

#endif // NCMPCPP_SCREEN_TYPE_H
