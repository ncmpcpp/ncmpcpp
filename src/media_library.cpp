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
	
	Artists = new Menu<string>(0, main_start_y, itsLeftColWidth, main_height, IntoStr(Config.media_lib_primary_tag) + "s", Config.main_color, brNone);
	Artists->HighlightColor(Config.active_column_color);
	Artists->SetTimeout(ncmpcpp_window_timeout);
	Artists->SetItemDisplayer(Display::Generic);
	
	Albums = new Menu<string_pair>(itsMiddleColStartX, main_start_y, itsMiddleColWidth, main_height, "Albums", Config.main_color, brNone);
	Albums->HighlightColor(Config.main_highlight_color);
	Albums->SetTimeout(ncmpcpp_window_timeout);
	Albums->SetItemDisplayer(Display::StringPairs);
	
	Songs = new Menu<Song>(itsRightColStartX, main_start_y, itsRightColWidth, main_height, "Songs", Config.main_color, brNone);
	Songs->HighlightColor(Config.main_highlight_color);
	Songs->SetTimeout(ncmpcpp_window_timeout);
	Songs->SetSelectPrefix(&Config.selected_item_prefix);
	Songs->SetSelectSuffix(&Config.selected_item_suffix);
	Songs->SetItemDisplayer(Display::Songs);
	Songs->SetItemDisplayerUserData(&Config.song_library_format);
	
	w = Artists;
}

void MediaLibrary::Resize()
{
	itsLeftColWidth = COLS/3-1;
	itsMiddleColStartX = itsLeftColWidth+1;
	itsMiddleColWidth = COLS/3;
	itsRightColStartX = itsLeftColWidth+itsMiddleColWidth+2;
	itsRightColWidth = COLS-COLS/3*2-1;
	
	Artists->Resize(itsLeftColWidth, main_height);
	Albums->Resize(itsMiddleColWidth, main_height);
	Songs->Resize(itsRightColWidth, main_height);
	
	Albums->MoveTo(itsMiddleColStartX, main_start_y);
	Songs->MoveTo(itsRightColStartX, main_start_y);
	
	hasToBeResized = 0;
}

void MediaLibrary::Refresh()
{
	Artists->Display();
	mvvline(main_start_y, itsMiddleColStartX-1, 0, main_height);
	Albums->Display();
	mvvline(main_start_y, itsRightColStartX-1, 0, main_height);
	Songs->Display();
	if (Albums->Empty())
	{
		*Albums << XY(0, 0) << "No albums found." << wrefresh;
	}
}

void MediaLibrary::SwitchTo()
{
	if (myScreen == this)
		return;
	
	if (hasToBeResized)
		Resize();
	
	CLEAR_FIND_HISTORY;
	myScreen = this;
	redraw_header = 1;
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
		CLEAR_FIND_HISTORY;
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
		std::vector<string_pair> maplist;
		locale_to_utf(Artists->Current());
		if (Config.media_lib_primary_tag == MPD_TAG_ITEM_ARTIST)
			Mpd->GetAlbums(Artists->Current(), list);
		else
		{
			Mpd->StartSearch(1);
			Mpd->AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd->StartFieldSearch(MPD_TAG_ITEM_ALBUM);
			Mpd->CommitSearch(list);
		}
		
		// <mpd-0.14 doesn't support searching for empty tag
		if (Mpd->Version() > 13)
		{
			SongList noalbum_list;
			Mpd->StartSearch(1);
			Mpd->AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, "");
			Mpd->CommitSearch(noalbum_list);
			if (!noalbum_list.empty())
				Albums->AddOption(std::make_pair("<no album>", ""));
			FreeSongList(noalbum_list);
		}
		
		for (TagList::iterator it = list.begin(); it != list.end(); it++)
		{
			SongList l;
			Mpd->StartSearch(1);
			Mpd->AddSearch(Config.media_lib_primary_tag, Artists->Current());
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, *it);
			Mpd->CommitSearch(l);
			if (!l.empty() && !l[0]->GetAlbum().empty())
			{
				utf_to_locale(*it);
				l[0]->Localize();
				maplist.push_back(make_pair(l[0]->toString(Config.media_lib_album_format), *it));
			}
			FreeSongList(l);
		}
		utf_to_locale(Artists->Current());
		sort(maplist.begin(), maplist.end(), CaseInsensitiveSorting());
		for (std::vector<string_pair>::const_iterator it = maplist.begin(); it != maplist.end(); it++)
			Albums->AddOption(*it);
		Albums->Window::Clear();
		Albums->Refresh();
	}
	
	if (!Artists->Empty() && myScreen->Cmp() == Albums && Albums->Empty())
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
			*Albums << XY(0, 0) << "No albums found." << wrefresh;
		}
		else
			Mpd->AddSearch(MPD_TAG_ITEM_ALBUM, locale_to_utf_cpy(Albums->Current().second));
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

void MediaLibrary::NextColumn()
{
	CLEAR_FIND_HISTORY;
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
	CLEAR_FIND_HISTORY;
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
		for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
			Mpd->QueueAddSong(**it);
		if (Mpd->CommitQueue())
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
		for (size_t i = 0; i < Songs->Size(); i++)
			Mpd->QueueAddSong(Songs->at(i));
		if (Mpd->CommitQueue())
		{
			ShowMessage("Adding songs from album \"%s\"", Albums->Current().second.c_str());
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
			block_item_list_update = 1;
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
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd->QueueDeleteSong(i);
							myPlaylist->Main()->DeleteOption(i);
							i--;
						}
					}
					Mpd->CommitQueue();
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

bool MediaLibrary::SortSongsByTrack(Song *a, Song *b)
{
	if (a->GetDisc() == b->GetDisc())
		return StrToInt(a->GetTrack()) < StrToInt(b->GetTrack());
	else
		return StrToInt(a->GetDisc()) < StrToInt(b->GetDisc());
}

