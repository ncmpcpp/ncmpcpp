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

#ifndef _BROWSER_H
#define _BROWSER_H

#include "interfaces.h"
#include "mpdpp.h"
#include "screen.h"

class Browser : public Screen< NC::Menu<MPD::Item> >, public Filterable, public HasSongs, public Searchable
{
	public:
		Browser() : itsBrowseLocally(0), itsScrollBeginning(0), itsBrowsedDir("/") { }
		
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void EnterPressed();
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
		virtual MPD::Song *getSong(size_t pos);
		virtual MPD::Song *currentSong();
		virtual std::shared_ptr<ProxySongList> getProxySongList();
		
		virtual bool allowsSelection();
		virtual void reverseSelection();
		virtual MPD::SongList getSelectedSongs();
		
		virtual bool isMergable() { return true; }
		
		const std::string &CurrentDir() { return itsBrowsedDir; }
		
		bool isLocal() { return itsBrowseLocally; }
		void LocateSong(const MPD::Song &);
		void GetDirectory(std::string, std::string = "/");
#		ifndef WIN32
		void GetLocalDirectory(MPD::ItemList &, const std::string & = "", bool = 0) const;
		void ClearDirectory(const std::string &) const;
		void ChangeBrowseMode();
		bool DeleteItem(const MPD::Item &);
#		endif // !WIN32
		void UpdateItemList();
		
		static bool isParentDirectory(const MPD::Item &item) {
			return item.type == MPD::itDirectory && item.name == "..";
		}
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		bool itsBrowseLocally;
		size_t itsScrollBeginning;
		std::string itsBrowsedDir;
};

extern Browser *myBrowser;

#endif

