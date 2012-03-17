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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "charset.h"
#include "conv.h"
#include "error.h"
#include "song.h"

namespace
{
	unsigned calc_hash(const char *p)
	{
		unsigned hash = 5381;
		while (*p)
			hash = (hash << 5) + hash + *p++;
		return hash;
	}
}

MPD::Song::Song(mpd_song *s, bool copy_ptr) : 	itsSong(s),
						itsFile(0),
						itsTags(0),
						itsSlash(std::string::npos),
						itsHash(0),
						copyPtr(copy_ptr),
						isLocalised(0)
{
	if (itsSong)
		SetHashAndSlash();
}

MPD::Song::Song(const Song &s) : itsSong(s.copyPtr ? s.itsSong : mpd_song_dup(s.itsSong)),
				itsFile(s.itsFile ? strdup(s.itsFile) : 0),
				itsTags(s.itsTags ? new TagMap(*s.itsTags) : 0),
				itsNewName(s.itsNewName),
				itsSlash(s.itsSlash),
				itsHash(s.itsHash),
				copyPtr(s.copyPtr),
				isLocalised(s.isLocalised)
{
}

MPD::Song::~Song()
{
	if (itsSong)
		mpd_song_free(itsSong);
	delete [] itsFile;
	delete itsTags;
}

std::string MPD::Song::GetLength(unsigned pos) const
{
	if (pos > 0)
		return "";
	unsigned len = mpd_song_get_duration(itsSong);
	return !len ? "-:--" : ShowTime(len);
}

void MPD::Song::Localize()
{
#	ifdef HAVE_ICONV_H
	if (isLocalised)
		return;
	const char *tag, *conv_tag;
	conv_tag = tag = mpd_song_get_uri(itsSong);
	utf_to_locale(conv_tag, 0);
	if (tag != conv_tag) // file has been converted
	{
		itsFile = conv_tag;
		SetHashAndSlash();
	}
	for (unsigned t = MPD_TAG_ARTIST; t <= MPD_TAG_DISC; ++t)
	{
		unsigned pos = 0;
		for (; (tag = mpd_song_get_tag(itsSong, mpd_tag_type(t), pos)); ++pos)
		{
			conv_tag = tag;
			utf_to_locale(conv_tag, 0);
			if (tag != conv_tag) // tag has been converted
			{
				SetTag(mpd_tag_type(t), pos, conv_tag);
				delete [] conv_tag;
			}
		}
	}
	isLocalised = 1;
#	endif // HAVE_ICONV_H
}

void MPD::Song::Clear()
{
	if (itsSong)
		mpd_song_free(itsSong);
	itsSong = 0;
	
	delete [] itsFile;
	itsFile = 0;
	
	delete itsTags;
	itsTags = 0;
	
	itsNewName.clear();
	itsSlash = std::string::npos;
	itsHash = 0;
	isLocalised = 0;
	copyPtr = 0;
}

bool MPD::Song::Empty() const
{
	return !itsSong;
}

bool MPD::Song::isFromDB() const
{
	return (MyFilename()[0] != '/') || itsSlash == std::string::npos;
}

bool MPD::Song::isStream() const
{
	return !strncmp(MyFilename(), "http://", 7);
}

std::string MPD::Song::GetFile(unsigned pos) const
{
	return pos > 0 ? "" : MyFilename();
}

std::string MPD::Song::GetName(unsigned pos) const
{
	if (pos > 0)
		return "";
	std::string name = GetTag(MPD_TAG_NAME, 0);
	if (!name.empty())
		return name;
	else if (itsSlash != std::string::npos)
		return MyFilename()+itsSlash+1;
	else
		return MyFilename();
}

std::string MPD::Song::GetDirectory(unsigned pos) const
{
	if (pos > 0 || isStream())
		return "";
	else if (itsSlash == std::string::npos)
		return "/";
	else
		return std::string(MyFilename(), itsSlash);
}

std::string MPD::Song::GetArtist(unsigned pos) const
{
	return GetTag(MPD_TAG_ARTIST, pos);
}

std::string MPD::Song::GetTitle(unsigned pos) const
{
	return GetTag(MPD_TAG_TITLE, pos);
}

std::string MPD::Song::GetAlbum(unsigned pos) const
{
	return GetTag(MPD_TAG_ALBUM, pos);
}

std::string MPD::Song::GetAlbumArtist(unsigned pos) const
{
	return GetTag(MPD_TAG_ALBUM_ARTIST, pos);
}

std::string MPD::Song::GetTrack(unsigned pos) const
{
	std::string track = GetTag(MPD_TAG_TRACK, pos);
	return (track.length() == 1 && track[0] != '0') || (track.length() > 3 && track[1] == '/') ? "0"+track : track;
}

std::string MPD::Song::GetTrackNumber(unsigned pos) const
{
	std::string track = GetTag(MPD_TAG_TRACK, pos);
	size_t slash = track.find('/');
	if (slash != std::string::npos)
		track.resize(slash);
	return track.length() == 1 && track[0] != '0' ? "0"+track : track;
}

std::string MPD::Song::GetDate(unsigned pos) const
{
	return GetTag(MPD_TAG_DATE, pos);
}

std::string MPD::Song::GetGenre(unsigned pos) const
{
	return GetTag(MPD_TAG_GENRE, pos);
}

std::string MPD::Song::GetComposer(unsigned pos) const
{
	return GetTag(MPD_TAG_COMPOSER, pos);
}

std::string MPD::Song::GetPerformer(unsigned pos) const
{
	return GetTag(MPD_TAG_PERFORMER, pos);
}

std::string MPD::Song::GetDisc(unsigned pos) const
{
	return GetTag(MPD_TAG_DISC, pos);
}

std::string MPD::Song::GetComment(unsigned pos) const
{
	return GetTag(MPD_TAG_COMMENT, pos);
}

std::string MPD::Song::GetTags(GetFunction f) const
{
	unsigned pos = 0;
	std::string result;
	for (std::string tag; !(tag = (this->*f)(pos)).empty(); ++pos)
	{
		if (!result.empty())
			result += ", ";
		result += tag;
	}
	return result;
}

void MPD::Song::SetArtist(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_ARTIST, pos, str);
}

void MPD::Song::SetTitle(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_TITLE, pos, str);
}

void MPD::Song::SetAlbum(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_ALBUM, pos, str);
}

void MPD::Song::SetAlbumArtist(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_ALBUM_ARTIST, pos, str);
}

void MPD::Song::SetTrack(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_TRACK, pos, str);
}

void MPD::Song::SetTrack(unsigned track, unsigned pos)
{
	SetTag(MPD_TAG_TRACK, pos, IntoStr(track));
}

void MPD::Song::SetDate(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_DATE, pos, str);
}

void MPD::Song::SetDate(unsigned year, unsigned pos)
{
	SetTag(MPD_TAG_DATE, pos, IntoStr(year));
}

void MPD::Song::SetGenre(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_GENRE, pos, str);
}

void MPD::Song::SetComposer(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_COMPOSER, pos, str);
}

void MPD::Song::SetPerformer(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_PERFORMER, pos, str);
}

void MPD::Song::SetDisc(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_DISC, pos, str);
}

void MPD::Song::SetComment(const std::string &str, unsigned pos)
{
	SetTag(MPD_TAG_COMMENT, pos, str);
}

void MPD::Song::SetPosition(unsigned pos)
{
	mpd_song_set_pos(itsSong, pos);
}

void MPD::Song::SetTags(SetFunction f, const std::string &value)
{
	unsigned pos = 0;
	// tag editor can save multiple instances of performer and composer
	// tag, so we need to split them and allow it to read them separately.
	if (f == &Song::SetComposer || f == &Song::SetPerformer)
	{
		for (size_t i = 0; i != std::string::npos; i = value.find(",", i))
		{
			if (i)
				++i;
			while (value[i] == ' ')
				++i;
			size_t j = value.find(",", i);
			(this->*f)(value.substr(i, j-i), pos++);
		}
	}
	else
		(this->*f)(value, pos++);
	// there should be empty tag at the end since if we are
	// reading them, original tag from mpd_song at the position
	// after the last one locally set can be non-empty and in this
	// case GetTags() would read it, which is undesirable.
	(this->*f)("", pos);
}

std::string MPD::Song::ParseFormat(std::string::const_iterator &it, const char *escape_chars) const
{
	std::string result;
	bool has_some_tags = 0;
	MPD::Song::GetFunction get = 0;
	while (*++it != '}')
	{
		while (*it == '{')
		{
			std::string tags = ParseFormat(it, escape_chars);
			if (!tags.empty())
			{
				has_some_tags = 1;
				result += tags;
			}
		}
		if (*it == '}')
			break;
		
		if (*it == '%')
		{
			size_t delimiter = 0;
			if (isdigit(*++it))
			{
				delimiter = atol(&*it);
				while (isdigit(*++it)) { }
			}
			
			if (*it == '%')
			{
				result += *it;
				get = 0;
			}
			else
				get = toGetFunction(*it);
			
			if (get)
			{
				std::string tag = GetTags(get);
				if (escape_chars) // prepend format escape character to all given chars to escape
					for (const char *ch = escape_chars; *ch; ++ch)
						for (size_t i = 0; (i = tag.find(*ch, i)) != std::string::npos; i += 2)
							tag.replace(i, 1, std::string(1, FormatEscapeCharacter) + *ch);
				if (!tag.empty() && (get != &MPD::Song::GetLength || GetTotalLength()))
				{
					if (delimiter)
					{
						const std::basic_string<my_char_t> &s = TO_WSTRING(tag);
						if (NCurses::Window::Length(s) > delimiter)
							tag = Shorten(s, delimiter);
					}
					has_some_tags = 1;
					result += tag;
				}
				else
					break;
			}
		}
		else
			result += *it;
	}
	int brace_counter = 0;
	if (*it != '}' || !has_some_tags)
	{
		for (; *it != '}' || brace_counter; ++it)
		{
			if (*it == '{')
				++brace_counter;
			else if (*it == '}')
				--brace_counter;
		}
		if (*++it == '|')
			return ParseFormat(++it, escape_chars);
		else
			return "";
	}
	else
	{
		if (*(it+1) == '|')
		{
			for (; *it != '}' || *(it+1) == '|' || brace_counter; ++it)
			{
				if (*it == '{')
					++brace_counter;
				else if (*it == '}')
					--brace_counter;
			}
		}
		++it;
		return result;
	}
}

std::string MPD::Song::toString(const std::string &format, const char *escape_chars) const
{
	std::string::const_iterator it = format.begin();
	return ParseFormat(it, escape_chars);
}

MPD::Song &MPD::Song::operator=(const MPD::Song &s)
{
	if (this == &s)
		return *this;
	if (itsSong)
		mpd_song_free(itsSong);
	delete [] itsFile;
	delete itsTags;
	itsSong = s.copyPtr ? s.itsSong : (s.itsSong ? mpd_song_dup(s.itsSong) : 0);
	itsFile = s.itsFile ? strdup(s.itsFile) : 0;
	itsTags = s.itsTags ? new TagMap(*s.itsTags) : 0;
	itsNewName = s.itsNewName;
	itsSlash = s.itsSlash;
	itsHash = s.itsHash;
	copyPtr = s.copyPtr;
	isLocalised = s.isLocalised;
	return *this;
}

std::string MPD::Song::ShowTime(int length)
{
	std::ostringstream ss;
	
	int hours = length/3600;
	length -= hours*3600;
	int minutes = length/60;
	length -= minutes*60;
	int seconds = length;
	
	if (hours > 0)
	{
		ss << hours << ":"
		<< std::setw(2) << std::setfill('0') << minutes << ":"
		<< std::setw(2) << std::setfill('0') << seconds;
	}
	else
	{
		ss << minutes << ":"
		<< std::setw(2) << std::setfill('0') << seconds;
	}
	return ss.str();
}

bool MPD::Song::isFormatOk(const std::string &type, const std::string &s)
{
	int braces = 0;
	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
	{
		if (*it == '{')
			++braces;
		else if (*it == '}')
			--braces;
	}
	if (braces)
	{
		std::cerr << type << ": number of opening and closing braces does not equal!\n";
		return false;
	}
	
	for (size_t i = s.find('%'); i != std::string::npos; i = s.find('%', i))
	{
		if (isdigit(s[++i]))
			while (isdigit(s[++i])) { }
		if (!toGetFunction(s[i]))
		{
			std::cerr << type << ": invalid character at position " << IntoStr(i+1) << ": '" << s[i] << "'\n";
			return false;
		}
	}
	return true;
}

void MPD::Song::SetHashAndSlash()
{
	const char *filename = MyFilename();
	if (!isStream())
	{
		const char *tmp = strrchr(filename, '/');
		itsSlash = tmp ? tmp-filename : std::string::npos;
	}
	if (!itsFile)
		itsHash = calc_hash(filename);
}

const char *MPD::Song::MyFilename() const
{
	return itsFile ? itsFile : mpd_song_get_uri(itsSong);
}

void MPD::Song::SetTag(mpd_tag_type type, unsigned pos, const std::string &value)
{
	if (!itsTags)
		itsTags = new TagMap;
	(*itsTags)[std::make_pair(type, pos)] = value;
}

std::string MPD::Song::GetTag(mpd_tag_type type, unsigned pos) const
{
	if (itsTags)
	{
		TagMap::const_iterator it = itsTags->find(std::make_pair(type, pos));
		if (it != itsTags->end())
			return it->second;
	}
	const char *tag = mpd_song_get_tag(itsSong, type, pos);
	return tag ? tag : "";
}

