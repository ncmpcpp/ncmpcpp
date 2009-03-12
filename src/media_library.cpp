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
using std::string;

MediaLibrary *myLibrary = new MediaLibrary;

size_t MediaLibrary::itsLeftColWidth;
size_t MediaLibrary::itsMiddleColWidth;
size_t MediaLibrary::itsMiddleColStartX;
size_t MediaLibrary::itsRightColWidth;
size_t MediaLibrary::itsRightColStartX;

void MediaLibrary::Init()
{
	itsLeftColWidth = COLS/3-1;
	itsMiddleColWidth = COLS/3;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsRightColWidth = COLS-COLS/3*2-1;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	
	Artists = new Menu<string>(0, MainStartY, itsLeftColWidth, MainHeight, IntoStr(Config.media_lib_primary_tag) + "s", Config.main_color, brNone);
	Artists->HighlightColor(Config.active_column_color);
	Artists->SetTimeout(ncmpcpp_window_timeout);
	Artists->SetItemDisplayer(Display::Generic);
	
	Albums = new Menu< std::pair<std::string, SearchConstraints> >(itsMiddleColStartX, MainStartY, itsMiddleColWidth, MainHeight, "Albums", Config.main_color, brNone);
	Albums->HighlightColor(Config.main_highlight_color);
	Albums->SetTimeout(ncmpcpp_window_timeout);
	Albums->SetItemDisplayer(Display::Pairs);
	Albums->SetGetStringFunction(StringPairToString);
	
	Songs = new Menu<Song>(itsRightColStartX, MainStartY, itsRightColWidth, MainHeight, "Songs", Config.main_color, brNone);
	Songs->HighlightColor(Config.main_highlight_color);
	Songs->SetTimeout(ncmpcpp_window_timeout);
	Songs->SetSelectPrefix(&Config.selected_item_prefix);
	Songs->SetSelectSuffix(&Config.selected_item_suffix);
	Songs->SetItemDisplayer(Display::Songs);
	Songs->SetItemDisplayerUserData(&Config.song_library_format);
	Songs->SetGetStringFunction(SongToString);
	
	w = Artists;
}

void MediaLibrary::Resize()
{
	itsLeftColWidth = COLS/3-1;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsMiddleColWidth = COLS/3;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	itsRightColWidth = COLS-COLS/3*2-1;
	
	Artists->Resize(itsLeftColWidth, MainHeight);
	Albums->Resize(itsMiddleColWidth, MainHeight);
	Songs->Resize(itsRightColWidth, MainHeight);
	
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
		return;
	
	if (hasToBeResized)
		Resize();
	
	myScreen = this;
	RedrawHeader = 1;
	Refresh();
	UpdateSongList(Songs);
}

std::string MediaLibrary::Title()
{
	return "Media library";
}

void MediaLibrary::Update()
{
	if (Artists->Empty())
	{
		TagList list;
		Albums->Clear(0);
		Songs->Clear(0);
		Mpd->GetList(list, Config.media_lib_primary_tag);
		sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (TagList::iterator it = list.begin(); it != list.end(); it++)
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
	
	if (!Artists->Empty() && Albums->Empty() && Songs->Empty())
	{
		Albums->Reset();
		TagList list;
		locale_to_utf(Artists->Current());
		if (Config.media_lib_primary_tag == MPD_TAG_ITEM_ARTIST)
			Mpd->GetAlbums(Artists->Current(), list);
		else
		{
			Mpd->StartFieldSearch(MPD_TAG_ITEM_ALBUM);
			Mpd->AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd->CommitSearch(list);
		}
		
		// <mpd-0.14 doesn't support searching for empty tag
		if (Mpd->Version() > 13)
		{
			TagList noalbum_list;
			Mpd->StartFieldSearch(MPD_TAG_ITEM_FILENAME);
			Mpd->AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, "");
			Mpd->CommitSearch(noalbum_list);
			if (!noalbum_list.empty())
				Albums->AddOption(std::make_pair("<no album>", SearchConstraints("", "")));
		}
		
		for (TagList::iterator it = list.begin(); it != list.end(); it++)
		{
			TagList l;
			Mpd->StartFieldSearch(MPD_TAG_ITEM_DATE);
			Mpd->AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, *it);
			Mpd->CommitSearch(l);
			utf_to_locale(*it);
			if (l.empty())
			{
				Albums->AddOption(std::make_pair(*it, SearchConstraints(*it, "")));
				continue;
			}
			for (TagList::const_iterator j = l.begin(); j != l.end(); j++)
				Albums->AddOption(std::make_pair("(" + *j + ") " + *it, SearchConstraints(*it, *j)));
		}
		utf_to_locale(Artists->Current());
		Albums->Sort<CaseInsensitiveSorting>((*Albums)[0].first == "<no album>");
		Albums->Window::Clear();
		Albums->Refresh();
	}
	
	if (!Artists->Empty() && w == Albums && Albums->Empty())
	{
		Albums->HighlightColor(Config.main_highlight_color);
		Artists->HighlightColor(Config.active_column_color);
		w = Artists;
	}
	
	if (!Artists->Empty() && Songs->Empty())
	{
		Songs->Reset();
		SongList list;
		
		Songs->Clear(0);
		Mpd->StartSearch(1);
		Mpd->AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(Artists->Current()));
		if (Albums->Empty()) // left for compatibility with <mpd-0.14
		{
			*Albums << XY(0, 0) << "No albums found.";
			Albums->Window::Refresh();
		}
		else
		{
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, locale_to_utf_cpy(Albums->Current().second.Album));
			if (!Albums->Current().second.Album.empty()) // for <no album>
				Mpd->AddSearch(MPD_TAG_ITEM_DATE, locale_to_utf_cpy(Albums->Current().second.Year));
		}
		Mpd->CommitSearch(list);
		
		sort(list.begin(), list.end(), SortSongsByTrack);
		bool bold = 0;
		
		for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
		{
			for (size_t j = 0; j < myPlaylist->Main()->Size(); j++)
			{
				if ((*it)->GetHash() == myPlaylist->Main()->at(j).GetHash())
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
	if (Config.space_selects && w == Songs)
	{
		Songs->SelectCurrent();
		w->Scroll(wDown);
		return;
	}
	AddToPlaylist(0);
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

void MediaLibrary::GetSelectedSongs(MPD::SongList &v)
{
	std::vector<size_t> selected;
	Songs->GetSelected(selected);
	for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); it++)
	{
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
		if (Songs->Empty())
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
	if (w == Albums)
	{
		Albums->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Artists;
		Artists->HighlightColor(Config.active_column_color);
	}
}

void MediaLibrary::AddToPlaylist(bool add_n_play)
{
	SongList list;
	
	if (!Artists->Empty() && w == Artists)
	{
		Mpd->StartSearch(1);
		Mpd->AddSearch(Config.media_lib_primary_tag, locale_to_utf_cpy(Artists->Current()));
		Mpd->CommitSearch(list);
		Mpd->StartCommandsList();
		SongList::const_iterator it = list.begin();
		for (; it != list.end(); it++)
			if (Mpd->AddSong(**it) < 0)
				break;
		Mpd->CommitCommandsList();
		
		if (it != list.begin())
		{
			string tag_type = IntoStr(Config.media_lib_primary_tag);
			ToLower(tag_type);
			ShowMessage("Adding songs of %s \"%s\"", tag_type.c_str(), Artists->Current().c_str());
			Song *s = &myPlaylist->Main()->at(myPlaylist->Main()->Size()-list.size());
			if (s->GetHash() == list[0]->GetHash())
			{
				if (add_n_play)
					Mpd->PlayID(s->GetID());
			}
			else
				ShowMessage("%s", MPD::Message::PartOfSongsAdded);
		}
	}
	else if (w == Albums)
	{
		Mpd->StartCommandsList();
		size_t i = 0;
		for (; i < Songs->Size(); i++)
			if (Mpd->AddSong(Songs->at(i)) < 0)
				break;
		Mpd->CommitCommandsList();
		
		if (i)
		{
			ShowMessage("Adding songs from album \"%s\"", Albums->Current().second.Album.c_str());
			Song *s = &myPlaylist->Main()->at(myPlaylist->Main()->Size()-Songs->Size());
			if (s->GetHash() == Songs->at(0).GetHash())
			{
				if (add_n_play)
					Mpd->PlayID(s->GetID());
			}
			else
				ShowMessage("%s", MPD::Message::PartOfSongsAdded);
		}
	}
	else if (w == Songs)
	{
		if (!Songs->Empty())
		{
			BlockItemListUpdate = 1;
			if (Config.ncmpc_like_songs_adding && Songs->isBold())
			{
				long long hash = Songs->Current().GetHash();
				if (add_n_play)
				{
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd->Play(i);
							break;
						}
					}
				}
				else
				{
					Playlist::BlockUpdate = 1;
					Mpd->StartCommandsList();
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd->Delete(i);
							myPlaylist->Main()->DeleteOption(i);
							i--;
						}
					}
					Mpd->CommitCommandsList();
					Songs->BoldOption(Songs->Choice(), 0);
				}
			}
			else
			{
				Song &s = Songs->Current();
				int id = Mpd->AddSong(s);
				if (id >= 0)
				{
					ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
					if (add_n_play)
						Mpd->PlayID(id);
					Songs->BoldOption(Songs->Choice(), 1);
				}
			}
		}
	}
	FreeSongList(list);
	if (!add_n_play)
	{
		w->Scroll(wDown);
		if (w == Artists)
		{
			Albums->Clear(0);
			Songs->Clear(0);
		}
		else if (w == Albums)
			Songs->Clear(0);
	}
}

std::string MediaLibrary::SongToString(const MPD::Song &s, void *)
{
	return s.toString(Config.song_library_format);
}

bool MediaLibrary::SortSongsByYear(Song *a, Song *b)
{
	return a->GetYear() < b->GetYear();
}

bool MediaLibrary::SortSongsByTrack(Song *a, Song *b)
{
	if (a->GetDisc() == b->GetDisc())
		return StrToInt(a->GetTrack()) < StrToInt(b->GetTrack());
	else
		return StrToInt(a->GetDisc()) < StrToInt(b->GetDisc());
}

