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

#include "display.h"
#include "global.h"
#include "menu.h"
#include "playlist.h"
#include "song.h"

using namespace Global;

Menu<MPD::Song> *Global::mPlaylist;

void Playlist::Init()
{
	mPlaylist = new Menu<MPD::Song>(0, main_start_y, COLS, main_height, Config.columns_in_playlist ? Display::Columns(Config.song_columns_list_format) : "", Config.main_color, brNone);
	mPlaylist->SetTimeout(ncmpcpp_window_timeout);
	mPlaylist->HighlightColor(Config.main_highlight_color);
	mPlaylist->SetSelectPrefix(&Config.selected_item_prefix);
	mPlaylist->SetSelectSuffix(&Config.selected_item_suffix);
	mPlaylist->SetItemDisplayer(Config.columns_in_playlist ? Display::SongsInColumns : Display::Songs);
	mPlaylist->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
}

void Playlist::Resize()
{
	mPlaylist->Resize(COLS, main_height);
	mPlaylist->SetTitle(Config.columns_in_playlist ? Display::Columns(Config.song_columns_list_format) : "");
}

void Playlist::SwitchTo()
{
	if (current_screen != csPlaylist
#	ifdef HAVE_TAGLIB_H
	&&  current_screen != csTinyTagEditor
#	endif // HAVE_TAGLIB_H
	   )
	{
		CLEAR_FIND_HISTORY;
		wCurrent = mPlaylist;
		wCurrent->Hide();
		current_screen = csPlaylist;
//		redraw_screen = 1;
		redraw_header = 1;
	}
}

