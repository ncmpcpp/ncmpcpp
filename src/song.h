/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#ifndef NCMPCPP_SONG_H
#define NCMPCPP_SONG_H

#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <mpd/client.h>

namespace MPD {

struct Song
{
	struct Hash {
		size_t operator()(const Song &s) const { return s.m_hash; }
	};

	typedef std::string (Song::*GetFunction)(unsigned) const;
	
	Song() : m_hash(0) { }
	virtual ~Song() { }
	
	Song(mpd_song *s);

	Song(const Song &rhs) : m_song(rhs.m_song), m_hash(rhs.m_hash) { }
	Song(Song &&rhs) : m_song(std::move(rhs.m_song)), m_hash(rhs.m_hash) { }
	Song &operator=(Song rhs)
	{
		m_song = std::move(rhs.m_song);
		m_hash = rhs.m_hash;
		return *this;
	}
	
	std::string get(mpd_tag_type type, unsigned idx = 0) const;
	
	virtual std::string getURI(unsigned idx = 0) const;
	virtual std::string getName(unsigned idx = 0) const;
	virtual std::string getDirectory(unsigned idx = 0) const;
	virtual std::string getArtist(unsigned idx = 0) const;
	virtual std::string getTitle(unsigned idx = 0) const;
	virtual std::string getAlbum(unsigned idx = 0) const;
	virtual std::string getAlbumArtist(unsigned idx = 0) const;
	virtual std::string getTrack(unsigned idx = 0) const;
	virtual std::string getTrackNumber(unsigned idx = 0) const;
	virtual std::string getDate(unsigned idx = 0) const;
	virtual std::string getGenre(unsigned idx = 0) const;
	virtual std::string getComposer(unsigned idx = 0) const;
	virtual std::string getPerformer(unsigned idx = 0) const;
	virtual std::string getDisc(unsigned idx = 0) const;
	virtual std::string getComment(unsigned idx = 0) const;
	virtual std::string getLength(unsigned idx = 0) const;
	virtual std::string getPriority(unsigned idx = 0) const;
	
	virtual std::string getTags(GetFunction f) const;
	
	virtual unsigned getDuration() const;
	virtual unsigned getPosition() const;
	virtual unsigned getID() const;
	virtual unsigned getPrio() const;
	virtual time_t getMTime() const;
	
	virtual bool isFromDatabase() const;
	virtual bool isStream() const;
	
	virtual bool empty() const;
	
	bool operator==(const Song &rhs) const
	{
		if (m_hash != rhs.m_hash)
			return false;
		return strcmp(c_uri(), rhs.c_uri()) == 0;
	}
	bool operator!=(const Song &rhs) const
	{
		return !(operator==(rhs));
	}

	const char *c_uri() const { return m_song ? mpd_song_get_uri(m_song.get()) : ""; }

	static std::string ShowTime(unsigned length);

	static std::string TagsSeparator;

	static bool ShowDuplicateTags;

private:
	std::shared_ptr<mpd_song> m_song;
	size_t m_hash;
};

}

#endif // NCMPCPP_SONG_H

