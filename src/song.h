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

class Song
{
	public:
		Song() : itsSlash(string::npos), itsHash(0), copyPtr(0), isStream(0), itsGetEmptyFields(0) { itsSong = mpd_newSong(); }
		Song(mpd_Song *, bool = 0);
		Song(const Song &);
		~Song();
		
		string GetFile() const;
		string GetName() const;
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
		int GetTotalLength() const { return itsSong->time < 0 ? 0 : itsSong->time; }
		int GetPosition() const { return itsSong->pos; }
		int GetID() const { return itsSong->id; }
		
		void SetFile(const string &);
		void SetArtist(const string &);
		void SetTitle(const string &);
		void SetAlbum(const string &);
		void SetTrack(const string &);
		void SetTrack(int);
		void SetYear(const string &);
		void SetYear(int);
		void SetGenre(const string &);
		void SetComposer(const string &);
		void SetPerformer(const string &);
		void SetDisc(const string &);
		void SetComment(const string &);
		void SetPosition(int);
		
		void SetNewName(const string &name) { itsNewName = name == GetName() ? "" : name; }
		string GetNewName() const { return itsNewName; }
		
		std::string toString(const std::string &) const;
		
		void NullMe() { itsSong = 0; }
		void CopyPtr(bool copy) { copyPtr = copy; }
		
		void GetEmptyFields(bool get) { itsGetEmptyFields = get; }
		void Clear();
		bool Empty() const;
		bool IsFromDB() const;
		bool IsStream() const { return isStream; }
		
		Song & operator=(const Song &);
		bool operator==(const Song &) const;
		bool operator!=(const Song &) const;
		bool operator<(const Song &rhs) const;
		
		static string ShowTime(int);
	private:
		mpd_Song *itsSong;
		string itsNewName;
		size_t itsSlash;
		long long itsHash;
		bool copyPtr;
		bool isStream;
		bool itsGetEmptyFields;
};

#endif

