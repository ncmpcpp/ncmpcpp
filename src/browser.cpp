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

#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "settings.h"
#include "status_checker.h"
#ifdef HAVE_TAGLIB_H
# include "tag_editor.h"
#endif // HAVE_TAGLIB_H

using namespace Global;
using namespace MPD;
using std::string;

Menu<Item> *Global::mBrowser;

void Browser::Init()
{
	mBrowser = new Menu<Item>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mBrowser->HighlightColor(Config.main_highlight_color);
	mBrowser->SetTimeout(ncmpcpp_window_timeout);
	mBrowser->SetSelectPrefix(&Config.selected_item_prefix);
	mBrowser->SetSelectSuffix(&Config.selected_item_suffix);
	mBrowser->SetItemDisplayer(Display::Items);
}

void Browser::Resize()
{
	mBrowser->Resize(COLS, main_height);
}

void Browser::SwitchTo()
{
	if (current_screen != csBrowser
#	ifdef HAVE_TAGLIB_H
	&&  current_screen != csTinyTagEditor
#	endif // HAVE_TAGLIB_H
	   )
	{
		CLEAR_FIND_HISTORY;
		mBrowser->Empty() ? GetDirectory(browsed_dir) : UpdateItemList(mBrowser);
		wCurrent = mBrowser;
		wCurrent->Hide();
		current_screen = csBrowser;
//		redraw_screen = 1;
		redraw_header = 1;
	}
}

void Browser::EnterPressed()
{
	if (mBrowser->Empty())
		return;
	
	const Item &item = mBrowser->Current();
	switch (item.type)
	{
		case itDirectory:
		{
			CLEAR_FIND_HISTORY;
			GetDirectory(item.name, browsed_dir);
			redraw_header = 1;
			break;
		}
		case itSong:
		{
			block_item_list_update = 1;
			if (Config.ncmpc_like_songs_adding && mBrowser->isBold())
			{
				bool found = 0;
				long long hash = mBrowser->Current().song->GetHash();
				for (size_t i = 0; i < mPlaylist->Size(); i++)
				{
					if (mPlaylist->at(i).GetHash() == hash)
					{
						Mpd->Play(i);
						found = 1;
						break;
					}
				}
				if (found)
					break;
			}
			Song &s = *item.song;
			int id = Mpd->AddSong(s);
			if (id >= 0)
			{
				Mpd->PlayID(id);
				ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
				mBrowser->BoldOption(mBrowser->Choice(), 1);
			}
			break;
		}
		case itPlaylist:
		{
			SongList list;
			Mpd->GetPlaylistContent(locale_to_utf_cpy(item.name), list);
			for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				Mpd->QueueAddSong(**it);
			if (Mpd->CommitQueue())
			{
				ShowMessage("Loading and playing playlist %s...", item.name.c_str());
				Song *s = &mPlaylist->at(mPlaylist->Size()-list.size());
				if (s->GetHash() == list[0]->GetHash())
					Mpd->PlayID(s->GetID());
				else
					ShowMessage("%s", message_part_of_songs_added);
			}
			FreeSongList(list);
			break;
		}
	}
}

void Browser::SpacePressed()
{
	if (mBrowser->Empty())
		return;
	
	const Item &item = mBrowser->Current();
	switch (item.type)
	{
		case itDirectory:
		{
			if (browsed_dir != "/" && !mBrowser->Choice())
				break; // do not let add parent dir.
			
			if (Config.local_browser)
			{
				ShowMessage("Adding whole directories from local browser is not supported!");
				break;
			}
			
			SongList list;
			Mpd->GetDirectoryRecursive(locale_to_utf_cpy(item.name), list);
			
			for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				Mpd->QueueAddSong(**it);
			if (Mpd->CommitQueue())
			{
				ShowMessage("Added folder: %s", item.name.c_str());
				Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
				if (s.GetHash() != list[0]->GetHash())
					ShowMessage("%s", message_part_of_songs_added);
			}
			FreeSongList(list);
			break;
		}
		case itSong:
		{
			block_item_list_update = 1;
			if (Config.ncmpc_like_songs_adding && mBrowser->isBold())
			{
				block_playlist_update = 1;
				long long hash = mBrowser->Current().song->GetHash();
				for (size_t i = 0; i < mPlaylist->Size(); i++)
				{
					if (mPlaylist->at(i).GetHash() == hash)
					{
						Mpd->QueueDeleteSong(i);
						mPlaylist->DeleteOption(i);
						i--;
					}
				}
				Mpd->CommitQueue();
				mBrowser->BoldOption(mBrowser->Choice(), 0);
			}
			else
			{
				Song &s = *item.song;
				if (Mpd->AddSong(s) != -1)
				{
					ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
					mBrowser->BoldOption(mBrowser->Choice(), 1);
				}
			}
			break;
		}
		case itPlaylist:
		{
			SongList list;
			Mpd->GetPlaylistContent(locale_to_utf_cpy(item.name), list);
			for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
				Mpd->QueueAddSong(**it);
			if (Mpd->CommitQueue())
			{
				ShowMessage("Loading playlist %s...", item.name.c_str());
				Song &s = mPlaylist->at(mPlaylist->Size()-list.size());
				if (s.GetHash() != list[0]->GetHash())
					ShowMessage("%s", message_part_of_songs_added);
			}
			FreeSongList(list);
			break;
		}
	}
	mBrowser->Scroll(wDown);
}

namespace
{
	const char *supported_extensions[] = { "wma", "asf", "rm", "mp1", "mp2", "mp3", "mp4", "m4a", "flac", "ogg", "wav", "au", "aiff", "aif", "ac3", "aac", "mpc", "it", "mod", "s3m", "xm", "wv", 0 };
	
	bool hasSupportedExtension(const string &file)
	{
		size_t last_dot = file.rfind(".");
		if (last_dot > file.length())
			return false;
		
		string ext = file.substr(last_dot+1);
		ToLower(ext);
		for (int i = 0; supported_extensions[i]; i++)
			if (strcmp(ext.c_str(), supported_extensions[i]) == 0)
				return true;
		
		return false;
	}

	void GetLocalDirectory(const string &dir, ItemList &v)
	{
		dirent **list;
		int n = scandir(dir.c_str(), &list, NULL, alphasort);
		
		if (n < 0)
			return;
		
		struct stat file_stat;
		string full_path;
		for (int i = 2; i < n; i++)
		{
			Item new_item;
			full_path = dir;
			if (dir != "/")
				full_path += "/";
			full_path += list[i]->d_name;
			stat(full_path.c_str(), &file_stat);
			if (S_ISDIR(file_stat.st_mode))
			{
				new_item.type = itDirectory;
				new_item.name = full_path;
				v.push_back(new_item);
			}
			else if (hasSupportedExtension(list[i]->d_name))
			{
				new_item.type = itSong;
				mpd_Song *s = mpd_newSong();
				s->file = str_pool_get(full_path.c_str());
#				ifdef HAVE_TAGLIB_H
				ReadTagsFromFile(s);
#				endif // HAVE_TAGLIB_H
				new_item.song = new Song(s);
				v.push_back(new_item);
			}
			delete list[i];
		}
		delete list;
	}
}

void UpdateItemList(Menu<Item> *menu)
{
	bool bold = 0;
	for (size_t i = 0; i < menu->Size(); i++)
	{
		if (menu->at(i).type == itSong)
		{
			for (size_t j = 0; j < mPlaylist->Size(); j++)
			{
				if (mPlaylist->at(j).GetHash() == menu->at(i).song->GetHash())
				{
					bold = 1;
					break;
				}
			}
			menu->BoldOption(i, bold);
			bold = 0;
		}
	}
	menu->Refresh();
}

void GetDirectory(string dir, string subdir)
{
	if (dir.empty())
		dir = "/";
	
	int highlightme = -1;
	browsed_dir_scroll_begin = 0;
	if (browsed_dir != dir)
		mBrowser->Reset();
	browsed_dir = dir;
	
	locale_to_utf(dir);
	
	for (size_t i = 0; i < mBrowser->Size(); i++)
		if (mBrowser->at(i).type == itSong)
			delete mBrowser->at(i).song;
	
	mBrowser->Clear(0);
	
	if (dir != "/")
	{
		Item parent;
		size_t slash = dir.rfind("/");
		parent.song = (Song *) 1; // in that way we assume that's really parent dir
		parent.name = slash != string::npos ? dir.substr(0, slash) : "/";
		parent.type = itDirectory;
		mBrowser->AddOption(parent);
	}
	
	ItemList list;
	Config.local_browser ? GetLocalDirectory(dir, list) : Mpd->GetDirectory(dir, list);
	sort(list.begin(), list.end(), CaseInsensitiveSorting());
	
	for (ItemList::iterator it = list.begin(); it != list.end(); it++)
	{
		switch (it->type)
		{
			case itPlaylist:
			{
				utf_to_locale(it->name);
				mBrowser->AddOption(*it);
				break;
			}
			case itDirectory:
			{
				utf_to_locale(it->name);
				if (it->name == subdir)
					highlightme = mBrowser->Size();
				mBrowser->AddOption(*it);
				break;
			}
			case itSong:
			{
				bool bold = 0;
				for (size_t i = 0; i < mPlaylist->Size(); i++)
				{
					if (mPlaylist->at(i).GetHash() == it->song->GetHash())
					{
						bold = 1;
						break;
					}
				}
				mBrowser->AddOption(*it, bold);
				break;
			}
		}
	}
	if (highlightme >= 0)
		mBrowser->Highlight(highlightme);
	if (current_screen == csBrowser)
		mBrowser->Hide();
}

