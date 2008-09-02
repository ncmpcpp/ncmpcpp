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

#ifndef HAVE_HELPERS_H
#define HAVE_HELPERS_H

#include <algorithm>

#include "ncmpcpp.h"
#include "mpdpp.h"
#include "settings.h"
#include "song.h"

extern ncmpcpp_config Config;

void UpdateItemList(Menu<Item> *);
void UpdateSongList(Menu<Song> *);
void UpdateFoundList(const SongList &, Menu<string> *);

void DeleteSong(int);
void PlaylistDeleteSong(const string &, int);
bool MoveSongUp(int);
bool MoveSongDown(int);
bool PlaylistMoveSongUp(const string &, int);
bool PlaylistMoveSongDown(const string &, int);

string DisplayKeys(int *, int = 2);
bool Keypressed(int, const int *);
bool SortSongsByTrack(Song *, Song *);
bool CaseInsensitiveComparison(string, string);
void WindowTitle(const string &);
string TotalPlaylistLength();
string DisplayItem(const Item &, void * = NULL);
string DisplaySong(const Song &, void * = &Config.song_list_format);
void ShowMessage(const string &, int = Config.message_delay_time);
bool SortDirectory(const Item &a, const Item &b);
void GetDirectory(string);
bool GetSongInfo(Song &);
void PrepareSearchEngine(Song &s);
void Search(SongList &, Song &);

#endif

