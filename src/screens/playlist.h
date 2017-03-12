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

#ifndef NCMPCPP_PLAYLIST_H
#define NCMPCPP_PLAYLIST_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <unordered_map>

#include "interfaces.h"
#include "regex_filter.h"
#include "screens/screen.h"
#include "song.h"
#include "song_list.h"

struct Playlist: Screen<SongMenu>, Filterable, HasSongs, Searchable, Tabbable
{
	Playlist();
	
	// Screen<SongMenu> implementation
	virtual void switchTo() override;
	virtual void resize() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::Playlist; }
	
	virtual void update() override;
	
	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override { return true; }
	virtual bool isMergable() override { return true; }
	
	// Searchable implementation
	virtual bool allowsSearching() override;
	virtual const std::string &searchConstraint() override;
	virtual void setSearchConstraint(const std::string &constraint) override;
	virtual void clearSearchConstraint() override;
	virtual bool search(SearchDirection direction, bool wrap, bool skip_current) override;

	// Filterable implementation
	virtual bool allowsFiltering() override;
	virtual std::string currentFilter() override;
	virtual void applyFilter(const std::string &filter) override;
	
	// HasSongs implementation
	virtual bool itemAvailable() override;
	virtual bool addItemToPlaylist(bool play) override;
	virtual std::vector<MPD::Song> getSelectedSongs() override;
	
	// other members
	MPD::Song nowPlayingSong();

	// Locate song in playlist.
	void locateSong(const MPD::Song &s);

	void enableHighlighting();
	
	void setSelectedItemsPriority(int prio);

	bool checkForSong(const MPD::Song &s);
	void registerSong(const MPD::Song &s);
	void unregisterSong(const MPD::Song &s);
	
	void reloadTotalLength() { m_reload_total_length = true; }
	void reloadRemaining() { m_reload_remaining = true; }
	
private:
	std::string getTotalLength();

	std::string m_stats;
	
	std::unordered_map<MPD::Song, int, MPD::Song::Hash> m_song_refs;
	
	size_t m_total_length;;
	size_t m_remaining_time;
	size_t m_scroll_begin;
	
	boost::posix_time::ptime m_timer;

	bool m_reload_total_length;
	bool m_reload_remaining;

	Regex::Filter<MPD::Song> m_search_predicate;
};

extern Playlist *myPlaylist;

#endif // NCMPCPP_PLAYLIST_H

