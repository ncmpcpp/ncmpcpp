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

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale/conversion.hpp>
#include <time.h>

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "error.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
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

namespace fs = boost::filesystem;

Browser *myBrowser;

namespace {

std::set<std::string> lm_supported_extensions;

std::string realPath(bool local_browser, std::string path);
bool isStringParentDirectory(const std::string &directory);
bool isItemParentDirectory(const MPD::Item &item);
bool isRootDirectory(const std::string &directory);
bool isHidden(const fs::directory_iterator &entry);
bool hasSupportedExtension(const fs::directory_entry &entry);
MPD::Song getLocalSong(const fs::directory_entry &entry, bool read_tags);
void getLocalDirectory(std::vector<MPD::Item> &items, const std::string &directory);
void getLocalDirectoryRecursively(std::vector<MPD::Song> &songs, const std::string &directory);
void clearDirectory(const std::string &directory);

std::string itemToString(const MPD::Item &item);
bool browserEntryMatcher(const boost::regex &rx, const MPD::Item &item, bool filter);

}

Browser::Browser() : m_local_browser(false), m_scroll_beginning(0), m_current_directory("/")
{
	w = NC::Menu<MPD::Item>(0, MainStartY, COLS, MainHeight, Config.browser_display_mode == DisplayMode::Columns && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::Border());
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
	markSongsInPlaylist(proxySongList());
	drawHeader();
}

std::wstring Browser::title()
{
	std::wstring result = L"Browse: ";
	result += Scroller(ToWString(m_current_directory), m_scroll_beginning, COLS-result.length()-(Config.design == Design::Alternative ? 2 : Global::VolumeState.length()));
	return result;
}

void Browser::enterPressed()
{
	if (w.empty())
		return;
	
	const MPD::Item &item = w.current()->value();
	switch (item.type())
	{
		case MPD::Item::Type::Directory:
		{
			getDirectory(item.directory().path());
			drawHeader();
			break;
		}
		case MPD::Item::Type::Song:
		{
			addSongToPlaylist(item.song(), true, -1);
			break;
		}
		case MPD::Item::Type::Playlist:
		{
			std::vector<MPD::Song> list(
				std::make_move_iterator(Mpd.GetPlaylistContentNoInfo(item.playlist().path())),
				std::make_move_iterator(MPD::SongIterator())
			);
			// TODO: ask on failure if we want to continue
			bool success = addSongsToPlaylist(list.begin(), list.end(), true, -1);
			Statusbar::printf("Playlist \"%1%\" loaded%2%",
				item.playlist().path(), withErrors(success)
			);
		}
	}
}

void Browser::spacePressed()
{
	if (w.empty())
		return;
	
	size_t i = inRootDirectory() ? 0 : 1;
	if (Config.space_selects && w.choice() >= i)
	{
		i = w.choice();
		w[i].setSelected(!w[i].isSelected());
		w.scroll(NC::Scroll::Down);
		return;
	}

	const MPD::Item &item = w.current()->value();
	// ignore parent directory
	if (isParentDirectory(item))
		return;

	switch (item.type())
	{
		case MPD::Item::Type::Directory:
		{
			bool success = true;
			if (m_local_browser)
			{
				std::vector<MPD::Song> songs;
				getLocalDirectoryRecursively(songs, item.directory().path());
				success = addSongsToPlaylist(songs.begin(), songs.end(), false, -1);
			}
			else
				Mpd.Add(item.directory().path());
			Statusbar::printf("Directory \"%1%\" added%2%",
				item.directory().path(), withErrors(success)
			);
			break;
		}
		case MPD::Item::Type::Song:
			addSongToPlaylist(item.song(), false);
			break;
		case MPD::Item::Type::Playlist:
			Mpd.LoadPlaylist(item.playlist().path());
			Statusbar::printf("Playlist \"%1%\" loaded", item.playlist().path());
			break;
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
		switch (w.current()->value().type())
		{
			case MPD::Item::Type::Directory:
				if (me.bstate & BUTTON1_PRESSED)
				{
					getDirectory(w.current()->value().directory().path());
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
			case MPD::Item::Type::Playlist:
			case MPD::Item::Type::Song:
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

bool Browser::allowsSearching()
{
	return true;
}

void Browser::setSearchConstraint(const std::string &constraint)
{
	m_search_predicate = RegexFilter<MPD::Item>(
		boost::regex(constraint, Config.regex_type),
		boost::bind(browserEntryMatcher, _1, _2, false)
	);
}

void Browser::clearConstraint()
{
	m_search_predicate.clear();
}

bool Browser::find(SearchDirection direction, bool wrap, bool skip_current)
{
	return search(w, m_search_predicate, direction, wrap, skip_current);
}

/***********************************************************************/

ProxySongList Browser::proxySongList()
{
	return ProxySongList(w, [](NC::Menu<MPD::Item>::Item &item) -> MPD::Song * {
		MPD::Song *ptr = 0;
		if (item.value().type() == MPD::Item::Type::Song)
			ptr = const_cast<MPD::Song *>(&item.value().song());
		return ptr;
	});
}

bool Browser::allowsSelection()
{
	return true;
}

void Browser::reverseSelection()
{
	size_t offset = inRootDirectory() ? 0 : 1;
	reverseSelectionHelper(w.begin()+offset, w.end());
}

std::vector<MPD::Song> Browser::getSelectedSongs()
{
	std::vector<MPD::Song> songs;
	auto item_handler = [this, &songs](const MPD::Item &item) {
		switch (item.type())
		{
			case MPD::Item::Type::Directory:
				if (m_local_browser)
					getLocalDirectoryRecursively(songs, item.directory().path());
				else
				{
					std::copy(
						std::make_move_iterator(Mpd.GetDirectoryRecursive(item.directory().path())),
						std::make_move_iterator(MPD::SongIterator()),
						std::back_inserter(songs)
					);
				}
				break;
			case MPD::Item::Type::Song:
				songs.push_back(item.song());
				break;
			case MPD::Item::Type::Playlist:
				std::copy(
					std::make_move_iterator(Mpd.GetPlaylistContent(item.playlist().path())),
					std::make_move_iterator(MPD::SongIterator()),
					std::back_inserter(songs)
				);
				break;
		}
	};
	for (const auto &item : w)
		if (item.isSelected())
			item_handler(item.value());
	// if no item is selected, add current one
	if (songs.empty() && !w.empty())
		item_handler(w.current()->value());
	return songs;
}

/***********************************************************************/

bool Browser::inRootDirectory()
{
	return isRootDirectory(m_current_directory);
}

bool Browser::isParentDirectory(const MPD::Item &item)
{
	return isItemParentDirectory(item);
}

const std::string& Browser::currentDirectory()
{
	return m_current_directory;
}

void Browser::locateSong(const MPD::Song &s)
{
	if (s.getDirectory().empty())
		throw std::runtime_error("Song's directory is empty");
	
	m_local_browser = !s.isFromDatabase();
	
	if (myScreen != this)
		switchTo();
	
	// change to relevant directory
	if (m_current_directory != s.getDirectory())
	{
		getDirectory(s.getDirectory());
		drawHeader();
	}

	// highlight the item
	auto begin = w.beginV(), end = w.endV();
	auto it = std::find(begin, end, MPD::Item(s));
	if (it != end)
		w.highlight(it-begin);
}

void Browser::getDirectory(std::string directory)
{
	m_scroll_beginning = 0;
	w.clear();

	// reset the position if we change directories
	if (m_current_directory != directory)
		w.reset();

	// check if it's a parent directory
	if (isStringParentDirectory(directory))
	{
		directory.resize(directory.length()-3);
		directory = getParentDirectory(directory);
	}
	// when we go down to root, it can be empty
	if (directory.empty())
		directory = "/";

	std::vector<MPD::Item> items;
	if (m_local_browser)
		getLocalDirectory(items, directory);
	else
	{
		std::copy(
			std::make_move_iterator(Mpd.GetDirectory(directory)),
			std::make_move_iterator(MPD::ItemIterator()),
			std::back_inserter(items)
		);
	}

	// sort items
	if (Config.browser_sort_mode != SortMode::NoOp)
	{
		std::sort(items.begin(), items.end(),
			LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the, Config.browser_sort_mode)
		);
	}

	// if the requested directory is not root, add parent directory
	if (!isRootDirectory(directory))
	{
		// make it so that display function doesn't have to handle special cases
		w.addItem(MPD::Directory(directory + "/.."));
	}

	for (const auto &item : items)
	{
		switch (item.type())
		{
			case MPD::Item::Type::Playlist:
			{
				w.addItem(std::move(item));
				break;
			}
			case MPD::Item::Type::Directory:
			{
				bool is_current = item.directory().path() == m_current_directory;
				w.addItem(std::move(item));
				if (is_current)
					w.highlight(w.size()-1);
				break;
			}
			case MPD::Item::Type::Song:
			{
				bool is_bold = myPlaylist->checkForSong(item.song());
				w.addItem(std::move(item), is_bold);
				break;
			}
		}
	}
	m_current_directory = directory;
}

void Browser::changeBrowseMode()
{
	if (Mpd.GetHostname()[0] != '/')
	{
		Statusbar::print("For browsing local filesystem connection to MPD via UNIX Socket is required");
		return;
	}
	
	m_local_browser = !m_local_browser;
	Statusbar::printf("Browse mode: %1%",
		m_local_browser ? "local filesystem" : "MPD database"
	);
	if (m_local_browser)
	{
		m_current_directory = "~";
		expand_home(m_current_directory);
	}
	else
		m_current_directory = "/";
	w.reset();
	getDirectory(m_current_directory);
	drawHeader();
}

void Browser::remove(const MPD::Item &item)
{
	if (!Config.allow_for_physical_item_deletion)
		throw std::runtime_error("physical deletion is forbidden");
	if (isParentDirectory((item)))
		throw std::runtime_error("deletion of parent directory is forbidden");

	std::string path;
	switch (item.type())
	{
		case MPD::Item::Type::Directory:
			path = realPath(m_local_browser, item.directory().path());
			clearDirectory(path);
			fs::remove(path);
			break;
		case MPD::Item::Type::Song:
			path = realPath(m_local_browser, item.song().getURI());
			fs::remove(path);
			break;
		case MPD::Item::Type::Playlist:
			path = item.playlist().path();
			try {
				Mpd.DeletePlaylist(path);
			} catch (MPD::ServerError &e) {
				// if there is no such mpd playlist, it's a local one
				if (e.code() == MPD_SERVER_ERROR_NO_EXIST)
				{
					path = realPath(m_local_browser, std::move(path));
					fs::remove(path);
				}
				else
					throw;
			}
			break;
	}
}

/***********************************************************************/

void Browser::fetchSupportedExtensions()
{
	lm_supported_extensions.clear();
	MPD::StringIterator extension = Mpd.GetSupportedExtensions(), end;
	for (; extension != end; ++extension)
		lm_supported_extensions.insert("." + std::move(*extension));
}

/***********************************************************************/

namespace {

std::string realPath(bool local_browser, std::string path)
{
	if (!local_browser)
		path = Config.mpd_music_dir + path;
	return path;
}

bool isStringParentDirectory(const std::string &directory)
{
	return boost::algorithm::ends_with(directory, "/..");
}

bool isItemParentDirectory(const MPD::Item &item)
{
	return item.type() == MPD::Item::Type::Directory
	    && isStringParentDirectory(item.directory().path());
}

bool isRootDirectory(const std::string &directory)
{
	return directory == "/";
}

bool isHidden(const fs::directory_iterator &entry)
{
	return entry->path().filename().native()[0] == '.';
}

bool hasSupportedExtension(const fs::directory_entry &entry)
{
	return lm_supported_extensions.find(entry.path().extension().native())
	    != lm_supported_extensions.end();
}

MPD::Song getLocalSong(const fs::directory_entry &entry, bool read_tags)
{
	mpd_pair pair = { "file", entry.path().c_str() };
	mpd_song *s = mpd_song_begin(&pair);
	if (s == nullptr)
		throw std::runtime_error("invalid path: " + entry.path().native());
#	ifdef HAVE_TAGLIB_H
	if (read_tags)
	{
		Tags::setAttribute(s, "Last-Modified",
			timeFormat("%Y-%m-%dT%H:%M:%SZ", fs::last_write_time(entry.path()))
		);
		// read tags
		Tags::read(s);
	}
#	endif // HAVE_TAGLIB_H
	return s;
}

void getLocalDirectory(std::vector<MPD::Item> &items, const std::string &directory)
{
	for (fs::directory_iterator entry(directory), end; entry != end; ++entry)
	{
		if (!Config.local_browser_show_hidden_files && isHidden(entry))
			continue;

	if (fs::is_directory(*entry))
	{
		items.push_back(MPD::Directory(
			entry->path().native(),
			fs::last_write_time(entry->path())
		));
	}
	else if (hasSupportedExtension(*entry))
		items.push_back(getLocalSong(*entry, true));
	}
}

void getLocalDirectoryRecursively(std::vector<MPD::Song> &songs, const std::string &directory)
{
	size_t sort_offset = songs.size();
	for (fs::directory_iterator entry(directory), end; entry != end; ++entry)
	{
		if (!Config.local_browser_show_hidden_files && isHidden(entry))
			continue;

		if (fs::is_directory(*entry))
		{
			getLocalDirectoryRecursively(songs, entry->path().native());
			sort_offset = songs.size();
		}
		else if (hasSupportedExtension(*entry))
			songs.push_back(getLocalSong(*entry, false));
	};

	if (Config.browser_sort_mode != SortMode::NoOp)
	{
		std::sort(songs.begin()+sort_offset, songs.end(),
			LocaleBasedSorting(std::locale(), Config.ignore_leading_the)
		);
	}
}

void clearDirectory(const std::string &directory)
{
	for (fs::directory_iterator entry(directory), end; entry != end; ++entry)
	{
		if (!fs::is_symlink(*entry) && fs::is_directory(*entry))
			clearDirectory(entry->path().native());
		const char msg[] = "Deleting \"%1%\"...";
		Statusbar::printf(msg, wideShorten(entry->path().native(), COLS-const_strlen(msg)));
		fs::remove(entry->path());
	};
}

/***********************************************************************/

std::string itemToString(const MPD::Item &item)
{
	std::string result;
	switch (item.type())
	{
		case MPD::Item::Type::Directory:
			result = "[" + getBasename(item.directory().path()) + "]";
			break;
		case MPD::Item::Type::Song:
			switch (Config.browser_display_mode)
			{
				case DisplayMode::Classic:
					result = Format::stringify<char>(Config.song_list_format, &item.song());
					break;
				case DisplayMode::Columns:
					result = Format::stringify<char>(Config.song_columns_mode_format, &item.song());
					break;
			}
			break;
		case MPD::Item::Type::Playlist:
			result = Config.browser_playlist_prefix.str();
			result += getBasename(item.playlist().path());
			break;
	}
	return result;
}

bool browserEntryMatcher(const boost::regex &rx, const MPD::Item &item, bool filter)
{
	if (isItemParentDirectory(item))
		return filter;
	return boost::regex_search(itemToString(item), rx);
}

}
