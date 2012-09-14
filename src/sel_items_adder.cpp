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

#include "browser.h"
#include "global.h"
#include "helpers.h"
#include "mpdpp.h"
#include "playlist.h"
#include "sel_items_adder.h"
#include "settings.h"
#include "statusbar.h"
#include "utility/comparators.h"

SelectedItemsAdder *mySelectedItemsAdder;

namespace {//

void DisplayComponent(SelectedItemsAdder::Component &menu)
{
	menu << menu.drawn()->value().item();
}

}

SelectedItemsAdder::SelectedItemsAdder()
{
	using Global::MainHeight;
	using Global::MainStartY;
	setDimensions();
	
	m_playlist_selector = Component(
		(COLS-m_playlist_selector_width)/2,
		MainStartY+(MainHeight-m_playlist_selector_height)/2,
		m_playlist_selector_width,
		m_playlist_selector_height,
		"Add selected item(s) to...",
		Config.main_color,
		Config.window_border
	);
	m_playlist_selector.cyclicScrolling(Config.use_cyclic_scrolling);
	m_playlist_selector.centeredCursor(Config.centered_cursor);
	m_playlist_selector.setHighlightColor(Config.main_highlight_color);
	m_playlist_selector.setItemDisplayer(DisplayComponent);
	
	m_position_selector = Component(
		(COLS-m_position_selector_width)/2,
		MainStartY+(MainHeight-m_position_selector_height)/2,
		m_position_selector_width,
		m_position_selector_height,
		"Where?",
		Config.main_color,
		Config.window_border
	);
	m_position_selector.cyclicScrolling(Config.use_cyclic_scrolling);
	m_position_selector.centeredCursor(Config.centered_cursor);
	m_position_selector.setHighlightColor(Config.main_highlight_color);
	m_position_selector.setItemDisplayer(DisplayComponent);
	
	typedef SelectedItemsAdder Self;
	m_position_selector.addItem(Component::Item::Type("At the end of playlist",
		std::bind(&Self::addAtTheEndOfPlaylist, this)
	));
	m_position_selector.addItem(Component::Item::Type("At the beginning of playlist",
		std::bind(&Self::addAtTheBeginningOfPlaylist, this)
	));
	m_position_selector.addItem(Component::Item::Type("After current song",
		std::bind(&Self::addAfterCurrentSong, this)
	));
	m_position_selector.addItem(Component::Item::Type("After current album",
		std::bind(&Self::addAfterCurrentAlbum, this)
	));
	m_position_selector.addItem(Component::Item::Type("After highlighted item",
		std::bind(&Self::addAfterHighlightedSong, this)
	));
	m_position_selector.addSeparator();
	m_position_selector.addItem(Component::Item::Type("Cancel",
		std::bind(&Self::cancel, this)
	));
	
	w = &m_playlist_selector;
}

void SelectedItemsAdder::switchTo()
{
	using Global::myScreen;
	
	assert(myScreen != this);
	auto hs = hasSongs(myScreen);
	if (!hs || !hs->allowsSelection())
		return;
	
	Statusbar::msg(1, "Fetching selected songs...");
	m_selected_items = hs->getSelectedSongs();
	if (m_selected_items.empty())
	{
		Statusbar::msg("List of selected items is empty");
		return;
	}
	
	if (hasToBeResized)
		resize();
	
	populatePlaylistSelector(myScreen);
	
	// default to main window
	w = &m_playlist_selector;
	myScreen = this;
}

void SelectedItemsAdder::resize()
{
	using Global::MainHeight;
	using Global::MainStartY;
	setDimensions();
	m_playlist_selector.resize(m_playlist_selector_width, m_playlist_selector_height);
	m_playlist_selector.moveTo(
		(COLS-m_playlist_selector_width)/2,
		MainStartY+(MainHeight-m_playlist_selector_height)/2
	);
	m_position_selector.resize(m_position_selector_width, m_position_selector_height);
	m_position_selector.moveTo(
		(COLS-m_position_selector_width)/2,
		MainStartY+(MainHeight-m_position_selector_height)/2
	);
	if (m_old_screen && m_old_screen->hasToBeResized) // resize background window
	{
		m_old_screen->resize();
		m_old_screen->refresh();
	}
	hasToBeResized = 0;
}

void SelectedItemsAdder::refresh()
{
	if (isActiveWindow(m_position_selector))
	{
		m_playlist_selector.display();
		m_position_selector.display();
	}
	else if (isActiveWindow(m_playlist_selector))
		m_playlist_selector.display();
}

std::wstring SelectedItemsAdder::title()
{
	return m_old_screen->title();
}

void SelectedItemsAdder::enterPressed()
{
	w->current().value().exec()();
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
		Screen<WindowType>::mouseButtonPressed(me);
}

void SelectedItemsAdder::populatePlaylistSelector(BasicScreen *old_screen)
{
	typedef SelectedItemsAdder Self;
	m_old_screen = old_screen;
	m_playlist_selector.reset();
	m_playlist_selector.clear();
	if (old_screen != myPlaylist)
	{
		m_playlist_selector.addItem(Component::Item::Type("Current playlist",
			std::bind(&Self::addToCurrentPlaylist, this)
		));
	}
	m_playlist_selector.addItem(Component::Item::Type("New playlist",
		std::bind(&Self::addToNewPlaylist, this)
	));
	m_playlist_selector.addSeparator();
	
	// stored playlists don't support songs from outside of mpd database
	if (old_screen != myBrowser || !myBrowser->isLocal())
	{
		auto playlists = Mpd.GetPlaylists();
		std::sort(playlists.begin(), playlists.end(),
			LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
		for (auto pl = playlists.begin(); pl != playlists.end(); ++pl)
		{
			m_playlist_selector.addItem(Component::Item::Type(*pl,
				std::bind(&Self::addToExistingPlaylist, this, *pl)
			));
		}
		if (!playlists.empty())
			m_playlist_selector.addSeparator();
	}
	m_playlist_selector.addItem(Component::Item::Type("Cancel",
		std::bind(&Self::cancel, this)
	));
}

void SelectedItemsAdder::addToCurrentPlaylist()
{
	w = &m_position_selector;
	m_position_selector.reset();
}

void SelectedItemsAdder::addToNewPlaylist() const
{
	Statusbar::lock();
	Statusbar::put() << "Save playlist as: ";
	std::string playlist = Global::wFooter->getString();
	Statusbar::unlock();
	if (!playlist.empty())
		addToExistingPlaylist(playlist);
}

void SelectedItemsAdder::addToExistingPlaylist(const std::string &playlist) const
{
	Mpd.StartCommandsList();
	for (auto s = m_selected_items.begin(); s != m_selected_items.end(); ++s)
		Mpd.AddToPlaylist(playlist, *s);
	if (Mpd.CommitCommandsList())
	{
		Statusbar::msg("Selected item(s) added to playlist \"%s\"", playlist.c_str());
		m_old_screen->switchTo();
	}
}

void SelectedItemsAdder::addAtTheEndOfPlaylist() const
{
	bool success = addSongsToPlaylist(m_selected_items, false);
	if (success)
		exitSuccessfully();
}

void SelectedItemsAdder::addAtTheBeginningOfPlaylist() const
{
	bool success = addSongsToPlaylist(m_selected_items, false, 0);
	if (success)
		exitSuccessfully();
}

void SelectedItemsAdder::addAfterCurrentSong() const
{
	if (!Mpd.isPlaying())
		return;
	size_t pos = Mpd.GetCurrentlyPlayingSongPos();
	bool success = addSongsToPlaylist(m_selected_items, false, pos);
	if (success)
		exitSuccessfully();
}

void SelectedItemsAdder::addAfterCurrentAlbum() const
{
	if (!Mpd.isPlaying())
		return;
	auto &pl = myPlaylist->main();
	size_t pos = Mpd.GetCurrentlyPlayingSongPos();
	withUnfilteredMenu(pl, [&pos, &pl]() {
		std::string album =  pl[pos].value().getAlbum();
		while (pos < pl.size() && pl[pos].value().getAlbum() == album)
			++pos;
	});
	bool success = addSongsToPlaylist(m_selected_items, false, pos);
	if (success)
		exitSuccessfully();
}

void SelectedItemsAdder::addAfterHighlightedSong() const
{
	size_t pos = myPlaylist->main().current().value().getPosition();
	++pos;
	bool success = addSongsToPlaylist(m_selected_items, false, pos);
	if (success)
		exitSuccessfully();
}

void SelectedItemsAdder::cancel()
{
	if (isActiveWindow(m_playlist_selector))
		m_old_screen->switchTo();
	else if (isActiveWindow(m_position_selector))
		w = &m_playlist_selector;
}

void SelectedItemsAdder::exitSuccessfully() const
{
	Statusbar::msg("Selected items added");
	m_old_screen->switchTo();
}

void SelectedItemsAdder::setDimensions()
{
	using Global::MainHeight;
	
	m_playlist_selector_width = COLS*0.6;
	m_playlist_selector_height = std::min(MainHeight, size_t(LINES*0.66));
	
	m_position_selector_width = std::min(size_t(35), size_t(COLS));
	m_position_selector_height = std::min(size_t(11), MainHeight);
}
