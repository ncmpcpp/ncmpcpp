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

#include "screen_type.h"

ScreenType stringtoStarterScreenType(const std::string &s)
{
	ScreenType result = ScreenType::Unknown;
	if (s == "browser")
		result = ScreenType::Browser;
	else if (s == "clock")
		result = ScreenType::Clock;
	else if (s == "help")
		result = ScreenType::Help;
	else if (s == "media_library")
		result = ScreenType::MediaLibrary;
	else if (s == "outputs")
		result = ScreenType::Outputs;
	else if (s == "playlist")
		result = ScreenType::Playlist;
	else if (s == "playlist_editor")
		result = ScreenType::PlaylistEditor;
	else if (s == "search_engine")
		result = ScreenType::SearchEngine;
	else if (s == "tag_editor")
		result = ScreenType::TagEditor;
	else if (s == "visualizer")
		result = ScreenType::Visualizer;
	return result;
}

ScreenType stringToScreenType(const std::string &s)
{
	ScreenType result = stringtoStarterScreenType(s);
	if (result == ScreenType::Unknown)
	{
		if (s == "last_fm")
			result = ScreenType::Lastfm;
		else if (s == "lyrics")
			result = ScreenType::Lyrics;
		else if (s == "selected_items_adder")
			result = ScreenType::SelectedItemsAdder;
		else if (s == "server_info")
			result = ScreenType::ServerInfo;
		else if (s == "song_info")
			result = ScreenType::SongInfo;
		else if (s == "sort_playlist_dialog")
			result = ScreenType::SortPlaylistDialog;
		else if (s == "tiny_tag_editor")
			result = ScreenType::TinyTagEditor;
	}
	return result;
}
