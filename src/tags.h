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

#ifndef NCMPCPP_TAGS_H
#define NCMPCPP_TAGS_H

#include "config.h"

#ifdef HAVE_TAGLIB_H

#include <tfile.h>
#include "mutable_song.h"

namespace Tags {//

struct ReplayGainInfo
{
	ReplayGainInfo() { }
	ReplayGainInfo(std::string reference_loudness, std::string track_gain,
				   std::string track_peak, std::string album_gain,
				   std::string album_peak)
	: m_reference_loudness(reference_loudness), m_track_gain(track_gain)
	, m_track_peak(track_peak), m_album_gain(album_gain), m_album_peak(album_peak) { }
	
	bool empty() const
	{
		return m_reference_loudness.empty()
		    && m_track_gain.empty()
		    && m_track_peak.empty()
		    && m_album_gain.empty()
		    && m_album_peak.empty();
	}
	
	const std::string &referenceLoudness() const { return m_reference_loudness; }
	const std::string &trackGain() const { return m_track_gain; }
	const std::string &trackPeak() const { return m_track_peak; }
	const std::string &albumGain() const { return m_album_gain; }
	const std::string &albumPeak() const { return m_album_peak; }
	
private:
	std::string m_reference_loudness;
	std::string m_track_gain;
	std::string m_track_peak;
	std::string m_album_gain;
	std::string m_album_peak;
};

ReplayGainInfo readReplayGain(TagLib::File *f);

bool extendedSetSupported(const TagLib::File *f);

void read(MPD::MutableSong &);
bool write(MPD::MutableSong &);

}

#endif // HAVE_TAGLIB_H

#endif // NCMPCPP_TAGS_H
