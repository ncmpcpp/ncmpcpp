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

#include "mpdpp.h"
#include "ncmpcpp.h"
#include "settings.h"

extern ncmpcpp_config Config;

class CaseInsensitiveSorting
{
	public:
		bool operator()(string, string);
		bool operator()(Song *, Song *);
		bool operator()(const Item &, const Item &);
};

bool SortSongsByTrack(Song *, Song *);

void UpdateItemList(Menu<Item> *);
void UpdateSongList(Menu<Song> *);

string DisplayKeys(int *, int = 2);
bool Keypressed(int, const int *);

void WindowTitle(const string &);

string TotalPlaylistLength();
string DisplayStringPair(const StringPair &, void * = NULL);
string DisplayItem(const Item &, void * = NULL);
string DisplayColumns(string);
string DisplaySongInColumns(const Song &, void *);
string DisplaySong(const Song &, void * = &Config.song_list_format);
void ShowMessage(const string &, int = Config.message_delay_time);
void GetDirectory(string, string = "/");

#endif

