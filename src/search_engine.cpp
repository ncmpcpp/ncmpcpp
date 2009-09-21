/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

using namespace MPD;
using namespace Global;

SearchEngine *mySearcher = new SearchEngine;

const char *SearchEngine::ConstraintsNames[] =
{
	"Any:",
	"Artist:",
	"Title:",
	"Album:",
	"Filename:",
	"Composer:",
	"Performer:",
	"Genre:",
	"Year:",
	"Comment:"
};

const char *SearchEngine::NormalMode = "Match if tag contains searched phrase (regexes supported)";
const char *SearchEngine::StrictMode = "Match only if both values are the same";

size_t SearchEngine::StaticOptions = 20;
size_t SearchEngine::SearchButton = 15;
size_t SearchEngine::ResetButton = 16;

bool SearchEngine::MatchToPattern = 1;
int SearchEngine::CaseSensitive = REG_ICASE;

void SearchEngine::Init()
{
	w = new Menu< std::pair<Buffer *, Song *> >(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	w->HighlightColor(Config.main_highlight_color);
	w->SetTimeout(ncmpcpp_window_timeout);
	w->CyclicScrolling(Config.use_cyclic_scrolling);
	w->SetItemDisplayer(Display::SearchEngine);
	w->SetSelectPrefix(&Config.selected_item_prefix);
	w->SetSelectSuffix(&Config.selected_item_suffix);
	w->SetGetStringFunction(SearchEngineOptionToString);
	isInitialized = 1;
}

void SearchEngine::Resize()
{
	w->Resize(COLS, MainHeight);
	w->MoveTo(0, MainStartY);
	hasToBeResized = 0;
}

void SearchEngine::SwitchTo()
{
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (hasToBeResized)
		Resize();
	
	if (w->Empty())
		Prepare();
	myScreen = this;
	RedrawHeader = 1;
	
	if (!w->Back().first)
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
	if (option < SearchButton)
		w->Current().first->Clear();
	if (option < 15)
		LockStatusbar();
	
	if (option < 10)
	{
		Statusbar() << fmtBold << ConstraintsNames[option] << fmtBoldEnd << ' ';
		itsConstraints[option] = wFooter->GetString(itsConstraints[option]);
		*w->Current().first << fmtBold << std::setw(10) << std::left << ConstraintsNames[option] << fmtBoldEnd << ' ';
		ShowTag(*w->Current().first, itsConstraints[option]);
	}
	else if (option == 11)
	{
		Config.search_in_db = !Config.search_in_db;
		*w->Current().first << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	}
	else if (option == 12)
	{
		MatchToPattern = !MatchToPattern;
		*w->Current().first << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << (MatchToPattern ? NormalMode : StrictMode);
	}
	else if (option == 13)
	{
		CaseSensitive = !CaseSensitive * REG_ICASE;
		*w->Current().first << fmtBold << "Case sensitive:" << fmtBoldEnd << ' ' << (!CaseSensitive ? "Yes" : "No");
	}
	else if (option == 15)
	{
		ShowMessage("Searching...");
		if (w->Size() > StaticOptions)
			Prepare();
		Search();
		if (!w->Back().first)
		{
			if (Config.columns_in_search_engine)
				w->SetTitle(Display::Columns());
			size_t found = w->Size()-SearchEngine::StaticOptions;
			found += 3; // don't count options inserted below
			w->InsertSeparator(ResetButton+1);
			w->InsertOption(ResetButton+2, std::make_pair(static_cast<Buffer *>(0), static_cast<Song *>(0)), 1, 1);
			w->at(ResetButton+2).first = new Buffer();
			*w->at(ResetButton+2).first << Config.color1 << "Search results: " << Config.color2 << "Found " << found  << (found > 1 ? " songs" : " song") << clDefault;
			w->InsertSeparator(ResetButton+3);
			UpdateFoundList();
			ShowMessage("Searching finished!");
			if (Config.block_search_constraints_change)
				for (size_t i = 0; i < StaticOptions-4; ++i)
					w->Static(i, 1);
			w->Scroll(wDown);
			w->Scroll(wDown);
		}
		else
			ShowMessage("No results found");
	}
	else if (option == 16)
	{
		for (size_t i = 0; i < ConstraintsNumber; ++i)
			itsConstraints[i].clear();
		w->Reset();
		Prepare();
		ShowMessage("Search state reset");
	}
	else
		w->Bold(w->Choice(), myPlaylist->Add(*w->Current().second, w->isBold(), 1));
	
	if (option < 15)
		UnlockStatusbar();
}

void SearchEngine::SpacePressed()
{
	if (w->Current().first)
		return;
	
	if (Config.space_selects)
	{
		w->Select(w->Choice(), !w->isSelected());
		w->Scroll(wDown);
		return;
	}
	
	w->Bold(w->Choice(), myPlaylist->Add(*w->Current().second, w->isBold(), 0));
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
		if ((me.bstate & BUTTON3_PRESSED || w->Choice() > 10) && w->Choice() < StaticOptions)
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
		Screen< Menu< std::pair<Buffer *, MPD::Song *> > >::MouseButtonPressed(me);
}

MPD::Song *SearchEngine::CurrentSong()
{
	return !w->Empty() ? w->Current().second : 0;
}

void SearchEngine::GetSelectedSongs(MPD::SongList &v)
{
	std::vector<size_t> selected;
	w->GetSelected(selected);
	for (std::vector<size_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
		v.push_back(new MPD::Song(*w->at(*it).second));
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
			if (myPlaylist->Items->at(j).GetHash() == w->at(i).second->GetHash())
			{
				bold = 1;
				break;
			}
		}
		w->Bold(i, bold);
		bold = 0;
	}
}

void SearchEngine::Prepare()
{
	for (size_t i = 0; i < w->Size(); ++i)
	{
		if (i == 10 || i == 14 || i == ResetButton+1 || i == ResetButton+3) // separators
			continue;
		delete (*w)[i].first;
		delete (*w)[i].second;
	}
	
	w->SetTitle("");
	w->Clear(0);
	w->ResizeList(17);
	
	w->IntoSeparator(10);
	w->IntoSeparator(14);
	
	for (size_t i = 0; i < 17; ++i)
	{
		if (i == 10 || i == 14) // separators
			continue;
		(*w)[i].first = new Buffer();
	}
	
	for (size_t i = 0; i < ConstraintsNumber; ++i)
	{
		*(*w)[i].first << fmtBold << std::setw(10) << std::left << ConstraintsNames[i] << fmtBoldEnd << ' ';
		ShowTag(*(*w)[i].first, itsConstraints[i]);
	}
	
	*w->at(11).first << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	*w->at(12).first << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << (MatchToPattern ? NormalMode : StrictMode);
	*w->at(13).first << fmtBold << "Case sensitive:" << fmtBoldEnd << ' ' << (!CaseSensitive ? "Yes" : "No");
	
	*w->at(15).first << "Search";
	*w->at(16).first << "Reset";
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
	
	SongList list;
	if (Config.search_in_db)
		Mpd.GetDirectoryRecursive("/", list);
	else
	{
		list.reserve(myPlaylist->Items->Size());
		for (size_t i = 0; i < myPlaylist->Items->Size(); ++i)
			list.push_back(&(*myPlaylist->Items)[i]);
	}
	
	bool any_found = 1;
	bool found = 1;
	
	if (!CaseSensitive && !MatchToPattern)
		for (size_t i = 0; i < ConstraintsNumber; ++i)
			ToLower(itsConstraints[i]);
	
	for (SongList::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		(*it)->CopyPtr(CaseSensitive || MatchToPattern);
		Song copy = **it;
		
		if (!CaseSensitive && !MatchToPattern)
		{
			std::string t;
			t = copy.GetArtist();
			ToLower(t);
			copy.SetArtist(t);
			
			t = copy.GetTitle();
			ToLower(t);
			copy.SetTitle(t);
			
			t = copy.GetAlbum();
			ToLower(t);
			copy.SetAlbum(t);
			
			t = copy.GetName();
			ToLower(t);
			copy.SetFile(t);
			
			t = copy.GetComposer();
			ToLower(t);
			copy.SetComposer(t);
			
			t = copy.GetPerformer();
			ToLower(t);
			copy.SetPerformer(t);
			
			t = copy.GetGenre();
			ToLower(t);
			copy.SetGenre(t);
			
			t = copy.GetComment();
			ToLower(t);
			copy.SetComment(t);
		}
		
		if (MatchToPattern)
		{
			regex_t rx;
			
			if (!itsConstraints[0].empty())
			{
				if (regcomp(&rx, itsConstraints[0].c_str(), CaseSensitive | Config.regex_type) == 0)
				{
					any_found =	!regexec(&rx, copy.GetArtist().c_str(), 0, 0, 0)
					||		!regexec(&rx, copy.GetTitle().c_str(), 0, 0, 0)
					||		!regexec(&rx, copy.GetAlbum().c_str(), 0, 0, 0)
					||		!regexec(&rx, copy.GetName().c_str(), 0, 0, 0)
					||		!regexec(&rx, copy.GetComposer().c_str(), 0, 0, 0)
					||		!regexec(&rx, copy.GetPerformer().c_str(), 0, 0, 0)
					||		!regexec(&rx, copy.GetGenre().c_str(), 0, 0, 0)
					||		!regexec(&rx, copy.GetDate().c_str(), 0, 0, 0)
					||		!regexec(&rx, copy.GetComment().c_str(), 0, 0, 0);
				}
				regfree(&rx);
			}
			
			if (found && !itsConstraints[1].empty())
			{
				if (!regcomp(&rx, itsConstraints[1].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetArtist().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[2].empty())
			{
				if (!regcomp(&rx, itsConstraints[2].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetTitle().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[3].empty())
			{
				if (!regcomp(&rx, itsConstraints[3].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetAlbum().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[4].empty())
			{
				if (!regcomp(&rx, itsConstraints[4].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetName().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[5].empty())
			{
				if (!regcomp(&rx, itsConstraints[5].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetComposer().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[6].empty())
			{
				if (!regcomp(&rx, itsConstraints[6].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetPerformer().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[7].empty())
			{
				if (!regcomp(&rx, itsConstraints[7].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetGenre().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[8].empty())
			{
				if (!regcomp(&rx, itsConstraints[8].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetDate().c_str(), 0, 0, 0);
				regfree(&rx);
			}
			if (found && !itsConstraints[9].empty())
			{
				if (!regcomp(&rx, itsConstraints[9].c_str(), CaseSensitive | Config.regex_type))
					found = !regexec(&rx, copy.GetComment().c_str(), 0, 0, 0);
				regfree(&rx);
			}
		}
		else
		{
			if (!itsConstraints[0].empty())
				any_found =	copy.GetArtist() == itsConstraints[0]
				||		copy.GetTitle() == itsConstraints[0]
				||		copy.GetAlbum() == itsConstraints[0]
				||		copy.GetName() == itsConstraints[0]
				||		copy.GetComposer() == itsConstraints[0]
				||		copy.GetPerformer() == itsConstraints[0]
				||		copy.GetGenre() == itsConstraints[0]
				||		copy.GetDate() == itsConstraints[0]
				||		copy.GetComment() == itsConstraints[0];
			
			if (found && !itsConstraints[1].empty())
				found = copy.GetArtist() == itsConstraints[1];
			if (found && !itsConstraints[2].empty())
				found = copy.GetTitle() == itsConstraints[2];
			if (found && !itsConstraints[3].empty())
				found = copy.GetAlbum() == itsConstraints[3];
			if (found && !itsConstraints[4].empty())
				found = copy.GetName() == itsConstraints[4];
			if (found && !itsConstraints[5].empty())
				found = copy.GetComposer() == itsConstraints[5];
			if (found && !itsConstraints[6].empty())
				found = copy.GetPerformer() == itsConstraints[6];
			if (found && !itsConstraints[7].empty())
				found = copy.GetGenre() == itsConstraints[7];
			if (found && !itsConstraints[8].empty())
				found = copy.GetDate() == itsConstraints[8];
			if (found && !itsConstraints[9].empty())
				found = copy.GetComment() == itsConstraints[9];
		}
		
		if (CaseSensitive || MatchToPattern)
			copy.NullMe();
		(*it)->CopyPtr(0);
		
		if (found && any_found)
		{
			Song *ss = Config.search_in_db ? *it : new Song(**it);
			w->AddOption(std::make_pair(static_cast<Buffer *>(0), ss));
			list[it-list.begin()] = 0;
		}
		found = 1;
		any_found = 1;
	}
	if (Config.search_in_db) // free song list only if it's database
		FreeSongList(list);
}

std::string SearchEngine::SearchEngineOptionToString(const std::pair<Buffer *, MPD::Song *> &pair, void *)
{
	if (!Config.columns_in_search_engine)
		return pair.second->toString(Config.song_list_format);
	else
		return Playlist::SongInColumnsToString(*pair.second, 0);
}

