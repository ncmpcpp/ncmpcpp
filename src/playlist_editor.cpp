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
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "mpdpp.h"
#include "status.h"
#include "tag_editor.h"

using namespace Global;
using namespace MPD;
using std::string;

PlaylistEditor *myPlaylistEditor = new PlaylistEditor;

size_t PlaylistEditor::LeftColumnWidth;
size_t PlaylistEditor::RightColumnStartX;
size_t PlaylistEditor::RightColumnWidth;

void PlaylistEditor::Init()
{
	LeftColumnWidth = COLS/3-1;
	RightColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-1;
	
	Playlists = new Menu<string>(0, MainStartY, LeftColumnWidth, MainHeight, "Playlists", Config.main_color, brNone);
	Playlists->HighlightColor(Config.active_column_color);
	Playlists->SetTimeout(ncmpcpp_window_timeout);
	Playlists->CyclicScrolling(Config.use_cyclic_scrolling);
	Playlists->SetItemDisplayer(Display::Generic);
	
	Content = new Menu<Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, "Playlist's content", Config.main_color, brNone);
	Content->HighlightColor(Config.main_highlight_color);
	Content->SetTimeout(ncmpcpp_window_timeout);
	Content->CyclicScrolling(Config.use_cyclic_scrolling);
	Content->SetSelectPrefix(&Config.selected_item_prefix);
	Content->SetSelectSuffix(&Config.selected_item_suffix);
	Content->SetItemDisplayer(Display::Songs);
	Content->SetItemDisplayerUserData(&Config.song_list_format);
	Content->SetGetStringFunction(Playlist::SongToString);
	Content->SetGetStringFunctionUserData(&Config.song_list_format);
	
	w = Playlists;
	isInitialized = 1;
}

void PlaylistEditor::Resize()
{
	LeftColumnWidth = COLS/3-1;
	RightColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-1;
	
	Playlists->Resize(LeftColumnWidth, MainHeight);
	Content->Resize(RightColumnWidth, MainHeight);
	
	Content->MoveTo(RightColumnStartX, MainStartY);
	
	hasToBeResized = 0;
}

std::string PlaylistEditor::Title()
{
	return "Playlist editor";
}

void PlaylistEditor::Refresh()
{
	Playlists->Display();
	mvvline(MainStartY, RightColumnStartX-1, 0, MainHeight);
	Content->Display();
}

void PlaylistEditor::SwitchTo()
{
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (hasToBeResized)
		Resize();
	
	myScreen = this;
	RedrawHeader = 1;
	Refresh();
	UpdateSongList(Content);
}

void PlaylistEditor::Update()
{
	if (Playlists->Empty())
	{
		Content->Clear(0);
		TagList list;
		Mpd.GetPlaylists(list);
		sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (TagList::iterator it = list.begin(); it != list.end(); it++)
		{
			utf_to_locale(*it);
			Playlists->AddOption(*it);
		}
		Playlists->Window::Clear();
		Playlists->Refresh();
	}
	
	if (!Playlists->Empty() && Content->Empty())
	{
		Content->Reset();
		SongList list;
		Mpd.GetPlaylistContent(locale_to_utf_cpy(Playlists->Current()), list);
		if (!list.empty())
			Content->SetTitle("Playlist's content (" + IntoStr(list.size()) + " item" + (list.size() == 1 ? ")" : "s)"));
		else
			Content->SetTitle("Playlist's content");
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
			Content->AddOption(**it, bold);
			bold = 0;
		}
		FreeSongList(list);
		Content->Window::Clear();
		Content->Display();
	}
	
	if (w == Content && Content->Empty())
	{
		Content->HighlightColor(Config.main_highlight_color);
		Playlists->HighlightColor(Config.active_column_color);
		w = Playlists;
	}
	
	if (Content->Empty())
	{
		*Content << XY(0, 0) << "Playlist is empty.";
		Content->Window::Refresh();
	}
}

void PlaylistEditor::NextColumn()
{
	if (w == Playlists)
	{
		Playlists->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Content;
		Content->HighlightColor(Config.active_column_color);
	}
}

void PlaylistEditor::PrevColumn()
{
	if (w == Content)
	{
		Content->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Playlists;
		Playlists->HighlightColor(Config.active_column_color);
	}
}

void PlaylistEditor::AddToPlaylist(bool add_n_play)
{
	SongList list;
	
	if (w == Playlists && !Playlists->Empty())
	{
		Mpd.GetPlaylistContent(locale_to_utf_cpy(Playlists->Current()), list);
		Mpd.StartCommandsList();
		SongList::const_iterator it = list.begin();
		for (; it != list.end(); it++)
			if (Mpd.AddSong(**it) < 0)
				break;
		Mpd.CommitCommandsList();
		
		if (it != list.begin())
		{
			ShowMessage("Loading playlist %s...", Playlists->Current().c_str());
			Song &s = myPlaylist->Main()->at(myPlaylist->Main()->Size()-list.size());
			if (s.GetHash() == list[0]->GetHash())
			{
				if (add_n_play)
					Mpd.PlayID(s.GetID());
			}
			else
				ShowMessage("%s", MPD::Message::PartOfSongsAdded);
		}
	}
	else if (w == Content)
	{
		if (!Content->Empty())
		{
			BlockItemListUpdate = 1;
			if (Config.ncmpc_like_songs_adding && Content->isBold())
			{
				long long hash = Content->Current().GetHash();
				if (add_n_play)
				{
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd.Play(i);
							break;
						}
					}
				}
				else
				{
					Playlist::BlockUpdate = 1;
					Mpd.StartCommandsList();
					for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
					{
						if (myPlaylist->Main()->at(i).GetHash() == hash)
						{
							Mpd.Delete(i);
							myPlaylist->Main()->DeleteOption(i);
							i--;
						}
					}
					Mpd.CommitCommandsList();
					Content->BoldOption(Content->Choice(), 0);
					Playlist::BlockUpdate = 0;
				}
			}
			else
			{
				const Song &s = Content->Current();
				int id = Mpd.AddSong(s);
				if (id >= 0)
				{
					ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
					if (add_n_play)
						Mpd.PlayID(id);
					Content->BoldOption(Content->Choice(), 1);
				}
			}
		}
	}
	FreeSongList(list);
	if (!add_n_play)
		w->Scroll(wDown);
}

void PlaylistEditor::SpacePressed()
{
	if (Config.space_selects && w == Content)
	{
		Content->SelectCurrent();
		w->Scroll(wDown);
		return;
	}
	AddToPlaylist(0);
}

void PlaylistEditor::MouseButtonPressed(MEVENT me)
{
	if (!Playlists->Empty() && Playlists->hasCoords(me.x, me.y))
	{
		if (w != Playlists)
			PrevColumn();
		if (size_t(me.y) < Playlists->Size() && (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED))
		{
			Playlists->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				EnterPressed();
		}
		else
			Screen<Window>::MouseButtonPressed(me);
		Content->Clear(0);
	}
	else if (!Content->Empty() && Content->hasCoords(me.x, me.y))
	{
		if (w != Content)
			NextColumn();
		if (size_t(me.y) < Content->Size() && (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED))
		{
			Content->Goto(me.y);
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = Content->Choice();
				SpacePressed();
				if (pos < Content->Size()-1)
					Content->Scroll(wUp);
			}
			else
				EnterPressed();
		}
		else
			Screen<Window>::MouseButtonPressed(me);
	}
}

MPD::Song *PlaylistEditor::CurrentSong()
{
	return w == Content && !Content->Empty() ? &Content->Current() : 0;
}

void PlaylistEditor::GetSelectedSongs(MPD::SongList &v)
{
	std::vector<size_t> selected;
	Content->GetSelected(selected);
	for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); it++)
	{
		v.push_back(new MPD::Song(Content->at(*it)));
	}
}

void PlaylistEditor::ApplyFilter(const std::string &s)
{
	GetList()->ApplyFilter(s, 0, REG_ICASE | Config.regex_type);
}

List *PlaylistEditor::GetList()
{
	if (w == Playlists)
		return Playlists;
	else if (w == Content)
		return Content;
	else // silence compiler
		return 0;
}

