/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ncmpcpp.h"

#ifdef HAVE_TAGLIB_H

#ifndef HAVE_TAG_EDITOR_H
#define HAVE_TAG_EDITOR_H

// taglib headers
#include "fileref.h"
#include "tag.h"

#include "mpdpp.h"
#include "settings.h"

typedef void (Song::*SongSetFunction)(const string &);

string FindSharedDir(Menu<Song> *);
string FindSharedDir(const SongList &);
string DisplayTag(const Song &, void *, const Menu<Song> *);

string IntoStr(mpd_TagItems);
SongSetFunction IntoSetFunction(mpd_TagItems);

bool GetSongTags(Song &);
bool WriteTags(Song &);

void __deal_with_filenames(SongList &);

#endif

#endif

