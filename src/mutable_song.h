/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_EDITABLE_SONG_H
#define NCMPCPP_EDITABLE_SONG_H

#include <map>
#include "config.h"
#include "song.h"

namespace MPD {//

struct MutableSong : public Song
{
	typedef void (MutableSong::*SetFunction)(const std::string &, unsigned);
	
	MutableSong() : m_mtime(0), m_duration(0) { }
	MutableSong(Song s) : Song(s), m_mtime(0), m_duration(0) { }
	
	virtual std::string getArtist(unsigned idx = 0) const OVERRIDE;
	virtual std::string getTitle(unsigned idx = 0) const OVERRIDE;
	virtual std::string getAlbum(unsigned idx = 0) const OVERRIDE;
	virtual std::string getAlbumArtist(unsigned idx = 0) const OVERRIDE;
	virtual std::string getTrack(unsigned idx = 0) const OVERRIDE;
	virtual std::string getDate(unsigned idx = 0) const OVERRIDE;
	virtual std::string getGenre(unsigned idx = 0) const OVERRIDE;
	virtual std::string getComposer(unsigned idx = 0) const OVERRIDE;
	virtual std::string getPerformer(unsigned idx = 0) const OVERRIDE;
	virtual std::string getDisc(unsigned idx = 0) const OVERRIDE;
	virtual std::string getComment(unsigned idx = 0) const OVERRIDE;
	
	void setArtist(const std::string &value, unsigned idx = 0);
	void setTitle(const std::string &value, unsigned idx = 0);
	void setAlbum(const std::string &value, unsigned idx = 0);
	void setAlbumArtist(const std::string &value, unsigned idx = 0);
	void setTrack(const std::string &value, unsigned idx = 0);
	void setDate(const std::string &value, unsigned idx = 0);
	void setGenre(const std::string &value, unsigned idx = 0);
	void setComposer(const std::string &value, unsigned idx = 0);
	void setPerformer(const std::string &value, unsigned idx = 0);
	void setDisc(const std::string &value, unsigned idx = 0);
	void setComment(const std::string &value, unsigned idx = 0);
	
	const std::string &getNewName() const;
	void setNewName(const std::string &value);
	
	virtual unsigned getDuration() const OVERRIDE;
	virtual time_t getMTime() const OVERRIDE;
	void setDuration(unsigned duration);
	void setMTime(time_t mtime);
	
	void setTags(SetFunction set, const std::string &value, const std::string &delimiter);
	
	bool isModified() const;
	void clearModifications();
	
private:
	struct Tag
	{
		Tag(mpd_tag_type type_, unsigned idx_) : m_type(type_), m_idx(idx_) { }
		
		mpd_tag_type type() const { return m_type; }
		unsigned idx() const { return m_idx; }
		
		bool operator<(const Tag &t) const
		{
			if (m_type != t.m_type)
				return m_type < t.m_type;
			return m_idx < t.m_idx;
		}
		
	private:
		mpd_tag_type m_type;
		unsigned m_idx;
	};
	
	void replaceTag(mpd_tag_type tag_type, std::string orig_value,
	                const std::string &value, unsigned idx);
	
	template <typename F>
	std::string getTag(mpd_tag_type tag_type, F orig_value, unsigned idx) const {
		auto it = m_tags.find(Tag(tag_type, idx));
		std::string result;
		if (it == m_tags.end())
			result = orig_value();
		else
			result = it->second;
		return result;
	}
	
	std::string m_name;
	time_t m_mtime;
	unsigned m_duration;
	std::map<Tag, std::string> m_tags;
};

}

#endif // NCMPCPP_EDITABLE_SONG_H
