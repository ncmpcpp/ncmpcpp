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
#include "utility/comparators.h"
#include "utility/type_conversions.h"

using namespace std::placeholders;

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

MediaLibrary *myLibrary = new MediaLibrary;

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
bool SortAllTracks(const MPD::Song &a, const MPD::Song &b);
bool SortSearchConstraints(const SearchConstraints &a, const SearchConstraints &b);

}

void MediaLibrary::Init()
{
	hasTwoColumns = 0;
	itsLeftColWidth = COLS/3-1;
	itsMiddleColWidth = COLS/3;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsRightColWidth = COLS-COLS/3*2-1;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	
	Tags = new NC::Menu<std::string>(0, MainStartY, itsLeftColWidth, MainHeight, Config.titles_visibility ? tagTypeToString(Config.media_lib_primary_tag) + "s" : "", Config.main_color, NC::brNone);
	Tags->HighlightColor(Config.active_column_color);
	Tags->CyclicScrolling(Config.use_cyclic_scrolling);
	Tags->CenteredCursor(Config.centered_cursor);
	Tags->SetSelectPrefix(Config.selected_item_prefix);
	Tags->SetSelectSuffix(Config.selected_item_suffix);
	Tags->setItemDisplayer(DisplayPrimaryTags);
	
	Albums = new NC::Menu<SearchConstraints>(itsMiddleColStartX, MainStartY, itsMiddleColWidth, MainHeight, Config.titles_visibility ? "Albums" : "", Config.main_color, NC::brNone);
	Albums->HighlightColor(Config.main_highlight_color);
	Albums->CyclicScrolling(Config.use_cyclic_scrolling);
	Albums->CenteredCursor(Config.centered_cursor);
	Albums->SetSelectPrefix(Config.selected_item_prefix);
	Albums->SetSelectSuffix(Config.selected_item_suffix);
	Albums->setItemDisplayer(DisplayAlbums);
	
	Songs = new NC::Menu<MPD::Song>(itsRightColStartX, MainStartY, itsRightColWidth, MainHeight, Config.titles_visibility ? "Songs" : "", Config.main_color, NC::brNone);
	Songs->HighlightColor(Config.main_highlight_color);
	Songs->CyclicScrolling(Config.use_cyclic_scrolling);
	Songs->CenteredCursor(Config.centered_cursor);
	Songs->SetSelectPrefix(Config.selected_item_prefix);
	Songs->SetSelectSuffix(Config.selected_item_suffix);
	Songs->setItemDisplayer(std::bind(Display::Songs, _1, *this, Config.song_library_format));
	
	w = Tags;
	isInitialized = 1;
}

void MediaLibrary::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
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
	
	Tags->Resize(itsLeftColWidth, MainHeight);
	Albums->Resize(itsMiddleColWidth, MainHeight);
	Songs->Resize(itsRightColWidth, MainHeight);
	
	Tags->MoveTo(itsLeftColStartX, MainStartY);
	Albums->MoveTo(itsMiddleColStartX, MainStartY);
	Songs->MoveTo(itsRightColStartX, MainStartY);
	
	hasToBeResized = 0;
}

void MediaLibrary::Refresh()
{
	Tags->Display();
	mvvline(MainStartY, itsMiddleColStartX-1, 0, MainHeight);
	Albums->Display();
	mvvline(MainStartY, itsRightColStartX-1, 0, MainHeight);
	Songs->Display();
	if (Albums->Empty())
	{
		*Albums << NC::XY(0, 0) << "No albums found.";
		Albums->Window::Refresh();
	}
}

void MediaLibrary::SwitchTo()
{
	using Global::myLockedScreen;
	
	if (myScreen == this)
	{
		if (Config.media_library_disable_two_column_mode)
			return;
		else
		{
			hasTwoColumns = !hasTwoColumns;
			hasToBeResized = 1;
			Tags->Clear();
			Albums->Clear();
			Albums->Reset();
			Songs->Clear();
			if (hasTwoColumns)
			{
				if (w == Tags)
					NextColumn();
				if (Config.titles_visibility)
				{
					std::string item_type = tagTypeToString(Config.media_lib_primary_tag);
					lowercase(item_type);
					Albums->SetTitle("Albums (sorted by " + item_type + ")");
				}
				else
					Albums->SetTitle("");
			}
			else
				Albums->SetTitle(Config.titles_visibility ? "Albums" : "");
		}
	}
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	Global::RedrawHeader = true;
	Refresh();
	UpdateSongList(Songs);
}

std::basic_string<my_char_t> MediaLibrary::Title()
{
	return U("Media library");
}

void MediaLibrary::Update()
{
	if (!hasTwoColumns && Tags->ReallyEmpty())
	{
		Albums->Clear();
		Songs->Clear();
		auto list = Mpd.GetList(Config.media_lib_primary_tag);
		std::sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (auto it = list.begin(); it != list.end(); ++it)
		{
			if (it->empty() && !Config.media_library_display_empty_tag)
				continue;
			Tags->AddItem(*it);
		}
		Tags->Refresh();
	}
	
	if (!hasTwoColumns && !Tags->Empty() && Albums->ReallyEmpty() && Songs->ReallyEmpty())
	{
		// idle has to be blocked for now since it would be enabled and
		// disabled a few times by each mpd command, which makes no sense
		// and slows down the whole process.
		Mpd.BlockIdle(true);
		Albums->Reset();
		Mpd.StartFieldSearch(MPD_TAG_ALBUM);
		Mpd.AddSearch(Config.media_lib_primary_tag, Tags->Current().value());
		auto albums = Mpd.CommitSearchTags();
		for (auto album = albums.begin(); album != albums.end(); ++album)
		{
			if (Config.media_library_display_date)
			{
				Mpd.StartFieldSearch(MPD_TAG_DATE);
				Mpd.AddSearch(Config.media_lib_primary_tag, Tags->Current().value());
				Mpd.AddSearch(MPD_TAG_ALBUM, *album);
				auto dates = Mpd.CommitSearchTags();
				for (auto date = dates.begin(); date != dates.end(); ++date)
					Albums->AddItem(SearchConstraints(*album, *date));
			}
			else
				Albums->AddItem(SearchConstraints(*album, ""));
		}
		if (!Albums->Empty())
			std::sort(Albums->BeginV(), Albums->EndV(), SortSearchConstraints);
		if (Albums->Size() > 1)
		{
			Albums->AddSeparator();
			Albums->AddItem(SearchConstraints("", AllTracksMarker));
		}
		Albums->Refresh();
		Mpd.BlockIdle(false);
	}
	else if (hasTwoColumns && Albums->ReallyEmpty())
	{
		Songs->Clear();
		*Albums << NC::XY(0, 0) << "Fetching albums...";
		Albums->Window::Refresh();
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
							Albums->AddItem(SearchConstraints(*artist, *album, *date));
					}
					else
						Albums->AddItem(SearchConstraints(*artist, *album, *artist));
				}
				else
					Albums->AddItem(SearchConstraints(*artist, *album, ""));
			}
		}
		Mpd.BlockIdle(0);
		if (!Albums->Empty())
			std::sort(Albums->BeginV(), Albums->EndV(), SortSearchConstraints);
		Albums->Refresh();
	}
	
	if (!hasTwoColumns && !Tags->Empty() && w == Albums && Albums->ReallyEmpty())
	{
		Albums->HighlightColor(Config.main_highlight_color);
		Tags->HighlightColor(Config.active_column_color);
		w = Tags;
	}
	
	if (!(hasTwoColumns ? Albums->Empty() : Tags->Empty()) && Songs->ReallyEmpty())
	{
		Songs->Reset();
		
		Mpd.StartSearch(1);
		Mpd.AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(hasTwoColumns ? Albums->Current().value().PrimaryTag : Tags->Current().value()));
		if (Albums->Current().value().Date != AllTracksMarker)
		{
			Mpd.AddSearch(MPD_TAG_ALBUM, locale_to_utf_cpy(Albums->Current().value().Album));
			if (Config.media_library_display_date)
				Mpd.AddSearch(MPD_TAG_DATE, locale_to_utf_cpy(Albums->Current().value().Date));
		}
		auto songs = Mpd.CommitSearchSongs();
		for (auto s = songs.begin(); s != songs.end(); ++s)
			Songs->AddItem(*s, myPlaylist->checkForSong(*s));
		
		if (Albums->Current().value().Date == AllTracksMarker)
			std::sort(Songs->BeginV(), Songs->EndV(), SortAllTracks);
		else
			std::sort(Songs->BeginV(), Songs->EndV(), SortSongsByTrack);
		
		Songs->Refresh();
	}
}

void MediaLibrary::SpacePressed()
{
	if (Config.space_selects)
	{
		if (w == Tags)
		{
			size_t i = Tags->Choice();
			Tags->at(i).setSelected(!Tags->at(i).isSelected());
			Albums->Clear();
			Songs->Clear();
		}
		else if (w == Albums)
		{
			if (Albums->Current().value().Date != AllTracksMarker)
			{
				size_t i = Albums->Choice();
				Albums->at(i).setSelected(!Albums->at(i).isSelected());
				Songs->Clear();
			}
		}
		else if (w == Songs)
		{
			size_t i = Songs->Choice();
			Songs->at(i).setSelected(!Songs->at(i).isSelected());
		}
		w->Scroll(NC::wDown);
	}
	else
		AddToPlaylist(0);
}

void MediaLibrary::MouseButtonPressed(MEVENT me)
{
	if (!Tags->Empty() && Tags->hasCoords(me.x, me.y))
	{
		if (w != Tags)
		{
			PrevColumn();
			PrevColumn();
		}
		if (size_t(me.y) < Tags->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Tags->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Tags->Choice();
				SpacePressed();
				if (pos < Tags->Size()-1)
					Tags->Scroll(NC::wUp);
			}
		}
		else
			Screen<NC::Window>::MouseButtonPressed(me);
		Albums->Clear();
		Songs->Clear();
	}
	else if (!Albums->Empty() && Albums->hasCoords(me.x, me.y))
	{
		if (w != Albums)
			w == Tags ? NextColumn() : PrevColumn();
		if (size_t(me.y) < Albums->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Albums->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Albums->Choice();
				SpacePressed();
				if (pos < Albums->Size()-1)
					Albums->Scroll(NC::wUp);
			}
		}
		else
			Screen<NC::Window>::MouseButtonPressed(me);
		Songs->Clear();
	}
	else if (!Songs->Empty() && Songs->hasCoords(me.x, me.y))
	{
		if (w != Songs)
		{
			NextColumn();
			NextColumn();
		}
		if (size_t(me.y) < Songs->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Songs->Goto(me.y);
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = Songs->Choice();
				SpacePressed();
				if (pos < Songs->Size()-1)
					Songs->Scroll(NC::wUp);
			}
			else
				EnterPressed();
		}
		else
			Screen<NC::Window>::MouseButtonPressed(me);
	}
}

/***********************************************************************/

std::string MediaLibrary::currentFilter()
{
	std::string filter;
	if (w == Tags)
		filter = RegexFilter<std::string>::currentFilter(*Tags);
	else if (w == Albums)
		filter = RegexItemFilter<SearchConstraints>::currentFilter(*Albums);
	else if (w == Songs)
		filter = RegexFilter<MPD::Song>::currentFilter(*Songs);
	return filter;
}

void MediaLibrary::applyFilter(const std::string &filter)
{
	if (w == Tags)
	{
		Tags->ShowAll();
		auto rx = RegexFilter<std::string>(filter, Config.regex_type, TagEntryMatcher);
		Tags->filter(Tags->Begin(), Tags->End(), rx);
	}
	else if (w == Albums)
	{
		Albums->ShowAll();
		auto fun = std::bind(AlbumEntryMatcher, _1, _2, true);
		auto rx = RegexItemFilter<SearchConstraints>(filter, Config.regex_type, fun);
		Albums->filter(Albums->Begin(), Albums->End(), rx);
	}
	else if (w == Songs)
	{
		Songs->ShowAll();
		auto rx = RegexFilter<MPD::Song>(filter, Config.regex_type, SongEntryMatcher);
		Songs->filter(Songs->Begin(), Songs->End(), rx);
	}
}

/***********************************************************************/

bool MediaLibrary::search(const std::string &constraint)
{
	bool result = false;
	if (w == Tags)
	{
		auto rx = RegexFilter<std::string>(constraint, Config.regex_type, TagEntryMatcher);
		result = Tags->search(Tags->Begin(), Tags->End(), rx);
	}
	else if (w == Albums)
	{
		auto fun = std::bind(AlbumEntryMatcher, _1, _2, false);
		auto rx = RegexItemFilter<SearchConstraints>(constraint, Config.regex_type, fun);
		result = Albums->search(Albums->Begin(), Albums->End(), rx);
	}
	else if (w == Songs)
	{
		auto rx = RegexFilter<MPD::Song>(constraint, Config.regex_type, SongEntryMatcher);
		result = Songs->search(Songs->Begin(), Songs->End(), rx);
	}
	return result;
}

void MediaLibrary::nextFound(bool wrap)
{
	if (w == Tags)
		Tags->NextFound(wrap);
	else if (w == Albums)
		Albums->NextFound(wrap);
	else if (w == Songs)
		Songs->NextFound(wrap);
}

void MediaLibrary::prevFound(bool wrap)
{
	if (w == Tags)
		Tags->PrevFound(wrap);
	else if (w == Albums)
		Albums->PrevFound(wrap);
	else if (w == Songs)
		Songs->PrevFound(wrap);
}

/***********************************************************************/

std::shared_ptr<ProxySongList> MediaLibrary::getProxySongList()
{
	auto ptr = nullProxySongList();
	if (w == Songs)
		ptr = mkProxySongList(*Songs, [](NC::Menu<MPD::Song>::Item &item) {
			return &item.value();
		});
	return ptr;
}

bool MediaLibrary::allowsSelection()
{
	return true;
}

void MediaLibrary::reverseSelection()
{
	if (w == Tags)
		reverseSelectionHelper(Tags->Begin(), Tags->End());
	else if (w == Albums)
	{
		// omit "All tracks"
		if (Albums->Size() > 1)
			reverseSelectionHelper(Albums->Begin(), Albums->End()-2);
		else
			reverseSelectionHelper(Albums->Begin(), Albums->End());
	}
	else if (w == Songs)
		reverseSelectionHelper(Songs->Begin(), Songs->End());
}

MPD::SongList MediaLibrary::getSelectedSongs()
{
	MPD::SongList result;
	if (w == Tags)
	{
		auto tag_handler = [&result](const std::string &tag) {
			Mpd.StartSearch(true);
			Mpd.AddSearch(Config.media_lib_primary_tag, tag);
			auto songs = Mpd.CommitSearchSongs();
			std::sort(songs.begin(), songs.end(), SortAllTracks);
			result.insert(result.end(), songs.begin(), songs.end());
		};
		for (auto it = Tags->Begin(); it != Tags->End(); ++it)
			if (it->isSelected())
				tag_handler(it->value());
		// if no item is selected, add current one
		if (result.empty() && !Tags->Empty())
			tag_handler(Tags->Current().value());
	}
	else if (w == Albums)
	{
		for (auto it = Albums->Begin(); it != Albums->End() && !it->isSeparator(); ++it)
		{
			if (it->isSelected())
			{
				auto &sc = it->value();
				Mpd.StartSearch(true);
				if (hasTwoColumns)
					Mpd.AddSearch(Config.media_lib_primary_tag, sc.PrimaryTag);
				else
					Mpd.AddSearch(Config.media_lib_primary_tag, Tags->Current().value());
				Mpd.AddSearch(MPD_TAG_ALBUM, sc.Album);
				Mpd.AddSearch(MPD_TAG_DATE, sc.Date);
				auto songs = Mpd.CommitSearchSongs();
				std::sort(songs.begin(), songs.end(), SortSongsByTrack);
				result.insert(result.end(), songs.begin(), songs.end());
			}
		}
		// if no item is selected, add songs from right column
		if (result.empty() && !Albums->Empty())
		{
			withUnfilteredMenu(*Songs, [this, &result]() {
				result.insert(result.end(), Songs->BeginV(), Songs->EndV());
			});
		}
	}
	else if (w == Songs)
	{
		for (auto it = Songs->Begin(); it != Songs->End(); ++it)
			if (it->isSelected())
				result.push_back(it->value());
		// if no item is selected, add current one
		if (result.empty() && !Songs->Empty())
			result.push_back(Songs->Current().value());
	}
	return result;
}

/***********************************************************************/

int MediaLibrary::Columns()
{
	if (hasTwoColumns)
		return 2;
	else
		return 3;
}

bool MediaLibrary::isNextColumnAvailable()
{
	assert(!hasTwoColumns || w != Tags);
	if (w == Tags)
	{
		if (!Albums->ReallyEmpty() && !Songs->ReallyEmpty())
			return true;
	}
	else if (w == Albums)
	{
		if (!Songs->ReallyEmpty())
			return true;
	}
	return false;
}

void MediaLibrary::NextColumn()
{
	if (w == Tags)
	{
		Tags->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Albums;
		Albums->HighlightColor(Config.active_column_color);
	}
	else if (w == Albums)
	{
		Albums->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Songs;
		Songs->HighlightColor(Config.active_column_color);
	}
}

bool MediaLibrary::isPrevColumnAvailable()
{
	assert(!hasTwoColumns || w != Tags);
	if (w == Songs)
	{
		if (!Albums->ReallyEmpty() && (hasTwoColumns || !Tags->ReallyEmpty()))
			return true;
	}
	else if (w == Albums)
	{
		if (!hasTwoColumns && !Tags->ReallyEmpty())
			return true;
	}
	return false;
}

void MediaLibrary::PrevColumn()
{
	if (w == Songs)
	{
		Songs->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Albums;
		Albums->HighlightColor(Config.active_column_color);
	}
	else if (w == Albums && !hasTwoColumns)
	{
		Albums->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Tags;
		Tags->HighlightColor(Config.active_column_color);
	}
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
		std::string item_type = tagTypeToString(Config.media_lib_primary_tag);
		lowercase(item_type);
		ShowMessage("Can't use this function because the song has no %s tag set", item_type.c_str());
		return;
	}
	
	if (myScreen != this)
		SwitchTo();
	Statusbar() << "Jumping to song...";
	Global::wFooter->Refresh();
	
	if (!hasTwoColumns)
	{
		Tags->ShowAll();
		if (Tags->Empty())
			Update();
		if (primary_tag != Tags->Current().value())
		{
			for (size_t i = 0; i < Tags->Size(); ++i)
			{
				if (primary_tag == (*Tags)[i].value())
				{
					Tags->Highlight(i);
					Albums->Clear();
					Songs->Clear();
					break;
				}
			}
		}
	}
	
	Albums->ShowAll();
	if (Albums->Empty())
		Update();
	
	std::string album = s.getAlbum();
	std::string date = s.getDate();
	if ((hasTwoColumns && Albums->Current().value().PrimaryTag != primary_tag)
	||  album != Albums->Current().value().Album
	||  date != Albums->Current().value().Date)
	{
		for (size_t i = 0; i < Albums->Size(); ++i)
		{
			if ((!hasTwoColumns || (*Albums)[i].value().PrimaryTag == primary_tag)
			&&   album == (*Albums)[i].value().Album
			&&   date == (*Albums)[i].value().Date)
			{
				Albums->Highlight(i);
				Songs->Clear();
				break;
			}
		}
	}
	
	Songs->ShowAll();
	if (Songs->Empty())
		Update();
	
	if (s.getHash() != Songs->Current().value().getHash())
	{
		for (size_t i = 0; i < Songs->Size(); ++i)
		{
			if (s.getHash()  == (*Songs)[i].value().getHash())
			{
				Songs->Highlight(i);
				break;
			}
		}
	}
	
	Tags->HighlightColor(Config.main_highlight_color);
	Albums->HighlightColor(Config.main_highlight_color);
	Songs->HighlightColor(Config.active_column_color);
	w = Songs;
	Refresh();
}

void MediaLibrary::AddToPlaylist(bool add_n_play)
{
	if (w == Songs && !Songs->Empty())
	{
		bool res = myPlaylist->Add(Songs->Current().value(), Songs->Current().isBold(), add_n_play);
		Songs->Current().setBold(res);
	}
	else
	{
		auto list = getSelectedSongs();
		if (myPlaylist->Add(list, add_n_play))
		{
			if ((!Tags->Empty() && w == Tags)
			||  (w == Albums && Albums->Current().value().Date == AllTracksMarker))
			{
				std::string tag_type = tagTypeToString(Config.media_lib_primary_tag);
				lowercase(tag_type);
				ShowMessage("Songs with %s = \"%s\" added", tag_type.c_str(), Tags->Current().value().c_str());
			}
			else if (w == Albums)
				ShowMessage("Songs from album \"%s\" added", Albums->Current().value().Album.c_str());
		}
	}

	if (!add_n_play)
	{
		w->Scroll(NC::wDown);
		if (w == Tags)
		{
			Albums->Clear();
			Songs->Clear();
		}
		else if (w == Albums)
			Songs->Clear();
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
	return s.toString(Config.song_library_format);
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
	menu << AlbumToString(menu.Drawn().value());
}

void DisplayPrimaryTags(NC::Menu<std::string> &menu)
{
	const std::string &tag = menu.Drawn().value();
	if (tag.empty())
		menu << Config.empty_tag;
	else
		menu << tag;
}

/***********************************************************************/

bool SortSongsByTrack(const MPD::Song &a, const MPD::Song &b)
{
	int cmp = a.getDisc().compare(a.getDisc());
	if (cmp != 0)
		return cmp;
	return a.getTrack() < b.getTrack();
}

bool SortAllTracks(const MPD::Song &a, const MPD::Song &b)
{
	const std::array<MPD::Song::GetFunction, 3> gets = {{
		&MPD::Song::getDate,
		&MPD::Song::getAlbum,
		&MPD::Song::getDisc
	}};
	CaseInsensitiveStringComparison cmp;
	for (auto get = gets.begin(); get != gets.end(); ++get)
	{
		int ret = cmp(a.getTags(*get), b.getTags(*get));
		if (ret != 0)
			return ret < 0;
	}
	return a.getTrack() < b.getTrack();
}

bool SortSearchConstraints(const SearchConstraints &a, const SearchConstraints &b)
{
	int result;
	CaseInsensitiveStringComparison cmp;
	result = cmp(a.PrimaryTag, b.PrimaryTag);
	if (result != 0)
		return result < 0;
	result = cmp(a.Date, b.Date);
	if (result != 0)
		return result < 0;
	return cmp(a.Album, b.Album) < 0;
}

}
