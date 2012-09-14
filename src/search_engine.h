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

#ifndef _SEARCH_ENGINE_H
#define _SEARCH_ENGINE_H

#include <cassert>

#include "interfaces.h"
#include "mpdpp.h"
#include "screen.h"

struct SEItem
{
	SEItem() : isThisSong(false), itsBuffer(0) { }
	SEItem(NC::Buffer *buf) : isThisSong(false), itsBuffer(buf) { }
	SEItem(const MPD::Song &s) : isThisSong(true), itsSong(s) { }
	SEItem(const SEItem &ei) { *this = ei; }
	~SEItem() {
		if (!isThisSong)
			delete itsBuffer;
	}
	
	NC::Buffer &mkBuffer() {
		assert(!isThisSong);
		delete itsBuffer;
		itsBuffer = new NC::Buffer();
		return *itsBuffer;
	}
	
	bool isSong() const { return isThisSong; }
	
	NC::Buffer &buffer() { assert(!isThisSong && itsBuffer); return *itsBuffer; }
	MPD::Song &song() { assert(isThisSong); return itsSong; }
	
	const NC::Buffer &buffer() const { assert(!isThisSong && itsBuffer); return *itsBuffer; }
	const MPD::Song &song() const { assert(isThisSong); return itsSong; }
	
	SEItem &operator=(const SEItem &se) {
		if (this == &se)
			return *this;
		isThisSong = se.isThisSong;
		if (se.isThisSong)
			itsSong = se.itsSong;
		else if (se.itsBuffer)
			itsBuffer = new NC::Buffer(*se.itsBuffer);
		else
			itsBuffer = 0;
		return *this;
	}
	
	private:
		bool isThisSong;
		
		NC::Buffer *itsBuffer;
		MPD::Song itsSong;
};

struct SearchEngine : public Screen<NC::Menu<SEItem>>, public Filterable, public HasSongs, public Searchable
{
	SearchEngine();
	
	// Screen< NC::Menu<SEItem> > implementation
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	
	virtual void update() OVERRIDE { }
	
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
	
	// private members
	
	static size_t StaticOptions;
	static size_t SearchButton;
	static size_t ResetButton;
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
private:
	void Prepare();
	void Search();
	void reset();
	
	const char **SearchMode;
	
	static const char *SearchModes[];
	
	static const size_t ConstraintsNumber = 11;
	static const char *ConstraintsNames[];
	std::string itsConstraints[ConstraintsNumber];
	
	static bool MatchToPattern;
};

extern SearchEngine *mySearcher;

#endif

