/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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
#include "screens/screen_type.h"

#include "screens/browser.h"
#include "screens/clock.h"
#include "screens/help.h"
#include "screens/lastfm.h"
#include "screens/lyrics.h"
#include "screens/media_library.h"
#include "screens/outputs.h"
#include "screens/playlist.h"
#include "screens/playlist_editor.h"
#include "screens/search_engine.h"
#include "screens/sel_items_adder.h"
#include "screens/server_info.h"
#include "screens/song_info.h"
#include "screens/sort_playlist.h"
#include "screens/tag_editor.h"
#include "screens/tiny_tag_editor.h"
#include "screens/visualizer.h"

std::string screenTypeToString(ScreenType st)
{
	switch (st)
	{
	case ScreenType::Browser:
		return "browser";
#ifdef ENABLE_CLOCK
	case ScreenType::Clock:
		return "clock";
#endif // ENABLE_CLOCK
	case ScreenType::Help:
		return "help";
	case ScreenType::Lastfm:
		return "last_fm";
	case ScreenType::Lyrics:
		return "lyrics";
	case ScreenType::MediaLibrary:
		return "media_library";
#ifdef ENABLE_OUTPUTS
	case ScreenType::Outputs:
		return "outputs";
#endif // ENABLE_OUTPUTS
	case ScreenType::Playlist:
		return "playlist";
	case ScreenType::PlaylistEditor:
		return "playlist_editor";
	case ScreenType::SearchEngine:
		return "search_engine";
	case ScreenType::SelectedItemsAdder:
		return "selected_items_adder";
	case ScreenType::ServerInfo:
		return "server_info";
	case ScreenType::SongInfo:
		return "song_info";
	case ScreenType::SortPlaylistDialog:
		return "sort_playlist_dialog";
#ifdef HAVE_TAGLIB_H
	case ScreenType::TagEditor:
		return "tag_editor";
	case ScreenType::TinyTagEditor:
		return "tiny_tag_editor";
#endif // HAVE_TAGLIB_H
	case ScreenType::Unknown:
		return "unknown";
#ifdef ENABLE_VISUALIZER
	case ScreenType::Visualizer:
		return "visualizer";
#endif // ENABLE_VISUALIZER
	}
	// silence gcc warning
	throw std::runtime_error("unreachable");
}

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
	else if (s == "lyrics")
		result = ScreenType::Lyrics;
	else if (s == "last_fm")
		result = ScreenType::Lastfm;
	return result;
}

ScreenType stringToScreenType(const std::string &s)
{
	ScreenType result = stringtoStartupScreenType(s);
	if (result == ScreenType::Unknown)
	{
		if (s == "lyrics")
			result = ScreenType::Lyrics;
		else if (s == "last_fm")
			result = ScreenType::Lastfm;
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
		case ScreenType::Lastfm:
			return myLastfm;
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
