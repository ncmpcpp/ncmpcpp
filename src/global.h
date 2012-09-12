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

#include "mpdpp.h"
#include "screen.h"

namespace Global
{
	// currently active screen (displayed in main window)
	extern BasicScreen *myScreen;
	
	// for info, lyrics, popups to remember which screen return to
	extern BasicScreen *myOldScreen;
	
	// "real" screen switching (browser, search, etc.)
	extern BasicScreen *myPrevScreen;
	
	// points at the screen that was locked (or is null if no screen is locked)
	extern BasicScreen *myLockedScreen;
	
	// points at inactive screen, if locking was enabled and two screens are displayed
	extern BasicScreen *myInactiveScreen; 
	
	// header window (above main window)
	extern NC::Window *wHeader;
	
	// footer window (below main window)
	extern NC::Window *wFooter;
	
	// Y coordinate of top of main window
	extern size_t MainStartY;
	
	// height of main window
	extern size_t MainHeight;
	
	// indicates whether messages from Statusbar::msg function should be shown
	extern bool ShowMessages;
	
	// indicates whether seeking action in currently in progress
	extern bool SeekingInProgress;
	
	// string that represents volume in right top corner. being global
	// to be used for calculating width offsets in various files.
	extern std::string VolumeState;
	
	// global timer
	extern timeval Timer;
}

#endif
