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

class CaseInsensitiveComparison
{
	public:
		bool operator()(string a, string b)
		{
			transform(a.begin(), a.end(), a.begin(), tolower);
			transform(b.begin(), b.end(), b.begin(), tolower);
			return a < b;
		}
};

void UpdateItemList(Menu<Item> *);
void UpdateSongList(Menu<Song> *);
void UpdateFoundList(const SongList &, Menu<string> *);

string DisplayKeys(int *, int = 2);
bool Keypressed(int, const int *);
bool SortSongsByTrack(Song *, Song *);

void WindowTitle(const string &);
string FindSharedDir(Menu<Song> *);
string FindSharedDir(const SongList &);
string TotalPlaylistLength();
string DisplayTag(const Song &, void *);
string DisplayItem(const Item &, void * = NULL);
string DisplayColumns(string);
string DisplaySongInColumns(const Song &, void *);
string DisplaySong(const Song &, void * = &Config.song_list_format);
void ShowMessage(const string &, int = Config.message_delay_time);
bool SortDirectory(const Item &a, const Item &b);
void GetDirectory(string, string = "/");
#ifdef HAVE_TAGLIB_H
bool WriteTags(Song &);
#endif
bool GetSongInfo(Song &);
void PrepareSearchEngine(Song &s);
void Search(SongList &, Song &);

#endif

