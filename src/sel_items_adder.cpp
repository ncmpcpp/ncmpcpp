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

#include <algorithm>
#include "charset.h"
#include "browser.h"
#include "display.h"
#include "global.h"
#include "mpdpp.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "sel_items_adder.h"
#include "settings.h"
#include "status.h"
#include "utility/comparators.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;
using Global::myOldScreen;

SelectedItemsAdder *mySelectedItemsAdder = new SelectedItemsAdder;

void SelectedItemsAdder::Init()
{
	SetDimensions();
	itsPlaylistSelector = new NC::Menu<std::string>((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY, itsWidth, itsHeight, "Add selected item(s) to...", Config.main_color, Config.window_border);
	itsPlaylistSelector->cyclicScrolling(Config.use_cyclic_scrolling);
	itsPlaylistSelector->centeredCursor(Config.centered_cursor);
	itsPlaylistSelector->setHighlightColor(Config.main_highlight_color);
	itsPlaylistSelector->setItemDisplayer(Display::Default<std::string>);
	
	itsPositionSelector = new NC::Menu<std::string>((COLS-itsPSWidth)/2, (MainHeight-itsPSHeight)/2+MainStartY, itsPSWidth, itsPSHeight, "Where?", Config.main_color, Config.window_border);
	itsPositionSelector->cyclicScrolling(Config.use_cyclic_scrolling);
	itsPositionSelector->centeredCursor(Config.centered_cursor);
	itsPositionSelector->setHighlightColor(Config.main_highlight_color);
	itsPositionSelector->setItemDisplayer(Display::Default<std::string>);
	itsPositionSelector->addItem("At the end of playlist");
	itsPositionSelector->addItem("At the beginning of playlist");
	itsPositionSelector->addItem("After current track");
	itsPositionSelector->addItem("After current album");
	itsPositionSelector->addItem("After highlighted item");
	itsPositionSelector->addSeparator();
	itsPositionSelector->addItem("Cancel");
	
	w = itsPlaylistSelector;
	isInitialized = 1;
}

void SelectedItemsAdder::SwitchTo()
{
	if (myScreen == this)
	{
		myOldScreen->SwitchTo();
		return;
	}
	auto hs = dynamic_cast<HasSongs *>(myScreen);
	if (!hs || !hs->allowsSelection())
		return;
	
	if (MainHeight < 5)
	{
		ShowMessage("Screen is too small to display this window");
		return;
	}
	
	if (!isInitialized)
		Init();
	
	// default to main window
	w = itsPlaylistSelector;
	
	// Resize() can fall back to old screen, so we need it updated
	myOldScreen = myScreen;
	
	if (hasToBeResized)
		Resize();
	
	bool playlists_not_active = myScreen == myBrowser && myBrowser->isLocal();
	if (playlists_not_active)
		ShowMessage("Local items can't be added to stored playlists");
	
	w->clear();
	w->reset();
	if (myOldScreen != myPlaylist)
		w->addItem("Current MPD playlist", 0, 0);
	w->addItem("New playlist", 0, playlists_not_active);
	w->addSeparator();
	
	auto playlists = Mpd.GetPlaylists();
	std::sort(playlists.begin(), playlists.end(), CaseInsensitiveSorting());
	for (auto it = playlists.begin(); it != playlists.end(); ++it)
		w->addItem(*it, 0, playlists_not_active);
	w->addSeparator();
	w->addItem("Cancel");
	
	myScreen = this;
}

void SelectedItemsAdder::Resize()
{
	SetDimensions();
	if (itsHeight < 5) // screen too low to display this window
		return myOldScreen->SwitchTo();
	itsPlaylistSelector->resize(itsWidth, itsHeight);
	itsPlaylistSelector->moveTo((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY);
	size_t poss_width = std::min(itsPSWidth, size_t(COLS));
	size_t poss_height = std::min(itsPSHeight, size_t(MainHeight));
	itsPositionSelector->resize(poss_width, poss_height);
	itsPositionSelector->moveTo((COLS-poss_width)/2, (MainHeight-poss_height)/2+MainStartY);
	if (myOldScreen && myOldScreen->hasToBeResized) // resize background window
	{
		myOldScreen->Resize();
		myOldScreen->Refresh();
	}
	hasToBeResized = 0;
}

void SelectedItemsAdder::Refresh()
{
	if (w == itsPositionSelector)
	{
		itsPlaylistSelector->display();
		itsPositionSelector->display();
	}
	else
		itsPlaylistSelector->display();
}

std::basic_string<my_char_t> SelectedItemsAdder::Title()
{
	return myOldScreen->Title();
}

void SelectedItemsAdder::EnterPressed()
{
	size_t pos = w->choice();

	// adding to current playlist is disabled when playlist is active
	if (w == itsPlaylistSelector && myOldScreen == myPlaylist && pos == 0)
		pos++;
	
	MPD::SongList list;
	if ((w != itsPlaylistSelector || pos != 0) && pos != w->size()-1)
		list = dynamic_cast<HasSongs &>(*myOldScreen).getSelectedSongs();
	
	if (w == itsPlaylistSelector)
	{
		if (pos == 0) // add to mpd playlist
		{
			w = itsPositionSelector;
			itsPositionSelector->reset();
			return;
		}
		else if (pos == 1) // create new playlist
		{
			LockStatusbar();
			Statusbar() << "Save playlist as: ";
			std::string playlist = Global::wFooter->getString();
			UnlockStatusbar();
			if (!playlist.empty())
			{
				std::string utf_playlist = locale_to_utf_cpy(playlist);
				Mpd.StartCommandsList();
				for (auto it = list.begin(); it != list.end(); ++it)
					Mpd.AddToPlaylist(utf_playlist, *it);
				if (Mpd.CommitCommandsList())
					ShowMessage("Selected item(s) added to playlist \"%s\"", playlist.c_str());
			}
		}
		else if (pos > 1 && pos < w->size()-1) // add items to existing playlist
		{
			std::string playlist = locale_to_utf_cpy(w->current().value());
			Mpd.StartCommandsList();
			for (auto it = list.begin(); it != list.end(); ++it)
				Mpd.AddToPlaylist(playlist, *it);
			if (Mpd.CommitCommandsList())
				ShowMessage("Selected item(s) added to playlist \"%s\"", w->current().value().c_str());
		}
		if (pos != w->size()-1)
		{
			// refresh playlist's lists
			if (myBrowser->Main() && !myBrowser->isLocal() && myBrowser->CurrentDir() == "/")
				myBrowser->GetDirectory("/");
			if (myPlaylistEditor->Main())
				myPlaylistEditor->Playlists->clear(); // make playlist editor update itself
		}
	}
	else
	{
		// disable adding after current track/album when stopped
		if (pos > 1 && pos < 4 && !Mpd.isPlaying())
		{
			ShowMessage("Player is stopped");
			return;
		}
		
		bool successful_operation;
		if (pos == 0) // end of playlist
		{
			successful_operation = myPlaylist->Add(list, 0);
		}
		else if (pos == 1) // beginning of playlist
		{
			successful_operation = myPlaylist->Add(list, 0, 0);
		}
		else if (pos == 2) // after currently playing track
		{
			successful_operation = myPlaylist->Add(list, 0, Mpd.GetCurrentlyPlayingSongPos()+1);
		}
		else if (pos == 3) // after currently playing album
		{
			std::string album = myPlaylist->NowPlayingSong()->getAlbum();
			int i;
			for (i = Mpd.GetCurrentlyPlayingSongPos()+1; i < int(myPlaylist->Items->size()); ++i)
				if ((*myPlaylist->Items)[i].value().getAlbum() != album)
					break;
			successful_operation = myPlaylist->Add(list, 0, i);
		}
		else if (pos == 4) // after highlighted item
		{
			successful_operation = myPlaylist->Add(list, 0, std::min(myPlaylist->Items->choice()+1, myPlaylist->Items->size()));
		}
		else
		{
			w = itsPlaylistSelector;
			return;
		}
		
		if (successful_operation)
			ShowMessage("Selected item(s) added");
	}
	SwitchTo();
}

void SelectedItemsAdder::MouseButtonPressed(MEVENT me)
{
	if (w->empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		w->Goto(me.y);
		if (me.bstate & BUTTON3_PRESSED)
			EnterPressed();
	}
	else
		Screen< NC::Menu<std::string> >::MouseButtonPressed(me);
}

void SelectedItemsAdder::SetDimensions()
{
	itsWidth = COLS*0.6;
	itsHeight = std::min(size_t(LINES*0.6), MainHeight);
}

