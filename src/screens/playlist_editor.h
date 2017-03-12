/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_PLAYLIST_EDITOR_H
#define NCMPCPP_PLAYLIST_EDITOR_H

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "interfaces.h"
#include "regex_filter.h"
#include "screens/screen.h"
#include "song_list.h"

struct PlaylistEditor: Screen<NC::Window *>, Filterable, HasColumns, HasSongs, Searchable, Tabbable
{
	PlaylistEditor();
	
	virtual void switchTo() override;
	virtual void resize() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::PlaylistEditor; }
	
	virtual void refresh() override;
	virtual void update() override;
	
	virtual int windowTimeout() override;

	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override { return true; }
	virtual bool isMergable() override { return true; }
	
	// Searchable implementation
	virtual bool allowsSearching() override;
	virtual const std::string &searchConstraint() override;
	virtual void setSearchConstraint(const std::string &constraint) override;
	virtual void clearSearchConstraint() override;
	virtual bool search(SearchDirection direction, bool wrap, bool skip_current) override;

	// Filterable implementation
	virtual bool allowsFiltering() override;
	virtual std::string currentFilter() override;
	virtual void applyFilter(const std::string &filter) override;

	// HasSongs implementation
	virtual bool itemAvailable() override;
	virtual bool addItemToPlaylist(bool play) override;
	virtual std::vector<MPD::Song> getSelectedSongs() override;
	
	// HasColumns implementation
	virtual bool previousColumnAvailable() override;
	virtual void previousColumn() override;
	
	virtual bool nextColumnAvailable() override;
	virtual void nextColumn() override;
	
	// private members
	void updateTimer();

	void requestPlaylistsUpdate() { m_playlists_update_requested = true; }
	void requestContentUpdate() { m_content_update_requested = true; }
	
	void locatePlaylist(const MPD::Playlist &playlist);
	void locateSong(const MPD::Song &s);

	NC::Menu<MPD::Playlist> Playlists;
	SongMenu Content;
	
private:
	bool m_playlists_update_requested;
	bool m_content_update_requested;

	boost::posix_time::ptime m_timer;

	const int m_window_timeout;
	const boost::posix_time::time_duration m_fetching_delay;

	Regex::Filter<MPD::Playlist> m_playlists_search_predicate;
	Regex::Filter<MPD::Song> m_content_search_predicate;
};

extern PlaylistEditor *myPlaylistEditor;

#endif // NCMPCPP_PLAYLIST_EDITOR_H

