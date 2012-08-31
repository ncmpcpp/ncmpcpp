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

#include <cassert>
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
#include "utility/comparators.h"

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
	Playlists->setItemDisplayer(Display::Default<std::string>);
	
	Content = new Menu<MPD::Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Playlist's content" : "", Config.main_color, brNone);
	Content->HighlightColor(Config.main_highlight_color);
	Content->CyclicScrolling(Config.use_cyclic_scrolling);
	Content->CenteredCursor(Config.centered_cursor);
	Content->SetSelectPrefix(Config.selected_item_prefix);
	Content->SetSelectSuffix(Config.selected_item_suffix);
	if (Config.columns_in_playlist_editor)
	{
		Content->setItemDisplayer(std::bind(Display::SongsInColumns, _1, *this));
		Content->SetItemStringifier(Playlist::SongInColumnsToString);
	}
	else
	{
		Content->setItemDisplayer(std::bind(Display::Songs, _1, *this, Config.song_list_format));
		Content->SetItemStringifier(Playlist::SongToString);
	}
	
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
	Global::RedrawHeader = true;
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
			Playlists->AddItem(*it);
		}
		Playlists->Window::Clear();
		Playlists->Refresh();
	}
	
	if (!Playlists->Empty() && Content->ReallyEmpty())
	{
		Content->Reset();
		size_t plsize = 0;
		Mpd.GetPlaylistContent(locale_to_utf_cpy(Playlists->Current().value()), [this, &plsize](MPD::Song &&s) {
			Content->AddItem(s, myPlaylist->checkForSong(s));
			++plsize;
		});
		if (plsize > 0)
		{
			std::string title = Config.titles_visibility ? "Playlist content (" + unsignedLongIntTo<std::string>::apply(plsize) + " item" + (plsize == 1 ? ")" : "s)") : "";
			title.resize(Content->GetWidth());
			Content->SetTitle(title);
		}
		else
			Content->SetTitle(Config.titles_visibility ? "Playlist content" : "");
		
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

void PlaylistEditor::MoveSelectedItems(Playlist::Movement where)
{
	if (Content->Empty())
		return;
	
	// remove search results as we may move them to different positions, but
	// search rememebers positions and may point to wrong ones after that.
	Content->Search("");
	
	switch (where)
	{
		case Playlist::mUp:
		{
			if (Content->hasSelected())
			{
				std::vector<size_t> list;
				Content->GetSelected(list);
					
				if (list.front() > 0)
				{
					Mpd.StartCommandsList();
					std::vector<size_t>::const_iterator it = list.begin();
					for (; it != list.end(); ++it)
						Mpd.Move(Playlists->Current().value(), *it-1, *it);
					if (Mpd.CommitCommandsList())
					{
						for (it = list.begin(); it != list.end(); ++it)
							Content->Swap(*it-1, *it);
						Content->Highlight(list[(list.size()-1)/2]-1);
					}
				}
			}
			else
			{
				size_t pos = Content->Choice();
				if (pos > 0)
				{
					if (Mpd.Move(Playlists->Current().value(), pos-1, pos))
					{
						Content->Scroll(wUp);
						Content->Swap(pos-1, pos);
					}
				}
			}
			break;
		}
		case Playlist::mDown:
		{
			if (Content->hasSelected())
			{
				std::vector<size_t> list;
				Content->GetSelected(list);
				
				if (list.back() < Content->Size()-1)
				{
					Mpd.StartCommandsList();
					std::vector<size_t>::const_reverse_iterator it = list.rbegin();
					for (; it != list.rend(); ++it)
						Mpd.Move(Playlists->Current().value(), *it, *it+1);
					if (Mpd.CommitCommandsList())
					{
						Content->Highlight(list[(list.size()-1)/2]+1);
						for (it = list.rbegin(); it != list.rend(); ++it)
							Content->Swap(*it, *it+1);
					}
				}
			}
			else
			{
				size_t pos = Content->Choice();
				if (pos < Content->Size()-1)
				{
					if (Mpd.Move(Playlists->Current().value(), pos, pos+1))
					{
						Content->Scroll(wDown);
						Content->Swap(pos, pos+1);
					}
				}
			}
			break;
		}
	}
}

bool PlaylistEditor::isContentFiltered()
{
	if (Content->isFiltered())
	{
		ShowMessage("Function currently unavailable due to filtered playlist content");
		return true;
	}
	return false;
}


bool PlaylistEditor::isNextColumnAvailable()
{
	if (w == Playlists)
	{
		if (!Content->ReallyEmpty())
			return true;
	}
	return false;
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

bool PlaylistEditor::isPrevColumnAvailable()
{
	if (w == Content)
	{
		if (!Playlists->ReallyEmpty())
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
		if (Mpd.LoadPlaylist(utf_to_locale_cpy(Playlists->Current().value())))
		{
			ShowMessage("Playlist \"%s\" loaded", Playlists->Current().value().c_str());
			if (add_n_play)
				myPlaylist->PlayNewlyAddedSongs();
		}
	}
	else if (w == Content && !Content->Empty())
	{
		bool res = myPlaylist->Add(Content->Current().value(), Content->Current().isBold(), add_n_play);
		Content->Current().setBold(res);
	}
	
	if (!add_n_play)
		w->Scroll(wDown);
}

void PlaylistEditor::SpacePressed()
{
	if (Config.space_selects && w == Content)
	{
		Content->Current().setSelected(!Content->Current().isSelected());
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
	return w == Content && !Content->Empty() ? &Content->Current().value() : 0;
}

void PlaylistEditor::GetSelectedSongs(MPD::SongList &v)
{
	std::vector<size_t> selected;
	Content->GetSelected(selected);
	if (selected.empty())
		selected.push_back(Content->Choice());
	for (auto it = selected.begin(); it != selected.end(); ++it)
		v.push_back(Content->at(*it).value());
}

std::string PlaylistEditor::currentFilter()
{
	std::string filter;
	if (w == Playlists)
		filter = RegexFilter<std::string>::currentFilter(*Playlists);
	else if (w == Content)
		filter = RegexFilter<MPD::Song>::currentFilter(*Content);
	return filter;
}

void PlaylistEditor::applyFilter(const std::string &filter)
{
	if (w == Playlists)
	{
		Playlists->ShowAll();
		auto rx = RegexFilter<std::string>(filter, Config.regex_type);
		Playlists->Filter(Playlists->Begin(), Playlists->End(), rx);
	}
	else if (w == Content)
	{
		Content->ShowAll();
		auto rx = RegexFilter<MPD::Song>(filter, Config.regex_type);
		Content->Filter(Content->Begin(), Content->End(), rx);
	}
}

void PlaylistEditor::Locate(const std::string &name)
{
	if (!isInitialized)
		Init();
	Update();
	for (size_t i = 0; i < Playlists->Size(); ++i)
	{
		if (name == (*Playlists)[i].value())
		{
			Playlists->Highlight(i);
			Content->Clear();
			break;
		}
	}
	SwitchTo();
}

List *PlaylistEditor::GetList()
{
	if (w == Playlists)
		return Playlists;
	else if (w == Content)
		return Content;
	else // silence compiler
		assert(false);
}
