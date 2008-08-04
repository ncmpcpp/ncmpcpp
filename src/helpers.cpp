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
#include "settings.h"

#define FOR_EACH_MPD_DATA(x) for (; (x); (x) = mpd_data_get_next(x))

extern MpdObj *conn;
extern MpdData *browser;

extern ncmpcpp_config Config;

extern Menu *mBrowser;
extern Menu *mTagEditor;
extern Menu *mSearcher;

extern Window *wFooter;

extern vector<Song> vPlaylist;
extern vector<MpdDataType> vFileType;
extern vector<string> vNameList;

extern Song edited_song;
extern Song searched_song;

extern int block_statusbar_update_delay;
extern int browsed_dir_scroll_begin;

extern time_t block_delay;

extern string browsed_dir;
extern string browsed_subdir;

extern bool messages_allowed;
extern bool block_progressbar_update;
extern bool block_statusbar_update;

extern bool search_case_sensitive;
extern bool search_mode_match;

extern string EMPTY;
extern string UNKNOWN_ARTIST;
extern string UNKNOWN_TITLE;
extern string UNKNOWN_ALBUM;

void WindowTitle(const string &status)
{
	if (TERMINAL_TYPE != "linux" && Config.set_window_title)
		   printf("\033]0;%s\7",status.c_str());
}

string DisplaySong(const Song &s, const string &song_template)
{	
	string result;
	bool link_tags = 0;
	bool tags_present = 0;
	int i = 0;
	
	for (string::const_iterator it = song_template.begin(); it != song_template.end(); it++)
	{
		if (*it == '}')
		{
			if (!tags_present)
				result = result.substr(0, result.length()-i);
			
			it++;
			link_tags = 0;
			i = 0;
			
			if (*it == '|' && *(it+1) == '{')
			{
				if (!tags_present)
					it++;
				else
					while (*it++ != '}');
			}
		}
		
		if (*it == '{')
		{
			i = 0;
			tags_present = 1;
			link_tags = 1;
			it++;
		}
		
		if (it == song_template.end())
			break;
		
		if (*it != '%')
		{
			i++;
			result += *it;
		}
		else
		{
			switch (*++it)
			{
				case 'l':
				{
					result += s.GetLength();
					break;
				}
				case 'F':
				{
					result += s.GetFile();
					break;
				}
				case 'f':
				{
					result += s.GetShortFilename();
					break;
				}
				case 'a':
				{
					if (link_tags)
					{
						if (s.GetArtist() != UNKNOWN_ARTIST)
							result += s.GetArtist();
						else
							tags_present = 0;
					}
					else
						result += s.GetArtist();
					break;
				}
				case 'b':
				{
					if (link_tags)
					{
						if (s.GetAlbum() != UNKNOWN_ALBUM)
							result += s.GetAlbum();
						else
							tags_present = 0;
					}
					else
						result += s.GetAlbum();
					break;
				}
				case 'y':
				{
					if (link_tags)
					{
						if (s.GetYear() != EMPTY)
							result += s.GetYear();
						else
							tags_present = 0;
					}
					else
						result += s.GetYear();
					break;
				}
				case 'n':
				{
					if (link_tags)
					{
						if (s.GetTrack() != EMPTY)
							result += s.GetTrack();
						else
							tags_present = 0;
					}
					else
						result += s.GetTrack();
					break;
				}
				case 'g':
				{
					if (link_tags)
					{
						if (s.GetGenre() != EMPTY)
							result += s.GetGenre();
						else
							tags_present = 0;
					}
					else
						result += s.GetGenre();
					break;
				}
				case 'c':
				{
					if (link_tags)
					{
						if (s.GetComment() != EMPTY)
							result += s.GetComment();
						else
							tags_present = 0;
					}
					else
						result += s.GetComment();
					break;
				}
				case 't':
				{
					if (link_tags)
					{
						if (s.GetTitle() != UNKNOWN_TITLE)
							result += s.GetTitle();
						else
							tags_present = 0;
					}
					else
						result += s.GetTitle();
					break;
				}
				
			}
		}
	}
	return result;
}

void PrepareSearchEngine(Song &s)
{
	s.Clear();
	mSearcher->Clear();
	mSearcher->Reset();
	mSearcher->AddOption("[b]Filename:[/b] " + s.GetShortFilename());
	mSearcher->AddOption("[b]Title:[/b] " + s.GetTitle());
	mSearcher->AddOption("[b]Artist:[/b] " + s.GetArtist());
	mSearcher->AddOption("[b]Album:[/b] " + s.GetAlbum());
	mSearcher->AddOption("[b]Year:[/b] " + s.GetYear());
	mSearcher->AddOption("[b]Track:[/b] " + s.GetTrack());
	mSearcher->AddOption("[b]Genre:[/b] " + s.GetGenre());
	mSearcher->AddOption("[b]Comment:[/b] " + s.GetComment());
	mSearcher->AddSeparator();
	mSearcher->AddOption("[b]Search mode:[/b] " + (search_mode_match ? search_mode_one : search_mode_two));
	mSearcher->AddOption("[b]Case sensitive:[/b] " + (string)(search_case_sensitive ? "Yes" : "No"));
	mSearcher->AddSeparator();
	mSearcher->AddOption("Search");
	mSearcher->AddOption("Reset");
}

void Search(vector<Song> &result, Song &s)
{
	result.clear();
	
	if (s.Empty())
		return;
	
	MpdData *everything = mpd_database_get_complete(conn);
	bool found = 1;
	
	s.GetEmptyFields(1);
	
	if (!search_case_sensitive)
	{
		string t;
		t = s.GetShortFilename();
		transform(t.begin(), t.end(), t.begin(), tolower);
		s.SetShortFilename(t);
		
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
	
	FOR_EACH_MPD_DATA(everything)
	{
		Song new_song = everything->song;
		Song copy = new_song;
		
		if (!search_case_sensitive)
		{
			string t;
			t = copy.GetShortFilename();
			transform(t.begin(), t.end(), t.begin(), tolower);
			copy.SetShortFilename(t);
		
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
		
		if (search_mode_match)
		{
			if (found && !s.GetShortFilename().empty())
				found = copy.GetShortFilename().find(s.GetShortFilename()) != string::npos;
			if (found && !s.GetTitle().empty())
				found = copy.GetTitle().find(s.GetTitle()) != string::npos;
			if (found && !s.GetArtist().empty())
				found = copy.GetArtist().find(s.GetArtist()) != string::npos;
			if (found && !s.GetAlbum().empty())
				found = copy.GetAlbum().find(s.GetAlbum()) != string::npos;
			if (found && !s.GetYear().empty())
				found = atoi(copy.GetYear().c_str()) == atoi(s.GetYear().c_str()) && atoi(s.GetYear().c_str());
			if (found && !s.GetTrack().empty())
				found = atoi(copy.GetTrack().c_str()) == atoi(s.GetTrack().c_str()) && atoi(s.GetTrack().c_str());
			if (found && !s.GetGenre().empty())
				found = copy.GetGenre().find(s.GetGenre()) != string::npos;
			if (found && !s.GetComment().empty())
				found = copy.GetComment().find(s.GetComment()) != string::npos;
		}
		else
		{
			if (found && !s.GetShortFilename().empty())
				found = copy.GetShortFilename() == s.GetShortFilename();
			if (found && !s.GetTitle().empty())
				found = copy.GetTitle() == s.GetTitle();
			if (found && !s.GetArtist().empty())
				found = copy.GetArtist() == s.GetArtist();
			if (found && !s.GetAlbum().empty())
				found = copy.GetAlbum() == s.GetAlbum();
			if (found && !s.GetYear().empty())
				found = atoi(copy.GetYear().c_str()) == atoi(s.GetYear().c_str()) && atoi(s.GetYear().c_str());
			if (found && !s.GetTrack().empty())
				found = atoi(copy.GetTrack().c_str()) == atoi(s.GetTrack().c_str()) && atoi(s.GetTrack().c_str());
			if (found && !s.GetGenre().empty())
				found = copy.GetGenre() == s.GetGenre();
			if (found && !s.GetComment().empty())
				found = copy.GetComment() == s.GetComment();
		}
		
		if (found)
			result.push_back(new_song);
		
		found = 1;
	}
	s.GetEmptyFields(0);
}

void ShowMessage(const string &message, int delay)
{
	if (messages_allowed)
	{
		block_delay = time(NULL);
		block_statusbar_update_delay = delay;
		block_statusbar_update = 1;
		wFooter->Bold(0);
		wFooter->WriteXY(0, 1, message, 1);
		wFooter->Bold(1);
	}
}

bool GetSongInfo(Song &s)
{
	mTagEditor->Clear();
	mTagEditor->Reset();
	
	string path_to_file = Config.mpd_music_dir + "/" + s.GetFile();
	
	TagLib::FileRef f(path_to_file.c_str());
	
	if (f.isNull())
		return false;
	
	s.SetComment(f.tag()->comment().to8Bit(UNICODE));
	
	mTagEditor->AddStaticOption("[b][white]Song name: [green][/b]" + s.GetShortFilename());
	mTagEditor->AddStaticOption("[b][white]Location in DB: [green][/b]" + s.GetDirectory());
	mTagEditor->AddStaticOption("");
	mTagEditor->AddStaticOption("[b][white]Length: [green][/b]" + s.GetLength());
	mTagEditor->AddStaticOption("[b][white]Bitrate: [green][/b]" + IntoStr(f.audioProperties()->bitrate()) + " kbps");
	mTagEditor->AddStaticOption("[b][white]Sample rate: [green][/b]" + IntoStr(f.audioProperties()->sampleRate()) + " Hz");
	mTagEditor->AddStaticOption("[b][white]Channels: [green][/b]" + (string)(f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") + "[/green]");
	
	mTagEditor->AddSeparator();
	
	mTagEditor->AddOption("[b]Title:[/b] " + s.GetTitle());
	mTagEditor->AddOption("[b]Artist:[/b] " + s.GetArtist());
	mTagEditor->AddOption("[b]Album:[/b] " + s.GetAlbum());
	mTagEditor->AddOption("[b]Year:[/b] " + s.GetYear());
	mTagEditor->AddOption("[b]Track:[/b] " + s.GetTrack());
	mTagEditor->AddOption("[b]Genre:[/b] " + s.GetGenre());
	mTagEditor->AddOption("[b]Comment:[/b] " + s.GetComment());
	mTagEditor->AddSeparator();
	mTagEditor->AddOption("Save");
	mTagEditor->AddOption("Cancel");
	
	edited_song = s;
	return true;
}

void GetDirectory(string dir)
{
	browsed_dir_scroll_begin = 0;
	if (browsed_dir != dir)
		mBrowser->Reset();
	browsed_dir = dir;
	vFileType.clear();
	vNameList.clear();
	mBrowser->Clear();
	if (dir != "/")
	{
		mBrowser->AddOption("[..]");
		vFileType.push_back(MPD_DATA_TYPE_DIRECTORY);
		vNameList.push_back("");
	}
	
	browser = mpd_database_get_directory(conn, (char *)dir.c_str());
	FOR_EACH_MPD_DATA(browser)
	{
		switch (browser->type)
		{
			case MPD_DATA_TYPE_PLAYLIST:
			{
				vFileType.push_back(MPD_DATA_TYPE_PLAYLIST);
				vNameList.push_back(browser->playlist);
				mBrowser->AddOption("[red](playlist)[/red] " + string(browser->playlist));
				break;
			}
			case MPD_DATA_TYPE_DIRECTORY:
			{
				string subdir = browser->directory;
				vFileType.push_back(MPD_DATA_TYPE_DIRECTORY);
				if (dir == "/")
					vNameList.push_back(subdir.substr(browsed_dir.size()-1,subdir.size()-browsed_dir.size()+1));
				else
					vNameList.push_back(subdir.substr(browsed_dir.size()+1,subdir.size()-browsed_dir.size()-1));
				
				mBrowser->AddOption("[" + vNameList.back() + "]");
				if (vNameList.back() == browsed_subdir)
					mBrowser->Highlight(mBrowser->MaxChoice());
				break;
			}
			case MPD_DATA_TYPE_SONG:
			{
				vFileType.push_back(MPD_DATA_TYPE_SONG);
				Song s = browser->song;
				vNameList.push_back(s.GetFile());
				bool bold = 0;
				for (vector<Song>::const_iterator it = vPlaylist.begin(); it != vPlaylist.end(); it++)
					if (it->GetFile() == s.GetFile())
						bold = 1;
				bold ? mBrowser->AddBoldOption(DisplaySong(s)) : mBrowser->AddOption(DisplaySong(s));
				break;
			}
		}
	}
	mpd_data_free(browser);
	browsed_subdir.clear();
}

