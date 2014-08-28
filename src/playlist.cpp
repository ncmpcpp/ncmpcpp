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

bool Playlist::ReloadTotalLength = 0;
bool Playlist::ReloadRemaining = false;

namespace {//

std::string songToString(const MPD::Song &s);
bool playlistEntryMatcher(const boost::regex &rx, const MPD::Song &s);

}

Playlist::Playlist()
: itsTotalLength(0), itsRemainingTime(0), itsScrollBegin(0), m_old_playlist_version(0)
{
	w = NC::Menu<MPD::Song>(0, MainStartY, COLS, MainHeight, Config.columns_in_playlist && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::Border::None);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setHighlightColor(Config.main_highlight_color);
	w.setSelectedPrefix(Config.selected_item_prefix);
	w.setSelectedSuffix(Config.selected_item_suffix);
	if (Config.columns_in_playlist)
		w.setItemDisplayer(boost::bind(Display::SongsInColumns, _1, proxySongList()));
	else
		w.setItemDisplayer(boost::bind(Display::Songs, _1, proxySongList(), Config.song_list_format));
}

void Playlist::switchTo()
{
	SwitchTo::execute(this);
	itsScrollBegin = 0;
	EnableHighlighting();
	drawHeader();
}

void Playlist::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);

	if (Config.columns_in_playlist && Config.titles_visibility)
		w.setTitle(Display::Columns(w.getWidth()));
	else
		w.setTitle("");
	
	hasToBeResized = 0;
}

std::wstring Playlist::title()
{
	std::wstring result = L"Playlist ";
	if (ReloadTotalLength || ReloadRemaining)
		itsBufferedStats = TotalLength();
	result += Scroller(ToWString(itsBufferedStats), itsScrollBegin, COLS-result.length()-(Config.new_design ? 2 : Global::VolumeState.length()));
	return result;
}

void Playlist::update()
{
	if (w.isHighlighted()
	&&  Config.playlist_disable_highlight_delay.time_duration::seconds() > 0
	&&  Global::Timer - itsTimer > Config.playlist_disable_highlight_delay)
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
			s = w.at(currentSongPosition()).value();
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
	itsTimer = Global::Timer;
}

std::string Playlist::TotalLength()
{
	std::ostringstream result;
	
	if (ReloadTotalLength)
	{
		itsTotalLength = 0;
		for (size_t i = 0; i < w.size(); ++i)
			itsTotalLength += w[i].value().getDuration();
		ReloadTotalLength = 0;
	}
	if (Config.playlist_show_remaining_time && ReloadRemaining && !w.isFiltered())
	{
		itsRemainingTime = 0;
		for (size_t i = currentSongPosition(); i < w.size(); ++i)
			itsRemainingTime += w[i].value().getDuration();
		ReloadRemaining = false;
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
	
	if (itsTotalLength)
	{
		result << ", length: ";
		ShowTime(result, itsTotalLength, Config.playlist_shorten_total_times);
	}
	if (Config.playlist_show_remaining_time && itsRemainingTime && !w.isFiltered() && w.size() > 1)
	{
		result << " :: remaining: ";
		ShowTime(result, itsRemainingTime, Config.playlist_shorten_total_times);
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

void Playlist::setStatus(MPD::Status status)
{
	m_status = status;
}

unsigned Playlist::oldVersion() const
{
	return m_old_playlist_version;
}

int Playlist::currentSongPosition() const
{
	return m_status.empty() ? -1 : m_status.currentSongPosition();
}

unsigned Playlist::currentSongLength() const
{
	return m_status.empty() ? 0 : m_status.totalTime();
}

bool Playlist::checkForSong(const MPD::Song &s)
{
	return itsSongHashes.find(s.getHash()) != itsSongHashes.end();
}

void Playlist::registerHash(size_t hash)
{
	++itsSongHashes[hash];
}

void Playlist::unregisterHash(size_t hash)
{
	auto it = itsSongHashes.find(hash);
	assert(it != itsSongHashes.end());
	if (it->second == 1)
		itsSongHashes.erase(it);
	else
		--it->second;
}

namespace {//

std::string songToString(const MPD::Song &s)
{
	std::string result;
	if (Config.columns_in_playlist)
		result = s.toString(Config.song_in_columns_to_string_format, Config.tags_separator);
	else
		result = s.toString(Config.song_list_format_dollar_free, Config.tags_separator);
	return result;
}

bool playlistEntryMatcher(const boost::regex &rx, const MPD::Song &s)
{
	return boost::regex_search(songToString(s), rx);
}

}
