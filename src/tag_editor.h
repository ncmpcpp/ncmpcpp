/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include "ncmpcpp.h"

#ifdef HAVE_TAGLIB_H

#ifndef _TAG_EDITOR_H
#define _TAG_EDITOR_H

// taglib headers
#include "fileref.h"
#include "tag.h"

#include "mpdpp.h"
#include "settings.h"

typedef void (Song::*SongSetFunction)(const std::string &);
typedef std::string (Song::*SongGetFunction)() const;

std::string FindSharedDir(Menu<Song> *);
std::string FindSharedDir(const MPD::SongList &);
void DisplayTag(const Song &, void *, Menu<Song> *);

SongSetFunction IntoSetFunction(mpd_TagItems);

void ReadTagsFromFile(mpd_Song *);
bool GetSongTags(Song &);
bool WriteTags(Song &);

void __deal_with_filenames(MPD::SongList &);

void CapitalizeFirstLetters(Song &);
void LowerAllLetters(Song &);

#endif

#endif

