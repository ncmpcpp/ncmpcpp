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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/locale/conversion.hpp>
#include <algorithm>
#include <array>
#include <cassert>

#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "global.h"
#include "menu_impl.h"
#include "mpdpp.h"
#include "playlist.h"
#include "media_library.h"
#include "status.h"
#include "statusbar.h"
#include "helpers/song_iterator_maker.h"
#include "utility/comparators.h"
#include "utility/functional.h"
#include "utility/type_conversions.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

namespace ph = std::placeholders;

MediaLibrary *myLibrary;

namespace {

bool hasTwoColumns;
size_t itsLeftColStartX;
size_t itsLeftColWidth;
size_t itsMiddleColWidth;
size_t itsMiddleColStartX;
size_t itsRightColWidth;
size_t itsRightColStartX;

typedef MediaLibrary::PrimaryTag PrimaryTag;
typedef MediaLibrary::AlbumEntry AlbumEntry;

MPD::SongIterator getSongsFromAlbum(const AlbumEntry &album)
{
	Mpd.StartSearch(true);
	Mpd.AddSearch(Config.media_lib_primary_tag, album.entry().tag());
	if (!album.isAllTracksEntry())
	{
		Mpd.AddSearch(MPD_TAG_ALBUM, album.entry().album());
		Mpd.AddSearch(MPD_TAG_DATE, album.entry().date());
	}
	return Mpd.CommitSearchSongs();
}

std::string AlbumToString(const AlbumEntry &ae);
std::string SongToString(const MPD::Song &s);

bool TagEntryMatcher(const Regex::Regex &rx, const MediaLibrary::PrimaryTag &tagmtime);
bool AlbumEntryMatcher(const Regex::Regex &rx, const NC::Menu<AlbumEntry>::Item &item, bool filter);
bool SongEntryMatcher(const Regex::Regex &rx, const MPD::Song &s);

bool MoveToTag(NC::Menu<PrimaryTag> &tags, const std::string &primary_tag);
bool MoveToAlbum(NC::Menu<AlbumEntry> &albums, const std::string &primary_tag, const MPD::Song &s);

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
			int ret = m_cmp(a.getTags(*get),
			                b.getTags(*get));
			if (ret != 0)
				return ret < 0;
		}
		try {
			int ret = boost::lexical_cast<int>(a.getTags(&MPD::Song::getTrackNumber))
			        - boost::lexical_cast<int>(b.getTags(&MPD::Song::getTrackNumber));
			return ret < 0;
		} catch (boost::bad_lexical_cast &) {
			return a.getTrackNumber() < b.getTrackNumber();
		}
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
, m_window_timeout(Config.data_fetching_delay ? 250 : BaseScreen::defaultWindowTimeout)
, m_fetching_delay(boost::posix_time::milliseconds(Config.data_fetching_delay ? 250 : -1))
{
	hasTwoColumns = 0;
	itsLeftColWidth = COLS/3-1;
	itsMiddleColWidth = COLS/3;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsRightColWidth = COLS-COLS/3*2-1;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	
	Tags = NC::Menu<PrimaryTag>(0, MainStartY, itsLeftColWidth, MainHeight, Config.titles_visibility ? tagTypeToString(Config.media_lib_primary_tag) + "s" : "", Config.main_color, NC::Border());
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
	
	Albums = NC::Menu<AlbumEntry>(itsMiddleColStartX, MainStartY, itsMiddleColWidth, MainHeight, Config.titles_visibility ? "Albums" : "", Config.main_color, NC::Border());
	Albums.setHighlightColor(Config.main_highlight_color);
	Albums.cyclicScrolling(Config.use_cyclic_scrolling);
	Albums.centeredCursor(Config.centered_cursor);
	Albums.setSelectedPrefix(Config.selected_item_prefix);
	Albums.setSelectedSuffix(Config.selected_item_suffix);
	Albums.setItemDisplayer([](NC::Menu<AlbumEntry> &menu) {
		menu << Charset::utf8ToLocale(AlbumToString(menu.drawn()->value()));
	});
	
	Songs = NC::Menu<MPD::Song>(itsRightColStartX, MainStartY, itsRightColWidth, MainHeight, Config.titles_visibility ? "Songs" : "", Config.main_color, NC::Border());
	Songs.setHighlightColor(Config.main_highlight_color);
	Songs.cyclicScrolling(Config.use_cyclic_scrolling);
	Songs.centeredCursor(Config.centered_cursor);
	Songs.setSelectedPrefix(Config.selected_item_prefix);
	Songs.setSelectedSuffix(Config.selected_item_suffix);
	Songs.setItemDisplayer(std::bind(
		Display::Songs, ph::_1, std::cref(Songs), std::cref(Config.song_library_format)
	));
	
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
	markSongsInPlaylist(Songs);
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
		if (Albums.empty() || m_albums_update_request)
		{
			m_albums_update_request = false;
			std::map<std::tuple<std::string, std::string, std::string>, time_t> albums;
			for (MPD::SongIterator s = getDatabaseIterator(Mpd), end; s != end; ++s)
			{
				std::string tag;
				unsigned idx = 0;
				while (!(tag = s->get(Config.media_lib_primary_tag, idx++)).empty())
				{
					auto key = std::make_tuple(std::move(tag), s->getAlbum(), s->getDate());
					auto it = albums.find(key);
					if (it == albums.end())
						albums[std::move(key)] = s->getMTime();
					else
						it->second = s->getMTime();
				}
			}
			size_t idx = 0;
			for (const auto &album : albums)
			{
				auto entry = AlbumEntry(Album(
					std::move(std::get<0>(album.first)),
					std::move(std::get<1>(album.first)),
					std::move(std::get<2>(album.first)),
					album.second)
				);
				if (idx < Albums.size())
					Albums[idx].value() = std::move(entry);
				else
					Albums.addItem(std::move(entry));
				++idx;
			}
			if (idx < Albums.size())
				Albums.resizeList(idx);
			std::sort(Albums.beginV(), Albums.endV(), SortAlbumEntries());
			Albums.refresh();
		}
	}
	else
	{
		if (Tags.empty() || m_tags_update_request)
		{
			m_tags_update_request = false;
			std::map<std::string, time_t> tags;
			if (Config.media_library_sort_by_mtime)
			{
				for (MPD::SongIterator s = getDatabaseIterator(Mpd), end; s != end; ++s)
				{
					std::string tag;
					unsigned idx = 0;
					while (!(tag = s->get(Config.media_lib_primary_tag, idx++)).empty())
					{
						auto it = tags.find(tag);
						if (it == tags.end())
							tags[std::move(tag)] = s->getMTime();
						else
							it->second = std::max(it->second, s->getMTime());
					}
				}
			}
			else
			{
				MPD::StringIterator tag = Mpd.GetList(Config.media_lib_primary_tag), end;
				for (; tag != end; ++tag)
					tags[std::move(*tag)] = 0;
			}
			size_t idx = 0;
			for (const auto &tag : tags)
			{
				auto ptag = PrimaryTag(std::move(tag.first), tag.second);
				if (idx < Tags.size())
					Tags[idx].value() = std::move(ptag);
				else
					Tags.addItem(std::move(ptag));
				++idx;
			}
			if (idx < Tags.size())
				Tags.resizeList(idx);
			std::sort(Tags.beginV(), Tags.endV(), SortPrimaryTags());
			Tags.refresh();
		}
		
		if (!Tags.empty()
		&& ((Albums.empty() && Global::Timer - m_timer > m_fetching_delay) || m_albums_update_request)
		)
		{
			m_albums_update_request = false;
			auto &primary_tag = Tags.current()->value().tag();
			Mpd.StartSearch(true);
			Mpd.AddSearch(Config.media_lib_primary_tag, primary_tag);
			std::map<std::tuple<std::string, std::string>, time_t> albums;
			for (MPD::SongIterator s = Mpd.CommitSearchSongs(), end; s != end; ++s)
			{
				auto key = std::make_tuple(s->getAlbum(), s->getDate());
				auto it = albums.find(key);
				if (it == albums.end())
					albums[std::move(key)] = s->getMTime();
				else
					it->second = std::max(it->second, s->getMTime());
			};
			size_t idx = 0;
			for (const auto &album : albums)
			{
				auto entry = AlbumEntry(Album(
					primary_tag,
					std::move(std::get<0>(album.first)),
					std::move(std::get<1>(album.first)),
					album.second)
				);
				if (idx < Albums.size())
				{
					Albums[idx].value() = std::move(entry);
					Albums[idx].setSeparator(false);
				}
				else
					Albums.addItem(std::move(entry));
				++idx;
			}
			if (idx < Albums.size())
				Albums.resizeList(idx);
			std::sort(Albums.beginV(), Albums.endV(), SortAlbumEntries());
			if (albums.size() > 1)
			{
				Albums.addSeparator();
				Albums.addItem(AlbumEntry::mkAllTracksEntry(primary_tag));
			}
			Albums.refresh();
		}
	}
	
	if (!Albums.empty()
	&& ((Songs.empty() && Global::Timer - m_timer > m_fetching_delay) || m_songs_update_request)
	)
	{
		m_songs_update_request = false;
		auto &album = Albums.current()->value();
		Mpd.StartSearch(true);
		Mpd.AddSearch(Config.media_lib_primary_tag, album.entry().tag());
		if (!album.isAllTracksEntry())
		{
			Mpd.AddSearch(MPD_TAG_ALBUM, album.entry().album());
			Mpd.AddSearch(MPD_TAG_DATE, album.entry().date());
		}
		size_t idx = 0;
		for (MPD::SongIterator s = Mpd.CommitSearchSongs(), end; s != end; ++s, ++idx)
		{
			bool in_playlist = myPlaylist->checkForSong(*s);
			if (idx < Songs.size())
			{
				Songs[idx].value() = std::move(*s);
				Songs[idx].setBold(in_playlist);
			}
			else
			{
				auto properties = NC::List::Properties::Selectable;
				if (in_playlist)
					properties |= NC::List::Properties::Bold;
				Songs.addItem(std::move(*s), properties);
			}
		};
		if (idx < Songs.size())
			Songs.resizeList(idx);
		std::sort(Songs.begin(), Songs.end(), SortSongs(!album.isAllTracksEntry()));
		Songs.refresh();
	}
}

int MediaLibrary::windowTimeout()
{
	if (Albums.empty() || Songs.empty())
		return m_window_timeout;
	else
		return Screen<WindowType>::windowTimeout();
}

void MediaLibrary::enterPressed()
{
	addItemToPlaylist(true);
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
				addItemToPlaylist();
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
				addItemToPlaylist();
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
				addItemToPlaylist();
			else
				enterPressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/***********************************************************************/

bool MediaLibrary::allowsSearching()
{
	return true;
}

void MediaLibrary::setSearchConstraint(const std::string &constraint)
{
	if (isActiveWindow(Tags))
	{
		m_tags_search_predicate = Regex::Filter<PrimaryTag>(
			Regex::make(constraint, Config.regex_type),
			TagEntryMatcher
		);
	}
	else if (isActiveWindow(Albums))
	{
		m_albums_search_predicate = Regex::ItemFilter<AlbumEntry>(
			Regex::make(constraint, Config.regex_type),
			std::bind(AlbumEntryMatcher, ph::_1, ph::_2, false)
		);
	}
	else if (isActiveWindow(Songs))
	{
		m_songs_search_predicate = Regex::Filter<MPD::Song>(
			Regex::make(constraint, Config.regex_type),
			SongEntryMatcher
		);
	}
}

void MediaLibrary::clearConstraint()
{
	if (isActiveWindow(Tags))
		m_tags_search_predicate.clear();
	else if (isActiveWindow(Albums))
		m_albums_search_predicate.clear();
	else if (isActiveWindow(Songs))
		m_songs_search_predicate.clear();
}

bool MediaLibrary::find(SearchDirection direction, bool wrap, bool skip_current)
{
	bool result = false;
	if (isActiveWindow(Tags))
		result = search(Tags, m_tags_search_predicate, direction, wrap, skip_current);
	else if (isActiveWindow(Albums))
		result = search(Albums, m_albums_search_predicate, direction, wrap, skip_current);
	else if (isActiveWindow(Songs))
		result = search(Songs, m_songs_search_predicate, direction, wrap, skip_current);
	return result;
}

/***********************************************************************/

bool MediaLibrary::addItemToPlaylist()
{
	return addItemToPlaylist(false);
}

std::vector<MPD::Song> MediaLibrary::getSelectedSongs()
{
	std::vector<MPD::Song> result;
	if (isActiveWindow(Tags))
	{
		auto tag_handler = [&result](const std::string &tag) {
			Mpd.StartSearch(true);
			Mpd.AddSearch(Config.media_lib_primary_tag, tag);
			std::copy(
				std::make_move_iterator(Mpd.CommitSearchSongs()),
				std::make_move_iterator(MPD::SongIterator()),
				std::back_inserter(result)
			);
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
			tag_handler(Tags.current()->value().tag());
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
					              Tags.current()->value().tag());
					Mpd.AddSearch(MPD_TAG_ALBUM, sc.entry().album());
				Mpd.AddSearch(MPD_TAG_DATE, sc.entry().date());
				size_t begin = result.size();
				std::copy(
					std::make_move_iterator(Mpd.CommitSearchSongs()),
					std::make_move_iterator(MPD::SongIterator()),
					std::back_inserter(result)
				);
				std::sort(result.begin()+begin, result.end(), SortSongs(false));
			}
		}
		// if no item is selected, add songs from right column
		if (!any_selected && !Albums.empty())
		{
			size_t begin = result.size();
			std::copy(
				std::make_move_iterator(getSongsFromAlbum(Albums.current()->value().entry())),
				std::make_move_iterator(MPD::SongIterator()),
				std::back_inserter(result)
			);
			std::sort(result.begin()+begin, result.end(), SortSongs(false));
		}
	}
	else if (isActiveWindow(Songs))
		Songs.getSelectedSongs();
	return result;
}

/***********************************************************************/

bool MediaLibrary::previousColumnAvailable()
{
	assert(!hasTwoColumns || !isActiveWindow(Tags));
	if (isActiveWindow(Songs))
	{
		if (!Albums.empty() && (hasTwoColumns || !Tags.empty()))
			return true;
	}
	else if (isActiveWindow(Albums))
	{
		if (!hasTwoColumns && !Tags.empty())
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
		if (!Albums.empty() && !Songs.empty())
			return true;
	}
	else if (isActiveWindow(Albums))
	{
		if (!Songs.empty())
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

void MediaLibrary::toggleSortMode()
{
	Config.media_library_sort_by_mtime = !Config.media_library_sort_by_mtime;
	Statusbar::printf("Sorting library by: %1%",
		Config.media_library_sort_by_mtime ? "modification time" : "name");
	if (hasTwoColumns)
	{
		std::sort(Albums.beginV(), Albums.endV(), SortAlbumEntries());
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
		// if we already have modification times, just resort. otherwise refetch the list.
		if (!Tags.empty() && Tags[0].value().mtime() > 0)
		{
			std::sort(Tags.beginV(), Tags.endV(), SortPrimaryTags());
			Tags.refresh();
		}
		else
			Tags.clear();
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
		if (Tags.empty())
			update();

		if (!MoveToTag(Tags, primary_tag))
		{
			// The tag could not be found. Since this was called from an existing
			// song, the tag should exist in the library, but it was not listed by
			// list/listallinfo. This is the case with some players where it is not
			// possible to list all of the library, e.g. mopidy with mopidy-spotify.
			// To workaround this we simply insert the missing tag.
			Tags.addItem(PrimaryTag(primary_tag, s.getMTime()));
			std::sort(Tags.beginV(), Tags.endV(), SortPrimaryTags());
			Tags.refresh();
			MoveToTag(Tags, primary_tag);
		}
		Albums.clear();
	}
	
	if (Albums.empty())
		update();

	// When you locate a song in the media library, if no albums or no songs
	// are found, set the active column to the previous one (tags if no albums,
	// and albums if no songs). This makes sure that the active column is not
	// empty, which may make it impossible to move out of.
	//
	// The problem was if you highlight some song in the rightmost column in
	// the media browser and then go to some other window and select locate
	// song. If the tag or album it looked up in the media library was
	// empty, the selection would stay in the songs column while it was empty.
	// This made the selection impossible to change.
	//
	// This only is a problem if a song has some tag or album for which the
	// find command doesn't return any results. This should never really happen
	// unless there is some inconsistency in the player. However, it may
	// happen, so we need to handle it.
	//
	// Note: We don't want to return when no albums are found in two column
	// mode. In this case, we try to insert the album, as we do with tags when
	// they are not found.
	if (hasTwoColumns || !Albums.empty())
	{
		if (!MoveToAlbum(Albums, primary_tag, s))
		{
			// The album could not be found, insert it if in two column mode.
			// See comment about tags not found above. This is the equivalent
			// for two column mode.
			Albums.addItem(AlbumEntry(
				Album(primary_tag, s.getAlbum(), s.getDate(), s.getMTime())
			));
			std::sort(Albums.beginV(), Albums.endV(), SortAlbumEntries());
			Albums.refresh();
			MoveToAlbum(Albums, primary_tag, s);
		}

		Songs.clear();
		update();

		if (!Songs.empty())
		{
			if (s != Songs.current()->value())
			{
				auto begin = Songs.beginV(), end = Songs.endV();
				auto it = std::find(begin, end, s);
				if (it != end)
					Songs.highlight(it-begin);
			}
			nextColumn();
			nextColumn();
		}
		else // invalid album was added, clear the list
			Albums.clear();
	}
	else // invalid tag was added, clear the list
		Tags.clear();
	refresh();
}

bool MediaLibrary::addItemToPlaylist(bool play)
{
	bool result = false;
	if (isActiveWindow(Songs) && !Songs.empty())
		result = addSongToPlaylist(Songs.current()->value(), play);
	else
	{
		if ((!Tags.empty() && isActiveWindow(Tags))
		||  (isActiveWindow(Albums) && Albums.current()->value().isAllTracksEntry()))
		{
			Mpd.StartSearch(true);
			Mpd.AddSearch(Config.media_lib_primary_tag, Tags.current()->value().tag());
			std::vector<MPD::Song> list(
				std::make_move_iterator(Mpd.CommitSearchSongs()),
				std::make_move_iterator(MPD::SongIterator())
			);
			result = addSongsToPlaylist(list.begin(), list.end(), play, -1);
			std::string tag_type = boost::locale::to_lower(
				tagTypeToString(Config.media_lib_primary_tag));
			Statusbar::printf("Songs with %1% \"%2%\" added%3%",
				tag_type, Tags.current()->value().tag(), withErrors(result)
			);
		}
		else if (isActiveWindow(Albums))
		{
			std::vector<MPD::Song> list(
				std::make_move_iterator(getSongsFromAlbum(Albums.current()->value())),
				std::make_move_iterator(MPD::SongIterator())
			);
			result = addSongsToPlaylist(list.begin(), list.end(), play, -1);
			Statusbar::printf("Songs from album \"%1%\" added%2%",
				Albums.current()->value().entry().album(), withErrors(result)
			);
		}
	}
	return result;
}

namespace {

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
	return Format::stringify<char>(
		Config.song_library_format, &s
	);
}

bool TagEntryMatcher(const Regex::Regex &rx, const PrimaryTag &pt)
{
	return Regex::search(pt.tag(), rx);
}

bool AlbumEntryMatcher(const Regex::Regex &rx, const NC::Menu<AlbumEntry>::Item &item, bool filter)
{
	if (item.isSeparator() || item.value().isAllTracksEntry())
		return filter;
	return Regex::search(AlbumToString(item.value()), rx);
}

bool SongEntryMatcher(const Regex::Regex &rx, const MPD::Song &s)
{
	return Regex::search(SongToString(s), rx);
}

bool MoveToTag(NC::Menu<PrimaryTag> &tags, const std::string &primary_tag)
{
	if (tags.empty())
		return false;

	auto equals_fun_argument = [&](PrimaryTag &e) {
		return e.tag() == primary_tag;
	};

	if (equals_fun_argument(*tags.currentV()))
		return true;

	auto begin = tags.beginV(), end = tags.endV();
	auto it = std::find_if(begin, end, equals_fun_argument);
	if (it != end)
	{
		tags.highlight(it-begin);
		return true;
	}

	return false;
}

bool MoveToAlbum(NC::Menu<AlbumEntry> &albums, const std::string &primary_tag, const MPD::Song &s)
{
	if (albums.empty())
		return false;

	std::string album = s.getAlbum();
	std::string date = s.getDate();

	auto equals_fun_argument = [&](AlbumEntry &e) {
		return (!hasTwoColumns || e.entry().tag() == primary_tag)
		    && e.entry().album() == album
		    && e.entry().date() == date;
	};

	if (equals_fun_argument(*albums.currentV()))
		return true;

	auto begin = albums.beginV(), end = albums.endV();
	auto it = std::find_if(begin, end, equals_fun_argument);
	if (it != end)
	{
		albums.highlight(it-begin);
		return true;
	}

	return false;
}

}
