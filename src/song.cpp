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
#include "settings.h"

extern ncmpcpp_config Config;

string EMPTY_TAG;
string UNKNOWN_ARTIST;
string UNKNOWN_TITLE;
string UNKNOWN_ALBUM;

void DefineEmptyTags()
{
	if (Config.empty_tags_color != clDefault)
	{
		const string et_col = IntoStr(Config.empty_tags_color);
		EMPTY_TAG = "[." + et_col + "]<empty>[/" + et_col + "]";
		UNKNOWN_ARTIST = "[." + et_col + "]<no artist>[/" + et_col + "]";
		UNKNOWN_TITLE = "[." + et_col + "]<no title>[/" + et_col + "]";
		UNKNOWN_ALBUM = "[." + et_col + "]<no album>[/" + et_col + "]";
	}
	else
	{
		EMPTY_TAG = "<empty>";
		UNKNOWN_ARTIST = "<no artist>";
		UNKNOWN_TITLE = "<no title>";
		UNKNOWN_ALBUM = "<no album";
	}
}

Song::Song(mpd_Song *s, bool copy_ptr) : itsSong(s),
					 itsHash(0),
					 copyPtr(copy_ptr),
					 isStream(0),
					 itsGetEmptyFields(0)
{
	string itsFile = itsSong->file ? itsSong->file : "";
	
	itsSlash = itsFile.find_last_of("/");
	
	if (itsFile.substr(0, 7) == "http://")
		isStream = 1;
	
	// generate pseudo-hash
	for (int i = 0; i < itsFile.length(); i++)
	{
		itsHash += itsFile[i];
		if (i%3)
			itsHash *= itsFile[i];
	}
}

Song::Song(const Song &s) : itsSong(0),
			    itsNewName(s.itsNewName),
			    itsSlash(s.itsSlash),
			    itsHash(s.itsHash),
			    copyPtr(s.copyPtr),
			    isStream(s.isStream),
			    itsGetEmptyFields(s.itsGetEmptyFields)
{
	itsSong = s.copyPtr ? s.itsSong : mpd_songDup(s.itsSong);
}

Song::~Song()
{
	if (itsSong)
		mpd_freeSong(itsSong);
}

string Song::GetLength() const
{
	if (itsSong->time <= 0)
		return "-:--";
	return ShowTime(itsSong->time);
}

void Song::Clear()
{
	if (itsSong)
		mpd_freeSong(itsSong);
	itsSong = mpd_newSong();
	itsNewName.clear();
	itsSlash = 0;
	itsHash = 0;
	copyPtr = 0;
	itsGetEmptyFields = 0;
}

bool Song::Empty() const
{
	return !itsSong || (!itsSong->file && !itsSong->title && !itsSong->artist && !itsSong->album && !itsSong->date && !itsSong->track && !itsSong->genre && !itsSong->composer && !itsSong->performer && !itsSong->disc && !itsSong->comment);
}

bool Song::IsFromDB() const
{
	const string &dir = GetDirectory();
	return dir[0] != '/' || dir == "/";
}

string Song::GetFile() const
{
	return !itsSong->file ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsSong->file;
}

string Song::GetName() const
{
	return !itsSong->file ? (itsGetEmptyFields ? "" : EMPTY_TAG) : (itsSlash != string::npos && !isStream ? string(itsSong->file).substr(itsSlash+1) : itsSong->file);
}

string Song::GetDirectory() const
{
	return !itsSong->file || isStream ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsSlash != string::npos ? string(itsSong->file).substr(0, itsSlash) : "/";
}

string Song::GetArtist() const
{
	return !itsSong->artist ? (itsGetEmptyFields ? "" : UNKNOWN_ARTIST) : itsSong->artist;
}

string Song::GetTitle() const
{
	return !itsSong->title ? (itsGetEmptyFields ? "" : UNKNOWN_TITLE) : itsSong->title;
}

string Song::GetAlbum() const
{
	return !itsSong->album ? (itsGetEmptyFields ? "" : UNKNOWN_ALBUM) : itsSong->album;
}

string Song::GetTrack() const
{
	return !itsSong->track ? (itsGetEmptyFields ? "" : EMPTY_TAG) : (StrToInt(itsSong->track) < 10 && itsSong->track[0] != '0' ? "0"+string(itsSong->track) : itsSong->track);
}

string Song::GetYear() const
{
	return !itsSong->date ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsSong->date;
}

string Song::GetGenre() const
{
	return !itsSong->genre ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsSong->genre;
}

string Song::GetComposer() const
{
	return !itsSong->composer ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsSong->composer;
}

string Song::GetPerformer() const
{
	return !itsSong->performer ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsSong->performer;
}

string Song::GetDisc() const
{
	return !itsSong->disc ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsSong->disc;
}

string Song::GetComment() const
{
	return !itsSong->comment ? (itsGetEmptyFields ? "" : EMPTY_TAG) : itsSong->comment;
}

void Song::SetFile(const string &str)
{
	if (itsSong->file)
		str_pool_put(itsSong->file);
	itsSong->file = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetArtist(const string &str)
{
	if (itsSong->artist)
		str_pool_put(itsSong->artist);
	itsSong->artist = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetTitle(const string &str)
{
	if (itsSong->title)
		str_pool_put(itsSong->title);
	itsSong->title = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetAlbum(const string &str)
{
	if (itsSong->album)
		str_pool_put(itsSong->album);
	itsSong->album = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetTrack(const string &str)
{
	if (itsSong->track)
		str_pool_put(itsSong->track);
	itsSong->track = str.empty() ? 0 : str_pool_get(IntoStr(StrToInt(str)).c_str());
}

void Song::SetTrack(int track)
{
	if (itsSong->track)
		str_pool_put(itsSong->track);
	itsSong->track = str_pool_get(IntoStr(track).c_str());
}

void Song::SetYear(const string &str)
{
	if (itsSong->date)
		str_pool_put(itsSong->date);
	itsSong->date = str.empty() ? 0 : str_pool_get(IntoStr(StrToInt(str)).c_str());
}

void Song::SetYear(int year)
{
	if (itsSong->date)
		str_pool_put(itsSong->date);
	itsSong->date = str_pool_get(IntoStr(year).c_str());
}

void Song::SetGenre(const string &str)
{
	if (itsSong->genre)
		str_pool_put(itsSong->genre);
	itsSong->genre = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetComposer(const string &str)
{
	if (itsSong->composer)
		str_pool_put(itsSong->composer);
	itsSong->composer = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetPerformer(const string &str)
{
	if (itsSong->performer)
		str_pool_put(itsSong->performer);
	itsSong->performer = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetDisc(const string &str)
{
	if (itsSong->disc)
		str_pool_put(itsSong->disc);
	itsSong->disc = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetComment(const string &str)
{
	if (itsSong->comment)
		str_pool_put(itsSong->comment);
	itsSong->comment = str.empty() ? 0 : str_pool_get(str.c_str());
}

void Song::SetPosition(int pos)
{
	itsSong->pos = pos;
}

Song & Song::operator=(const Song &s)
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
	isStream = s.isStream;
	itsGetEmptyFields = s.itsGetEmptyFields;
	return *this;
}

bool Song::operator==(const Song &s) const
{
	return  (itsSong->file && s.itsSong->file
			? strcmp(itsSong->file, s.itsSong->file) == 0
			: !(itsSong->file || s.itsSong->file))
	&&	(itsSong->title && s.itsSong->title
			? strcmp(itsSong->title, s.itsSong->title) == 0
			: !(itsSong->title || s.itsSong->title))
	&&	(itsSong->artist && s.itsSong->artist
			? strcmp(itsSong->artist, s.itsSong->artist) == 0
			: !(itsSong->artist || s.itsSong->artist))
	&&	(itsSong->album && s.itsSong->album
			? strcmp(itsSong->album, s.itsSong->album) == 0
			: !(itsSong->album || s.itsSong->album))
	&&	(itsSong->track && s.itsSong->track
			? strcmp(itsSong->track, s.itsSong->track) == 0
			: !(itsSong->track || s.itsSong->track))
	&&	(itsSong->date && s.itsSong->date
			? strcmp(itsSong->date, s.itsSong->date) == 0
			: !(itsSong->date || s.itsSong->date))
	&&	(itsSong->genre && s.itsSong->genre
			? strcmp(itsSong->genre, s.itsSong->genre) == 0
			: !(itsSong->genre || s.itsSong->genre))
	&&	(itsSong->composer && s.itsSong->composer
			? strcmp(itsSong->composer, s.itsSong->composer) == 0
			: !(itsSong->composer || s.itsSong->composer))
	&&	(itsSong->performer && s.itsSong->performer
			? strcmp(itsSong->performer, s.itsSong->performer) == 0
			: !(itsSong->performer || s.itsSong->performer))
	&&	(itsSong->disc && s.itsSong->disc
			? strcmp(itsSong->disc, s.itsSong->disc) == 0
			: !(itsSong->disc || s.itsSong->disc))
	&&	(itsSong->comment && s.itsSong->comment
			? strcmp(itsSong->comment, s.itsSong->comment) == 0
			: !(itsSong->comment || s.itsSong->comment))
	&&	itsSong->time == s.itsSong->time
	&&	itsSong->pos == s.itsSong->pos
	&&	itsSong->id == s.itsSong->id
	&&	itsHash == itsHash;
}

bool Song::operator!=(const Song &s) const
{
	return !operator==(s);
}

bool Song::operator<(const Song &s) const
{
	return itsSong->pos < s.itsSong->pos;
}

string Song::ShowTime(int length)
{
	std::stringstream ss;
	
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

