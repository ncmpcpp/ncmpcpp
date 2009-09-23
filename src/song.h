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

#include <mpd/client.h>

namespace MPD
{
	class Song
	{
		public:
			
			typedef void (Song::*SetFunction)(const std::string &);
			typedef std::string (Song::*GetFunction)() const;
			
			Song() : itsSong(0), itsFile(0), itsSlash(std::string::npos), itsHash(0), copyPtr(0), isLocalised(0) { }
			Song(mpd_song *, bool = 0);
			Song(const Song &);
			~Song();
			
			std::string GetFile() const;
			std::string GetName() const;
			std::string GetDirectory() const;
			std::string GetArtist() const;
			std::string GetTitle() const;
			std::string GetAlbum() const;
			std::string GetTrack() const;
			std::string GetTrackNumber() const;
			std::string GetDate() const;
			std::string GetGenre() const;
			std::string GetComposer() const;
			std::string GetPerformer() const;
			std::string GetDisc() const;
			std::string GetComment() const;
			std::string GetLength() const;
			
			unsigned GetHash() const { return itsHash; }
			unsigned GetTotalLength() const { return mpd_song_get_duration(itsSong); }
			unsigned GetPosition() const { return mpd_song_get_pos(itsSong); }
			unsigned GetID() const { return mpd_song_get_id(itsSong); }
			
			void SetArtist(const std::string &);
			void SetTitle(const std::string &);
			void SetAlbum(const std::string &);
			void SetTrack(const std::string &);
			void SetTrack(unsigned);
			void SetDate(const std::string &);
			void SetDate(unsigned);
			void SetGenre(const std::string &);
			void SetComposer(const std::string &);
			void SetPerformer(const std::string &);
			void SetDisc(const std::string &);
			void SetComment(const std::string &);
			void SetPosition(unsigned);
			void SetLength(unsigned);
			
			void SetNewName(const std::string &name) { itsNewName = name == GetName() ? "" : name; }
			std::string GetNewName() const { return itsNewName; }
			
			std::string toString(const std::string &) const;
			
			void NullMe() { itsSong = 0; }
			void CopyPtr(bool copy) { copyPtr = copy; }
			
			void Localize();
			void Clear();
			bool Empty() const;
			bool isFromDB() const;
			bool isStream() const;
			bool Localized() const { return isLocalised; }
			
			Song &operator=(const Song &);
			
			static std::string ShowTime(int);
			static void ValidateFormat(const std::string &type, const std::string &format);
			
		private:
			void SetHashAndSlash();
			std::string ParseFormat(std::string::const_iterator &it) const;
			
			/// Used internally for handling filename, since we don't have
			/// write access to file string in mpd_song, manage our own if
			/// localization was done and there is localized filename that
			/// is different than the original one.
			///
			const char *MyFilename() const;
			
			/// internal mpd_song structure
			mpd_song *itsSong;
			
			/// localized version of filename
			const char *itsFile;
			
			std::string itsNewName;
			
			size_t itsSlash;
			unsigned itsHash;
			bool copyPtr;
			bool isLocalised;
	};
}

#endif

