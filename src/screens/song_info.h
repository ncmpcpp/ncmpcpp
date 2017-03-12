/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_SONG_INFO_H
#define NCMPCPP_SONG_INFO_H

#include "interfaces.h"
#include "mutable_song.h"
#include "screens/screen.h"

struct SongInfo: Screen<NC::Scrollpad>, Tabbable
{
	struct Metadata
	{
		const char *Name;
		MPD::Song::GetFunction Get;
		MPD::MutableSong::SetFunction Set;
	};
	
	SongInfo();
	
	// Screen<NC::Scrollpad> implementation
	virtual void switchTo() override;
	virtual void resize() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::SongInfo; }
	
	virtual void update() override { }
	
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return true; }
	
	// private members
	static const Metadata Tags[];
	
private:
	void PrepareSong(const MPD::Song &s);
};

extern SongInfo *mySongInfo;

#endif // NCMPCPP_SONG_INFO_H

