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
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/locale/conversion.hpp>
#include <algorithm>
#include <array>
#include <cassert>

#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "global.h"
#include "media_library.h"
#include "mpdpp.h"
#include "playlist.h"
#include "regex_filter.h"
#include "status.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "utility/type_conversions.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

MediaLibrary *myLibrary;

namespace {

bool hasTwoColumns;
size_t itsLeftColStartX;
size_t itsLeftColWidth;
size_t itsMiddleColWidth;
size_t itsMiddleColStartX;
size_t itsRightColWidth;
size_t itsRightColStartX;

typedef MediaLibrary::AlbumEntry AlbumEntry;

std::string AlbumToString(const AlbumEntry &ae);
std::string SongToString(const MPD::Song &s);

bool TagEntryMatcher(const boost::regex &rx, const MediaLibrary::PrimaryTag &tagmtime);
bool AlbumEntryMatcher(const boost::regex &rx, const NC::Menu<AlbumEntry>::Item &item, bool filter);
bool SongEntryMatcher(const boost::regex &rx, const MPD::Song &s);

struct SortSongs {
	typedef NC::Menu<MPD::Song>::Item SongItem;
	
	static const std::array<MPD::Song::GetFunction, 3> GetFuns;
	
	LocaleStringComparison m_cmp;
	std::ptrdiff_t m_offset;
	
public:
	SortSongs(bool disc_only)
	: m_cmp(std::locale(), Config.ignore_leading_the), m_offset(disc_only ? 2 : 0) { }
	
	bool operator()(const SongItem &a, const SongItem &b) {
		return (*this)(a.value(), b.value());
	}
	bool operator()(const MPD::Song &a, const MPD::Song &b) {
		for (auto get = GetFuns.begin()+m_offset; get != GetFuns.end(); ++get) {
			int ret = m_cmp(a.getTags(*get, Config.tags_separator),
			                b.getTags(*get, Config.tags_separator));
			if (ret != 0)
				return ret < 0;
		}
		return a.getTrack() < b.getTrack();
	}
};

const std::array<MPD::Song::GetFunction, 3> SortSongs::GetFuns = {{
	&MPD::Song::getDate,
	&MPD::Song::getAlbum,
	&MPD::Song::getDisc
}};

class SortAlbumEntries {
	typedef MediaLibrary::Album Album;
	
	LocaleStringComparison m_cmp;
	
public:
	SortAlbumEntries() : m_cmp(std::locale(), Config.ignore_leading_the) { }
	
	bool operator()(const AlbumEntry &a, const AlbumEntry &b) const {
		return (*this)(a.entry(), b.entry());
	}
	
	bool operator()(const Album &a, const Album &b) const {
		if (Config.media_library_sort_by_mtime)
			return a.mtime() > b.mtime();
		else
		{
			int result;
			result = m_cmp(a.tag(), b.tag());
			if (result != 0)
				return result < 0;
			result = m_cmp(a.date(), b.date());
			if (result != 0)
				return result < 0;
			return m_cmp(a.album(), b.album()) < 0;
		}
	}
};

class SortPrimaryTags {
	typedef MediaLibrary::PrimaryTag PrimaryTag;
	
	LocaleStringComparison m_cmp;
	
public:
	SortPrimaryTags() : m_cmp(std::locale(), Config.ignore_leading_the) { }
	
	bool operator()(const PrimaryTag &a, const PrimaryTag &b) const {
		if (Config.media_library_sort_by_mtime)
			return a.mtime() > b.mtime();
		else
			return m_cmp(a.tag(), b.tag()) < 0;
	}
};

}

MediaLibrary::MediaLibrary()
: m_timer(boost::posix_time::from_time_t(0))
, m_window_timeout(Config.data_fetching_delay ? 250 : 500)
, m_fetching_delay(boost::posix_time::milliseconds(Config.data_fetching_delay ? 250 : -1))
{
	hasTwoColumns = 0;
	itsLeftColWidth = COLS/3-1;
	itsMiddleColWidth = COLS/3;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsRightColWidth = COLS-COLS/3*2-1;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	
	Tags = NC::Menu<PrimaryTag>(0, MainStartY, itsLeftColWidth, MainHeight, Config.titles_visibility ? tagTypeToString(Config.media_lib_primary_tag) + "s" : "", Config.main_color, NC::Border::None);
	Tags.setHighlightColor(Config.active_column_color);
	Tags.cyclicScrolling(Config.use_cyclic_scrolling);
	Tags.centeredCursor(Config.centered_cursor);
	Tags.setSelectedPrefix(Config.selected_item_prefix);
	Tags.setSelectedSuffix(Config.selected_item_suffix);
	Tags.setItemDisplayer([](NC::Menu<PrimaryTag> &menu) {
		const std::string &tag = menu.drawn()->value().tag();
		if (tag.empty())
			menu << Config.empty_tag;
		else
			menu << Charset::utf8ToLocale(tag);
	});
	
	Albums = NC::Menu<AlbumEntry>(itsMiddleColStartX, MainStartY, itsMiddleColWidth, MainHeight, Config.titles_visibility ? "Albums" : "", Config.main_color, NC::Border::None);
	Albums.setHighlightColor(Config.main_highlight_color);
	Albums.cyclicScrolling(Config.use_cyclic_scrolling);
	Albums.centeredCursor(Config.centered_cursor);
	Albums.setSelectedPrefix(Config.selected_item_prefix);
	Albums.setSelectedSuffix(Config.selected_item_suffix);
	Albums.setItemDisplayer([](NC::Menu<AlbumEntry> &menu) {
		menu << Charset::utf8ToLocale(AlbumToString(menu.drawn()->value()));
	});
	
	Songs = NC::Menu<MPD::Song>(itsRightColStartX, MainStartY, itsRightColWidth, MainHeight, Config.titles_visibility ? "Songs" : "", Config.main_color, NC::Border::None);
	Songs.setHighlightColor(Config.main_highlight_color);
	Songs.cyclicScrolling(Config.use_cyclic_scrolling);
	Songs.centeredCursor(Config.centered_cursor);
	Songs.setSelectedPrefix(Config.selected_item_prefix);
	Songs.setSelectedSuffix(Config.selected_item_suffix);
	Songs.setItemDisplayer(boost::bind(Display::Songs, _1, songsProxyList(), Config.song_library_format));
	
	w = &Tags;
}

void MediaLibrary::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	if (!hasTwoColumns)
	{
		itsLeftColStartX = x_offset;
		itsLeftColWidth = width/3-1;
		itsMiddleColStartX = itsLeftColStartX+itsLeftColWidth+1;
		itsMiddleColWidth = width/3;
		itsRightColStartX = itsMiddleColStartX+itsMiddleColWidth+1;
		itsRightColWidth = width-width/3*2-1;
	}
	else
	{
		itsMiddleColStartX = x_offset;
		itsMiddleColWidth = width/2;
		itsRightColStartX = x_offset+itsMiddleColWidth+1;
		itsRightColWidth = width-itsMiddleColWidth-1;
	}
	
	Tags.resize(itsLeftColWidth, MainHeight);
	Albums.resize(itsMiddleColWidth, MainHeight);
	Songs.resize(itsRightColWidth, MainHeight);
	
	Tags.moveTo(itsLeftColStartX, MainStartY);
	Albums.moveTo(itsMiddleColStartX, MainStartY);
	Songs.moveTo(itsRightColStartX, MainStartY);
	
	hasToBeResized = 0;
}

void MediaLibrary::refresh()
{
	Tags.display();
	drawSeparator(itsMiddleColStartX-1);
	Albums.display();
	drawSeparator(itsRightColStartX-1);
	Songs.display();
	if (Albums.empty())
	{
		Albums << NC::XY(0, 0) << "No albums found.";
		Albums.Window::refresh();
	}
}

void MediaLibrary::switchTo()
{
	SwitchTo::execute(this);
	markSongsInPlaylist(songsProxyList());
	drawHeader();
	refresh();
}

std::wstring MediaLibrary::title()
{
	return L"Media library";
}

void MediaLibrary::update()
{
	if (hasTwoColumns)
	{
		if (Albums.reallyEmpty() || m_albums_update_request)
		{
			Albums.clearSearchResults();
			m_albums_update_request = false;
			std::map<std::tuple<std::string, std::string, std::string>, time_t> albums;
			Mpd.GetDirectoryRecursive("/", [&albums](MPD::Song s) {
				unsigned idx = 0;
				std::string tag = s.get(Config.media_lib_primary_tag, idx);
				do
				{
					auto key = std::make_tuple(tag, s.getAlbum(), s.getDate());
					auto it = albums.find(key);
					if (it == albums.end())
						albums[key] = s.getMTime();
					else
						it->second = s.getMTime();
				}
				while (!(tag = s.get(Config.media_lib_primary_tag, ++idx)).empty());
			});
			withUnfilteredMenuReapplyFilter(Albums, [this, &albums]() {
				size_t idx = 0;
				for (auto it = albums.begin(); it != albums.end(); ++it, ++idx)
				{
					auto &&entry = AlbumEntry(Album(
						std::move(std::get<0>(it->first)),
						std::move(std::get<1>(it->first)),
						std::move(std::get<2>(it->first)),
						it->second));
					if (idx < Albums.size())
						Albums[idx].value() = entry;
					else
						Albums.addItem(entry);
				}
				if (idx < Albums.size())
					Albums.resizeList(idx);
				std::sort(Albums.beginV(), Albums.endV(), SortAlbumEntries());
			});
			Albums.refresh();
		}
	}
	else
	{
		if (Tags.reallyEmpty() || m_tags_update_request)
		{
			Tags.clearSearchResults();
			m_tags_update_request = false;
			std::map<std::string, time_t> tags;
			if (Config.media_library_sort_by_mtime)
			{
				Mpd.GetDirectoryRecursive("/", [&tags](MPD::Song s) {
					unsigned idx = 0;
					std::string tag = s.get(Config.media_lib_primary_tag, idx);
					do
					{
						auto it = tags.find(tag);
						if (it == tags.end())
							tags[tag] = s.getMTime();
						else
							it->second = std::max(it->second, s.getMTime());
					}
					while (!(tag = s.get(Config.media_lib_primary_tag, ++idx)).empty());
				});
			}
			else
			{
				Mpd.GetList(Config.media_lib_primary_tag, [&tags](std::string tag) {
					tags[tag] = 0;
				});
			}
			withUnfilteredMenuReapplyFilter(Tags, [this, &tags]() {
				size_t idx = 0;
				for (auto it = tags.begin(); it != tags.end(); ++it, ++idx)
				{
					auto &&tag = PrimaryTag(std::move(it->first), it->second);
					if (idx < Tags.size())
						Tags[idx].value() = tag;
					else
						Tags.addItem(tag);
				}
				if (idx < Tags.size())
					Tags.resizeList(idx);
				std::sort(Tags.beginV(), Tags.endV(), SortPrimaryTags());
			});
			Tags.refresh();
		}
		
		if (!Tags.empty()
		&& ((Albums.reallyEmpty() && Global::Timer - m_timer > m_fetching_delay) || m_albums_update_request)
		)
		{
			Albums.clearSearchResults();
			m_albums_update_request = false;
			auto &primary_tag = Tags.current().value().tag();
			Mpd.StartSearch(true);
			Mpd.AddSearch(Config.media_lib_primary_tag, primary_tag);
			std::map<std::tuple<std::string, std::string>, time_t> albums;
			Mpd.CommitSearchSongs([&albums](MPD::Song s) {
				auto key = std::make_tuple(s.getAlbum(), s.getDate());
				auto it = albums.find(key);
				if (it == albums.end())
					albums[key] = s.getMTime();
				else
					it->second = s.getMTime();
			});
			withUnfilteredMenuReapplyFilter(Albums, [this, &albums, &primary_tag]() {
				size_t idx = 0;
				for (auto it = albums.begin(); it != albums.end(); ++it, ++idx)
				{
					auto &&entry = AlbumEntry(Album(
						primary_tag,
						std::move(std::get<0>(it->first)),
						std::move(std::get<1>(it->first)),
						it->second));
					if (idx < Albums.size())
					{
						Albums[idx].value() = entry;
						Albums[idx].setSeparator(false);
					}
					else
						Albums.addItem(entry);
				}
				if (idx < Albums.size())
					Albums.resizeList(idx);
				std::sort(Albums.beginV(), Albums.endV(), SortAlbumEntries());
				if (albums.size() > 1)
				{
					Albums.addSeparator();
					Albums.addItem(AlbumEntry::mkAllTracksEntry(primary_tag));
				}
			});
			Albums.refresh();
		}
	}
	
	if (!Albums.empty()
	&& ((Songs.reallyEmpty() && Global::Timer - m_timer > m_fetching_delay) || m_songs_update_request)
	)
	{
		Songs.clearSearchResults();
		m_songs_update_request = false;
		auto &album = Albums.current().value();
		Mpd.StartSearch(true);
		Mpd.AddSearch(Config.media_lib_primary_tag, album.entry().tag());
		if (!album.isAllTracksEntry())
		{
			Mpd.AddSearch(MPD_TAG_ALBUM, album.entry().album());
			Mpd.AddSearch(MPD_TAG_DATE, album.entry().date());
		}
		withUnfilteredMenuReapplyFilter(Songs, [this, &album]() {
			size_t idx = 0;
			Mpd.CommitSearchSongs([this, &idx](MPD::Song s) {
				bool is_playlist = myPlaylist->checkForSong(s);
				if (idx < Songs.size())
				{
					Songs[idx].value() = s;
					Songs[idx].setBold(is_playlist);
				}
				else
					Songs.addItem(s, is_playlist);
				++idx;
			});
			if (idx < Songs.size())
				Songs.resizeList(idx);
			std::sort(Songs.begin(), Songs.end(), SortSongs(!album.isAllTracksEntry()));
		});
		Songs.refresh();
	}
}

int MediaLibrary::windowTimeout()
{
	if (Albums.reallyEmpty() || Songs.reallyEmpty())
		return m_window_timeout;
	else
		return Screen<WindowType>::windowTimeout();
}

void MediaLibrary::enterPressed()
{
	AddToPlaylist(true);
}

void MediaLibrary::spacePressed()
{
	if (Config.space_selects)
	{
		if (isActiveWindow(Tags))
		{
			size_t idx = Tags.choice();
			Tags[idx].setSelected(!Tags[idx].isSelected());
			Tags.scroll(NC::Scroll::Down);
			Albums.clear();
			Songs.clear();
		}
		else if (isActiveWindow(Albums))
		{
			if (!Albums.current().value().isAllTracksEntry())
			{
				size_t idx = Albums.choice();
				Albums[idx].setSelected(!Albums[idx].isSelected());
				Albums.scroll(NC::Scroll::Down);
				Songs.clear();
			}
		}
		else if (isActiveWindow(Songs))
		{
			size_t idx = Songs.choice();
			Songs[idx].setSelected(!Songs[idx].isSelected());
			Songs.scroll(NC::Scroll::Down);
		}
	}
	else
		AddToPlaylist(false);
}

void MediaLibrary::mouseButtonPressed(MEVENT me)
{
	auto tryNextColumn = [this]() -> bool {
		bool result = true;
		if (!isActiveWindow(Songs))
		{
			if (nextColumnAvailable())
				nextColumn();
			else
				result = false;
		}
		return result;
	};
	auto tryPreviousColumn = [this]() -> bool {
		bool result = true;
		if (!isActiveWindow(Tags))
		{
			if (previousColumnAvailable())
				previousColumn();
			else
				result = false;
		}
		return result;
	};
	if (!Tags.empty() && Tags.hasCoords(me.x, me.y))
	{
		if (!tryPreviousColumn() || !tryPreviousColumn())
			return;
		if (size_t(me.y) < Tags.size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Tags.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Tags.choice();
				spacePressed();
				if (pos < Tags.size()-1)
					Tags.scroll(NC::Scroll::Up);
			}
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
		Albums.clear();
		Songs.clear();
	}
	else if (!Albums.empty() && Albums.hasCoords(me.x, me.y))
	{
		if (!isActiveWindow(Albums))
		{
			bool success;
			if (isActiveWindow(Tags))
				success = tryNextColumn();
			else
				success = tryPreviousColumn();
			if (!success)
				return;
		}
		if (size_t(me.y) < Albums.size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Albums.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Albums.choice();
				spacePressed();
				if (pos < Albums.size()-1)
					Albums.scroll(NC::Scroll::Up);
			}
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
		Songs.clear();
	}
	else if (!Songs.empty() && Songs.hasCoords(me.x, me.y))
	{
		if (!tryNextColumn() || !tryNextColumn())
			return;
		if (size_t(me.y) < Songs.size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Songs.Goto(me.y);
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = Songs.choice();
				spacePressed();
				if (pos < Songs.size()-1)
					Songs.scroll(NC::Scroll::Up);
			}
			else
				enterPressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/***********************************************************************/

bool MediaLibrary::allowsFiltering()
{
	return true;
}

std::string MediaLibrary::currentFilter()
{
	std::string filter;
	if (isActiveWindow(Tags))
		filter = RegexFilter<PrimaryTag>::currentFilter(Tags);
	else if (isActiveWindow(Albums))
		filter = RegexItemFilter<AlbumEntry>::currentFilter(Albums);
	else if (isActiveWindow(Songs))
		filter = RegexFilter<MPD::Song>::currentFilter(Songs);
	return filter;
}

void MediaLibrary::applyFilter(const std::string &filter)
{
	if (filter.empty())
	{
		if (isActiveWindow(Tags))
		{
			Tags.clearFilter();
			Tags.clearFilterResults();
		}
		else if (isActiveWindow(Albums))
		{
			Albums.clearFilter();
			Albums.clearFilterResults();
		}
		else if (isActiveWindow(Songs))
		{
			Songs.clearFilter();
			Songs.clearFilterResults();
		}
		return;
	}
	try
	{
		if (isActiveWindow(Tags))
		{
			Tags.showAll();
			auto rx = RegexFilter<PrimaryTag>(
				boost::regex(filter, Config.regex_type), TagEntryMatcher);
			Tags.filter(Tags.begin(), Tags.end(), rx);
		}
		else if (isActiveWindow(Albums))
		{
			Albums.showAll();
			auto fun = boost::bind(AlbumEntryMatcher, _1, _2, true);
			auto rx = RegexItemFilter<AlbumEntry>(
				boost::regex(filter, Config.regex_type), fun);
			Albums.filter(Albums.begin(), Albums.end(), rx);
		}
		else if (isActiveWindow(Songs))
		{
			Songs.showAll();
			auto rx = RegexFilter<MPD::Song>(
				boost::regex(filter, Config.regex_type), SongEntryMatcher);
			Songs.filter(Songs.begin(), Songs.end(), rx);
		}
	}
	catch (boost::bad_expression &) { }
}

/***********************************************************************/

bool MediaLibrary::allowsSearching()
{
	return true;
}

bool MediaLibrary::search(const std::string &constraint)
{
	if (constraint.empty())
	{
		if (isActiveWindow(Tags))
			Tags.clearSearchResults();
		else if (isActiveWindow(Albums))
			Albums.clearSearchResults();
		else if (isActiveWindow(Songs))
			Songs.clearSearchResults();
		return false;
	}
	try
	{
		bool result = false;
		if (isActiveWindow(Tags))
		{
			auto rx = RegexFilter<PrimaryTag>(
				boost::regex(constraint, Config.regex_type), TagEntryMatcher);
			result = Tags.search(Tags.begin(), Tags.end(), rx);
		}
		else if (isActiveWindow(Albums))
		{
			auto fun = boost::bind(AlbumEntryMatcher, _1, _2, false);
			auto rx = RegexItemFilter<AlbumEntry>(
				boost::regex(constraint, Config.regex_type), fun);
			result = Albums.search(Albums.begin(), Albums.end(), rx);
		}
		else if (isActiveWindow(Songs))
		{
			auto rx = RegexFilter<MPD::Song>(
				boost::regex(constraint, Config.regex_type), SongEntryMatcher);
			result = Songs.search(Songs.begin(), Songs.end(), rx);
		}
		return result;
	}
	catch (boost::bad_expression &)
	{
		return false;
	}
}

void MediaLibrary::nextFound(bool wrap)
{
	if (isActiveWindow(Tags))
		Tags.nextFound(wrap);
	else if (isActiveWindow(Albums))
		Albums.nextFound(wrap);
	else if (isActiveWindow(Songs))
		Songs.nextFound(wrap);
}

void MediaLibrary::prevFound(bool wrap)
{
	if (isActiveWindow(Tags))
		Tags.prevFound(wrap);
	else if (isActiveWindow(Albums))
		Albums.prevFound(wrap);
	else if (isActiveWindow(Songs))
		Songs.prevFound(wrap);
}

/***********************************************************************/

ProxySongList MediaLibrary::proxySongList()
{
	auto ptr = ProxySongList();
	if (isActiveWindow(Songs))
		ptr = songsProxyList();
	return ptr;
}

bool MediaLibrary::allowsSelection()
{
	return true;
}

void MediaLibrary::reverseSelection()
{
	if (isActiveWindow(Tags))
		reverseSelectionHelper(Tags.begin(), Tags.end());
	else if (isActiveWindow(Albums))
	{
		// omit "All tracks"
		if (Albums.size() > 1)
			reverseSelectionHelper(Albums.begin(), Albums.end()-2);
		else
			reverseSelectionHelper(Albums.begin(), Albums.end());
	}
	else if (isActiveWindow(Songs))
		reverseSelectionHelper(Songs.begin(), Songs.end());
}

MPD::SongList MediaLibrary::getSelectedSongs()
{
	MPD::SongList result;
	if (isActiveWindow(Tags))
	{
		auto tag_handler = [&result](const std::string &tag) {
			Mpd.StartSearch(true);
			Mpd.AddSearch(Config.media_lib_primary_tag, tag);
			Mpd.CommitSearchSongs(vectorMoveInserter(result));
		};
		bool any_selected = false;
		for (auto &e : Tags)
		{
			if (e.isSelected())
			{
				any_selected = true;
				tag_handler(e.value().tag());
			}
		}
		// if no item is selected, add current one
		if (!any_selected && !Tags.empty())
			tag_handler(Tags.current().value().tag());
	}
	else if (isActiveWindow(Albums))
	{
		bool any_selected = false;
		for (auto it = Albums.begin(); it != Albums.end() && !it->isSeparator(); ++it)
		{
			if (it->isSelected())
			{
				any_selected = true;
				auto &sc = it->value();
				Mpd.StartSearch(true);
				if (hasTwoColumns)
					Mpd.AddSearch(Config.media_lib_primary_tag, sc.entry().tag());
				else
					Mpd.AddSearch(Config.media_lib_primary_tag,
					              Tags.current().value().tag());
					Mpd.AddSearch(MPD_TAG_ALBUM, sc.entry().album());
				Mpd.AddSearch(MPD_TAG_DATE, sc.entry().date());
				size_t begin = result.size();
				Mpd.CommitSearchSongs(vectorMoveInserter(result));
				std::sort(result.begin()+begin, result.end(), SortSongs(false));
			}
		}
		// if no item is selected, add songs from right column
		if (!any_selected && !Albums.empty())
		{
			withUnfilteredMenu(Songs, [this, &result]() {
				result.insert(result.end(), Songs.beginV(), Songs.endV());
			});
		}
	}
	else if (isActiveWindow(Songs))
	{
		for (auto it = Songs.begin(); it != Songs.end(); ++it)
			if (it->isSelected())
				result.push_back(it->value());
		// if no item is selected, add current one
		if (result.empty() && !Songs.empty())
			result.push_back(Songs.current().value());
	}
	return result;
}

/***********************************************************************/

bool MediaLibrary::previousColumnAvailable()
{
	assert(!hasTwoColumns || !isActiveWindow(Tags));
	if (isActiveWindow(Songs))
	{
		if (!Albums.reallyEmpty() && (hasTwoColumns || !Tags.reallyEmpty()))
			return true;
	}
	else if (isActiveWindow(Albums))
	{
		if (!hasTwoColumns && !Tags.reallyEmpty())
			return true;
	}
	return false;
}

void MediaLibrary::previousColumn()
{
	if (isActiveWindow(Songs))
	{
		Songs.setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = &Albums;
		Albums.setHighlightColor(Config.active_column_color);
	}
	else if (isActiveWindow(Albums) && !hasTwoColumns)
	{
		Albums.setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = &Tags;
		Tags.setHighlightColor(Config.active_column_color);
	}
}

bool MediaLibrary::nextColumnAvailable()
{
	assert(!hasTwoColumns || !isActiveWindow(Tags));
	if (isActiveWindow(Tags))
	{
		if (!Albums.reallyEmpty() && !Songs.reallyEmpty())
			return true;
	}
	else if (isActiveWindow(Albums))
	{
		if (!Songs.reallyEmpty())
			return true;
	}
	return false;
}

void MediaLibrary::nextColumn()
{
	if (isActiveWindow(Tags))
	{
		Tags.setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = &Albums;
		Albums.setHighlightColor(Config.active_column_color);
	}
	else if (isActiveWindow(Albums))
	{
		Albums.setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = &Songs;
		Songs.setHighlightColor(Config.active_column_color);
	}
}

/***********************************************************************/

void MediaLibrary::updateTimer()
{
	m_timer = Global::Timer;
}

void MediaLibrary::toggleColumnsMode()
{
	hasTwoColumns = !hasTwoColumns;
	Tags.clear();
	Albums.clear();
	Albums.reset();
	Songs.clear();
	if (hasTwoColumns)
	{
		if (isActiveWindow(Tags))
			nextColumn();
		if (Config.titles_visibility)
		{
			std::string item_type = boost::locale::to_lower(
				tagTypeToString(Config.media_lib_primary_tag));
			std::string and_mtime = Config.media_library_sort_by_mtime ? " and mtime" : "";
			Albums.setTitle("Albums (sorted by " + item_type + and_mtime + ")");
		}
	}
	else
		Albums.setTitle(Config.titles_visibility ? "Albums" : "");
	resize();
}

int MediaLibrary::Columns()
{
	if (hasTwoColumns)
		return 2;
	else
		return 3;
}

ProxySongList MediaLibrary::songsProxyList()
{
	return ProxySongList(Songs, [](NC::Menu<MPD::Song>::Item &item) {
		return &item.value();
	});
}

void MediaLibrary::toggleSortMode()
{
	Config.media_library_sort_by_mtime = !Config.media_library_sort_by_mtime;
	Statusbar::printf("Sorting library by: %1%",
		Config.media_library_sort_by_mtime ? "modification time" : "name");
	if (hasTwoColumns)
	{
		withUnfilteredMenuReapplyFilter(Albums, [this]() {
			std::sort(Albums.beginV(), Albums.endV(), SortAlbumEntries());
		});
		Albums.refresh();
		Songs.clear();
		if (Config.titles_visibility)
		{
			std::string item_type = boost::locale::to_lower(
				tagTypeToString(Config.media_lib_primary_tag));
			std::string and_mtime = Config.media_library_sort_by_mtime ? (" " "and mtime") : "";
			Albums.setTitle("Albums (sorted by " + item_type + and_mtime + ")");
		}
	}
	else
	{
		withUnfilteredMenuReapplyFilter(Tags, [this]() {
			// if we already have modification times,
			// just resort. otherwise refetch the list.
			if (!Tags.empty() && Tags[0].value().mtime() > 0)
			{
				std::sort(Tags.beginV(), Tags.endV(), SortPrimaryTags());
				Tags.refresh();
			}
			else
				Tags.clear();
		});
		Albums.clear();
		Songs.clear();
	}
	update();
}

void MediaLibrary::LocateSong(const MPD::Song &s)
{
	std::string primary_tag = s.get(Config.media_lib_primary_tag);
	if (primary_tag.empty())
	{
		std::string item_type = boost::locale::to_lower(
			tagTypeToString(Config.media_lib_primary_tag));
		Statusbar::printf("Can't use this function because the song has no %s tag", item_type);
		return;
	}
	if (!s.isFromDatabase())
	{
		Statusbar::print("Song is not from the database");
		return;
	}
	
	if (myScreen != this)
		switchTo();
	Statusbar::put() << "Jumping to song...";
	Global::wFooter->refresh();
	
	if (!hasTwoColumns)
	{
		Tags.showAll();
		if (Tags.empty())
			update();
		if (primary_tag != Tags.current().value().tag())
		{
			for (size_t i = 0; i < Tags.size(); ++i)
			{
				if (primary_tag == Tags[i].value().tag())
				{
					Tags.highlight(i);
					Albums.clear();
					Songs.clear();
					break;
				}
			}
		}
	}
	
	Albums.showAll();
	if (Albums.empty())
		update();
	
	std::string album = s.getAlbum();
	std::string date = s.getDate();
	if ((hasTwoColumns && Albums.current().value().entry().tag() != primary_tag)
	||  album != Albums.current().value().entry().album()
	||  date != Albums.current().value().entry().date())
	{
		for (size_t i = 0; i < Albums.size(); ++i)
		{
			if ((!hasTwoColumns || Albums[i].value().entry().tag() == primary_tag)
			&&   album == Albums[i].value().entry().album()
			&&   date == Albums[i].value().entry().date())
			{
				Albums.highlight(i);
				Songs.clear();
				break;
			}
		}
	}
	
	Songs.showAll();
	if (Songs.empty())
		update();
	
	if (s != Songs.current().value())
	{
		for (size_t i = 0; i < Songs.size(); ++i)
		{
			if (s == Songs[i].value())
			{
				Songs.highlight(i);
				break;
			}
		}
	}
	
	Tags.setHighlightColor(Config.main_highlight_color);
	Albums.setHighlightColor(Config.main_highlight_color);
	Songs.setHighlightColor(Config.active_column_color);
	w = &Songs;
	refresh();
}

void MediaLibrary::AddToPlaylist(bool add_n_play)
{
	if (isActiveWindow(Songs) && !Songs.empty())
		addSongToPlaylist(Songs.current().value(), add_n_play);
	else
	{
		if ((!Tags.empty() && isActiveWindow(Tags))
		||  (isActiveWindow(Albums) && Albums.current().value().isAllTracksEntry()))
		{
			MPD::SongList list;
			Mpd.StartSearch(true);
			Mpd.AddSearch(Config.media_lib_primary_tag, Tags.current().value().tag());
			Mpd.CommitSearchSongs(vectorMoveInserter(list));
			bool success = addSongsToPlaylist(list.begin(), list.end(), add_n_play, -1);
			std::string tag_type = boost::locale::to_lower(
				tagTypeToString(Config.media_lib_primary_tag));
			Statusbar::printf("Songs with %1% \"%2%\" added%3%",
				tag_type, Tags.current().value().tag(), withErrors(success)
			);
		}
		else if (isActiveWindow(Albums))
		{
			bool success;
			withUnfilteredMenu(Songs, [&]() {
				success = addSongsToPlaylist(Songs.beginV(), Songs.endV(), add_n_play, -1);
			});
			Statusbar::printf("Songs from album \"%1%\" added%2%",
				Albums.current().value().entry().album(), withErrors(success)
			);
		}
	}
	
	if (!add_n_play)
	{
		w->scroll(NC::Scroll::Down);
		if (isActiveWindow(Tags))
		{
			Albums.clear();
			Songs.clear();
		}
		else if (isActiveWindow(Albums))
			Songs.clear();
	}
}

namespace {//

std::string AlbumToString(const AlbumEntry &ae)
{
	std::string result;
	if (ae.isAllTracksEntry())
		result = "All tracks";
	else
	{
		if (hasTwoColumns)
		{
			if (ae.entry().tag().empty())
				result += Config.empty_tag;
			else
				result += ae.entry().tag();
			result += " - ";
		}
		if (Config.media_lib_primary_tag != MPD_TAG_DATE && !ae.entry().date().empty())
			result += "(" + ae.entry().date() + ") ";
		result += ae.entry().album().empty() ? "<no album>" : ae.entry().album();
	}
	return result;
}

std::string SongToString(const MPD::Song &s)
{
	return s.toString(Config.song_library_format, Config.tags_separator);
}

bool TagEntryMatcher(const boost::regex &rx, const MediaLibrary::PrimaryTag &pt)
{
	return boost::regex_search(pt.tag(), rx);
}

bool AlbumEntryMatcher(const boost::regex &rx, const NC::Menu<AlbumEntry>::Item &item, bool filter)
{
	if (item.isSeparator() || item.value().isAllTracksEntry())
		return filter;
	return boost::regex_search(AlbumToString(item.value()), rx);
}

bool SongEntryMatcher(const boost::regex &rx, const MPD::Song &s)
{
	return boost::regex_search(SongToString(s), rx);
}

}
