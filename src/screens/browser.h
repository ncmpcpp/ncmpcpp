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

#ifndef NCMPCPP_BROWSER_H
#define NCMPCPP_BROWSER_H

#include "interfaces.h"
#include "mpdpp.h"
#include "regex_filter.h"
#include "screens/screen.h"
#include "song_list.h"

struct BrowserWindow: NC::Menu<MPD::Item>, SongList
{
	BrowserWindow() { }
	BrowserWindow(NC::Menu<MPD::Item> &&base)
	: NC::Menu<MPD::Item>(std::move(base)) { }

	virtual SongIterator currentS() override;
	virtual ConstSongIterator currentS() const override;
	virtual SongIterator beginS() override;
	virtual ConstSongIterator beginS() const override;
	virtual SongIterator endS() override;
	virtual ConstSongIterator endS() const override;

	virtual std::vector<MPD::Song> getSelectedSongs() override;
};

struct Browser: Screen<BrowserWindow>, Filterable, HasSongs, Searchable, Tabbable
{
	Browser();
	
	// Screen<BrowserWindow> implementation
	virtual void resize() override;
	virtual void switchTo() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::Browser; }
	
	virtual void update() override;
	
	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override { return true; }
	virtual bool isMergable() override { return true; }
	
	// Searchable implementation
	virtual bool allowsSearching() override;
	virtual const std::string &searchConstraint() override;
	virtual void setSearchConstraint(const std::string &constraint) override;
	virtual void clearSearchConstraint() override;
	virtual bool search(SearchDirection direction, bool wrap, bool skip_current) override;

	// Filterable implemenetation
	virtual bool allowsFiltering() override;
	virtual std::string currentFilter() override;
	virtual void applyFilter(const std::string &filter) override;

	// HasSongs implementation
	virtual bool itemAvailable() override;
	virtual bool addItemToPlaylist(bool play) override;
	virtual std::vector<MPD::Song> getSelectedSongs() override;
	
	// private members
	void requestUpdate() { m_update_request = true; }

	bool inRootDirectory();
	bool isParentDirectory(const MPD::Item &item);
	const std::string &currentDirectory();
	
	bool isLocal() { return m_local_browser; }
	void locateSong(const MPD::Song &s);
	bool enterDirectory();
	void getDirectory(std::string directory);
	void changeBrowseMode();
	void remove(const MPD::Item &item);

	static void fetchSupportedExtensions();

private:
	bool m_update_request;
	bool m_local_browser;
	size_t m_scroll_beginning;
	std::string m_current_directory;
	Regex::Filter<MPD::Item> m_search_predicate;
};

extern Browser *myBrowser;

#endif // NCMPCPP_BROWSER_H

