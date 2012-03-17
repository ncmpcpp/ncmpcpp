/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
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

#include <map>
#include <string>

#include <mpd/client.h>

namespace MPD
{
	class Song
	{
		typedef std::map<std::pair<mpd_tag_type, unsigned>, std::string> TagMap;
		
		public:
			
			typedef void (Song::*SetFunction)(const std::string &, unsigned);
			typedef std::string (Song::*GetFunction)(unsigned) const;
			
			Song(mpd_song * = 0, bool = 0);
			Song(const Song &);
			~Song();
			
			std::string GetFile(unsigned = 0) const;
			std::string GetName(unsigned = 0) const;
			std::string GetDirectory(unsigned = 0) const;
			std::string GetArtist(unsigned = 0) const;
			std::string GetTitle(unsigned = 0) const;
			std::string GetAlbum(unsigned = 0) const;
			std::string GetAlbumArtist(unsigned = 0) const;
			std::string GetTrack(unsigned = 0) const;
			std::string GetTrackNumber(unsigned = 0) const;
			std::string GetDate(unsigned = 0) const;
			std::string GetGenre(unsigned = 0) const;
			std::string GetComposer(unsigned = 0) const;
			std::string GetPerformer(unsigned = 0) const;
			std::string GetDisc(unsigned = 0) const;
			std::string GetComment(unsigned = 0) const;
			std::string GetLength(unsigned = 0) const;
			
			std::string GetTags(GetFunction) const;
			
			unsigned GetHash() const { return itsHash; }
			unsigned GetTotalLength() const { return mpd_song_get_duration(itsSong); }
			unsigned GetPosition() const { return mpd_song_get_pos(itsSong); }
			unsigned GetID() const { return mpd_song_get_id(itsSong); }
			
			time_t GetMTime() const { return mpd_song_get_last_modified(itsSong); }
			
			void SetArtist(const std::string &, unsigned = 0);
			void SetTitle(const std::string &, unsigned = 0);
			void SetAlbum(const std::string &, unsigned = 0);
			void SetAlbumArtist(const std::string &, unsigned = 0);
			void SetTrack(const std::string &, unsigned = 0);
			void SetTrack(unsigned, unsigned = 0);
			void SetDate(const std::string &, unsigned = 0);
			void SetDate(unsigned, unsigned = 0);
			void SetGenre(const std::string &, unsigned = 0);
			void SetComposer(const std::string &, unsigned = 0);
			void SetPerformer(const std::string &, unsigned = 0);
			void SetDisc(const std::string &, unsigned = 0);
			void SetComment(const std::string &, unsigned = 0);
			void SetPosition(unsigned);
			
			void SetTags(SetFunction, const std::string &);
			
			void SetNewName(const std::string &name) { itsNewName = name == GetName() ? "" : name; }
			std::string GetNewName() const { return itsNewName; }
			
			std::string toString(const std::string &, const char *escape_chars = 0) const;
			static const char FormatEscapeCharacter = 1;
			
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
			static bool isFormatOk(const std::string &type, const std::string &format);
			
		private:
			void SetHashAndSlash();
			std::string ParseFormat(std::string::const_iterator &it, const char *escape_chars) const;
			
			void SetTag(mpd_tag_type, unsigned, const std::string &);
			std::string GetTag(mpd_tag_type, unsigned) const;
			
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
			
			/// map that contains localized tags or these set by tag editor
			TagMap *itsTags;
			
			std::string itsNewName;
			
			size_t itsSlash;
			unsigned itsHash;
			bool copyPtr;
			bool isLocalised;
	};
}

#endif

