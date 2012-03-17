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

#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "mpdpp.h"
#include "status.h"
#include "tag_editor.h"

using Global::MainHeight;
using Global::MainStartY;

PlaylistEditor *myPlaylistEditor = new PlaylistEditor;

size_t PlaylistEditor::LeftColumnStartX;
size_t PlaylistEditor::LeftColumnWidth;
size_t PlaylistEditor::RightColumnStartX;
size_t PlaylistEditor::RightColumnWidth;

void PlaylistEditor::Init()
{
	LeftColumnWidth = COLS/3-1;
	RightColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-1;
	
	Playlists = new Menu<std::string>(0, MainStartY, LeftColumnWidth, MainHeight, Config.titles_visibility ? "Playlists" : "", Config.main_color, brNone);
	Playlists->HighlightColor(Config.active_column_color);
	Playlists->CyclicScrolling(Config.use_cyclic_scrolling);
	Playlists->CenteredCursor(Config.centered_cursor);
	Playlists->SetItemDisplayer(Display::Generic);
	
	static Display::ScreenFormat sf = { this, &Config.song_list_format };
	
	Content = new Menu<MPD::Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Playlist's content" : "", Config.main_color, brNone);
	Content->HighlightColor(Config.main_highlight_color);
	Content->CyclicScrolling(Config.use_cyclic_scrolling);
	Content->CenteredCursor(Config.centered_cursor);
	Content->SetSelectPrefix(&Config.selected_item_prefix);
	Content->SetSelectSuffix(&Config.selected_item_suffix);
	Content->SetItemDisplayer(Config.columns_in_playlist_editor ? Display::SongsInColumns : Display::Songs);
	Content->SetItemDisplayerUserData(&sf);
	Content->SetGetStringFunction(Config.columns_in_playlist_editor ? Playlist::SongInColumnsToString : Playlist::SongToString);
	Content->SetGetStringFunctionUserData(&Config.song_list_format_dollar_free);
	
	w = Playlists;
	isInitialized = 1;
}

void PlaylistEditor::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	
	LeftColumnStartX = x_offset;
	LeftColumnWidth = width/3-1;
	RightColumnStartX = LeftColumnStartX+LeftColumnWidth+1;
	RightColumnWidth = width-LeftColumnWidth-1;
	
	Playlists->Resize(LeftColumnWidth, MainHeight);
	Content->Resize(RightColumnWidth, MainHeight);
	
	Playlists->MoveTo(LeftColumnStartX, MainStartY);
	Content->MoveTo(RightColumnStartX, MainStartY);
	
	hasToBeResized = 0;
}

std::basic_string<my_char_t> PlaylistEditor::Title()
{
	return U("Playlist editor");
}

void PlaylistEditor::Refresh()
{
	Playlists->Display();
	mvvline(MainStartY, RightColumnStartX-1, 0, MainHeight);
	Content->Display();
}

void PlaylistEditor::SwitchTo()
{
	using Global::myScreen;
	using Global::myLockedScreen;
	
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	Global::RedrawHeader = 1;
	Refresh();
	UpdateSongList(Content);
}

void PlaylistEditor::Update()
{
	if (Playlists->ReallyEmpty())
	{
		Content->Clear();
		MPD::TagList list;
		Mpd.GetPlaylists(list);
		sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (MPD::TagList::iterator it = list.begin(); it != list.end(); ++it)
		{
			utf_to_locale(*it);
			Playlists->AddOption(*it);
		}
		Playlists->Window::Clear();
		Playlists->Refresh();
	}
	
	if (!Playlists->Empty() && Content->ReallyEmpty())
	{
		Content->Reset();
		MPD::SongList list;
		Mpd.GetPlaylistContent(locale_to_utf_cpy(Playlists->Current()), list);
		if (!list.empty())
		{
			std::string title = Config.titles_visibility ? "Playlist's content (" + IntoStr(list.size()) + " item" + (list.size() == 1 ? ")" : "s)") : "";
			title.resize(Content->GetWidth());
			Content->SetTitle(title);
		}
		else
			Content->SetTitle(Config.titles_visibility ? "Playlist's content" : "");
		bool bold = 0;
		for (MPD::SongList::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			for (size_t j = 0; j < myPlaylist->Items->Size(); ++j)
			{
				if ((*it)->GetHash() == myPlaylist->Items->at(j).GetHash())
				{
					bold = 1;
					break;
				}
			}
			Content->AddOption(**it, bold);
			bold = 0;
		}
		MPD::FreeSongList(list);
		Content->Window::Clear();
		Content->Display();
	}
	
	if (w == Content && Content->ReallyEmpty())
	{
		Content->HighlightColor(Config.main_highlight_color);
		Playlists->HighlightColor(Config.active_column_color);
		w = Playlists;
	}
	
	if (Content->ReallyEmpty())
	{
		*Content << XY(0, 0) << "Playlist is empty.";
		Content->Window::Refresh();
	}
}

bool PlaylistEditor::NextColumn()
{
	if (w == Playlists)
	{
		Playlists->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Content;
		Content->HighlightColor(Config.active_column_color);
		return true;
	}
	return false;
}

bool PlaylistEditor::PrevColumn()
{
	if (w == Content)
	{
		Content->HighlightColor(Config.main_highlight_color);
		w->Refresh();
		w = Playlists;
		Playlists->HighlightColor(Config.active_column_color);
		return true;
	}
	return false;
}

void PlaylistEditor::AddToPlaylist(bool add_n_play)
{
	MPD::SongList list;
	
	if (w == Playlists && !Playlists->Empty())
	{
		Mpd.GetPlaylistContent(locale_to_utf_cpy(Playlists->Current()), list);
		if (myPlaylist->Add(list, add_n_play))
			ShowMessage("Loading playlist %s...", Playlists->Current().c_str());
	}
	else if (w == Content && !Content->Empty())
		Content->Bold(Content->Choice(), myPlaylist->Add(Content->Current(), Content->isBold(), add_n_play));
	
	FreeSongList(list);
	if (!add_n_play)
		w->Scroll(wDown);
}

void PlaylistEditor::SpacePressed()
{
	if (Config.space_selects && w == Content)
	{
		Content->Select(Content->Choice(), !Content->isSelected());
		w->Scroll(wDown);
	}
	else
		AddToPlaylist(0);
}

void PlaylistEditor::MouseButtonPressed(MEVENT me)
{
	if (!Playlists->Empty() && Playlists->hasCoords(me.x, me.y))
	{
		if (w != Playlists)
			PrevColumn();
		if (size_t(me.y) < Playlists->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Playlists->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Playlists->Choice();
				SpacePressed();
				if (pos < Playlists->Size()-1)
					Playlists->Scroll(wUp);
			}
		}
		else
			Screen<Window>::MouseButtonPressed(me);
		Content->Clear();
	}
	else if (!Content->Empty() && Content->hasCoords(me.x, me.y))
	{
		if (w != Content)
			NextColumn();
		if (size_t(me.y) < Content->Size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
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
	if (selected.empty())
		selected.push_back(Content->Choice());
	for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
	{
		v.push_back(new MPD::Song(Content->at(*it)));
	}
}

void PlaylistEditor::ApplyFilter(const std::string &s)
{
	GetList()->ApplyFilter(s, 0, REG_ICASE | Config.regex_type);
}

void PlaylistEditor::JumpTo(const std::string &s)
{
	SwitchTo();
	for (size_t i = 0; i < Playlists->Size(); ++i)
	{
		if (s == (*Playlists)[i])
		{
			Playlists->Highlight(i);
			Content->Clear();
			break;
		}
	}
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

