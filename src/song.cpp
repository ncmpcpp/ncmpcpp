/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "song.h"

extern ncmpcpp_config Config;

string EMPTY_TAG;
string UNKNOWN_ARTIST;
string UNKNOWN_TITLE;
string UNKNOWN_ALBUM;

void DefineEmptyTags()
{
	const string et_col = IntoStr(Config.empty_tags_color);
	EMPTY_TAG = "[." + et_col + "]<empty>[/" + et_col + "]";
	UNKNOWN_ARTIST = "[." + et_col + "]<no artist>[/" + et_col + "]";
	UNKNOWN_TITLE = "[." + et_col + "]<no title>[/" + et_col + "]";
	UNKNOWN_ALBUM = "[." + et_col + "]<no album>[/" + et_col + "]";
}

Song::Song(mpd_Song *s) : itsHash(0),
			  itsMinutesLength(s->time/60),
			  itsSecondsLength((s->time-itsMinutesLength*60)),
			  itsPosition(s->pos),
			  itsID(s->id),
			  itsGetEmptyFields(0)
{
	s->file ? itsFile = s->file : itsFile = "";
	s->artist ? itsArtist = s->artist : itsArtist = "";
	s->title ? itsTitle = s->title : itsTitle = "";
	s->album ? itsAlbum = s->album : itsAlbum = "";	
	s->track ? itsTrack = IntoStr(atoi(s->track)) : itsTrack = "";
	//s->name ? itsName = s->name : itsName = "";
	s->date ? itsYear = s->date : itsYear = "";
	s->genre ? itsGenre = s->genre : itsGenre = "";
	s->composer ? itsComposer = s->composer : itsComposer = "";
	s->performer ? itsPerformer = s->performer : itsPerformer = "";
	s->disc ? itsDisc = s->disc : itsDisc = "";
	s->comment ? itsComment = s->comment : itsComment = "";
	
	int i = itsFile.find_last_of("/");
	
	if (itsFile.substr(0, 7) == "http://")
	{
		itsShortName = itsFile;
	}
	else if (i != string::npos)
	{
		itsDirectory = itsFile.substr(0, i);
		itsShortName = itsFile.substr(i+1);
	}
	else
	{
		itsDirectory = "/";
		itsShortName = itsFile;
	}
	// generate pseudo-hash
	i = 0;
	for (string::const_iterator it = itsFile.begin(); it != itsFile.end(); it++, i++)
	{
		itsHash += *it;
		if (i%2)
			itsHash *= *it;
	}
}

string Song::GetLength() const
{
	std::stringstream ss;
	
	if (!GetTotalLength())
		return "-:--";
	
	ss << itsMinutesLength << ":";
	if (!itsSecondsLength)
	{
		ss << "00";
		return ss.str();
	}
	if (itsSecondsLength < 10)
		ss << "0" << itsSecondsLength;
	else
		ss << itsSecondsLength;
	return ss.str();
}

void Song::Clear()
{
	itsFile.clear();
	itsShortName.clear();
	itsDirectory.clear();
	itsArtist.clear();
	itsTitle.clear();
	itsAlbum.clear();
	itsTrack.clear();
	//itsName.clear();
	itsYear.clear();
	itsGenre.clear();
	itsComposer.clear();
	itsPerformer.clear();
	itsDisc.clear();
	itsComment.clear();
	itsMinutesLength = 0;
	itsSecondsLength = 0;
	itsPosition = 0;
	itsID = 0;
}

bool Song::Empty() const
{
	return itsFile.empty() && itsShortName.empty() && itsArtist.empty() && itsTitle.empty() && itsAlbum.empty() && itsTrack.empty() && itsYear.empty() && itsGenre.empty();
}

string Song::GetFile() const
{
	return itsFile.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsFile;
}

string Song::GetShortFilename() const
{
	return itsShortName.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsShortName;
}

string Song::GetDirectory() const
{
	return itsDirectory.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsDirectory;
}

string Song::GetArtist() const
{
	return itsArtist.empty() ? (itsGetEmptyFields ? "" : UNKNOWN_ARTIST) : itsArtist;
}

string Song::GetTitle() const
{
	return itsTitle.empty() ? (itsGetEmptyFields ? "" : UNKNOWN_TITLE) : itsTitle;
}

string Song::GetAlbum() const
{
	return itsAlbum.empty() ? (itsGetEmptyFields ? "" : UNKNOWN_ALBUM) : itsAlbum;
}

string Song::GetTrack() const
{
	return itsTrack.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : (StrToInt(itsTrack) < 10 && itsTrack[0] != '0' ? "0"+itsTrack : itsTrack);
}

string Song::GetYear() const
{
	return itsYear.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsYear;
}

string Song::GetGenre() const
{
	return itsGenre.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsGenre;
}

string Song::GetComposer() const
{
	return itsComposer.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsComposer;
}

string Song::GetPerformer() const
{
	return itsPerformer.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsPerformer;
}

string Song::GetDisc() const
{
	return itsDisc.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsDisc;
}

string Song::GetComment() const
{
	return itsComment.empty() ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsComment;
}

Song & Song::operator=(const Song &s)
{
	if (this == &s)
		return *this;
	itsFile = s.itsFile;
	itsShortName = s.itsShortName;
	itsDirectory = s.itsDirectory;
	itsArtist = s.itsArtist;
	itsTitle = s.itsTitle;
	itsAlbum = s.itsAlbum;
	itsTrack = s.itsTrack;
	itsYear = s.itsYear;
	itsGenre = s.itsGenre;
	itsComposer = s.itsComposer;
	itsPerformer = s.itsPerformer;
	itsDisc = s.itsDisc;
	itsComment = s.itsComment;
	itsHash = s.itsHash;
	itsMinutesLength = s.itsMinutesLength;
	itsSecondsLength = s.itsSecondsLength;
	itsPosition = s.itsPosition;
	itsID = s.itsID;
	itsGetEmptyFields = s.itsGetEmptyFields;
	return *this;
}

bool Song::operator==(const Song &s) const
{
	return itsFile == s.itsFile && itsArtist == s.itsArtist && itsTitle == s.itsTitle && itsAlbum == s.itsAlbum && itsTrack == s.itsTrack && itsYear == s.itsYear && itsGenre == s.itsGenre && itsComposer == s.itsComposer && itsPerformer == s.itsPerformer && itsDisc == s.itsDisc && itsComment == s.itsComment && itsHash == s.itsHash && itsMinutesLength && s.itsMinutesLength && itsSecondsLength == s.itsSecondsLength && itsPosition == s.itsPosition && itsID == s.itsID;
}

bool Song::operator!=(const Song &s) const
{
	return itsFile != s.itsFile || itsArtist != s.itsArtist || itsTitle != s.itsTitle || itsAlbum != s.itsAlbum || itsTrack != s.itsTrack || itsYear != s.itsYear || itsGenre != s.itsGenre || itsComposer != s.itsComposer || itsPerformer != s.itsPerformer || itsDisc != s.itsDisc || itsComment != s.itsComment || itsHash != s.itsHash || itsMinutesLength != s.itsMinutesLength || itsSecondsLength != s.itsSecondsLength || itsPosition != s.itsPosition || itsID != s.itsID;
}

bool Song::operator<(const Song &s) const
{
	return itsPosition < s.itsPosition;
}

