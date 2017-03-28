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

#include <algorithm>

#include "curses/menu_impl.h"
#include "screens/browser.h"
#include "global.h"
#include "helpers.h"
#include "mpdpp.h"
#include "screens/playlist.h"
#include "screens/sel_items_adder.h"
#include "settings.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "screens/screen_switcher.h"
#include "charset.h"

SelectedItemsAdder *mySelectedItemsAdder;

namespace {

void DisplayComponent(SelectedItemsAdder::Component &menu)
{
	menu << Charset::utf8ToLocale(menu.drawn()->value().item());
}

bool EntryMatcher(const Regex::Regex &rx, const NC::Menu<SelectedItemsAdder::Entry>::Item &item)
{
	if (!item.isSeparator())
		return Regex::search(item.value().item(), rx, Config.ignore_diacritics);
	else
		return false;
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
	setHighlightFixes(m_playlist_selector);
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
	setHighlightFixes(m_position_selector);
	m_position_selector.setItemDisplayer(DisplayComponent);
	
	m_position_selector.addItem(Entry("At the end of playlist",
		std::bind(&Self::addAtTheEndOfPlaylist, this)
	));
	m_position_selector.addItem(Entry("At the beginning of playlist",
		std::bind(&Self::addAtTheBeginningOfPlaylist, this)
	));
	m_position_selector.addItem(Entry("After current song",
		std::bind(&Self::addAfterCurrentSong, this)
	));
	m_position_selector.addItem(Entry("After current album",
		std::bind(&Self::addAfterCurrentAlbum, this)
	));
	m_position_selector.addItem(Entry("After highlighted item",
		std::bind(&Self::addAfterHighlightedSong, this)
	));
	m_position_selector.addSeparator();
	m_position_selector.addItem(Entry("Cancel",
		std::bind(&Self::cancel, this)
	));
	
	w = &m_playlist_selector;
}

void SelectedItemsAdder::switchTo()
{
	using Global::myScreen;
	
	auto hs = dynamic_cast<HasSongs *>(myScreen);
	if (!hs)
		return;
	
	Statusbar::print(1, "Fetching selected songs...");
	m_selected_items = hs->getSelectedSongs();
	if (m_selected_items.empty())
	{
		Statusbar::print("No selected songs");
		return;
	}
	populatePlaylistSelector(myScreen);
	SwitchTo::execute(this);
	// default to main window
	w = &m_playlist_selector;
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
	if (previousScreen() && previousScreen()->hasToBeResized) // resize background window
	{
		previousScreen()->resize();
		previousScreen()->refresh();
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
	return previousScreen()->title();
}

void SelectedItemsAdder::mouseButtonPressed(MEVENT me)
{
	if (w->empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		w->Goto(me.y);
		if (me.bstate & BUTTON3_PRESSED)
			runAction();
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

/***********************************************************************/

bool SelectedItemsAdder::actionRunnable()
{
	return !w->empty();
}

void SelectedItemsAdder::runAction()
{
	w->current()->value().run();
}

/***********************************************************************/

bool SelectedItemsAdder::allowsSearching()
{
	return true;
}

const std::string &SelectedItemsAdder::searchConstraint()
{
	return m_search_predicate.constraint();
}

void SelectedItemsAdder::setSearchConstraint(const std::string &constraint)
{
	m_search_predicate = Regex::ItemFilter<Entry>(
		constraint,
		Config.regex_type,
		EntryMatcher);
}

void SelectedItemsAdder::clearSearchConstraint()
{
	m_search_predicate.clear();
}

bool SelectedItemsAdder::search(SearchDirection direction, bool wrap, bool skip_current)
{
	return ::search(*w, m_search_predicate, direction, wrap, skip_current);
}

/***********************************************************************/

void SelectedItemsAdder::populatePlaylistSelector(BaseScreen *old_screen)
{
	// stored playlists don't support songs from outside of mpd database
	bool in_local_browser = old_screen == myBrowser && myBrowser->isLocal();

	m_playlist_selector.reset();
	m_playlist_selector.clear();
	m_playlist_selector.addItem(Entry("Current playlist",
		std::bind(&Self::addToCurrentPlaylist, this)
	));
	if (!in_local_browser)
	{
		m_playlist_selector.addItem(Entry("New playlist",
			std::bind(&Self::addToNewPlaylist, this)
		));
	}
	m_playlist_selector.addSeparator();
	if (!in_local_browser)
	{
		size_t begin = m_playlist_selector.size();
		for (MPD::PlaylistIterator it = Mpd.GetPlaylists(), end; it != end; ++it)
		{
			m_playlist_selector.addItem(Entry(it->path(),
				std::bind(&Self::addToExistingPlaylist, this, it->path())
			));
		};
		std::sort(m_playlist_selector.beginV()+begin, m_playlist_selector.endV(),
			LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
		if (begin < m_playlist_selector.size())
			m_playlist_selector.addSeparator();
	}
	m_playlist_selector.addItem(Entry("Cancel",
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
	std::string playlist;
	{
		Statusbar::ScopedLock slock;
		Statusbar::put() << "Save playlist as: ";
		playlist = Global::wFooter->prompt();
	}
	addToExistingPlaylist(playlist);
}

void SelectedItemsAdder::addToExistingPlaylist(const std::string &playlist) const
{
	Mpd.StartCommandsList();
	for (auto s = m_selected_items.begin(); s != m_selected_items.end(); ++s)
		Mpd.AddToPlaylist(playlist, *s);
	Mpd.CommitCommandsList();
	Statusbar::printf("Selected item(s) added to playlist \"%1%\"", playlist);
	switchToPreviousScreen();
}

void SelectedItemsAdder::addAtTheEndOfPlaylist() const
{
	bool success = addSongsToPlaylist(m_selected_items.begin(), m_selected_items.end(), false, -1);
	exitSuccessfully(success);
}

void SelectedItemsAdder::addAtTheBeginningOfPlaylist() const
{
	bool success = addSongsToPlaylist(m_selected_items.begin(), m_selected_items.end(), false, 0);
	exitSuccessfully(success);
}

void SelectedItemsAdder::addAfterCurrentSong() const
{
	if (Status::State::player() == MPD::psStop)
		return;
	size_t pos = Status::State::currentSongPosition();
	++pos;
	bool success = addSongsToPlaylist(m_selected_items.begin(), m_selected_items.end(), false, pos);
	exitSuccessfully(success);
}

void SelectedItemsAdder::addAfterCurrentAlbum() const
{
	if (Status::State::player() == MPD::psStop)
		return;
	auto &pl = myPlaylist->main();
	size_t pos = Status::State::currentSongPosition();
	std::string album =  pl[pos].value().getAlbum();
	while (pos < pl.size() && pl[pos].value().getAlbum() == album)
		++pos;
	bool success = addSongsToPlaylist(m_selected_items.begin(), m_selected_items.end(), false, pos);
	exitSuccessfully(success);
}

void SelectedItemsAdder::addAfterHighlightedSong() const
{
	size_t pos = myPlaylist->main().current()->value().getPosition();
	++pos;
	bool success = addSongsToPlaylist(m_selected_items.begin(), m_selected_items.end(), false, pos);
	exitSuccessfully(success);
}

void SelectedItemsAdder::cancel()
{
	if (isActiveWindow(m_playlist_selector))
		switchToPreviousScreen();
	else if (isActiveWindow(m_position_selector))
		w = &m_playlist_selector;
}

void SelectedItemsAdder::exitSuccessfully(bool success) const
{
	Statusbar::printf("Selected items added%1%", withErrors(success));
	switchToPreviousScreen();
}

void SelectedItemsAdder::setDimensions()
{
	using Global::MainHeight;
	
	m_playlist_selector_width = COLS*0.6;
	m_playlist_selector_height = std::min(MainHeight, size_t(LINES*0.66));
	
	m_position_selector_width = std::min(size_t(35), size_t(COLS));
	m_position_selector_height = std::min(size_t(11), MainHeight);
}
