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

#ifndef _H_MEDIA_LIBRARY
#define _H_MEDIA_LIBRARY

#include "ncmpcpp.h"
#include "screen.h"

class MediaLibrary : public Screen<Window>
{
	public:
		virtual void Init();
		virtual void SwitchTo();
		virtual void Resize();
		
		virtual std::string Title();
		
		virtual void Refresh();
		virtual void Update();
		
		virtual void EnterPressed() { AddToPlaylist(1); }
		virtual void SpacePressed() { AddToPlaylist(0); }
		
		void NextColumn();
		void PrevColumn();
		
		Menu<std::string> *Artists;
		Menu<string_pair> *Albums;
		Menu<MPD::Song> *Songs;
		
	protected:
		void AddToPlaylist(bool);
		
		static size_t itsLeftColWidth;
		static size_t itsMiddleColWidth;
		static size_t itsMiddleColStartX;
		static size_t itsRightColWidth;
		static size_t itsRightColStartX;
};

extern MediaLibrary *myLibrary;

#endif

