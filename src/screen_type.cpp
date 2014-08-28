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

#include "config.h"
#include "screen_type.h"

#include "browser.h"
#include "clock.h"
#include "help.h"
#include "lastfm.h"
#include "lyrics.h"
#include "media_library.h"
#include "outputs.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "sel_items_adder.h"
#include "server_info.h"
#include "song_info.h"
#include "sort_playlist.h"
#include "tag_editor.h"
#include "tiny_tag_editor.h"
#include "visualizer.h"

ScreenType stringtoStartupScreenType(const std::string &s)
{
	ScreenType result = ScreenType::Unknown;
	if (s == "browser")
		result = ScreenType::Browser;
#	ifdef ENABLE_CLOCK
	else if (s == "clock")
		result = ScreenType::Clock;
#	endif // ENABLE_CLOCK
	else if (s == "help")
		result = ScreenType::Help;
	else if (s == "media_library")
		result = ScreenType::MediaLibrary;
#	ifdef ENABLE_OUTPUTS
	else if (s == "outputs")
		result = ScreenType::Outputs;
#	endif // ENABLE_OUTPUTS
	else if (s == "playlist")
		result = ScreenType::Playlist;
	else if (s == "playlist_editor")
		result = ScreenType::PlaylistEditor;
	else if (s == "search_engine")
		result = ScreenType::SearchEngine;
#	ifdef HAVE_TAGLIB_H
	else if (s == "tag_editor")
		result = ScreenType::TagEditor;
#	endif // HAVE_TAGLIB_H
#	ifdef ENABLE_VISUALIZER
	else if (s == "visualizer")
		result = ScreenType::Visualizer;
#	endif // ENABLE_VISUALIZER
	return result;
}

ScreenType stringToScreenType(const std::string &s)
{
	ScreenType result = stringtoStartupScreenType(s);
	if (result == ScreenType::Unknown)
	{
		if (s == "lyrics")
			result = ScreenType::Lyrics;
#		ifdef HAVE_CURL_CURL_H
		else if (s == "last_fm")
			result = ScreenType::Lastfm;
#		endif // HAVE_CURL_CURL_H
		else if (s == "selected_items_adder")
			result = ScreenType::SelectedItemsAdder;
		else if (s == "server_info")
			result = ScreenType::ServerInfo;
		else if (s == "song_info")
			result = ScreenType::SongInfo;
		else if (s == "sort_playlist_dialog")
			result = ScreenType::SortPlaylistDialog;
#		ifdef HAVE_TAGLIB_H
		else if (s == "tiny_tag_editor")
			result = ScreenType::TinyTagEditor;
#		endif // HAVE_TAGLIB_H
	}
	return result;
}

BaseScreen *toScreen(ScreenType st)
{
	switch (st)
	{
		case ScreenType::Browser:
			return myBrowser;
#		ifdef ENABLE_CLOCK
		case ScreenType::Clock:
			return myClock;
#		endif // ENABLE_CLOCK
		case ScreenType::Help:
			return myHelp;
#		ifdef HAVE_CURL_CURL_H
		case ScreenType::Lastfm:
			return myLastfm;
#		endif // HAVE_CURL_CURL_H
		case ScreenType::Lyrics:
			return myLyrics;
		case ScreenType::MediaLibrary:
			return myLibrary;
#		ifdef ENABLE_OUTPUTS
		case ScreenType::Outputs:
			return myOutputs;
#		endif // ENABLE_OUTPUTS
		case ScreenType::Playlist:
			return myPlaylist;
		case ScreenType::PlaylistEditor:
			return myPlaylistEditor;
		case ScreenType::SearchEngine:
			return mySearcher;
		case ScreenType::SelectedItemsAdder:
			return mySelectedItemsAdder;
		case ScreenType::ServerInfo:
			return myServerInfo;
		case ScreenType::SongInfo:
			return mySongInfo;
		case ScreenType::SortPlaylistDialog:
			return mySortPlaylistDialog;
#		ifdef HAVE_TAGLIB_H
		case ScreenType::TagEditor:
			return myTagEditor;
		case ScreenType::TinyTagEditor:
			return myTinyTagEditor;
#		endif // HAVE_TAGLIB_H
#		ifdef ENABLE_VISUALIZER
		case ScreenType::Visualizer:
			return myVisualizer;
#		endif // ENABLE_VISUALIZER
		default:
			return nullptr;
	}
}
