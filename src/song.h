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

#ifndef _SONG_H
#define _SONG_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <mpd/client.h>

namespace MPD {//

struct Song
{
	typedef std::string (Song::*GetFunction)(unsigned) const;
	
	Song() { }
	virtual ~Song() { }
	
	Song(mpd_song *s);
	
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
	
	virtual std::string getTags(GetFunction f, const std::string &tags_separator) const;
	
	virtual unsigned getHash() const;
	virtual unsigned getDuration() const;
	virtual unsigned getPosition() const;
	virtual unsigned getID() const;
	virtual unsigned getPrio() const;
	virtual time_t getMTime() const;
	
	virtual bool isFromDatabase() const;
	virtual bool isStream() const;
	
	virtual bool empty() const;
	
	virtual std::string toString(const std::string &fmt, const std::string &tags_separator,
	                             const std::string &escape_chars = "") const;
	
	bool operator==(const Song &rhs) const { return m_hash == rhs.m_hash; }
	bool operator!=(const Song &rhs) const { return m_hash != rhs.m_hash; }
	
	static std::string ShowTime(unsigned length);
	static bool isFormatOk(const std::string &type, const std::string &fmt);
	
	static const char FormatEscapeCharacter = 1;
	
	const char *getTag(mpd_tag_type type, unsigned idx = 0) const;

private:
	std::string ParseFormat(std::string::const_iterator &it, const std::string &tags_separator,
							const std::string &escape_chars) const;
	
	std::shared_ptr<mpd_song> m_song;
	size_t m_hash;
};

typedef std::vector<Song> SongList;

}

#endif

