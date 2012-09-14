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

struct PlaylistEditor : public Screen<NC::Window *>, public Filterable, public HasColumns, public HasSongs, public Searchable
{
	PlaylistEditor();
	
	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	
	virtual void refresh() OVERRIDE;
	virtual void update() OVERRIDE;
	
	virtual void enterPressed() OVERRIDE;
	virtual void spacePressed() OVERRIDE;
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;
	
	virtual bool isTabbable() OVERRIDE { return true; }
	virtual bool isMergable() OVERRIDE { return true; }
	
	// Filterable implementation
	virtual bool allowsFiltering() OVERRIDE;
	virtual std::string currentFilter() OVERRIDE;
	virtual void applyFilter(const std::string &filter) OVERRIDE;
	
	// Searchable implementation
	virtual bool allowsSearching() OVERRIDE;
	virtual bool search(const std::string &constraint) OVERRIDE;
	virtual void nextFound(bool wrap) OVERRIDE;
	virtual void prevFound(bool wrap) OVERRIDE;
	
	// HasSongs implementation
	virtual std::shared_ptr<ProxySongList> getProxySongList() OVERRIDE;
	
	virtual bool allowsSelection() OVERRIDE;
	virtual void reverseSelection() OVERRIDE;
	virtual MPD::SongList getSelectedSongs() OVERRIDE;
	
	// HasColumns implementation
	virtual bool previousColumnAvailable() OVERRIDE;
	virtual void previousColumn() OVERRIDE;
	
	virtual bool nextColumnAvailable() OVERRIDE;
	virtual void nextColumn() OVERRIDE;
	
	// private members
	void requestPlaylistsUpdate() { playlistsUpdateRequested = true; }
	void requestContentsUpdate() { contentUpdateRequested = true; }
	
	virtual void Locate(const std::string &);
	bool isContentFiltered();
	std::shared_ptr<ProxySongList> contentProxyList();
	
	NC::Menu<std::string> Playlists;
	NC::Menu<MPD::Song> Content;
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
private:
	void AddToPlaylist(bool);
	
	bool playlistsUpdateRequested;
	bool contentUpdateRequested;
};

extern PlaylistEditor *myPlaylistEditor;

#endif

