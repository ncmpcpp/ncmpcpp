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

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include "settings.h"
#include "ncmpcpp.h"
#include "mpdpp.h"

/// FIXME: this is absolutely shitty, I need to get rid of this.

namespace Global
{
	extern ncmpcpp_config Config;
	extern ncmpcpp_keys Key;
	
	extern Window *wCurrent;
	extern Window *wPrev;
	
	extern Menu<MPD::Song> *mPlaylist;
	extern Menu<MPD::Item> *mBrowser;
	extern Menu< std::pair<Buffer *, MPD::Song *> > *mSearcher;
	
	extern Window *wLibActiveCol;
	extern Menu<std::string> *mLibArtists;
	extern Menu<string_pair> *mLibAlbums;
	extern Menu<MPD::Song> *mLibSongs;
	
#	ifdef HAVE_TAGLIB_H
	extern Window *wTagEditorActiveCol;
	extern Menu<Buffer> *mTagEditor;
	extern Menu<string_pair> *mEditorAlbums;
	extern Menu<string_pair> *mEditorDirs;
#	endif // HAVE_TAGLIB_H
	// blah, I use below in conditionals.
	extern Menu<string_pair> *mEditorLeftCol;
	extern Menu<std::string> *mEditorTagTypes;
	extern Menu<MPD::Song> *mEditorTags;
	
	extern Window *wPlaylistEditorActiveCol;
	extern Menu<std::string> *mPlaylistList;
	extern Menu<MPD::Song> *mPlaylistEditor;
	
	extern Scrollpad *sHelp;
	extern Scrollpad *sLyrics;
	extern Scrollpad *sInfo;
	
	extern Window *wHeader;
	extern Window *wFooter;
#	ifdef ENABLE_CLOCK
	extern Scrollpad *wClock;
#	endif
	
	extern MPD::Connection *Mpd;
	
	extern int now_playing;
	extern int lock_statusbar_delay;

	extern size_t browsed_dir_scroll_begin;

	extern time_t timer;

	extern std::string browsed_dir;
	extern std::string editor_browsed_dir;
	extern std::string editor_highlighted_dir;

	extern NcmpcppScreen current_screen;
	extern NcmpcppScreen prev_screen;

#	ifdef HAVE_CURL_CURL_H
	extern pthread_t lyrics_downloader;
	extern pthread_t artist_info_downloader;
	extern bool lyrics_ready;
	extern bool artist_info_ready;
#	endif

	extern bool dont_change_now_playing;
	extern bool block_progressbar_update;
	extern bool block_playlist_update;
	extern bool block_item_list_update;

	extern bool messages_allowed;
	extern bool redraw_header;
	extern bool reload_lyrics;
	
	extern std::string volume_state;

	extern bool header_update_status;
	
	extern bool header_update_status;
	extern bool search_case_sensitive;
	extern bool search_match_to_pattern;

	extern std::string volume_state;

	extern const char *search_mode_normal;
	extern const char *search_mode_strict;
}

#endif

