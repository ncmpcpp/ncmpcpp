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

#ifndef NCMPCPP_SCREEN_TYPE_H
#define NCMPCPP_SCREEN_TYPE_H

#include <string>

enum class ScreenType {
	Browser,
	Clock,
	Help,
	Lastfm,
	Lyrics,
	MediaLibrary,
	Outputs,
	Playlist,
	PlaylistEditor,
	SearchEngine,
	SelectedItemsAdder,
	ServerInfo,
	SongInfo,
	SortPlaylistDialog,
	TagEditor,
	TinyTagEditor,
	Unknown,
	Visualizer,
};

ScreenType stringtoStarterScreenType(const std::string &s);
ScreenType stringToScreenType(const std::string &s);

#endif // NCMPCPP_SCREEN_TYPE_H
