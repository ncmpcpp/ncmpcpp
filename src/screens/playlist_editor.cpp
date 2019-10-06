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
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cassert>

#include "curses/menu_impl.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "screens/playlist.h"
#include "screens/playlist_editor.h"
#include "mpdpp.h"
#include "status.h"
#include "statusbar.h"
#include "screens/tag_editor.h"
#include "format_impl.h"
#include "helpers/song_iterator_maker.h"
#include "utility/functional.h"
#include "utility/comparators.h"
#include "title.h"
#include "screens/screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

namespace ph = std::placeholders;

PlaylistEditor *myPlaylistEditor;

namespace {

size_t LeftColumnStartX;
size_t LeftColumnWidth;
size_t RightColumnStartX;
size_t RightColumnWidth;

std::string SongToString(const MPD::Song &s);
bool PlaylistEntryMatcher(const Regex::Regex &rx, const MPD::Playlist &playlist);
bool SongEntryMatcher(const Regex::Regex &rx, const MPD::Song &s);
boost::optional<size_t> GetSongIndexInPlaylist(MPD::Playlist playlist, const MPD::Song &song);
}

PlaylistEditor::PlaylistEditor()
: m_timer(boost::posix_time::from_time_t(0))
, m_window_timeout(Config.data_fetching_delay ? 250 : BaseScreen::defaultWindowTimeout)
, m_fetching_delay(boost::posix_time::milliseconds(Config.data_fetching_delay ? 250 : -1))
{
	LeftColumnWidth = COLS/3-1;
	RightColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-1;
	
	Playlists = NC::Menu<MPD::Playlist>(0, MainStartY, LeftColumnWidth, MainHeight, Config.titles_visibility ? "Playlists" : "", Config.main_color, NC::Border());
	setHighlightFixes(Playlists);
	Playlists.cyclicScrolling(Config.use_cyclic_scrolling);
	Playlists.centeredCursor(Config.centered_cursor);
	Playlists.setSelectedPrefix(Config.selected_item_prefix);
	Playlists.setSelectedSuffix(Config.selected_item_suffix);
	Playlists.setItemDisplayer([](NC::Menu<MPD::Playlist> &menu) {
		menu << Charset::utf8ToLocale(menu.drawn()->value().path());
	});
	
	Content = NC::Menu<MPD::Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Content" : "", Config.main_color, NC::Border());
	setHighlightInactiveColumnFixes(Content);
	Content.cyclicScrolling(Config.use_cyclic_scrolling);
	Content.centeredCursor(Config.centered_cursor);
	Content.setSelectedPrefix(Config.selected_item_prefix);
	Content.setSelectedSuffix(Config.selected_item_suffix);
	switch (Config.playlist_editor_display_mode)
	{
		case DisplayMode::Classic:
			Content.setItemDisplayer(std::bind(
				Display::Songs, ph::_1, std::cref(Content), std::cref(Config.song_list_format)
			));
			break;
		case DisplayMode::Columns:
			Content.setItemDisplayer(std::bind(
				Display::SongsInColumns, ph::_1, std::cref(Content)
			));
			break;
	}
	
	w = &Playlists;
}

void PlaylistEditor::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	
	LeftColumnStartX = x_offset;
	LeftColumnWidth = width/3-1;
	RightColumnStartX = LeftColumnStartX+LeftColumnWidth+1;
	RightColumnWidth = width-LeftColumnWidth-1;
	
	Playlists.resize(LeftColumnWidth, MainHeight);
	Content.resize(RightColumnWidth, MainHeight);
	
	Playlists.moveTo(LeftColumnStartX, MainStartY);
	Content.moveTo(RightColumnStartX, MainStartY);
	
	hasToBeResized = 0;
}

std::wstring PlaylistEditor::title()
{
	return L"Playlist editor";
}

void PlaylistEditor::refresh()
{
	Playlists.display();
	drawSeparator(RightColumnStartX-1);
	Content.display();
}

void PlaylistEditor::switchTo()
{
	SwitchTo::execute(this);
	drawHeader();
	refresh();
}

void PlaylistEditor::update()
{
	{
		ScopedUnfilteredMenu<MPD::Playlist> sunfilter_playlists(ReapplyFilter::No, Playlists);
		if (Playlists.empty() || m_playlists_update_requested)
		{
			m_playlists_update_requested = false;
			sunfilter_playlists.set(ReapplyFilter::Yes, true);
			size_t idx = 0;
			try
			{
				for (MPD::PlaylistIterator it = Mpd.GetPlaylists(), end; it != end; ++it, ++idx)
				{
					if (idx < Playlists.size())
						Playlists[idx].value() = std::move(*it);
					else
						Playlists.addItem(std::move(*it));
				};
			}
			catch (MPD::ServerError &e)
			{
				if (e.code() == MPD_SERVER_ERROR_SYSTEM) // no playlists directory
					Statusbar::print(e.what());
				else
					throw;
			}
			if (idx < Playlists.size())
				Playlists.resizeList(idx);
			std::sort(Playlists.beginV(), Playlists.endV(),
			          LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
		}
	}

	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter_content(ReapplyFilter::No, Content);
		if (!Playlists.empty()
		    && ((Content.empty() && Global::Timer - m_timer > m_fetching_delay)
		        || m_content_update_requested))
		{
			m_content_update_requested = false;
			sunfilter_content.set(ReapplyFilter::Yes, true);
			size_t idx = 0;
			MPD::SongIterator s = Mpd.GetPlaylistContent(Playlists.current()->value().path()), end;
			for (; s != end; ++s, ++idx)
			{
				if (idx < Content.size())
					Content[idx].value() = std::move(*s);
				else
					Content.addItem(std::move(*s));
			}
			if (idx < Content.size())
				Content.resizeList(idx);
			std::string wtitle;
			if (Config.titles_visibility)
			{
				wtitle = (boost::format("Content (%1% %2%)")
				          % boost::lexical_cast<std::string>(Content.size())
				          % (Content.size() == 1 ? "item" : "items")).str();
				wtitle.resize(Content.getWidth());
			}
			Content.setTitle(wtitle);
			Content.refreshBorder();
		}
	}
}

int PlaylistEditor::windowTimeout()
{
	ScopedUnfilteredMenu<MPD::Song> sunfilter_content(ReapplyFilter::No, Content);
	if (Content.empty())
		return m_window_timeout;
	else
		return Screen<WindowType>::windowTimeout();
}

void PlaylistEditor::mouseButtonPressed(MEVENT me)
{
	if (Playlists.hasCoords(me.x, me.y))
	{
		if (!isActiveWindow(Playlists))
		{
			if (previousColumnAvailable())
				previousColumn();
			else
				return;
		}
		if (size_t(me.y) < Playlists.size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Playlists.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				addItemToPlaylist(false);
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
		Content.clear();
	}
	else if (Content.hasCoords(me.x, me.y))
	{
		if (!isActiveWindow(Content))
		{
			if (nextColumnAvailable())
				nextColumn();
			else
				return;
		}
		if (size_t(me.y) < Content.size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Content.Goto(me.y);
			bool play = me.bstate & BUTTON3_PRESSED;
			addItemToPlaylist(play);
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/***********************************************************************/

bool PlaylistEditor::allowsSearching()
{
	return true;
}

const std::string &PlaylistEditor::searchConstraint()
{
	if (isActiveWindow(Playlists))
		return m_playlists_search_predicate.constraint();
	else if (isActiveWindow(Content))
		return m_content_search_predicate.constraint();
	throw std::runtime_error("no active window");
}

void PlaylistEditor::setSearchConstraint(const std::string &constraint)
{
	if (isActiveWindow(Playlists))
	{
		m_playlists_search_predicate = Regex::Filter<MPD::Playlist>(
			constraint,
			Config.regex_type,
			PlaylistEntryMatcher);
	}
	else if (isActiveWindow(Content))
	{
		m_content_search_predicate = Regex::Filter<MPD::Song>(
			constraint,
			Config.regex_type,
			SongEntryMatcher);
	}
}

void PlaylistEditor::clearSearchConstraint()
{
	if (isActiveWindow(Playlists))
		m_playlists_search_predicate.clear();
	else if (isActiveWindow(Content))
		m_content_search_predicate.clear();
}

bool PlaylistEditor::search(SearchDirection direction, bool wrap, bool skip_current)
{
	bool result = false;
	if (isActiveWindow(Playlists))
		result = ::search(Playlists, m_playlists_search_predicate, direction, wrap, skip_current);
	else if (isActiveWindow(Content))
		result = ::search(Content, m_content_search_predicate, direction, wrap, skip_current);
	return result;
}

/***********************************************************************/

bool PlaylistEditor::allowsFiltering()
{
	return allowsSearching();
}

std::string PlaylistEditor::currentFilter()
{
	std::string result;
	if (isActiveWindow(Playlists))
	{
		if (auto pred = Playlists.filterPredicate<Regex::Filter<MPD::Playlist>>())
			result = pred->constraint();
	}
	else if (isActiveWindow(Content))
	{
		if (auto pred = Content.filterPredicate<Regex::Filter<MPD::Song>>())
			result = pred->constraint();
	}
	return result;
}

void PlaylistEditor::applyFilter(const std::string &constraint)
{
	if (isActiveWindow(Playlists))
	{
		if (!constraint.empty())
		{
			Playlists.applyFilter(Regex::Filter<MPD::Playlist>(
				                      constraint,
				                      Config.regex_type,
				                      PlaylistEntryMatcher));
		}
		else
			Playlists.clearFilter();
	}
	else if (isActiveWindow(Content))
	{
		if (!constraint.empty())
		{
			Content.applyFilter(Regex::Filter<MPD::Song>(
				                    constraint,
				                    Config.regex_type,
				                    SongEntryMatcher));
		}
		else
			Content.clearFilter();
	}
}


/***********************************************************************/

bool PlaylistEditor::itemAvailable()
{
	if (isActiveWindow(Playlists))
		return !Playlists.empty();
	if (isActiveWindow(Content))
		return !Content.empty();
	return false;
}

bool PlaylistEditor::addItemToPlaylist(bool play)
{
	bool result = false;
	if (isActiveWindow(Playlists))
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter_content(ReapplyFilter::No, Content);
		result = addSongsToPlaylist(Content.beginV(), Content.endV(), play, -1);
		Statusbar::printf("Playlist \"%1%\" loaded%2%",
		                  Playlists.current()->value().path(), withErrors(result));
	}
	else if (isActiveWindow(Content))
		result = addSongToPlaylist(Content.current()->value(), play);
	return result;
}

std::vector<MPD::Song> PlaylistEditor::getSelectedSongs()
{
	std::vector<MPD::Song> result;
	if (isActiveWindow(Playlists))
	{
		bool any_selected = false;
		for (auto &e : Playlists)
		{
			if (e.isSelected())
			{
				any_selected = true;
				std::copy(
					std::make_move_iterator(Mpd.GetPlaylistContent(e.value().path())),
					std::make_move_iterator(MPD::SongIterator()),
					std::back_inserter(result));
			}
		}
		// if no item is selected, add songs from right column
		ScopedUnfilteredMenu<MPD::Song> sunfilter_content(ReapplyFilter::No, Content);
		if (!any_selected && !Playlists.empty())
			std::copy(Content.beginV(), Content.endV(), std::back_inserter(result));
	}
	else if (isActiveWindow(Content))
		result = Content.getSelectedSongs();
	return result;
}

/***********************************************************************/

bool PlaylistEditor::previousColumnAvailable()
{
	if (isActiveWindow(Content))
	{
		ScopedUnfilteredMenu<MPD::Playlist> sunfilter_playlists(ReapplyFilter::No, Playlists);
		if (!Playlists.empty())
			return true;
	}
	return false;
}

void PlaylistEditor::previousColumn()
{
	if (isActiveWindow(Content))
	{
		setHighlightInactiveColumnFixes(Content);
		w->refresh();
		w = &Playlists;
		setHighlightFixes(Playlists);
	}
}

bool PlaylistEditor::nextColumnAvailable()
{
	if (isActiveWindow(Playlists))
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter_content(ReapplyFilter::No, Content);
		if (!Content.empty())
			return true;
	}
	return false;
}

void PlaylistEditor::nextColumn()
{
	if (isActiveWindow(Playlists))
	{
		setHighlightInactiveColumnFixes(Playlists);
		w->refresh();
		w = &Content;
		setHighlightFixes(Content);
	}
}

/***********************************************************************/

void PlaylistEditor::updateTimer()
{
	m_timer = Global::Timer;
}

void PlaylistEditor::locatePlaylist(const MPD::Playlist &playlist)
{
	update();
	Playlists.clearFilter();
	auto first = Playlists.beginV(), last = Playlists.endV();
	auto it = std::find(first, last, playlist);
	if (it != last)
	{
		Playlists.highlight(it - first);
		Content.clear();
		Content.clearFilter();
		switchTo();
	}
}

void PlaylistEditor::locateSong(const MPD::Song &s)
{
	if (Playlists.empty())
		return;

	Content.clearFilter();
	Playlists.clearFilter();

	auto locate_song_in_current_playlist = [this, &s](auto front, auto back) {
		if (!Content.empty())
		{
			auto it = std::find(front, back, s);
			if (it != back)
			{
				Content.highlight(it - Content.beginV());
				nextColumn();
				return true;
			}
		}
		return false;
	};
	auto locate_song_in_playlists = [this, &s](auto front, auto back) {
		for (auto it = front; it != back; ++it)
		{
			if (auto song_index = GetSongIndexInPlaylist(*it, s))
			{
				Playlists.highlight(it - Playlists.beginV());
				Playlists.refresh();

				requestContentUpdate();
				update();
				Content.highlight(*song_index);
				nextColumn();

				return true;
			}
		}
		return false;
	};


	if (locate_song_in_current_playlist(Content.currentV() + 1, Content.endV()))
		return;
	Statusbar::print("Jumping to song...");
	if (locate_song_in_playlists(Playlists.currentV() + 1, Playlists.endV()))
		return;
	if (locate_song_in_playlists(Playlists.beginV(), Playlists.currentV()))
		return;
	if (locate_song_in_current_playlist(Content.beginV(), Content.currentV()))
		return;

	// Highlighted song was skipped, so if that's the one we're looking for, we're
	// good.
	if (Content.empty() || *Content.currentV() != s)
		Statusbar::print("Song was not found in playlists");
}

namespace {

std::string SongToString(const MPD::Song &s)
{
	std::string result;
	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Classic:
			result = Format::stringify<char>(Config.song_list_format, &s);
			break;
		case DisplayMode::Columns:
			result = Format::stringify<char>(Config.song_columns_mode_format, &s);
			break;
	}
	return result;
}

bool PlaylistEntryMatcher(const Regex::Regex &rx, const MPD::Playlist &playlist)
{
	return Regex::search(playlist.path(), rx, Config.ignore_diacritics);
}

bool SongEntryMatcher(const Regex::Regex &rx, const MPD::Song &s)
{
	return Regex::search(SongToString(s), rx, Config.ignore_diacritics);
}

boost::optional<size_t> GetSongIndexInPlaylist(MPD::Playlist playlist, const MPD::Song &song)
{
	size_t index = 0;
	MPD::SongIterator it = Mpd.GetPlaylistContentNoInfo(playlist.path()), end;

	for (;;)
	{
		if (it == end)
			return boost::none;
		if (*it == song)
			return index;

		++it, ++index;
	}
}

}
