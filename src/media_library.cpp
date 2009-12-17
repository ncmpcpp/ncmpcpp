/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "global.h"
#include "media_library.h"
#include "mpdpp.h"
#include "playlist.h"
#include "status.h"

using namespace MPD;
using namespace Global;

MediaLibrary *myLibrary = new MediaLibrary;

bool MediaLibrary::hasTwoColumns;
size_t MediaLibrary::itsLeftColWidth;
size_t MediaLibrary::itsMiddleColWidth;
size_t MediaLibrary::itsMiddleColStartX;
size_t MediaLibrary::itsRightColWidth;
size_t MediaLibrary::itsRightColStartX;

void MediaLibrary::Init()
{
	hasTwoColumns = 0;
	itsLeftColWidth = COLS/3-1;
	itsMiddleColWidth = COLS/3;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsRightColWidth = COLS-COLS/3*2-1;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	
	Artists = new Menu<std::string>(0, MainStartY, itsLeftColWidth, MainHeight, IntoStr(Config.media_lib_primary_tag) + "s", Config.main_color, brNone);
	Artists->HighlightColor(Config.active_column_color);
	Artists->CyclicScrolling(Config.use_cyclic_scrolling);
	Artists->SetSelectPrefix(&Config.selected_item_prefix);
	Artists->SetSelectSuffix(&Config.selected_item_suffix);
	Artists->SetItemDisplayer(Display::Generic);
	
	Albums = new Menu< std::pair<std::string, SearchConstraints> >(itsMiddleColStartX, MainStartY, itsMiddleColWidth, MainHeight, "Albums", Config.main_color, brNone);
	Albums->HighlightColor(Config.main_highlight_color);
	Albums->CyclicScrolling(Config.use_cyclic_scrolling);
	Albums->SetSelectPrefix(&Config.selected_item_prefix);
	Albums->SetSelectSuffix(&Config.selected_item_suffix);
	Albums->SetItemDisplayer(Display::Pairs);
	Albums->SetGetStringFunction(StringPairToString);
	
	Songs = new Menu<Song>(itsRightColStartX, MainStartY, itsRightColWidth, MainHeight, "Songs", Config.main_color, brNone);
	Songs->HighlightColor(Config.main_highlight_color);
	Songs->CyclicScrolling(Config.use_cyclic_scrolling);
	Songs->SetSelectPrefix(&Config.selected_item_prefix);
	Songs->SetSelectSuffix(&Config.selected_item_suffix);
	Songs->SetItemDisplayer(Display::Songs);
	Songs->SetItemDisplayerUserData(&Config.song_library_format);
	Songs->SetGetStringFunction(SongToString);
	
	w = Artists;
	isInitialized = 1;
}

void MediaLibrary::Resize()
{
	if (!hasTwoColumns)
	{
		itsLeftColWidth = COLS/3-1;
		itsMiddleColStartX = itsLeftColWidth+1;
		itsMiddleColWidth = COLS/3;
		itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
		itsRightColWidth = COLS-COLS/3*2-1;
	}
	else
	{
		itsMiddleColStartX = 0;
		itsMiddleColWidth = COLS/2;
		itsRightColStartX = itsMiddleColWidth+1;
		itsRightColWidth = COLS-itsMiddleColWidth-1;
	}
	
	Artists->Resize(itsLeftColWidth, MainHeight);
	Albums->Resize(itsMiddleColWidth, MainHeight);
	Songs->Resize(itsRightColWidth, MainHeight);
	
	Artists->MoveTo(0, MainStartY);
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
	if (myScreen == this)
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
			std::string item_type = IntoStr(Config.media_lib_primary_tag);
			ToLower(item_type);
			Albums->SetTitle("Albums (sorted by " + item_type + ")");
		}
		else
			Albums->SetTitle("Albums");
	}
	
	if (!isInitialized)
		Init();
	
	if (hasToBeResized)
		Resize();
	
	if (myScreen != this && myScreen->isTabbable())
		myPrevScreen = myScreen;
	myScreen = this;
	RedrawHeader = 1;
	Refresh();
	UpdateSongList(Songs);
}

std::basic_string<my_char_t> MediaLibrary::Title()
{
	return U("Media library");
}

void MediaLibrary::Update()
{
	if (!hasTwoColumns && Artists->Empty())
	{
		TagList list;
		Albums->Clear();
		Mpd.GetList(list, Config.media_lib_primary_tag);
		sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (TagList::iterator it = list.begin(); it != list.end(); ++it)
		{
			if (!it->empty())
			{
				utf_to_locale(*it);
				Artists->AddOption(*it);
			}
		}
		Artists->Window::Clear();
		Artists->Refresh();
	}
	
	if (!hasTwoColumns && !Artists->Empty() && Albums->Empty())
	{
		Songs->Clear();
		Albums->Reset();
		TagList list;
		locale_to_utf(Artists->Current());
		if (Config.media_lib_primary_tag == MPD_TAG_ARTIST)
			Mpd.GetAlbums(Artists->Current(), list);
		else
		{
			Mpd.StartFieldSearch(MPD_TAG_ALBUM);
			Mpd.AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd.CommitSearch(list);
		}
		
		// <mpd-0.14 doesn't support searching for empty tag
		if (Mpd.Version() > 13)
		{
			SongList noalbum_list;
			Mpd.StartSearch(1);
			Mpd.AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd.AddSearch(MPD_TAG_ALBUM, "");
			Mpd.CommitSearch(noalbum_list);
			if (!noalbum_list.empty())
				Albums->AddOption(std::make_pair("<no album>", SearchConstraints("", "")));
			FreeSongList(noalbum_list);
		}
		
		for (TagList::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			TagList l;
			Mpd.StartFieldSearch(MPD_TAG_DATE);
			Mpd.AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd.AddSearch(MPD_TAG_ALBUM, *it);
			Mpd.CommitSearch(l);
			if (l.empty())
			{
				Albums->AddOption(std::make_pair(*it, SearchConstraints(*it, "")));
				continue;
			}
			for (TagList::const_iterator j = l.begin(); j != l.end(); ++j)
				Albums->AddOption(std::make_pair("(" + *j + ") " + *it, SearchConstraints(*it, *j)));
		}
		utf_to_locale(Artists->Current());
		for (size_t i = 0; i < Albums->Size(); ++i)
			utf_to_locale((*Albums)[i].first);
		if (!Albums->Empty())
			Albums->Sort<CaseInsensitiveSorting>((*Albums)[0].first == "<no album>");
		Albums->Refresh();
	}
	else if (hasTwoColumns && Albums->Empty())
	{
		Songs->Clear();
		TagList artists;
		*Albums << XY(0, 0) << "Fetching albums...";
		Albums->Window::Refresh();
		Mpd.GetList(artists, Config.media_lib_primary_tag);
		for (TagList::const_iterator i = artists.begin(); i != artists.end(); ++i)
		{
			TagList albums;
			Mpd.StartFieldSearch(MPD_TAG_ALBUM);
			Mpd.AddSearch(Config.media_lib_primary_tag, *i);
			Mpd.CommitSearch(albums);
			for (TagList::const_iterator j = albums.begin(); j != albums.end(); ++j)
			{
				if (Config.media_lib_primary_tag != MPD_TAG_DATE)
				{
					TagList years;
					Mpd.StartFieldSearch(MPD_TAG_DATE);
					Mpd.AddSearch(Config.media_lib_primary_tag, *i);
					Mpd.AddSearch(MPD_TAG_ALBUM, *j);
					Mpd.CommitSearch(years);
					if (!years.empty())
					{
						for (TagList::const_iterator k = years.begin(); k != years.end(); ++k)
						{
							Albums->AddOption(std::make_pair(*i + " - (" + *k + ") " + *j, SearchConstraints(*i, *j, *k)));
						}
					}
					else
						Albums->AddOption(std::make_pair(*i + " - " + *j, SearchConstraints(*i, *j, "")));
				}
				else
					Albums->AddOption(std::make_pair(*i + " - " + *j, SearchConstraints(*i, *j, *i)));
			}
		}
		for (size_t i = 0; i < Albums->Size(); ++i)
			utf_to_locale((*Albums)[i].first);
		if (!Albums->Empty())
			Albums->Sort<CaseInsensitiveSorting>();
		Albums->Refresh();
	}
	
	if (!hasTwoColumns && !Artists->Empty() && w == Albums && Albums->Empty())
	{
		Albums->HighlightColor(Config.main_highlight_color);
		Artists->HighlightColor(Config.active_column_color);
		w = Artists;
	}
	
	if ((hasTwoColumns || !Artists->Empty()) && Songs->Empty())
	{
		Songs->Reset();
		SongList list;
		
		Mpd.StartSearch(1);
		Mpd.AddSearch(Config.media_lib_primary_tag, hasTwoColumns ? Albums->Current().second.Artist : locale_to_utf_cpy(Artists->Current()));
		if (Albums->Empty()) // left for compatibility with <mpd-0.14
		{
			*Albums << XY(0, 0) << "No albums found.";
			Albums->Window::Refresh();
		}
		else
		{
			Mpd.AddSearch(MPD_TAG_ALBUM, Albums->Current().second.Album);
			if (!Albums->Current().second.Album.empty()) // for <no album>
				Mpd.AddSearch(MPD_TAG_DATE, Albums->Current().second.Year);
		}
		Mpd.CommitSearch(list);
		
		sort(list.begin(), list.end(), SortSongsByTrack);
		bool bold = 0;
		
		for (SongList::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			for (size_t j = 0; j < myPlaylist->Items->Size(); ++j)
			{
				if ((*it)->GetHash() == myPlaylist->Items->at(j).GetHash())
				{
					bold = 1;
					break;
				}
			}
			Songs->AddOption(**it, bold);
			bold = 0;
		}
		FreeSongList(list);
		Songs->Window::Clear();
		Songs->Refresh();
	}
}

void MediaLibrary::SpacePressed()
{
	if (Config.space_selects)
	{
		if (w == Artists)
			Artists->Select(Artists->Choice(), !Artists->isSelected());
		else if (w == Albums)
			Albums->Select(Albums->Choice(), !Albums->isSelected());
		else if (w == Songs)
			Songs->Select(Songs->Choice(), !Songs->isSelected());
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
	return w == Songs && !Songs->Empty() ? &Songs->Current() : 0;
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
		for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
		{
			MPD::SongList list;
			Mpd.StartSearch(1);
			Mpd.AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(Artists->at(*it)));
			Mpd.CommitSearch(list);
			for (MPD::SongList::const_iterator sIt = list.begin(); sIt != list.end(); ++sIt)
				v.push_back(new MPD::Song(**sIt));
			FreeSongList(list);
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
				v.push_back(new MPD::Song((*Songs)[i]));
		}
		else
		{
			for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
			{
				MPD::SongList list;
				Mpd.StartSearch(1);
				Mpd.AddSearch(Config.media_lib_primary_tag, hasTwoColumns
						?  Albums->at(*it).second.Artist
						: locale_to_utf_cpy(Artists->Current()));
				Mpd.AddSearch(MPD_TAG_ALBUM, Albums->at(*it).second.Album);
				if (!Albums->at(*it).second.Album.empty()) // for <no album>
					Mpd.AddSearch(MPD_TAG_DATE, Albums->at(*it).second.Year);
				Mpd.CommitSearch(list);
				for (MPD::SongList::const_iterator sIt = list.begin(); sIt != list.end(); ++sIt)
					v.push_back(new MPD::Song(**sIt));
				FreeSongList(list);
			}
		}
	}
	else if (w == Songs && !Songs->Empty())
	{
		Songs->GetSelected(selected);
		if (selected.empty())
			selected.push_back(Songs->Choice());
		for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
			v.push_back(new MPD::Song(Songs->at(*it)));
	}
}

void MediaLibrary::ApplyFilter(const std::string &s)
{
	GetList()->ApplyFilter(s, 0, REG_ICASE | Config.regex_type);
}

void MediaLibrary::NextColumn()
{
	if (w == Artists)
	{
		if (!hasTwoColumns && Songs->Empty())
			return;
		Artists->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Albums;
		Albums->HighlightColor(Config.active_column_color);
		if (!Albums->Empty())
			return;
	}
	if (w == Albums && !Songs->Empty())
	{
		Albums->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Songs;
		Songs->HighlightColor(Config.active_column_color);
	}
}

void MediaLibrary::PrevColumn()
{
	if (w == Songs)
	{
		Songs->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Albums;
		Albums->HighlightColor(Config.active_column_color);
		if (!Albums->Empty())
			return;
	}
	if (w == Albums && !hasTwoColumns)
	{
		Albums->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Artists;
		Artists->HighlightColor(Config.active_column_color);
	}
}

void MediaLibrary::LocateSong(const MPD::Song &s)
{
	std::string primary_tag;
	switch (Config.media_lib_primary_tag)
	{
		case MPD_TAG_ARTIST:
			primary_tag = s.GetArtist();
			break;
		case MPD_TAG_DATE:
			primary_tag = s.GetDate();
			break;
		case MPD_TAG_GENRE:
			primary_tag = s.GetGenre();
			break;
		case MPD_TAG_COMPOSER:
			primary_tag = s.GetComposer();
			break;
		case MPD_TAG_PERFORMER:
			primary_tag = s.GetPerformer();
			break;
		default:
			ShowMessage("Invalid tag type in left column of the media library");
			return;
	}
	if (primary_tag == "")
	{
		std::string item_type = IntoStr(Config.media_lib_primary_tag);
		ToLower(item_type);
		ShowMessage("Can't jump to media library because the song has no %s tag set.", item_type.c_str());
		return;
	}

	if (myScreen != this)
		SwitchTo();
	Statusbar() << "Jumping to song...";
	wFooter->Refresh();

	if (Artists->Empty() || primary_tag != Artists->Current())
	{
		Update();
		for (size_t i = 0; i < Artists->Size(); ++i)
		{
			if (primary_tag == Artists->at(i))
			{
				Artists->Highlight(i);
				Albums->Clear();
				break;
			}
		}
	}

	std::string album = s.GetAlbum();
	std::string date = s.GetDate();
	if (Albums->Empty() || (album != Albums->Current().second.Album
				&& date != Albums->Current().second.Year))
	{
		Update();
		for (size_t i = 0; i < Albums->Size(); ++i)
		{
			if (album == Albums->at(i).second.Album && date == Albums->at(i).second.Year)
			{
				Albums->Highlight(i);
				Songs->Clear();
				break;
			}
		}
	}

	std::string song = s.GetTitle();
	if (Songs->Empty() || song != Songs->Current().GetTitle())
	{
		Update();
		for (size_t i = 0; i < Songs->Size(); ++i)
		{
			if (song == Songs->at(i).GetTitle())
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
		Songs->Bold(Songs->Choice(), myPlaylist->Add(Songs->Current(), Songs->isBold(), add_n_play));
	else
	{
		MPD::SongList list;
		GetSelectedSongs(list);

		if (myPlaylist->Add(list, add_n_play))
		{
			if (!Artists->Empty() && w == Artists)
			{
				std::string tag_type = IntoStr(Config.media_lib_primary_tag);
				ToLower(tag_type);
				ShowMessage("Adding songs of %s \"%s\"", tag_type.c_str(), Artists->Current().c_str());
			}
			else if (w == Albums)
				ShowMessage("Adding songs from album \"%s\"", Albums->Current().second.Album.c_str());
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

std::string MediaLibrary::SongToString(const MPD::Song &s, void *)
{
	return s.toString(Config.song_library_format);
}

bool MediaLibrary::SortSongsByYear(Song *a, Song *b)
{
	return a->GetDate() < b->GetDate();
}

bool MediaLibrary::SortSongsByTrack(Song *a, Song *b)
{
	if (a->GetDisc() == b->GetDisc())
		return StrToInt(a->GetTrack()) < StrToInt(b->GetTrack());
	else
		return StrToInt(a->GetDisc()) < StrToInt(b->GetDisc());
}

