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

#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

#include "browser.h"
#include "helpers.h"
#include "settings.h"
#ifdef HAVE_TAGLIB_H
# include "tag_editor.h"
#endif // HAVE_TAGLIB_H

using namespace MPD;

extern Connection *Mpd;

extern ncmpcpp_config Config;

extern Menu<Song> *mPlaylist;
extern Menu<Item> *mBrowser;

extern NcmpcppScreen current_screen;

extern string browsed_dir;

extern int browsed_dir_scroll_begin;

namespace
{
	const string supported_extensions[] = { "wma", "asf", "rm", "mp1", "mp2", "mp3", "mp4", "m4a", "flac", "ogg", "wav", "au", "aiff", "aif", "ac3", "aac", "mpc", "it", "mod", "s3m", "xm", "wv", "." };
	
	bool hasSupportedExtension(const string &file)
	{
		unsigned int last_dot = file.find_last_of(".");
		if (last_dot > file.length())
			return false;
		
		string ext = file.substr(last_dot+1);
		ToLower(ext);
		for (int i = 0; supported_extensions[i] != "."; i++)
			if (ext == supported_extensions[i])
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

void DisplayItem(const Item &item, void *, Menu<Item> *menu)
{
	switch (item.type)
	{
		case itDirectory:
		{
			if (item.song)
			{
				*menu << "[..]";
				return;
			}
			size_t slash = item.name.find_last_of("/");
			*menu << "[" << (slash != string::npos ? item.name.substr(slash+1) : item.name) << "]";
			return;
		}
		case itSong:
			// I know casting that way is ugly etc., but it works.
			DisplaySong(*item.song, &Config.song_list_format, (Menu<Song> *)menu);
			return;
		case itPlaylist:
			*menu << Config.browser_playlist_prefix << item.name;
			return;
		default:
			return;
	}
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
	
	for (size_t i = 0; i < mBrowser->Size(); i++)
		if (mBrowser->at(i).song != (void *)1)
			delete mBrowser->at(i).song;
	
	mBrowser->Clear(0);
	
	if (dir != "/")
	{
		Item parent;
		size_t slash = dir.find_last_of("/");
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
				mBrowser->AddOption(*it);
				break;
			}
			case itDirectory:
			{
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

