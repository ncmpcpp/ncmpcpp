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

#include <array>
#include <boost/bind.hpp>
#include <iomanip>

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "regex_filter.h"
#include "search_engine.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

SearchEngine *mySearcher;

namespace {//

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

namespace pos {//
	const size_t searchIn = constraintsNames.size()-1+1+1; // separated
	const size_t searchMode = searchIn+1;
	const size_t search = searchMode+1+1; // separated
	const size_t reset = search+1;
}*/

std::string SEItemToString(const SEItem &ei);
bool SEItemEntryMatcher(const boost::regex &rx, const NC::Menu<SEItem>::Item &item, bool filter);

}

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
: Screen(NC::Menu<SEItem>(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border::None))
{
	w.setHighlightColor(Config.main_highlight_color);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer(boost::bind(Display::SEItems, _1, proxySongList()));
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
	markSongsInPlaylist(proxySongList());
	drawHeader();
}

std::wstring SearchEngine::title()
{
	return L"Search engine";
}

void SearchEngine::enterPressed()
{
	size_t option = w.choice();
	if (option > ConstraintsNumber && option < SearchButton)
		w.current().value().buffer().clear();
	if (option < SearchButton)
		Statusbar::lock();
	
	if (option < ConstraintsNumber)
	{
		std::string constraint = ConstraintsNames[option];
		Statusbar::put() << NC::Format::Bold << constraint << NC::Format::NoBold << ": ";
		itsConstraints[option] = Global::wFooter->getString(itsConstraints[option]);
		w.current().value().buffer().clear();
		constraint.resize(13, ' ');
		w.current().value().buffer() << NC::Format::Bold << constraint << NC::Format::NoBold << ": ";
		ShowTag(w.current().value().buffer(), itsConstraints[option]);
	}
	else if (option == ConstraintsNumber+1)
	{
		Config.search_in_db = !Config.search_in_db;
		w.current().value().buffer() << NC::Format::Bold << "Search in:" << NC::Format::NoBold << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	}
	else if (option == ConstraintsNumber+2)
	{
		if (!*++SearchMode)
			SearchMode = &SearchModes[0];
		w.current().value().buffer() << NC::Format::Bold << "Search mode:" << NC::Format::NoBold << ' ' << *SearchMode;
	}
	else if (option == SearchButton)
	{
		w.showAll();
		Statusbar::print("Searching...");
		if (w.size() > StaticOptions)
			Prepare();
		Search();
		if (w.back().value().isSong())
		{
			if (Config.search_engine_display_mode == DisplayMode::Columns)
				w.setTitle(Config.titles_visibility ? Display::Columns(w.getWidth()) : "");
			size_t found = w.size()-SearchEngine::StaticOptions;
			found += 3; // don't count options inserted below
			w.insertSeparator(ResetButton+1);
			w.insertItem(ResetButton+2, SEItem(), 1, 1);
			w.at(ResetButton+2).value().mkBuffer() << Config.color1 << "Search results: " << Config.color2 << "Found " << found << (found > 1 ? " songs" : " song") << NC::Color::Default;
			w.insertSeparator(ResetButton+3);
			markSongsInPlaylist(proxySongList());
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
		addSongToPlaylist(w.current().value().song(), true);
	
	if (option < SearchButton)
		Statusbar::unlock();
}

void SearchEngine::spacePressed()
{
	if (!w.current().value().isSong())
		return;
	
	if (Config.space_selects)
	{
		w.current().setSelected(!w.current().isSelected());
		w.scroll(NC::Scroll::Down);
		return;
	}
	
	addSongToPlaylist(w.current().value().song(), false);
	w.scroll(NC::Scroll::Down);
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
		if ((me.bstate & BUTTON3_PRESSED || w.choice() > ConstraintsNumber) && w.choice() < StaticOptions)
			enterPressed();
		else if (w.choice() >= StaticOptions)
		{
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = w.choice();
				spacePressed();
				if (pos < w.size()-1)
					w.scroll(NC::Scroll::Up);
			}
			else
				enterPressed();
		}
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

/***********************************************************************/

bool SearchEngine::allowsFiltering()
{
	return w.back().value().isSong();
}

std::string SearchEngine::currentFilter()
{
	return RegexItemFilter<SEItem>::currentFilter(w);
}

void SearchEngine::applyFilter(const std::string &filter)
{
	if (filter.empty())
	{
		w.clearFilter();
		w.clearFilterResults();
		return;
	}
	try
	{
		w.showAll();
		auto fun = boost::bind(SEItemEntryMatcher, _1, _2, true);
		auto rx = RegexItemFilter<SEItem>(
			boost::regex(filter, Config.regex_type), fun);
		w.filter(w.begin(), w.end(), rx);
	}
	catch (boost::bad_expression &) { }
}

/***********************************************************************/

bool SearchEngine::allowsSearching()
{
	return w.back().value().isSong();
}

bool SearchEngine::search(const std::string &constraint)
{
	if (constraint.empty())
	{
		w.clearSearchResults();
		return false;
	}
	try
	{
		auto fun = boost::bind(SEItemEntryMatcher, _1, _2, false);
		auto rx = RegexItemFilter<SEItem>(
			boost::regex(constraint, Config.regex_type), fun);
		return w.search(w.begin(), w.end(), rx);
	}
	catch (boost::bad_expression &)
	{
		return false;
	}
}

void SearchEngine::nextFound(bool wrap)
{
	w.nextFound(wrap);
}

void SearchEngine::prevFound(bool wrap)
{
	w.prevFound(wrap);
}

/***********************************************************************/

ProxySongList SearchEngine::proxySongList()
{
	return ProxySongList(w, [](NC::Menu<SEItem>::Item &item) -> MPD::Song * {
		MPD::Song *ptr = 0;
		if (!item.isSeparator() && item.value().isSong())
			ptr = &item.value().song();
		return ptr;
	});
}

bool SearchEngine::allowsSelection()
{
	return w.current().value().isSong();
}

void SearchEngine::reverseSelection()
{
	reverseSelectionHelper(w.begin()+std::min(StaticOptions, w.size()), w.end());
}

MPD::SongList SearchEngine::getSelectedSongs()
{
	MPD::SongList result;
	for (auto it = w.begin(); it != w.end(); ++it)
	{
		if (it->isSelected())
		{
			assert(it->value().isSong());
			result.push_back(it->value().song());
		}
	}
	// if no item is selected, add current one
	if (result.empty() && !w.empty())
	{
		assert(w.current().value().isSong());
		result.push_back(w.current().value().song());
	}
	return result;
}

/***********************************************************************/

void SearchEngine::Prepare()
{
	w.setTitle("");
	w.clear();
	w.resizeList(StaticOptions-3);
	
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
		Mpd.CommitSearchSongs([this](MPD::Song s) {
			w.addItem(s);
		});
		return;
	}
	
	MPD::SongList list;
	if (Config.search_in_db)
		Mpd.GetDirectoryRecursive("/", vectorMoveInserter(list));
	else
		list.insert(list.end(), myPlaylist->main().beginV(), myPlaylist->main().endV());
	
	bool any_found = 1;
	bool found = 1;
	
	LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
	for (auto it = list.begin(); it != list.end(); ++it)
	{
		if (SearchMode != &SearchModes[2]) // match to pattern
		{
			boost::regex rx;
			if (!itsConstraints[0].empty())
			{
				try
				{
					rx.assign(itsConstraints[0], Config.regex_type);
					any_found =
					   boost::regex_search(it->getArtist(), rx)
					|| boost::regex_search(it->getAlbumArtist(), rx)
					|| boost::regex_search(it->getTitle(), rx)
					|| boost::regex_search(it->getAlbum(), rx)
					|| boost::regex_search(it->getName(), rx)
					|| boost::regex_search(it->getComposer(), rx)
					|| boost::regex_search(it->getPerformer(), rx)
					|| boost::regex_search(it->getGenre(), rx)
					|| boost::regex_search(it->getDate(), rx)
					|| boost::regex_search(it->getComment(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			
			if (found && !itsConstraints[1].empty())
			{
				try
				{
					rx.assign(itsConstraints[1], Config.regex_type);
					found = boost::regex_search(it->getArtist(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && !itsConstraints[2].empty())
			{
				try
				{
					rx.assign(itsConstraints[2], Config.regex_type);
					found = boost::regex_search(it->getAlbumArtist(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && !itsConstraints[3].empty())
			{
				try
				{
					rx.assign(itsConstraints[3], Config.regex_type);
					found = boost::regex_search(it->getTitle(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && !itsConstraints[4].empty())
			{
				try
				{
					rx.assign(itsConstraints[4], Config.regex_type);
					found = boost::regex_search(it->getAlbum(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && !itsConstraints[5].empty())
			{
				try
				{
					rx.assign(itsConstraints[5], Config.regex_type);
					found = boost::regex_search(it->getName(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && !itsConstraints[6].empty())
			{
				try
				{
					rx.assign(itsConstraints[6], Config.regex_type);
					found = boost::regex_search(it->getComposer(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && !itsConstraints[7].empty())
			{
				try
				{
					rx.assign(itsConstraints[7], Config.regex_type);
					found = boost::regex_search(it->getPerformer(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && itsConstraints[8].empty())
			{
				try
				{
					rx.assign(itsConstraints[8], Config.regex_type);
					found = boost::regex_search(it->getGenre(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && !itsConstraints[9].empty())
			{
				try
				{
					rx.assign(itsConstraints[9], Config.regex_type);
					found = boost::regex_search(it->getDate(), rx);
				}
				catch (boost::bad_expression &) { }
			}
			if (found && !itsConstraints[10].empty())
			{
				try
				{
					rx.assign(itsConstraints[10], Config.regex_type);
					found = boost::regex_search(it->getComment(), rx);
				}
				catch (boost::bad_expression &) { }
			}
		}
		else // match only if values are equal
		{
			if (!itsConstraints[0].empty())
				any_found =
				   !cmp(it->getArtist(), itsConstraints[0])
				|| !cmp(it->getAlbumArtist(), itsConstraints[0])
				|| !cmp(it->getTitle(), itsConstraints[0])
				|| !cmp(it->getAlbum(), itsConstraints[0])
				|| !cmp(it->getName(), itsConstraints[0])
				|| !cmp(it->getComposer(), itsConstraints[0])
				|| !cmp(it->getPerformer(), itsConstraints[0])
				|| !cmp(it->getGenre(), itsConstraints[0])
				|| !cmp(it->getDate(), itsConstraints[0])
				|| !cmp(it->getComment(), itsConstraints[0]);
			
			if (found && !itsConstraints[1].empty())
				found = !cmp(it->getArtist(), itsConstraints[1]);
			if (found && !itsConstraints[2].empty())
				found = !cmp(it->getAlbumArtist(), itsConstraints[2]);
			if (found && !itsConstraints[3].empty())
				found = !cmp(it->getTitle(), itsConstraints[3]);
			if (found && !itsConstraints[4].empty())
				found = !cmp(it->getAlbum(), itsConstraints[4]);
			if (found && !itsConstraints[5].empty())
				found = !cmp(it->getName(), itsConstraints[5]);
			if (found && !itsConstraints[6].empty())
				found = !cmp(it->getComposer(), itsConstraints[6]);
			if (found && !itsConstraints[7].empty())
				found = !cmp(it->getPerformer(), itsConstraints[7]);
			if (found && !itsConstraints[8].empty())
				found = !cmp(it->getGenre(), itsConstraints[8]);
			if (found && !itsConstraints[9].empty())
				found = !cmp(it->getDate(), itsConstraints[9]);
			if (found && !itsConstraints[10].empty())
				found = !cmp(it->getComment(), itsConstraints[10]);
		}
		
		if (found && any_found)
			w.addItem(*it);
		found = 1;
		any_found = 1;
	}
}

namespace {//

std::string SEItemToString(const SEItem &ei)
{
	std::string result;
	if (ei.isSong())
	{
		switch (Config.search_engine_display_mode)
		{
			case DisplayMode::Classic:
				result = ei.song().toString(Config.song_list_format_dollar_free, Config.tags_separator);
				break;
			case DisplayMode::Columns:
				result = ei.song().toString(Config.song_in_columns_to_string_format, Config.tags_separator);
				break;
		}
	}
	else
		result = ei.buffer().str();
	return result;
}

bool SEItemEntryMatcher(const boost::regex &rx, const NC::Menu<SEItem>::Item &item, bool filter)
{
	if (item.isSeparator() || !item.value().isSong())
		return filter;
	return boost::regex_search(SEItemToString(item.value()), rx);
}

}
