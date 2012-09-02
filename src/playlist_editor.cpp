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

using namespace std::placeholders;

using Global::MainHeight;
using Global::MainStartY;

PlaylistEditor *myPlaylistEditor = new PlaylistEditor;

namespace {//

size_t LeftColumnStartX;
size_t LeftColumnWidth;
size_t RightColumnStartX;
size_t RightColumnWidth;

std::string SongToString(const MPD::Song &s);
bool PlaylistEntryMatcher(const Regex &rx, const std::string &playlist);
bool SongEntryMatcher(const Regex &rx, const MPD::Song &s);

}

void PlaylistEditor::Init()
{
	LeftColumnWidth = COLS/3-1;
	RightColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-1;
	
	Playlists = new NC::Menu<std::string>(0, MainStartY, LeftColumnWidth, MainHeight, Config.titles_visibility ? "Playlists" : "", Config.main_color, NC::brNone);
	Playlists->HighlightColor(Config.active_column_color);
	Playlists->CyclicScrolling(Config.use_cyclic_scrolling);
	Playlists->CenteredCursor(Config.centered_cursor);
	Playlists->SetSelectPrefix(Config.selected_item_prefix);
	Playlists->SetSelectSuffix(Config.selected_item_suffix);
	Playlists->setItemDisplayer(Display::Default<std::string>);
	
	Content = new NC::Menu<MPD::Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Playlist content" : "", Config.main_color, NC::brNone);
	Content->HighlightColor(Config.main_highlight_color);
	Content->CyclicScrolling(Config.use_cyclic_scrolling);
	Content->CenteredCursor(Config.centered_cursor);
	Content->SetSelectPrefix(Config.selected_item_prefix);
	Content->SetSelectSuffix(Config.selected_item_suffix);
	if (Config.columns_in_playlist_editor)
		Content->setItemDisplayer(std::bind(Display::SongsInColumns, _1, *this));
	else
		Content->setItemDisplayer(std::bind(Display::Songs, _1, *this, Config.song_list_format));
	
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
		auto list = Mpd.GetPlaylists();
		std::sort(list.begin(), list.end(), CaseInsensitiveSorting());
		for (auto it = list.begin(); it != list.end(); ++it)
			Playlists->AddItem(*it);
		Playlists->Refresh();
	}
	
	if (!Playlists->Empty() && Content->ReallyEmpty())
	{
		Content->Reset();
		size_t plsize = 0;
		auto songs = Mpd.GetPlaylistContent(Playlists->Current().value());
		for (auto s = songs.begin(); s != songs.end(); ++s, ++plsize)
			Content->AddItem(*s, myPlaylist->checkForSong(*s));
		std::string title;
		if (Config.titles_visibility)
		{
			title = "Playlist content";
			if (plsize > 0)
			{
				title += " (";
				title += unsignedLongIntTo<std::string>::apply(plsize);
				title += " item";
				if (plsize == 1)
					title += ")";
				else
					title += "s)";
			}
			title.resize(Content->GetWidth());
		}
		Content->SetTitle(title);
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
		*Content << NC::XY(0, 0) << "Playlist is empty.";
		Content->Window::Refresh();
	}
}

void PlaylistEditor::MoveSelectedItems(Playlist::Movement where)
{
	if (Content->Empty())
		return;
	
	// remove search results as we may move them to different positions, but
	// search rememebers positions and may point to wrong ones after that.
	Content->clearSearchResults();
	
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
						Content->Scroll(NC::wUp);
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
						Content->Scroll(NC::wDown);
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
		w->Scroll(NC::wDown);
}

void PlaylistEditor::SpacePressed()
{
	if (Config.space_selects)
	{
		if (w == Playlists)
		{
			if (!Playlists->Empty())
			{
				Playlists->Current().setSelected(!Playlists->Current().isSelected());
				Playlists->Scroll(NC::wDown);
			}
		}
		else if (w == Content)
		{
			if (!Content->Empty())
			{
				Content->Current().setSelected(!Content->Current().isSelected());
				Content->Scroll(NC::wDown);
			}
		}
	}
	else
		AddToPlaylist(false);
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
					Playlists->Scroll(NC::wUp);
			}
		}
		else
			Screen<NC::Window>::MouseButtonPressed(me);
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
					Content->Scroll(NC::wUp);
			}
			else
				EnterPressed();
		}
		else
			Screen<NC::Window>::MouseButtonPressed(me);
	}
}

/***********************************************************************/

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
		auto rx = RegexFilter<std::string>(filter, Config.regex_type, PlaylistEntryMatcher);
		Playlists->filter(Playlists->Begin(), Playlists->End(), rx);
	}
	else if (w == Content)
	{
		Content->ShowAll();
		auto rx = RegexFilter<MPD::Song>(filter, Config.regex_type, SongEntryMatcher);
		Content->filter(Content->Begin(), Content->End(), rx);
	}
}

/***********************************************************************/

bool PlaylistEditor::search(const std::string &constraint)
{
	bool result = false;
	if (w == Playlists)
	{
		auto rx = RegexFilter<std::string>(constraint, Config.regex_type, PlaylistEntryMatcher);
		result = Playlists->search(Playlists->Begin(), Playlists->End(), rx);
	}
	else if (w == Content)
	{
		auto rx = RegexFilter<MPD::Song>(constraint, Config.regex_type, SongEntryMatcher);
		result = Content->search(Content->Begin(), Content->End(), rx);
	}
	return result;
}

void PlaylistEditor::nextFound(bool wrap)
{
	if (w == Playlists)
		Playlists->NextFound(wrap);
	else if (w == Content)
		Content->NextFound(wrap);
}

void PlaylistEditor::prevFound(bool wrap)
{
	if (w == Playlists)
		Playlists->PrevFound(wrap);
	else if (w == Content)
		Content->PrevFound(wrap);
}

/***********************************************************************/

std::shared_ptr<ProxySongList> PlaylistEditor::getProxySongList()
{
	auto ptr = nullProxySongList();
	if (w == Content)
		ptr = mkProxySongList(*Content, [](NC::Menu<MPD::Song>::Item &item) {
			return &item.value();
		});
	return ptr;
}

bool PlaylistEditor::allowsSelection()
{
	return true;
}

void PlaylistEditor::reverseSelection()
{
	if (w == Playlists)
		reverseSelectionHelper(Playlists->Begin(), Playlists->End());
	else if (w == Content)
		reverseSelectionHelper(Content->Begin(), Content->End());
}

MPD::SongList PlaylistEditor::getSelectedSongs()
{
	MPD::SongList result;
	if (w == Playlists)
	{
		bool any_selected = false;
		for (auto it = Playlists->Begin(); it != Playlists->End(); ++it)
		{
			if (it->isSelected())
			{
				any_selected = true;
				auto songs = Mpd.GetPlaylistContent(it->value());
				result.insert(result.end(), songs.begin(), songs.end());
			}
		}
		// we don't check for empty result here as it's possible that
		// all selected playlists are empty.
		if (!any_selected && !Content->Empty())
		{
			withUnfilteredMenu(*Content, [this, &result]() {
				result.insert(result.end(), Content->BeginV(), Content->EndV());
			});
		}
	}
	else if (w == Content)
	{
		for (auto it = Content->Begin(); it != Content->End(); ++it)
			if (it->isSelected())
				result.push_back(it->value());
		// if no item is selected, add current one
		if (result.empty() && !Content->Empty())
			result.push_back(Content->Current().value());
	}
	return result;
}

/***********************************************************************/

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

NC::List *PlaylistEditor::GetList()
{
	if (w == Playlists)
		return Playlists;
	else if (w == Content)
		return Content;
	else // silence compiler
	{
		assert(false);
		return 0;
	}
}

namespace {//

std::string SongToString(const MPD::Song &s)
{
	std::string result;
	if (Config.columns_in_playlist_editor)
		result = s.toString(Config.song_in_columns_to_string_format);
	else
		result = s.toString(Config.song_list_format_dollar_free);
	return result;
}

bool PlaylistEntryMatcher(const Regex &rx, const std::string &playlist)
{
	return rx.match(playlist);
}

bool SongEntryMatcher(const Regex &rx, const MPD::Song &s)
{
	return rx.match(SongToString(s));
}

}
