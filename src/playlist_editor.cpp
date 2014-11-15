/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cassert>

#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "mpdpp.h"
#include "status.h"
#include "statusbar.h"
#include "tag_editor.h"
#include "utility/comparators.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

PlaylistEditor *myPlaylistEditor;

namespace {

size_t LeftColumnStartX;
size_t LeftColumnWidth;
size_t RightColumnStartX;
size_t RightColumnWidth;

std::string SongToString(const MPD::Song &s);
bool PlaylistEntryMatcher(const boost::regex &rx, const MPD::Playlist &playlist);
bool SongEntryMatcher(const boost::regex &rx, const MPD::Song &s);

}

PlaylistEditor::PlaylistEditor()
: m_timer(boost::posix_time::from_time_t(0))
, m_window_timeout(Config.data_fetching_delay ? 250 : 500)
, m_fetching_delay(boost::posix_time::milliseconds(Config.data_fetching_delay ? 250 : -1))
{
	LeftColumnWidth = COLS/3-1;
	RightColumnStartX = LeftColumnWidth+1;
	RightColumnWidth = COLS-LeftColumnWidth-1;
	
	Playlists = NC::Menu<MPD::Playlist>(0, MainStartY, LeftColumnWidth, MainHeight, Config.titles_visibility ? "Playlists" : "", Config.main_color, NC::Border());
	Playlists.setHighlightColor(Config.active_column_color);
	Playlists.cyclicScrolling(Config.use_cyclic_scrolling);
	Playlists.centeredCursor(Config.centered_cursor);
	Playlists.setSelectedPrefix(Config.selected_item_prefix);
	Playlists.setSelectedSuffix(Config.selected_item_suffix);
	Playlists.setItemDisplayer([](NC::Menu<MPD::Playlist> &menu) {
		menu << Charset::utf8ToLocale(menu.drawn()->value().path());
	});
	
	Content = NC::Menu<MPD::Song>(RightColumnStartX, MainStartY, RightColumnWidth, MainHeight, Config.titles_visibility ? "Playlist content" : "", Config.main_color, NC::Border());
	Content.setHighlightColor(Config.main_highlight_color);
	Content.cyclicScrolling(Config.use_cyclic_scrolling);
	Content.centeredCursor(Config.centered_cursor);
	Content.setSelectedPrefix(Config.selected_item_prefix);
	Content.setSelectedSuffix(Config.selected_item_suffix);
	switch (Config.playlist_editor_display_mode)
	{
		case DisplayMode::Classic:
			Content.setItemDisplayer(
				boost::bind(Display::Songs, _1, contentProxyList(), Config.song_list_format
			));
			break;
		case DisplayMode::Columns:
			Content.setItemDisplayer(
				boost::bind(Display::SongsInColumns, _1, contentProxyList())
			);
			break;
	}
	
	w = &Playlists;
}

void PlaylistEditor::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	
	LeftColumnStartX = x_offset;
	LeftColumnWidth = width/3-1;
	RightColumnStartX = LeftColumnStartX+LeftColumnWidth+1;
	RightColumnWidth = width-LeftColumnWidth-1;
	
	Playlists.resize(LeftColumnWidth, MainHeight);
	Content.resize(RightColumnWidth, MainHeight);
	
	Playlists.moveTo(LeftColumnStartX, MainStartY);
	Content.moveTo(RightColumnStartX, MainStartY);
	
	hasToBeResized = 0;
}

std::wstring PlaylistEditor::title()
{
	return L"Playlist editor";
}

void PlaylistEditor::refresh()
{
	Playlists.display();
	drawSeparator(RightColumnStartX-1);
	Content.display();
}

void PlaylistEditor::switchTo()
{
	SwitchTo::execute(this);
	markSongsInPlaylist(contentProxyList());
	drawHeader();
	refresh();
}

void PlaylistEditor::update()
{
	if (Playlists.empty() || m_playlists_update_requested)
	{
		m_playlists_update_requested = false;
		size_t idx = 0;
		for (MPD::PlaylistIterator it = Mpd.GetPlaylists(), end; it != end; ++it, ++idx)
		{
			if (idx < Playlists.size())
				Playlists[idx].value() = std::move(*it);
			else
				Playlists.addItem(std::move(*it));
		};
		if (idx < Playlists.size())
			Playlists.resizeList(idx);
		std::sort(Playlists.beginV(), Playlists.endV(),
			LocaleBasedSorting(std::locale(), Config.ignore_leading_the));
		Playlists.refresh();
	}
	
	if ((Content.empty() && Global::Timer - m_timer > m_fetching_delay)
	||  m_content_update_requested)
	{
		m_content_update_requested = false;
		if (Playlists.empty())
			Content.clear();
		else
		{
			size_t idx = 0;
			MPD::SongIterator s = Mpd.GetPlaylistContent(Playlists.current()->value().path()), end;
			for (; s != end; ++s, ++idx)
			{
				bool is_bold = myPlaylist->checkForSong(*s);
				if (idx < Content.size())
				{
					Content[idx].setBold(is_bold);
					Content[idx].value() = std::move(*s);
				}
				else
					Content.addItem(std::move(*s), is_bold);
			}
			if (idx < Content.size())
				Content.resizeList(idx);
			std::string wtitle;
			if (Config.titles_visibility)
			{
				wtitle = (boost::format("Playlist content (%1%) %2%")
					% boost::lexical_cast<std::string>(Content.size())
					% (Content.size() == 1 ? "item" : "items")
					).str();
				wtitle.resize(Content.getWidth());
			}
			Content.setTitle(wtitle);
		}
		Content.display();
	}
	
	if (isActiveWindow(Content) && Content.empty())
	{
		Content.setHighlightColor(Config.main_highlight_color);
		Playlists.setHighlightColor(Config.active_column_color);
		w = &Playlists;
	}
	
	if (Playlists.empty() && Content.empty())
	{
		Content.Window::clear();
		Content.Window::display();
	}
}

int PlaylistEditor::windowTimeout()
{
	if (Content.empty())
		return m_window_timeout;
	else
		return Screen<WindowType>::windowTimeout();
}

ProxySongList PlaylistEditor::contentProxyList()
{
	return ProxySongList(Content, [](NC::Menu<MPD::Song>::Item &item) {
		return &item.value();
	});
}

void PlaylistEditor::AddToPlaylist(bool add_n_play)
{
	if (isActiveWindow(Playlists) && !Playlists.empty())
	{
		std::vector<MPD::Song> list(
			std::make_move_iterator(Mpd.GetPlaylistContent(Playlists.current()->value().path())),
			std::make_move_iterator(MPD::SongIterator())
		);
		bool success = addSongsToPlaylist(list.begin(), list.end(), add_n_play, -1);
		Statusbar::printf("Playlist \"%1%\" loaded%2%",
			Playlists.current()->value().path(), withErrors(success)
		);
	}
	else if (isActiveWindow(Content) && !Content.empty())
		addSongToPlaylist(Content.current()->value(), add_n_play);
	
	if (!add_n_play)
		w->scroll(NC::Scroll::Down);
}

void PlaylistEditor::enterPressed()
{
	AddToPlaylist(true);
}

void PlaylistEditor::spacePressed()
{
	if (Config.space_selects)
	{
		if (isActiveWindow(Playlists))
		{
			if (!Playlists.empty())
			{
				Playlists.current()->setSelected(!Playlists.current()->isSelected());
				Playlists.scroll(NC::Scroll::Down);
			}
		}
		else if (isActiveWindow(Content))
		{
			if (!Content.empty())
			{
				Content.current()->setSelected(!Content.current()->isSelected());
				Content.scroll(NC::Scroll::Down);
			}
		}
	}
	else
		AddToPlaylist(false);
}

void PlaylistEditor::mouseButtonPressed(MEVENT me)
{
	if (!Playlists.empty() && Playlists.hasCoords(me.x, me.y))
	{
		if (!isActiveWindow(Playlists))
		{
			if (previousColumnAvailable())
				previousColumn();
			else
				return;
		}
		if (size_t(me.y) < Playlists.size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Playlists.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
			{
				size_t pos = Playlists.choice();
				spacePressed();
				if (pos < Playlists.size()-1)
					Playlists.scroll(NC::Scroll::Up);
			}
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
		Content.clear();
	}
	else if (!Content.empty() && Content.hasCoords(me.x, me.y))
	{
		if (!isActiveWindow(Content))
		{
			if (nextColumnAvailable())
				nextColumn();
			else
				return;
		}
		if (size_t(me.y) < Content.size() && (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)))
		{
			Content.Goto(me.y);
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = Content.choice();
				spacePressed();
				if (pos < Content.size()-1)
					Content.scroll(NC::Scroll::Up);
			}
			else
				enterPressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/***********************************************************************/

bool PlaylistEditor::allowsSearching()
{
	return true;
}

void PlaylistEditor::setSearchConstraint(const std::string &constraint)
{
	if (isActiveWindow(Playlists))
	{
		m_playlists_search_predicate = RegexFilter<MPD::Playlist>(
			boost::regex(constraint, Config.regex_type),
			PlaylistEntryMatcher
		);
	}
	else if (isActiveWindow(Content))
	{
		m_content_search_predicate = RegexFilter<MPD::Song>(
			boost::regex(constraint, Config.regex_type),
			SongEntryMatcher
		);
	}
}

void PlaylistEditor::clearConstraint()
{
	if (isActiveWindow(Playlists))
		m_playlists_search_predicate.clear();
	else if (isActiveWindow(Content))
		m_content_search_predicate.clear();
}

bool PlaylistEditor::find(SearchDirection direction, bool wrap, bool skip_current)
{
	bool result = false;
	if (isActiveWindow(Playlists))
		result = search(Playlists, m_playlists_search_predicate, direction, wrap, skip_current);
	else if (isActiveWindow(Content))
		result = search(Content, m_content_search_predicate, direction, wrap, skip_current);
	return result;
}

/***********************************************************************/

ProxySongList PlaylistEditor::proxySongList()
{
	auto ptr = ProxySongList();
	if (isActiveWindow(Content))
		ptr = contentProxyList();
	return ptr;
}

bool PlaylistEditor::allowsSelection()
{
	return true;
}

void PlaylistEditor::reverseSelection()
{
	if (isActiveWindow(Playlists))
		reverseSelectionHelper(Playlists.begin(), Playlists.end());
	else if (isActiveWindow(Content))
		reverseSelectionHelper(Content.begin(), Content.end());
}

std::vector<MPD::Song> PlaylistEditor::getSelectedSongs()
{
	std::vector<MPD::Song> result;
	if (isActiveWindow(Playlists))
	{
		bool any_selected = false;
		for (auto &e : Playlists)
		{
			if (e.isSelected())
			{
				any_selected = true;
				std::copy(
					std::make_move_iterator(Mpd.GetPlaylistContent(e.value().path())),
					std::make_move_iterator(MPD::SongIterator()),
					std::back_inserter(result)
				);
			}
		}
		// if no item is selected, add songs from right column
		if (!any_selected && !Playlists.empty())
		{
			std::copy(
				std::make_move_iterator(Mpd.GetPlaylistContent(Playlists.current()->value().path())),
				std::make_move_iterator(MPD::SongIterator()),
				std::back_inserter(result)
			);
		}
	}
	else if (isActiveWindow(Content))
	{
		for (auto &e : Content)
			if (e.isSelected())
				result.push_back(e.value());
		// if no item is selected, add current one
		if (result.empty() && !Content.empty())
			result.push_back(Content.current()->value());
	}
	return result;
}

/***********************************************************************/

bool PlaylistEditor::previousColumnAvailable()
{
	if (isActiveWindow(Content))
	{
		if (!Playlists.empty())
			return true;
	}
	return false;
}

void PlaylistEditor::previousColumn()
{
	if (isActiveWindow(Content))
	{
		Content.setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = &Playlists;
		Playlists.setHighlightColor(Config.active_column_color);
	}
}

bool PlaylistEditor::nextColumnAvailable()
{
	if (isActiveWindow(Playlists))
	{
		if (!Content.empty())
			return true;
	}
	return false;
}

void PlaylistEditor::nextColumn()
{
	if (isActiveWindow(Playlists))
	{
		Playlists.setHighlightColor(Config.main_highlight_color);
		w->refresh();
		w = &Content;
		Content.setHighlightColor(Config.active_column_color);
	}
}

/***********************************************************************/

void PlaylistEditor::updateTimer()
{
	m_timer = Global::Timer;
}

void PlaylistEditor::Locate(const MPD::Playlist &playlist)
{
	update();
	auto begin = Playlists.beginV(), end = Playlists.endV();
	auto it = std::find(begin, end, playlist);
	if (it != end)
	{
		Playlists.highlight(it-begin);
		Content.clear();
		switchTo();
	}
}

namespace {

std::string SongToString(const MPD::Song &s)
{
	std::string result;
	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Classic:
			result = Format::stringify<char>(Config.song_list_format, &s);
			break;
		case DisplayMode::Columns:
			result = Format::stringify<char>(Config.song_columns_mode_format, &s);
			break;
	}
	return result;
}

bool PlaylistEntryMatcher(const boost::regex &rx, const MPD::Playlist &playlist)
{
	return boost::regex_search(playlist.path(), rx);
}

bool SongEntryMatcher(const boost::regex &rx, const MPD::Song &s)
{
	return boost::regex_search(SongToString(s), rx);
}

}
