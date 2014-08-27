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

#include "statusbar.h"
#include <boost/algorithm/string/split.hpp>
#include "mutable_song.h"

namespace MPD {//

std::string MutableSong::getArtist(unsigned idx) const
{
	return getTag(MPD_TAG_ARTIST, [this, idx](){ return Song::getArtist(idx); }, idx);
}

std::string MutableSong::getTitle(unsigned idx) const
{
	return getTag(MPD_TAG_TITLE, [this, idx](){ return Song::getTitle(idx); }, idx);
}

std::string MutableSong::getAlbum(unsigned idx) const
{
	return getTag(MPD_TAG_ALBUM, [this, idx](){ return Song::getAlbum(idx); }, idx);
}

std::string MutableSong::getAlbumArtist(unsigned idx) const
{
	return getTag(MPD_TAG_ALBUM_ARTIST, [this, idx](){ return Song::getAlbumArtist(idx); }, idx);
}

std::string MutableSong::getTrack(unsigned idx) const
{
	std::string track = getTag(MPD_TAG_TRACK, [this, idx](){ return Song::getTrack(idx); }, idx);
	if ((track.length() == 1 && track[0] != '0')
	||  (track.length() > 3  && track[1] == '/'))
		return "0"+track;
	else
		return track;
}

std::string MutableSong::getDate(unsigned idx) const
{
	return getTag(MPD_TAG_DATE, [this, idx](){ return Song::getDate(idx); }, idx);
}

std::string MutableSong::getGenre(unsigned idx) const
{
	return getTag(MPD_TAG_GENRE, [this, idx](){ return Song::getGenre(idx); }, idx);
}

std::string MutableSong::getComposer(unsigned idx) const
{
	return getTag(MPD_TAG_COMPOSER, [this, idx](){ return Song::getComposer(idx); }, idx);
}

std::string MutableSong::getPerformer(unsigned idx) const
{
	return getTag(MPD_TAG_PERFORMER, [this, idx](){ return Song::getPerformer(idx); }, idx);
}

std::string MutableSong::getDisc(unsigned idx) const
{
	return getTag(MPD_TAG_DISC, [this, idx](){ return Song::getDisc(idx); }, idx);
}

std::string MutableSong::getComment(unsigned idx) const
{
	return getTag(MPD_TAG_COMMENT, [this, idx](){ return Song::getComment(idx); }, idx);
}

void MutableSong::setArtist(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_ARTIST, Song::getArtist(idx), value, idx);
}

void MutableSong::setTitle(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_TITLE, Song::getTitle(idx), value, idx);
}

void MutableSong::setAlbum(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_ALBUM, Song::getAlbum(idx), value, idx);
}

void MutableSong::setAlbumArtist(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_ALBUM_ARTIST, Song::getAlbumArtist(idx), value, idx);
}

void MutableSong::setTrack(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_TRACK, Song::getTrack(idx), value, idx);
}

void MutableSong::setDate(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_DATE, Song::getDate(idx), value, idx);
}

void MutableSong::setGenre(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_GENRE, Song::getGenre(idx), value, idx);
}

void MutableSong::setComposer(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_COMPOSER, Song::getComposer(idx), value, idx);
}

void MutableSong::setPerformer(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_PERFORMER, Song::getPerformer(idx), value, idx);
}

void MutableSong::setDisc(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_DISC, Song::getDisc(idx), value, idx);
}

void MutableSong::setComment(const std::string &value, unsigned idx)
{
	replaceTag(MPD_TAG_COMMENT, Song::getComment(idx), value, idx);
}

const std::string &MutableSong::getNewName() const
{
	return m_name;
}

void MutableSong::setNewName(const std::string &value)
{
	if (getName() == value)
		m_name.clear();
	else
		m_name = value;
}

unsigned MutableSong::getDuration() const
{
	if (m_duration > 0)
		return m_duration;
	else
		return Song::getDuration();
}

time_t MutableSong::getMTime() const
{
	if (m_mtime > 0)
		return m_mtime;
	else
		return Song::getMTime();
}

void MutableSong::setDuration(unsigned int duration)
{
	m_duration = duration;
}

void MutableSong::setMTime(time_t mtime)
{
	m_mtime = mtime;
}

void MutableSong::setTags(SetFunction set, const std::string &value, const std::string &delimiter)
{
	std::vector<std::string> tags;
	boost::iter_split(tags, value, boost::first_finder(delimiter));
	size_t i = 0;
	for (; i < tags.size(); ++i)
		(this->*set)(tags[i], i);
	// set next tag to be empty, so tags with bigger indexes won't be read
	(this->*set)("", i);
}

bool MutableSong::isModified() const
{
	return !m_name.empty() || !m_tags.empty();
}

void MutableSong::clearModifications()
{
	m_name.clear();
	m_tags.clear();
}

void MutableSong::replaceTag(mpd_tag_type tag_type, std::string orig_value, const std::string &value, unsigned idx)
{
	Tag tag(tag_type, idx);
	if (value == orig_value)
	{
		auto it = m_tags.find(tag);
		if (it != m_tags.end())
			m_tags.erase(it);
	}
	else
		m_tags[tag] = value;
}

}
