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

#include <cassert>

#include "screen.h"
#include "global.h"

using Global::myScreen;
using Global::myLockedScreen;
using Global::myInactiveScreen;

void ApplyToVisibleWindows(void (BasicScreen::*f)())
{
	if (myLockedScreen && myScreen->isMergable())
	{
		if (myScreen == myLockedScreen)
		{
			if (myInactiveScreen)
				(myInactiveScreen->*f)();
		}
		else
			(myLockedScreen->*f)();
	}
	(myScreen->*f)();
}

void UpdateInactiveScreen(BasicScreen *screen)
{
	myInactiveScreen = myLockedScreen == screen ? 0 : myLockedScreen;
}

bool isVisible(BasicScreen *screen)
{
	assert(screen != 0);
	if (myLockedScreen && myScreen->isMergable())
		return screen == myScreen || screen == myInactiveScreen || screen == myLockedScreen;
	else
		return screen == myScreen;
}

void BasicScreen::GetWindowResizeParams(size_t &x_offset, size_t &width, bool adjust_locked_screen)
{
	width = COLS;
	x_offset = 0;
	if (myLockedScreen && myInactiveScreen)
	{
		size_t locked_width = COLS*Config.locked_screen_width_part;
		if (myLockedScreen == this)
			width = locked_width;
		else
		{
			width = COLS-locked_width-1;
			x_offset = locked_width+1;
			
			if (adjust_locked_screen)
			{
				myLockedScreen->Resize();
				myLockedScreen->Refresh();
				
				attron(COLOR_PAIR(Config.main_color));
				mvvline(Global::MainStartY, x_offset-1, 0, Global::MainHeight);
				attroff(COLOR_PAIR(Config.main_color));
				refresh();
			}
		}
	}
}

bool BasicScreen::Lock()
{
	if (myLockedScreen)
		return false;
	if (isLockable())
	{
		 myLockedScreen = this;
		 return true;
	}
	else
		return false;
}

void BasicScreen::Unlock()
{
	if (myInactiveScreen && myInactiveScreen != myLockedScreen)
		myScreen = myInactiveScreen;
	myLockedScreen->SwitchTo();
	myLockedScreen = 0;
	myInactiveScreen = 0;
}
