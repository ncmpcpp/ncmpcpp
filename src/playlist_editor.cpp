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
#include "regex_filter.h"
#include "status.h"
#include "statusbar.h"
#include "tag_editor.h"
#include "utility/comparators.h"
#include "title.h"

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
	Playlists->setHighlightColor(Config.active_column_color);
	Playlists->cyclicScrolling(Config.use_cyclic_scrolling);
	Playlists->centeredCursor(Config.centered_cursor);
	Playlists->setSelectedPrefix(Config.selected_item_prefix);
	Playlists->setSelectedSuffix(Config.selected_item_suffix);
	Playlists->setItemDisplayer(Display::Default<std::string>);
	
	Content = new NC::Menu<MPD::Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Playlist content" : "", Config.main_color, NC::brNone);
	Content->setHighlightColor(Config.main_highlight_color);
	Content->cyclicScrolling(Config.use_cyclic_scrolling);
	Content->centeredCursor(Config.centered_cursor);
	Content->setSelectedPrefix(Config.selected_item_prefix);
	Content->setSelectedSuffix(Config.selected_item_suffix);
	if (Config.columns_in_playlist_editor)
		Content->setItemDisplayer(std::bind(Display::SongsInColumns, _1, this));
	else
		Content->setItemDisplayer(std::bind(Display::Songs, _1, this, Config.song_list_format));
	
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
	
	Playlists->resize(LeftColumnWidth, MainHeight);
	Content->resize(RightColumnWidth, MainHeight);
	
	Playlists->moveTo(LeftColumnStartX, MainStartY);
	Content->moveTo(RightColumnStartX, MainStartY);
	
	hasToBeResized = 0;
}

std::wstring PlaylistEditor::Title()
{
	return L"Playlist editor";
}

void PlaylistEditor::Refresh()
{
	Playlists->display();
	mvvline(MainStartY, RightColumnStartX-1, 0, MainHeight);
	Content->display();
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
	drawHeader();
	markSongsInPlaylist(contentProxyList());
	Refresh();
}

void PlaylistEditor::Update()
{
	if (Playlists->reallyEmpty() || playlistsUpdateRequested)
	{
		playlistsUpdateRequested = false;
		Playlists->clearSearchResults();
		withUnfilteredMenuReapplyFilter(*Playlists, [this]() {
			auto list = Mpd.GetPlaylists();
			std::sort(list.begin(), list.end(),
				LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
			auto playlist = list.begin();
			if (Playlists->size() > list.size())
				Playlists->resizeList(list.size());
			for (auto it = Playlists->begin(); it != Playlists->end(); ++it, ++playlist)
				it->value() = *playlist;
			for (; playlist != list.end(); ++playlist)
				Playlists->addItem(*playlist);
		});
		Playlists->refresh();
	}
	
	if (!Playlists->empty() && (Content->reallyEmpty() || contentUpdateRequested))
	{
		contentUpdateRequested = false;
		Content->clearSearchResults();
		withUnfilteredMenuReapplyFilter(*Content, [this]() {
			auto list = Mpd.GetPlaylistContent(Playlists->current().value());
			auto song = list.begin();
			if (Content->size() > list.size())
				Content->resizeList(list.size());
			for (auto it = Content->begin(); it != Content->end(); ++it, ++song)
			{
				it->value() = *song;
				it->setBold(myPlaylist->checkForSong(*song));
			}
			for (; song != list.end(); ++song)
				Content->addItem(*song, myPlaylist->checkForSong(*song));
			std::string title;
			if (Config.titles_visibility)
			{
				title = "Playlist content";
				title += " (";
				title += unsignedLongIntTo<std::string>::apply(list.size());
				title += " item";
				if (list.size() == 1)
					title += ")";
				else
					title += "s)";
				title.resize(Content->getWidth());
			}
			Content->setTitle(title);
		});
		Content->display();
	}
	
	if (w == Content && Content->reallyEmpty())
	{
		Content->setHighlightColor(Config.main_highlight_color);
		Playlists->setHighlightColor(Config.active_column_color);
		w = Playlists;
	}
	
	if (Playlists->empty() && Content->reallyEmpty())
	{
		Content->Window::clear();
		Content->Window::display();
	}
}

bool PlaylistEditor::isContentFiltered()
{
	if (Content->isFiltered())
	{
		Statusbar::msg("Function currently unavailable due to filtered playlist content");
		return true;
	}
	return false;
}

bool PlaylistEditor::isNextColumnAvailable()
{
	if (w == Playlists)
	{
		if (!Content->reallyEmpty())
			return true;
	}
	return false;
}

bool PlaylistEditor::NextColumn()
{
	if (w == Playlists)
	{
		Playlists->setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = Content;
		Content->setHighlightColor(Config.active_column_color);
		return true;
	}
	return false;
}

bool PlaylistEditor::isPrevColumnAvailable()
{
	if (w == Content)
	{
		if (!Playlists->reallyEmpty())
			return true;
	}
	return false;
}

bool PlaylistEditor::PrevColumn()
{
	if (w == Content)
	{
		Content->setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = Playlists;
		Playlists->setHighlightColor(Config.active_column_color);
		return true;
	}
	return false;
}

std::shared_ptr<ProxySongList> PlaylistEditor::contentProxyList()
{
	return mkProxySongList(*Content, [](NC::Menu<MPD::Song>::Item &item) {
		return &item.value();
	});
}

void PlaylistEditor::AddToPlaylist(bool add_n_play)
{
	MPD::SongList list;
	
	if (w == Playlists && !Playlists->empty())
	{
		if (Mpd.LoadPlaylist(Playlists->current().value()))
		{
			Statusbar::msg("Playlist \"%s\" loaded", Playlists->current().value().c_str());
			if (add_n_play)
				myPlaylist->PlayNewlyAddedSongs();
		}
	}
	else if (w == Content && !Content->empty())
		myPlaylist->Add(Content->current().value(), add_n_play);
	
	if (!add_n_play)
		w->scroll(NC::wDown);
}

void PlaylistEditor::EnterPressed()
{
	AddToPlaylist(true);
}

void PlaylistEditor::SpacePressed()
{
	if (Config.space_selects)
	{
		if (w == Playlists)
		{
			if (!Playlists->empty())
			{
				Playlists->current().setSelected(!Playlists->current().isSelected());
				Playlists->scroll(NC::wDown);
			}
		}
		else if (w == Content)
		{
			if (!Content->empty())
			{
				Content->current().setSelected(!Content->current().isSelected());
				Content->scroll(NC::wDown);
			}
		}
	}
	else
		AddToPlaylist(false);
}

void PlaylistEditor::MouseButtonPressed(MEVENT me)
{
	if (!Playlists->empty() && Playlists->hasCoords(me.x, me.y))
	{
		if (w != Playlists)
			PrevColumn();
		if (size_t(me.y) < Playlists->size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Playlists->Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Playlists->choice();
				SpacePressed();
				if (pos < Playlists->size()-1)
					Playlists->scroll(NC::wUp);
			}
		}
		else
			Screen<NC::Window>::MouseButtonPressed(me);
		Content->clear();
	}
	else if (!Content->empty() && Content->hasCoords(me.x, me.y))
	{
		if (w != Content)
			NextColumn();
		if (size_t(me.y) < Content->size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Content->Goto(me.y);
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = Content->choice();
				SpacePressed();
				if (pos < Content->size()-1)
					Content->scroll(NC::wUp);
			}
			else
				EnterPressed();
		}
		else
			Screen<NC::Window>::MouseButtonPressed(me);
	}
}

/***********************************************************************/

bool PlaylistEditor::allowsFiltering()
{
	return true;
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
		Playlists->showAll();
		auto rx = RegexFilter<std::string>(filter, Config.regex_type, PlaylistEntryMatcher);
		Playlists->filter(Playlists->begin(), Playlists->end(), rx);
	}
	else if (w == Content)
	{
		Content->showAll();
		auto rx = RegexFilter<MPD::Song>(filter, Config.regex_type, SongEntryMatcher);
		Content->filter(Content->begin(), Content->end(), rx);
	}
}

/***********************************************************************/

bool PlaylistEditor::allowsSearching()
{
	return true;
}

bool PlaylistEditor::search(const std::string &constraint)
{
	bool result = false;
	if (w == Playlists)
	{
		auto rx = RegexFilter<std::string>(constraint, Config.regex_type, PlaylistEntryMatcher);
		result = Playlists->search(Playlists->begin(), Playlists->end(), rx);
	}
	else if (w == Content)
	{
		auto rx = RegexFilter<MPD::Song>(constraint, Config.regex_type, SongEntryMatcher);
		result = Content->search(Content->begin(), Content->end(), rx);
	}
	return result;
}

void PlaylistEditor::nextFound(bool wrap)
{
	if (w == Playlists)
		Playlists->nextFound(wrap);
	else if (w == Content)
		Content->nextFound(wrap);
}

void PlaylistEditor::prevFound(bool wrap)
{
	if (w == Playlists)
		Playlists->prevFound(wrap);
	else if (w == Content)
		Content->prevFound(wrap);
}

/***********************************************************************/

std::shared_ptr<ProxySongList> PlaylistEditor::getProxySongList()
{
	auto ptr = nullProxySongList();
	if (w == Content)
		ptr = contentProxyList();
	return ptr;
}

bool PlaylistEditor::allowsSelection()
{
	return true;
}

void PlaylistEditor::reverseSelection()
{
	if (w == Playlists)
		reverseSelectionHelper(Playlists->begin(), Playlists->end());
	else if (w == Content)
		reverseSelectionHelper(Content->begin(), Content->end());
}

MPD::SongList PlaylistEditor::getSelectedSongs()
{
	MPD::SongList result;
	if (w == Playlists)
	{
		bool any_selected = false;
		for (auto it = Playlists->begin(); it != Playlists->end(); ++it)
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
		if (!any_selected && !Content->empty())
		{
			withUnfilteredMenu(*Content, [this, &result]() {
				result.insert(result.end(), Content->beginV(), Content->endV());
			});
		}
	}
	else if (w == Content)
	{
		for (auto it = Content->begin(); it != Content->end(); ++it)
			if (it->isSelected())
				result.push_back(it->value());
		// if no item is selected, add current one
		if (result.empty() && !Content->empty())
			result.push_back(Content->current().value());
	}
	return result;
}

/***********************************************************************/

void PlaylistEditor::Locate(const std::string &name)
{
	if (!isInitialized)
		Init();
	Update();
	for (size_t i = 0; i < Playlists->size(); ++i)
	{
		if (name == (*Playlists)[i].value())
		{
			Playlists->highlight(i);
			Content->clear();
			break;
		}
	}
	SwitchTo();
}

namespace {//

std::string SongToString(const MPD::Song &s)
{
	std::string result;
	if (Config.columns_in_playlist_editor)
		result = s.toString(Config.song_in_columns_to_string_format, Config.tags_separator);
	else
		result = s.toString(Config.song_list_format_dollar_free, Config.tags_separator);
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
