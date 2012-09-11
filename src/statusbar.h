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

#ifndef _STATUSBAR_H
#define _STATUSBAR_H

#include "gcc.h"
#include "interfaces.h"
#include "window.h"

namespace Progressbar {//

/// locks progressbar (usually used for seeking)
void lock();

/// unlocks progressbar (usually right after seeking is done)
void unlock();

/// @return true if progressbar is unlocked
bool isUnlocked();

/// draws progressbar
void draw(unsigned elapsed, unsigned time);

}

namespace Statusbar{//

/// locks statusbar (usually for prompting the user)
void lock();

/// unlocks statusbar (usually after prompting the user)
void unlock();

/// @return true if statusbar is unlocked
bool isUnlocked();

/// tries to clear current message put there using Statusbar::msg if there is any
void tryRedraw();

/// clears statusbar and move cursor to beginning of line
/// @return window object that represents statusbar
NC::Window &put();

/// displays message in statusbar for period of time set in configuration file
void msg(const char *format, ...) GNUC_PRINTF(1, 2);

namespace Helpers {//

/// called when statusbar window detects incoming idle notification
void mpd();

/// called each time user types another character while inside Window::getString
void getString(const std::wstring &);

/// called each time user changes current filter (while being inside Window::getString)
struct ApplyFilterImmediately
{
	ApplyFilterImmediately(Filterable *f, const std::wstring &filter)
	: m_f(f), m_ws(filter) { }
	
	void operator()(const std::wstring &ws);
	
private:
	Filterable *m_f;
	std::wstring m_ws;
};

}

}

#endif // _STATUSBAR_H
