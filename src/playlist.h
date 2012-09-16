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

#ifndef _PLAYLIST_H
#define _PLAYLIST_H

#include <map>

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
	
	virtual void update() OVERRIDE { }
	
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
	void UpdateTimer();
	timeval Timer() const { return itsTimer; }
	
	void PlayNewlyAddedSongs();
	
	void SetSelectedItemsPriority(int prio);
	
	bool checkForSong(const MPD::Song &s);
	
	void registerHash(size_t hash);
	void unregisterHash(size_t hash);
	
	static bool ReloadTotalLength;
	static bool ReloadRemaining;
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
private:
	std::string TotalLength();
	std::string itsBufferedStats;
	
	std::map<size_t, int> itsSongHashes;
	
	size_t itsTotalLength;
	size_t itsRemainingTime;
	size_t itsScrollBegin;
	
	timeval itsTimer;
};

extern Playlist *myPlaylist;

#endif

