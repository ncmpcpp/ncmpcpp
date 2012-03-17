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

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <sys/time.h>

#include "ncmpcpp.h"
#include "mpdpp.h"
#include "screen.h"

namespace Global
{
	extern BasicScreen *myScreen;
	extern BasicScreen *myOldScreen;	// for info, lyrics, popups
	extern BasicScreen *myPrevScreen;	// "real" screen switching (browser, search, etc.)
	extern BasicScreen *myLockedScreen;     // points at the screen that was locked (or is null if no screen is locked)
	extern BasicScreen *myInactiveScreen;   // points at inactive screen, if locking was enabled and two screens are displayed
	
	extern Window *wHeader;
	extern Window *wFooter;
	
	extern size_t MainStartY;
	extern size_t MainHeight;
	
	extern bool BlockItemListUpdate;
	
	extern bool UpdateStatusImmediately;
	extern bool MessagesAllowed;
	extern bool SeekingInProgress;
	extern bool RedrawHeader;
	extern bool RedrawStatusbar;
	
	extern std::string VolumeState;
	
	extern timeval Timer;
}

#endif

