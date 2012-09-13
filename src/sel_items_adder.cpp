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
#include "statusbar.h"
#include "utility/comparators.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;
using Global::myOldScreen;

SelectedItemsAdder *mySelectedItemsAdder = new SelectedItemsAdder;

void SelectedItemsAdder::init()
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

void SelectedItemsAdder::switchTo()
{
	if (myScreen == this)
	{
		myOldScreen->switchTo();
		return;
	}
	auto hs = dynamic_cast<HasSongs *>(myScreen);
	if (!hs || !hs->allowsSelection())
		return;
	
	if (!isInitialized)
		init();
	
	// default to main window
	w = itsPlaylistSelector;
	
	// resize() can fall back to old screen, so we need it updated
	myOldScreen = myScreen;
	
	if (hasToBeResized)
		resize();
	
	bool playlists_not_active = myScreen == myBrowser && myBrowser->isLocal();
	if (playlists_not_active)
		Statusbar::msg("Local items can't be added to stored playlists");
	
	w->clear();
	w->reset();
	if (myOldScreen != myPlaylist)
		w->addItem("Current MPD playlist", 0, 0);
	w->addItem("New playlist", 0, playlists_not_active);
	w->addSeparator();
	
	auto playlists = Mpd.GetPlaylists();
	std::sort(playlists.begin(), playlists.end(),
		LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
	for (auto it = playlists.begin(); it != playlists.end(); ++it)
		w->addItem(*it, 0, playlists_not_active);
	w->addSeparator();
	w->addItem("Cancel");
	
	myScreen = this;
}

void SelectedItemsAdder::resize()
{
	SetDimensions();
	if (itsHeight < 5) // screen too low to display this window
		return myOldScreen->switchTo();
	itsPlaylistSelector->resize(itsWidth, itsHeight);
	itsPlaylistSelector->moveTo((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY);
	size_t poss_width = std::min(itsPSWidth, size_t(COLS));
	size_t poss_height = std::min(itsPSHeight, size_t(MainHeight));
	itsPositionSelector->resize(poss_width, poss_height);
	itsPositionSelector->moveTo((COLS-poss_width)/2, (MainHeight-poss_height)/2+MainStartY);
	if (myOldScreen && myOldScreen->hasToBeResized) // resize background window
	{
		myOldScreen->resize();
		myOldScreen->refresh();
	}
	hasToBeResized = 0;
}

void SelectedItemsAdder::refresh()
{
	if (w == itsPositionSelector)
	{
		itsPlaylistSelector->display();
		itsPositionSelector->display();
	}
	else
		itsPlaylistSelector->display();
}

std::wstring SelectedItemsAdder::title()
{
	return myOldScreen->title();
}

void SelectedItemsAdder::enterPressed()
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
			Statusbar::lock();
			Statusbar::put() << "Save playlist as: ";
			std::string playlist = Global::wFooter->getString();
			Statusbar::unlock();
			if (!playlist.empty())
			{
				Mpd.StartCommandsList();
				for (auto it = list.begin(); it != list.end(); ++it)
					Mpd.AddToPlaylist(playlist, *it);
				if (Mpd.CommitCommandsList())
					Statusbar::msg("Selected item(s) added to playlist \"%s\"", playlist.c_str());
			}
		}
		else if (pos > 1 && pos < w->size()-1) // add items to existing playlist
		{
			std::string playlist = w->current().value();
			Mpd.StartCommandsList();
			for (auto it = list.begin(); it != list.end(); ++it)
				Mpd.AddToPlaylist(playlist, *it);
			if (Mpd.CommitCommandsList())
				Statusbar::msg("Selected item(s) added to playlist \"%s\"", w->current().value().c_str());
		}
		if (pos != w->size()-1)
		{
			// refresh playlist's lists
			if (myBrowser->main() && !myBrowser->isLocal() && myBrowser->CurrentDir() == "/")
				myBrowser->GetDirectory("/");
			if (myPlaylistEditor->main())
				myPlaylistEditor->Playlists->clear(); // make playlist editor update itself
		}
	}
	else
	{
		// disable adding after current track/album when stopped
		if (pos > 1 && pos < 4 && !Mpd.isPlaying())
		{
			Statusbar::msg("Player is stopped");
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
			std::string album = myPlaylist->nowPlayingSong().getAlbum();
			int i;
			for (i = Mpd.GetCurrentlyPlayingSongPos()+1; i < int(myPlaylist->main()->size()); ++i)
				if ((*myPlaylist->main())[i].value().getAlbum() != album)
					break;
			successful_operation = myPlaylist->Add(list, 0, i);
		}
		else if (pos == 4) // after highlighted item
		{
			successful_operation = myPlaylist->Add(list, 0, std::min(myPlaylist->main()->choice()+1, myPlaylist->main()->size()));
		}
		else
		{
			w = itsPlaylistSelector;
			return;
		}
		
		if (successful_operation)
			Statusbar::msg("Selected item(s) added");
	}
	switchTo();
}

void SelectedItemsAdder::mouseButtonPressed(MEVENT me)
{
	if (w->empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		w->Goto(me.y);
		if (me.bstate & BUTTON3_PRESSED)
			enterPressed();
	}
	else
		Screen< NC::Menu<std::string> >::mouseButtonPressed(me);
}

void SelectedItemsAdder::SetDimensions()
{
	itsWidth = COLS*0.6;
	itsHeight = std::min(size_t(LINES*0.6), MainHeight);
}

