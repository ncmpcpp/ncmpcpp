/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale/conversion.hpp>
#include <algorithm>

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "error.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "regex_filter.h"
#include "screen_switcher.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "tag_editor.h"
#include "title.h"
#include "tags.h"
#include "utility/comparators.h"
#include "utility/string.h"
#include "configuration.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

using MPD::itDirectory;
using MPD::itSong;
using MPD::itPlaylist;

namespace fs = boost::filesystem;

Browser *myBrowser;

namespace {//

std::set<std::string> SupportedExtensions;
bool hasSupportedExtension(const std::string &file);

std::string ItemToString(const MPD::Item &item);
bool BrowserEntryMatcher(const boost::regex &rx, const MPD::Item &item, bool filter);

}

Browser::Browser() : itsBrowseLocally(0), itsScrollBeginning(0), itsBrowsedDir("/")
{
	w = NC::Menu<MPD::Item>(0, MainStartY, COLS, MainHeight, Config.browser_display_mode == DisplayMode::Columns && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::Border::None);
	w.setHighlightColor(Config.main_highlight_color);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setSelectedPrefix(Config.selected_item_prefix);
	w.setSelectedSuffix(Config.selected_item_suffix);
	w.setItemDisplayer(boost::bind(Display::Items, _1, proxySongList()));
}

void Browser::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	switch (Config.browser_display_mode)
	{
		case DisplayMode::Columns:
			if (Config.titles_visibility)
			{
				w.setTitle(Display::Columns(w.getWidth()));
				break;
			}
		case DisplayMode::Classic:
			w.setTitle("");
			break;
	}
	hasToBeResized = 0;
}

void Browser::switchTo()
{
	SwitchTo::execute(this);
	
	if (w.empty())
		GetDirectory(itsBrowsedDir);
	else
		markSongsInPlaylist(proxySongList());
	
	drawHeader();
}

std::wstring Browser::title()
{
	std::wstring result = L"Browse: ";
	result += Scroller(ToWString(itsBrowsedDir), itsScrollBeginning, COLS-result.length()-(Config.design == Design::Alternative ? 2 : Global::VolumeState.length()));
	return result;
}

void Browser::enterPressed()
{
	if (w.empty())
		return;
	
	const MPD::Item &item = w.current().value();
	switch (item.type)
	{
		case itDirectory:
		{
			if (isParentDirectory(item))
				GetDirectory(getParentDirectory(itsBrowsedDir), itsBrowsedDir);
			else
				GetDirectory(item.name, itsBrowsedDir);
			drawHeader();
			break;
		}
		case itSong:
		{
			addSongToPlaylist(*item.song, true, -1);
			break;
		}
		case itPlaylist:
		{
			MPD::SongList list;
			Mpd.GetPlaylistContentNoInfo(item.name, vectorMoveInserter(list));
			bool success = addSongsToPlaylist(list.begin(), list.end(), true, -1);
			Statusbar::printf("Playlist \"%1%\" loaded%2%",
				item.name, withErrors(success)
			);
		}
	}
}

void Browser::spacePressed()
{
	if (w.empty())
		return;
	
	size_t i = itsBrowsedDir != "/" ? 1 : 0;
	if (Config.space_selects && w.choice() >= i)
	{
		i = w.choice();
		w.at(i).setSelected(!w.at(i).isSelected());
		w.scroll(NC::Scroll::Down);
		return;
	}
	
	const MPD::Item &item = w.current().value();
	
	if (isParentDirectory(item))
		return;
	
	switch (item.type)
	{
		case itDirectory:
		{
			bool success;
#			ifndef WIN32
			if (isLocal())
			{
				MPD::SongList list;
				MPD::ItemList items;
				Statusbar::printf("Scanning directory \"%1%\"...", item.name);
				myBrowser->GetLocalDirectory(items, item.name, 1);
				list.reserve(items.size());
				for (MPD::ItemList::const_iterator it = items.begin(); it != items.end(); ++it)
					list.push_back(*it->song);
				success = addSongsToPlaylist(list.begin(), list.end(), false, -1);
			}
			else
#			endif // !WIN32
			{
				Mpd.Add(item.name);
				success = true;
			}
			Statusbar::printf("Directory \"%1%\" added%2%",
				item.name, withErrors(success)
			);
			break;
		}
		case itSong:
		{
			addSongToPlaylist(*item.song, false);
			break;
		}
		case itPlaylist:
		{
			Mpd.LoadPlaylist(item.name);
			Statusbar::printf("Playlist \"%1%\" loaded", item.name);
			break;
		}
	}
	w.scroll(NC::Scroll::Down);
}

void Browser::mouseButtonPressed(MEVENT me)
{
	if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		w.Goto(me.y);
		switch (w.current().value().type)
		{
			case itDirectory:
				if (me.bstate & BUTTON1_PRESSED)
				{
					GetDirectory(w.current().value().name);
					drawHeader();
				}
				else
				{
					size_t pos = w.choice();
					spacePressed();
					if (pos < w.size()-1)
						w.scroll(NC::Scroll::Up);
				}
				break;
			case itPlaylist:
			case itSong:
				if (me.bstate & BUTTON1_PRESSED)
				{
					size_t pos = w.choice();
					spacePressed();
					if (pos < w.size()-1)
						w.scroll(NC::Scroll::Up);
				}
				else
					enterPressed();
				break;
		}
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

/***********************************************************************/

bool Browser::allowsFiltering()
{
	return true;
}

std::string Browser::currentFilter()
{
	return RegexFilter<MPD::Item>::currentFilter(w);
}

void Browser::applyFilter(const std::string &filter)
{
	if (filter.empty())
	{
		w.clearFilter();
		w.clearFilterResults();
		return;
	}
	try
	{
		w.showAll();
		auto fun = boost::bind(BrowserEntryMatcher, _1, _2, true);
		auto rx = RegexFilter<MPD::Item>(
			boost::regex(filter, Config.regex_type), fun);
		w.filter(w.begin(), w.end(), rx);
	}
	catch (boost::bad_expression &) { }
}

/***********************************************************************/

bool Browser::allowsSearching()
{
	return true;
}

bool Browser::search(const std::string &constraint)
{
	if (constraint.empty())
	{
		w.clearSearchResults();
		return false;
	}
	try
	{
		auto fun = boost::bind(BrowserEntryMatcher, _1, _2, false);
		auto rx = RegexFilter<MPD::Item>(
			boost::regex(constraint, Config.regex_type), fun);
		return w.search(w.begin(), w.end(), rx);
	}
	catch (boost::bad_expression &)
	{
		return false;
	}
}

void Browser::nextFound(bool wrap)
{
	w.nextFound(wrap);
}

void Browser::prevFound(bool wrap)
{
	w.prevFound(wrap);
}

/***********************************************************************/

ProxySongList Browser::proxySongList()
{
	return ProxySongList(w, [](NC::Menu<MPD::Item>::Item &item) -> MPD::Song * {
		MPD::Song *ptr = 0;
		if (item.value().type == itSong)
			ptr = item.value().song.get();
		return ptr;
	});
}

bool Browser::allowsSelection()
{
	return true;
}

void Browser::reverseSelection()
{
	reverseSelectionHelper(w.begin()+(itsBrowsedDir == "/" ? 0 : 1), w.end());
}

MPD::SongList Browser::getSelectedSongs()
{
	MPD::SongList result;
	auto item_handler = [this, &result](const MPD::Item &item) {
		if (item.type == itDirectory)
		{
#			ifndef WIN32
			if (isLocal())
			{
				MPD::ItemList list;
				GetLocalDirectory(list, item.name, true);
				for (auto it = list.begin(); it != list.end(); ++it)
					result.push_back(*it->song);
			}
			else
#			endif // !WIN32
			{
				Mpd.GetDirectoryRecursive(item.name, vectorMoveInserter(result));
			}
		}
		else if (item.type == itSong)
			result.push_back(*item.song);
		else if (item.type == itPlaylist)
		{
			Mpd.GetPlaylistContent(item.name, vectorMoveInserter(result));
		}
	};
	for (auto it = w.begin(); it != w.end(); ++it)
		if (it->isSelected())
			item_handler(it->value());
	// if no item is selected, add current one
	if (result.empty() && !w.empty())
		item_handler(w.current().value());
	return result;
}

void Browser::fetchSupportedExtensions()
{
	SupportedExtensions.clear();
	Mpd.GetSupportedExtensions(SupportedExtensions);
}

void Browser::LocateSong(const MPD::Song &s)
{
	if (s.getDirectory().empty())
		return;
	
	itsBrowseLocally = !s.isFromDatabase();
	
	if (myScreen != this)
		switchTo();
	
	if (itsBrowsedDir != s.getDirectory())
		GetDirectory(s.getDirectory());
	for (size_t i = 0; i < w.size(); ++i)
	{
		if (w[i].value().type == itSong && s == *w[i].value().song)
		{
			w.highlight(i);
			break;
		}
	}
	drawHeader();
}

void Browser::GetDirectory(std::string dir, std::string subdir)
{
	if (dir.empty())
		dir = "/";
	
	int highlightme = -1;
	itsScrollBeginning = 0;
	if (itsBrowsedDir != dir)
		w.reset();
	itsBrowsedDir = dir;
	
	w.clear();
	
	if (dir != "/")
	{
		MPD::Item parent;
		parent.name = "..";
		parent.type = itDirectory;
		w.addItem(parent);
	}
	
	MPD::ItemList list;
#	ifndef WIN32
	if (isLocal())
		GetLocalDirectory(list, itsBrowsedDir, false);
	else
		Mpd.GetDirectory(dir, vectorMoveInserter(list));
#	else
	list = Mpd.GetDirectory(dir);
#	endif // !WIN32
	if (Config.browser_sort_mode != SortMode::NoOp && !isLocal()) // local directory is already sorted
		std::sort(list.begin(), list.end(),
			LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the, Config.browser_sort_mode)
		);
	
	for (MPD::ItemList::iterator it = list.begin(); it != list.end(); ++it)
	{
		switch (it->type)
		{
			case itPlaylist:
			{
				w.addItem(*it);
				break;
			}
			case itDirectory:
			{
				if (it->name == subdir)
					highlightme = w.size();
				w.addItem(*it);
				break;
			}
			case itSong:
			{
				w.addItem(*it, myPlaylist->checkForSong(*it->song));
				break;
			}
		}
	}
	if (highlightme >= 0)
		w.highlight(highlightme);
}

#ifndef WIN32
void Browser::GetLocalDirectory(MPD::ItemList &v, const std::string &directory, bool recursively) const
{
	size_t start_size = v.size();
	fs::path dir(directory);
	std::for_each(fs::directory_iterator(dir), fs::directory_iterator(), [&](fs::directory_entry &e) {
		if (!Config.local_browser_show_hidden_files && e.path().filename().native()[0] == '.')
			return;
		MPD::Item item;
		if (fs::is_directory(e))
		{
			if (recursively)
			{
				GetLocalDirectory(v, e.path().native(), true);
				start_size = v.size();
			}
			else
			{
				item.type = itDirectory;
				item.name = e.path().native();
				v.push_back(item);
			}
		}
		else if (hasSupportedExtension(e.path().native()))
		{
			item.type = itSong;
			mpd_pair file_pair = { "file", e.path().native().c_str() };
			MPD::MutableSong *s = new MPD::MutableSong(mpd_song_begin(&file_pair));
			item.song = std::shared_ptr<MPD::Song>(s);
#			ifdef HAVE_TAGLIB_H
			if (!recursively)
			{
				s->setMTime(fs::last_write_time(e.path()));
				Tags::read(*s);
			}
#			endif // HAVE_TAGLIB_H
			v.push_back(item);
		}
	});
	
	if (Config.browser_sort_mode != SortMode::NoOp)
		std::sort(v.begin()+start_size, v.end(),
			LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the, Config.browser_sort_mode)
		);
}

void Browser::ClearDirectory(const std::string &path) const
{
	fs::path dir(path);
	std::for_each(fs::directory_iterator(dir), fs::directory_iterator(), [&](fs::directory_entry &e) {
		if (!fs::is_symlink(e) && fs::is_directory(e))
			ClearDirectory(e.path().native());
		const char msg[] = "Deleting \"%1%\"...";
		Statusbar::printf(msg, wideShorten(e.path().native(), COLS-const_strlen(msg)));
		fs::remove(e.path());
	});
}

void Browser::ChangeBrowseMode()
{
	if (Mpd.GetHostname()[0] != '/')
	{
		Statusbar::print("For browsing local filesystem connection to MPD via UNIX Socket is required");
		return;
	}
	
	itsBrowseLocally = !itsBrowseLocally;
	Statusbar::printf("Browse mode: %1%",
		itsBrowseLocally ? "local filesystem" : "MPD database"
	);
	if (itsBrowseLocally)
	{
		itsBrowsedDir = "~";
		expand_home(itsBrowsedDir);
		if (*itsBrowsedDir.rbegin() == '/')
			itsBrowsedDir.resize(itsBrowsedDir.length()-1);
	}
	else
		itsBrowsedDir = "/";
	w.reset();
	GetDirectory(itsBrowsedDir);
	drawHeader();
}

bool Browser::deleteItem(const MPD::Item &item, std::string &errmsg)
{
	if (!Config.allow_for_physical_item_deletion)
		FatalError("Browser::deleteItem invoked with allow_for_physical_item_deletion = false");
	if (isParentDirectory((item)))
		FatalError("Parent directory passed to Browser::deleteItem");
	
	// playlist created by mpd
	if (!isLocal() && item.type == itPlaylist && CurrentDir() == "/")
	{
		try
		{
			Mpd.DeletePlaylist(item.name);
			return true;
		}
		catch (MPD::ServerError &e)
		{
			// if there is no such mpd playlist, we assume it's users's playlist.
			if (e.code() != MPD_SERVER_ERROR_NO_EXIST)
				throw;
		}
	}
	
	std::string path;
	if (!isLocal())
		path = Config.mpd_music_dir;
	path += item.type == itSong ? item.song->getURI() : item.name;
	
	bool rv;
	try
	{
		if (item.type == itDirectory)
			ClearDirectory(path);
		if (!boost::filesystem::exists(path))
		{
			errmsg = "No such item: " + path;
			rv = false;
		}
		else
		{
			boost::filesystem::remove(path);
			rv = true;
		}
	}
	catch (boost::filesystem::filesystem_error &err)
	{
		errmsg = err.what();
		rv = false;
	}
	return rv;
}
#endif // !WIN32

namespace {//

bool hasSupportedExtension(const std::string &file)
{
	size_t last_dot = file.rfind(".");
	if (last_dot > file.length())
		return false;
	
	std::string ext = boost::locale::to_lower(file.substr(last_dot+1));
	return SupportedExtensions.find(ext) != SupportedExtensions.end();
}

std::string ItemToString(const MPD::Item &item)
{
	std::string result;
	switch (item.type)
	{
		case MPD::itDirectory:
			result = "[" + getBasename(item.name) + "]";
			break;
		case MPD::itSong:
			switch (Config.browser_display_mode)
			{
				case DisplayMode::Classic:
					result = item.song->toString(Config.song_list_format_dollar_free, Config.tags_separator);
					break;
				case DisplayMode::Columns:
					result = item.song->toString(Config.song_in_columns_to_string_format, Config.tags_separator);
					break;
			}
			break;
		case MPD::itPlaylist:
			result = Config.browser_playlist_prefix.str() + getBasename(item.name);
			break;
	}
	return result;
}

bool BrowserEntryMatcher(const boost::regex &rx, const MPD::Item &item, bool filter)
{
	if (Browser::isParentDirectory(item))
		return filter;
	return boost::regex_search(ItemToString(item), rx);
}

}
