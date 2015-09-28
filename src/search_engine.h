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
#include "regex_filter.h"
#include "screen.h"
#include "song_list.h"

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

struct SearchEngineWindow: NC::Menu<SEItem>, SongList
{
	SearchEngineWindow() { }
	SearchEngineWindow(NC::Menu<SEItem> &&base)
	: NC::Menu<SEItem>(std::move(base)) { }

	virtual SongIterator currentS() OVERRIDE;
	virtual ConstSongIterator currentS() const OVERRIDE;
	virtual SongIterator beginS() OVERRIDE;
	virtual ConstSongIterator beginS() const OVERRIDE;
	virtual SongIterator endS() OVERRIDE;
	virtual ConstSongIterator endS() const OVERRIDE;

	virtual std::vector<MPD::Song> getSelectedSongs() OVERRIDE;
};

struct SearchEngine: Screen<SearchEngineWindow>, HasActions, HasSongs, Searchable, Tabbable
{
	SearchEngine();
	
	// Screen<SearchEngineWindow> implementation
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::SearchEngine; }
	
	virtual void update() OVERRIDE { }
	
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;
	
	virtual bool isLockable() OVERRIDE { return true; }
	virtual bool isMergable() OVERRIDE { return true; }
	
	// Searchable implementation
	virtual bool allowsSearching() OVERRIDE;
	virtual void setSearchConstraint(const std::string &constraint) OVERRIDE;
	virtual void clearConstraint() OVERRIDE;
	virtual bool find(SearchDirection direction, bool wrap, bool skip_current) OVERRIDE;

	// HasActions implementation
	virtual bool actionRunnable() OVERRIDE;
	virtual void runAction() OVERRIDE;

	// HasSongs implementation
	virtual bool itemAvailable() OVERRIDE;
	virtual bool addItemToPlaylist(bool play) OVERRIDE;
	virtual std::vector<MPD::Song> getSelectedSongs() OVERRIDE;
	
	// private members
	void reset();
	
	static size_t StaticOptions;
	static size_t SearchButton;
	static size_t ResetButton;
	
private:
	void Prepare();
	void Search();

	Regex::ItemFilter<SEItem> m_search_predicate;
	
	const char **SearchMode;
	
	static const char *SearchModes[];
	
	static const size_t ConstraintsNumber = 11;
	static const char *ConstraintsNames[];
	std::string itsConstraints[ConstraintsNumber];
	
	static bool MatchToPattern;
};

extern SearchEngine *mySearcher;

#endif // NCMPCPP_SEARCH_ENGINE_H

