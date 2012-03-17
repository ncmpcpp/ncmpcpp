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

/// NOTICE: Major part of this code is ported from ncmpc's clock screen

#include "clock.h"

#ifdef ENABLE_CLOCK

#include <cstring>
#include <sys/time.h>

#include "global.h"
#include "playlist.h"
#include "settings.h"
#include "status.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

Clock *myClock = new Clock;

short Clock::disp[11] =
{
	075557, 011111, 071747, 071717,
	055711, 074717, 074757, 071111,
	075757, 075717, 002020
};

long Clock::older[6], Clock::next[6], Clock::newer[6], Clock::mask;

size_t Clock::Width;
const size_t Clock::Height = 8;

void Clock::Init()
{
	Width = Config.clock_display_seconds ? 60 : 40;
	
	itsPane = new Window(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	w = new Window((COLS-Width)/2, (MainHeight-Height)/2+MainStartY, Width, Height-1, "", Config.main_color, Border(Config.main_color));
	isInitialized = 1;
}

void Clock::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	
	// used for clearing area out of clock window while resizing terminal
	itsPane->Resize(width, MainHeight);
	itsPane->MoveTo(x_offset, MainStartY);
	itsPane->Refresh();
	
	if (Width <= width && Height <= MainHeight)
		w->MoveTo(x_offset+(width-Width)/2, MainStartY+(MainHeight-Height)/2);
}

void Clock::SwitchTo()
{
	using Global::myLockedScreen;
	
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width, false);
	if (Width > width || Height > MainHeight)
	{
		ShowMessage("Screen is too small to display clock!");
		if (myLockedScreen)
			UpdateInactiveScreen(myLockedScreen);
		return;
	}
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	Global::RedrawHeader = 1;
	Prepare();
	itsPane->Refresh();
	// clearing screen apparently fixes the problem with last digits being misrendered
	w->Clear();
	w->Display();
}

std::basic_string<my_char_t> Clock::Title()
{
	return U("Clock");
}

void Clock::Update()
{
	if (Width > itsPane->GetWidth() || Height > MainHeight)
	{
		using Global::myLockedScreen;
		using Global::myInactiveScreen;
		
		if (myLockedScreen)
		{
			if (myInactiveScreen != myLockedScreen)
				myScreen = myInactiveScreen;
			myLockedScreen->SwitchTo();
		}
		else
			myPlaylist->SwitchTo();
	}
	
	static timeval past = { 0, 0 };
	gettimeofday(&past, 0);
	
	tm *time = localtime(&past.tv_sec);
	
	mask = 0;
	Set(time->tm_sec % 10, 0);
	Set(time->tm_sec / 10, 4);
	Set(time->tm_min % 10, 10);
	Set(time->tm_min / 10, 14);
	Set(time->tm_hour % 10, 20);
	Set(time->tm_hour / 10, 24);
	Set(10, 7);
	Set(10, 17);
	
	char buf[64];
	strftime(buf, 64, "%x", time);
	attron(COLOR_PAIR(Config.main_color));
	mvprintw(w->GetStartY()+w->GetHeight(), w->GetStartX()+(w->GetWidth()-strlen(buf))/2, "%s", buf);
	attroff(COLOR_PAIR(Config.main_color));
	refresh();
	
	for (int k = 0; k < 6; ++k)
	{
		newer[k] = (newer[k] & ~mask) | (next[k] & mask);
		next[k] = 0;
		for (int s = 1; s >= 0; --s)
		{
			*w << (s ? fmtReverse : fmtReverseEnd);
			for (int i = 0; i < 6; ++i)
			{
				long a = (newer[i] ^ older[i]) & (s ? newer : older)[i];
				if (a != 0)
				{
					long t = 1 << 26;
					for (int j = 0; t; t >>= 1, ++j)
					{
						if (a & t)
						{
							if (!(a & (t << 1)))
							{
								w->GotoXY(2*j+2, i);
							}
							if (Config.clock_display_seconds || j < 18)
								*w << "  ";
						}
					}
				}
				if (!s)
				{
					older[i] = newer[i];
				}
			}
		}
	}
	w->Refresh();
}

void Clock::Prepare()
{
	for (int i = 0; i < 5; ++i)
		older[i] = newer[i] = next[i] = 0;
}

void Clock::Set(int t, int n)
{
	int m = 7 << n;
	for (int i = 0; i < 5; ++i)
	{
		next[i] |= ((disp[t] >> ((4 - i) * 3)) & 07) << n;
		mask |= (next[i] ^ older[i]) & m;
	}
	if (mask & m)
		mask |= m;
}

#endif // ENABLE_CLOCK

