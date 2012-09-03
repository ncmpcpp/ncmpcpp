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

#include <sys/time.h>
#include <iomanip>

#include "global.h"
#include "helpers.h"
#include "server_info.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myOldScreen;

ServerInfo *myServerInfo = new ServerInfo;

void ServerInfo::Init()
{
	SetDimensions();
	w = new NC::Scrollpad((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY, itsWidth, itsHeight, "MPD server info", Config.main_color, Config.window_border);
	
	itsURLHandlers = Mpd.GetURLHandlers();
	itsTagTypes = Mpd.GetTagTypes();
	
	isInitialized = 1;
}

void ServerInfo::SwitchTo()
{
	using Global::myScreen;
	
	if (myScreen == this)
	{
		myOldScreen->SwitchTo();
		return;
	}
	if (MainHeight < 5)
	{
		ShowMessage("Screen is too small to display this window");
		return;
	}
	
	if (!isInitialized)
		Init();
	
	// Resize() can fall back to old screen, so we need it updated
	myOldScreen = myScreen;
	
	if (hasToBeResized)
		Resize();
	
	myScreen = this;
	//w->Window::clear();
}

void ServerInfo::Resize()
{
	SetDimensions();
	if (itsHeight < 5) // screen too low to display this window
		return myOldScreen->SwitchTo();
	w->resize(itsWidth, itsHeight);
	w->moveTo((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY);
	if (myOldScreen && myOldScreen->hasToBeResized) // resize background window
	{
		myOldScreen->Resize();
		myOldScreen->Refresh();
	}
	hasToBeResized = 0;
}

std::basic_string<my_char_t> ServerInfo::Title()
{
	return myOldScreen->Title();
}

void ServerInfo::Update()
{
	static timeval past = { 0, 0 };
	if (Global::Timer.tv_sec <= past.tv_sec)
		return;
	gettimeofday(&past, 0);
	
	Mpd.UpdateStats();
	w->clear();
	
	*w << NC::fmtBold << U("Version: ") << NC::fmtBoldEnd << U("0.") << Mpd.Version() << U(".*\n");
	*w << NC::fmtBold << U("Uptime: ") << NC::fmtBoldEnd;
	ShowTime(*w, Mpd.Uptime(), 1);
	*w << '\n';
	*w << NC::fmtBold << U("Time playing: ") << NC::fmtBoldEnd << MPD::Song::ShowTime(Mpd.PlayTime()) << '\n';
	*w << '\n';
	*w << NC::fmtBold << U("Total playtime: ") << NC::fmtBoldEnd;
	ShowTime(*w, Mpd.DBPlayTime(), 1);
	*w << '\n';
	*w << NC::fmtBold << U("Artist names: ") << NC::fmtBoldEnd << Mpd.NumberOfArtists() << '\n';
	*w << NC::fmtBold << U("Album names: ") << NC::fmtBoldEnd << Mpd.NumberOfAlbums() << '\n';
	*w << NC::fmtBold << U("Songs in database: ") << NC::fmtBoldEnd << Mpd.NumberOfSongs() << '\n';
	*w << '\n';
	*w << NC::fmtBold << U("Last DB update: ") << NC::fmtBoldEnd << Timestamp(Mpd.DBUpdateTime()) << '\n';
	*w << '\n';
	*w << NC::fmtBold << U("URL Handlers:") << NC::fmtBoldEnd;
	for (auto it = itsURLHandlers.begin(); it != itsURLHandlers.end(); ++it)
		*w << (it != itsURLHandlers.begin() ? U(", ") : U(" ")) << *it;
	*w << U("\n\n");
	*w << NC::fmtBold << U("Tag Types:") << NC::fmtBoldEnd;
	for (auto it = itsTagTypes.begin(); it != itsTagTypes.end(); ++it)
		*w << (it != itsTagTypes.begin() ? U(", ") : U(" ")) << *it;
	
	w->flush();
	w->refresh();
}

void ServerInfo::SetDimensions()
{
	itsWidth = COLS*0.6;
	itsHeight = std::min(size_t(LINES*0.7), MainHeight);
}

