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

class SearchEngine : public Screen< NC::Menu<SEItem> >, public Filterable, public HasSongs, public Searchable
{
	public:
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed();
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		/// Filterable implementation
		virtual bool allowsFiltering();
		virtual std::string currentFilter();
		virtual void applyFilter(const std::string &filter);
		
		/// Searchable implementation
		virtual bool allowsSearching();
		virtual bool search(const std::string &constraint);
		virtual void nextFound(bool wrap);
		virtual void prevFound(bool wrap);
		
		/// HasSongs implementation
		virtual std::shared_ptr<ProxySongList> getProxySongList();
		
		virtual bool allowsSelection();
		virtual void reverseSelection();
		virtual MPD::SongList getSelectedSongs();
		
		virtual bool isMergable() { return true; }
		
		void UpdateFoundList();
		
		static size_t StaticOptions;
		static size_t SearchButton;
		static size_t ResetButton;
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
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

