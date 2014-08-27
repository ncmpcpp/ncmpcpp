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

#ifndef NCMPCPP_SEARCH_ENGINE_H
#define NCMPCPP_SEARCH_ENGINE_H

#include <cassert>

#include "interfaces.h"
#include "mpdpp.h"
#include "screen.h"

struct SearchEngine: Screen<NC::Menu<struct SEItem>>, Filterable, HasSongs, Searchable, Tabbable
{
	SearchEngine();
	
	// Screen< NC::Menu<SEItem> > implementation
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::SearchEngine; }
	
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
	void reset();
	
	static size_t StaticOptions;
	static size_t SearchButton;
	static size_t ResetButton;
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
private:
	void Prepare();
	void Search();
	
	const char **SearchMode;
	
	static const char *SearchModes[];
	
	static const size_t ConstraintsNumber = 11;
	static const char *ConstraintsNames[];
	std::string itsConstraints[ConstraintsNumber];
	
	static bool MatchToPattern;
};

struct SEItem
{
	SEItem() : m_is_song(false), m_buffer(0) { }
	SEItem(NC::Buffer *buf) : m_is_song(false), m_buffer(buf) { }
	SEItem(const MPD::Song &s) : m_is_song(true), m_song(s) { }
	SEItem(const SEItem &ei) { *this = ei; }
	~SEItem() {
		if (!m_is_song)
			delete m_buffer;
	}
	
	NC::Buffer &mkBuffer() {
		assert(!m_is_song);
		delete m_buffer;
		m_buffer = new NC::Buffer();
		return *m_buffer;
	}
	
	bool isSong() const { return m_is_song; }
	
	NC::Buffer &buffer() { assert(!m_is_song && m_buffer); return *m_buffer; }
	MPD::Song &song() { assert(m_is_song); return m_song; }
	
	const NC::Buffer &buffer() const { assert(!m_is_song && m_buffer); return *m_buffer; }
	const MPD::Song &song() const { assert(m_is_song); return m_song; }
	
	SEItem &operator=(const SEItem &se) {
		if (this == &se)
			return *this;
		m_is_song = se.m_is_song;
		if (se.m_is_song)
			m_song = se.m_song;
		else if (se.m_buffer)
			m_buffer = new NC::Buffer(*se.m_buffer);
		else
			m_buffer = 0;
		return *this;
	}
	
private:
	bool m_is_song;
	
	NC::Buffer *m_buffer;
	MPD::Song m_song;
};

extern SearchEngine *mySearcher;

#endif // NCMPCPP_SEARCH_ENGINE_H

