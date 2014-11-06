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

#ifndef NCMPCPP_BROWSER_H
#define NCMPCPP_BROWSER_H

#include "interfaces.h"
#include "mpdpp.h"
#include "regex_filter.h"
#include "screen.h"

struct Browser: Screen<NC::Menu<MPD::Item>>, HasSongs, Searchable, Tabbable
{
	Browser();
	
	// Screen< NC::Menu<MPD::Item> > implementation
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::Browser; }
	
	virtual void update() OVERRIDE { }
	
	virtual void enterPressed() OVERRIDE;
	virtual void spacePressed() OVERRIDE;
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;
	
	virtual bool isMergable() OVERRIDE { return true; }
	
	// Searchable implementation
	virtual bool allowsSearching() OVERRIDE;
	virtual void setSearchConstraint(const std::string &constraint) OVERRIDE;
	virtual void clearConstraint() OVERRIDE;
	virtual bool find(SearchDirection direction, bool wrap, bool skip_current) OVERRIDE;
	
	// HasSongs implementation
	virtual ProxySongList proxySongList() OVERRIDE;
	
	virtual bool allowsSelection() OVERRIDE;
	virtual void reverseSelection() OVERRIDE;
	virtual std::vector<MPD::Song> getSelectedSongs() OVERRIDE;
	
	// private members
	bool inRootDirectory();
	bool isParentDirectory(const MPD::Item &item);
	const std::string &currentDirectory();
	
	bool isLocal() { return m_local_browser; }
	void locateSong(const MPD::Song &s);
	void getDirectory(std::string directory);
	void changeBrowseMode();
	void remove(const MPD::Item &item);

	static void fetchSupportedExtensions();

protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
private:
	bool m_local_browser;
	size_t m_scroll_beginning;
	std::string m_current_directory;
	RegexFilter<MPD::Item> m_search_predicate;
};

extern Browser *myBrowser;

#endif // NCMPCPP_BROWSER_H

