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
#include "screen.h"

struct Browser: Screen<NC::Menu<MPD::Item>>, Filterable, HasSongs, Searchable, Tabbable
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
	
	// private members
	const std::string &CurrentDir() { return itsBrowsedDir; }
	
	void fetchSupportedExtensions();
	
	bool isLocal() { return itsBrowseLocally; }
	void LocateSong(const MPD::Song &);
	void GetDirectory(std::string, std::string = "/");
#	ifndef WIN32
	void GetLocalDirectory(MPD::ItemList &, const std::string &, bool) const;
	void ClearDirectory(const std::string &) const;
	void ChangeBrowseMode();
	bool deleteItem(const MPD::Item &, std::string &errmsg);
#	endif // !WIN32
	
	static bool isParentDirectory(const MPD::Item &item) {
		return item.type == MPD::itDirectory && item.name == "..";
	}
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
private:
	bool itsBrowseLocally;
	size_t itsScrollBeginning;
	std::string itsBrowsedDir;
};

extern Browser *myBrowser;

#endif // NCMPCPP_BROWSER_H

