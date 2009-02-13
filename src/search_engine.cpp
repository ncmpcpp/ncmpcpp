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

#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "search_engine.h"
#include "settings.h"
#include "status_checker.h"

using namespace MPD;
using namespace Global;
using std::string;

Menu< std::pair<Buffer *, Song *> > *Global::mSearcher;

bool Global::search_match_to_pattern = 1;
bool Global::search_case_sensitive = 0;

const char *Global::search_mode_normal = "Match if tag contains searched phrase";
const char *Global::search_mode_strict = "Match only if both values are the same";

namespace
{
	SearchPattern sought;
}

void SearchEngine::Init()
{
	mSearcher = new Menu< std::pair<Buffer *, Song *> >(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	mSearcher->HighlightColor(Config.main_highlight_color);
	mSearcher->SetTimeout(ncmpcpp_window_timeout);
	mSearcher->SetItemDisplayer(Display::SearchEngine);
	mSearcher->SetSelectPrefix(&Config.selected_item_prefix);
	mSearcher->SetSelectSuffix(&Config.selected_item_suffix);
}

void SearchEngine::Resize()
{
	mSearcher->Resize(COLS, main_height);
}

void SearchEngine::SwitchTo()
{
	if (current_screen != csSearcher
#	ifdef HAVE_TAGLIB_H
	&&  current_screen != csTinyTagEditor
#	endif // HAVE_TAGLIB_H
	   )
	{
		CLEAR_FIND_HISTORY;
		if (mSearcher->Empty())
			PrepareSearchEngine();
		wCurrent = mSearcher;
		wCurrent->Hide();
		current_screen = csSearcher;
//		redraw_screen = 1;
		redraw_header = 1;
		if (!mSearcher->Back().first)
		{
			wCurrent->WriteXY(0, 0, 0, "Updating list...");
			UpdateFoundList();
		}
	}
}

void SearchEngine::EnterPressed()
{
	size_t option = mSearcher->Choice();
	LockStatusbar();
	SearchPattern &s = sought;
	
	if (option <= 12)
		mSearcher->Current().first->Clear();
	
	switch (option)
	{
		case 0:
		{
			Statusbar() << fmtBold << "Any: " << fmtBoldEnd;
			s.Any(wFooter->GetString(s.Any()));
			*mSearcher->Current().first << fmtBold << "Any:      " << fmtBoldEnd << ' ' << ShowTag(s.Any());
			break;
		}
		case 1:
		{
			Statusbar() << fmtBold << "Artist: " << fmtBoldEnd;
			s.SetArtist(wFooter->GetString(s.GetArtist()));
			*mSearcher->Current().first << fmtBold << "Artist:   " << fmtBoldEnd << ' ' << ShowTag(s.GetArtist());
			break;
		}
		case 2:
		{
			Statusbar() << fmtBold << "Title: " << fmtBoldEnd;
			s.SetTitle(wFooter->GetString(s.GetTitle()));
			*mSearcher->Current().first << fmtBold << "Title:    " << fmtBoldEnd << ' ' << ShowTag(s.GetTitle());
			break;
		}
		case 3:
		{
			Statusbar() << fmtBold << "Album: " << fmtBoldEnd;
			s.SetAlbum(wFooter->GetString(s.GetAlbum()));
			*mSearcher->Current().first << fmtBold << "Album:    " << fmtBoldEnd << ' ' << ShowTag(s.GetAlbum());
			break;
		}
		case 4:
		{
			Statusbar() << fmtBold << "Filename: " << fmtBoldEnd;
			s.SetFile(wFooter->GetString(s.GetFile()));
			*mSearcher->Current().first << fmtBold << "Filename: " << fmtBoldEnd << ' ' << ShowTag(s.GetFile());
			break;
		}
		case 5:
		{
			Statusbar() << fmtBold << "Composer: " << fmtBoldEnd;
			s.SetComposer(wFooter->GetString(s.GetComposer()));
			*mSearcher->Current().first << fmtBold << "Composer: " << fmtBoldEnd << ' ' << ShowTag(s.GetComposer());
			break;
		}
		case 6:
		{
			Statusbar() << fmtBold << "Performer: " << fmtBoldEnd;
			s.SetPerformer(wFooter->GetString(s.GetPerformer()));
			*mSearcher->Current().first << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(s.GetPerformer());
			break;
		}
		case 7:
		{
			Statusbar() << fmtBold << "Genre: " << fmtBoldEnd;
			s.SetGenre(wFooter->GetString(s.GetGenre()));
			*mSearcher->Current().first << fmtBold << "Genre:    " << fmtBoldEnd << ' ' << ShowTag(s.GetGenre());
			break;
		}
		case 8:
		{
			Statusbar() << fmtBold << "Year: " << fmtBoldEnd;
			s.SetYear(wFooter->GetString(s.GetYear(), 4));
			*mSearcher->Current().first << fmtBold << "Year:     " << fmtBoldEnd << ' ' << ShowTag(s.GetYear());
			break;
		}
		case 9:
		{
			Statusbar() << fmtBold << "Comment: " << fmtBoldEnd;
			s.SetComment(wFooter->GetString(s.GetComment()));
			*mSearcher->Current().first << fmtBold << "Comment:  " << fmtBoldEnd << ' ' << ShowTag(s.GetComment());
			break;
		}
		case 11:
		{
			Config.search_in_db = !Config.search_in_db;
			*mSearcher->Current().first << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
			break;
		}
		case 12:
		{
			search_match_to_pattern = !search_match_to_pattern;
			*mSearcher->Current().first << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << (search_match_to_pattern ? search_mode_normal : search_mode_strict);
			break;
		}
		case 13:
		{
			search_case_sensitive = !search_case_sensitive;
			*mSearcher->Current().first << fmtBold << "Case sensitive:" << fmtBoldEnd << ' ' << (search_case_sensitive ? "Yes" : "No");
			break;
		}
		case 15:
		{
			ShowMessage("Searching...");
			Search();
			if (!mSearcher->Back().first)
			{
				if (Config.columns_in_search_engine)
					mSearcher->SetTitle(Display::Columns(Config.song_columns_list_format));
				size_t found = mSearcher->Size()-search_engine_static_options;
				found += 3; // don't count options inserted below
				mSearcher->InsertSeparator(search_engine_reset_button+1);
				mSearcher->InsertOption(search_engine_reset_button+2, std::make_pair((Buffer *)0, (Song *)0), 1, 1);
				mSearcher->at(search_engine_reset_button+2).first = new Buffer();
				*mSearcher->at(search_engine_reset_button+2).first << Config.color1 << "Search results: " << Config.color2 << "Found " << found  << (found > 1 ? " songs" : " song") << clDefault;
				mSearcher->InsertSeparator(search_engine_reset_button+3);
				UpdateFoundList();
				ShowMessage("Searching finished!");
				for (size_t i = 0; i < search_engine_static_options-4; i++)
					mSearcher->Static(i, 1);
				mSearcher->Scroll(wDown);
				mSearcher->Scroll(wDown);
			}
			else
				ShowMessage("No results found");
			break;
		}
		case 16:
		{
			CLEAR_FIND_HISTORY;
			PrepareSearchEngine();
			ShowMessage("Search state reset");
			break;
		}
		default:
		{
			block_item_list_update = 1;
			if (Config.ncmpc_like_songs_adding && mSearcher->isBold())
			{
				long long hash = mSearcher->Current().second->GetHash();
				for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
				{
					if (myPlaylist->Main()->at(i).GetHash() == hash)
					{
						Mpd->Play(i);
						break;
					}
				}
			}
			else
			{
				const Song &s = *mSearcher->Current().second;
				int id = Mpd->AddSong(s);
				if (id >= 0)
				{
					Mpd->PlayID(id);
					ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
					mSearcher->BoldOption(mSearcher->Choice(), 1);
				}
			}
			break;
		}
	}
	UnlockStatusbar();
}

void SearchEngine::SpacePressed()
{
	if (mSearcher->Current().first)
		return;
	
	block_item_list_update = 1;
	if (Config.ncmpc_like_songs_adding && mSearcher->isBold())
	{
		block_playlist_update = 1;
		long long hash = mSearcher->Current().second->GetHash();
		for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
		{
			if (myPlaylist->Main()->at(i).GetHash() == hash)
			{
				Mpd->QueueDeleteSong(i);
				myPlaylist->Main()->DeleteOption(i);
				i--;
			}
		}
		Mpd->CommitQueue();
		mSearcher->BoldOption(mSearcher->Choice(), 0);
	}
	else
	{
		const Song &s = *mSearcher->Current().second;
		if (Mpd->AddSong(s) != -1)
		{
			ShowMessage("Added to playlist: %s", s.toString(Config.song_status_format).c_str());
			mSearcher->BoldOption(mSearcher->Choice(), 1);
		}
	}
	mSearcher->Scroll(wDown);
}

void UpdateFoundList()
{
	bool bold = 0;
	for (size_t i = search_engine_static_options; i < mSearcher->Size(); i++)
	{
		for (size_t j = 0; j < myPlaylist->Main()->Size(); j++)
		{
			if (myPlaylist->Main()->at(j).GetHash() == mSearcher->at(i).second->GetHash())
			{
				bold = 1;
				break;
			}
		}
		mSearcher->BoldOption(i, bold);
		bold = 0;
	}
}

void PrepareSearchEngine()
{
	SearchPattern &s = sought;
	
	for (size_t i = 0; i < mSearcher->Size(); i++)
	{
		try
		{
			delete (*mSearcher)[i].first;
			delete (*mSearcher)[i].second;
		}
		catch (List::InvalidItem) { }
	}
	
	s.Clear();
	mSearcher->SetTitle("");
	mSearcher->Clear();
	mSearcher->Reset();
	mSearcher->ResizeBuffer(17);
	
	mSearcher->IntoSeparator(10);
	mSearcher->IntoSeparator(14);
	
	for (size_t i = 0; i < 17; i++)
	{
		try
		{
			mSearcher->at(i).first = new Buffer();
		}
		catch (List::InvalidItem) { }
	}
	
	*mSearcher->at(0).first << fmtBold << "Any:      " << fmtBoldEnd << ' ' << ShowTag(s.Any());
	*mSearcher->at(1).first << fmtBold << "Artist:   " << fmtBoldEnd << ' ' << ShowTag(s.GetArtist());
	*mSearcher->at(2).first << fmtBold << "Title:    " << fmtBoldEnd << ' ' << ShowTag(s.GetTitle());
	*mSearcher->at(3).first << fmtBold << "Album:    " << fmtBoldEnd << ' ' << ShowTag(s.GetAlbum());
	*mSearcher->at(4).first << fmtBold << "Filename: " << fmtBoldEnd << ' ' << ShowTag(s.GetName());
	*mSearcher->at(5).first << fmtBold << "Composer: " << fmtBoldEnd << ' ' << ShowTag(s.GetComposer());
	*mSearcher->at(6).first << fmtBold << "Performer:" << fmtBoldEnd << ' ' << ShowTag(s.GetPerformer());
	*mSearcher->at(7).first << fmtBold << "Genre:    " << fmtBoldEnd << ' ' << ShowTag(s.GetGenre());
	*mSearcher->at(8).first << fmtBold << "Year:     " << fmtBoldEnd << ' ' << ShowTag(s.GetYear());
	*mSearcher->at(9).first << fmtBold << "Comment:  " << fmtBoldEnd << ' ' << ShowTag(s.GetComment());
	
	*mSearcher->at(11).first << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	*mSearcher->at(12).first << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << (search_match_to_pattern ? search_mode_normal : search_mode_strict);
	*mSearcher->at(13).first << fmtBold << "Case sensitive:" << fmtBoldEnd << ' ' << (search_case_sensitive ? "Yes" : "No");
	
	*mSearcher->at(15).first << "Search";
	*mSearcher->at(16).first << "Reset";
}

void Search()
{
	if (sought.Empty())
		return;
	
	SearchPattern s = sought;
	
	SongList list;
	if (Config.search_in_db)
		Mpd->GetDirectoryRecursive("/", list);
	else
	{
		list.reserve(myPlaylist->Main()->Size());
		for (size_t i = 0; i < myPlaylist->Main()->Size(); i++)
			list.push_back(&(*myPlaylist->Main())[i]);
	}
	
	bool any_found = 1;
	bool found = 1;
	
	if (!search_case_sensitive)
	{
		string t;
		t = s.Any();
		ToLower(t);
		s.Any(t);
		
		t = s.GetArtist();
		ToLower(t);
		s.SetArtist(t);
		
		t = s.GetTitle();
		ToLower(t);
		s.SetTitle(t);
		
		t = s.GetAlbum();
		ToLower(t);
		s.SetAlbum(t);
		
		t = s.GetFile();
		ToLower(t);
		s.SetFile(t);
		
		t = s.GetComposer();
		ToLower(t);
		s.SetComposer(t);
		
		t = s.GetPerformer();
		ToLower(t);
		s.SetPerformer(t);
		
		t = s.GetGenre();
		ToLower(t);
		s.SetGenre(t);
		
		t = s.GetComment();
		ToLower(t);
		s.SetComment(t);
	}
	
	for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
	{
		Song copy = **it;
		
		if (!search_case_sensitive)
		{
			string t;
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
		else
			copy.SetFile(copy.GetName());
		
		if (search_match_to_pattern)
		{
			if (!s.Any().empty())
				any_found =
					copy.GetArtist().find(s.Any()) != string::npos
				||	copy.GetTitle().find(s.Any()) != string::npos
				||	copy.GetAlbum().find(s.Any()) != string::npos
				||	copy.GetFile().find(s.Any()) != string::npos
				||	copy.GetComposer().find(s.Any()) != string::npos
				||	copy.GetPerformer().find(s.Any()) != string::npos
				||	copy.GetGenre().find(s.Any()) != string::npos
				||	copy.GetYear().find(s.Any()) != string::npos
				||	copy.GetComment().find(s.Any()) != string::npos;
			
			if (found && !s.GetArtist().empty())
				found = copy.GetArtist().find(s.GetArtist()) != string::npos;
			if (found && !s.GetTitle().empty())
				found = copy.GetTitle().find(s.GetTitle()) != string::npos;
			if (found && !s.GetAlbum().empty())
				found = copy.GetAlbum().find(s.GetAlbum()) != string::npos;
			if (found && !s.GetFile().empty())
				found = copy.GetFile().find(s.GetFile()) != string::npos;
			if (found && !s.GetComposer().empty())
				found = copy.GetComposer().find(s.GetComposer()) != string::npos;
			if (found && !s.GetPerformer().empty())
				found = copy.GetPerformer().find(s.GetPerformer()) != string::npos;
			if (found && !s.GetGenre().empty())
				found = copy.GetGenre().find(s.GetGenre()) != string::npos;
			if (found && !s.GetYear().empty())
				found = copy.GetYear().find(s.GetYear()) != string::npos;
			if (found && !s.GetComment().empty())
				found = copy.GetComment().find(s.GetComment()) != string::npos;
		}
		else
		{
			if (!s.Any().empty())
				any_found =
					copy.GetArtist() == s.Any()
				||	copy.GetTitle() == s.Any()
				||	copy.GetAlbum() == s.Any()
				||	copy.GetFile() == s.Any()
				||	copy.GetComposer() == s.Any()
				||	copy.GetPerformer() == s.Any()
				||	copy.GetGenre() == s.Any()
				||	copy.GetYear() == s.Any()
				||	copy.GetComment() == s.Any();
			
			if (found && !s.GetArtist().empty())
				found = copy.GetArtist() == s.GetArtist();
			if (found && !s.GetTitle().empty())
				found = copy.GetTitle() == s.GetTitle();
			if (found && !s.GetAlbum().empty())
				found = copy.GetAlbum() == s.GetAlbum();
			if (found && !s.GetFile().empty())
				found = copy.GetFile() == s.GetFile();
			if (found && !s.GetComposer().empty())
				found = copy.GetComposer() == s.GetComposer();
			if (found && !s.GetPerformer().empty())
				found = copy.GetPerformer() == s.GetPerformer();
			if (found && !s.GetGenre().empty())
				found = copy.GetGenre() == s.GetGenre();
			if (found && !s.GetYear().empty())
				found = copy.GetYear() == s.GetYear();
			if (found && !s.GetComment().empty())
				found = copy.GetComment() == s.GetComment();
		}
		
		if (found && any_found)
		{
			Song *ss = Config.search_in_db ? *it : new Song(**it);
			mSearcher->AddOption(std::make_pair((Buffer *)0, ss));
			list[it-list.begin()] = 0;
		}
		found = 1;
		any_found = 1;
	}
	if (Config.search_in_db) // free song list only if it's database
		FreeSongList(list);
}

