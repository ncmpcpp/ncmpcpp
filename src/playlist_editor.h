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

#ifndef _PLAYLIST_EDITOR_H
#define _PLAYLIST_EDITOR_H

#include "ncmpcpp.h"

class PlaylistEditor : public Screen<Window>
{
	public:
		virtual void SwitchTo();
		virtual void Resize();
		
		virtual std::string Title();
		
		virtual void Refresh();
		virtual void Update();
		
		virtual void EnterPressed() { AddToPlaylist(1); }
		virtual void SpacePressed();
		virtual void MouseButtonPressed(MEVENT);
		
		virtual MPD::Song *CurrentSong();
		
		virtual bool allowsSelection() { return w == Content; }
		virtual void ReverseSelection() { Content->ReverseSelection(); }
		virtual void GetSelectedSongs(MPD::SongList &);
		
		virtual void ApplyFilter(const std::string &);
		
		virtual List *GetList();
		
		void NextColumn();
		void PrevColumn();
		
		Menu<std::string> *Playlists;
		Menu<MPD::Song> *Content;
		
	protected:
		virtual void Init();
		
	private:
		void AddToPlaylist(bool);
		
		static size_t LeftColumnWidth;
		static size_t RightColumnStartX;
		static size_t RightColumnWidth;
};

extern PlaylistEditor *myPlaylistEditor;

#endif

