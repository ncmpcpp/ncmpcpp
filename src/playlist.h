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

#include "ncmpcpp.h"
#include "screen.h"
#include "song.h"

class Playlist : public Screen< Menu<MPD::Song> >
{
	public:
		Playlist() : NowPlaying(-1), OldPlaying(-1) { }
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
		
		virtual List *GetList() { return w; }
		
		bool isPlaying() { return NowPlaying >= 0 && !w->Empty(); }
		const MPD::Song &NowPlayingSong();
		
		int NowPlaying;
		int OldPlaying;
		
		static bool BlockNowPlayingUpdate;
		static bool BlockUpdate;
		
	protected:
		std::string TotalLength();
};

extern Playlist *myPlaylist;

#endif

