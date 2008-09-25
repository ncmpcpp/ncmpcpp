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

extern MPDConnection *Mpd;
extern Menu<Song> *mPlaylist;
extern Menu< std::pair<string, Song> > *mSearcher;

bool search_match_to_pattern = 1;
bool search_case_sensitive = 0;

string SearchEngineDisplayer(const std::pair<string, Song> &pair, void *, const Menu< std::pair<string, Song> > *)
{
	return pair.first == "." ? DisplaySong(pair.second) : pair.first;
}

void UpdateFoundList()
{
	bool bold = 0;
	for (int i = search_engine_static_options-1; i < mSearcher->Size(); i++)
	{
		for (int j = 0; j < mPlaylist->Size(); j++)
		{
			if (mPlaylist->at(j).GetHash() == mSearcher->at(i).second.GetHash())
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
	// adding several empty songs may seem riddiculous, but they hardly steal memory
	// and it's much cleaner solution than having two different containers imo
	s.Clear();
	mSearcher->Clear();
	mSearcher->Reset();
	mSearcher->AddOption(make_pair("[.b]Filename:[/b] " + s.GetName(), Song()));
	mSearcher->AddOption(make_pair("[.b]Title:[/b] " + s.GetTitle(), Song()));
	mSearcher->AddOption(make_pair("[.b]Artist:[/b] " + s.GetArtist(), Song()));
	mSearcher->AddOption(make_pair("[.b]Album:[/b] " + s.GetAlbum(), Song()));
	mSearcher->AddOption(make_pair("[.b]Year:[/b] " + s.GetYear(), Song()));
	mSearcher->AddOption(make_pair("[.b]Track:[/b] " + s.GetTrack(), Song()));
	mSearcher->AddOption(make_pair("[.b]Genre:[/b] " + s.GetGenre(), Song()));
	mSearcher->AddOption(make_pair("[.b]Comment:[/b] " + s.GetComment(), Song()));
	mSearcher->AddSeparator();
	mSearcher->AddOption(make_pair("[.b]Search mode:[/b] " + (search_match_to_pattern ? search_mode_normal : search_mode_strict), Song()));
	mSearcher->AddOption(make_pair("[.b]Case sensitive:[/b] " + (string)(search_case_sensitive ? "Yes" : "No"), Song()));
	mSearcher->AddSeparator();
	mSearcher->AddOption(make_pair("Search", Song()));
	mSearcher->AddOption(make_pair("Reset", Song()));
}

void Search(Song &s)
{
	if (s.Empty())
		return;
	
	SongList list;
	Mpd->GetDirectoryRecursive("/", list);
	
	bool found = 1;
	
	s.GetEmptyFields(1);
	
	if (!search_case_sensitive)
	{
		string t;
		t = s.GetFile();
		transform(t.begin(), t.end(), t.begin(), tolower);
		s.SetFile(t);
		
		t = s.GetTitle();
		transform(t.begin(), t.end(), t.begin(), tolower);
		s.SetTitle(t);
		
		t = s.GetArtist();
		transform(t.begin(), t.end(), t.begin(), tolower);
		s.SetArtist(t);
		
		t = s.GetAlbum();
		transform(t.begin(), t.end(), t.begin(), tolower);
		s.SetAlbum(t);
		
		t = s.GetGenre();
		transform(t.begin(), t.end(), t.begin(), tolower);
		s.SetGenre(t);
		
		t = s.GetComment();
		transform(t.begin(), t.end(), t.begin(), tolower);
		s.SetComment(t);
	}
	
	for (SongList::const_iterator it = list.begin(); it != list.end(); it++)
	{
		Song copy = **it;
		
		if (!search_case_sensitive)
		{
			string t;
			t = copy.GetName();
			transform(t.begin(), t.end(), t.begin(), tolower);
			copy.SetFile(t);
		
			t = copy.GetTitle();
			transform(t.begin(), t.end(), t.begin(), tolower);
			copy.SetTitle(t);
		
			t = copy.GetArtist();
			transform(t.begin(), t.end(), t.begin(), tolower);
			copy.SetArtist(t);
		
			t = copy.GetAlbum();
			transform(t.begin(), t.end(), t.begin(), tolower);
			copy.SetAlbum(t);
		
			t = copy.GetGenre();
			transform(t.begin(), t.end(), t.begin(), tolower);
			copy.SetGenre(t);
		
			t = copy.GetComment();
			transform(t.begin(), t.end(), t.begin(), tolower);
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
			mSearcher->AddOption(make_pair(".", **it));
		found = 1;
	}
	FreeSongList(list);
	s.GetEmptyFields(0);
}

