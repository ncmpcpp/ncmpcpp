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
#include "song.h"

MPD::Song::Song(mpd_Song *s, bool copy_ptr) : 	itsSong(s ? s : mpd_newSong()),
						itsSlash(std::string::npos),
						itsHash(0),
						copyPtr(copy_ptr),
						isLocalised(0)
{
	if (itsSong->file)
		SetHashAndSlash();
}

MPD::Song::Song(const Song &s) : itsSong(0),
				itsNewName(s.itsNewName),
				itsSlash(s.itsSlash),
				itsHash(s.itsHash),
				copyPtr(s.copyPtr),
				isLocalised(s.isLocalised)
{
	itsSong = s.copyPtr ? s.itsSong : mpd_songDup(s.itsSong);
}

MPD::Song::~Song()
{
	if (itsSong)
		mpd_freeSong(itsSong);
}

std::string MPD::Song::GetLength() const
{
	return itsSong->time <= 0 ? "-:--" : ShowTime(itsSong->time);
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
		mpd_freeSong(itsSong);
	itsSong = mpd_newSong();
	itsNewName.clear();
	itsSlash = std::string::npos;
	itsHash = 0;
	isLocalised = 0;
	copyPtr = 0;
}

bool MPD::Song::Empty() const
{
	return !itsSong || (!itsSong->file && !itsSong->title && !itsSong->artist && !itsSong->album && !itsSong->date && !itsSong->track && !itsSong->genre && !itsSong->composer && !itsSong->performer && !itsSong->disc && !itsSong->comment);
}

bool MPD::Song::isFromDB() const
{
	return (itsSong->file && itsSong->file[0] != '/') || itsSlash == std::string::npos;
}

bool MPD::Song::isStream() const
{
	return !strncmp(itsSong->file, "http://", 7);
}

std::string MPD::Song::GetFile() const
{
	return !itsSong->file ? "" : itsSong->file;
}

std::string MPD::Song::GetName() const
{
	if (itsSong->name)
		return itsSong->name;
	else if (!itsSong->file)
		return "";
	else if (itsSlash != std::string::npos)
		return itsSong->file+itsSlash+1;
	else
		return itsSong->file;
}

std::string MPD::Song::GetDirectory() const
{
	if (!itsSong->file || isStream())
		return "";
	else if (itsSlash == std::string::npos)
		return "/";
	else
		return std::string(itsSong->file, itsSlash);
}

std::string MPD::Song::GetArtist() const
{
	return !itsSong->artist ? "" : itsSong->artist;
}

std::string MPD::Song::GetTitle() const
{
	return !itsSong->title ? "" : itsSong->title;
}

std::string MPD::Song::GetAlbum() const
{
	return !itsSong->album ? "" : itsSong->album;
}

std::string MPD::Song::GetTrack() const
{
	if (!itsSong->track)
		return "";
	else if (itsSong->track[0] != '0' && !itsSong->track[1])
		return "0"+std::string(itsSong->track);
	else
		return itsSong->track;
}

std::string MPD::Song::GetTrackNumber() const
{
	if (!itsSong->track)
		return "";
	const char *slash = strrchr(itsSong->track, '/');
	if (slash)
	{
		std::string result(itsSong->track, slash-itsSong->track);
		return result[0] != '0' && result.length() == 1 ? "0"+result : result;
	}
	else
		return GetTrack();
}

std::string MPD::Song::GetDate() const
{
	return !itsSong->date ? "" : itsSong->date;
}

std::string MPD::Song::GetGenre() const
{
	return !itsSong->genre ? "" : itsSong->genre;
}

std::string MPD::Song::GetComposer() const
{
	return !itsSong->composer ? "" : itsSong->composer;
}

std::string MPD::Song::GetPerformer() const
{
	return !itsSong->performer ? "" : itsSong->performer;
}

std::string MPD::Song::GetDisc() const
{
	return !itsSong->disc ? "" : itsSong->disc;
}

std::string MPD::Song::GetComment() const
{
	return !itsSong->comment ? "" : itsSong->comment;
}

void MPD::Song::SetFile(const std::string &str)
{
	if (itsSong->file)
		str_pool_put(itsSong->file);
	itsSong->file = str.empty() ? 0 : str_pool_get(str.c_str());
	SetHashAndSlash();
}

void MPD::Song::SetArtist(const std::string &str)
{
	if (itsSong->artist)
		str_pool_put(itsSong->artist);
	itsSong->artist = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetTitle(const std::string &str)
{
	if (itsSong->title)
		str_pool_put(itsSong->title);
	itsSong->title = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetAlbum(const std::string &str)
{
	if (itsSong->album)
		str_pool_put(itsSong->album);
	itsSong->album = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetTrack(const std::string &str)
{
	if (itsSong->track)
		str_pool_put(itsSong->track);
	itsSong->track = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetTrack(int track)
{
	if (itsSong->track)
		str_pool_put(itsSong->track);
	itsSong->track = str_pool_get(IntoStr(track).c_str());
}

void MPD::Song::SetDate(const std::string &str)
{
	if (itsSong->date)
		str_pool_put(itsSong->date);
	itsSong->date = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetDate(int year)
{
	if (itsSong->date)
		str_pool_put(itsSong->date);
	itsSong->date = str_pool_get(IntoStr(year).c_str());
}

void MPD::Song::SetGenre(const std::string &str)
{
	if (itsSong->genre)
		str_pool_put(itsSong->genre);
	itsSong->genre = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetComposer(const std::string &str)
{
	if (itsSong->composer)
		str_pool_put(itsSong->composer);
	itsSong->composer = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetPerformer(const std::string &str)
{
	if (itsSong->performer)
		str_pool_put(itsSong->performer);
	itsSong->performer = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetDisc(const std::string &str)
{
	if (itsSong->disc)
		str_pool_put(itsSong->disc);
	itsSong->disc = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetComment(const std::string &str)
{
	if (itsSong->comment)
		str_pool_put(itsSong->comment);
	itsSong->comment = str.empty() ? 0 : str_pool_get(str.c_str());
}

void MPD::Song::SetPosition(int pos)
{
	itsSong->pos = pos;
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
				default:
					result += *it;
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
		mpd_freeSong(itsSong);
	itsSong = s.copyPtr ? s.itsSong : mpd_songDup(s.itsSong);
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
		throw std::runtime_error(type + ": number of opening and closing braces does not equal!");
}

void MPD::Song::SetHashAndSlash()
{
	if (!itsSong->file)
		return;
	if (!isStream())
	{
		const char *tmp = strrchr(itsSong->file, '/');
		itsSlash = tmp ? tmp-itsSong->file : std::string::npos;
	}
	itsHash = calc_hash(itsSong->file);
}

