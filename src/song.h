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
#include "settings.h"

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
		//string GetComposer() const { return itsComposer; }
		//string GetPerformer() const { return itsPerformer; }
		//string GetDisc() const { return itsDisc; }
		string GetComment() const;
		string GetLength() const;
		long long GetHash() const { return itsHash; }
		int GetTotalLength() const { return itsMinutesLength*60+itsSecondsLength; }
		int GetMinutesLength() const { return itsMinutesLength; }
		int GetSecondsLength() const { return itsSecondsLength; }
		int GetPosition() const { return itsPosition; }
		int GetID() const { return itsID; }
		
		void SetFile(string str) { itsFile = str; }
		void SetShortFilename(string str) { itsShortName = str; }
		void SetArtist(string str) { itsArtist = str; }
		void SetTitle(string str) { itsTitle = str; }
		void SetAlbum(string str) { itsAlbum = str; }
		void SetTrack(string str) { itsTrack = str.empty() ? "" : IntoStr(StrToInt(str)); }
		void SetYear(string str) { itsYear = str.empty() ? "" : IntoStr(StrToInt(str)); }
		void SetGenre(string str) { itsGenre = str; }
		void SetComment(string str) { itsComment = str; }
		void SetPosition(int pos) { itsPosition = pos; }
		
		void GetEmptyFields(bool get) { itsGetEmptyFields = get; }
		void Clear();
		bool Empty() const;
		
		Song & operator=(const Song &);
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
		//string itsComposer;
		//string itsPerformer;
		//string itsDisc;
		string itsComment;
		long long itsHash;
		int itsMinutesLength;
		int itsSecondsLength;
		int itsPosition;
		int itsID;
		bool itsGetEmptyFields;
};

//void GetCurrentPlaylist(vector<Song> &);

#endif

