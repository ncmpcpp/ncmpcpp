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

class Playlist : public Screen<NC::Window>, public Filterable, public HasSongs, public Searchable
{
	public:
		Playlist() : itsTotalLength(0), itsRemainingTime(0), itsScrollBegin(0) { }
		~Playlist() { }
		
		// Screen<NC::Window> implementation
		virtual void SwitchTo() OVERRIDE;
		virtual void Resize() OVERRIDE;
		
		virtual std::wstring Title() OVERRIDE;
		
		virtual void Update() OVERRIDE { }
		
		virtual void EnterPressed() OVERRIDE;
		virtual void SpacePressed() OVERRIDE;
		virtual void MouseButtonPressed(MEVENT me) OVERRIDE;
		
		virtual bool isTabbable() OVERRIDE { return true; }
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
		virtual std::shared_ptr<ProxySongList> getProxySongList() OVERRIDE;
		
		virtual bool allowsSelection() OVERRIDE;
		virtual void reverseSelection() OVERRIDE;
		virtual MPD::SongList getSelectedSongs() OVERRIDE;
		
		// private members
		MPD::Song nowPlayingSong();
		
		bool isFiltered();
		
		void Sort();
		void Reverse();
		bool SortingInProgress();
		
		void EnableHighlighting();
		void UpdateTimer();
		timeval Timer() const { return itsTimer; }
		
		bool Add(const MPD::Song &s, bool play, int position = -1);
		bool Add(const MPD::SongList &l, bool play, int position = -1);
		void PlayNewlyAddedSongs();
		
		void SetSelectedItemsPriority(int prio);
		
		bool checkForSong(const MPD::Song &s);
		
		void moveSortOrderUp();
		void moveSortOrderDown();
		
		void registerHash(size_t hash);
		void unregisterHash(size_t hash);
		
		NC::Menu< MPD::Song > *Items;
		
		static bool ReloadTotalLength;
		static bool ReloadRemaining;
		
	protected:
		virtual void Init() OVERRIDE;
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

