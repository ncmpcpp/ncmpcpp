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

using MPD::Song;
using std::string;

string Display::Columns(string st)
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

void Display::StringPairs(const string_pair &pair, void *, Menu<string_pair> *menu)
{
	*menu << pair.first;
}

void Display::SongsInColumns(const MPD::Song &s, void *s_template, Menu<MPD::Song> *menu)
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

void Display::Songs(const MPD::Song &s, void *data, Menu<MPD::Song> *menu)
{
	if (!s.Localized())
		const_cast<MPD::Song *>(&s)->Localize();
	
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

void Display::Tags(const MPD::Song &s, void *data, Menu<MPD::Song> *menu)
{
	switch (static_cast<Menu<string> *>(data)->Choice())
	{
		case 0:
			*menu << ShowTag(s.GetTitle());
			return;
		case 1:
			*menu << ShowTag(s.GetArtist());
			return;
		case 2:
			*menu << ShowTag(s.GetAlbum());
			return;
		case 3:
			*menu << ShowTag(s.GetYear());
			return;
		case 4:
			*menu << ShowTag(s.GetTrack());
			return;
		case 5:
			*menu << ShowTag(s.GetGenre());
			return;
		case 6:
			*menu << ShowTag(s.GetComposer());
			return;
		case 7:
			*menu << ShowTag(s.GetPerformer());
			return;
		case 8:
			*menu << ShowTag(s.GetDisc());
			return;
		case 9:
			*menu << ShowTag(s.GetComment());
			return;
		case 11:
			if (s.GetNewName().empty())
				*menu << s.GetName();
			else
				*menu << s.GetName() << Config.color2 << " -> " << clEnd << s.GetNewName();
			return;
		default:
			return;
	}
}

void Display::Items(const MPD::Item &item, void *, Menu<MPD::Item> *menu)
{
	switch (item.type)
	{
		case MPD::itDirectory:
		{
			if (item.song)
			{
				*menu << "[..]";
				return;
			}
			size_t slash = item.name.rfind("/");
			*menu << "[" << (slash != string::npos ? item.name.substr(slash+1) : item.name) << "]";
			return;
		}
		case MPD::itSong:
			if (!Config.columns_in_browser)
				Display::Songs(*item.song, &Config.song_list_format, reinterpret_cast<Menu<MPD::Song> *>(menu));
			else
				Display::SongsInColumns(*item.song, &Config.song_columns_list_format, reinterpret_cast<Menu<MPD::Song> *>(menu));
			return;
		case MPD::itPlaylist:
			*menu << Config.browser_playlist_prefix << item.name;
			return;
		default:
			return;
	}
}

void Display::SearchEngine(const std::pair<Buffer *, Song *> &pair, void *, Menu< std::pair<Buffer *, Song *> > *menu)
{
	if (pair.second)
	{
		if (!Config.columns_in_search_engine)
			Display::Songs(*pair.second, &Config.song_list_format, reinterpret_cast<Menu<Song> *>(menu));
		else
			Display::SongsInColumns(*pair.second, &Config.song_columns_list_format, reinterpret_cast<Menu<Song> *>(menu));
	}
	
	else
		*menu << *pair.first;
}

