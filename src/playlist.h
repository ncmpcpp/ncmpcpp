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

#include <sstream>

#include "ncmpcpp.h"
#include "screen.h"
#include "song.h"

class Playlist : public Screen<Window>
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
		virtual void ReadKey(int &);
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		virtual MPD::Song *CurrentSong();
		virtual MPD::Song *GetSong(size_t pos) { return w == Items ? &Items->at(pos) : 0; }
		
		virtual bool allowsSelection() { return w == Items; }
		virtual void ReverseSelection() { Items->ReverseSelection(); }
		virtual void GetSelectedSongs(MPD::SongList &);
		
		virtual void ApplyFilter(const std::string &);
		
		virtual List *GetList() { return w == Items ? Items : 0; }
		
		virtual bool isMergable() { return true; }
		
		bool isFiltered();
		bool isPlaying() { return NowPlaying >= 0 && !Items->Empty(); }
		const MPD::Song *NowPlayingSong();
		
		void MoveSelectedItems(Movement where);
		
		void Sort();
		void Reverse();
		void AdjustSortOrder(Movement where);
		bool SortingInProgress() { return w == SortDialog; }
		
		void EnableHighlighting();
		void UpdateTimer() { time(&itsTimer); }
		time_t Timer() const { return itsTimer; }
		
		bool Add(const MPD::Song &s, bool in_playlist, bool play, int position = -1);
		bool Add(const MPD::SongList &l, bool play, int position = -1);
		void PlayNewlyAddedSongs();
		
		void SetSelectedItemsPriority(int prio);
		
		static std::string SongToString(const MPD::Song &, void *);
		static std::string SongInColumnsToString(const MPD::Song &, void *);
		
		Menu< MPD::Song > *Items;
		
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
		
		// stuff for sorting playlist
		static void QuickSort(MPD::SongList::iterator first, MPD::SongList::iterator last, MPD::SongList::iterator begin);
		inline static void IterSwap(MPD::SongList::iterator a, MPD::SongList::iterator b, MPD::SongList::iterator begin)
		{
			iter_swap(a, b);
			Mpd.Swap(a-begin, b-begin);
		}
		inline static bool SongComp(MPD::Song *a, MPD::Song *b)
		{
			CaseInsensitiveStringComparison cmp;
			for (size_t i = 0; i < SortOptions; ++i)
				if (int ret = cmp(a->GetTags((*SortDialog)[i].second), b->GetTags((*SortDialog)[i].second)))
					return ret < 0;
			return a->GetPosition() < b->GetPosition();
		}
		
		static Menu< std::pair<std::string, MPD::Song::GetFunction> > *SortDialog;
		static const size_t SortOptions;
		static const size_t SortDialogWidth;
		static size_t SortDialogHeight;
};

extern Playlist *myPlaylist;

#endif

