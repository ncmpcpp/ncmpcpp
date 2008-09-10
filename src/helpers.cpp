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

#include <algorithm>
#include "helpers.h"

extern MPDConnection *Mpd;

extern ncmpcpp_config Config;

extern Menu<Song> *mPlaylist;
extern Menu<Item> *mBrowser;
extern Window *wFooter;

extern NcmpcppScreen current_screen;

extern int lock_statusbar_delay;
extern int browsed_dir_scroll_begin;

extern time_t time_of_statusbar_lock;

extern string browsed_dir;

extern bool messages_allowed;
extern bool block_progressbar_update;
extern bool block_statusbar_update;

extern bool search_case_sensitive;
extern bool search_match_to_pattern;

extern bool redraw_screen;

extern string EMPTY_TAG;
extern string UNKNOWN_ARTIST;
extern string UNKNOWN_TITLE;
extern string UNKNOWN_ALBUM;

bool CaseInsensitiveSorting::operator()(string a, string b)
{
	transform(a.begin(), a.end(), a.begin(), tolower);
	transform(b.begin(), b.end(), b.begin(), tolower);
	return a < b;
}

bool CaseInsensitiveSorting::operator()(Song *sa, Song *sb)
{
	string a = sa->GetShortFilename();
	string b = sb->GetShortFilename();
	transform(a.begin(), a.end(), a.begin(), tolower);
	transform(b.begin(), b.end(), b.begin(), tolower);
	return a < b;
}

bool CaseInsensitiveSorting::operator()(const Item &a, const Item &b)
{
	if (a.type == b.type)
	{
		string sa = a.type == itSong ? a.song->GetShortFilename() : a.name;
		string sb = b.type == itSong ? b.song->GetShortFilename() : b.name;
		transform(sa.begin(), sa.end(), sa.begin(), tolower);
		transform(sb.begin(), sb.end(), sb.begin(), tolower);
		return sa < sb;
	}
	else
		return a.type < b.type;
}

void UpdateItemList(Menu<Item> *menu)
{
	bool bold = 0;
	for (int i = 0; i < menu->Size(); i++)
	{
		if (menu->at(i).type == itSong)
		{
			for (int j = 0; j < mPlaylist->Size(); j++)
			{
				if (mPlaylist->at(j).GetHash() == menu->at(i).song->GetHash())
				{
					bold = 1;
					break;
				}
			}
			menu->BoldOption(i, bold);
			bold = 0;
		}
	}
	menu->Refresh();
}

void UpdateSongList(Menu<Song> *menu)
{
	bool bold = 0;
	for (int i = 0; i < menu->Size(); i++)
	{
		for (int j = 0; j < mPlaylist->Size(); j++)
		{
			if (mPlaylist->at(j).GetHash() == menu->at(i).GetHash())
			{
				bold = 1;
				break;
			}
		}
		menu->BoldOption(i, bold);
		bold = 0;
	}
	menu->Refresh();
}

string DisplayKeys(int *key, int size)
{
	bool backspace = 1;
	string result = "\t";
	for (int i = 0; i < size; i++)
	{
		if (key[i] == null_key);
		else if (key[i] == 259)
			result += "Up";
		else if (key[i] == 258)
			result += "Down";
		else if (key[i] == 339)
			result += "Page Up";
		else if (key[i] == 338)
			result += "Page Down";
		else if (key[i] == 262)
			result += "Home";
		else if (key[i] == 360)
			result += "End";
		else if (key[i] == 32)
			result += "Space";
		else if (key[i] == 10)
			result += "Enter";
		else if (key[i] == 330)
			result += "Delete";
		else if (key[i] == 261)
			result += "Right";
		else if (key[i] == 260)
			result += "Left";
		else if (key[i] == 9)
			result += "Tab";
		else if (key[i] >= 1 && key[i] <= 26)
		{
			result += "Ctrl-";
			result += key[i]+64;
		}
		else if (key[i] >= 265 && key[i] <= 276)
		{
			result += "F";
			result += key[i]-216;
		}
		else if ((key[i] == 263 || key[i] == 127) && !backspace);
		else if ((key[i] == 263 || key[i] == 127) && backspace)
		{
			result += "Backspace";
			backspace = 0;
		}
		else
			result += key[i];
		result += " ";
	}
	if (result.length() > 12)
		result = result.substr(0, 12);
	for (int i = result.length(); i <= 12; result += " ", i++);
	result += ": ";
	return result;
}

bool Keypressed(int in, const int *key)
{
	return in == key[0] || in == key[1];
}

bool SortSongsByTrack(Song *a, Song *b)
{
	return StrToInt(a->GetTrack()) < StrToInt(b->GetTrack());
}

void WindowTitle(const string &status)
{
	if (TERMINAL_TYPE != "linux" && Config.set_window_title)
		printf("\033]0;%s\7",status.c_str());
}

string TotalPlaylistLength()
{
	const int MINUTE = 60;
	const int HOUR = 60*MINUTE;
	const int DAY = 24*HOUR;
	const int YEAR = 365*DAY;
	string result;
	int length = 0;
	for (int i = 0; i < mPlaylist->Size(); i++)
		length += mPlaylist->at(i).GetTotalLength();
	
	if (!length)
		return result;
	
	result += ", length: ";
	
	int years = length/YEAR;
	if (years)
	{
		result += IntoStr(years) + (years == 1 ? " year" : " years");
		length -= years*YEAR;
		if (length)
			result += ", ";
	}
	int days = length/DAY;
	if (days)
	{
		result += IntoStr(days) + (days == 1 ? " day" : " days");
		length -= days*DAY;
		if (length)
			result += ", ";
	}
	int hours = length/HOUR;
	if (hours)
	{
		result += IntoStr(hours) + (hours == 1 ? " hour" : " hours");
		length -= hours*HOUR;
		if (length)
			result += ", ";
	}
	int minutes = length/MINUTE;
	if (minutes)
	{
		result += IntoStr(minutes) + (minutes == 1 ? " minute" : " minutes");
		length -= minutes*MINUTE;
		if (length)
			result += ", ";
	}
	if (length)
		result += IntoStr(length) + (length == 1 ? " second" : " seconds");
	return result;
}

string DisplayStringPair(const StringPair &pair, void *null)
{
	return pair.first;
}

string DisplayItem(const Item &item, void *)
{
	switch (item.type)
	{
		case itDirectory:
		{
			if (item.song)
				return "[..]";
			int slash = item.name.find_last_of("/");
			return "[" + (slash != string::npos ? item.name.substr(slash+1) : item.name) + "]";
		}
		case itSong:
			return DisplaySong(*item.song);
		case itPlaylist:
			return Config.browser_playlist_prefix + item.name;
	}
}

string DisplayColumns(string song_template)
{
	vector<string> cols;
	for (int i = song_template.find(" "); i != string::npos; i = song_template.find(" "))
	{
		cols.push_back(song_template.substr(0, i));
		song_template = song_template.substr(i+1);
	}
	cols.push_back(song_template);
	
	string result, v;
	
	for (vector<string>::const_iterator it = cols.begin(); it != cols.end(); it++)
	{
		int width = StrToInt(GetLineValue(*it, '(', ')'));
		char type = GetLineValue(*it, '{', '}')[0];
		
		width *= COLS/100.0;
		
		switch (type)
		{
			case 'l':
				v = "Time";
				break;
			case 'f':
				v = "Filename";
				break;
			case 'F':
				v = "Full filename";
				break;
			case 'a':
				v = "Artist";
				break;
			case 't':
				v = "Title";
				break;
			case 'b':
				v = "Album";
				break;
			case 'y':
				v = "Year";
				break;
			case 'n':
				v = "Track";
				break;
			case 'g':
				v = "Genre";
				break;
			case 'c':
				v = "Composer";
				break;
			case 'p':
				v = "Performer";
				break;
			case 'd':
				v = "Disc";
				break;
			case 'C':
				v = "Comment";
				break;
			default:
				break;
		}
		
		v = v.substr(0, width-1);
		for (int i = v.length(); i < width; i++, v += " ");
		result += v;
	}
	
	return result.substr(0, COLS);
}

string DisplaySongInColumns(const Song &s, void *s_template)
{
	string song_template = s_template ? *static_cast<string *>(s_template) : "";
	
	vector<string> cols;
	for (int i = song_template.find(" "); i != string::npos; i = song_template.find(" "))
	{
		cols.push_back(song_template.substr(0, i));
		song_template = song_template.substr(i+1);
	}
	cols.push_back(song_template);
	
	my_string_t result, v;
	
#	ifdef UTF8_ENABLED
	const wstring space = L" ";
	const wstring open_col = L"[.";
	const wstring close_col = L"]";
	const wstring close_col2 = L"[/red]";
#	else
	const string space = " ";
	const string open_col = "[.";
	const string close_col = "]";
	const string close_col2 = "[/red]";
#	endif
	
	for (vector<string>::const_iterator it = cols.begin(); it != cols.end(); it++)
	{
		int width = StrToInt(GetLineValue(*it, '(', ')'));
		my_string_t color = TO_WSTRING(GetLineValue(*it, '[', ']'));
		char type = GetLineValue(*it, '{', '}')[0];
		
		width *= COLS/100.0;
		
		string ss;
		switch (type)
		{
			case 'l':
				ss = s.GetLength();
				break;
			case 'f':
				ss = s.GetShortFilename();
				break;
			case 'F':
				ss = s.GetFile();
				break;
			case 'a':
				ss = s.GetArtist();
				break;
			case 't':
				ss = s.GetTitle() != UNKNOWN_TITLE ? s.GetTitle() : s.GetShortFilename().substr(0, s.GetShortFilename().find_last_of("."));
				break;
			case 'b':
				ss = s.GetAlbum();
				break;
			case 'y':
				ss = s.GetYear();
				break;
			case 'n':
				ss = s.GetTrack();
				break;
			case 'g':
				ss = s.GetGenre();
				break;
			case 'c':
				ss = s.GetComposer();
				break;
			case 'p':
				ss = s.GetPerformer();
				break;
			case 'd':
				ss = s.GetDisc();
				break;
			case 'C':
				ss = s.GetComment();
				break;
			default:
				break;
		}
		
		v = TO_WSTRING(Window::OmitBBCodes(ss)).substr(0, width-1);
		for (int i = v.length(); i < width; i++, v += space);
		if (!color.empty())
			result += open_col + color + close_col;
		result += v;
		if (!color.empty())
			result += close_col2;
	}
	
	return TO_STRING(result);
}

string DisplaySong(const Song &s, void *s_template)
{	
	const string &song_template = s_template ? *static_cast<string *>(s_template) : "";
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
					while (*++it != '}');
			}
		}
		
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
					if (link_tags)
					{
						if (s.GetTotalLength() > 0)
						{
							result += s.GetLength();
							i += s.GetLength().length();
						}
						else
							tags_present = 0;
					}
					else
						result += s.GetLength();
					break;
				}
				case 'F':
				{
					result += s.GetFile();
					i += s.GetFile().length();
					break;
				}
				case 'f':
				{
					result += s.GetShortFilename();
					i += s.GetShortFilename().length();
					break;
				}
				case 'a':
				{
					if (link_tags)
					{
						if (s.GetArtist() != UNKNOWN_ARTIST)
						{
							result += s.GetArtist();
							i += s.GetArtist().length();
						}
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
						{
							result += s.GetAlbum();
							i += s.GetAlbum().length();
						}
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
						if (s.GetYear() != EMPTY_TAG)
						{
							result += s.GetYear();
							i += s.GetYear().length();
						}
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
						if (s.GetTrack() != EMPTY_TAG)
						{
							result += s.GetTrack();
							i += s.GetTrack().length();
						}
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
						if (s.GetGenre() != EMPTY_TAG)
						{
							result += s.GetGenre();
							i += s.GetGenre().length();
						}
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
						if (s.GetComposer() != EMPTY_TAG)
						{
							result += s.GetComposer();
							i += s.GetComposer().length();
						}
						else
							tags_present = 0;
					}
					else
						result += s.GetComposer();
					break;
				}
				case 'p':
				{
					if (link_tags)
					{
						if (s.GetPerformer() != EMPTY_TAG)
						{
							result += s.GetPerformer();
							i += s.GetPerformer().length();
						}
						else
							tags_present = 0;
					}
					else
						result += s.GetPerformer();
					break;
				}
				case 'd':
				{
					if (link_tags)
					{
						if (s.GetDisc() != EMPTY_TAG)
						{
							result += s.GetDisc();
							i += s.GetDisc().length();
						}
						else
							tags_present = 0;
					}
					else
						result += s.GetDisc();
					break;
				}
				case 'C':
				{
					if (link_tags)
					{
						if (s.GetComment() != EMPTY_TAG)
						{
							result += s.GetComment();
							i += s.GetComment().length();
						}
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
						{
							result += s.GetTitle();
							i += s.GetTitle().length();
						}
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

void ShowMessage(const string &message, int delay)
{
	if (messages_allowed)
	{
		time_of_statusbar_lock = time(NULL);
		lock_statusbar_delay = delay;
		if (Config.statusbar_visibility)
			block_statusbar_update = 1;
		else
			block_progressbar_update = 1;
		wFooter->Bold(0);
		wFooter->WriteXY(0, Config.statusbar_visibility, message, 1);
		wFooter->Bold(1);
	}
}

void GetDirectory(string dir, string subdir)
{
	int highlightme = -1;
	browsed_dir_scroll_begin = 0;
	if (browsed_dir != dir)
		mBrowser->Reset();
	browsed_dir = dir;
	mBrowser->Clear(0);
	
	if (dir != "/")
	{
		Item parent;
		int slash = dir.find_last_of("/");
		parent.song = (Song *) 1; // in that way we assume that's really parent dir
		parent.name = slash != string::npos ? dir.substr(0, slash) : "/";
		parent.type = itDirectory;
		mBrowser->AddOption(parent);
	}
	
	ItemList list;
	Mpd->GetDirectory(dir, list);
	sort(list.begin(), list.end(), CaseInsensitiveSorting());
	
	for (ItemList::iterator it = list.begin(); it != list.end(); it++)
	{
		switch (it->type)
		{
			case itPlaylist:
			{
				mBrowser->AddOption(*it);
				break;
			}
			case itDirectory:
			{
				if (it->name == subdir)
					highlightme = mBrowser->Size();
				mBrowser->AddOption(*it);
				break;
			}
			case itSong:
			{
				bool bold = 0;
				for (int i = 0; i < mPlaylist->Size(); i++)
				{
					if (mPlaylist->at(i).GetHash() == it->song->GetHash())
					{
						bold = 1;
						break;
					}
				}
				bold ? mBrowser->AddBoldOption(*it) : mBrowser->AddOption(*it);
				break;
			}
		}
	}
	mBrowser->Highlight(highlightme);
	
	if (current_screen == csBrowser)
		mBrowser->Hide();
}

