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

#include "interfaces.h"
#include "screen.h"
#include "song.h"

class Playlist : public Screen<NC::Window>, public Filterable, public HasSongs, public Searchable
{
	public:
		enum Movement { mUp, mDown };
		
		Playlist() : NowPlaying(-1), itsTotalLength(0), itsRemainingTime(0), itsScrollBegin(0) { }
		~Playlist() { }
		
		virtual void SwitchTo();
		virtual void Resize();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed();
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		/// Filterable implementation
		virtual bool allowsFiltering();
		virtual std::string currentFilter();
		virtual void applyFilter(const std::string &filter);
		
		/// Searchable implementation
		virtual bool allowsSearching();
		virtual bool search(const std::string &constraint);
		virtual void nextFound(bool wrap);
		virtual void prevFound(bool wrap);
		
		/// HasSongs implementation
		virtual MPD::Song *getSong(size_t pos);
		virtual MPD::Song *currentSong();
		virtual std::shared_ptr<ProxySongList> getProxySongList();
		
		virtual bool allowsSelection();
		virtual void reverseSelection();
		virtual MPD::SongList getSelectedSongs();
		
		virtual bool isMergable() { return true; }
		
		bool isFiltered();
		bool isPlaying() { return NowPlaying >= 0 && !Items->empty(); }
		const MPD::Song *NowPlayingSong();
		
		void MoveSelectedItems(Movement where);
		
		void Sort();
		void Reverse();
		void AdjustSortOrder(Movement where);
		bool SortingInProgress();
		
		void EnableHighlighting();
		void UpdateTimer() { time(&itsTimer); }
		time_t Timer() const { return itsTimer; }
		
		bool Add(const MPD::Song &s, bool in_playlist, bool play, int position = -1);
		bool Add(const MPD::SongList &l, bool play, int position = -1);
		void PlayNewlyAddedSongs();
		
		void SetSelectedItemsPriority(int prio);
		
		bool checkForSong(const MPD::Song &s);
		
		//static std::string SongToString(const MPD::Song &s);
		//static std::string SongInColumnsToString(const MPD::Song &s);
		
		NC::Menu< MPD::Song > *Items;
		
		int NowPlaying;
		
		static bool ReloadTotalLength;
		static bool ReloadRemaining;
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		std::string TotalLength();
		std::string itsBufferedStats;
		
		size_t itsTotalLength;
		size_t itsRemainingTime;
		size_t itsScrollBegin;
		
		time_t itsTimer;
};

extern Playlist *myPlaylist;

#endif

