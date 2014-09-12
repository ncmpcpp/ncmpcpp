/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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
#include <boost/bind.hpp>
#include <sstream>

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "menu.h"
#include "playlist.h"
#include "regex_filter.h"
#include "screen_switcher.h"
#include "song.h"
#include "status.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

Playlist *myPlaylist;

namespace {

std::string songToString(const MPD::Song &s);
bool playlistEntryMatcher(const boost::regex &rx, const MPD::Song &s);

}

Playlist::Playlist()
: m_total_length(0), m_remaining_time(0), m_scroll_begin(0)
, m_reload_total_length(false), m_reload_remaining(false)
{
	w = NC::Menu<MPD::Song>(0, MainStartY, COLS, MainHeight, Config.playlist_display_mode == DisplayMode::Columns && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::Border::None);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setHighlightColor(Config.main_highlight_color);
	w.setSelectedPrefix(Config.selected_item_prefix);
	w.setSelectedSuffix(Config.selected_item_suffix);
	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Classic:
			w.setItemDisplayer(boost::bind(Display::Songs, _1, proxySongList(), Config.song_list_format));
			break;
		case DisplayMode::Columns:
			w.setItemDisplayer(boost::bind(Display::SongsInColumns, _1, proxySongList()));
			break;
	}
}

void Playlist::switchTo()
{
	SwitchTo::execute(this);
	m_scroll_begin = 0;
	EnableHighlighting();
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

void Playlist::enterPressed()
{
	if (!w.empty())
		Mpd.PlayID(w.current().value().getID());
}

void Playlist::spacePressed()
{
	if (!w.empty())
	{
		w.current().setSelected(!w.current().isSelected());
		w.scroll(NC::Scroll::Down);
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
				enterPressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/***********************************************************************/

bool Playlist::allowsFiltering()
{
	return true;
}

std::string Playlist::currentFilter()
{
	return RegexFilter<MPD::Song>::currentFilter(w);
}

void Playlist::applyFilter(const std::string &filter)
{
	if (filter.empty())
	{
		w.clearFilter();
		w.clearFilterResults();
		return;
	}
	try
	{
		w.showAll();
		auto rx = RegexFilter<MPD::Song>(
			boost::regex(filter, Config.regex_type), playlistEntryMatcher);
		w.filter(w.begin(), w.end(), rx);
	}
	catch (boost::bad_expression &) { }
}

/***********************************************************************/

bool Playlist::allowsSearching()
{
	return true;
}

bool Playlist::search(const std::string &constraint)
{
	if (constraint.empty())
	{
		w.clearSearchResults();
		return false;
	}
	try
	{
		auto rx = RegexFilter<MPD::Song>(
			boost::regex(constraint, Config.regex_type), playlistEntryMatcher);
		return w.search(w.begin(), w.end(), rx);
	}
	catch (boost::bad_expression &)
	{
		return false;
	}
}

void Playlist::nextFound(bool wrap)
{
	w.nextFound(wrap);
}

void Playlist::prevFound(bool wrap)
{
	w.prevFound(wrap);
}

/***********************************************************************/

ProxySongList Playlist::proxySongList()
{
	return ProxySongList(w, [](NC::Menu<MPD::Song>::Item &item) {
		return &item.value();
	});
}

bool Playlist::allowsSelection()
{
	return true;
}

void Playlist::reverseSelection()
{
	reverseSelectionHelper(w.begin(), w.end());
}

MPD::SongList Playlist::getSelectedSongs()
{
	MPD::SongList result;
	for (auto it = w.begin(); it != w.end(); ++it)
		if (it->isSelected())
			result.push_back(it->value());
	if (result.empty() && !w.empty())
		result.push_back(w.current().value());
	return result;
}

/***********************************************************************/

MPD::Song Playlist::nowPlayingSong()
{
	MPD::Song s;
	if (Status::State::player() != MPD::psStop)
		withUnfilteredMenu(w, [this, &s]() {
			s = w.at(Status::State::currentSongPosition()).value();
		});
	return s;
}

bool Playlist::isFiltered()
{
	if (w.isFiltered())
	{
		Statusbar::print("Function currently unavailable due to filtered playlist");
		return true;
	}
	return false;
}

void Playlist::Reverse()
{
	Statusbar::print("Reversing playlist order...");
	auto begin = w.begin(), end = w.end();
	std::tie(begin, end) = getSelectedRange(begin, end);
	Mpd.StartCommandsList();
	for (--end; begin < end; ++begin, --end)
		Mpd.Swap(begin->value().getPosition(), end->value().getPosition());
	Mpd.CommitCommandsList();
	Statusbar::print("Playlist reversed");
}

void Playlist::EnableHighlighting()
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
	if (Config.playlist_show_remaining_time && m_reload_remaining && !w.isFiltered())
	{
		m_remaining_time = 0;
		for (size_t i = Status::State::currentSongPosition(); i < w.size(); ++i)
			m_remaining_time += w[i].value().getDuration();
		m_reload_remaining = false;
	}
	
	result << '(' << w.size() << (w.size() == 1 ? " item" : " items");
	
	if (w.isFiltered())
	{
		w.showAll();
		size_t real_size = w.size();
		w.showFiltered();
		if (w.size() != real_size)
			result << " (out of " << real_size << ")";
	}
	
	if (m_total_length)
	{
		result << ", length: ";
		ShowTime(result, m_total_length, Config.playlist_shorten_total_times);
	}
	if (Config.playlist_show_remaining_time && m_remaining_time && !w.isFiltered() && w.size() > 1)
	{
		result << " :: remaining: ";
		ShowTime(result, m_remaining_time, Config.playlist_shorten_total_times);
	}
	result << ')';
	return result.str();
}

void Playlist::SetSelectedItemsPriority(int prio)
{
	auto list = getSelectedOrCurrent(w.begin(), w.end(), w.currentI());
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

namespace {//

std::string songToString(const MPD::Song &s)
{
	std::string result;
	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Classic:
			result = s.toString(Config.song_list_format_dollar_free, Config.tags_separator);
			break;
		case DisplayMode::Columns:
			result = s.toString(Config.song_in_columns_to_string_format, Config.tags_separator);
	}
	return result;
}

bool playlistEntryMatcher(const boost::regex &rx, const MPD::Song &s)
{
	return boost::regex_search(songToString(s), rx);
}

}
