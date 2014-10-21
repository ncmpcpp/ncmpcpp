/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_MEDIA_LIBRARY_H
#define NCMPCPP_MEDIA_LIBRARY_H

#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "interfaces.h"
#include "screen.h"

struct MediaLibrary: Screen<NC::Window *>, Filterable, HasColumns, HasSongs, Searchable, Tabbable
{
	MediaLibrary();
	
	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::MediaLibrary; }
	
	virtual void refresh() OVERRIDE;
	virtual void update() OVERRIDE;
	
	virtual int windowTimeout() OVERRIDE;

	virtual void enterPressed() OVERRIDE;
	virtual void spacePressed() OVERRIDE;
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;
	
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
	virtual ProxySongList proxySongList() OVERRIDE;
	
	virtual bool allowsSelection() OVERRIDE;
	virtual void reverseSelection() OVERRIDE;
	virtual MPD::SongList getSelectedSongs() OVERRIDE;
	
	// HasColumns implementation
	virtual bool previousColumnAvailable() OVERRIDE;
	virtual void previousColumn() OVERRIDE;
	
	virtual bool nextColumnAvailable() OVERRIDE;
	virtual void nextColumn() OVERRIDE;
	
	// private members
	void updateTimer();
	void toggleColumnsMode();
	int Columns();
	void LocateSong(const MPD::Song &);
	ProxySongList songsProxyList();
	void toggleSortMode();
	
	void requestTagsUpdate() { m_tags_update_request = true; }
	void requestAlbumsUpdate() { m_albums_update_request = true; }
	void requestSongsUpdate() { m_songs_update_request = true; }
	
	struct PrimaryTag
	{
		PrimaryTag() : m_mtime(0) { }
		PrimaryTag(std::string tag_, time_t mtime_)
		: m_tag(std::move(tag_)), m_mtime(mtime_) { }
		
		const std::string &tag() const { return m_tag; }
		time_t mtime() const { return m_mtime; }
		
	private:
		std::string m_tag;
		time_t m_mtime;
	};
	
	struct Album
	{
		Album(std::string tag_, std::string album_, std::string date_, time_t mtime_)
		: m_tag(std::move(tag_)), m_album(std::move(album_))
		, m_date(std::move(date_)), m_mtime(mtime_) { }
		
		const std::string &tag() const { return m_tag; }
		const std::string &album() const { return m_album; }
		const std::string &date() const { return m_date; }
		time_t mtime() const { return m_mtime; }
		
	private:
		std::string m_tag;
		std::string m_album;
		std::string m_date;
		time_t m_mtime;
	};
	
	struct AlbumEntry
	{
		AlbumEntry() : m_all_tracks_entry(false), m_album("", "", "", 0) { }
		AlbumEntry(Album album_) : m_all_tracks_entry(false), m_album(album_) { }
		
		const Album &entry() const { return m_album; }
		bool isAllTracksEntry() const { return m_all_tracks_entry; }
		
		static AlbumEntry mkAllTracksEntry(std::string tag) {
			auto result = AlbumEntry(Album(tag, "", "", 0));
			result.m_all_tracks_entry = true;
			return result;
		}
		
	private:
		bool m_all_tracks_entry;
		Album m_album;
	};
	
	NC::Menu<PrimaryTag> Tags;
	NC::Menu<AlbumEntry> Albums;
	NC::Menu<MPD::Song> Songs;
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
private:
	void AddToPlaylist(bool);
	
	bool m_tags_update_request;
	bool m_albums_update_request;
	bool m_songs_update_request;

	boost::posix_time::ptime m_timer;

	const int m_window_timeout;
	const boost::posix_time::time_duration m_fetching_delay;
};

extern MediaLibrary *myLibrary;

#endif // NCMPCPP_MEDIA_LIBRARY_H

