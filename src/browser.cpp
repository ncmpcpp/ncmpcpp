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
#include "utility/comparators.h"
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
	w = new Menu<MPD::Item>(0, MainStartY, COLS, MainHeight, Config.columns_in_browser && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, brNone);
	w->HighlightColor(Config.main_highlight_color);
	w->CyclicScrolling(Config.use_cyclic_scrolling);
	w->CenteredCursor(Config.centered_cursor);
	w->SetSelectPrefix(Config.selected_item_prefix);
	w->SetSelectSuffix(Config.selected_item_suffix);
	w->setItemDisplayer(Display::Items);
	w->SetItemStringifier(ItemToString);
	
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
	
	if (isLocal() && Config.browser_sort_mode == smMTime) // local browser doesn't support sorting by mtime
		Config.browser_sort_mode = smName;
	
	w->Empty() ? myBrowser->GetDirectory(itsBrowsedDir) : myBrowser->UpdateItemList();

	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	RedrawHeader = true;
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
	
	const MPD::Item &item = w->Current().value();
	switch (item.type)
	{
		case itDirectory:
		{
			if (isParentDir(w->Choice()))
			{
				size_t slash = itsBrowsedDir.rfind("/");
				if (slash != std::string::npos)
					GetDirectory(itsBrowsedDir.substr(0, slash), itsBrowsedDir);
				else
					GetDirectory("/", itsBrowsedDir);
			}
			else
				GetDirectory(item.name, itsBrowsedDir);
			RedrawHeader = true;
			break;
		}
		case itSong:
		{
			size_t i = w->Choice();
			bool res = myPlaylist->Add(*item.song, w->at(i).isBold(), 1);
			w->at(i).setBold(res);
			break;
		}
		case itPlaylist:
		{
			if (Mpd.LoadPlaylist(locale_to_utf_cpy(item.name)))
			{
				ShowMessage("Playlist \"%s\" loaded", item.name.c_str());
				myPlaylist->PlayNewlyAddedSongs();
			}
		}
	}
}

void Browser::SpacePressed()
{
	if (w->Empty())
		return;
	
	size_t i = itsBrowsedDir != "/" ? 1 : 0;
	if (Config.space_selects && w->Choice() >= i)
	{
		i = w->Choice();
		w->at(i).setSelected(!w->at(i).isSelected());
		w->Scroll(wDown);
		return;
	}
	
	if (isParentDir(w->Choice()))
		return;
	
	const MPD::Item &item = w->Current().value();
	switch (item.type)
	{
		case itDirectory:
		{
			bool result;
#			ifndef WIN32
			if (isLocal())
			{
				MPD::SongList list;
				MPD::ItemList items;
				ShowMessage("Scanning directory \"%s\"...", item.name.c_str());
				myBrowser->GetLocalDirectory(items, item.name, 1);
				list.reserve(items.size());
				for (MPD::ItemList::const_iterator it = items.begin(); it != items.end(); ++it)
					list.push_back(*it->song);
				result = myPlaylist->Add(list, 0);
			}
			else
#			endif // !WIN32
				result = Mpd.Add(locale_to_utf_cpy(item.name));
			if (result)
				ShowMessage("Directory \"%s\" added", item.name.c_str());
			break;
		}
		case itSong:
		{
			i = w->Choice();
			bool res = myPlaylist->Add(*item.song, w->at(i).isBold(), 0);
			w->at(i).setBold(res);
			break;
		}
		case itPlaylist:
		{
			if (Mpd.LoadPlaylist(locale_to_utf_cpy(item.name)))
				ShowMessage("Playlist \"%s\" loaded", item.name.c_str());
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
		switch (w->Current().value().type)
		{
			case itDirectory:
				if (me.bstate & BUTTON1_PRESSED)
				{
					GetDirectory(w->Current().value().name);
					RedrawHeader = true;
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
	const MPD::Item &item = w->Current().value();
	if (!w->Empty() && item.type == itSong)
		return item.song.get();
	else
		return 0;
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
	for (auto it = selected.begin(); it != selected.end(); ++it)
	{
		const MPD::Item &item = w->at(*it).value();
		switch (item.type)
		{
			case itDirectory:
			{
#				ifndef WIN32
				if (isLocal())
				{
					MPD::ItemList list;
					GetLocalDirectory(list, item.name, 1);
					for (auto j = list.begin(); j != list.end(); ++j)
						v.push_back(*j->song);
				}
				else
#				endif // !WIN32
				{
					Mpd.GetDirectoryRecursive(locale_to_utf_cpy(item.name), [&v](MPD::Song &&s) {
						v.push_back(s);
					});
				}
				break;
			}
			case itSong:
			{
				v.push_back(*item.song);
				break;
			}
			case itPlaylist:
			{
				Mpd.GetPlaylistContent(locale_to_utf_cpy(item.name), [&v](MPD::Song &&s) {
					v.push_back(s);
				});
				break;
			}
		}
	}
}

std::string Browser::currentFilter()
{
	return RegexFilter<MPD::Item>::currentFilter(*w);
}

void Browser::applyFilter(const std::string &filter)
{
	w->ShowAll();
	auto fun = [](const Regex &rx, Menu<MPD::Item> &menu, const Menu<MPD::Item>::Item &item) {
		if (item.value().type == MPD::itDirectory && item.value().name == "..")
			return true;
		return rx.match(menu.Stringify(item));
	};
	auto rx = RegexFilter<MPD::Item>(filter, Config.regex_type, fun);
	w->Filter(w->Begin(), w->End(), rx);
}

bool Browser::hasSupportedExtension(const std::string &file)
{
	size_t last_dot = file.rfind(".");
	if (last_dot > file.length())
		return false;
	
	std::string ext = file.substr(last_dot+1);
	lowercase(ext);
	return SupportedExtensions.find(ext) != SupportedExtensions.end();
}

void Browser::LocateSong(const MPD::Song &s)
{
	if (s.getDirectory().empty())
		return;
	
	itsBrowseLocally = !s.isFromDatabase();
	
	if (myScreen != this)
		SwitchTo();
	
	if (itsBrowsedDir != s.getDirectory())
		GetDirectory(s.getDirectory());
	for (size_t i = 0; i < w->Size(); ++i)
	{
		if ((*w)[i].value().type == itSong && s.getHash() == (*w)[i].value().song->getHash())
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
	
	w->Clear();
	
	if (dir != "/")
	{
		MPD::Item parent;
		parent.name = "..";
		parent.type = itDirectory;
		w->AddItem(parent);
	}
	
	MPD::ItemList list;
#	ifndef WIN32
	if (isLocal())
		GetLocalDirectory(list);
	else
	{
		Mpd.GetDirectory(dir, [&list](MPD::Item &&i) {
			list.push_back(i);
		});
	}
#	else
	Mpd.GetDirectory(dir, [&list](MPD::Item &&i) {
		list.push_back(i);
	});
#	endif // !WIN32
	if (!isLocal()) // local directory is already sorted
		std::sort(list.begin(), list.end(), CaseInsensitiveSorting());
	
	for (MPD::ItemList::iterator it = list.begin(); it != list.end(); ++it)
	{
		switch (it->type)
		{
			case itPlaylist:
			{
				utf_to_locale(it->name);
				w->AddItem(*it);
				break;
			}
			case itDirectory:
			{
				utf_to_locale(it->name);
				if (it->name == subdir)
					highlightme = w->Size();
				w->AddItem(*it);
				break;
			}
			case itSong:
			{
				bool bold = 0;
				for (size_t i = 0; i < myPlaylist->Items->Size(); ++i)
				{
					if (myPlaylist->Items->at(i).value().getHash() == it->song->getHash())
					{
						bold = 1;
						break;
					}
				}
				w->AddItem(*it, bold);
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
			MPD::MutableSong *s = new MPD::MutableSong(mpd_song_begin(&file_pair));
			new_item.song = std::shared_ptr<MPD::Song>(s);
#			ifdef HAVE_TAGLIB_H
			if (!recursively)
				TagEditor::ReadTags(*s);
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
			const char msg[] = "Deleting \"%s\"...";
			ShowMessage(msg, Shorten(TO_WSTRING(full_path), COLS-const_strlen(msg)).c_str());
		}
		else
		{
			const char msg[] = "Couldn't remove \"%s\": %s";
			ShowMessage(msg, Shorten(TO_WSTRING(full_path), COLS-const_strlen(msg)-25).c_str(), strerror(errno));
		}
	}
	closedir(dir);
}

void Browser::ChangeBrowseMode()
{
	if (Mpd.GetHostname()[0] != '/')
	{
		ShowMessage("For browsing local filesystem connection to MPD via UNIX Socket is required");
		return;
	}
	
	itsBrowseLocally = !itsBrowseLocally;
	ShowMessage("Browse mode: %s", itsBrowseLocally ? "Local filesystem" : "MPD database");
	itsBrowsedDir = itsBrowseLocally ? Config.GetHomeDirectory() : "/";
	if (itsBrowseLocally && *itsBrowsedDir.rbegin() == '/')
		itsBrowsedDir.resize(itsBrowsedDir.length()-1);
	w->Reset();
	GetDirectory(itsBrowsedDir);
	RedrawHeader = true;
}

bool Browser::DeleteItem(const MPD::Item &item)
{
	// parent dir
	if (item.type == itDirectory && item.name == "..")
		return false;
	
	// playlist created by mpd
	if (!isLocal() && item.type == itPlaylist && CurrentDir() == "/")
		return Mpd.DeletePlaylist(locale_to_utf_cpy(item.name));
	
	std::string path;
	if (!isLocal())
		path = Config.mpd_music_dir;
	path += item.type == itSong ? item.song->getURI() : item.name;
	
	if (item.type == itDirectory)
		ClearDirectory(path);
	
	return remove(path.c_str()) == 0;
}
#endif // !WIN32

void Browser::UpdateItemList()
{
	for (size_t i = 0; i < w->Size(); ++i)
		if ((*w)[i].value().type == itSong)
			w->at(i).setBold(myPlaylist->checkForSong(*(*w)[i].value().song));
	w->Refresh();
}

std::string Browser::ItemToString(const MPD::Item &item)
{
	std::string result;
	switch (item.type)
	{
		case MPD::itDirectory:
			result = "[" + getBasename(item.name) + "]";
			break;
		case MPD::itSong:
			if (!Config.columns_in_browser)
				result = item.song->toString(Config.song_list_format_dollar_free);
			else
				result = Playlist::SongInColumnsToString(*item.song);
		case MPD::itPlaylist:
			result = Config.browser_playlist_prefix.Str() + getBasename(item.name);
	}
	return result;
}

