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

#ifndef NCMPCPP_DISPLAY_H
#define NCMPCPP_DISPLAY_H

#include "interfaces.h"
#include "format.h"
#include "menu.h"
#include "mutable_song.h"
#include "screens/search_engine.h"
#include "song_list.h"

namespace Display {

std::string Columns(size_t);

void SongsInColumns(NC::Menu<MPD::Song> &menu, const SongList &list);

void Songs(NC::Menu<MPD::Song> &menu, const SongList &list, const Format::AST<char> &ast);

void Tags(NC::Menu<MPD::MutableSong> &menu);

void SEItems(NC::Menu<SEItem> &menu, const SongList &list);

void Items(NC::Menu<MPD::Item> &menu, const SongList &list);

}

#endif // NCMPCPP_DISPLAY_H

