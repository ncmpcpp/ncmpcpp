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

#ifndef _INTERFACES_H
#define _INTERFACES_H

#include <string>
#include "gcc.h"
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
	virtual std::shared_ptr<ProxySongList> getProxySongList() = 0;
	
	virtual bool allowsSelection() = 0;
	virtual void reverseSelection() = 0;
	virtual MPD::SongList getSelectedSongs() = 0;
};

#endif // _INTERFACES_H
