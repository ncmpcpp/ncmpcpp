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

#ifndef _H_MEDIA_LIBRARY
#define _H_MEDIA_LIBRARY

#include "ncmpcpp.h"
#include "screen.h"

class MediaLibrary : public Screen<Window>
{
	struct SearchConstraints
	{
		SearchConstraints() { }
		SearchConstraints(const std::string &tag, const std::string &album, const std::string &date) : PrimaryTag(tag), Album(album), Date(date) { }
		SearchConstraints(const std::string &album, const std::string &date) : Album(album), Date(date) { }
		
		std::string PrimaryTag;
		std::string Album;
		std::string Date;
	};
	
	struct SearchConstraintsSorting
	{
		bool operator()(const SearchConstraints &a, const SearchConstraints &b) const;
	};
	
	public:
		virtual void SwitchTo();
		virtual void Resize();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void Refresh();
		virtual void Update();
		
		virtual void EnterPressed() { AddToPlaylist(1); }
		virtual void SpacePressed();
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		virtual MPD::Song *CurrentSong();
		virtual MPD::Song *GetSong(size_t pos) { return w == Songs ? &Songs->at(pos).value() : 0; }
		
		virtual bool allowsSelection() { return true; }
		virtual void ReverseSelection();
		virtual void GetSelectedSongs(MPD::SongList &);
		
		virtual void ApplyFilter(const std::string &);
		
		virtual List *GetList();
		
		virtual bool isMergable() { return true; }
		
		int Columns() { return hasTwoColumns ? 2 : 3; }
		bool isNextColumnAvailable();
		void NextColumn();
		bool isPrevColumnAvailable();
		void PrevColumn();
		
		void LocateSong(const MPD::Song &);
		
		Menu<std::string> *Artists;
		Menu<SearchConstraints> *Albums;
		Menu<MPD::Song> *Songs;
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		void AddToPlaylist(bool);
		
		static std::string SongToString(const MPD::Song &s);
		static std::string AlbumToString(const SearchConstraints &);
		static void DisplayAlbums(Menu<SearchConstraints> &menu, const SearchConstraints &sc);
		static void DisplayPrimaryTags(Menu<std::string> &menu, const std::string &tag);
		
		static bool SortSongsByTrack(const MPD::Song &, const MPD::Song &);
		static bool SortAllTracks(const MPD::Song &, const MPD::Song &);
		
		static bool hasTwoColumns;
		static size_t itsLeftColStartX;
		static size_t itsLeftColWidth;
		static size_t itsMiddleColWidth;
		static size_t itsMiddleColStartX;
		static size_t itsRightColWidth;
		static size_t itsRightColStartX;
		
		static const char AllTracksMarker[];
};

extern MediaLibrary *myLibrary;

#endif

