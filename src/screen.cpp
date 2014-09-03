/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#include "global.h"
#include "screen.h"
#include "settings.h"

using Global::myScreen;
using Global::myLockedScreen;
using Global::myInactiveScreen;

void drawSeparator(int x)
{
	attron(COLOR_PAIR(int(Config.main_color)));
	mvvline(Global::MainStartY, x, 0, Global::MainHeight);
	attroff(COLOR_PAIR(int(Config.main_color)));
	refresh();
}

void genericMouseButtonPressed(NC::Window &w, MEVENT me)
{
	if (me.bstate & BUTTON2_PRESSED)
	{
		if (Config.mouse_list_scroll_whole_page)
			w.scroll(NC::Scroll::PageDown);
		else
			for (size_t i = 0; i < Config.lines_scrolled; ++i)
				w.scroll(NC::Scroll::Down);
	}
	else if (me.bstate & BUTTON4_PRESSED)
	{
		if (Config.mouse_list_scroll_whole_page)
			w.scroll(NC::Scroll::PageUp);
		else
			for (size_t i = 0; i < Config.lines_scrolled; ++i)
				w.scroll(NC::Scroll::Up);
	}
}

void scrollpadMouseButtonPressed(NC::Scrollpad &w, MEVENT me)
{
	if (me.bstate & BUTTON2_PRESSED)
	{
		for (size_t i = 0; i < Config.lines_scrolled; ++i)
			w.scroll(NC::Scroll::Down);
	}
	else if (me.bstate & BUTTON4_PRESSED)
	{
		for (size_t i = 0; i < Config.lines_scrolled; ++i)
			w.scroll(NC::Scroll::Up);
	}
}

/***********************************************************************/

void BaseScreen::getWindowResizeParams(size_t &x_offset, size_t &width, bool adjust_locked_screen)
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
				myLockedScreen->resize();
				myLockedScreen->refresh();
				drawSeparator(x_offset-1);
			}
		}
	}
}

bool BaseScreen::lock()
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

void BaseScreen::unlock()
{
	if (myInactiveScreen && myInactiveScreen != myLockedScreen)
		myScreen = myInactiveScreen;
	if (myScreen != myLockedScreen)
		myLockedScreen->switchTo();
	myLockedScreen = 0;
	myInactiveScreen = 0;
}

/***********************************************************************/

void applyToVisibleWindows(std::function<void(BaseScreen *)> f)
{
	if (myLockedScreen && myScreen->isMergable())
	{
		if (myScreen == myLockedScreen)
		{
			if (myInactiveScreen)
				f(myInactiveScreen);
		}
		else
			f(myLockedScreen);
	}
	f(myScreen);
}

void updateInactiveScreen(BaseScreen *screen_to_be_set)
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
		myInactiveScreen->refresh();
		drawSeparator(COLS*Config.locked_screen_width_part);
	}
	else
	{
		if (myLockedScreen == screen_to_be_set)
			myInactiveScreen = 0;
		else
			myInactiveScreen = myLockedScreen;
	}
}

bool isVisible(BaseScreen *screen)
{
	assert(screen != 0);
	if (myLockedScreen && myScreen->isMergable())
		return screen == myScreen || screen == myInactiveScreen || screen == myLockedScreen;
	else
		return screen == myScreen;
}
