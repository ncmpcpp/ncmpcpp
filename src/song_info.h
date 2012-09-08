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

#ifndef _SONG_INFO_H
#define _SONG_INFO_H

#include "screen.h"
#include "mutable_song.h"

class SongInfo : public Screen<NC::Scrollpad>
{
	public:
		struct Metadata
		{
			const char *Name;
			MPD::Song::GetFunction Get;
			MPD::MutableSong::SetFunction Set;
		};
		
		// Screen<NC::Scrollpad> implementation
		virtual void SwitchTo() OVERRIDE;
		virtual void Resize() OVERRIDE;
		
		virtual std::wstring Title() OVERRIDE;
		
		virtual void Update() OVERRIDE { }
		
		virtual void EnterPressed() OVERRIDE { }
		virtual void SpacePressed() OVERRIDE { }
		
		virtual bool isMergable() OVERRIDE { return true; }
		virtual bool isTabbable() OVERRIDE { return false; }
		
		// private members
		
		static const Metadata Tags[];
		
	protected:
		virtual void Init() OVERRIDE;
		virtual bool isLockable() OVERRIDE { return false; }
		
	private:
		void PrepareSong(MPD::Song &);
};

extern SongInfo *mySongInfo;

#endif

