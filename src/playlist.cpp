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
#include <sstream>

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "menu.h"
#include "playlist.h"
#include "regex_filter.h"
#include "song.h"
#include "status.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "title.h"

using namespace std::placeholders;

using Global::MainHeight;
using Global::MainStartY;

Playlist *myPlaylist;

bool Playlist::ReloadTotalLength = 0;
bool Playlist::ReloadRemaining = false;

namespace {//

std::string songToString(const MPD::Song &s);
bool playlistEntryMatcher(const Regex &rx, const MPD::Song &s);

}

Playlist::Playlist() : itsTotalLength(0), itsRemainingTime(0), itsScrollBegin(0)
{
	w = NC::Menu<MPD::Song>(0, MainStartY, COLS, MainHeight, Config.columns_in_playlist && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::brNone);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setHighlightColor(Config.main_highlight_color);
	w.setSelectedPrefix(Config.selected_item_prefix);
	w.setSelectedSuffix(Config.selected_item_suffix);
	if (Config.columns_in_playlist)
		w.setItemDisplayer(std::bind(Display::SongsInColumns, _1, this));
	else
		w.setItemDisplayer(std::bind(Display::Songs, _1, this, Config.song_list_format));
}

void Playlist::switchTo()
{
	using Global::myScreen;
	using Global::myLockedScreen;
	using Global::myInactiveScreen;
	
	if (myScreen == this)
		return;
	
	itsScrollBegin = 0;
	
	if (myLockedScreen)
		updateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
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
		w.scroll(NC::wDown);
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
	w.showAll();
	auto rx = RegexFilter<MPD::Song>(filter, Config.regex_type, playlistEntryMatcher);
	w.filter(w.begin(), w.end(), rx);
}

/***********************************************************************/

bool Playlist::allowsSearching()
{
	return true;
}

bool Playlist::search(const std::string &constraint)
{
	auto rx = RegexFilter<MPD::Song>(constraint, Config.regex_type, playlistEntryMatcher);
	return w.search(w.begin(), w.end(), rx);
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

std::shared_ptr<ProxySongList> Playlist::getProxySongList()
{
	return mkProxySongList(w, [](NC::Menu<MPD::Song>::Item &item) {
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
	if (Mpd.isPlaying())
		withUnfilteredMenu(w, [this, &s]() {
			s = w.at(Mpd.GetCurrentSongPos()).value();
		});
	return s;
}

bool Playlist::isFiltered()
{
	if (w.isFiltered())
	{
		Statusbar::msg("Function currently unavailable due to filtered playlist");
		return true;
	}
	return false;
}

void Playlist::Reverse()
{
	if (isFiltered())
		return;
	Statusbar::msg("Reversing playlist order...");
	size_t beginning = -1, end = -1;
	for (size_t i = 0; i < w.size(); ++i)
	{
		if (w.at(i).isSelected())
		{
			if (beginning == size_t(-1))
				beginning = i;
			end = i;
		}
	}
	if (beginning == size_t(-1)) // no selected items
	{
		beginning = 0;
		end = w.size();
	}
	Mpd.StartCommandsList();
	for (size_t i = beginning, j = end-1; i < (beginning+end)/2; ++i, --j)
		Mpd.Swap(i, j);
	if (Mpd.CommitCommandsList())
		 Statusbar::msg("Playlist reversed");
}

void Playlist::EnableHighlighting()
{
	w.setHighlighting(true);
	UpdateTimer();
}

void Playlist::UpdateTimer()
{
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
		for (size_t i = Mpd.GetCurrentlyPlayingSongPos(); i < w.size(); ++i)
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
			result << " (out of " << Mpd.GetPlaylistLength() << ")";
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

void Playlist::PlayNewlyAddedSongs()
{
	// FIXME for removal
	bool is_filtered = w.isFiltered();
	w.showAll();
	size_t old_size = w.size();
	Mpd.UpdateStatus();
	if (old_size < w.size())
		Mpd.Play(old_size);
	if (is_filtered)
		w.showFiltered();
}

void Playlist::SetSelectedItemsPriority(int prio)
{
	auto list = getSelectedOrCurrent(w.begin(), w.end(), w.currentI());
	Mpd.StartCommandsList();
	for (auto it = list.begin(); it != list.end(); ++it)
		Mpd.SetPriority((*it)->value(), prio);
	if (Mpd.CommitCommandsList())
		Statusbar::msg("Priority set");
}

bool Playlist::checkForSong(const MPD::Song &s)
{
	return itsSongHashes.find(s.getHash()) != itsSongHashes.end();
}

void Playlist::registerHash(size_t hash)
{
	itsSongHashes[hash] += 1;
}

void Playlist::unregisterHash(size_t hash)
{
	auto it = itsSongHashes.find(hash);
	assert(it != itsSongHashes.end());
	if (it->second == 1)
		itsSongHashes.erase(it);
	else
		it->second -= 1;
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

bool playlistEntryMatcher(const Regex &rx, const MPD::Song &s)
{
	return rx.match(songToString(s));
}

}
