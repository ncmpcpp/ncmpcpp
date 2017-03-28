/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#include <array>
#include <boost/range/detail/any_iterator.hpp>
#include <iomanip>

#include "curses/menu_impl.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "screens/playlist.h"
#include "screens/search_engine.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "format_impl.h"
#include "helpers/song_iterator_maker.h"
#include "utility/comparators.h"
#include "title.h"
#include "screens/screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

namespace ph = std::placeholders;

SearchEngine *mySearcher;

namespace {

/*const std::array<const std::string, 11> constraintsNames = {{
	"Any",
	"Artist",
	"Album Artist",
	"Title",
	"Album",
	"Filename",
	"Composer",
	"Performer",
	"Genre",
	"Date",
	"Comment"
}};

const std::array<const char *, 3> searchModes = {{
	"Match if tag contains searched phrase (no regexes)",
	"Match if tag contains searched phrase (regexes supported)",
	"Match only if both values are the same"
}};

namespace pos {
	const size_t searchIn = constraintsNames.size()-1+1+1; // separated
	const size_t searchMode = searchIn+1;
	const size_t search = searchMode+1+1; // separated
	const size_t reset = search+1;
}*/

std::string SEItemToString(const SEItem &ei);
bool SEItemEntryMatcher(const Regex::Regex &rx,
                        const NC::Menu<SEItem>::Item &item,
                        bool filter);

}

template <>
struct SongPropertiesExtractor<SEItem>
{
	template <typename ItemT>
	auto &operator()(ItemT &item) const
	{
		auto s = !item.isSeparator() && item.value().isSong()
			? &item.value().song()
			: nullptr;
		return m_cache.assign(&item.properties(), s);
	}

private:
	mutable SongProperties m_cache;
};

SongIterator SearchEngineWindow::currentS()
{
	return makeSongIterator(current());
}

ConstSongIterator SearchEngineWindow::currentS() const
{
	return makeConstSongIterator(current());
}

SongIterator SearchEngineWindow::beginS()
{
	return makeSongIterator(begin());
}

ConstSongIterator SearchEngineWindow::beginS() const
{
	return makeConstSongIterator(begin());
}

SongIterator SearchEngineWindow::endS()
{
	return makeSongIterator(end());
}

ConstSongIterator SearchEngineWindow::endS() const
{
	return makeConstSongIterator(end());
}

std::vector<MPD::Song> SearchEngineWindow::getSelectedSongs()
{
	std::vector<MPD::Song> result;
	for (auto &item : *this)
	{
		if (item.isSelected())
		{
			assert(item.value().isSong());
			result.push_back(item.value().song());
		}
	}
	// If no item is selected, add the current one if it's a song.
	if (result.empty() && !empty() && current()->value().isSong())
		result.push_back(current()->value().song());
	return result;
}

/**********************************************************************/

const char *SearchEngine::ConstraintsNames[] =
{
	"Any",
	"Artist",
	"Album Artist",
	"Title",
	"Album",
	"Filename",
	"Composer",
	"Performer",
	"Genre",
	"Date",
	"Comment"
};

const char *SearchEngine::SearchModes[] =
{
	"Match if tag contains searched phrase (no regexes)",
	"Match if tag contains searched phrase (regexes supported)",
	"Match only if both values are the same",
	0
};

size_t SearchEngine::StaticOptions = 20;
size_t SearchEngine::ResetButton = 16;
size_t SearchEngine::SearchButton = 15;

SearchEngine::SearchEngine()
: Screen(NC::Menu<SEItem>(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border()))
{
	setHighlightFixes(w);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer(std::bind(Display::SEItems, ph::_1, std::cref(w)));
	w.setSelectedPrefix(Config.selected_item_prefix);
	w.setSelectedSuffix(Config.selected_item_suffix);
	SearchMode = &SearchModes[Config.search_engine_default_search_mode];
}

void SearchEngine::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	switch (Config.search_engine_display_mode)
	{
		case DisplayMode::Columns:
			if (Config.titles_visibility)
			{
				w.setTitle(Display::Columns(w.getWidth()));
				break;
			}
		case DisplayMode::Classic:
			w.setTitle("");
	}
	hasToBeResized = 0;
}

void SearchEngine::switchTo()
{
	SwitchTo::execute(this);
	if (w.empty())
		Prepare();
	drawHeader();
}

std::wstring SearchEngine::title()
{
	return L"Search engine";
}

void SearchEngine::mouseButtonPressed(MEVENT me)
{
	if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		if (!w.Goto(me.y))
			return;
		w.refresh();
		if ((me.bstate & BUTTON3_PRESSED)
		    && w.choice() < StaticOptions)
			runAction();
		else if (w.choice() >= StaticOptions)
		{
			bool play = me.bstate & BUTTON3_PRESSED;
			addItemToPlaylist(play);
		}
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

/***********************************************************************/

bool SearchEngine::allowsSearching()
{
	return w.rbegin()->value().isSong();
}

const std::string &SearchEngine::searchConstraint()
{
	return m_search_predicate.constraint();
}

void SearchEngine::setSearchConstraint(const std::string &constraint)
{
	m_search_predicate = Regex::ItemFilter<SEItem>(
		constraint,
		Config.regex_type,
		std::bind(SEItemEntryMatcher, ph::_1, ph::_2, false));
}

void SearchEngine::clearSearchConstraint()
{
	m_search_predicate.clear();
}

bool SearchEngine::search(SearchDirection direction, bool wrap, bool skip_current)
{
	return ::search(w, m_search_predicate, direction, wrap, skip_current);
}

/***********************************************************************/

bool SearchEngine::allowsFiltering()
{
	return allowsSearching();
}

std::string SearchEngine::currentFilter()
{
	std::string result;
	if (auto pred = w.filterPredicate<Regex::ItemFilter<SEItem>>())
		result = pred->constraint();
	return result;
}

void SearchEngine::applyFilter(const std::string &constraint)
{
	if (!constraint.empty())
	{
		w.applyFilter(Regex::ItemFilter<SEItem>(
			              constraint,
			              Config.regex_type,
			              std::bind(SEItemEntryMatcher, ph::_1, ph::_2, true)));
	}
	else
		w.clearFilter();
}

/***********************************************************************/

bool SearchEngine::actionRunnable()
{
	return !w.empty() && !w.current()->value().isSong();
}

void SearchEngine::runAction()
{
	size_t option = w.choice();
	if (option > ConstraintsNumber && option < SearchButton)
		w.current()->value().buffer().clear();

	if (option < ConstraintsNumber)
	{
		Statusbar::ScopedLock slock;
		std::string constraint = ConstraintsNames[option];
		Statusbar::put() << NC::Format::Bold << constraint << NC::Format::NoBold << ": ";
		itsConstraints[option] = Global::wFooter->prompt(itsConstraints[option]);
		w.current()->value().buffer().clear();
		constraint.resize(13, ' ');
		w.current()->value().buffer() << NC::Format::Bold << constraint << NC::Format::NoBold << ": ";
		ShowTag(w.current()->value().buffer(), itsConstraints[option]);
	}
	else if (option == ConstraintsNumber+1)
	{
		Config.search_in_db = !Config.search_in_db;
		w.current()->value().buffer() << NC::Format::Bold << "Search in:" << NC::Format::NoBold << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	}
	else if (option == ConstraintsNumber+2)
	{
		if (!*++SearchMode)
			SearchMode = &SearchModes[0];
		w.current()->value().buffer() << NC::Format::Bold << "Search mode:" << NC::Format::NoBold << ' ' << *SearchMode;
	}
	else if (option == SearchButton)
	{
		w.clearFilter();
		Statusbar::print("Searching...");
		if (w.size() > StaticOptions)
			Prepare();
		Search();
		if (w.rbegin()->value().isSong())
		{
			if (Config.search_engine_display_mode == DisplayMode::Columns)
				w.setTitle(Config.titles_visibility ? Display::Columns(w.getWidth()) : "");
			size_t found = w.size()-SearchEngine::StaticOptions;
			found += 3; // don't count options inserted below
			w.insertSeparator(ResetButton+1);
			w.insertItem(ResetButton+2, SEItem(), NC::List::Properties::Inactive);
			w.at(ResetButton+2).value().mkBuffer()
				<< NC::Format::Bold
				<< Config.color1
				<< "Search results: "
				<< NC::FormattedColor::End<>(Config.color1)
				<< Config.color2
				<< "Found " << found << (found > 1 ? " songs" : " song")
				<< NC::FormattedColor::End<>(Config.color2)
				<< NC::Format::NoBold;
			w.insertSeparator(ResetButton+3);
				Statusbar::print("Searching finished");
			if (Config.block_search_constraints_change)
				for (size_t i = 0; i < StaticOptions-4; ++i)
					w.at(i).setInactive(true);
			w.scroll(NC::Scroll::Down);
			w.scroll(NC::Scroll::Down);
		}
		else
			Statusbar::print("No results found");
	}
	else if (option == ResetButton)
	{
		reset();
	}
	else
		addSongToPlaylist(w.current()->value().song(), true);
}

/***********************************************************************/

bool SearchEngine::itemAvailable()
{
	return !w.empty() && w.current()->value().isSong();
}

bool SearchEngine::addItemToPlaylist(bool play)
{
	return addSongToPlaylist(w.current()->value().song(), play);
}

std::vector<MPD::Song> SearchEngine::getSelectedSongs()
{
	return w.getSelectedSongs();
}

/***********************************************************************/

void SearchEngine::Prepare()
{
	w.setTitle("");
	w.clear();
	w.resizeList(StaticOptions-3);

	for (auto &item : w)
		item.setSelectable(false);
	
	w.at(ConstraintsNumber).setSeparator(true);
	w.at(SearchButton-1).setSeparator(true);
	
	for (size_t i = 0; i < ConstraintsNumber; ++i)
	{
		std::string constraint = ConstraintsNames[i];
		constraint.resize(13, ' ');
		w[i].value().mkBuffer() << NC::Format::Bold << constraint << NC::Format::NoBold << ": ";
		ShowTag(w[i].value().buffer(), itsConstraints[i]);
	}
	
	w.at(ConstraintsNumber+1).value().mkBuffer() << NC::Format::Bold << "Search in:" << NC::Format::NoBold << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	w.at(ConstraintsNumber+2).value().mkBuffer() << NC::Format::Bold << "Search mode:" << NC::Format::NoBold << ' ' << *SearchMode;
	
	w.at(SearchButton).value().mkBuffer() << "Search";
	w.at(ResetButton).value().mkBuffer() << "Reset";
}

void SearchEngine::reset()
{
	for (size_t i = 0; i < ConstraintsNumber; ++i)
		itsConstraints[i].clear();
	w.reset();
	Prepare();
	Statusbar::print("Search state reset");
}

void SearchEngine::Search()
{
	bool constraints_empty = 1;
	for (size_t i = 0; i < ConstraintsNumber; ++i)
	{
		if (!itsConstraints[i].empty())
		{
			constraints_empty = 0;
			break;
		}
	}
	if (constraints_empty)
		return;
	
	if (Config.search_in_db && (SearchMode == &SearchModes[0] || SearchMode == &SearchModes[2])) // use built-in mpd searching
	{
		Mpd.StartSearch(SearchMode == &SearchModes[2]);
		if (!itsConstraints[0].empty())
			Mpd.AddSearchAny(itsConstraints[0]);
		if (!itsConstraints[1].empty())
			Mpd.AddSearch(MPD_TAG_ARTIST, itsConstraints[1]);
		if (!itsConstraints[2].empty())
			Mpd.AddSearch(MPD_TAG_ALBUM_ARTIST, itsConstraints[2]);
		if (!itsConstraints[3].empty())
			Mpd.AddSearch(MPD_TAG_TITLE, itsConstraints[3]);
		if (!itsConstraints[4].empty())
			Mpd.AddSearch(MPD_TAG_ALBUM, itsConstraints[4]);
		if (!itsConstraints[5].empty())
			Mpd.AddSearchURI(itsConstraints[5]);
		if (!itsConstraints[6].empty())
			Mpd.AddSearch(MPD_TAG_COMPOSER, itsConstraints[6]);
		if (!itsConstraints[7].empty())
			Mpd.AddSearch(MPD_TAG_PERFORMER, itsConstraints[7]);
		if (!itsConstraints[8].empty())
			Mpd.AddSearch(MPD_TAG_GENRE, itsConstraints[8]);
		if (!itsConstraints[9].empty())
			Mpd.AddSearch(MPD_TAG_DATE, itsConstraints[9]);
		if (!itsConstraints[10].empty())
			Mpd.AddSearch(MPD_TAG_COMMENT, itsConstraints[10]);
		for (MPD::SongIterator s = Mpd.CommitSearchSongs(), end; s != end; ++s)
			w.addItem(std::move(*s));
		return;
	}

	Regex::Regex rx[ConstraintsNumber];
	if (SearchMode != &SearchModes[2]) // match to pattern
	{
		for (size_t i = 0; i < ConstraintsNumber; ++i)
		{
			if (!itsConstraints[i].empty())
			{
				try
				{
					rx[i] = Regex::make(itsConstraints[i], Config.regex_type);
				}
				catch (boost::bad_expression &) { }
			}
		}
	}

	typedef boost::range_detail::any_iterator<
		const MPD::Song,
		boost::single_pass_traversal_tag,
		const MPD::Song &,
		std::ptrdiff_t
	> input_song_iterator;
	input_song_iterator s, end;
	if (Config.search_in_db)
	{
		s = input_song_iterator(getDatabaseIterator(Mpd));
		end = input_song_iterator(MPD::SongIterator());
	}
	else
	{
		s = input_song_iterator(myPlaylist->main().beginV());
		end = input_song_iterator(myPlaylist->main().endV());
	}

	LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
	for (; s != end; ++s)
	{
		bool any_found = true, found = true;

		if (SearchMode != &SearchModes[2]) // match to pattern
		{
			if (!rx[0].empty())
				any_found =
					   Regex::search(s->getArtist(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getAlbumArtist(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getTitle(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getAlbum(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getName(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getComposer(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getPerformer(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getGenre(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getDate(), rx[0], Config.ignore_diacritics)
					|| Regex::search(s->getComment(), rx[0], Config.ignore_diacritics);
			if (found && !rx[1].empty())
				found = Regex::search(s->getArtist(), rx[1], Config.ignore_diacritics);
			if (found && !rx[2].empty())
				found = Regex::search(s->getAlbumArtist(), rx[2], Config.ignore_diacritics);
			if (found && !rx[3].empty())
				found = Regex::search(s->getTitle(), rx[3], Config.ignore_diacritics);
			if (found && !rx[4].empty())
				found = Regex::search(s->getAlbum(), rx[4], Config.ignore_diacritics);
			if (found && !rx[5].empty())
				found = Regex::search(s->getName(), rx[5], Config.ignore_diacritics);
			if (found && !rx[6].empty())
				found = Regex::search(s->getComposer(), rx[6], Config.ignore_diacritics);
			if (found && !rx[7].empty())
				found = Regex::search(s->getPerformer(), rx[7], Config.ignore_diacritics);
			if (found && !rx[8].empty())
				found = Regex::search(s->getGenre(), rx[8], Config.ignore_diacritics);
			if (found && !rx[9].empty())
				found = Regex::search(s->getDate(), rx[9], Config.ignore_diacritics);
			if (found && !rx[10].empty())
				found = Regex::search(s->getComment(), rx[10], Config.ignore_diacritics);
		}
		else // match only if values are equal
		{
			if (!itsConstraints[0].empty())
				any_found =
				   !cmp(s->getArtist(), itsConstraints[0])
				|| !cmp(s->getAlbumArtist(), itsConstraints[0])
				|| !cmp(s->getTitle(), itsConstraints[0])
				|| !cmp(s->getAlbum(), itsConstraints[0])
				|| !cmp(s->getName(), itsConstraints[0])
				|| !cmp(s->getComposer(), itsConstraints[0])
				|| !cmp(s->getPerformer(), itsConstraints[0])
				|| !cmp(s->getGenre(), itsConstraints[0])
				|| !cmp(s->getDate(), itsConstraints[0])
				|| !cmp(s->getComment(), itsConstraints[0]);
			
			if (found && !itsConstraints[1].empty())
				found = !cmp(s->getArtist(), itsConstraints[1]);
			if (found && !itsConstraints[2].empty())
				found = !cmp(s->getAlbumArtist(), itsConstraints[2]);
			if (found && !itsConstraints[3].empty())
				found = !cmp(s->getTitle(), itsConstraints[3]);
			if (found && !itsConstraints[4].empty())
				found = !cmp(s->getAlbum(), itsConstraints[4]);
			if (found && !itsConstraints[5].empty())
				found = !cmp(s->getName(), itsConstraints[5]);
			if (found && !itsConstraints[6].empty())
				found = !cmp(s->getComposer(), itsConstraints[6]);
			if (found && !itsConstraints[7].empty())
				found = !cmp(s->getPerformer(), itsConstraints[7]);
			if (found && !itsConstraints[8].empty())
				found = !cmp(s->getGenre(), itsConstraints[8]);
			if (found && !itsConstraints[9].empty())
				found = !cmp(s->getDate(), itsConstraints[9]);
			if (found && !itsConstraints[10].empty())
				found = !cmp(s->getComment(), itsConstraints[10]);
		}
		
		if (any_found && found)
			w.addItem(*s);
	}
}

namespace {

std::string SEItemToString(const SEItem &ei)
{
	std::string result;
	if (ei.isSong())
	{
		switch (Config.search_engine_display_mode)
		{
			case DisplayMode::Classic:
				result = Format::stringify<char>(Config.song_list_format, &ei.song());
				break;
			case DisplayMode::Columns:
				result = Format::stringify<char>(Config.song_columns_mode_format, &ei.song());
				break;
		}
	}
	else
		result = ei.buffer().str();
	return result;
}

bool SEItemEntryMatcher(const Regex::Regex &rx, const NC::Menu<SEItem>::Item &item, bool filter)
{
	if (item.isSeparator() || !item.value().isSong())
		return filter;
	return Regex::search(SEItemToString(item.value()), rx, Config.ignore_diacritics);
}

}
