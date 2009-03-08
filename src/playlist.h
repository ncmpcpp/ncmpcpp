/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

class Playlist : public Screen< Menu<MPD::Song> >
{
	public:
		Playlist() : NowPlaying(-1), OldPlaying(-1), itsTotalLength(0), itsRemainingTime(0), itsScrollBegin(0) { }
		~Playlist() { }
		
		virtual void Init();
		virtual void SwitchTo();
		virtual void Resize();
		
		virtual std::string Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed();
		
		virtual MPD::Song *CurrentSong();
		
		virtual bool allowsSelection() { return true; }
		virtual void ReverseSelection() { w->ReverseSelection(); }
		virtual void GetSelectedSongs(MPD::SongList &);
		
		virtual void ApplyFilter(const std::string &);
		
		virtual List *GetList() { return w; }
		
		bool isPlaying() { return NowPlaying >= 0 && !w->Empty(); }
		const MPD::Song *NowPlayingSong();
		
		void Sort();
		void FixPositions(size_t = 0);
		
		static std::string SongToString(const MPD::Song &, void *);
		static std::string SongInColumnsToString(const MPD::Song &, void *);
		
		int NowPlaying;
		int OldPlaying;
		
		static bool ReloadTotalLength;
		static bool ReloadRemaining;
		
		static bool BlockNowPlayingUpdate;
		static bool BlockUpdate;
		static bool BlockRefreshing;
		
	private:
		std::string TotalLength();
		
		std::string itsBufferedStats;
		
		size_t itsTotalLength;
		size_t itsRemainingTime;
		size_t itsScrollBegin;
		
		static void ShowTime(std::ostringstream &, size_t);
		static bool Sorting(MPD::Song *a, MPD::Song *b);
		
		static Menu< std::pair<std::string, MPD::Song::GetFunction> > *SortDialog;
		
		static const size_t SortOptions;
		static const size_t SortDialogWidth;
		static const size_t SortDialogHeight;
};

extern Playlist *myPlaylist;

#endif

