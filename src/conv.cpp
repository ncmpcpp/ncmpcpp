/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
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
#include "conv.h"

int StrToInt(const std::string &str)
{
	return atoi(str.c_str());
}

long StrToLong(const std::string &str)
{
	return atol(str.c_str());
}

std::string IntoStr(mpd_tag_type tag) // this is only for left column's title in media library
{
	switch (tag)
	{
		case MPD_TAG_ARTIST:
			return "Artist";
		case MPD_TAG_ALBUM:
			return "Album";
		case MPD_TAG_ALBUM_ARTIST:
			return "Album Artist";
		case MPD_TAG_TITLE:
			return "Title";
		case MPD_TAG_TRACK:
			return "Track";
		case MPD_TAG_GENRE:
			return "Genre";
		case MPD_TAG_DATE:
			return "Date";
		case MPD_TAG_COMPOSER:
			return "Composer";
		case MPD_TAG_PERFORMER:
			return "Performer";
		case MPD_TAG_COMMENT:
			return "Comment";
		case MPD_TAG_DISC:
			return "Disc";
		default:
			return "";
	}
}

std::string IntoStr(NCurses::Color color)
{
	std::string result;
	
	if (color == NCurses::clDefault)
		result = "default";
	else if (color == NCurses::clBlack)
		result = "black";
	else if (color == NCurses::clRed)
		result = "red";
	else if (color == NCurses::clGreen)
		result = "green";
	else if (color == NCurses::clYellow)
		result = "yellow";
	else if (color == NCurses::clBlue)
		result = "blue";
	else if (color == NCurses::clMagenta)
		result = "magenta";
	else if (color == NCurses::clCyan)
		result = "cyan";
	else if (color == NCurses::clWhite)
		result = "white";
	
	return result;
}

std::string IntoStr(const Action::Key &key, bool *print_backspace)
{
	std::string result;
	if (key == Action::Key(KEY_UP, ctNCurses))
		result += "Up";
	else if (key == Action::Key(KEY_DOWN, ctNCurses))
		result += "Down";
	else if (key == Action::Key(KEY_PPAGE, ctNCurses))
		result += "PageUp";
	else if (key == Action::Key(KEY_NPAGE, ctNCurses))
		result += "PageDown";
	else if (key == Action::Key(KEY_HOME, ctNCurses))
		result += "Home";
	else if (key == Action::Key(KEY_END, ctNCurses))
		result += "End";
	else if (key == Action::Key(KEY_SPACE, ctStandard))
		result += "Space";
	else if (key == Action::Key(KEY_ENTER, ctStandard))
		result += "Enter";
	else if (key == Action::Key(KEY_DC, ctNCurses))
		result += "Delete";
	else if (key == Action::Key(KEY_RIGHT, ctNCurses))
		result += "Right";
	else if (key == Action::Key(KEY_LEFT, ctNCurses))
		result += "Left";
	else if (key == Action::Key(KEY_TAB, ctStandard))
		result += "Tab";
	else if (key == Action::Key(KEY_SHIFT_TAB, ctNCurses))
		result += "Shift-Tab";
	else if (key >= Action::Key(KEY_CTRL_A, ctStandard) && key <= Action::Key(KEY_CTRL_Z, ctStandard))
	{
		result += "Ctrl-";
		result += key.getChar()+64;
	}
	else if (key >= Action::Key(KEY_F1, ctNCurses) && key <= Action::Key(KEY_F12, ctNCurses))
	{
		result += "F";
		result += intTo<std::string>::apply(key.getChar()-264);
	}
	else if ((key == Action::Key(KEY_BACKSPACE, ctNCurses) || key == Action::Key(KEY_BACKSPACE_2, ctStandard)))
	{
		// since some terminals interpret KEY_BACKSPACE as backspace and other need KEY_BACKSPACE_2,
		// actions have to be bound to either of them, but we want to display "Backspace" only once,
		// hance this 'print_backspace' switch.
		if (!print_backspace || *print_backspace)
		{
			result += "Backspace";
			if (print_backspace)
				*print_backspace = false;
		}
	}
	else
		result += ToString(std::wstring(1, key.getChar()));
	return result;
}

NCurses::Color IntoColor(const std::string &color)
{
	NCurses::Color result = NCurses::clDefault;
	
	if (color == "black")
		result = NCurses::clBlack;
	else if (color == "red")
		result = NCurses::clRed;
	else if (color == "green")
		result = NCurses::clGreen;
	else if (color == "yellow")
		result = NCurses::clYellow;
	else if (color == "blue")
		result = NCurses::clBlue;
	else if (color == "magenta")
		result = NCurses::clMagenta;
	else if (color == "cyan")
		result = NCurses::clCyan;
	else if (color == "white")
		result = NCurses::clWhite;
	
	return result;
}

mpd_tag_type IntoTagItem(char c)
{
	switch (c)
	{
		case 'a':
			return MPD_TAG_ARTIST;
		case 'A':
			return MPD_TAG_ALBUM_ARTIST;
		case 'b':
			return MPD_TAG_ALBUM;
		case 'y':
			return MPD_TAG_DATE;
		case 'g':
			return MPD_TAG_GENRE;
		case 'c':
			return MPD_TAG_COMPOSER;
		case 'p':
			return MPD_TAG_PERFORMER;
		default:
			return MPD_TAG_ARTIST;
	}
}

MPD::Song::GetFunction toGetFunction(char c)
{
	switch (c)
	{
		case 'l':
			return &MPD::Song::getLength;
		case 'D':
			return &MPD::Song::getDirectory;
		case 'f':
			return &MPD::Song::getName;
		case 'a':
			return &MPD::Song::getArtist;
		case 'A':
			return &MPD::Song::getAlbumArtist;
		case 'b':
			return &MPD::Song::getAlbum;
		case 'y':
			return &MPD::Song::getDate;
		case 'n':
			return &MPD::Song::getTrackNumber;
		case 'N':
			return &MPD::Song::getTrack;
		case 'g':
			return &MPD::Song::getGenre;
		case 'c':
			return &MPD::Song::getComposer;
		case 'p':
			return &MPD::Song::getPerformer;
		case 'd':
			return &MPD::Song::getDisc;
		case 'C':
			return &MPD::Song::getComment;
		case 't':
			return &MPD::Song::getTitle;
		case 'P':
			return &MPD::Song::getPriority;
		default:
			return 0;
	}
}

#ifdef HAVE_TAGLIB_H
MPD::MutableSong::SetFunction IntoSetFunction(mpd_tag_type tag)
{
	switch (tag)
	{
		case MPD_TAG_ARTIST:
			return &MPD::MutableSong::setArtist;
		case MPD_TAG_ALBUM:
			return &MPD::MutableSong::setAlbum;
		case MPD_TAG_ALBUM_ARTIST:
			return &MPD::MutableSong::setAlbumArtist;
		case MPD_TAG_TITLE:
			return &MPD::MutableSong::setTitle;
		case MPD_TAG_TRACK:
			return &MPD::MutableSong::setTrack;
		case MPD_TAG_GENRE:
			return &MPD::MutableSong::setGenre;
		case MPD_TAG_DATE:
			return &MPD::MutableSong::setDate;
		case MPD_TAG_COMPOSER:
			return &MPD::MutableSong::setComposer;
		case MPD_TAG_PERFORMER:
			return &MPD::MutableSong::setPerformer;
		case MPD_TAG_COMMENT:
			return &MPD::MutableSong::setComment;
		case MPD_TAG_DISC:
			return &MPD::MutableSong::setDisc;
		default:
			return 0;
	}
}
#endif // HAVE_TAGLIB_H

std::string Shorten(const std::basic_string<my_char_t> &s, size_t max_length)
{
	if (s.length() <= max_length)
		return TO_STRING(s);
	if (max_length < 2)
		return "";
	std::basic_string<my_char_t> result(s, 0, max_length/2-!(max_length%2));
	result += U("..");
	result += s.substr(s.length()-max_length/2+1);
	return TO_STRING(result);
}

void EscapeUnallowedChars(std::string &s)
{
	static const std::string unallowed_chars = "\"*/:<>?\\|";
	
	for (std::string::const_iterator it = unallowed_chars.begin(); it != unallowed_chars.end(); ++it)
	{
		for (size_t i = 0; i < s.length(); ++i)
		{
			if (s[i] == *it)
			{
				s.erase(s.begin()+i);
				i--;
			}
		}
	}
}
