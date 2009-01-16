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

#ifndef HAVE_HELPERS_H
#define HAVE_HELPERS_H

#include "mpdpp.h"
#include "ncmpcpp.h"
#include "settings.h"

bool ConnectToMPD();
void ParseArgv(int, char **);

class CaseInsensitiveSorting
{
	public:
		bool operator()(string, string);
		bool operator()(Song *, Song *);
		bool operator()(const MPD::Item &, const MPD::Item &);
};

template <class T> void GenericDisplayer(const T &t, void *, Menu<T> *menu)
{
	*menu << t;
}

bool SortSongsByTrack(Song *, Song *);

void UpdateSongList(Menu<Song> *);

bool Keypressed(int, const int *);

void WindowTitle(const string &);
void EscapeUnallowedChars(string &);

Window &operator<<(Window &, mpd_TagItems);

string IntoStr(mpd_TagItems);
string FindSharedDir(const string &, const string &);
void DisplayTotalPlaylistLength(Window &);
void DisplayStringPair(const StringPair &, void *, Menu<StringPair> *);
string DisplayColumns(string);
void DisplaySongInColumns(const Song &, void *, Menu<Song> *);
void DisplaySong(const Song &, void * = &Config.song_list_format, Menu<Song> * = NULL);
void GetInfo(Song &, Scrollpad &);

Window &Statusbar();

const Buffer &ShowTag(const string &);
const basic_buffer<my_char_t> &ShowTagInInfoScreen(const string &);

void Scroller(Window &, const string &, size_t, size_t &);

#endif

