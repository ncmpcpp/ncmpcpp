/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include <string>

#include "misc.h"
#include "libmpdclient.h"

namespace MPD
{
	class Song
	{
		public:
			Song() : itsSlash(std::string::npos), itsHash(0), copyPtr(0), isStream(0), isLocalised(0) { itsSong = mpd_newSong(); }
			Song(mpd_Song *, bool = 0);
			Song(const Song &);
			~Song();
			
			std::string GetFile() const;
			std::string GetName() const;
			std::string GetDirectory() const;
			std::string GetArtist() const;
			std::string GetTitle() const;
			std::string GetAlbum() const;
			std::string GetTrack() const;
			std::string GetYear() const;
			std::string GetGenre() const;
			std::string GetComposer() const;
			std::string GetPerformer() const;
			std::string GetDisc() const;
			std::string GetComment() const;
			std::string GetLength() const;
			const long long &GetHash() const { return itsHash; }
			int GetTotalLength() const { return itsSong->time < 0 ? 0 : itsSong->time; }
			int GetPosition() const { return itsSong->pos; }
			int GetID() const { return itsSong->id; }
			
			void SetFile(const std::string &);
			void SetArtist(const std::string &);
			void SetTitle(const std::string &);
			void SetAlbum(const std::string &);
			void SetTrack(const std::string &);
			void SetTrack(int);
			void SetYear(const std::string &);
			void SetYear(int);
			void SetGenre(const std::string &);
			void SetComposer(const std::string &);
			void SetPerformer(const std::string &);
			void SetDisc(const std::string &);
			void SetComment(const std::string &);
			void SetPosition(int);
			
			void SetNewName(const std::string &name) { itsNewName = name == GetName() ? "" : name; }
			std::string GetNewName() const { return itsNewName; }
			
			std::string toString(const std::string &) const;
			
			void NullMe() { itsSong = 0; }
			void CopyPtr(bool copy) { copyPtr = copy; }
			
			//void GetEmptyFields(bool get) { itsGetEmptyFields = get; }
			void Localize();
			//void Delocalize();
			void Clear();
			bool Empty() const;
			bool IsFromDB() const;
			bool IsStream() const { return isStream; }
			bool Localized() const { return isLocalised; }
			
			Song & operator=(const Song &);
			bool operator==(const Song &) const;
			bool operator!=(const Song &) const;
			bool operator<(const Song &rhs) const;
			
			static std::string ShowTime(int);
		private:
			void __Count_Last_Slash_Position();
			
			mpd_Song *itsSong;
			std::string itsNewName;
			size_t itsSlash;
			long long itsHash;
			bool copyPtr;
			bool isStream;
			bool isLocalised;
		//bool itsGetEmptyFields;
	};
}

#endif

