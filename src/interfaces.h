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

#ifndef NCMPCPP_INTERFACES_H
#define NCMPCPP_INTERFACES_H

#include <string>
#include "gcc.h"
#include "screen.h"
#include "song.h"
#include "proxy_song_list.h"

struct Filterable
{
	virtual bool allowsFiltering() = 0;
	virtual std::string currentFilter() = 0;
	virtual void applyFilter(const std::string &filter) = 0;
};

struct Searchable
{
	virtual bool allowsSearching() = 0;
	virtual bool search(const std::string &constraint) = 0;
	virtual void nextFound(bool wrap) = 0;
	virtual void prevFound(bool wrap) = 0;
};

struct HasSongs
{
	virtual ProxySongList proxySongList() = 0;
	
	virtual bool allowsSelection() = 0;
	virtual void reverseSelection() = 0;
	virtual MPD::SongList getSelectedSongs() = 0;
};

struct HasColumns
{
	virtual bool previousColumnAvailable() = 0;
	virtual void previousColumn() = 0;
	
	virtual bool nextColumnAvailable() = 0;
	virtual void nextColumn() = 0;
};

struct Tabbable
{
	Tabbable() : m_previous_screen(0) { }
	
	void switchToPreviousScreen() const {
		if (m_previous_screen)
			m_previous_screen->switchTo();
	}
	void setPreviousScreen(BaseScreen *screen) {
		m_previous_screen = screen;
	}
	BaseScreen *previousScreen() const {
		return m_previous_screen;
	}
	
private:
	BaseScreen *m_previous_screen;
};

#endif // NCMPCPP_INTERFACES_H
