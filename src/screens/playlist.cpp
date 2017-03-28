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
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>

#include "curses/menu_impl.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "screens/playlist.h"
#include "screens/screen_switcher.h"
#include "song.h"
#include "status.h"
#include "statusbar.h"
#include "format_impl.h"
#include "helpers/song_iterator_maker.h"
#include "utility/comparators.h"
#include "utility/functional.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

namespace ph = std::placeholders;

Playlist *myPlaylist;

namespace {

std::string songToString(const MPD::Song &s);
bool playlistEntryMatcher(const Regex::Regex &rx, const MPD::Song &s);

}

Playlist::Playlist()
: m_total_length(0), m_remaining_time(0), m_scroll_begin(0)
, m_timer(boost::posix_time::from_time_t(0))
, m_reload_total_length(false), m_reload_remaining(false)
{
	w = NC::Menu<MPD::Song>(0, MainStartY, COLS, MainHeight, Config.playlist_display_mode == DisplayMode::Columns && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::Border());
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	setHighlightFixes(w);
	w.setSelectedPrefix(Config.selected_item_prefix);
	w.setSelectedSuffix(Config.selected_item_suffix);
	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Classic:
			w.setItemDisplayer(std::bind(
				Display::Songs, ph::_1, std::cref(w), std::cref(Config.song_list_format)
			));
			break;
		case DisplayMode::Columns:
			w.setItemDisplayer(std::bind(
				Display::SongsInColumns, ph::_1, std::cref(w)
			));
			break;
	}
}

void Playlist::switchTo()
{
	SwitchTo::execute(this);
	m_scroll_begin = 0;
	drawHeader();
}

void Playlist::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);

	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Columns:
			if (Config.titles_visibility)
			{
				w.setTitle(Display::Columns(w.getWidth()));
				break;
			}
		case DisplayMode::Classic:
			w.setTitle("");
	}

	hasToBeResized = 0;
}

std::wstring Playlist::title()
{
	std::wstring result = L"Playlist ";
	if (Config.playlist_show_mpd_host)
	{
		result += L"on ";
		result += ToWString(Mpd.GetHostname());
		result += L" ";
	}
	if (m_reload_total_length || m_reload_remaining)
		m_stats = getTotalLength();
	result += Scroller(ToWString(m_stats), m_scroll_begin, COLS-result.length()-(Config.design == Design::Alternative ? 2 : Global::VolumeState.length()));
	return result;
}

void Playlist::update()
{
	if (w.isHighlighted()
	&&  Config.playlist_disable_highlight_delay.time_duration::seconds() > 0
	&&  Global::Timer - m_timer > Config.playlist_disable_highlight_delay)
	{
		w.setHighlighting(false);
		w.refresh();
	}
}

void Playlist::mouseButtonPressed(MEVENT me)
{
	if (!w.empty() && w.hasCoords(me.x, me.y))
	{
		if (size_t(me.y) < w.size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			w.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				addItemToPlaylist(true);
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/***********************************************************************/

bool Playlist::allowsSearching()
{
	return true;
}

const std::string &Playlist::searchConstraint()
{
	return m_search_predicate.constraint();
}

void Playlist::setSearchConstraint(const std::string &constraint)
{
	m_search_predicate = Regex::Filter<MPD::Song>(
		constraint,
		Config.regex_type,
		playlistEntryMatcher);
}

void Playlist::clearSearchConstraint()
{
	m_search_predicate.clear();
}

bool Playlist::search(SearchDirection direction, bool wrap, bool skip_current)
{
	return ::search(w, m_search_predicate, direction, wrap, skip_current);
}

/***********************************************************************/

bool Playlist::allowsFiltering()
{
	return allowsSearching();
}

std::string Playlist::currentFilter()
{
	std::string result;
	if (auto pred = w.filterPredicate<Regex::Filter<MPD::Song>>())
		result = pred->constraint();
	return result;
}

void Playlist::applyFilter(const std::string &constraint)
{
	if (!constraint.empty())
	{
		w.applyFilter(Regex::Filter<MPD::Song>(
			              constraint,
			              Config.regex_type,
			              playlistEntryMatcher));
	}
	else
		w.clearFilter();
}

/***********************************************************************/

bool Playlist::itemAvailable()
{
	return !w.empty();
}

bool Playlist::addItemToPlaylist(bool play)
{
	if (play)
		Mpd.PlayID(w.currentV()->getID());
	return true;
}

std::vector<MPD::Song> Playlist::getSelectedSongs()
{
	return w.getSelectedSongs();
}

/***********************************************************************/

MPD::Song Playlist::nowPlayingSong()
{
	MPD::Song s;
	if (Status::State::player() != MPD::psUnknown)
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter(ReapplyFilter::No, w);
		auto sp = Status::State::currentSongPosition();
		if (sp >= 0 && size_t(sp) < w.size())
			s = w.at(sp).value();
	}
	return s;
}

void Playlist::locateSong(const MPD::Song &s)
{
	if (!w.isFiltered())
		w.highlight(s.getPosition());
	else
	{
		auto cmp = [](const MPD::Song &a, const MPD::Song &b) {
			return a.getPosition() < b.getPosition();
		};
		auto first = w.beginV(), last = w.endV();
		auto it = std::lower_bound(first, last, s, cmp);
		if (it != last && it->getPosition() == s.getPosition())
			w.highlight(it - first);
		else
			Statusbar::print("Song is filtered out");
	}
}

void Playlist::enableHighlighting()
{
	w.setHighlighting(true);
	m_timer = Global::Timer;
}

std::string Playlist::getTotalLength()
{
	std::ostringstream result;
	
	if (m_reload_total_length)
	{
		m_total_length = 0;
		for (const auto &s : w)
			m_total_length += s.value().getDuration();
		m_reload_total_length = false;
	}
	if (Config.playlist_show_remaining_time && m_reload_remaining)
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter(ReapplyFilter::No, w);
		m_remaining_time = 0;
		for (size_t i = Status::State::currentSongPosition(); i < w.size(); ++i)
			m_remaining_time += w[i].value().getDuration();
		m_reload_remaining = false;
	}
	
	result << '(' << w.size() << (w.size() == 1 ? " item" : " items");

	if (w.isFiltered())
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter(ReapplyFilter::No, w);
		result << " (out of " << w.size() << ")";
	}
	
	if (m_total_length)
	{
		result << ", length: ";
		ShowTime(result, m_total_length, Config.playlist_shorten_total_times);
	}
	if (Config.playlist_show_remaining_time && m_remaining_time && w.size() > 1)
	{
		result << ", remaining: ";
		ShowTime(result, m_remaining_time, Config.playlist_shorten_total_times);
	}
	result << ')';
	return result.str();
}

void Playlist::setSelectedItemsPriority(int prio)
{
	auto list = getSelectedOrCurrent(w.begin(), w.end(), w.current());
	Mpd.StartCommandsList();
	for (auto it = list.begin(); it != list.end(); ++it)
		Mpd.SetPriority((*it)->value(), prio);
	Mpd.CommitCommandsList();
	Statusbar::print("Priority set");
}

bool Playlist::checkForSong(const MPD::Song &s)
{
	return m_song_refs.find(s) != m_song_refs.end();
}

void Playlist::registerSong(const MPD::Song &s)
{
	++m_song_refs[s];
}

void Playlist::unregisterSong(const MPD::Song &s)
{
	auto it = m_song_refs.find(s);
	assert(it != m_song_refs.end());
	if (it->second == 1)
		m_song_refs.erase(it);
	else
		--it->second;
}

namespace {

std::string songToString(const MPD::Song &s)
{
	std::string result;
	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Classic:
			result = Format::stringify<char>(Config.song_list_format, &s);
			break;
		case DisplayMode::Columns:
			result = Format::stringify<char>(Config.song_columns_mode_format, &s);
	}
	return result;
}

bool playlistEntryMatcher(const Regex::Regex &rx, const MPD::Song &s)
{
	return Regex::search(songToString(s), rx, Config.ignore_diacritics);
}

}
