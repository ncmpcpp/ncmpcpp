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

#ifndef _PLAYLIST_EDITOR_H
#define _PLAYLIST_EDITOR_H

#include "playlist.h"
#include "ncmpcpp.h"

class PlaylistEditor : public Screen<Window>, public Filterable, public HasSongs, public Searchable
{
	public:
		virtual void SwitchTo();
		virtual void Resize();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void Refresh();
		virtual void Update();
		
		virtual void EnterPressed() { AddToPlaylist(1); }
		virtual void SpacePressed();
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		/// Filterable implementation
		virtual std::string currentFilter();
		virtual void applyFilter(const std::string &filter);
		
		/// Searchable implementation
		virtual bool search(const std::string &constraint);
		virtual void nextFound(bool wrap);
		virtual void prevFound(bool wrap);
		
		/// HasSongs implementation
		virtual MPD::Song *getSong(size_t pos);
		virtual MPD::Song *currentSong();
		
		virtual bool allowsSelection();
		virtual void reverseSelection();
		virtual void removeSelection();
		virtual MPD::SongList getSelectedSongs();
		
		virtual void Locate(const std::string &);
		
		virtual List *GetList();
		
		virtual bool isMergable() { return true; }
		
		void MoveSelectedItems(Playlist::Movement where);
		
		bool isContentFiltered();
		bool isNextColumnAvailable();
		bool NextColumn();
		bool isPrevColumnAvailable();
		bool PrevColumn();
		
		Menu<std::string> *Playlists;
		Menu<MPD::Song> *Content;
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		void AddToPlaylist(bool);
};

extern PlaylistEditor *myPlaylistEditor;

#endif

