/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
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
#include <cerrno>
#include <cstring>
#include <algorithm>

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "settings.h"
#include "status.h"
#ifdef HAVE_TAGLIB_H
# include "tag_editor.h"
#endif // HAVE_TAGLIB_H

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;
using Global::RedrawHeader;

using MPD::itDirectory;
using MPD::itSong;
using MPD::itPlaylist;

Browser *myBrowser = new Browser;

std::set<std::string> Browser::SupportedExtensions;

void Browser::Init()
{
	static Display::ScreenFormat sf = { this, &Config.song_list_format };
	
	w = new Menu<MPD::Item>(0, MainStartY, COLS, MainHeight, Config.columns_in_browser && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, brNone);
	w->HighlightColor(Config.main_highlight_color);
	w->CyclicScrolling(Config.use_cyclic_scrolling);
	w->CenteredCursor(Config.centered_cursor);
	w->SetSelectPrefix(&Config.selected_item_prefix);
	w->SetSelectSuffix(&Config.selected_item_suffix);
	w->SetItemDisplayer(Display::Items);
	w->SetItemDisplayerUserData(&sf);
	w->SetGetStringFunction(ItemToString);
	
	if (SupportedExtensions.empty())
		Mpd.GetSupportedExtensions(SupportedExtensions);
	
	isInitialized = 1;
}

void Browser::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->Resize(width, MainHeight);
	w->MoveTo(x_offset, MainStartY);
	w->SetTitle(Config.columns_in_browser && Config.titles_visibility ? Display::Columns(w->GetWidth()) : "");
	hasToBeResized = 0;
}

void Browser::SwitchTo()
{
	using Global::myLockedScreen;
	using Global::myInactiveScreen;
	
	if (myScreen == this)
	{
#		ifndef WIN32
		myBrowser->ChangeBrowseMode();
#		endif // !WIN32
	}
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (isLocal()) // local browser doesn't support sorting by mtime
		Config.browser_sort_by_mtime = 0;
	
	w->Empty() ? myBrowser->GetDirectory(itsBrowsedDir) : myBrowser->UpdateItemList();

	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	RedrawHeader = 1;
}

std::basic_string<my_char_t> Browser::Title()
{
	std::basic_string<my_char_t> result = U("Browse: ");
	result += Scroller(TO_WSTRING(itsBrowsedDir), itsScrollBeginning, COLS-result.length()-(Config.new_design ? 2 : Global::VolumeState.length()));
	return result;
}

void Browser::EnterPressed()
{
	if (w->Empty())
		return;
	
	const MPD::Item &item = w->Current();
	switch (item.type)
	{
		case itDirectory:
		{
			GetDirectory(item.name, itsBrowsedDir);
			RedrawHeader = 1;
			break;
		}
		case itSong:
		{
			w->Bold(w->Choice(), myPlaylist->Add(*item.song, w->isBold(), 1));
			break;
		}
		case itPlaylist:
		{
			if (itsBrowsedDir == "/")
			{
				MPD::SongList list;
				Mpd.GetPlaylistContent(locale_to_utf_cpy(item.name), list);
				if (myPlaylist->Add(list, 1))
					ShowMessage("Loading and playing playlist %s...", item.name.c_str());
				FreeSongList(list);
			}
			else
			{
				std::string name = item.name;
				ShowMessage("Loading playlist %s...", name.c_str());
				locale_to_utf(name);
				if (!Mpd.LoadPlaylist(name))
					ShowMessage("Couldn't load playlist.");
			}
			break;
		}
	}
}

void Browser::SpacePressed()
{
	if (w->Empty())
		return;
	
	if (Config.space_selects && w->Choice() >= (itsBrowsedDir != "/" ? 1 : 0))
	{
		w->Select(w->Choice(), !w->isSelected());
		w->Scroll(wDown);
		return;
	}
	
	if (itsBrowsedDir != "/" && w->Choice() == 0 /* parent dir */)
		return;
	
	const MPD::Item &item = w->Current();
	switch (item.type)
	{
		case itDirectory:
		{
			if (itsBrowsedDir != "/" && !w->Choice())
				break; // do not let add parent dir.
			
			MPD::SongList list;
#			ifndef WIN32
			if (isLocal())
			{
				MPD::ItemList items;
				ShowMessage("Scanning \"%s\"...", item.name.c_str());
				myBrowser->GetLocalDirectory(items, item.name, 1);
				list.reserve(items.size());
				for (MPD::ItemList::const_iterator it = items.begin(); it != items.end(); ++it)
					list.push_back(it->song);
			}
			else
#			endif // !WIN32
				Mpd.GetDirectoryRecursive(locale_to_utf_cpy(item.name), list);
			
			if (myPlaylist->Add(list, 0))
				ShowMessage("Added folder: %s", item.name.c_str());
			
			FreeSongList(list);
			break;
		}
		case itSong:
		{
			w->Bold(w->Choice(), myPlaylist->Add(*item.song, w->isBold(), 0));
			break;
		}
		case itPlaylist:
		{
			std::string name = item.name;
			ShowMessage("Loading playlist %s...", name.c_str());
			locale_to_utf(name);
			if (!Mpd.LoadPlaylist(name))
				ShowMessage("Couldn't load playlist.");
			break;
		}
	}
	w->Scroll(wDown);
}

void Browser::MouseButtonPressed(MEVENT me)
{
	if (w->Empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->Size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		w->Goto(me.y);
		switch (w->Current().type)
		{
			case itDirectory:
				if (me.bstate & BUTTON1_PRESSED)
				{
					GetDirectory(w->Current().name);
					RedrawHeader = 1;
				}
				else
				{
					size_t pos = w->Choice();
					SpacePressed();
					if (pos < w->Size()-1)
						w->Scroll(wUp);
				}
				break;
			case itPlaylist:
			case itSong:
				if (me.bstate & BUTTON1_PRESSED)
				{
					size_t pos = w->Choice();
					SpacePressed();
					if (pos < w->Size()-1)
						w->Scroll(wUp);
				}
				else
					EnterPressed();
				break;
		}
	}
	else
		Screen< Menu<MPD::Item> >::MouseButtonPressed(me);
}

MPD::Song *Browser::CurrentSong()
{
	return !w->Empty() && w->Current().type == itSong ? w->Current().song : 0;
}

void Browser::ReverseSelection()
{
	w->ReverseSelection(itsBrowsedDir == "/" ? 0 : 1);
}

void Browser::GetSelectedSongs(MPD::SongList &v)
{
	if (w->Empty())
		return;
	std::vector<size_t> selected;
	w->GetSelected(selected);
	if (selected.empty())
		selected.push_back(w->Choice());
	for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
	{
		const MPD::Item &item = w->at(*it);
		switch (item.type)
		{
			case itDirectory:
			{
#				ifndef WIN32
				if (isLocal())
				{
					MPD::ItemList list;
					GetLocalDirectory(list, item.name, 1);
					for (MPD::ItemList::const_iterator j = list.begin(); j != list.end(); ++j)
						v.push_back(j->song);
				}
				else
#				endif // !WIN32
					Mpd.GetDirectoryRecursive(locale_to_utf_cpy(item.name), v);
				break;
			}
			case itSong:
			{
				v.push_back(new MPD::Song(*item.song));
				break;
			}
			case itPlaylist:
			{
				Mpd.GetPlaylistContent(locale_to_utf_cpy(item.name), v);
				break;
			}
		}
	}
}

void Browser::ApplyFilter(const std::string &s)
{
	w->ApplyFilter(s, itsBrowsedDir == "/" ? 0 : 1, REG_ICASE | Config.regex_type);
}

bool Browser::hasSupportedExtension(const std::string &file)
{
	size_t last_dot = file.rfind(".");
	if (last_dot > file.length())
		return false;
	
	std::string ext = file.substr(last_dot+1);
	ToLower(ext);
	return SupportedExtensions.find(ext) != SupportedExtensions.end();
}

void Browser::LocateSong(const MPD::Song &s)
{
	if (s.GetDirectory().empty())
		return;
	
	itsBrowseLocally = !s.isFromDB();
	
	if (myScreen != this)
		SwitchTo();
	
	if (itsBrowsedDir != s.GetDirectory())
		GetDirectory(s.GetDirectory());
	for (size_t i = 0; i < w->Size(); ++i)
	{
		if ((*w)[i].type == itSong && s.GetHash() == (*w)[i].song->GetHash())
		{
			w->Highlight(i);
			break;
		}
	}
}

void Browser::GetDirectory(std::string dir, std::string subdir)
{
	if (dir.empty())
		dir = "/";
	
	int highlightme = -1;
	itsScrollBeginning = 0;
	if (itsBrowsedDir != dir)
		w->Reset();
	itsBrowsedDir = dir;
	
	locale_to_utf(dir);
	
	for (size_t i = 0; i < w->Size(); ++i)
		if (w->at(i).type == itSong)
			delete w->at(i).song;
	
	w->Clear();
	
	if (dir != "/")
	{
		MPD::Item parent;
		size_t slash = dir.rfind("/");
		parent.song = reinterpret_cast<MPD::Song *>(1); // in that way we assume that's really parent dir
		parent.name = slash != std::string::npos ? dir.substr(0, slash) : "/";
		parent.type = itDirectory;
		utf_to_locale(parent.name);
		w->AddOption(parent);
	}
	
	MPD::ItemList list;
#	ifndef WIN32
	isLocal() ? GetLocalDirectory(list) : Mpd.GetDirectory(dir, list);
#	else
	Mpd.GetDirectory(dir, list);
#	endif // !WIN32
	if (!isLocal()) // local directory is already sorted
		sort(list.begin(), list.end(), CaseInsensitiveSorting());
	
	for (MPD::ItemList::iterator it = list.begin(); it != list.end(); ++it)
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
				for (size_t i = 0; i < myPlaylist->Items->Size(); ++i)
				{
					if (myPlaylist->Items->at(i).GetHash() == it->song->GetHash())
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
}

#ifndef WIN32
void Browser::GetLocalDirectory(MPD::ItemList &v, const std::string &directory, bool recursively) const
{
	DIR *dir = opendir((directory.empty() ? itsBrowsedDir : directory).c_str());
	
	if (!dir)
		return;
	
	dirent *file;
	
	struct stat file_stat;
	std::string full_path;
	
	size_t old_size = v.size();
	while ((file = readdir(dir)))
	{
		// omit . and ..
		if (file->d_name[0] == '.' && (file->d_name[1] == '\0' || (file->d_name[1] == '.' && file->d_name[2] == '\0')))
			continue;
		
		if (!Config.local_browser_show_hidden_files && file->d_name[0] == '.')
			continue;
		MPD::Item new_item;
		full_path = directory.empty() ? itsBrowsedDir : directory;
		if (itsBrowsedDir != "/")
			full_path += "/";
		full_path += file->d_name;
		stat(full_path.c_str(), &file_stat);
		if (S_ISDIR(file_stat.st_mode))
		{
			if (recursively)
			{
				GetLocalDirectory(v, full_path, 1);
				old_size = v.size();
			}
			else
			{
				new_item.type = itDirectory;
				new_item.name = full_path;
				v.push_back(new_item);
			}
		}
		else if (hasSupportedExtension(file->d_name))
		{
			new_item.type = itSong;
			mpd_pair file_pair = { "file", full_path.c_str() };
			new_item.song = new MPD::Song(mpd_song_begin(&file_pair));
#			ifdef HAVE_TAGLIB_H
			if (!recursively)
				TagEditor::ReadTags(*new_item.song);
#			endif // HAVE_TAGLIB_H
			v.push_back(new_item);
		}
	}
	closedir(dir);
	std::sort(v.begin()+old_size, v.end(), CaseInsensitiveSorting());
}

void Browser::ClearDirectory(const std::string &path) const
{
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return;
	
	dirent *file;
	struct stat file_stat;
	std::string full_path;
	
	while ((file = readdir(dir)))
	{
		// omit . and ..
		if (file->d_name[0] == '.' && (file->d_name[1] == '\0' || (file->d_name[1] == '.' && file->d_name[2] == '\0')))
			continue;
		
		full_path = path;
		if (*full_path.rbegin() != '/')
			full_path += '/';
		full_path += file->d_name;
		lstat(full_path.c_str(), &file_stat);
		if (S_ISDIR(file_stat.st_mode))
			ClearDirectory(full_path);
		if (remove(full_path.c_str()) == 0)
		{
			static const char msg[] = "Deleting \"%s\"...";
			ShowMessage(msg, Shorten(TO_WSTRING(full_path), COLS-static_strlen(msg)).c_str());
		}
		else
		{
			static const char msg[] = "Couldn't remove \"%s\": %s";
			ShowMessage(msg, Shorten(TO_WSTRING(full_path), COLS-static_strlen(msg)-25).c_str(), strerror(errno));
		}
	}
	closedir(dir);
}

void Browser::ChangeBrowseMode()
{
	if (Mpd.GetHostname()[0] != '/')
		return;
	
	itsBrowseLocally = !itsBrowseLocally;
	ShowMessage("Browse mode: %s", itsBrowseLocally ? "Local filesystem" : "MPD music dir");
	itsBrowsedDir = itsBrowseLocally ? Config.GetHomeDirectory() : "/";
	if (itsBrowseLocally && *itsBrowsedDir.rbegin() == '/')
		itsBrowsedDir.resize(itsBrowsedDir.length()-1);
	w->Reset();
	GetDirectory(itsBrowsedDir);
	RedrawHeader = 1;
}

bool Browser::DeleteItem(const MPD::Item &item)
{
	// parent dir
	if (item.type == itDirectory && item.song)
		return false;
	
	// playlist creatd by mpd
	if (!isLocal() && item.type == itPlaylist && CurrentDir() == "/")
		return Mpd.DeletePlaylist(locale_to_utf_cpy(item.name));
	
	std::string path;
	if (!isLocal())
		path = Config.mpd_music_dir;
	path += item.type == itSong ? item.song->GetFile() : item.name;
	
	if (item.type == itDirectory)
		ClearDirectory(path);
	
	return remove(path.c_str()) == 0;
}
#endif // !WIN32

void Browser::UpdateItemList()
{
	bool bold = 0;
	for (size_t i = 0; i < w->Size(); ++i)
	{
		if (w->at(i).type == itSong)
		{
			for (size_t j = 0; j < myPlaylist->Items->Size(); ++j)
			{
				if (myPlaylist->Items->at(j).GetHash() == w->at(i).song->GetHash())
				{
					bold = 1;
					break;
				}
			}
			w->Bold(i, bold);
			bold = 0;
		}
	}
	w->Refresh();
}

std::string Browser::ItemToString(const MPD::Item &item, void *)
{
	switch (item.type)
	{
		case MPD::itDirectory:
		{
			if (item.song)
				return "[..]";
			return "[" + ExtractTopName(item.name) + "]";
		}
		case MPD::itSong:
		{
			if (!Config.columns_in_browser)
				return item.song->toString(Config.song_list_format_dollar_free);
			else
				return Playlist::SongInColumnsToString(*item.song, 0);
		}
		case MPD::itPlaylist:
		{
			return Config.browser_playlist_prefix.Str() + ExtractTopName(item.name);
		}
		default:
		{
			return "";
		}
	}
}

