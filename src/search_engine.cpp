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
#include "helpers.h"
#include "search_engine.h"
#include "settings.h"

using namespace MPD;
using std::string;

extern Connection *Mpd;
extern Menu<Song> *mPlaylist;
extern Menu< std::pair<Buffer *, Song *> > *mSearcher;

bool search_match_to_pattern = 1;
bool search_case_sensitive = 0;

const char *search_mode_normal = "Match if tag contains searched phrase";
const char *search_mode_strict = "Match only if both values are the same";

void UpdateFoundList()
{
	bool bold = 0;
	for (size_t i = search_engine_static_options; i < mSearcher->Size(); i++)
	{
		for (size_t j = 0; j < mPlaylist->Size(); j++)
		{
			if (mPlaylist->at(j).GetHash() == mSearcher->at(i).second->GetHash())
			{
				bold = 1;
				break;
			}
		}
		mSearcher->BoldOption(i, bold);
		bold = 0;
	}
}

void PrepareSearchEngine(SearchPattern &s)
{
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

void Search(SearchPattern s)
{
	if (s.Empty())
		return;
	
	SongList list;
	if (Config.search_in_db)
		Mpd->GetDirectoryRecursive("/", list);
	else
	{
		list.reserve(mPlaylist->Size());
		for (size_t i = 0; i < mPlaylist->Size(); i++)
			list.push_back(&(*mPlaylist)[i]);
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

