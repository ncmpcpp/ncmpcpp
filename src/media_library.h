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

#include "interfaces.h"
#include "screen.h"

class MediaLibrary : public Screen<NC::Window>, public Filterable, public HasSongs, public Searchable
{
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
		
		/// Filterable implementation
		virtual std::string currentFilter();
		virtual void applyFilter(const std::string &filter);
		
		/// Searchable implementation
		virtual bool search(const std::string &constraint);
		virtual void nextFound(bool wrap);
		virtual void prevFound(bool wrap);
		
		/// HasSongs implementation
		virtual std::shared_ptr<ProxySongList> getProxySongList();
		
		virtual bool allowsSelection();
		virtual void reverseSelection();
		virtual MPD::SongList getSelectedSongs();
		
		virtual NC::List *GetList();
		
		virtual bool isMergable() { return true; }
		
		int Columns();
		bool isNextColumnAvailable();
		void NextColumn();
		bool isPrevColumnAvailable();
		void PrevColumn();
		
		void LocateSong(const MPD::Song &);
		
		struct SearchConstraints
		{
			SearchConstraints() { }
			SearchConstraints(const std::string &tag, const std::string &album, const std::string &date)
			: PrimaryTag(tag), Album(album), Date(date) { }
			SearchConstraints(const std::string &album, const std::string &date)
			: Album(album), Date(date) { }
			
			std::string PrimaryTag;
			std::string Album;
			std::string Date;
		};
		
		NC::Menu<std::string> *Tags;
		NC::Menu<SearchConstraints> *Albums;
		NC::Menu<MPD::Song> *Songs;
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		void AddToPlaylist(bool);
};

extern MediaLibrary *myLibrary;

#endif

