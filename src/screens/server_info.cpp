/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iomanip>

#include "global.h"
#include "helpers.h"
#include "screens/server_info.h"
#include "statusbar.h"
#include "screens/screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

ServerInfo *myServerInfo;

ServerInfo::ServerInfo()
: m_timer(boost::posix_time::from_time_t(0))
{
	SetDimensions();
	w = NC::Scrollpad((COLS-m_width)/2, (MainHeight-m_height)/2+MainStartY, m_width, m_height, "MPD server info", Config.main_color, Config.window_border);
}

void ServerInfo::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
	{
		SwitchTo::execute(this);
		
		m_url_handlers.clear();
		std::copy(
			std::make_move_iterator(Mpd.GetURLHandlers()),
			std::make_move_iterator(MPD::StringIterator()),
			std::back_inserter(m_url_handlers)
		);

		m_tag_types.clear();
		std::copy(
			std::make_move_iterator(Mpd.GetTagTypes()),
			std::make_move_iterator(MPD::StringIterator()),
			std::back_inserter(m_tag_types)
		);
	}
	else
		switchToPreviousScreen();
}

void ServerInfo::resize()
{
	SetDimensions();
	w.resize(m_width, m_height);
	w.moveTo((COLS-m_width)/2, (MainHeight-m_height)/2+MainStartY);
	if (previousScreen() && previousScreen()->hasToBeResized) // resize background window
	{
		previousScreen()->resize();
		previousScreen()->refresh();
	}
	hasToBeResized = 0;
}

std::wstring ServerInfo::title()
{
	return previousScreen()->title();
}

void ServerInfo::update()
{
	if (Global::Timer - m_timer < boost::posix_time::seconds(1))
		return;
	m_timer = Global::Timer;
	
	MPD::Statistics stats = Mpd.getStatistics();
	if (stats.empty())
		return;
	
	w.clear();
	
	w << NC::Format::Bold << "Version: " << NC::Format::NoBold << "0." << Mpd.Version() << ".*\n";
	w << NC::Format::Bold << "Uptime: " << NC::Format::NoBold;
	ShowTime(w, stats.uptime(), 1);
	w << '\n';
	w << NC::Format::Bold << "Time playing: " << NC::Format::NoBold << MPD::Song::ShowTime(stats.playTime()) << '\n';
	w << '\n';
	w << NC::Format::Bold << "Total playtime: " << NC::Format::NoBold;
	ShowTime(w, stats.dbPlayTime(), 1);
	w << '\n';
	w << NC::Format::Bold << "Artist names: " << NC::Format::NoBold << stats.artists() << '\n';
	w << NC::Format::Bold << "Album names: " << NC::Format::NoBold << stats.albums() << '\n';
	w << NC::Format::Bold << "Songs in database: " << NC::Format::NoBold << stats.songs() << '\n';
	w << '\n';
	w << NC::Format::Bold << "Last DB update: " << NC::Format::NoBold << Timestamp(stats.dbUpdateTime()) << '\n';
	w << '\n';
	w << NC::Format::Bold << "URL Handlers:" << NC::Format::NoBold;
	for (auto it = m_url_handlers.begin(); it != m_url_handlers.end(); ++it)
		w << (it != m_url_handlers.begin() ? ", " : " ") << *it;
	w << "\n\n";
	w << NC::Format::Bold << "Tag Types:" << NC::Format::NoBold;
	for (auto it = m_tag_types.begin(); it != m_tag_types.end(); ++it)
		w << (it != m_tag_types.begin() ? ", " : " ") << *it;
	
	w.flush();
	w.refresh();
}

void ServerInfo::SetDimensions()
{
	m_width = COLS*0.6;
	m_height = std::min(size_t(LINES*0.7), MainHeight);
}

