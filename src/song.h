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

#ifndef HAVE_SONG_H
#define HAVE_SONG_H

#include <cstdlib>
#include <string>
#include <sstream>
#include <stdexcept>

#include "misc.h"
#include "libmpdclient.h"

using std::string;

void DefineEmptyTags();

class Song
{
	public:
		Song() : itsHash(0), itsMinutesLength(0), itsSecondsLength(0), itsPosition(0), itsID(0), itsGetEmptyFields(0) { }
		Song(mpd_Song *);
		~Song() {};
		
		string GetFile() const;
		string GetShortFilename() const;
		string GetDirectory() const;
		string GetArtist() const;
		string GetTitle() const;
		string GetAlbum() const;
		string GetTrack() const;
		string GetYear() const;
		string GetGenre() const;
		//string GetName() const { return itsName; }
		string GetComposer() const;
		string GetPerformer() const;
		string GetDisc() const;
		string GetComment() const;
		string GetLength() const;
		long long GetHash() const { return itsHash; }
		int GetTotalLength() const { return itsSecondsLength < 0 ? 0 : itsMinutesLength*60+itsSecondsLength; }
		int GetMinutesLength() const { return itsMinutesLength; }
		int GetSecondsLength() const { return itsSecondsLength; }
		int GetPosition() const { return itsPosition; }
		int GetID() const { return itsID; }
		
		void SetFile(const string &str) { itsFile = str; }
		void SetShortFilename(const string &str) { itsShortName = str; }
		void SetArtist(const string &str) { itsArtist = str; }
		void SetTitle(const string &str) { itsTitle = str; }
		void SetAlbum(const string &str) { itsAlbum = str; }
		void SetTrack(const string &str) { itsTrack = str.empty() ? "" : IntoStr(StrToInt(str)); }
		void SetTrack(int track) { itsTrack = IntoStr(track); }
		void SetYear(const string &str) { itsYear = str.empty() ? "" : IntoStr(StrToInt(str)); }
		void SetYear(int year) { itsYear = IntoStr(year); }
		void SetGenre(const string &str) { itsGenre = str; }
		void SetComment(const string &str) { itsComment = str; }
		void SetPosition(int pos) { itsPosition = pos; }
		
		void GetEmptyFields(bool get) { itsGetEmptyFields = get; }
		void Clear();
		bool Empty() const;
		
		bool operator==(const Song &) const;
		bool operator!=(const Song &) const;
		bool operator<(const Song &rhs) const;
	private:
		string itsFile;
		string itsShortName;
		string itsDirectory;
		string itsArtist;
		string itsTitle;
		string itsAlbum;
		string itsTrack;
		//string itsName;
		string itsYear;
		string itsGenre;
		string itsComposer;
		string itsPerformer;
		string itsDisc;
		string itsComment;
		long long itsHash;
		int itsMinutesLength;
		int itsSecondsLength;
		int itsPosition;
		int itsID;
		bool itsGetEmptyFields;
};

#endif

