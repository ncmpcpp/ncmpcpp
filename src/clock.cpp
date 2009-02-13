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

/// NOTICE: Major part of this code is ported from ncmpc's clock screen

#include "clock.h"

#ifdef ENABLE_CLOCK

#include "display.h"
#include "global.h"
#include "settings.h"
#include "status_checker.h"

using namespace Global;

Scrollpad *Global::wClock;

namespace
{
	short disp[11] =
	{
		075557, 011111, 071747, 071717,
		055711, 074717, 074757, 071111,
		075757, 075717, 002020
	};
	
	long older[6], next[6], newer[6], mask;
	
	void set(int t, int n)
	{
		int m = 7 << n;
		for (int i = 0; i < 5; i++)
		{
			next[i] |= ((disp[t] >> ((4 - i) * 3)) & 07) << n;
			mask |= (next[i] ^ older[i]) & m;
		}
		if (mask & m)
			mask |= m;
	}
}

namespace Clock
{
	const size_t width = Config.clock_display_seconds ? 60 : 40;
	const size_t height = 8;
}

void Clock::Init()
{
	wClock = new Scrollpad((COLS-width)/2, (LINES-height)/2, width, height-1, "", Config.main_color, Border(Config.main_color));
	wClock->SetTimeout(ncmpcpp_window_timeout);
}

void Clock::Resize()
{
	if (width <= size_t(COLS) && height <= main_height)
	{
		wClock->MoveTo((COLS-width)/2, (LINES-height)/2);
		if (current_screen == csClock)
		{
			mPlaylist->Hide();
			Prepare();
			wClock->Display();
		}
	}
}

void Clock::Prepare()
{
	for (int i = 0; i < 5; i++)
		older[i] = newer[i] = next[i] = 0;
}

void Clock::Update()
{
	if (width > size_t(COLS) || height > main_height)
		return;
	
	time_t rawtime;
	time(&rawtime);
	tm *time = localtime(&rawtime);
	
	mask = 0;
	set(time->tm_sec % 10, 0);
	set(time->tm_sec / 10, 4);
	set(time->tm_min % 10, 10);
	set(time->tm_min / 10, 14);
	set(time->tm_hour % 10, 20);
	set(time->tm_hour / 10, 24);
	set(10, 7);
	set(10, 17);
	
	char buf[54];
	strftime(buf, 64, "%x", time);
	attron(COLOR_PAIR(Config.main_color));
	mvprintw(wCurrent->GetStartY()+wCurrent->GetHeight(), wCurrent->GetStartX()+(wCurrent->GetWidth()-strlen(buf))/2, "%s", buf);
	attroff(COLOR_PAIR(Config.main_color));
	refresh();
	
	for (int k = 0; k < 6; k++)
	{
		newer[k] = (newer[k] & ~mask) | (next[k] & mask);
		next[k] = 0;
		for (int s = 1; s >= 0; s--)
		{
			wCurrent->Reverse(s);
			for (int i = 0; i < 6; i++)
			{
				long a = (newer[i] ^ older[i]) & (s ? newer : older)[i];
				if (a != 0)
				{
					long t = 1 << 26;
					for (int j = 0; t; t >>= 1, j++)
					{
						if (a & t)
						{
							if (!(a & (t << 1)))
							{
								wCurrent->GotoXY(2*j+2, i);
							}
							if (Global::Config.clock_display_seconds || j < 18)
								*wCurrent << "  ";
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
}

void Clock::SwitchTo()
{
	if (width > size_t(COLS) || height > main_height)
	{
		ShowMessage("Screen is too small to display clock!");
	}
	else if (
	   current_screen != csClock
#	ifdef HAVE_TAGLIB_H
	&& current_screen != csTinyTagEditor
#	endif // HAVE_TAGLIB_H
		)
	{
		CLEAR_FIND_HISTORY;
		wCurrent = wClock;
		mPlaylist->Hide();
		current_screen = csClock;
		redraw_header = 1;
		Clock::Prepare();
		wCurrent->Display();
	}
}

#endif // ENABLE_CLOCK

