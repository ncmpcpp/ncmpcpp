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
#include <iostream>

#include "helpers.h"
#include "tag_editor.h"

using namespace MPD;

extern Connection *Mpd;

extern Menu<Song> *mPlaylist;
extern Menu<MPD::Item> *mBrowser;
extern Window *wFooter;

extern NcmpcppScreen current_screen;

extern int lock_statusbar_delay;

extern time_t time_of_statusbar_lock;

extern bool messages_allowed;
extern bool block_progressbar_update;
extern bool block_statusbar_update;
extern bool allow_statusbar_unlock;

extern bool search_case_sensitive;
extern bool search_match_to_pattern;

extern string EMPTY_TAG;

const string term_type = getenv("TERM") ? getenv("TERM") : "";

bool ConnectToMPD()
{
	if (!Mpd->Connect())
	{
		std::cout << "Cannot connect to mpd: " << Mpd->GetErrorMessage() << std::endl;
		return false;
	}
	return true;
}

bool ParseArgv(int argc, char **argv)
{
	using std::cout;
	using std::endl;
	
	vector<string> v;
	v.reserve(argc-1);
	for (int i = 1; i < argc; i++)
		v.push_back(argv[i]);
	
	bool exit = 0;
	for (vector<string>::iterator it = v.begin(); it != v.end() && !exit; it++)
	{
		if (*it == "-v" || *it == "--version")
		{
			cout << "ncmpcpp version: " << VERSION << endl
			<< "built with support for:"
#			ifdef HAVE_CURL_CURL_H
			<< " curl"
#			endif
#			ifdef HAVE_TAGLIB_H
			<< " taglib"
#			endif
#			ifdef _UTF8
			<< " unicode"
#			endif
			<< endl;
			return 1;
		}
		else if (*it == "-?" || *it == "--help")
		{
			cout
			<< "Usage: ncmpcpp [OPTION]...\n"
			<< "  -?, --help                show this help message\n"
			<< "  -v, --version             display version information\n\n"
			<< "  play                      start playing\n"
			<< "  pause                     pause the currently playing song\n"
			<< "  toggle                    toggle play/pause mode\n"
			<< "  stop                      stop playing\n"
			<< "  next                      play the next song\n"
			<< "  prev                      play the previous song\n"
			<< "  volume [+-]<num>          adjusts volume by [+-]<num>\n"
			;
			return 1;
		}
		
		if (!ConnectToMPD())
			return 1;
		
		if (*it == "play")
		{
			Mpd->Play();
			exit = 1;
		}
		else if (*it == "pause")
		{
			Mpd->Execute("pause \"1\"\n");
			exit = 1;
		}
		else if (*it == "toggle")
		{
			Mpd->Execute("pause\n");
			exit = 1;
		}
		else if (*it == "stop")
		{
			Mpd->Stop();
			exit = 1;
		}
		else if (*it == "next")
		{
			Mpd->Next();
			exit = 1;
		}
		else if (*it == "prev")
		{
			Mpd->Prev();
			exit = 1;
		}
		else if (*it == "volume")
		{
			it++;
			Mpd->UpdateStatus();
			if (!Mpd->GetErrorMessage().empty())
			{
				cout << "Error: " << Mpd->GetErrorMessage() << endl;
				return 1;
			}
			if (it != v.end())
				Mpd->SetVolume(Mpd->GetVolume()+StrToInt(*it));
			exit = 1;
		}
		else
		{
			cout << "ncmpcpp: invalid option " << *it << endl;
			return 1;
		}
		if (!Mpd->GetErrorMessage().empty())
		{
			cout << "Error: " << Mpd->GetErrorMessage() << endl;
			return 1;
		}
	}
	return exit;
}

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

void UpdateSongList(Menu<Song> *menu)
{
	bool bold = 0;
	for (size_t i = 0; i < menu->Size(); i++)
	{
		for (size_t j = 0; j < mPlaylist->Size(); j++)
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
	if (a->GetDisc() == b->GetDisc())
		return StrToInt(a->GetTrack()) < StrToInt(b->GetTrack());
	else
		return StrToInt(a->GetDisc()) < StrToInt(b->GetDisc());
}

void WindowTitle(const string &status)
{
	if (term_type != "linux" && Config.set_window_title)
		std::cout << "\033]0;" << status << "\7";
}

void EscapeUnallowedChars(string &s)
{
	const string unallowed_chars = "\"*/:<>?\\|";
	for (string::const_iterator it = unallowed_chars.begin(); it != unallowed_chars.end(); it++)
	{
		for (size_t i = 0; i < s.length(); i++)
		{
			if (s[i] == *it)
			{
				s.erase(s.begin()+i);
				i--;
			}
		}
	}
}

Window &operator<<(Window &w, mpd_TagItems tag)
{
	switch (tag)
	{
		case MPD_TAG_ITEM_ARTIST:
			w << "Artist";
			break;
		case MPD_TAG_ITEM_ALBUM:
			w << "Album";
			break;
		case MPD_TAG_ITEM_TITLE:
			w << "Title";
			break;
		case MPD_TAG_ITEM_TRACK:
			w << "Track";
			break;
		case MPD_TAG_ITEM_GENRE:
			w << "Genre";
			break;
		case MPD_TAG_ITEM_DATE:
			w << "Year";
			break;
		case MPD_TAG_ITEM_COMPOSER:
			w << "Composer";
			break;
		case MPD_TAG_ITEM_PERFORMER:
			w << "Performer";
			break;
		case MPD_TAG_ITEM_COMMENT:
			w << "Comment";
			break;
		case MPD_TAG_ITEM_DISC:
			w << "Disc";
			break;
		case MPD_TAG_ITEM_FILENAME:
			w << "Filename";
			break;
		default:
			break;
	}
	return w;
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
	size_t i = 1;
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
	for (size_t i = 0; i < mPlaylist->Size(); i++)
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

void DisplayStringPair(const StringPair &pair, void *, Menu<StringPair> *menu)
{
	*menu << pair.first;
}

string DisplayColumns(string song_template)
{
	vector<string> cols;
	for (size_t i = song_template.find(" "); i != string::npos; i = song_template.find(" "))
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

void DisplaySongInColumns(const Song &s, void *s_template, Menu<Song> *menu)
{
	/*string song_template = s_template ? *static_cast<string *>(s_template) : "";
	
	vector<string> cols;
	for (size_t i = song_template.find(" "); i != string::npos; i = song_template.find(" "))
	{
		cols.push_back(song_template.substr(0, i));
		song_template = song_template.substr(i+1);
	}
	cols.push_back(song_template);
	
	my_string_t result, v;
	
#	ifdef _UTF8
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
				if (s.GetTitle() != EMPTY_TAG)
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
		
		v = TO_WSTRING(ss.substr(0, width-1));
		for (int i = v.length(); i < width; i++, v += space) { }
		if (!color.empty())
		{
			result += open_col;
			result += color;
			result += close_col;
		}
		result += v;
		if (!color.empty())
			result += close_col2;
	}
	
	return TO_STRING(result);*/
	*menu << "dupa";
}

void DisplaySong(const Song &s, void *data, Menu<Song> *menu)
{
	const string &song_template = data ? *static_cast<string *>(data) : "";
	basic_buffer<my_char_t> buf;
	bool right = 0;
	
	string::const_iterator goto_pos, prev_pos;
	
	for (string::const_iterator it = song_template.begin(); it != song_template.end(); it++)
	{
		CHECK_LINKED_TAGS:;
		if (*it == '{')
		{
			prev_pos = it;
			string (Song::*get)() const = 0;
			for (; *it != '}'; it++)
			{
				if (*it == '%')
				{
					switch (*++it)
					{
						case 'l':
							get = &Song::GetLength;
							break;
						case 'F':
							get = &Song::GetFile;
							break;
						case 'f':
							get = &Song::GetName;
							break;
						case 'a':
							get = &Song::GetArtist;
							break;
						case 'b':
							get = &Song::GetAlbum;
							break;
						case 'y':
							get = &Song::GetYear;
							break;
						case 'n':
							get = &Song::GetTrack;
							break;
						case 'g':
							get = &Song::GetGenre;
							break;
						case 'c':
							get = &Song::GetComposer;
							break;
						case 'p':
							get = &Song::GetPerformer;
							break;
						case 'd':
							get = &Song::GetDisc;
							break;
						case 'C':
							get = &Song::GetComment;
							break;
						case 't':
							get = &Song::GetTitle;
							break;
						default:
							break;
					}
					if (get && (s.*get)() == EMPTY_TAG)
						break;
				}
			}
			if (*it == '}')
			{
				while (1)
				{
					if (*it == '}' && *(it+1) != '|')
						break;
					it++;
				}
				goto_pos = ++it;
				it = ++prev_pos;
			}
			else
			{
				for (; *it != '}'; it++) { }
				it++;
				if (*it == '{' || *it == '|')
				{
					if (*it == '|')
						it++;
					goto CHECK_LINKED_TAGS;
				}
			}
		}
		
		if (*it == '}')
		{
			if (goto_pos == song_template.end())
				break;
			it = goto_pos;
			if (*it == '{')
				goto CHECK_LINKED_TAGS;
		}
		
		if (*it != '%' && *it != '$')
		{
			if (!right)
				*menu << *it;
			else
				buf << *it;
		}
		else if (*it == '%')
		{
			switch (*++it)
			{
				case 'l':
					if (!right)
						*menu << s.GetLength();
					else
						buf << TO_WSTRING(s.GetLength());
					break;
				case 'F':
					if (!right)
						*menu << s.GetFile();
					else
						buf << TO_WSTRING(s.GetFile());
					break;
				case 'f':
					if (!right)
						*menu << s.GetName();
					else
						buf << TO_WSTRING(s.GetName());
					break;
				case 'a':
					if (!right)
						*menu << s.GetArtist();
					else
						buf << TO_WSTRING(s.GetArtist());
					break;
				case 'b':
					if (!right)
						*menu << s.GetAlbum();
					else
					break;
				case 'y':
					if (!right)
						*menu << s.GetYear();
					else
						buf << TO_WSTRING(s.GetYear());
					break;
				case 'n':
					if (!right)
						*menu << s.GetTrack();
					else
						buf << TO_WSTRING(s.GetTrack());
					break;
				case 'g':
					if (!right)
						*menu << s.GetGenre();
					else
						buf << TO_WSTRING(s.GetGenre());
					break;
				case 'c':
					if (!right)
						*menu << s.GetComposer();
					else
						buf << TO_WSTRING(s.GetComposer());
					break;
				case 'p':
					if (!right)
						*menu << s.GetPerformer();
					else
						buf << TO_WSTRING(s.GetPerformer());
					break;
				case 'd':
					if (!right)
						*menu << s.GetDisc();
					else
						buf << TO_WSTRING(s.GetDisc());
					break;
				case 'C':
					if (!right)
						*menu << s.GetComment();
					else
						buf << TO_WSTRING(s.GetComment());
					break;
				case 't':
					if (!right)
						*menu << s.GetTitle();
					else
						buf << TO_WSTRING(s.GetTitle());
					break;
				case 'r':
					right = 1;
					break;
				default:
					break;
			}
		}
		else
		{
			it++;
			if (!right)
				*menu << Color(*it-'0');
			else
				buf << Color(*it-'0');
		}
	}
	if (right)
	{
		menu->GotoXY(menu->GetWidth()-buf.Str().length(), menu->Y());
		*menu << buf;
	}
}

void GetInfo(Song &s, Scrollpad &info)
{
#	ifdef HAVE_TAGLIB_H
	string path_to_file;
	if (s.IsFromDB())
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	TagLib::FileRef f(path_to_file.c_str());
	if (!f.isNull())
		s.SetComment(f.tag()->comment().to8Bit(UNICODE));
#	endif // HAVE_TAGLIB_H
	
	info << fmtBold << clWhite << "Filename: " << fmtBoldEnd << clGreen << s.GetName() << "\n" << clEnd;
	info << fmtBold << "Directory: " << fmtBoldEnd << clGreen << s.GetDirectory() + "\n\n" << clEnd;
	info << fmtBold << "Length: " << fmtBoldEnd << clGreen << s.GetLength() + "\n" << clEnd;
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		info << fmtBold << "Bitrate: " << fmtBoldEnd << clGreen << f.audioProperties()->bitrate() << " kbps\n" << clEnd;
		info << fmtBold << "Sample rate: " << fmtBoldEnd << clGreen << f.audioProperties()->sampleRate() << " Hz\n" << clEnd;
		info << fmtBold << "Channels: " << fmtBoldEnd << clGreen << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << "\n" << clDefault;
	}
#	endif // HAVE_TAGLIB_H
	info << fmtBold << "\nTitle: " << fmtBoldEnd << s.GetTitle();
	info << fmtBold << "\nArtist: " << fmtBoldEnd << s.GetArtist();
	info << fmtBold << "\nAlbum: " << fmtBoldEnd << s.GetAlbum();
	info << fmtBold << "\nYear: " << fmtBoldEnd << s.GetYear();
	info << fmtBold << "\nTrack: " << fmtBoldEnd << s.GetTrack();
	info << fmtBold << "\nGenre: " << fmtBoldEnd << s.GetGenre();
	info << fmtBold << "\nComposer: " << fmtBoldEnd << s.GetComposer();
	info << fmtBold << "\nPerformer: " << fmtBoldEnd << s.GetPerformer();
	info << fmtBold << "\nDisc: " << fmtBoldEnd << s.GetDisc();
	info << fmtBold << "\nComment: " << fmtBoldEnd << s.GetComment();
}

void ShowMessage(const char *format, ...)
{
	if (messages_allowed)
	{
		time_of_statusbar_lock = time(NULL);
		lock_statusbar_delay = Config.message_delay_time;
		if (Config.statusbar_visibility)
			block_statusbar_update = 1;
		else
			block_progressbar_update = 1;
		wFooter->Bold(0);
		va_list list;
		va_start(list, format);
		wmove(wFooter->Raw(), Config.statusbar_visibility, 0);
		vw_printw(wFooter->Raw(), format, list);
		wclrtoeol(wFooter->Raw());
		va_end(list);
		wFooter->Bold(1);
		wFooter->Refresh();
	}
}

Window &Statusbar()
{
	wFooter->GotoXY(0, Config.statusbar_visibility);
	wclrtoeol(wFooter->Raw());
	return *wFooter;
}
