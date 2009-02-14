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
#include "playlist.h"
#include "settings.h"
#include "status_checker.h"
#ifdef HAVE_TAGLIB_H
# include "tag_editor.h"
#endif // HAVE_TAGLIB_H

using namespace Global;
using namespace MPD;
using std::string;

Browser *myBrowser = new Browser;

void Browser::Init()
{
	w = new Menu<Item>(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	w->HighlightColor(Config.main_highlight_color);
	w->SetTimeout(ncmpcpp_window_timeout);
	w->SetSelectPrefix(&Config.selected_item_prefix);
	w->SetSelectSuffix(&Config.selected_item_suffix);
	w->SetItemDisplayer(Display::Items);
}

void Browser::Resize()
{
	w->Resize(COLS, main_height);
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
		w->Empty() ? myBrowser->GetDirectory(itsBrowsedDir) : myBrowser->UpdateItemList();
		wCurrent = w;
		wCurrent->Hide();
		current_screen = csBrowser;
//		redraw_screen = 1;
		redraw_header = 1;
	}
}

std::string Browser::Title()
{
	string result = "Browse: ";
	result += TO_STRING(Scroller(itsBrowsedDir, COLS-volume_state.length(), itsScrollBeginning));
	return result;
}

void Browser::EnterPressed()
{
	if (w->Empty())
		return;
	
	const Item &item = w->Current();
	switch (item.type)
	{
		case itDirectory:
		{
			CLEAR_FIND_HISTORY;
			GetDirectory(item.name, itsBrowsedDir);
			redraw_header = 1;
			break;
		}
		case itSong:
		{
			block_item_list_update = 1;
			if (Config.ncmpc_like_songs_adding && w->isBold())
			{
				bool found = 0;
				long long hash = w->Current().song->GetHash();
				for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
				{
					if (myPlaylist->Main()->at(i).GetHash() == hash)
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
				w->BoldOption(w->Choice(), 1);
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
				Song *s = &myPlaylist->Main()->at(myPlaylist->Main()->Size()-list.size());
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
	if (w->Empty())
		return;
	
	const Item &item = w->Current();
	switch (item.type)
	{
		case itDirectory:
		{
			if (itsBrowsedDir != "/" && !w->Choice())
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
				Song &s = myPlaylist->Main()->at(myPlaylist->Main()->Size()-list.size());
				if (s.GetHash() != list[0]->GetHash())
					ShowMessage("%s", message_part_of_songs_added);
			}
			FreeSongList(list);
			break;
		}
		case itSong:
		{
			block_item_list_update = 1;
			if (Config.ncmpc_like_songs_adding && w->isBold())
			{
				block_playlist_update = 1;
				long long hash = w->Current().song->GetHash();
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
				w->BoldOption(w->Choice(), 0);
			}
			else
			{
				Song &s = *item.song;
				if (Mpd->AddSong(s) != -1)
				{
					ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
					w->BoldOption(w->Choice(), 1);
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
				Song &s = myPlaylist->Main()->at(myPlaylist->Main()->Size()-list.size());
				if (s.GetHash() != list[0]->GetHash())
					ShowMessage("%s", message_part_of_songs_added);
			}
			FreeSongList(list);
			break;
		}
	}
	w->Scroll(wDown);
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
				TagEditor::ReadTags(s);
#				endif // HAVE_TAGLIB_H
				new_item.song = new Song(s);
				v.push_back(new_item);
			}
			delete list[i];
		}
		delete list;
	}
}

void Browser::GetDirectory(string dir, string subdir)
{
	if (dir.empty())
		dir = "/";
	
	int highlightme = -1;
	itsScrollBeginning = 0;
	if (itsBrowsedDir != dir)
		w->Reset();
	itsBrowsedDir = dir;
	
	locale_to_utf(dir);
	
	for (size_t i = 0; i < w->Size(); i++)
		if (w->at(i).type == itSong)
			delete w->at(i).song;
	
	w->Clear(0);
	
	if (dir != "/")
	{
		Item parent;
		size_t slash = dir.rfind("/");
		parent.song = (Song *) 1; // in that way we assume that's really parent dir
		parent.name = slash != string::npos ? dir.substr(0, slash) : "/";
		parent.type = itDirectory;
		w->AddOption(parent);
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
				w->AddOption(*it);
				break;
			}
			case itDirectory:
			{
				utf_to_locale(it->name);
				if (it->name == subdir)
					highlightme = w->Size();
				w->AddOption(*it);
				break;
			}
			case itSong:
			{
				bool bold = 0;
				for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
				{
					if (myPlaylist->Main()->at(i).GetHash() == it->song->GetHash())
					{
						bold = 1;
						break;
					}
				}
				w->AddOption(*it, bold);
				break;
			}
		}
	}
	if (highlightme >= 0)
		w->Highlight(highlightme);
	if (current_screen == csBrowser)
		w->Hide();
}

void Browser::ChangeBrowseMode()
{
	if (Mpd->GetHostname()[0] != '/')
		return;
	
	Config.local_browser = !Config.local_browser;
	ShowMessage("Browse mode: %s", Config.local_browser ? "Local filesystem" : "MPD music dir");
	itsBrowsedDir = Config.local_browser ? home_folder : "/";
	w->Reset();
	GetDirectory(itsBrowsedDir);
	redraw_header = 1;
}

void Browser::UpdateItemList()
{
	bool bold = 0;
	for (size_t i = 0; i < w->Size(); i++)
	{
		if (w->at(i).type == itSong)
		{
			for (size_t j = 0; j < myPlaylist->Main()->Size(); j++)
			{
				if (myPlaylist->Main()->at(j).GetHash() == w->at(i).song->GetHash())
				{
					bold = 1;
					break;
				}
			}
			w->BoldOption(i, bold);
			bold = 0;
		}
	}
	w->Refresh();
}

