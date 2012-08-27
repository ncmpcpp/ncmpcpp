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

#include <mpd/client.h>

namespace MPD {//

struct Song
{
	typedef std::string (Song::*GetFunction)(unsigned) const;
	
	Song() { }
	Song(mpd_song *s);
	
	std::string getURI(unsigned idx = 0) const;
	std::string getName(unsigned idx = 0) const;
	std::string getDirectory(unsigned idx = 0) const;
	std::string getArtist(unsigned idx = 0) const;
	std::string getTitle(unsigned idx = 0) const;
	std::string getAlbum(unsigned idx = 0) const;
	std::string getAlbumArtist(unsigned idx = 0) const;
	std::string getTrack(unsigned idx = 0) const;
	std::string getTrackNumber(unsigned idx = 0) const;
	std::string getDate(unsigned idx = 0) const;
	std::string getGenre(unsigned idx = 0) const;
	std::string getComposer(unsigned idx = 0) const;
	std::string getPerformer(unsigned idx = 0) const;
	std::string getDisc(unsigned idx = 0) const;
	std::string getComment(unsigned idx = 0) const;
	std::string getLength(unsigned idx = 0) const;
	std::string getPriority(unsigned idx = 0) const;
	
	std::string getTags(GetFunction f, const std::string tag_separator = ", ") const;
	
	unsigned getHash() const;
	unsigned getDuration() const;
	unsigned getPosition() const;
	unsigned getID() const;
	unsigned getPrio() const;
	time_t getMTime() const;
	
	bool isFromDatabase() const;
	bool isStream() const;
	
	bool empty() const;
	
	std::string toString(const std::string &fmt, const std::string &tag_separator = ", ", const std::string &escape_chars = "") const;
	
	static std::string ShowTime(unsigned length);
	static bool isFormatOk(const std::string &type, const std::string &fmt);
	
	static const char FormatEscapeCharacter = 1;
	
	private:
		std::string ParseFormat(std::string::const_iterator &it, const std::string &tag_separator, const std::string &escape_chars) const;
		
		std::shared_ptr<struct SongImpl> pimpl;
};

}

#endif

