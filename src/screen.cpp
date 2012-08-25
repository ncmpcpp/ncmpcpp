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

namespace
{
	void DrawScreenSeparator(int x)
	{
		attron(COLOR_PAIR(Config.main_color));
		mvvline(Global::MainStartY, x, 0, Global::MainHeight);
		attroff(COLOR_PAIR(Config.main_color));
		refresh();
	}
}

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

void UpdateInactiveScreen(BasicScreen *screen_to_be_set)
{
	if (myInactiveScreen && myLockedScreen != myInactiveScreen && myLockedScreen == screen_to_be_set)
	{
		// if we're here, the following conditions are (or at least should be) met:
		// 1. screen is split (myInactiveScreen is not null)
		// 2. current screen (myScreen) is not splittable, ie. is stacked on top of split screens
		// 3. current screen was activated while master screen was active
		// 4. we are returning to master screen
		// in such case we want to keep slave screen visible, so we never set it to null
		// as in "else" case. we also need to refresh it and redraw separator between
		// them as stacked screen probably has overwritten part ot it.
		myInactiveScreen->Refresh();
		DrawScreenSeparator(COLS*Config.locked_screen_width_part);
	}
	else
	{
		if (myLockedScreen == screen_to_be_set)
			myInactiveScreen = 0;
		else
			myInactiveScreen = myLockedScreen;
	}
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
				DrawScreenSeparator(x_offset-1);
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
