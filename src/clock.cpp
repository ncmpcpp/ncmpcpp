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

/// NOTICE: Major part of this code is ported from ncmpc's clock screen

#include "clock.h"

#ifdef ENABLE_CLOCK

#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstring>

#include "global.h"
#include "playlist.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

Clock *myClock;

short Clock::disp[11] =
{
	075557, 011111, 071747, 071717,
	055711, 074717, 074757, 071111,
	075757, 075717, 002020
};

long Clock::older[6], Clock::next[6], Clock::newer[6], Clock::mask;

size_t Clock::Width;
const size_t Clock::Height = 8;

Clock::Clock()
{
	Width = Config.clock_display_seconds ? 60 : 40;
	
	m_pane = NC::Window(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border::None);
	w = NC::Window((COLS-Width)/2, (MainHeight-Height)/2+MainStartY, Width, Height-1, "", Config.main_color, NC::Border(Config.main_color));
}

void Clock::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	
	// used for clearing area out of clock window while resizing terminal
	m_pane.resize(width, MainHeight);
	m_pane.moveTo(x_offset, MainStartY);
	m_pane.refresh();
	
	if (Width <= width && Height <= MainHeight)
		w.moveTo(x_offset+(width-Width)/2, MainStartY+(MainHeight-Height)/2);
}

void Clock::switchTo()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width, false);
	if (Width > width || Height > MainHeight)
		Statusbar::print("Screen is too small to display clock");
	else
	{
		SwitchTo::execute(this);
		drawHeader();
		Prepare();
		m_pane.refresh();
		// clearing screen apparently fixes the problem with last digits being misrendered
		w.clear();
		w.display();
	}
}

std::wstring Clock::title()
{
	return L"Clock";
}

void Clock::update()
{
	if (Width > m_pane.getWidth() || Height > MainHeight)
	{
		using Global::myLockedScreen;
		using Global::myInactiveScreen;
		
		if (myLockedScreen)
		{
			if (myInactiveScreen != myLockedScreen)
				myScreen = myInactiveScreen;
			myLockedScreen->switchTo();
		}
		else
			myPlaylist->switchTo();
	}
	
	auto time = boost::posix_time::to_tm(Global::Timer);
	
	mask = 0;
	Set(time.tm_sec % 10, 0);
	Set(time.tm_sec / 10, 4);
	Set(time.tm_min % 10, 10);
	Set(time.tm_min / 10, 14);
	Set(time.tm_hour % 10, 20);
	Set(time.tm_hour / 10, 24);
	Set(10, 7);
	Set(10, 17);
	
	char buf[64];
	std::strftime(buf, 64, "%x", &time);
	attron(COLOR_PAIR(int(Config.main_color)));
	mvprintw(w.getStarty()+w.getHeight(), w.getStartX()+(w.getWidth()-strlen(buf))/2, "%s", buf);
	attroff(COLOR_PAIR(int(Config.main_color)));
	refresh();
	
	for (int k = 0; k < 6; ++k)
	{
		newer[k] = (newer[k] & ~mask) | (next[k] & mask);
		next[k] = 0;
		for (int s = 1; s >= 0; --s)
		{
			w << (s ? NC::Format::Reverse : NC::Format::NoReverse);
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
								w.goToXY(2*j+2, i);
							}
							if (Config.clock_display_seconds || j < 18)
								w << "  ";
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
	w.refresh();
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

