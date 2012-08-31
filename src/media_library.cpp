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
#include <cassert>

#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "global.h"
#include "media_library.h"
#include "mpdpp.h"
#include "playlist.h"
#include "status.h"
#include "utility/comparators.h"
#include "utility/type_conversions.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

MediaLibrary *myLibrary = new MediaLibrary;

bool MediaLibrary::hasTwoColumns;
size_t MediaLibrary::itsLeftColStartX;
size_t MediaLibrary::itsLeftColWidth;
size_t MediaLibrary::itsMiddleColWidth;
size_t MediaLibrary::itsMiddleColStartX;
size_t MediaLibrary::itsRightColWidth;
size_t MediaLibrary::itsRightColStartX;

// this string marks the position in middle column that works as "All tracks" option. it's
// assigned to Date in SearchConstraint class since date normally cannot contain other chars
// than ciphers and -'s (0x7f is interpreted as backspace keycode, so it's quite safe to assume
// that it won't appear in any tag, let alone date).
const char MediaLibrary::AllTracksMarker[] = "\x7f";

void MediaLibrary::Init()
{
	hasTwoColumns = 0;
	itsLeftColWidth = COLS/3-1;
	itsMiddleColWidth = COLS/3;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsRightColWidth = COLS-COLS/3*2-1;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	
	Artists = new Menu<std::string>(0, MainStartY, itsLeftColWidth, MainHeight, Config.titles_visibility ? tagTypeToString(Config.media_lib_primary_tag) + "s" : "", Config.main_color, brNone);
	Artists->HighlightColor(Config.active_column_color);
	Artists->CyclicScrolling(Config.use_cyclic_scrolling);
	Artists->CenteredCursor(Config.centered_cursor);
	Artists->SetSelectPrefix(Config.selected_item_prefix);
	Artists->SetSelectSuffix(Config.selected_item_suffix);
	Artists->setItemDisplayer(DisplayPrimaryTags);
	
	Albums = new Menu<SearchConstraints>(itsMiddleColStartX, MainStartY, itsMiddleColWidth, MainHeight, Config.titles_visibility ? "Albums" : "", Config.main_color, brNone);
	Albums->HighlightColor(Config.main_highlight_color);
	Albums->CyclicScrolling(Config.use_cyclic_scrolling);
	Albums->CenteredCursor(Config.centered_cursor);
	Albums->SetSelectPrefix(Config.selected_item_prefix);
	Albums->SetSelectSuffix(Config.selected_item_suffix);
	Albums->setItemDisplayer(DisplayAlbums);
	Albums->SetItemStringifier(AlbumToString);
	
	Songs = new Menu<MPD::Song>(itsRightColStartX, MainStartY, itsRightColWidth, MainHeight, Config.titles_visibility ? "Songs" : "", Config.main_color, brNone);
	Songs->HighlightColor(Config.main_highlight_color);
	Songs->CyclicScrolling(Config.use_cyclic_scrolling);
	Songs->CenteredCursor(Config.centered_cursor);
	Songs->SetSelectPrefix(Config.selected_item_prefix);
	Songs->SetSelectSuffix(Config.selected_item_suffix);
	Songs->setItemDisplayer(std::bind(Display::Songs, _1, *this, Config.song_library_format));
	Songs->SetItemStringifier(SongToString);
	
	w = Artists;
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
	
	Artists->Resize(itsLeftColWidth, MainHeight);
	Albums->Resize(itsMiddleColWidth, MainHeight);
	Songs->Resize(itsRightColWidth, MainHeight);
	
	Artists->MoveTo(itsLeftColStartX, MainStartY);
	Albums->MoveTo(itsMiddleColStartX, MainStartY);
	Songs->MoveTo(itsRightColStartX, MainStartY);
	
	hasToBeResized = 0;
}

void MediaLibrary::Refresh()
{
	Artists->Display();
	mvvline(MainStartY, itsMiddleColStartX-1, 0, MainHeight);
	Albums->Display();
	mvvline(MainStartY, itsRightColStartX-1, 0, MainHeight);
	Songs->Display();
	if (Albums->Empty())
	{
		*Albums << XY(0, 0) << "No albums found.";
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
			Artists->Clear();
			Albums->Clear();
			Albums->Reset();
			Songs->Clear();
			if (hasTwoColumns)
			{
				if (w == Artists)
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
	if (!hasTwoColumns && Artists->ReallyEmpty())
	{
		MPD::TagList list;
		Albums->Clear();
		Songs->Clear();
		Mpd.GetList(list, Config.media_lib_primary_tag);
		sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (MPD::TagList::iterator it = list.begin(); it != list.end(); ++it)
		{
			if (it->empty() && !Config.media_library_display_empty_tag)
				continue;
			utf_to_locale(*it);
			Artists->AddItem(*it);
		}
		Artists->Window::Clear();
		Artists->Refresh();
	}
	
	if (!hasTwoColumns && !Artists->Empty() && Albums->ReallyEmpty() && Songs->ReallyEmpty())
	{
		// idle has to be blocked for now since it would be enabled and
		// disabled a few times by each mpd command, which makes no sense
		// and slows down the whole process.
		Mpd.BlockIdle(1);
		Albums->Reset();
		MPD::TagList list;
		locale_to_utf(Artists->Current().value());
		Mpd.StartFieldSearch(MPD_TAG_ALBUM);
		Mpd.AddSearch(Config.media_lib_primary_tag, Artists->Current().value());
		Mpd.CommitSearchTags([&list](std::string &&album) {
			list.push_back(album);
		});
		for (auto album = list.begin(); album != list.end(); ++album)
		{
			if (Config.media_library_display_date)
			{
				Mpd.StartFieldSearch(MPD_TAG_DATE);
				Mpd.AddSearch(Config.media_lib_primary_tag, Artists->Current().value());
				Mpd.AddSearch(MPD_TAG_ALBUM, *album);
				utf_to_locale(*album);
				Mpd.CommitSearchTags([this, &album](std::string &&date) {
					utf_to_locale(date);
					Albums->AddItem(SearchConstraints(*album, date));
				});
			}
			else
			{
				utf_to_locale(*album);
				Albums->AddItem(SearchConstraints(*album, ""));
			}
		}
		utf_to_locale(Artists->Current().value());
		if (!Albums->Empty())
			std::sort(Albums->BeginV(), Albums->EndV(), SearchConstraintsSorting());
		if (Albums->Size() > 1)
		{
			Albums->AddSeparator();
			Albums->AddItem(SearchConstraints("", AllTracksMarker));
		}
		Albums->Refresh();
		Mpd.BlockIdle(0);
	}
	else if (hasTwoColumns && Albums->ReallyEmpty())
	{
		Songs->Clear();
		MPD::TagList artists;
		*Albums << XY(0, 0) << "Fetching albums...";
		Albums->Window::Refresh();
		Mpd.BlockIdle(1);
		Mpd.GetList(artists, Config.media_lib_primary_tag);
		for (auto artist = artists.begin(); artist != artists.end(); ++artist)
		{
			MPD::TagList albums;
			Mpd.StartFieldSearch(MPD_TAG_ALBUM);
			Mpd.AddSearch(Config.media_lib_primary_tag, *artist);
			Mpd.CommitSearchTags([&albums](std::string &&album) {
				albums.push_back(album);
			});
			for (auto album = albums.begin(); album != albums.end(); ++album)
			{
				if (Config.media_library_display_date)
				{
					if (Config.media_lib_primary_tag != MPD_TAG_DATE)
					{
						Mpd.StartFieldSearch(MPD_TAG_DATE);
						Mpd.AddSearch(Config.media_lib_primary_tag, *artist);
						Mpd.AddSearch(MPD_TAG_ALBUM, *album);
						utf_to_locale(*artist);
						utf_to_locale(*album);
						Mpd.CommitSearchTags([this, &artist, &album](std::string &&tag) {
							utf_to_locale(tag);
							Albums->AddItem(SearchConstraints(*artist, *album, tag));
						});
					}
					else
					{
						utf_to_locale(*artist);
						utf_to_locale(*album);
						Albums->AddItem(SearchConstraints(*artist, *album, *artist));
					}
				}
				else
				{
					utf_to_locale(*artist);
					utf_to_locale(*album);
					Albums->AddItem(SearchConstraints(*artist, *album, ""));
				}
			}
		}
		Mpd.BlockIdle(0);
		if (!Albums->Empty())
			std::sort(Albums->BeginV(), Albums->EndV(), SearchConstraintsSorting());
		Albums->Refresh();
	}
	
	if (!hasTwoColumns && !Artists->Empty() && w == Albums && Albums->ReallyEmpty())
	{
		Albums->HighlightColor(Config.main_highlight_color);
		Artists->HighlightColor(Config.active_column_color);
		w = Artists;
	}
	
	if (!(hasTwoColumns ? Albums->Empty() : Artists->Empty()) && Songs->ReallyEmpty())
	{
		Songs->Reset();
		
		Mpd.StartSearch(1);
		Mpd.AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(hasTwoColumns ? Albums->Current().value().PrimaryTag : Artists->Current().value()));
		if (Albums->Current().value().Date != AllTracksMarker)
		{
			Mpd.AddSearch(MPD_TAG_ALBUM, locale_to_utf_cpy(Albums->Current().value().Album));
			if (Config.media_library_display_date)
				Mpd.AddSearch(MPD_TAG_DATE, locale_to_utf_cpy(Albums->Current().value().Date));
		}
		Mpd.CommitSearchSongs([this](MPD::Song &&s) {
			Songs->AddItem(s, myPlaylist->checkForSong(s));
		});
		
		if (Albums->Current().value().Date == AllTracksMarker)
			std::sort(Songs->BeginV(), Songs->EndV(), SortAllTracks);
		else
			std::sort(Songs->BeginV(), Songs->EndV(), SortSongsByTrack);
		
		Songs->Window::Clear();
		Songs->Refresh();
	}
}

void MediaLibrary::SpacePressed()
{
	if (Config.space_selects)
	{
		if (w == Artists)
		{
			size_t i = Artists->Choice();
			Artists->at(i).setSelected(!Artists->at(i).isSelected());
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
		w->Scroll(wDown);
	}
	else
		AddToPlaylist(0);
}

void MediaLibrary::MouseButtonPressed(MEVENT me)
{
	if (!Artists->Empty() && Artists->hasCoords(me.x, me.y))
	{
		if (w != Artists)
		{
			PrevColumn();
			PrevColumn();
		}
		if (size_t(me.y) < Artists->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Artists->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Artists->Choice();
				SpacePressed();
				if (pos < Artists->Size()-1)
					Artists->Scroll(wUp);
			}
		}
		else
			Screen<Window>::MouseButtonPressed(me);
		Albums->Clear();
		Songs->Clear();
	}
	else if (!Albums->Empty() && Albums->hasCoords(me.x, me.y))
	{
		if (w != Albums)
			w == Artists ? NextColumn() : PrevColumn();
		if (size_t(me.y) < Albums->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Albums->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Albums->Choice();
				SpacePressed();
				if (pos < Albums->Size()-1)
					Albums->Scroll(wUp);
			}
		}
		else
			Screen<Window>::MouseButtonPressed(me);
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
					Songs->Scroll(wUp);
			}
			else
				EnterPressed();
		}
		else
			Screen<Window>::MouseButtonPressed(me);
	}
}

MPD::Song *MediaLibrary::CurrentSong()
{
	return w == Songs && !Songs->Empty() ? &Songs->Current().value() : 0;
}

List *MediaLibrary::GetList()
{
	if (w == Artists)
		return Artists;
	else if (w == Albums)
		return Albums;
	else if (w == Songs)
		return Songs;
	else // silence compiler
		return 0;
}

void MediaLibrary::ReverseSelection()
{
	if (w == Artists)
		Artists->ReverseSelection();
	else if (w == Albums)
		Albums->ReverseSelection();
	else if (w == Songs)
		Songs->ReverseSelection();
}

void MediaLibrary::GetSelectedSongs(MPD::SongList &v)
{
	std::vector<size_t> selected;
	if (w == Artists && !Artists->Empty())
	{
		Artists->GetSelected(selected);
		if (selected.empty())
			selected.push_back(Artists->Choice());
		for (auto it = selected.begin(); it != selected.end(); ++it)
		{
			MPD::SongList list;
			Mpd.StartSearch(1);
			Mpd.AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(Artists->at(*it).value()));
			Mpd.CommitSearchSongs([&list](MPD::Song &&s) {
				list.push_back(s);
			});
			std::sort(list.begin(), list.end(), SortAllTracks);
			std::copy(list.begin(), list.end(), std::back_inserter(v));
		}
	}
	else if (w == Albums && !Albums->Empty())
	{
		Albums->GetSelected(selected);
		if (selected.empty())
		{
			// shortcut via the existing song list in right column
			if (v.empty())
				v.reserve(Songs->Size());
			for (size_t i = 0; i < Songs->Size(); ++i)
				v.push_back((*Songs)[i].value());
		}
		else
		{
			for (auto it = selected.begin(); it != selected.end(); ++it)
			{
				Mpd.StartSearch(1);
				Mpd.AddSearch(Config.media_lib_primary_tag, hasTwoColumns
						?  Albums->at(*it).value().PrimaryTag
						: locale_to_utf_cpy(Artists->Current().value()));
				Mpd.AddSearch(MPD_TAG_ALBUM, Albums->at(*it).value().Album);
				Mpd.AddSearch(MPD_TAG_DATE, Albums->at(*it).value().Date);
				Mpd.CommitSearchSongs([&v](MPD::Song &&s) {
					v.push_back(s);
				});
			}
		}
	}
	else if (w == Songs && !Songs->Empty())
	{
		Songs->GetSelected(selected);
		if (selected.empty())
			selected.push_back(Songs->Choice());
		for (auto it = selected.begin(); it != selected.end(); ++it)
			v.push_back(Songs->at(*it).value());
	}
}

std::string MediaLibrary::currentFilter()
{
	std::string filter;
	if (w == Artists)
		filter = RegexFilter<std::string>::currentFilter(*Artists);
	else if (w == Albums)
		filter = RegexFilter<SearchConstraints>::currentFilter(*Albums);
	else if (w == Songs)
		filter = RegexFilter<MPD::Song>::currentFilter(*Songs);
	return filter;
}

void MediaLibrary::applyFilter(const std::string &filter)
{
	if (w == Artists)
	{
		Artists->ShowAll();
		auto rx = RegexFilter<std::string>(filter, Config.regex_type);
		Artists->Filter(Artists->Begin(), Artists->End(), rx);
	}
	else if (w == Albums)
	{
		Albums->ShowAll();
		auto fun = [](const Regex &rx, Menu<SearchConstraints> &menu, const Menu<SearchConstraints>::Item &item) {
			if (item.isSeparator() || item.value().Date == AllTracksMarker)
				return true;
			return rx.match(menu.Stringify(item));
		};
		auto rx = RegexFilter<SearchConstraints>(filter, Config.regex_type, fun);
		Albums->Filter(Albums->Begin(), Albums->End(), rx);
	}
	else if (w == Songs)
	{
		Songs->ShowAll();
		auto rx = RegexFilter<MPD::Song>(filter, Config.regex_type);
		Songs->Filter(Songs->Begin(), Songs->End(), rx);
	}
}

bool MediaLibrary::isNextColumnAvailable()
{
	assert(!hasTwoColumns || w != Artists);
	if (w == Artists)
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
	if (w == Artists)
	{
		Artists->HighlightColor(Config.main_highlight_color);
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
	assert(!hasTwoColumns || w != Artists);
	if (w == Songs)
	{
		if (!Albums->ReallyEmpty() && (hasTwoColumns || !Artists->ReallyEmpty()))
			return true;
	}
	else if (w == Albums)
	{
		if (!hasTwoColumns && !Artists->ReallyEmpty())
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
		w = Artists;
		Artists->HighlightColor(Config.active_column_color);
	}
}

void MediaLibrary::LocateSong(const MPD::Song &s)
{
	if (Mpd.Version() < 14)
	{
		// <mpd-0.14.* has no ability to search for empty tags, which sometimes
		// leaves albums column empty. since this function relies on this column
		// being non-empty, it has to be disabled for these versions.
		ShowMessage("This function is supported in MPD >= 0.14.0");
		return;
	}
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
		Artists->ShowAll();
		if (Artists->Empty())
			Update();
		if (primary_tag != Artists->Current().value())
		{
			for (size_t i = 0; i < Artists->Size(); ++i)
			{
				if (primary_tag == (*Artists)[i].value())
				{
					Artists->Highlight(i);
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
	
	Artists->HighlightColor(Config.main_highlight_color);
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
		MPD::SongList list;
		GetSelectedSongs(list);

		if (myPlaylist->Add(list, add_n_play))
		{
			if ((!Artists->Empty() && w == Artists)
			||  (w == Albums && Albums->Current().value().Date == AllTracksMarker))
			{
				std::string tag_type = tagTypeToString(Config.media_lib_primary_tag);
				lowercase(tag_type);
				ShowMessage("Songs with %s = \"%s\" added", tag_type.c_str(), Artists->Current().value().c_str());
			}
			else if (w == Albums)
				ShowMessage("Songs from album \"%s\" added", Albums->Current().value().Album.c_str());
		}
	}

	if (!add_n_play)
	{
		w->Scroll(wDown);
		if (w == Artists)
		{
			Albums->Clear();
			Songs->Clear();
		}
		else if (w == Albums)
			Songs->Clear();
	}
}

std::string MediaLibrary::SongToString(const MPD::Song &s)
{
	return s.toString(Config.song_library_format);
}

std::string MediaLibrary::AlbumToString(const SearchConstraints &sc)
{
	if (sc.Date == AllTracksMarker)
		return "All tracks";
	std::string result;
	if (myLibrary->hasTwoColumns)
		(result += sc.PrimaryTag.empty() ? Config.empty_tag : sc.PrimaryTag) += " - ";
	if ((!myLibrary || Config.media_lib_primary_tag != MPD_TAG_DATE) && !sc.Date.empty())
		((result += "(") += sc.Date) += ") ";
	result += sc.Album.empty() ? "<no album>" : sc.Album;
	return result;
}

void MediaLibrary::DisplayAlbums(Menu<SearchConstraints> &menu)
{
	menu << AlbumToString(menu.Drawn().value());
}

void MediaLibrary::DisplayPrimaryTags(Menu<std::string> &menu)
{
	const std::string &tag = menu.Drawn().value();
	if (tag.empty())
		menu << Config.empty_tag;
	else
		menu << tag;
}

bool MediaLibrary::SearchConstraintsSorting::operator()(const SearchConstraints &a, const SearchConstraints &b) const
{
	int result;
	CaseInsensitiveStringComparison cmp;
	result = cmp(a.PrimaryTag, b.PrimaryTag);
	if (result != 0)
		return result < 0;
	result = cmp(a.Date, b.Date);
	return (result == 0 ? cmp(a.Album, b.Album) : result) < 0;
}

bool MediaLibrary::SortSongsByTrack(const MPD::Song &a, const MPD::Song &b)
{
	if (a.getDisc() == b.getDisc())
		return stringToInt(a.getTrack()) < stringToInt(b.getTrack());
	else
		return stringToInt(a.getDisc()) < stringToInt(b.getDisc());
}

bool MediaLibrary::SortAllTracks(const MPD::Song &a, const MPD::Song &b)
{
	static MPD::Song::GetFunction gets[] = { &MPD::Song::getDate, &MPD::Song::getAlbum, &MPD::Song::getDisc, 0 };
	CaseInsensitiveStringComparison cmp;
	for (MPD::Song::GetFunction *get = gets; *get; ++get)
		if (int ret = cmp(a.getTags(*get), b.getTags(*get)))
			return ret < 0;
	return a.getTrack() < b.getTrack();
}

