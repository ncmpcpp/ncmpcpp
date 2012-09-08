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

#include <array>
#include <iomanip>

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "regex_filter.h"
#include "search_engine.h"
#include "settings.h"
#include "status.h"
#include "utility/comparators.h"

using namespace std::placeholders;

using Global::MainHeight;
using Global::MainStartY;

SearchEngine *mySearcher = new SearchEngine;

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
bool SEItemEntryMatcher(const Regex &rx, const NC::Menu<SEItem>::Item &item, bool filter);

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

void SearchEngine::Init()
{
	w = new NC::Menu<SEItem>(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone);
	w->setHighlightColor(Config.main_highlight_color);
	w->cyclicScrolling(Config.use_cyclic_scrolling);
	w->centeredCursor(Config.centered_cursor);
	w->setItemDisplayer(Display::SearchEngine);
	w->setSelectedPrefix(Config.selected_item_prefix);
	w->setSelectedSuffix(Config.selected_item_suffix);
	SearchMode = &SearchModes[Config.search_engine_default_search_mode];
	isInitialized = 1;
}

void SearchEngine::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->resize(width, MainHeight);
	w->moveTo(x_offset, MainStartY);
	w->setTitle(Config.columns_in_search_engine && Config.titles_visibility ? Display::Columns(w->getWidth()) : "");
	hasToBeResized = 0;
}

void SearchEngine::SwitchTo()
{
	using Global::myScreen;
	using Global::myLockedScreen;
	
	if (myScreen == this)
	{
		reset();
		return;
	}
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (w->empty())
		Prepare();

	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	DrawHeader();
	markSongsInPlaylist(getProxySongList());
}

std::wstring SearchEngine::Title()
{
	return L"Search engine";
}

void SearchEngine::EnterPressed()
{
	size_t option = w->choice();
	if (option > ConstraintsNumber && option < SearchButton)
		w->current().value().buffer().clear();
	if (option < SearchButton)
		LockStatusbar();
	
	if (option < ConstraintsNumber)
	{
		std::string constraint = ConstraintsNames[option];
		Statusbar() << NC::fmtBold << constraint << NC::fmtBoldEnd << ": ";
		itsConstraints[option] = Global::wFooter->getString(itsConstraints[option]);
		w->current().value().buffer().clear();
		constraint.resize(13, ' ');
		w->current().value().buffer() << NC::fmtBold << constraint << NC::fmtBoldEnd << ": ";
		ShowTag(w->current().value().buffer(), itsConstraints[option]);
	}
	else if (option == ConstraintsNumber+1)
	{
		Config.search_in_db = !Config.search_in_db;
		w->current().value().buffer() << NC::fmtBold << "Search in:" << NC::fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	}
	else if (option == ConstraintsNumber+2)
	{
		if (!*++SearchMode)
			SearchMode = &SearchModes[0];
		w->current().value().buffer() << NC::fmtBold << "Search mode:" << NC::fmtBoldEnd << ' ' << *SearchMode;
	}
	else if (option == SearchButton)
	{
		w->showAll();
		ShowMessage("Searching...");
		if (w->size() > StaticOptions)
			Prepare();
		Search();
		if (w->back().value().isSong())
		{
			if (Config.columns_in_search_engine)
				w->setTitle(Config.titles_visibility ? Display::Columns(w->getWidth()) : "");
			size_t found = w->size()-SearchEngine::StaticOptions;
			found += 3; // don't count options inserted below
			w->insertSeparator(ResetButton+1);
			w->insertItem(ResetButton+2, SEItem(), 1, 1);
			w->at(ResetButton+2).value().mkBuffer() << Config.color1 << "Search results: " << Config.color2 << "Found " << found << (found > 1 ? " songs" : " song") << NC::clDefault;
			w->insertSeparator(ResetButton+3);
			markSongsInPlaylist(getProxySongList());
			ShowMessage("Searching finished");
			if (Config.block_search_constraints_change)
				for (size_t i = 0; i < StaticOptions-4; ++i)
					w->at(i).setInactive(true);
			w->scroll(NC::wDown);
			w->scroll(NC::wDown);
		}
		else
			ShowMessage("No results found");
	}
	else if (option == ResetButton)
	{
		reset();
	}
	else
		myPlaylist->Add(w->current().value().song(), 1);
	
	if (option < SearchButton)
		UnlockStatusbar();
}

void SearchEngine::SpacePressed()
{
	if (!w->current().value().isSong())
		return;
	
	if (Config.space_selects)
	{
		w->current().setSelected(!w->current().isSelected());
		w->scroll(NC::wDown);
		return;
	}
	
	myPlaylist->Add(w->current().value().song(), 0);
	w->scroll(NC::wDown);
}

void SearchEngine::MouseButtonPressed(MEVENT me)
{
	if (w->empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		if (!w->Goto(me.y))
			return;
		w->refresh();
		if ((me.bstate & BUTTON3_PRESSED || w->choice() > ConstraintsNumber) && w->choice() < StaticOptions)
			EnterPressed();
		else if (w->choice() >= StaticOptions)
		{
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = w->choice();
				SpacePressed();
				if (pos < w->size()-1)
					w->scroll(NC::wUp);
			}
			else
				EnterPressed();
		}
	}
	else
		Screen< NC::Menu<SEItem> >::MouseButtonPressed(me);
}

/***********************************************************************/

bool SearchEngine::allowsFiltering()
{
	return w->back().value().isSong();
}

std::string SearchEngine::currentFilter()
{
	return RegexItemFilter<SEItem>::currentFilter(*w);
}

void SearchEngine::applyFilter(const std::string &filter)
{
	w->showAll();
	auto fun = std::bind(SEItemEntryMatcher, _1, _2, true);
	auto rx = RegexItemFilter<SEItem>(filter, Config.regex_type, fun);
	w->filter(w->begin(), w->end(), rx);
}

/***********************************************************************/

bool SearchEngine::allowsSearching()
{
	return w->back().value().isSong();
}

bool SearchEngine::search(const std::string &constraint)
{
	auto fun = std::bind(SEItemEntryMatcher, _1, _2, false);
	auto rx = RegexItemFilter<SEItem>(constraint, Config.regex_type, fun);
	return w->search(w->begin(), w->end(), rx);
}

void SearchEngine::nextFound(bool wrap)
{
	w->nextFound(wrap);
}

void SearchEngine::prevFound(bool wrap)
{
	w->prevFound(wrap);
}

/***********************************************************************/

std::shared_ptr< ProxySongList > SearchEngine::getProxySongList()
{
	return mkProxySongList(*w, [](NC::Menu<SEItem>::Item &item) -> MPD::Song * {
		MPD::Song *ptr = 0;
		if (!item.isSeparator() && item.value().isSong())
			ptr = &item.value().song();
		return ptr;
	});
}

bool SearchEngine::allowsSelection()
{
	return w->current().value().isSong();
}

void SearchEngine::reverseSelection()
{
	reverseSelectionHelper(w->begin()+std::min(StaticOptions, w->size()), w->end());
}

MPD::SongList SearchEngine::getSelectedSongs()
{
	MPD::SongList result;
	for (auto it = w->begin(); it != w->end(); ++it)
	{
		if (it->isSelected())
		{
			assert(it->value().isSong());
			result.push_back(it->value().song());
		}
	}
	// if no item is selected, add current one
	if (result.empty() && !w->empty())
	{
		assert(w->current().value().isSong());
		result.push_back(w->current().value().song());
	}
	return result;
}

/***********************************************************************/

void SearchEngine::Prepare()
{
	w->setTitle("");
	w->clear();
	w->resizeList(StaticOptions-3);
	
	w->at(ConstraintsNumber).setSeparator(true);
	w->at(SearchButton-1).setSeparator(true);
	
	for (size_t i = 0; i < ConstraintsNumber; ++i)
	{
		std::string constraint = ConstraintsNames[i];
		constraint.resize(13, ' ');
		(*w)[i].value().mkBuffer() << NC::fmtBold << constraint << NC::fmtBoldEnd << ": ";
		ShowTag((*w)[i].value().buffer(), itsConstraints[i]);
	}
	
	w->at(ConstraintsNumber+1).value().mkBuffer() << NC::fmtBold << "Search in:" << NC::fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	w->at(ConstraintsNumber+2).value().mkBuffer() << NC::fmtBold << "Search mode:" << NC::fmtBoldEnd << ' ' << *SearchMode;
	
	w->at(SearchButton).value().mkBuffer() << "Search";
	w->at(ResetButton).value().mkBuffer() << "Reset";
}

void SearchEngine::reset()
{
	for (size_t i = 0; i < ConstraintsNumber; ++i)
		itsConstraints[i].clear();
	w->reset();
	Prepare();
	ShowMessage("Search state reset");
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
		auto songs = Mpd.CommitSearchSongs();
		for (auto s = songs.begin(); s != songs.end(); ++s)
			w->addItem(*s);
		return;
	}
	
	MPD::SongList list;
	if (Config.search_in_db)
		list = Mpd.GetDirectoryRecursive("/");
	else
	{
		list.reserve(myPlaylist->Items->size());
		for (auto s = myPlaylist->Items->beginV(); s != myPlaylist->Items->endV(); ++s)
			list.push_back(*s);
	}
	
	bool any_found = 1;
	bool found = 1;
	
	LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
	for (auto it = list.begin(); it != list.end(); ++it)
	{
		if (SearchMode != &SearchModes[2]) // match to pattern
		{
			regex_t rx;
			if (!itsConstraints[0].empty())
			{
				if (regcomp(&rx, itsConstraints[0].c_str(), REG_ICASE | Config.regex_type) == 0)
				{
					any_found =
						!regexec(&rx, it->getArtist().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getAlbumArtist().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getTitle().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getAlbum().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getName().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getComposer().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getPerformer().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getGenre().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getDate().c_str(), 0, 0, 0)
					||	!regexec(&rx, it->getComment().c_str(), 0, 0, 0);
				}
				regfree(&rx);
			}
			
			if (found && !itsConstraints[1].empty())
			{
				if (!regcomp(&rx, itsConstraints[1].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getArtist().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[2].empty())
			{
				if (!regcomp(&rx, itsConstraints[2].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getAlbumArtist().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[3].empty())
			{
				if(!regcomp(&rx, itsConstraints[3].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getTitle().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[4].empty())
			{
				if (!regcomp(&rx, itsConstraints[4].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getAlbum().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[5].empty())
			{
				if (!regcomp(&rx, itsConstraints[5].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getName().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[6].empty())
			{
				if (!regcomp(&rx, itsConstraints[6].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getComposer().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[7].empty())
			{
				if (!regcomp(&rx, itsConstraints[7].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getPerformer().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[8].empty())
			{
				if (!regcomp(&rx, itsConstraints[8].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getGenre().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[9].empty())
			{
				if (!regcomp(&rx, itsConstraints[9].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getDate().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[10].empty())
			{
				if (!regcomp(&rx, itsConstraints[10].c_str(), REG_ICASE | Config.regex_type))
					found = !regexec(&rx, it->getComment().c_str(), 0, 0, 0);
				regfree(&rx);
			}
		}
		else // match only if values are equal
		{
			if (!itsConstraints[0].empty())
				any_found =
					!cmp(it->getArtist(), itsConstraints[0])
				||	!cmp(it->getAlbumArtist(), itsConstraints[0])
				||	!cmp(it->getTitle(), itsConstraints[0])
				||	!cmp(it->getAlbum(), itsConstraints[0])
				||	!cmp(it->getName(), itsConstraints[0])
				||	!cmp(it->getComposer(), itsConstraints[0])
				||	!cmp(it->getPerformer(), itsConstraints[0])
				||	!cmp(it->getGenre(), itsConstraints[0])
				||	!cmp(it->getDate(), itsConstraints[0])
				||	!cmp(it->getComment(), itsConstraints[0]);
			
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
			w->addItem(*it);
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
		if (Config.columns_in_search_engine)
			result = ei.song().toString(Config.song_in_columns_to_string_format);
		else
			result = ei.song().toString(Config.song_list_format_dollar_free);
	}
	else
		result = ei.buffer().str();
	return result;
}

bool SEItemEntryMatcher(const Regex &rx, const NC::Menu<SEItem>::Item &item, bool filter)
{
	if (item.isSeparator() || !item.value().isSong())
		return filter;
	return rx.match(SEItemToString(item.value()));
}

}
