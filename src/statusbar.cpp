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

#include "global.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"

using Global::wFooter;

namespace {//

timeval statusbarLockTime;
int statusbarLockDelay = -1;

bool statusbarBlockUpdate = false;
bool progressbarBlockUpdate = false;
bool statusbarAllowUnlock = true;

}

void Progressbar::lock()
{
	progressbarBlockUpdate = true;
}

void Progressbar::unlock()
{
	progressbarBlockUpdate = false;
}

bool Progressbar::isUnlocked()
{
	return !progressbarBlockUpdate;
}

void Progressbar::draw(unsigned int elapsed, unsigned int time)
{
	unsigned pb_width = wFooter->getWidth();
	unsigned howlong = time ? pb_width*elapsed/time : 0;
	if (Config.progressbar_boldness)
		*wFooter << NC::fmtBold;
	*wFooter << Config.progressbar_color;
	if (Config.progressbar[2] != '\0')
	{
		wFooter->goToXY(0, 0);
		for (unsigned i = 0; i < pb_width; ++i)
			*wFooter << Config.progressbar[2];
		wFooter->goToXY(0, 0);
	}
	else
		mvwhline(wFooter->raw(), 0, 0, 0, pb_width);
	if (time)
	{
		*wFooter << Config.progressbar_elapsed_color;
		pb_width = std::min(size_t(howlong), wFooter->getWidth());
		for (unsigned i = 0; i < pb_width; ++i)
			*wFooter << Config.progressbar[0];
		if (howlong < wFooter->getWidth())
			*wFooter << Config.progressbar[1];
		*wFooter << NC::clEnd;
	}
	*wFooter << NC::clEnd;
	if (Config.progressbar_boldness)
		*wFooter << NC::fmtBoldEnd;
}

void Statusbar::lock()
{
	if (Config.statusbar_visibility)
		statusbarBlockUpdate = true;
	else
		progressbarBlockUpdate = true;
	statusbarAllowUnlock = false;
}

void Statusbar::unlock()
{
	statusbarAllowUnlock = true;
	if (statusbarLockDelay < 0)
	{
		if (Config.statusbar_visibility)
			statusbarBlockUpdate = false;
		else
			progressbarBlockUpdate = false;
	}
	if (!Mpd.isPlaying())
	{
		if (Config.new_design)
			Progressbar::draw(Mpd.GetElapsedTime(), Mpd.GetTotalTime());
		else
			put() << wclrtoeol;
		wFooter->refresh();
	}
}

bool Statusbar::isUnlocked()
{
	return !statusbarBlockUpdate;
}

void Statusbar::tryRedraw()
{
	using Global::Timer;
	if (statusbarLockDelay > 0
	&&  Timer.tv_sec >= statusbarLockTime.tv_sec+statusbarLockDelay)
	{
		statusbarLockDelay = -1;
		
		if (Config.statusbar_visibility)
			statusbarBlockUpdate = !statusbarAllowUnlock;
		else
			progressbarBlockUpdate = !statusbarAllowUnlock;
		
		if (Mpd.GetState() != MPD::psPlay && !statusbarBlockUpdate && !progressbarBlockUpdate)
		{
			if (Config.new_design)
				Progressbar::draw(Mpd.GetElapsedTime(), Mpd.GetTotalTime());
			else
				put() << wclrtoeol;
			wFooter->refresh();
		}
	}
}

NC::Window &Statusbar::put()
{
	*wFooter << NC::XY(0, Config.statusbar_visibility ? 1 : 0) << wclrtoeol;
	return *wFooter;
}

void Statusbar::msg(const char *format, ...)
{
	if (Global::ShowMessages && statusbarAllowUnlock)
	{
		statusbarLockTime = Global::Timer;
		statusbarLockDelay = Config.message_delay_time;
		if (Config.statusbar_visibility)
			statusbarBlockUpdate = 1;
		else
			progressbarBlockUpdate = 1;
		wFooter->goToXY(0, Config.statusbar_visibility);
		*wFooter << NC::fmtBoldEnd;
		va_list list;
		va_start(list, format);
		wmove(wFooter->raw(), Config.statusbar_visibility, 0);
		vw_printw(wFooter->raw(), format, list);
		wclrtoeol(wFooter->raw());
		va_end(list);
		wFooter->refresh();
	}
}

void Statusbar::Helpers::mpd()
{
	Mpd.OrderDataFetching();
}

void Statusbar::Helpers::getString(const std::wstring &)
{
	Status::trace();
}

void Statusbar::Helpers::ApplyFilterImmediately::operator()(const std::wstring &ws)
{
	using Global::myScreen;
	// if input queue is not empty, we don't want to update filter since next
	// character will be taken from queue immediately, trigering this function
	// again and thus making it inefficient, so let's apply filter only if
	// "real" user input arrived. however, we want to apply filter if ENTER
	// is next in queue, so its effects will be seen.
	if (wFooter->inputQueue().empty() || wFooter->inputQueue().front() == KEY_ENTER)
	{
		if (m_ws != ws)
		{
			m_ws = ws;
			m_f->applyFilter(ToString(m_ws));
			myScreen->RefreshWindow();
		}
		Status::trace();
	}
}
