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

using namespace std::placeholders;

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

MediaLibrary *myLibrary;

namespace {//

bool hasTwoColumns;
size_t itsLeftColStartX;
size_t itsLeftColWidth;
size_t itsMiddleColWidth;
size_t itsMiddleColStartX;
size_t itsRightColWidth;
size_t itsRightColStartX;

// this string marks the position in middle column that works as "All tracks" option. it's
// assigned to Date in SearchConstraint class since date normally cannot contain other chars
// than ciphers and -'s (0x7f is interpreted as backspace keycode, so it's quite safe to assume
// that it won't appear in any tag, let alone date).
const std::string AllTracksMarker = "\x7f_\x7f_\x7f";

typedef MediaLibrary::SearchConstraints SearchConstraints;

std::string AlbumToString(const SearchConstraints &sc);
std::string SongToString(const MPD::Song &s);

bool TagEntryMatcher(const Regex &rx, const std::string &tag);
bool AlbumEntryMatcher(const Regex &rx, const NC::Menu<SearchConstraints>::Item &item, bool filter);
bool SongEntryMatcher(const Regex &rx, const MPD::Song &s);

void DisplayAlbums(NC::Menu<SearchConstraints> &menu);
void DisplayPrimaryTags(NC::Menu<std::string> &menu);

bool SortSongsByTrack(const MPD::Song &a, const MPD::Song &b);

struct SortAllTracks {
	static const std::array<MPD::Song::GetFunction, 3> GetFuns;
	LocaleStringComparison m_cmp;
public:
	SortAllTracks() : m_cmp(std::locale(), Config.ignore_leading_the) { }
	bool operator()(const MPD::Song &a, const MPD::Song &b) {
		for (auto get = GetFuns.begin(); get != GetFuns.end(); ++get) {
			int ret = m_cmp(a.getTags(*get, Config.tags_separator),
			                b.getTags(*get, Config.tags_separator));
			if (ret != 0)
				return ret < 0;
		}
		return a.getTrack() < b.getTrack();
	}
};
const std::array<MPD::Song::GetFunction, 3> SortAllTracks::GetFuns = {{
	&MPD::Song::getDate,
	&MPD::Song::getAlbum,
	&MPD::Song::getDisc
}};

class SortSearchConstraints {
	LocaleStringComparison m_cmp;
public:
	SortSearchConstraints() : m_cmp(std::locale(), Config.ignore_leading_the) { }
	bool operator()(const SearchConstraints &a, const SearchConstraints &b) const {
		int result;
		result = m_cmp(a.PrimaryTag, b.PrimaryTag);
		if (result != 0)
			return result < 0;
		result = m_cmp(a.Date, b.Date);
		if (result != 0)
			return result < 0;
		return m_cmp(a.Album, b.Album) < 0;
	}
};

}

MediaLibrary::MediaLibrary()
{
	hasTwoColumns = 0;
	itsLeftColWidth = COLS/3-1;
	itsMiddleColWidth = COLS/3;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsRightColWidth = COLS-COLS/3*2-1;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	
	Tags = NC::Menu<std::string>(0, MainStartY, itsLeftColWidth, MainHeight, Config.titles_visibility ? tagTypeToString(Config.media_lib_primary_tag) + "s" : "", Config.main_color, NC::brNone);
	Tags.setHighlightColor(Config.active_column_color);
	Tags.cyclicScrolling(Config.use_cyclic_scrolling);
	Tags.centeredCursor(Config.centered_cursor);
	Tags.setSelectedPrefix(Config.selected_item_prefix);
	Tags.setSelectedSuffix(Config.selected_item_suffix);
	Tags.setItemDisplayer(DisplayPrimaryTags);
	
	Albums = NC::Menu<SearchConstraints>(itsMiddleColStartX, MainStartY, itsMiddleColWidth, MainHeight, Config.titles_visibility ? "Albums" : "", Config.main_color, NC::brNone);
	Albums.setHighlightColor(Config.main_highlight_color);
	Albums.cyclicScrolling(Config.use_cyclic_scrolling);
	Albums.centeredCursor(Config.centered_cursor);
	Albums.setSelectedPrefix(Config.selected_item_prefix);
	Albums.setSelectedSuffix(Config.selected_item_suffix);
	Albums.setItemDisplayer(DisplayAlbums);
	
	Songs = NC::Menu<MPD::Song>(itsRightColStartX, MainStartY, itsRightColWidth, MainHeight, Config.titles_visibility ? "Songs" : "", Config.main_color, NC::brNone);
	Songs.setHighlightColor(Config.main_highlight_color);
	Songs.cyclicScrolling(Config.use_cyclic_scrolling);
	Songs.centeredCursor(Config.centered_cursor);
	Songs.setSelectedPrefix(Config.selected_item_prefix);
	Songs.setSelectedSuffix(Config.selected_item_suffix);
	Songs.setItemDisplayer(std::bind(Display::Songs, _1, this, Config.song_library_format));
	
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
	mvvline(MainStartY, itsMiddleColStartX-1, 0, MainHeight);
	Albums.display();
	mvvline(MainStartY, itsRightColStartX-1, 0, MainHeight);
	Songs.display();
	if (Albums.empty())
	{
		Albums << NC::XY(0, 0) << "No albums found.";
		Albums.Window::refresh();
	}
}

void MediaLibrary::switchTo()
{
	if (myScreen == this)
	{
		if (!Config.media_library_disable_two_column_mode)
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
					std::string item_type = lowercase(tagTypeToString(Config.media_lib_primary_tag));
					Albums.setTitle("Albums (sorted by " + item_type + ")");
				}
				else
					Albums.setTitle("");
			}
			else
				Albums.setTitle(Config.titles_visibility ? "Albums" : "");
			resize();
			refresh();
		}
	}
	else
	{
		SwitchTo::execute(this);
		markSongsInPlaylist(songsProxyList());
		drawHeader();
		refresh();
	}
}

std::wstring MediaLibrary::title()
{
	return L"Media library";
}

void MediaLibrary::update()
{
	if (!hasTwoColumns && Tags.reallyEmpty())
	{
		Albums.clear();
		Songs.clear();
		auto list = Mpd.GetList(Config.media_lib_primary_tag);
		std::sort(list.begin(), list.end(),
			LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
		for (auto it = list.begin(); it != list.end(); ++it)
		{
			if (it->empty() && !Config.media_library_display_empty_tag)
				continue;
			Tags.addItem(*it);
		}
		Tags.refresh();
	}
	
	if (!hasTwoColumns && !Tags.empty() && Albums.reallyEmpty() && Songs.reallyEmpty())
	{
		// idle has to be blocked for now since it would be enabled and
		// disabled a few times by each mpd command, which makes no sense
		// and slows down the whole process.
		Mpd.BlockIdle(true);
		Albums.reset();
		Mpd.StartFieldSearch(MPD_TAG_ALBUM);
		Mpd.AddSearch(Config.media_lib_primary_tag, Tags.current().value());
		auto albums = Mpd.CommitSearchTags();
		for (auto album = albums.begin(); album != albums.end(); ++album)
		{
			if (Config.media_library_display_date)
			{
				Mpd.StartFieldSearch(MPD_TAG_DATE);
				Mpd.AddSearch(Config.media_lib_primary_tag, Tags.current().value());
				Mpd.AddSearch(MPD_TAG_ALBUM, *album);
				auto dates = Mpd.CommitSearchTags();
				for (auto date = dates.begin(); date != dates.end(); ++date)
					Albums.addItem(SearchConstraints(*album, *date));
			}
			else
				Albums.addItem(SearchConstraints(*album, ""));
		}
		if (!Albums.empty())
			std::sort(Albums.beginV(), Albums.endV(), SortSearchConstraints());
		if (Albums.size() > 1)
		{
			Albums.addSeparator();
			Albums.addItem(SearchConstraints("", AllTracksMarker));
		}
		Albums.refresh();
		Mpd.BlockIdle(false);
	}
	else if (hasTwoColumns && Albums.reallyEmpty())
	{
		Songs.clear();
		Albums << NC::XY(0, 0) << "Fetching albums...";
		Albums.Window::refresh();
		Mpd.BlockIdle(true);
		auto artists = Mpd.GetList(Config.media_lib_primary_tag);
		for (auto artist = artists.begin(); artist != artists.end(); ++artist)
		{
			Mpd.StartFieldSearch(MPD_TAG_ALBUM);
			Mpd.AddSearch(Config.media_lib_primary_tag, *artist);
			auto albums = Mpd.CommitSearchTags();
			for (auto album = albums.begin(); album != albums.end(); ++album)
			{
				if (Config.media_library_display_date)
				{
					if (Config.media_lib_primary_tag != MPD_TAG_DATE)
					{
						Mpd.StartFieldSearch(MPD_TAG_DATE);
						Mpd.AddSearch(Config.media_lib_primary_tag, *artist);
						Mpd.AddSearch(MPD_TAG_ALBUM, *album);
						auto dates = Mpd.CommitSearchTags();
						for (auto date = dates.begin(); date != dates.end(); ++date)
							Albums.addItem(SearchConstraints(*artist, *album, *date));
					}
					else
						Albums.addItem(SearchConstraints(*artist, *album, *artist));
				}
				else
					Albums.addItem(SearchConstraints(*artist, *album, ""));
			}
		}
		Mpd.BlockIdle(0);
		if (!Albums.empty())
			std::sort(Albums.beginV(), Albums.endV(), SortSearchConstraints());
		Albums.refresh();
	}
	
	if (!hasTwoColumns && !Tags.empty() && isActiveWindow(Albums) && Albums.reallyEmpty())
	{
		Albums.setHighlightColor(Config.main_highlight_color);
		Tags.setHighlightColor(Config.active_column_color);
		w = &Tags;
	}
	
	if (!(hasTwoColumns ? Albums.empty() : Tags.empty()) && Songs.reallyEmpty())
	{
		Songs.reset();
		
		Mpd.StartSearch(1);
		Mpd.AddSearch(Config.media_lib_primary_tag, hasTwoColumns ? Albums.current().value().PrimaryTag : Tags.current().value());
		if (Albums.current().value().Date != AllTracksMarker)
		{
			Mpd.AddSearch(MPD_TAG_ALBUM, Albums.current().value().Album);
			if (Config.media_library_display_date)
				Mpd.AddSearch(MPD_TAG_DATE, Albums.current().value().Date);
		}
		auto songs = Mpd.CommitSearchSongs();
		for (auto s = songs.begin(); s != songs.end(); ++s)
			Songs.addItem(*s, myPlaylist->checkForSong(*s));
		
		if (Albums.current().value().Date == AllTracksMarker)
			std::sort(Songs.beginV(), Songs.endV(), SortAllTracks());
		else
			std::sort(Songs.beginV(), Songs.endV(), SortSongsByTrack);
		
		Songs.refresh();
	}
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
			size_t i = Tags.choice();
			Tags.at(i).setSelected(!Tags.at(i).isSelected());
			Albums.clear();
			Songs.clear();
		}
		else if (isActiveWindow(Albums))
		{
			if (Albums.current().value().Date != AllTracksMarker)
			{
				size_t i = Albums.choice();
				Albums.at(i).setSelected(!Albums.at(i).isSelected());
				Songs.clear();
			}
		}
		else if (isActiveWindow(Songs))
		{
			size_t i = Songs.choice();
			Songs.at(i).setSelected(!Songs.at(i).isSelected());
		}
		w->scroll(NC::wDown);
	}
	else
		AddToPlaylist(0);
}

void MediaLibrary::mouseButtonPressed(MEVENT me)
{
	auto tryNextColumn = [this]() -> bool {
		bool result = true;
		if (this->isActiveWindow(Songs))
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
		if (this->isActiveWindow(Tags))
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
					Tags.scroll(NC::wUp);
			}
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
		Albums.clear();
		Songs.clear();
	}
	else if (!Albums.empty() && Albums.hasCoords(me.x, me.y))
	{
		if (isActiveWindow(Albums))
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
					Albums.scroll(NC::wUp);
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
					Songs.scroll(NC::wUp);
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
		filter = RegexFilter<std::string>::currentFilter(Tags);
	else if (isActiveWindow(Albums))
		filter = RegexItemFilter<SearchConstraints>::currentFilter(Albums);
	else if (isActiveWindow(Songs))
		filter = RegexFilter<MPD::Song>::currentFilter(Songs);
	return filter;
}

void MediaLibrary::applyFilter(const std::string &filter)
{
	if (isActiveWindow(Tags))
	{
		Tags.showAll();
		auto rx = RegexFilter<std::string>(filter, Config.regex_type, TagEntryMatcher);
		Tags.filter(Tags.begin(), Tags.end(), rx);
	}
	else if (isActiveWindow(Albums))
	{
		Albums.showAll();
		auto fun = std::bind(AlbumEntryMatcher, _1, _2, true);
		auto rx = RegexItemFilter<SearchConstraints>(filter, Config.regex_type, fun);
		Albums.filter(Albums.begin(), Albums.end(), rx);
	}
	else if (isActiveWindow(Songs))
	{
		Songs.showAll();
		auto rx = RegexFilter<MPD::Song>(filter, Config.regex_type, SongEntryMatcher);
		Songs.filter(Songs.begin(), Songs.end(), rx);
	}
}

/***********************************************************************/

bool MediaLibrary::allowsSearching()
{
	return true;
}

bool MediaLibrary::search(const std::string &constraint)
{
	bool result = false;
	if (isActiveWindow(Tags))
	{
		auto rx = RegexFilter<std::string>(constraint, Config.regex_type, TagEntryMatcher);
		result = Tags.search(Tags.begin(), Tags.end(), rx);
	}
	else if (isActiveWindow(Albums))
	{
		auto fun = std::bind(AlbumEntryMatcher, _1, _2, false);
		auto rx = RegexItemFilter<SearchConstraints>(constraint, Config.regex_type, fun);
		result = Albums.search(Albums.begin(), Albums.end(), rx);
	}
	else if (isActiveWindow(Songs))
	{
		auto rx = RegexFilter<MPD::Song>(constraint, Config.regex_type, SongEntryMatcher);
		result = Songs.search(Songs.begin(), Songs.end(), rx);
	}
	return result;
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

std::shared_ptr<ProxySongList> MediaLibrary::getProxySongList()
{
	auto ptr = nullProxySongList();
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
			auto songs = Mpd.CommitSearchSongs();
			std::sort(songs.begin(), songs.end(), SortAllTracks());
			result.insert(result.end(), songs.begin(), songs.end());
		};
		for (auto it = Tags.begin(); it != Tags.end(); ++it)
			if (it->isSelected())
				tag_handler(it->value());
		// if no item is selected, add current one
		if (result.empty() && !Tags.empty())
			tag_handler(Tags.current().value());
	}
	else if (isActiveWindow(Albums))
	{
		for (auto it = Albums.begin(); it != Albums.end() && !it->isSeparator(); ++it)
		{
			if (it->isSelected())
			{
				auto &sc = it->value();
				Mpd.StartSearch(true);
				if (hasTwoColumns)
					Mpd.AddSearch(Config.media_lib_primary_tag, sc.PrimaryTag);
				else
					Mpd.AddSearch(Config.media_lib_primary_tag, Tags.current().value());
				Mpd.AddSearch(MPD_TAG_ALBUM, sc.Album);
				Mpd.AddSearch(MPD_TAG_DATE, sc.Date);
				auto songs = Mpd.CommitSearchSongs();
				std::sort(songs.begin(), songs.end(), SortSongsByTrack);
				result.insert(result.end(), songs.begin(), songs.end());
			}
		}
		// if no item is selected, add songs from right column
		if (result.empty() && !Albums.empty())
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

int MediaLibrary::Columns()
{
	if (hasTwoColumns)
		return 2;
	else
		return 3;
}

std::shared_ptr<ProxySongList> MediaLibrary::songsProxyList()
{
	return mkProxySongList(Songs, [](NC::Menu<MPD::Song>::Item &item) {
		return &item.value();
	});
}

void MediaLibrary::LocateSong(const MPD::Song &s)
{
	std::string primary_tag;
	switch (Config.media_lib_primary_tag)
	{
		case MPD_TAG_ARTIST:
			primary_tag = s.getArtist();
			break;
		case MPD_TAG_DATE:
			primary_tag = s.getDate();
			break;
		case MPD_TAG_GENRE:
			primary_tag = s.getGenre();
			break;
		case MPD_TAG_COMPOSER:
			primary_tag = s.getComposer();
			break;
		case MPD_TAG_PERFORMER:
			primary_tag = s.getPerformer();
			break;
		default: // shouldn't happen
			assert(false);
			return;
	}
	if (primary_tag.empty())
	{
		std::string item_type = lowercase(tagTypeToString(Config.media_lib_primary_tag));
		Statusbar::msg("Can't use this function because the song has no %s tag set", item_type.c_str());
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
		if (primary_tag != Tags.current().value())
		{
			for (size_t i = 0; i < Tags.size(); ++i)
			{
				if (primary_tag == Tags[i].value())
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
	if ((hasTwoColumns && Albums.current().value().PrimaryTag != primary_tag)
	||  album != Albums.current().value().Album
	||  date != Albums.current().value().Date)
	{
		for (size_t i = 0; i < Albums.size(); ++i)
		{
			if ((!hasTwoColumns || Albums[i].value().PrimaryTag == primary_tag)
			&&   album == Albums[i].value().Album
			&&   date == Albums[i].value().Date)
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
	
	if (s.getHash() != Songs.current().value().getHash())
	{
		for (size_t i = 0; i < Songs.size(); ++i)
		{
			if (s.getHash()  == Songs[i].value().getHash())
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
		auto list = getSelectedSongs();
		if (addSongsToPlaylist(list, add_n_play))
		{
			if ((!Tags.empty() && isActiveWindow(Tags))
			||  (isActiveWindow(Albums) && Albums.current().value().Date == AllTracksMarker))
			{
				std::string tag_type = lowercase(tagTypeToString(Config.media_lib_primary_tag));
				Statusbar::msg("Songs with %s = \"%s\" added", tag_type.c_str(), Tags.current().value().c_str());
			}
			else if (isActiveWindow(Albums))
				Statusbar::msg("Songs from album \"%s\" added", Albums.current().value().Album.c_str());
		}
	}

	if (!add_n_play)
	{
		w->scroll(NC::wDown);
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

std::string AlbumToString(const SearchConstraints &sc)
{
	std::string result;
	if (sc.Date == AllTracksMarker)
		result = "All tracks";
	else
	{
		if (hasTwoColumns)
		{
			if (sc.PrimaryTag.empty())
				result += Config.empty_tag;
			else
				result += sc.PrimaryTag;
			result += " - ";
		}
		if (Config.media_lib_primary_tag != MPD_TAG_DATE && !sc.Date.empty())
			result += "(" + sc.Date + ") ";
		result += sc.Album.empty() ? "<no album>" : sc.Album;
	}
	return result;
}

std::string SongToString(const MPD::Song &s)
{
	return s.toString(Config.song_library_format, Config.tags_separator);
}

bool TagEntryMatcher(const Regex &rx, const std::string &tag)
{
	return rx.match(tag);
}

bool AlbumEntryMatcher(const Regex &rx, const NC::Menu<SearchConstraints>::Item &item, bool filter)
{
	if (item.isSeparator() || item.value().Date == AllTracksMarker)
		return filter;
	return rx.match(AlbumToString(item.value()));
}

bool SongEntryMatcher(const Regex &rx, const MPD::Song &s)
{
	return rx.match(SongToString(s));
}

/***********************************************************************/

void DisplayAlbums(NC::Menu<SearchConstraints> &menu)
{
	menu << AlbumToString(menu.drawn()->value());
}

void DisplayPrimaryTags(NC::Menu<std::string> &menu)
{
	const std::string &tag = menu.drawn()->value();
	if (tag.empty())
		menu << Config.empty_tag;
	else
		menu << tag;
}

/***********************************************************************/

bool SortSongsByTrack(const MPD::Song &a, const MPD::Song &b)
{
	int cmp = a.getDisc().compare(b.getDisc());
	if (cmp != 0)
		return cmp < 0;
	return a.getTrack() < b.getTrack();
}

}
