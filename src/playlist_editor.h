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

#include "interfaces.h"
#include "screen.h"

class PlaylistEditor : public Screen<NC::Window>, public Filterable, public HasSongs, public Searchable
{
	public:
		virtual void SwitchTo() OVERRIDE;
		virtual void Resize() OVERRIDE;
		
		virtual std::basic_string<my_char_t> Title() OVERRIDE;
		
		virtual void Refresh() OVERRIDE;
		virtual void Update() OVERRIDE;
		
		virtual void EnterPressed() OVERRIDE;
		virtual void SpacePressed() OVERRIDE;
		virtual void MouseButtonPressed(MEVENT me) OVERRIDE;
		
		virtual bool isTabbable() OVERRIDE { return true; }
		virtual bool isMergable() OVERRIDE { return true; }
		
		/// Filterable implementation
		virtual bool allowsFiltering() OVERRIDE;
		virtual std::string currentFilter() OVERRIDE;
		virtual void applyFilter(const std::string &filter) OVERRIDE;
		
		/// Searchable implementation
		virtual bool allowsSearching() OVERRIDE;
		virtual bool search(const std::string &constraint) OVERRIDE;
		virtual void nextFound(bool wrap) OVERRIDE;
		virtual void prevFound(bool wrap) OVERRIDE;
		
		/// HasSongs implementation
		virtual std::shared_ptr<ProxySongList> getProxySongList() OVERRIDE;
		
		virtual bool allowsSelection() OVERRIDE;
		virtual void reverseSelection() OVERRIDE;
		virtual MPD::SongList getSelectedSongs() OVERRIDE;
		
		// private members
		virtual void Locate(const std::string &);
		
		void requestPlaylistsUpdate() { playlistsUpdateRequested = true; }
		void requestContentsUpdate() { contentUpdateRequested = true; }
		
		bool isContentFiltered();
		bool isNextColumnAvailable();
		bool NextColumn();
		bool isPrevColumnAvailable();
		bool PrevColumn();
		
		std::shared_ptr<ProxySongList> contentProxyList();
		
		NC::Menu<std::string> *Playlists;
		NC::Menu<MPD::Song> *Content;
		
	protected:
		virtual void Init() OVERRIDE;
		virtual bool isLockable() OVERRIDE { return true; }
		
	private:
		void AddToPlaylist(bool);
		
		bool playlistsUpdateRequested;
		bool contentUpdateRequested;
};

extern PlaylistEditor *myPlaylistEditor;

#endif

