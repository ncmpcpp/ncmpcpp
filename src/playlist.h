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

#ifndef NCMPCPP_PLAYLIST_H
#define NCMPCPP_PLAYLIST_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <unordered_map>

#include "interfaces.h"
#include "screen.h"
#include "song.h"

struct Playlist: Screen<NC::Menu<MPD::Song>>, Filterable, HasSongs, Searchable, Tabbable
{
	Playlist();
	
	// Screen<NC::Menu<MPD::Song>> implementation
	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::Playlist; }
	
	virtual void update() OVERRIDE;
	
	virtual void enterPressed() OVERRIDE;
	virtual void spacePressed() OVERRIDE;
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;
	
	virtual bool isMergable() OVERRIDE { return true; }
	
	// Filterable implementation
	virtual bool allowsFiltering() OVERRIDE;
	virtual std::string currentFilter() OVERRIDE;
	virtual void applyFilter(const std::string &filter) OVERRIDE;
	
	// Searchable implementation
	virtual bool allowsSearching();
	virtual bool search(const std::string &constraint) OVERRIDE;
	virtual void nextFound(bool wrap) OVERRIDE;
	virtual void prevFound(bool wrap) OVERRIDE;
	
	// HasSongs implementation
	virtual ProxySongList proxySongList() OVERRIDE;
	
	virtual bool allowsSelection() OVERRIDE;
	virtual void reverseSelection() OVERRIDE;
	virtual MPD::SongList getSelectedSongs() OVERRIDE;
	
	// private members
	MPD::Song nowPlayingSong();
	
	bool isFiltered();
	void Reverse();
	
	void EnableHighlighting();
	
	void SetSelectedItemsPriority(int prio);
	
	bool checkForSong(const MPD::Song &s);
	void registerSong(const MPD::Song &s);
	void unregisterSong(const MPD::Song &s);
	
	void reloadTotalLength() { m_reload_total_length = true; }
	void reloadRemaining() { m_reload_remaining = true; }
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
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
};

extern Playlist *myPlaylist;

#endif // NCMPCPP_PLAYLIST_H

