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

#ifndef _BROWSER_H
#define _BROWSER_H

#include "ncmpcpp.h"
#include "screen.h"

class Browser : public Screen< Menu<MPD::Item> >
{
	public:
		Browser() : itsScrollBeginning(0), itsBrowsedDir("/") { }
		
		virtual void Init();
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::string Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed();
		
		virtual MPD::Song *CurrentSong();
		
		virtual bool allowsSelection() { return true; }
		virtual void ReverseSelection();
		virtual void GetSelectedSongs(MPD::SongList &);
		
		virtual void ApplyFilter(const std::string &s) { w->ApplyFilter(s, itsBrowsedDir == "/" ? 0 : 1); }
		
		virtual List *GetList() { return w; }
		
		const std::string &CurrentDir() { return itsBrowsedDir; }
		
		void LocateSong(const MPD::Song &);
		void GetDirectory(std::string, std::string = "/");
		void ChangeBrowseMode();
		void UpdateItemList();
	
	protected:
		static std::string ItemToString(const MPD::Item &, void *);
		
		size_t itsScrollBeginning;
		
		std::string itsBrowsedDir;
};

extern Browser *myBrowser;

#endif

