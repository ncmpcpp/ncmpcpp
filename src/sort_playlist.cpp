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

#include "display.h"
#include "global.h"
#include "playlist.h"
#include "settings.h"
#include "sort_playlist.h"
#include "statusbar.h"
#include "utility/comparators.h"

using Global::MainHeight;
using Global::MainStartY;

SortPlaylistDialog *mySortPlaylistDialog;

SortPlaylistDialog::SortPlaylistDialog()
: m_sort_entry(std::make_pair("Sort", static_cast<MPD::Song::GetFunction>(0)))
, m_cancel_entry(std::make_pair("Cancel", static_cast<MPD::Song::GetFunction>(0)))
{
	setDimensions();
	
	w = NC::Menu< std::pair<std::string, MPD::Song::GetFunction> >((COLS-m_width)/2, (MainHeight-m_height)/2+MainStartY, m_width, m_height, "Sort songs by...", Config.main_color, Config.window_border);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer(Display::Pair<std::string, MPD::Song::GetFunction>);
	
	w.addItem(std::make_pair("Artist", &MPD::Song::getArtist));
	w.addItem(std::make_pair("Album", &MPD::Song::getAlbum));
	w.addItem(std::make_pair("Disc", &MPD::Song::getDisc));
	w.addItem(std::make_pair("Track", &MPD::Song::getTrack));
	w.addItem(std::make_pair("Genre", &MPD::Song::getGenre));
	w.addItem(std::make_pair("Date", &MPD::Song::getDate));
	w.addItem(std::make_pair("Composer", &MPD::Song::getComposer));
	w.addItem(std::make_pair("Performer", &MPD::Song::getPerformer));
	w.addItem(std::make_pair("Title", &MPD::Song::getTitle));
	w.addItem(std::make_pair("Filename", &MPD::Song::getURI));
	
	m_sort_options = w.size();
	
	w.addSeparator();
	w.addItem(m_sort_entry);
	w.addItem(m_cancel_entry);
}

void SortPlaylistDialog::switchTo()
{
	using Global::myScreen;
	using Global::myOldScreen;
	using Global::myLockedScreen;
	using Global::myInactiveScreen;
	
	assert(myScreen != this);
	
	if (hasToBeResized)
		resize();
	
	myOldScreen = myScreen;
	myScreen = this;
	
	w.reset();
	refresh();
}

void SortPlaylistDialog::resize()
{
	setDimensions();
	w.resize(m_width, m_height);
	w.moveTo((COLS-m_width)/2, (MainHeight-m_height)/2+MainStartY);
	
	hasToBeResized = false;
}

std::wstring SortPlaylistDialog::title()
{
	return Global::myOldScreen->title();
}

void SortPlaylistDialog::enterPressed()
{
	auto option = w.currentVI();
	if (*option == m_sort_entry)
	{
		auto begin = myPlaylist->main().begin(), end = myPlaylist->main().end();
		// if songs are selected, sort range from first selected to last selected
		if (myPlaylist->main().hasSelected())
		{
			while (!begin->isSelected())
				++begin;
			while (!(end-1)->isSelected())
				--end;
		}
		
		size_t start_pos = begin - myPlaylist->main().begin();
		MPD::SongList playlist;
		playlist.reserve(end - begin);
		for (; begin != end; ++begin)
			playlist.push_back(begin->value());
		
		LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
		std::function<void(MPD::SongList::iterator, MPD::SongList::iterator)> iter_swap, quick_sort;
		auto song_cmp = [this, &cmp](const MPD::Song &a, const MPD::Song &b) -> bool {
			for (size_t i = 0; i < m_sort_options; ++i)
			{
				int res = cmp(a.getTags(w[i].value().second, Config.tags_separator),
				              b.getTags(w[i].value().second, Config.tags_separator));
				if (res != 0)
					return res < 0;
			}
			return a.getPosition() < b.getPosition();
		};
		iter_swap = [&playlist, &start_pos](MPD::SongList::iterator a, MPD::SongList::iterator b) {
			std::iter_swap(a, b);
			Mpd.Swap(start_pos+a-playlist.begin(), start_pos+b-playlist.begin());
		};
		quick_sort = [this, &song_cmp, &quick_sort, &iter_swap](MPD::SongList::iterator first, MPD::SongList::iterator last) {
			if (last-first > 1)
			{
				MPD::SongList::iterator pivot = first+rand()%(last-first);
				iter_swap(pivot, last-1);
				pivot = last-1;
				
				MPD::SongList::iterator tmp = first;
				for (MPD::SongList::iterator i = first; i != pivot; ++i)
					if (song_cmp(*i, *pivot))
						iter_swap(i, tmp++);
					iter_swap(tmp, pivot);
				
				quick_sort(first, tmp);
				quick_sort(tmp+1, last);
			}
		};
		
		Statusbar::msg("Sorting...");
		Mpd.StartCommandsList();
		quick_sort(playlist.begin(), playlist.end());
		if (Mpd.CommitCommandsList())
			Statusbar::msg("Playlist sorted");
		
		Global::myOldScreen->switchTo();
	}
	else if (*option == m_cancel_entry)
	{
		Global::myOldScreen->switchTo();
	}
	else
		Statusbar::msg("Move tag types up and down to adjust sort order");
}

void SortPlaylistDialog::mouseButtonPressed(MEVENT me)
{
	if (w.hasCoords(me.x, me.y))
	{
		if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
		{
			w.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				enterPressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

void SortPlaylistDialog::moveSortOrderDown()
{
	size_t pos = w.choice();
	if (pos < m_sort_options-1)
	{
		w.Swap(pos, pos+1);
		w.scroll(NC::wDown);
	}
}

void SortPlaylistDialog::moveSortOrderUp()
{
	size_t pos = w.choice();
	if (pos > 0 && pos < m_sort_options)
	{
		w.Swap(pos, pos-1);
		w.scroll(NC::wUp);
	}
}

void SortPlaylistDialog::setDimensions()
{
	m_height = std::min(size_t(17), MainHeight);
	m_width = 30;
}
