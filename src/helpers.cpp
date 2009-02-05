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

#include <algorithm>
#include <iostream>

#include "charset.h"
#include "helpers.h"
#include "tag_editor.h"

using namespace MPD;

extern Connection *Mpd;

extern Menu<Song> *mPlaylist;
extern Menu<MPD::Item> *mBrowser;
extern Window *wFooter;

extern NcmpcppScreen current_screen;

extern bool search_case_sensitive;
extern bool search_match_to_pattern;

const string term_type = getenv("TERM") ? getenv("TERM") : "";

namespace
{
	inline void remove_the_word(string &s)
	{
		size_t the_pos = s.find("the ");
		if (the_pos == 0 && the_pos != string::npos)
			s = s.substr(4);
	}
	
	inline string extract_top_directory(const string &s)
	{
		size_t slash = s.rfind("/");
		return slash != string::npos ? s.substr(++slash) : s;
	}
}

bool ConnectToMPD()
{
	if (!Mpd->Connect())
	{
		std::cout << "Cannot connect to mpd: " << Mpd->GetErrorMessage() << std::endl;
		return false;
	}
	return true;
}

void ParseArgv(int argc, char **argv)
{
	using std::cout;
	using std::endl;
	
	bool quit = 0;
	
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
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
			exit(0);
		}
		else if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0)
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
			exit(0);
		}
		
		if (!ConnectToMPD())
			exit(0);
		
		if (strcmp(argv[i], "play") == 0)
		{
			Mpd->Play();
			quit = 1;
		}
		else if (strcmp(argv[i], "pause") == 0)
		{
			Mpd->Execute("pause \"1\"\n");
			quit = 1;
		}
		else if (strcmp(argv[i], "toggle") == 0)
		{
			Mpd->UpdateStatus();
			switch (Mpd->GetState())
			{
				case psPause:
				case psPlay:
					Mpd->Pause();
					break;
				case psStop:
					Mpd->Play();
					break;
				default:
					break;
			}
			quit = 1;
		}
		else if (strcmp(argv[i], "stop") == 0)
		{
			Mpd->Stop();
			quit = 1;
		}
		else if (strcmp(argv[i], "next") == 0)
		{
			Mpd->Next();
			quit = 1;
		}
		else if (strcmp(argv[i], "prev") == 0)
		{
			Mpd->Prev();
			quit = 1;
		}
		else if (strcmp(argv[i], "volume") == 0)
		{
			i++;
			Mpd->UpdateStatus();
			if (!Mpd->GetErrorMessage().empty())
			{
				cout << "Error: " << Mpd->GetErrorMessage() << endl;
				exit(0);
			}
			if (i != argc)
				Mpd->SetVolume(Mpd->GetVolume()+atoi(argv[i]));
			quit = 1;
		}
		else
		{
			cout << "ncmpcpp: invalid option: " << argv[i] << endl;
			exit(0);
		}
		if (!Mpd->GetErrorMessage().empty())
		{
			cout << "Error: " << Mpd->GetErrorMessage() << endl;
			exit(0);
		}
	}
	if (quit)
		exit(0);
}

bool CaseInsensitiveSorting::operator()(string a, string b)
{
	ToLower(a);
	ToLower(b);
	if (Config.ignore_leading_the)
	{
		remove_the_word(a);
		remove_the_word(b);
	}
	return a < b;
}

bool CaseInsensitiveSorting::operator()(Song *sa, Song *sb)
{
	string a = sa->GetName();
	string b = sb->GetName();
	ToLower(a);
	ToLower(b);
	if (Config.ignore_leading_the)
	{
		remove_the_word(a);
		remove_the_word(b);
	}
	return a < b;
}

bool CaseInsensitiveSorting::operator()(const Item &a, const Item &b)
{
	if (a.type == b.type)
	{
		switch (a.type)
		{
			case itDirectory:
				return operator()(extract_top_directory(a.name), extract_top_directory(b.name));
			case itPlaylist:
				return operator()(a.name, b.name);
			case itSong:
				return operator()(a.song, b.song);
			default: // there's no other type, just silence compiler.
				return 0;
		}
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

string IntoStr(mpd_TagItems tag) // this is only for left column's title in media library
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

void DisplayTotalPlaylistLength(Window &w)
{
	const int MINUTE = 60;
	const int HOUR = 60*MINUTE;
	const int DAY = 24*HOUR;
	const int YEAR = 365*DAY;
	int length = 0;
	
	for (size_t i = 0; i < mPlaylist->Size(); i++)
		length += mPlaylist->at(i).GetTotalLength();
	
	w << '(' << mPlaylist->Size() << (mPlaylist->Size() == 1 ? " item" : " items");
	
	if (length)
	{
		w << ", length: ";
		int years = length/YEAR;
		if (years)
		{
			w << years << (years == 1 ? " year" : " years");
			length -= years*YEAR;
			if (length)
				w << ", ";
		}
		int days = length/DAY;
		if (days)
		{
			w << days << (days == 1 ? " day" : " days");
			length -= days*DAY;
			if (length)
				w << ", ";
		}
		int hours = length/HOUR;
		if (hours)
		{
			w << hours << (hours == 1 ? " hour" : " hours");
			length -= hours*HOUR;
			if (length)
				w << ", ";
		}
		int minutes = length/MINUTE;
		if (minutes)
		{
			w << minutes << (minutes == 1 ? " minute" : " minutes");
			length -= minutes*MINUTE;
			if (length)
				w << ", ";
		}
		if (length)
			w << length << (length == 1 ? " second" : " seconds");
	}
	w << ')';
	w.Refresh();
}

void DisplayStringPair(const StringPair &pair, void *, Menu<StringPair> *menu)
{
	*menu << pair.first;
}

string DisplayColumns(string st)
{
	string result;
	size_t where = 0;
	
	for (int width = StrToInt(GetLineValue(st, '(', ')', 1)); width; width = StrToInt(GetLineValue(st, '(', ')', 1)))
	{
		width *= COLS/100.0;
		char type = GetLineValue(st, '{', '}', 1)[0];
		
		switch (type)
		{
			case 'l':
				result += "Time";
				break;
			case 'f':
				result += "Filename";
				break;
			case 'F':
				result += "Full filename";
				break;
			case 'a':
				result += "Artist";
				break;
			case 't':
				result += "Title";
				break;
			case 'b':
				result += "Album";
				break;
			case 'y':
				result += "Year";
				break;
			case 'n':
				result += "Track";
				break;
			case 'g':
				result += "Genre";
				break;
			case 'c':
				result += "Composer";
				break;
			case 'p':
				result += "Performer";
				break;
			case 'd':
				result += "Disc";
				break;
			case 'C':
				result += "Comment";
				break;
			default:
				break;
		}
		where += width;
		
		if (result.length() > where)
			result = result.substr(0, where);
		else
			for (size_t i = result.length(); i <= where && i < size_t(COLS); i++, result += ' ') { }
	}
	return result;
}

void DisplaySongInColumns(const Song &s, void *s_template, Menu<Song> *menu)
{
	string st = s_template ? *static_cast<string *>(s_template) : "";
	size_t where = 0;
	Color color;
	
	for (int width = StrToInt(GetLineValue(st, '(', ')', 1)); width; width = StrToInt(GetLineValue(st, '(', ')', 1)))
	{
		if (where)
		{
			menu->GotoXY(where, menu->Y());
			*menu << ' ';
			if (color != clDefault)
				*menu << clEnd;
		}
		
		width *= COLS/100.0;
		color = IntoColor(GetLineValue(st, '[', ']', 1));
		char type = GetLineValue(st, '{', '}', 1)[0];
		
		string (Song::*get)() const = 0;
		
		switch (type)
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
				if (!s.GetTitle().empty())
					get = &Song::GetTitle;
				else
					get = &Song::GetName;
				break;
			default:
				break;
		}
		if (color != clDefault)
			*menu << color;
		whline(menu->Raw(), 32, menu->GetWidth()-where);
		string tag = (s.*get)();
		if (!tag.empty())
			*menu << tag;
		else
			*menu << Config.empty_tag;
		where += width;
	}
	if (color != clDefault)
		*menu << clEnd;
}

void DisplaySong(const Song &s, void *data, Menu<Song> *menu)
{
	if (!s.Localized())
		const_cast<Song *>(&s)->Localize();
	
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
					if (get == &Song::GetLength)
					{
						if  (!s.GetTotalLength())
							break;
					}
					else if (get)
					{
						if ((s.*get)().empty())
							break;
					}
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
				if (it == song_template.end())
					break;
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
						buf << TO_WSTRING(s.GetAlbum());
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
		s.SetComment(f.tag()->comment().to8Bit(1));
#	endif // HAVE_TAGLIB_H
	
	info << fmtBold << Config.color1 << "Filename: " << fmtBoldEnd << Config.color2 << s.GetName() << "\n" << clEnd;
	info << fmtBold << "Directory: " << fmtBoldEnd << Config.color2 << ShowTagInInfoScreen(s.GetDirectory()) << "\n\n" << clEnd;
	info << fmtBold << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << "\n" << clEnd;
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		info << fmtBold << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps\n" << clEnd;
		info << fmtBold << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz\n" << clEnd;
		info << fmtBold << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << "\n" << clDefault;
	}
	else
		info << clDefault;
#	endif // HAVE_TAGLIB_H
	info << fmtBold << "\nTitle: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetTitle());
	info << fmtBold << "\nArtist: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetArtist());
	info << fmtBold << "\nAlbum: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetAlbum());
	info << fmtBold << "\nYear: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetYear());
	info << fmtBold << "\nTrack: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetTrack());
	info << fmtBold << "\nGenre: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetGenre());
	info << fmtBold << "\nComposer: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetComposer());
	info << fmtBold << "\nPerformer: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetPerformer());
	info << fmtBold << "\nDisc: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetDisc());
	info << fmtBold << "\nComment: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetComment());
}

Window &Statusbar()
{
	wFooter->GotoXY(0, Config.statusbar_visibility);
	wclrtoeol(wFooter->Raw());
	return *wFooter;
}

const Buffer &ShowTag(const string &tag)
{
	static Buffer result;
	result.Clear();
	if (tag.empty())
		result << Config.empty_tags_color << Config.empty_tag << clEnd;
	else
		result << tag;
	return result;
}

const basic_buffer<my_char_t> &ShowTagInInfoScreen(const string &tag)
{
#	ifdef _UTF8
	static WBuffer result;
	result.Clear();
	if (tag.empty())
		result << Config.empty_tags_color << ToWString(Config.empty_tag) << clEnd;
	else
		result << ToWString(tag);
	return result;
#	else
	return ShowTag(tag);
#	endif
}

void Scroller(Window &w, const string &string, size_t width, size_t &pos)
{
	std::basic_string<my_char_t> s = TO_WSTRING(string);
	size_t len;
#	ifdef _UTF8
	len = Window::Length(s);
#	else
	len = s.length();
#	endif
	
	if (len > width)
	{
#		ifdef _UTF8
		s += L" ** ";
#		else
		s += " ** ";
#		endif
		len = 0;
		std::basic_string<my_char_t>::const_iterator b = s.begin(), e = s.end();
		for (std::basic_string<my_char_t>::const_iterator it = b+pos; it < e && len < width; it++)
		{
#			ifdef _UTF8
			len += wcwidth(*it);
#			else
			len++;
#			endif
			w << *it;
		}
		if (++pos >= s.length())
			pos = 0;
		for (; len < width; b++)
		{
#			ifdef _UTF8
			len += wcwidth(*b);
#			else
			len++;
#			endif
			w << *b;
		}
	}
	else
		w << s;
}

