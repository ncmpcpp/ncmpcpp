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

#include <algorithm>
#include <map>

#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "global.h"
#include "media_library.h"
#include "mpdpp.h"
#include "playlist.h"
#include "status_checker.h"

using namespace MPD;
using namespace Global;
using std::string;

Window *Global::wLibActiveCol;
Menu<string> *Global::mLibArtists;
Menu<string_pair> *Global::mLibAlbums;
Menu<Song> *Global::mLibSongs;

namespace MediaLibrary
{
	size_t left_col_width;
	size_t middle_col_width;
	size_t middle_col_startx;
	size_t right_col_width;
	size_t right_col_startx;
}

void MediaLibrary::Init()
{
	left_col_width = COLS/3-1;
	middle_col_width = COLS/3;
	middle_col_startx = left_col_width+1;
	right_col_width = COLS-COLS/3*2-1;
	right_col_startx = left_col_width+middle_col_width+2;
	
	mLibArtists = new Menu<string>(0, main_start_y, left_col_width, main_height, IntoStr(Config.media_lib_primary_tag) + "s", Config.main_color, brNone);
	mLibArtists->HighlightColor(Config.active_column_color);
	mLibArtists->SetTimeout(ncmpcpp_window_timeout);
	mLibArtists->SetItemDisplayer(Display::Generic);
	
	mLibAlbums = new Menu<string_pair>(middle_col_startx, main_start_y, middle_col_width, main_height, "Albums", Config.main_color, brNone);
	mLibAlbums->HighlightColor(Config.main_highlight_color);
	mLibAlbums->SetTimeout(ncmpcpp_window_timeout);
	mLibAlbums->SetItemDisplayer(Display::StringPairs);
	
	mLibSongs = new Menu<Song>(right_col_startx, main_start_y, right_col_width, main_height, "Songs", Config.main_color, brNone);
	mLibSongs->HighlightColor(Config.main_highlight_color);
	mLibSongs->SetTimeout(ncmpcpp_window_timeout);
	mLibSongs->SetSelectPrefix(&Config.selected_item_prefix);
	mLibSongs->SetSelectSuffix(&Config.selected_item_suffix);
	mLibSongs->SetItemDisplayer(Display::Songs);
	mLibSongs->SetItemDisplayerUserData(&Config.song_library_format);
	
	wLibActiveCol = mLibArtists;
}

void MediaLibrary::Resize()
{
	left_col_width = COLS/3-1;
	middle_col_startx = left_col_width+1;
	middle_col_width = COLS/3;
	right_col_startx = left_col_width+middle_col_width+2;
	right_col_width = COLS-COLS/3*2-1;
	
	mLibArtists->Resize(left_col_width, main_height);
	mLibAlbums->Resize(middle_col_width, main_height);
	mLibSongs->Resize(right_col_width, main_height);
	
	mLibAlbums->MoveTo(middle_col_startx, main_start_y);
	mLibSongs->MoveTo(right_col_startx, main_start_y);
}

void MediaLibrary::Refresh()
{
	mLibArtists->Display();
	mvvline(main_start_y, middle_col_startx-1, 0, main_height);
	mLibAlbums->Display();
	mvvline(main_start_y, right_col_startx-1, 0, main_height);
	mLibSongs->Display();
	if (mLibAlbums->Empty())
	{
		mLibAlbums->WriteXY(0, 0, 0, "No albums found.");
		mLibAlbums->Refresh();
	}
}

void MediaLibrary::SwitchTo()
{
	if (current_screen != csLibrary
#	ifdef HAVE_TAGLIB_H
	&&  current_screen != csTinyTagEditor
#	endif // HAVE_TAGLIB_H
	   )
	{
		CLEAR_FIND_HISTORY;
		
		myPlaylist->Main()->Hide(); // hack, should be wCurrent, but it doesn't always have 100% width
		
//		redraw_screen = 1;
		redraw_header = 1;
		MediaLibrary::Refresh();
				
		wCurrent = wLibActiveCol;
		current_screen = csLibrary;
				
		UpdateSongList(mLibSongs);
	}
}

void MediaLibrary::Update()
{
	if (mLibArtists->Empty())
	{
		CLEAR_FIND_HISTORY;
		TagList list;
		mLibAlbums->Clear(0);
		mLibSongs->Clear(0);
		Mpd->GetList(list, Config.media_lib_primary_tag);
		sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (TagList::iterator it = list.begin(); it != list.end(); it++)
		{
			if (!it->empty())
			{
				utf_to_locale(*it);
				mLibArtists->AddOption(*it);
			}
		}
		mLibArtists->Window::Clear();
		mLibArtists->Refresh();
	}
	
	if (!mLibArtists->Empty() && mLibAlbums->Empty() && mLibSongs->Empty())
	{
		mLibAlbums->Reset();
		TagList list;
		std::map<string, string, CaseInsensitiveSorting> maplist;
		locale_to_utf(mLibArtists->Current());
		if (Config.media_lib_primary_tag == MPD_TAG_ITEM_ARTIST)
			Mpd->GetAlbums(mLibArtists->Current(), list);
		else
		{
			Mpd->StartSearch(1);
			Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
			Mpd->StartFieldSearch(MPD_TAG_ITEM_ALBUM);
			Mpd->CommitSearch(list);
		}
		
		// <mpd-0.14 doesn't support searching for empty tag
		if (Mpd->Version() > 13)
		{
			SongList noalbum_list;
			Mpd->StartSearch(1);
			Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, "");
			Mpd->CommitSearch(noalbum_list);
			if (!noalbum_list.empty())
				mLibAlbums->AddOption(std::make_pair("<no album>", ""));
			FreeSongList(noalbum_list);
		}
		
		for (TagList::iterator it = list.begin(); it != list.end(); it++)
		{
			SongList l;
			Mpd->StartSearch(1);
			Mpd->AddSearch(Config.media_lib_primary_tag, mLibArtists->Current());
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, *it);
			Mpd->CommitSearch(l);
			if (!l.empty() && !l[0]->GetAlbum().empty())
			{
				utf_to_locale(*it);
				l[0]->Localize();
				maplist[l[0]->toString(Config.media_lib_album_format)] = *it;
			}
			FreeSongList(l);
		}
		utf_to_locale(mLibArtists->Current());
		for (std::map<string, string>::const_iterator it = maplist.begin(); it != maplist.end(); it++)
			mLibAlbums->AddOption(make_pair(it->first, it->second));
		mLibAlbums->Window::Clear();
		mLibAlbums->Refresh();
	}
	
	if (!mLibArtists->Empty() && wCurrent == mLibAlbums && mLibAlbums->Empty())
	{
		mLibAlbums->HighlightColor(Config.main_highlight_color);
		mLibArtists->HighlightColor(Config.active_column_color);
		wCurrent = wLibActiveCol = mLibArtists;
	}
	
	if (!mLibArtists->Empty() && mLibSongs->Empty())
	{
		mLibSongs->Reset();
		SongList list;
		
		mLibSongs->Clear(0);
		Mpd->StartSearch(1);
		Mpd->AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(mLibArtists->Current()));
		if (mLibAlbums->Empty()) // left for compatibility with <mpd-0.14
		{
			mLibAlbums->WriteXY(0, 0, 0, "No albums found.");
			mLibAlbums->Refresh();
		}
		else
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, locale_to_utf_cpy(mLibAlbums->Current().second));
		Mpd->CommitSearch(list);
		
		sort(list.begin(), list.end(), SortSongsByTrack);
		bool bold = 0;
		
		for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
		{
			for (size_t j = 0; j < myPlaylist->Main()->Size(); j++)
			{
				if ((*it)->GetHash() == myPlaylist->Main()->at(j).GetHash())
				{
					bold = 1;
					break;
				}
			}
			mLibSongs->AddOption(**it, bold);
			bold = 0;
		}
		FreeSongList(list);
		mLibSongs->Window::Clear();
		mLibSongs->Refresh();
	}
}

void MediaLibrary::EnterPressed(bool add_n_play)
{
	SongList list;
	
	if (!mLibArtists->Empty() && wCurrent == mLibArtists)
	{
		Mpd->StartSearch(1);
		Mpd->AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(mLibArtists->Current()));
		Mpd->CommitSearch(list);
		for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
			Mpd->QueueAddSong(**it);
		if (Mpd->CommitQueue())
		{
			string tag_type = IntoStr(Config.media_lib_primary_tag);
			ToLower(tag_type);
			ShowMessage("Adding songs of %s \"%s\"", tag_type.c_str(), mLibArtists->Current().c_str());
			Song *s = &myPlaylist->Main()->at(myPlaylist->Main()->Size()-list.size());
			if (s->GetHash() == list[0]->GetHash())
			{
				if (add_n_play)
					Mpd->PlayID(s->GetID());
			}
			else
				ShowMessage("%s", message_part_of_songs_added);
		}
	}
	else if (wCurrent == mLibAlbums)
	{
		for (size_t i = 0; i < mLibSongs->Size(); i++)
			Mpd->QueueAddSong(mLibSongs->at(i));
		if (Mpd->CommitQueue())
		{
			ShowMessage("Adding songs from album \"%s\"", mLibAlbums->Current().second.c_str());
			Song *s = &myPlaylist->Main()->at(myPlaylist->Main()->Size()-mLibSongs->Size());
			if (s->GetHash() == mLibSongs->at(0).GetHash())
			{
				if (add_n_play)
					Mpd->PlayID(s->GetID());
			}
			else
				ShowMessage("%s", message_part_of_songs_added);
		}
	}
	else if (wCurrent == mLibSongs)
	{
		if (!mLibSongs->Empty())
		{
			block_item_list_update = 1;
			if (Config.ncmpc_like_songs_adding && mLibSongs->isBold())
			{
				long long hash = mLibSongs->Current().GetHash();
				if (add_n_play)
				{
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd->Play(i);
							break;
						}
					}
				}
				else
				{
					block_playlist_update = 1;
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd->QueueDeleteSong(i);
							myPlaylist->Main()->DeleteOption(i);
							i--;
						}
					}
					Mpd->CommitQueue();
					mLibSongs->BoldOption(mLibSongs->Choice(), 0);
				}
			}
			else
			{
				Song &s = mLibSongs->Current();
				int id = Mpd->AddSong(s);
				if (id >= 0)
				{
					ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
					if (add_n_play)
						Mpd->PlayID(id);
					mLibSongs->BoldOption(mLibSongs->Choice(), 1);
				}
			}
		}
	}
	FreeSongList(list);
	if (!add_n_play)
	{
		wCurrent->Scroll(wDown);
		if (wCurrent == mLibArtists)
		{
			mLibAlbums->Clear(0);
			mLibSongs->Clear(0);
		}
		else if (wCurrent == mLibAlbums)
			mLibSongs->Clear(0);
	}
}

