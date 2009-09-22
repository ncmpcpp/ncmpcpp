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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>
#include <iomanip>
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
						itsSlash(std::string::npos),
						itsHash(0),
						copyPtr(copy_ptr),
						isLocalised(0)
{
	if (itsSong)
		SetHashAndSlash();
}

MPD::Song::Song(const Song &s) : itsSong(0),
				itsNewName(s.itsNewName),
				itsSlash(s.itsSlash),
				itsHash(s.itsHash),
				copyPtr(s.copyPtr),
				isLocalised(s.isLocalised)
{
	itsSong = s.copyPtr ? s.itsSong : mpd_song_dup(s.itsSong);
}

MPD::Song::~Song()
{
	if (itsSong)
		mpd_song_free(itsSong);
}

std::string MPD::Song::GetLength() const
{
	unsigned len = mpd_song_get_duration(itsSong);
	return !len ? "-:--" : ShowTime(len);
}

void MPD::Song::Localize()
{
#	ifdef HAVE_ICONV_H
	if (isLocalised)
		return;
	str_pool_utf_to_locale(itsSong->file);
	SetHashAndSlash();
	str_pool_utf_to_locale(itsSong->artist);
	str_pool_utf_to_locale(itsSong->title);
	str_pool_utf_to_locale(itsSong->album);
	str_pool_utf_to_locale(itsSong->track);
	str_pool_utf_to_locale(itsSong->name);
	str_pool_utf_to_locale(itsSong->date);
	str_pool_utf_to_locale(itsSong->genre);
	str_pool_utf_to_locale(itsSong->composer);
	str_pool_utf_to_locale(itsSong->performer);
	str_pool_utf_to_locale(itsSong->disc);
	str_pool_utf_to_locale(itsSong->comment);
	isLocalised = 1;
#	endif // HAVE_ICONV_H
}

void MPD::Song::Clear()
{
	if (itsSong)
		mpd_song_free(itsSong);
	itsSong = 0;
	itsNewName.clear();
	itsSlash = std::string::npos;
	itsHash = 0;
	isLocalised = 0;
	copyPtr = 0;
}

bool MPD::Song::Empty() const
{
	return !itsSong;// || (!itsSong->file && !itsSong->title && !itsSong->artist && !itsSong->album && !itsSong->date && !itsSong->track && !itsSong->genre && !itsSong->composer && !itsSong->performer && !itsSong->disc && !itsSong->comment);
}

bool MPD::Song::isFromDB() const
{
	return (mpd_song_get_uri(itsSong)[0] != '/') || itsSlash == std::string::npos;
}

bool MPD::Song::isStream() const
{
	return !strncmp(mpd_song_get_uri(itsSong), "http://", 7);
}

std::string MPD::Song::GetFile() const
{
	return mpd_song_get_uri(itsSong);
}

std::string MPD::Song::GetName() const
{
	if (const char *name = mpd_song_get_tag(itsSong, MPD_TAG_NAME, 0))
		return name;
	else if (itsSlash != std::string::npos)
		return mpd_song_get_uri(itsSong)+itsSlash+1;
	else
		return mpd_song_get_uri(itsSong);
}

std::string MPD::Song::GetDirectory() const
{
	if (isStream())
		return "";
	else if (itsSlash == std::string::npos)
		return "/";
	else
		return std::string(mpd_song_get_uri(itsSong), itsSlash);
}

std::string MPD::Song::GetArtist() const
{
	if (const char *artist = mpd_song_get_tag(itsSong, MPD_TAG_ARTIST, 0))
		return artist;
	else
		return "";
}

std::string MPD::Song::GetTitle() const
{
	if (const char *title = mpd_song_get_tag(itsSong, MPD_TAG_TITLE, 0))
		return title;
	else
		return "";
}

std::string MPD::Song::GetAlbum() const
{
	if (const char *album = mpd_song_get_tag(itsSong, MPD_TAG_ALBUM, 0))
		return album;
	else
		return "";
}

std::string MPD::Song::GetTrack() const
{
	const char *track = mpd_song_get_tag(itsSong, MPD_TAG_TRACK, 0);
	if (!track)
		return "";
	else if (track[0] != '0' && !track[1])
		return "0"+std::string(track);
	else
		return track;
}

std::string MPD::Song::GetTrackNumber() const
{
	const char *track = mpd_song_get_tag(itsSong, MPD_TAG_TRACK, 0);
	if (!track)
		return "";
	const char *slash = strrchr(track, '/');
	if (slash)
	{
		std::string result(track, slash-track);
		return result[0] != '0' && result.length() == 1 ? "0"+result : result;
	}
	else
		return GetTrack();
}

std::string MPD::Song::GetDate() const
{
	if (const char *date = mpd_song_get_tag(itsSong, MPD_TAG_DATE, 0))
		return date;
	else
		return "";
}

std::string MPD::Song::GetGenre() const
{
	if (const char *genre = mpd_song_get_tag(itsSong, MPD_TAG_GENRE, 0))
		return genre;
	else
		return "";
}

std::string MPD::Song::GetComposer() const
{
	if (const char *composer = mpd_song_get_tag(itsSong, MPD_TAG_COMPOSER, 0))
		return composer;
	else
		return "";
}

std::string MPD::Song::GetPerformer() const
{
	if (const char *performer = mpd_song_get_tag(itsSong, MPD_TAG_PERFORMER, 0))
		return performer;
	else
		return "";
}

std::string MPD::Song::GetDisc() const
{
	if (const char *disc = mpd_song_get_tag(itsSong, MPD_TAG_DISC, 0))
		return disc;
	else
		return "";
}

std::string MPD::Song::GetComment() const
{
	if (const char *comment = mpd_song_get_tag(itsSong, MPD_TAG_COMMENT, 0))
		return comment;
	else
		return "";
}

/*void MPD::Song::SetFile(const std::string &str)
{
	if (itsSong->file)
		str_pool_put(itsSong->file);
	itsSong->file = str.empty() ? 0 : str_pool_get(str.c_str());
	SetHashAndSlash();
}*/

void MPD::Song::SetArtist(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_ARTIST);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_ARTIST, str.c_str());
}

void MPD::Song::SetTitle(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_TITLE);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_TITLE, str.c_str());
}

void MPD::Song::SetAlbum(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_ALBUM);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_ALBUM, str.c_str());
}

void MPD::Song::SetTrack(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_TRACK);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_TRACK, str.c_str());
}

void MPD::Song::SetTrack(unsigned track)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_TRACK);
	if (track)
		mpd_song_add_tag(itsSong, MPD_TAG_ARTIST, IntoStr(track).c_str());
}

void MPD::Song::SetDate(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_DATE);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_DATE, str.c_str());
}

void MPD::Song::SetDate(unsigned year)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_DATE);
	if (year)
		mpd_song_add_tag(itsSong, MPD_TAG_DATE, IntoStr(year).c_str());
}

void MPD::Song::SetGenre(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_GENRE);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_GENRE, str.c_str());
}

void MPD::Song::SetComposer(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_COMPOSER);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_COMPOSER, str.c_str());
}

void MPD::Song::SetPerformer(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_PERFORMER);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_PERFORMER, str.c_str());
}

void MPD::Song::SetDisc(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_DISC);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_DISC, str.c_str());
}

void MPD::Song::SetComment(const std::string &str)
{
	mpd_song_clear_tag(itsSong, MPD_TAG_COMMENT);
	if (!str.empty())
		mpd_song_add_tag(itsSong, MPD_TAG_COMMENT, str.c_str());
}

void MPD::Song::SetPosition(unsigned pos)
{
	mpd_song_set_pos(itsSong, pos);
}

std::string MPD::Song::ParseFormat(std::string::const_iterator &it) const
{
	std::string result;
	bool has_some_tags = 0;
	MPD::Song::GetFunction get = 0;
	while (*++it != '}')
	{
		while (*it == '{')
		{
			std::string tags = ParseFormat(it);
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
			switch (*++it)
			{
				case 'l':
					get = &MPD::Song::GetLength;
					break;
				case 'D':
					get = &MPD::Song::GetDirectory;
					break;
				case 'f':
					get = &MPD::Song::GetName;
					break;
				case 'a':
					get = &MPD::Song::GetArtist;
					break;
				case 'b':
					get = &MPD::Song::GetAlbum;
					break;
				case 'y':
					get = &MPD::Song::GetDate;
					break;
				case 'n':
					get = &MPD::Song::GetTrackNumber;
					break;
				case 'N':
					get = &MPD::Song::GetTrack;
					break;
				case 'g':
					get = &MPD::Song::GetGenre;
					break;
				case 'c':
					get = &MPD::Song::GetComposer;
					break;
				case 'p':
					get = &MPD::Song::GetPerformer;
					break;
				case 'd':
					get = &MPD::Song::GetDisc;
					break;
				case 'C':
					get = &MPD::Song::GetComment;
					break;
				case 't':
					get = &MPD::Song::GetTitle;
					break;
				case '%':
					result += *it; // no break here
				default:
					get = 0;
					break;
			}
			if (get)
			{
				std::string tag = (this->*get)();
				if (!tag.empty() && (get != &MPD::Song::GetLength || GetTotalLength()))
				{
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
			return ParseFormat(++it);
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

std::string MPD::Song::toString(const std::string &format) const
{
	std::string::const_iterator it = format.begin();
	return ParseFormat(it);
}

MPD::Song &MPD::Song::operator=(const MPD::Song &s)
{
	if (this == &s)
		return *this;
	if (itsSong)
		mpd_song_free(itsSong);
	itsSong = s.copyPtr ? s.itsSong : (s.itsSong ? mpd_song_dup(s.itsSong) : 0);
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

void MPD::Song::ValidateFormat(const std::string &type, const std::string &s)
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
		FatalError(type + ": number of opening and closing braces does not equal!");
}

void MPD::Song::SetHashAndSlash()
{
	const char *filename = mpd_song_get_uri(itsSong);
	if (!isStream())
	{
		const char *tmp = strrchr(filename, '/');
		itsSlash = tmp ? tmp-filename : std::string::npos;
	}
	itsHash = calc_hash(filename);
}

