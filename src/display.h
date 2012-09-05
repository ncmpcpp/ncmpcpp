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

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "interfaces.h"
#include "menu.h"
#include "mutable_song.h"
#include "search_engine.h"

namespace Display
{
	std::string Columns(size_t);
	
	template <typename T> void Default(NC::Menu<T> &menu)
	{
		menu << menu.drawn()->value();
	}
	
	template <typename A, typename B> void Pair(NC::Menu< std::pair<A, B> > &menu)
	{
		menu << menu.drawn()->value().first;
	}
	
	void SongsInColumns(NC::Menu<MPD::Song> &menu, HasSongs *screen);
	
	void Songs(NC::Menu<MPD::Song> &menu, HasSongs *screen, const std::string &format);
	
	void Tags(NC::Menu<MPD::MutableSong> &menu);
	
	void Outputs(NC::Menu<MPD::Output> &menu);
	
	void SearchEngine(NC::Menu<SEItem> &menu);
	
	void Items(NC::Menu<MPD::Item> &menu);
}

#endif

