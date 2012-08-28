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

#include <iomanip>

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "search_engine.h"
#include "settings.h"
#include "status.h"

using Global::MainHeight;
using Global::MainStartY;

SearchEngine *mySearcher = new SearchEngine;

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
	static Display::ScreenFormat sf = { this, &Config.song_list_format };
	
	w = new Menu<SEItem>(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	w->HighlightColor(Config.main_highlight_color);
	w->CyclicScrolling(Config.use_cyclic_scrolling);
	w->CenteredCursor(Config.centered_cursor);
	w->SetItemDisplayer(Display::SearchEngine);
	w->SetItemDisplayerUserData(&sf);
	w->SetSelectPrefix(&Config.selected_item_prefix);
	w->SetSelectSuffix(&Config.selected_item_suffix);
	w->SetGetStringFunction(SearchEngineOptionToString);
	SearchMode = &SearchModes[Config.search_engine_default_search_mode];
	isInitialized = 1;
}

void SearchEngine::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->Resize(width, MainHeight);
	w->MoveTo(x_offset, MainStartY);
	w->SetTitle(Config.columns_in_search_engine && Config.titles_visibility ? Display::Columns(w->GetWidth()) : "");
	hasToBeResized = 0;
}

void SearchEngine::SwitchTo()
{
	using Global::myScreen;
	using Global::myLockedScreen;
	
	if (myScreen == this)
	{
		Reset();
		return;
	}
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (w->Empty())
		Prepare();

	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	Global::RedrawHeader = true;
	
	if (!w->Back().isSong())
	{
		*w << XY(0, 0) << "Updating list...";
		UpdateFoundList();
	}
}

std::basic_string<my_char_t> SearchEngine::Title()
{
	return U("Search engine");
}

void SearchEngine::EnterPressed()
{
	size_t option = w->Choice();
	if (option > ConstraintsNumber && option < SearchButton)
		w->Current().buffer().Clear();
	if (option < SearchButton)
		LockStatusbar();
	
	if (option < ConstraintsNumber)
	{
		Statusbar() << fmtBold << ConstraintsNames[option] << fmtBoldEnd << ": ";
		itsConstraints[option] = Global::wFooter->GetString(itsConstraints[option]);
		w->Current().buffer().Clear();
		w->Current().buffer() << fmtBold << std::setw(13) << std::left << ConstraintsNames[option] << fmtBoldEnd << ": ";
		ShowTag(w->Current().buffer(), itsConstraints[option]);
	}
	else if (option == ConstraintsNumber+1)
	{
		Config.search_in_db = !Config.search_in_db;
		w->Current().buffer() << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	}
	else if (option == ConstraintsNumber+2)
	{
		if (!*++SearchMode)
			SearchMode = &SearchModes[0];
		w->Current().buffer() << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << *SearchMode;
	}
	else if (option == SearchButton)
	{
		ShowMessage("Searching...");
		if (w->Size() > StaticOptions)
			Prepare();
		Search();
		if (w->Back().isSong())
		{
			if (Config.columns_in_search_engine)
				w->SetTitle(Config.titles_visibility ? Display::Columns(w->GetWidth()) : "");
			size_t found = w->Size()-SearchEngine::StaticOptions;
			found += 3; // don't count options inserted below
			w->InsertSeparator(ResetButton+1);
			w->InsertOption(ResetButton+2, SEItem(), 1, 1);
			w->at(ResetButton+2).mkBuffer() << Config.color1 << "Search results: " << Config.color2 << "Found " << found  << (found > 1 ? " songs" : " song") << clDefault;
			w->InsertSeparator(ResetButton+3);
			UpdateFoundList();
			ShowMessage("Searching finished");
			if (Config.block_search_constraints_change)
				for (size_t i = 0; i < StaticOptions-4; ++i)
					w->Static(i, 1);
			w->Scroll(wDown);
			w->Scroll(wDown);
		}
		else
			ShowMessage("No results found");
	}
	else if (option == ResetButton)
	{
		Reset();
	}
	else
	{
		bool res = myPlaylist->Add(w->Current().song(), 1, 1);
		w->Bold(w->Choice(), res);
	}
	
	if (option < SearchButton)
		UnlockStatusbar();
}

void SearchEngine::SpacePressed()
{
	if (!w->Current().isSong())
		return;
	
	if (Config.space_selects)
	{
		w->Select(w->Choice(), !w->isSelected());
		w->Scroll(wDown);
		return;
	}
	
	bool res = myPlaylist->Add(w->Current().song(), 0, 0);
	w->Bold(w->Choice(), res);
	w->Scroll(wDown);
}

void SearchEngine::MouseButtonPressed(MEVENT me)
{
	if (w->Empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->Size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		if (!w->Goto(me.y))
			return;
		w->Refresh();
		if ((me.bstate & BUTTON3_PRESSED || w->Choice() > ConstraintsNumber) && w->Choice() < StaticOptions)
			EnterPressed();
		else if (w->Choice() >= StaticOptions)
		{
			if (me.bstate & BUTTON1_PRESSED)
			{
				size_t pos = w->Choice();
				SpacePressed();
				if (pos < w->Size()-1)
					w->Scroll(wUp);
			}
			else
				EnterPressed();
		}
	}
	else
		Screen< Menu<SEItem> >::MouseButtonPressed(me);
}

MPD::Song *SearchEngine::CurrentSong()
{
	return !w->Empty() && w->Current().isSong() ? &w->Current().song() : 0;
}

void SearchEngine::GetSelectedSongs(MPD::SongList &v)
{
	if (w->Empty())
		return;
	std::vector<size_t> selected;
	w->GetSelected(selected);
	if (selected.empty() && w->Choice() >= StaticOptions)
		selected.push_back(w->Choice());
	for (auto it = selected.begin(); it != selected.end(); ++it)
	{
		assert(w->at(*it).isSong());
		v.push_back(w->at(*it).song());
	}
}

void SearchEngine::ApplyFilter(const std::string &s)
{
	w->ApplyFilter(s, StaticOptions, REG_ICASE | Config.regex_type);
}

void SearchEngine::UpdateFoundList()
{
	bool bold = 0;
	for (size_t i = StaticOptions; i < w->Size(); ++i)
	{
		for (size_t j = 0; j < myPlaylist->Items->Size(); ++j)
		{
			if (myPlaylist->Items->at(j).getHash() == w->at(i).song().getHash())
			{
				bold = 1;
				break;
			}
		}
		w->Bold(i, bold);
		bold = 0;
	}
}

void SearchEngine::SelectAlbum()
{
	size_t pos = w->Choice();
	if (pos < StaticOptions)
		return;		// not on a song
	
	std::string album = w->at(pos).song().getAlbum();
	
	// select song under cursor
	w->Select(pos, 1);
	
	// go up
	while (pos > StaticOptions)
	{
		if (w->at(--pos).song().getAlbum() != album)
			break;
		else
			w->Select(pos, 1);
	}
	
	// go down
	while (pos < w->Size() - 1)
	{
		if (w->at(++pos).song().getAlbum() != album)
			break;
		else
			w->Select(pos, 1);
	}
}

void SearchEngine::Prepare()
{
	w->SetTitle("");
	w->Clear();
	w->ResizeList(StaticOptions-3);
	
	w->IntoSeparator(ConstraintsNumber);
	w->IntoSeparator(SearchButton-1);
	
	for (size_t i = 0; i < ConstraintsNumber; ++i)
	{
		(*w)[i].mkBuffer() << fmtBold << std::setw(13) << std::left << ConstraintsNames[i] << fmtBoldEnd << ": ";
		ShowTag((*w)[i].buffer(), itsConstraints[i]);
	}
	
	w->at(ConstraintsNumber+1).mkBuffer() << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	w->at(ConstraintsNumber+2).mkBuffer() << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << *SearchMode;
	
	w->at(SearchButton).mkBuffer() << "Search";
	w->at(ResetButton).mkBuffer() << "Reset";
}

void SearchEngine::Reset()
{
	for (size_t i = 0; i < ConstraintsNumber; ++i)
		itsConstraints[i].clear();
	w->Reset();
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
		MPD::SongList results;
		Mpd.CommitSearch(results);
		for (auto it = results.begin(); it != results.end(); ++it)
			w->AddOption(*it);
		return;
	}
	
	MPD::SongList list;
	if (Config.search_in_db)
		Mpd.GetDirectoryRecursive("/", list);
	else
	{
		list.reserve(myPlaylist->Items->Size());
		for (size_t i = 0; i < myPlaylist->Items->Size(); ++i)
			list.push_back((*myPlaylist->Items)[i]);
	}
	
	bool any_found = 1;
	bool found = 1;
	
	for (MPD::SongList::const_iterator it = list.begin(); it != list.end(); ++it)
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
			CaseInsensitiveStringComparison cmp;
			
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
		{
			w->AddOption(*it);
			list[it-list.begin()] = 0;
		}
		found = 1;
		any_found = 1;
	}
}

std::string SearchEngine::SearchEngineOptionToString(const SEItem &ei, void *)
{
	if (!ei.isSong())
	{
		if (!Config.columns_in_search_engine)
			return ei.song().toString(Config.song_list_format_dollar_free);
		else
			return Playlist::SongInColumnsToString(ei.song(), 0);
	}
	else
		return "";
}
