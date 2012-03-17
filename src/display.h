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

#include "ncmpcpp.h"
#include "menu.h"
#include "mpdpp.h"
#include "screen.h"

namespace Display
{
	struct ScreenFormat
	{
		BasicScreen *screen;
		std::string *format;
	};
	
	std::string Columns(size_t);
	
	template <typename T> void Generic(const T &t, void *, Menu<T> *menu)
	{
		*menu << t;
	}
	
	template <typename A, typename B> void Pairs(const std::pair<A, B> &pair, void *, Menu< std::pair<A, B> > *menu)
	{
		*menu << pair.first;
	}
	
	void SongsInColumns(const MPD::Song &, void *, Menu<MPD::Song> *);
	
	void Songs(const MPD::Song &, void *, Menu<MPD::Song> *);
	
	void Tags(const MPD::Song &, void *, Menu<MPD::Song> *);
	
	void SearchEngine(const std::pair<Buffer *, MPD::Song *> &, void *, Menu< std::pair<Buffer *, MPD::Song *> > *);
	
	void Items(const MPD::Item &, void *, Menu<MPD::Item> *);
}

#endif

