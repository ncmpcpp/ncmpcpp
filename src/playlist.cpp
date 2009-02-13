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

Playlist *myPlaylist = new Playlist;

void Playlist::Init()
{
	w = new Menu<MPD::Song>(0, main_start_y, COLS, main_height, Config.columns_in_playlist ? Display::Columns(Config.song_columns_list_format) : "", Config.main_color, brNone);
	w->SetTimeout(ncmpcpp_window_timeout);
	w->HighlightColor(Config.main_highlight_color);
	w->SetSelectPrefix(&Config.selected_item_prefix);
	w->SetSelectSuffix(&Config.selected_item_suffix);
	w->SetItemDisplayer(Config.columns_in_playlist ? Display::SongsInColumns : Display::Songs);
	w->SetItemDisplayerUserData(Config.columns_in_playlist ? &Config.song_columns_list_format : &Config.song_list_format);
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
		wCurrent = w;
		wCurrent->Hide();
		current_screen = csPlaylist;
//		redraw_screen = 1;
		redraw_header = 1;
	}
}

void Playlist::Resize()
{
	w->Resize(COLS, main_height);
	w->SetTitle(Config.columns_in_playlist ? Display::Columns(Config.song_columns_list_format) : "");
}

const char *Playlist::Title()
{
	return "Playlist ";
}

void Playlist::SpacePressed()
{
	if (w->Empty())
		return;
	size_t i = w->Choice();
	w->Select(i, !w->isSelected(i));
	w->Scroll(wDown);
}

void Playlist::EnterPressed()
{
	if (!w->Empty())
		Mpd->PlayID(w->Current().GetID());
}

