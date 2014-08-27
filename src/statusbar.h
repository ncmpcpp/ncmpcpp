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

#ifndef NCMPCPP_STATUSBAR_H
#define NCMPCPP_STATUSBAR_H

#include <boost/format.hpp>
#include "settings.h"
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

/// tries to clear current message put there using Statusbar::printf if there is any
void tryRedraw();

/// clears statusbar and move cursor to beginning of line
/// @return window object that represents statusbar
NC::Window &put();

namespace Helpers {//

/// called when statusbar window detects incoming idle notification
void mpd();

/// called each time user types another character while inside Window::getString
bool getString(const char *);

/// called each time user changes current filter (while being inside Window::getString)
struct ApplyFilterImmediately
{
	template <typename StringT>
	ApplyFilterImmediately(Filterable *f, StringT &&filter)
	: m_f(f), m_s(std::forward<StringT>(filter)) { }
	
	bool operator()(const char *s);
	
private:
	Filterable *m_f;
	std::string m_s;
};

struct TryExecuteImmediateCommand
{
	bool operator()(const char *s);
	
private:
	std::string m_s;
};

}

/// displays message in statusbar for a given period of time
void print(int delay, const std::string& message);

/// displays message in statusbar for period of time set in configuration file
inline void print(const std::string &message)
{
	print(Config.message_delay_time, message);
}

/// displays formatted message in statusbar for period of time set in configuration file
template <typename FormatT>
void printf(FormatT &&fmt)
{
	print(Config.message_delay_time, boost::format(std::forward<FormatT>(fmt)).str());
}
template <typename FormatT, typename ArgT, typename... Args>
void printf(FormatT &&fmt, ArgT &&arg, Args&&... args)
{
	printf(boost::format(std::forward<FormatT>(fmt)) % std::forward<ArgT>(arg),
		std::forward<Args>(args)...
	);
}

/// displays formatted message in statusbar for a given period of time
template <typename FormatT>
void printf(int delay, FormatT &&fmt)
{
	print(delay, boost::format(std::forward<FormatT>(fmt)).str());
}
template <typename FormatT, typename ArgT, typename... Args>
void printf(int delay, FormatT &&fmt, ArgT &&arg, Args&&... args)
{
	printf(delay, boost::format(std::forward<FormatT>(fmt)) % std::forward<ArgT>(arg),
		std::forward<Args>(args)...
	);
}

}

#endif // NCMPCPP_STATUSBAR_H
