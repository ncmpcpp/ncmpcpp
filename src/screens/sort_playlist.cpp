/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#include "curses/menu_impl.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "screens/playlist.h"
#include "settings.h"
#include "screens/sort_playlist.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "screens/screen_switcher.h"

SortPlaylistDialog *mySortPlaylistDialog;

SortPlaylistDialog::SortPlaylistDialog()
{
	typedef WindowType::Item::Type Entry;
	
	using Global::MainHeight;
	using Global::MainStartY;
	
	setDimensions();
	w = WindowType((COLS-m_width)/2, (MainHeight-m_height)/2+MainStartY, m_width, m_height, "Sort songs by...", Config.main_color, Config.window_border);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer([](Self::WindowType &menu) {
		menu << Charset::utf8ToLocale(menu.drawn()->value().item().first);
	});
	
	w.addItem(Entry(std::make_pair("Artist", &MPD::Song::getArtist),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Album artist", &MPD::Song::getAlbumArtist),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Album", &MPD::Song::getAlbum),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Disc", &MPD::Song::getDisc),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Track", &MPD::Song::getTrack),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Genre", &MPD::Song::getGenre),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Date", &MPD::Song::getDate),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Composer", &MPD::Song::getComposer),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Performer", &MPD::Song::getPerformer),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Title", &MPD::Song::getTitle),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Filename", &MPD::Song::getURI),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addSeparator();
	w.addItem(Entry(std::make_pair("Sort", static_cast<MPD::Song::GetFunction>(0)),
		std::bind(&Self::sort, this)
	));
	w.addItem(Entry(std::make_pair("Cancel", static_cast<MPD::Song::GetFunction>(0)),
		std::bind(&Self::cancel, this)
	));
}

void SortPlaylistDialog::switchTo()
{
	SwitchTo::execute(this);
	w.reset();
}

void SortPlaylistDialog::resize()
{
	using Global::MainHeight;
	using Global::MainStartY;
	setDimensions();
	w.resize(m_width, m_height);
	w.moveTo((COLS-m_width)/2, (MainHeight-m_height)/2+MainStartY);
	hasToBeResized = false;
}

std::wstring SortPlaylistDialog::title()
{
	return previousScreen()->title();
}

void SortPlaylistDialog::mouseButtonPressed(MEVENT me)
{
	if (w.hasCoords(me.x, me.y))
	{
		if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
		{
			w.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				runAction();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/**********************************************************************/

bool SortPlaylistDialog::actionRunnable()
{
	return !w.empty();
}

void SortPlaylistDialog::runAction()
{
	w.current()->value().run();
}

/**********************************************************************/

void SortPlaylistDialog::moveSortOrderDown()
{
	auto cur = w.currentV();
	if ((cur+1)->item().second)
	{
		std::iter_swap(cur, cur+1);
		w.scroll(NC::Scroll::Down);
	}
}

void SortPlaylistDialog::moveSortOrderUp()
{
	auto cur = w.currentV();
	if (cur > w.beginV() && cur->item().second)
	{
		std::iter_swap(cur, cur-1);
		w.scroll(NC::Scroll::Up);
	}
}

void SortPlaylistDialog::moveSortOrderHint() const
{
	Statusbar::print("Move tag types up and down to adjust sort order");
}

void SortPlaylistDialog::sort() const
{
	auto &pl = myPlaylist->main();
	auto begin = pl.begin(), end = pl.end();
	if (!findSelectedRange(begin, end))
		return;

	size_t start_pos = begin - pl.begin();
	std::vector<MPD::Song> playlist;
	playlist.reserve(end - begin);
	for (; begin != end; ++begin)
		playlist.push_back(begin->value());
	
	typedef std::vector<MPD::Song>::iterator Iterator;
	LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
	std::function<void(Iterator, Iterator)> iter_swap, quick_sort;
	auto song_cmp = [this, &cmp](const MPD::Song &a, const MPD::Song &b) -> bool {
		for (auto it = w.beginV();  it->item().second; ++it)
		{
			int res = cmp(a.getTags(it->item().second),
			              b.getTags(it->item().second));
			if (res != 0)
				return res < 0;
		}
		return a.getPosition() < b.getPosition();
	};
	iter_swap = [&playlist, &start_pos](Iterator a, Iterator b) {
		std::iter_swap(a, b);
		Mpd.Swap(start_pos+a-playlist.begin(), start_pos+b-playlist.begin());
	};
	quick_sort = [&song_cmp, &quick_sort, &iter_swap](Iterator first, Iterator last) {
		if (last-first > 1)
		{
			Iterator pivot = first+Global::RNG()%(last-first);
			iter_swap(pivot, last-1);
			pivot = last-1;
			
			Iterator tmp = first;
			for (Iterator i = first; i != pivot; ++i)
				if (song_cmp(*i, *pivot))
					iter_swap(i, tmp++);
			iter_swap(tmp, pivot);
			
			quick_sort(first, tmp);
			quick_sort(tmp+1, last);
		}
	};
	
	Statusbar::print("Sorting...");
	Mpd.StartCommandsList();
	quick_sort(playlist.begin(), playlist.end());
	Mpd.CommitCommandsList();
	Statusbar::print("Range sorted");
	switchToPreviousScreen();
}

void SortPlaylistDialog::cancel() const
{
	switchToPreviousScreen();
}

void SortPlaylistDialog::setDimensions()
{
	m_height = std::min(size_t(17), Global::MainHeight);
	m_width = 30;
}
