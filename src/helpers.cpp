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
#include "tag_editor.h"

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
extern bool allow_statusbar_unlock;

extern bool search_case_sensitive;
extern bool search_match_to_pattern;

extern bool redraw_screen;

extern string EMPTY_TAG;
extern string UNKNOWN_ARTIST;
extern string UNKNOWN_TITLE;
extern string UNKNOWN_ALBUM;

void LockStatusbar()
{
	if (Config.statusbar_visibility)
		block_statusbar_update = 1;
	else
		block_progressbar_update = 1;
	allow_statusbar_unlock = 0;
}

void UnlockStatusbar()
{
	allow_statusbar_unlock = 1;
	if (lock_statusbar_delay < 0)
	{
		if (Config.statusbar_visibility)
			block_statusbar_update = 0;
		else
			block_progressbar_update = 0;
	}
}

bool CaseInsensitiveSorting::operator()(string a, string b)
{
	ToLower(a);
	ToLower(b);
	return a < b;
}

bool CaseInsensitiveSorting::operator()(Song *sa, Song *sb)
{
	string a = sa->GetName();
	string b = sb->GetName();
	ToLower(a);
	ToLower(b);
	return a < b;
}

bool CaseInsensitiveSorting::operator()(const Item &a, const Item &b)
{
	if (a.type == b.type)
	{
		string sa = a.type == itSong ? a.song->GetName() : a.name;
		string sb = b.type == itSong ? b.song->GetName() : b.name;
		ToLower(sa);
		ToLower(sb);
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

void EscapeUnallowedChars(string &s)
{
	const string unallowed_chars = "\"*/:<>?\\|";
	for (string::const_iterator it = unallowed_chars.begin(); it != unallowed_chars.end(); it++)
	{
		for (int i = 0; i < s.length(); i++)
		{
			if (s[i] == *it)
			{
				s.erase(s.begin()+i);
				i--;
			}
		}
	}
}

string IntoStr(mpd_TagItems tag)
{
	switch (tag)
	{
		case MPD_TAG_ITEM_ARTIST:
			return "Artist";
		case MPD_TAG_ITEM_ALBUM:
			return "Album";
		case MPD_TAG_ITEM_TITLE:
			return "Title";
		case MPD_TAG_ITEM_TRACK:
			return "Track";
		case MPD_TAG_ITEM_GENRE:
			return "Genre";
		case MPD_TAG_ITEM_DATE:
			return "Year";
		case MPD_TAG_ITEM_COMPOSER:
			return "Composer";
		case MPD_TAG_ITEM_PERFORMER:
			return "Performer";
		case MPD_TAG_ITEM_COMMENT:
			return "Comment";
		case MPD_TAG_ITEM_DISC:
			return "Disc";
		case MPD_TAG_ITEM_FILENAME:
			return "Filename";
		default:
			return "";
	}
}

string FindSharedDir(const string &one, const string &two)
{
	if (one == two)
		return one;
	string result;
	int i = 1;
	while (one.substr(0, i) == two.substr(0, i))
		i++;
	result = one.substr(0, i);
	i = result.find_last_of("/");
	return i != string::npos ? result.substr(0, i) : "/";
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

string DisplayStringPair(const StringPair &pair, void *, const Menu<StringPair> *)
{
	return pair.first;
}

string DisplayItem(const Item &item, void *, const Menu<Item> *menu)
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
			// I know casting that way is ugly etc., but it works.
			return DisplaySong(*item.song, &Config.song_list_format, (const Menu<Song> *)menu);
		case itPlaylist:
			return Config.browser_playlist_prefix + item.name;
		default:
			return "";
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
		for (int i = v.length(); i < width; i++, v += " ") { }
		result += v;
	}
	
	return result.substr(0, COLS);
}

string DisplaySongInColumns(const Song &s, void *s_template, const Menu<Song> *)
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
				ss = s.GetName();
				break;
			case 'F':
				ss = s.GetFile();
				break;
			case 'a':
				ss = s.GetArtist();
				break;
			case 't':
				if (s.GetTitle() != UNKNOWN_TITLE)
					ss = s.GetTitle();
				else
				{
					const string &file = s.GetName();
					ss = !s.IsStream() ? file.substr(0, file.find_last_of(".")) : file;
				}
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
		for (int i = v.length(); i < width; i++, v += space) { }
		if (!color.empty())
			result += open_col + color + close_col;
		result += v;
		if (!color.empty())
			result += close_col2;
	}
	
	return TO_STRING(result);
}

string DisplaySong(const Song &s, void *s_template, const Menu<Song> *menu)
{	
	const string &song_template = s_template ? *static_cast<string *>(s_template) : "";
	string result;
	string lresult;
	bool link_tags = 0;
	bool tags_present = 0;
	bool right = 0;
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
					while (*++it != '}') { }
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
					while (*it++ != '}') { }
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
					result += s.GetName();
					i += s.GetName().length();
					break;
				}
				case 'a':
				{
					if (link_tags)
					{
						if (!s.GetArtist().empty() && s.GetArtist() != UNKNOWN_ARTIST)
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
						if (!s.GetAlbum().empty() && s.GetAlbum() != UNKNOWN_ALBUM)
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
						if (!s.GetYear().empty() && s.GetYear() != EMPTY_TAG)
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
						if (!s.GetTrack().empty() && s.GetTrack() != EMPTY_TAG)
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
						if (!s.GetGenre().empty() && s.GetGenre() != EMPTY_TAG)
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
						if (!s.GetComposer().empty() && s.GetComposer() != EMPTY_TAG)
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
						if (!s.GetPerformer().empty() && s.GetPerformer() != EMPTY_TAG)
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
						if (!s.GetDisc().empty() && s.GetDisc() != EMPTY_TAG)
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
						if (!s.GetComment().empty() && s.GetComment() != EMPTY_TAG)
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
						if (!s.GetTitle().empty() && s.GetTitle() != UNKNOWN_TITLE)
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
				case 'r':
				{
					if (!right)
					{
						right = 1;
						lresult = result;
						result.clear();
						i = 0;
					}
				}
				
			}
		}
	}
	if (right && menu)
	{
		result = lresult + "[." + IntoStr(menu->GetWidth()-Window::RealLength(result)) + "]" + result;
	}
	return result;
}

string GetInfo(Song &s)
{
	string result;
#	ifdef HAVE_TAGLIB_H
	string path_to_file = Config.mpd_music_dir + s.GetFile();
	TagLib::FileRef f(path_to_file.c_str());
	if (!f.isNull())
		s.SetComment(f.tag()->comment().to8Bit(UNICODE));
#	endif // HAVE_TAGLIB_H
	
	result = "[.b][." + Config.color1 + "]Filename: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + s.GetName() + "[/" + Config.color2 + "]\n";
	result += "[.b][." + Config.color1 + "]Directory: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + s.GetDirectory() + "[/" + Config.color2 + "]\n\n";
	result += "[.b][." + Config.color1 + "]Length: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + s.GetLength() + "[/" + Config.color2 + "]\n";
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		result += "[.b][." + Config.color1 + "]Bitrate: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + IntoStr(f.audioProperties()->bitrate()) + " kbps[/" + Config.color2 + "]\n";
		result += "[.b][." + Config.color1 + "]Sample rate: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + IntoStr(f.audioProperties()->sampleRate()) + " Hz[/" + Config.color2 + "]\n";
		result += "[.b][." + Config.color1 + "]Channels: [/" + Config.color1 + "][." + Config.color2 + "][/b]" + string(f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") + "[/" + Config.color2 + "]\n";
	}
#	endif // HAVE_TAGLIB_H
	result += "\n[.b]Title:[/b] " + s.GetTitle();
	result += "\n[.b]Artist:[/b] " + s.GetArtist();
	result += "\n[.b]Album:[/b] " + s.GetAlbum();
	result += "\n[.b]Year:[/b] " + s.GetYear();
	result += "\n[.b]Track:[/b] " + s.GetTrack();
	result += "\n[.b]Genre:[/b] " + s.GetGenre();
	result += "\n[.b]Composer:[/b] " + s.GetComposer();
	result += "\n[.b]Performer:[/b] " + s.GetPerformer();
	result += "\n[.b]Disc:[/b] " + s.GetDisc();
	result += "\n[.b]Comment:[/b] " + s.GetComment();
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
				mBrowser->AddOption(*it, bold);
				break;
			}
		}
	}
	mBrowser->Highlight(highlightme);
	
	if (current_screen == csBrowser)
		mBrowser->Hide();
}

