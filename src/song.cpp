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

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

#include "song.h"
#include "utility/numeric_conversions.h"
#include "utility/type_conversions.h"
#include "window.h"

namespace {//

size_t calc_hash(const char* s, unsigned seed = 0)
{
	size_t hash = seed;
	while (*s)
		hash = hash * 101  +  *s++;
	return hash;
}

// temporary hack, it won't work properly with wide characters
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

}

namespace MPD {//

const char *Song::getTag(mpd_tag_type type, unsigned idx) const
{
	const char *tag = mpd_song_get_tag(m_song.get(), type, idx);
	return tag ? tag : "";
}

Song::Song(mpd_song *s)
{
	assert(s);
	m_song = std::shared_ptr<mpd_song>(s, mpd_song_free);
	m_hash = calc_hash(mpd_song_get_uri(s));
}

std::string Song::getURI(unsigned idx) const
{
	assert(m_song);
	if (idx > 0)
		return "";
	else
		return mpd_song_get_uri(m_song.get());
}

std::string Song::getName(unsigned idx) const
{
	assert(m_song);
	mpd_song *s = m_song.get();
	const char *res = mpd_song_get_tag(s, MPD_TAG_NAME, idx);
	if (!res && idx > 0)
		return "";
	const char *uri = mpd_song_get_uri(s);
	const char *name = strrchr(uri, '/');
	if (name)
		return name+1;
	else
		return uri;
}

std::string Song::getDirectory(unsigned idx) const
{
	assert(m_song);
	if (idx > 0 || isStream())
		return "";
	const char *uri = mpd_song_get_uri(m_song.get());
	const char *name = strrchr(uri, '/');
	if (name)
		return std::string(uri, name-uri);
	else
		return "/";
}

std::string Song::getArtist(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_ARTIST, idx);
}

std::string Song::getTitle(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_TITLE, idx);
}

std::string Song::getAlbum(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_ALBUM, idx);
}

std::string Song::getAlbumArtist(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_ALBUM_ARTIST, idx);
}

std::string Song::getTrack(unsigned idx) const
{
	assert(m_song);
	std::string track = getTag(MPD_TAG_TRACK, idx);
	if ((track.length() == 1 && track[0] != '0')
	||  (track.length() > 3  && track[1] == '/'))
		return "0"+track;
	else
		return track;
}

std::string Song::getTrackNumber(unsigned idx) const
{
	assert(m_song);
	std::string track = getTrack(idx);
	size_t slash = track.find('/');
	if (slash != std::string::npos)
		track.resize(slash);
	return track;
}

std::string Song::getDate(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_DATE, idx);
}

std::string Song::getGenre(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_GENRE, idx);
}

std::string Song::getComposer(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_COMPOSER, idx);
}

std::string Song::getPerformer(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_PERFORMER, idx);
}

std::string Song::getDisc(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_DISC, idx);
}

std::string Song::getComment(unsigned idx) const
{
	assert(m_song);
	return getTag(MPD_TAG_COMMENT, idx);
}

std::string Song::getLength(unsigned idx) const
{
	assert(m_song);
	if (idx > 0)
		return "";
	unsigned len = getDuration();
	if (len > 0)
		return ShowTime(len);
	else
		return "-:--";
}

std::string Song::getPriority(unsigned idx) const
{
	assert(m_song);
	if (idx > 0)
		return "";
	return unsignedIntTo<std::string>::apply(getPrio());
}

std::string MPD::Song::getTags(GetFunction f, const std::string &tag_separator) const
{
	assert(m_song);
	unsigned idx = 0;
	std::string result;
	for (std::string tag; !(tag = (this->*f)(idx)).empty(); ++idx)
	{
		if (!result.empty())
			result += tag_separator;
		result += tag;
	}
	return result;
}

unsigned Song::getHash() const
{
	assert(m_song);
	return m_hash;
}

unsigned Song::getDuration() const
{
	assert(m_song);
	return mpd_song_get_duration(m_song.get());
}

unsigned Song::getPosition() const
{
	assert(m_song);
	return mpd_song_get_pos(m_song.get());
}

unsigned Song::getID() const
{
	assert(m_song);
	return mpd_song_get_id(m_song.get());
}

unsigned Song::getPrio() const
{
	assert(m_song);
	return mpd_song_get_prio(m_song.get());
}

time_t Song::getMTime() const
{
	assert(m_song);
	return mpd_song_get_last_modified(m_song.get());
}

bool Song::isFromDatabase() const
{
	assert(m_song);
	const char *uri = mpd_song_get_uri(m_song.get());
	return uri[0] != '/' || !strrchr(uri, '/');
}

bool Song::isStream() const
{
	assert(m_song);
	return !strncmp(mpd_song_get_uri(m_song.get()), "http://", 7);
}

bool Song::empty() const
{
	return m_song.get() == 0;
}

std::string Song::toString(const std::string &fmt, const std::string &tag_separator, const std::string &escape_chars) const
{
	assert(m_song);
	std::string::const_iterator it = fmt.begin();
	return ParseFormat(it, tag_separator, escape_chars);
}

std::string Song::ShowTime(unsigned length)
{
	int hours = length/3600;
	length -= hours*3600;
	int minutes = length/60;
	length -= minutes*60;
	int seconds = length;
	
	if (hours > 0)
		return print<32, std::string>::apply("%d:%02d:%02d", hours, minutes, seconds);
	else
		return print<32, std::string>::apply("%d:%02d", minutes, seconds);
}

bool MPD::Song::isFormatOk(const std::string &type, const std::string &fmt)
{
	int braces = 0;
	for (std::string::const_iterator it = fmt.begin(); it != fmt.end(); ++it)
	{
		if (*it == '{')
			++braces;
		else if (*it == '}')
			--braces;
	}
	if (braces)
	{
		std::cerr << type << ": number of opening and closing braces does not equal\n";
		return false;
	}
	
	for (size_t i = fmt.find('%'); i != std::string::npos; i = fmt.find('%', i))
	{
		if (isdigit(fmt[++i]))
			while (isdigit(fmt[++i])) { }
		if (!charToGetFunction(fmt[i]))
		{
			std::cerr << type << ": invalid character at position " << unsignedLongIntTo<std::string>::apply(i+1) << ": '" << fmt[i] << "'\n";
			return false;
		}
	}
	return true;
}

std::string Song::ParseFormat(std::string::const_iterator &it, const std::string &tag_separator, const std::string &escape_chars) const
{
	std::string result;
	bool has_some_tags = 0;
	MPD::Song::GetFunction get = 0;
	while (*++it != '}')
	{
		while (*it == '{')
		{
			std::string tags = ParseFormat(it, tag_separator, escape_chars);
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
				get = charToGetFunction(*it);
			
			if (get)
			{
				std::string tag = getTags(get, tag_separator);
				if (!escape_chars.empty()) // prepend format escape character to all given chars to escape
				{
					for (size_t i = 0; i < escape_chars.length(); ++i)
						for (size_t j = 0; (j = tag.find(escape_chars[i], j)) != std::string::npos; j += 2)
							tag.replace(j, 1, std::string(1, FormatEscapeCharacter) + escape_chars[i]);
				}
				if (!tag.empty() && (get != &MPD::Song::getLength || getDuration() > 0))
				{
					if (delimiter)
					{
						const std::basic_string<my_char_t> &s = TO_WSTRING(tag);
						if (NC::Window::length(s) > delimiter)
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
			return ParseFormat(++it, tag_separator, escape_chars);
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

}
