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

#ifndef NCMPCPP_SEARCH_ENGINE_H
#define NCMPCPP_SEARCH_ENGINE_H

#include <cassert>

#include "interfaces.h"
#include "mpdpp.h"
#include "regex_filter.h"
#include "screens/screen.h"
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

	virtual SongIterator currentS() override;
	virtual ConstSongIterator currentS() const override;
	virtual SongIterator beginS() override;
	virtual ConstSongIterator beginS() const override;
	virtual SongIterator endS() override;
	virtual ConstSongIterator endS() const override;

	virtual std::vector<MPD::Song> getSelectedSongs() override;
};

struct SearchEngine: Screen<SearchEngineWindow>, Filterable, HasActions, HasSongs, Searchable, Tabbable
{
	SearchEngine();
	
	// Screen<SearchEngineWindow> implementation
	virtual void resize() override;
	virtual void switchTo() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::SearchEngine; }
	
	virtual void update() override { }
	
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

	// HasActions implementation
	virtual bool actionRunnable() override;
	virtual void runAction() override;

	// HasSongs implementation
	virtual bool itemAvailable() override;
	virtual bool addItemToPlaylist(bool play) override;
	virtual std::vector<MPD::Song> getSelectedSongs() override;
	
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

