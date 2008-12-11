/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "helpers.h"
#include "search_engine.h"

using namespace MPD;

extern Connection *Mpd;
extern Menu<Song> *mPlaylist;
extern Menu< std::pair<Buffer *, Song *> > *mSearcher;

bool search_match_to_pattern = 1;
bool search_case_sensitive = 0;

extern const char *search_mode_normal = "Match if tag contains searched phrase";
extern const char *search_mode_strict = "Match only if both values are the same";

void SearchEngineDisplayer(const std::pair<Buffer *, Song *> &pair, void *, Menu< std::pair<Buffer *, Song *> > *menu)
{
	if (pair.second)
		DisplaySong(*pair.second, &Config.song_list_format, reinterpret_cast<Menu<Song> *>(menu));
	else
		*menu << *pair.first;
}

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

void PrepareSearchEngine(Song &s)
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
	mSearcher->Clear();
	mSearcher->Reset();
	mSearcher->ResizeBuffer(15);
	
	mSearcher->IntoSeparator(8);
	mSearcher->IntoSeparator(12);
	
	for (size_t i = 0; i < 15; i++)
	{
		try
		{
			mSearcher->at(i).first = new Buffer();
		}
		catch (List::InvalidItem) { }
	}
	
	*mSearcher->at(0).first << fmtBold << "Filename:" << fmtBoldEnd << ' ' << ShowTag(s.GetName());
	*mSearcher->at(1).first << fmtBold << "Title:" << fmtBoldEnd << ' ' << ShowTag(s.GetTitle());
	*mSearcher->at(2).first << fmtBold << "Artist:" << fmtBoldEnd << ' ' << ShowTag(s.GetArtist());
	*mSearcher->at(3).first << fmtBold << "Album:" << fmtBoldEnd << ' ' << ShowTag(s.GetAlbum());
	*mSearcher->at(4).first << fmtBold << "Year:" << fmtBoldEnd << ' ' << ShowTag(s.GetYear());
	*mSearcher->at(5).first << fmtBold << "Track:" << fmtBoldEnd << ' ' << ShowTag(s.GetTrack());
	*mSearcher->at(6).first << fmtBold << "Genre:" << fmtBoldEnd << ' ' << ShowTag(s.GetGenre());
	*mSearcher->at(7).first << fmtBold << "Comment:" << fmtBoldEnd << ' ' << ShowTag(s.GetComment());
	
	*mSearcher->at(9).first << fmtBold << "Search in:" << fmtBoldEnd << ' ' << (Config.search_in_db ? "Database" : "Current playlist");
	*mSearcher->at(10).first << fmtBold << "Search mode:" << fmtBoldEnd << ' ' << (search_match_to_pattern ? search_mode_normal : search_mode_strict);
	*mSearcher->at(11).first << fmtBold << "Case sensitive:" << fmtBoldEnd << ' ' << (search_case_sensitive ? "Yes" : "No");
	
	*mSearcher->at(13).first << "Search";
	*mSearcher->at(14).first << "Reset";
}

void Search(Song &s)
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
	
	bool found = 1;
	
	if (!search_case_sensitive)
	{
		string t;
		t = s.GetFile();
		ToLower(t);
		s.SetFile(t);
		
		t = s.GetTitle();
		ToLower(t);
		s.SetTitle(t);
		
		t = s.GetArtist();
		ToLower(t);
		s.SetArtist(t);
		
		t = s.GetAlbum();
		ToLower(t);
		s.SetAlbum(t);
		
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
			t = copy.GetName();
			ToLower(t);
			copy.SetFile(t);
			
			t = copy.GetTitle();
			ToLower(t);
			copy.SetTitle(t);
			
			t = copy.GetArtist();
			ToLower(t);
			copy.SetArtist(t);
			
			t = copy.GetAlbum();
			ToLower(t);
			copy.SetAlbum(t);
			
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
			if (found && !s.GetFile().empty())
				found = copy.GetFile().find(s.GetFile()) != string::npos;
			if (found && !s.GetTitle().empty())
				found = copy.GetTitle().find(s.GetTitle()) != string::npos;
			if (found && !s.GetArtist().empty())
				found = copy.GetArtist().find(s.GetArtist()) != string::npos;
			if (found && !s.GetAlbum().empty())
				found = copy.GetAlbum().find(s.GetAlbum()) != string::npos;
			if (found && !s.GetYear().empty())
				found = StrToInt(copy.GetYear()) == StrToInt(s.GetYear()) && StrToInt(s.GetYear());
			if (found && !s.GetTrack().empty())
				found = StrToInt(copy.GetTrack()) == StrToInt(s.GetTrack()) && StrToInt(s.GetTrack());
			if (found && !s.GetGenre().empty())
				found = copy.GetGenre().find(s.GetGenre()) != string::npos;
			if (found && !s.GetComment().empty())
				found = copy.GetComment().find(s.GetComment()) != string::npos;
		}
		else
		{
			if (found && !s.GetFile().empty())
				found = copy.GetFile() == s.GetFile();
			if (found && !s.GetTitle().empty())
				found = copy.GetTitle() == s.GetTitle();
			if (found && !s.GetArtist().empty())
				found = copy.GetArtist() == s.GetArtist();
			if (found && !s.GetAlbum().empty())
				found = copy.GetAlbum() == s.GetAlbum();
			if (found && !s.GetYear().empty())
				found = StrToInt(copy.GetYear()) == StrToInt(s.GetYear()) && StrToInt(s.GetYear());
			if (found && !s.GetTrack().empty())
				found = StrToInt(copy.GetTrack()) == StrToInt(s.GetTrack()) && StrToInt(s.GetTrack());
			if (found && !s.GetGenre().empty())
				found = copy.GetGenre() == s.GetGenre();
			if (found && !s.GetComment().empty())
				found = copy.GetComment() == s.GetComment();
		}
		
		if (found)
		{
			mSearcher->AddOption(make_pair((Buffer *)0, *it));
			list[it-list.begin()] = 0;
		}
		found = 1;
	}
	if (Config.search_in_db) // free song list only if it's database
		FreeSongList(list);
}

