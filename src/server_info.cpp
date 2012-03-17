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
#include "server_info.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myOldScreen;

ServerInfo *myServerInfo = new ServerInfo;

void ServerInfo::Init()
{
	SetDimensions();
	w = new Scrollpad((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY, itsWidth, itsHeight, "MPD server info", Config.main_color, Config.window_border);
	
	Mpd.GetURLHandlers(itsURLHandlers);
	Mpd.GetTagTypes(itsTagTypes);
	
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
		ShowMessage("Screen is too small to display this window!");
		return;
	}
	
	if (!isInitialized)
		Init();
	
	// Resize() can fall back to old screen, so we need it updated
	myOldScreen = myScreen;
	
	if (hasToBeResized)
		Resize();
	
	myScreen = this;
	w->Window::Clear();
}

void ServerInfo::Resize()
{
	SetDimensions();
	if (itsHeight < 5) // screen too low to display this window
		return myOldScreen->SwitchTo();
	w->Resize(itsWidth, itsHeight);
	w->MoveTo((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY);
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
	w->Clear();
	
	*w << fmtBold << U("Version: ") << fmtBoldEnd << U("0.") << Mpd.Version() << U(".*\n");
	*w << fmtBold << U("Uptime: ") << fmtBoldEnd;
	ShowTime(*w, Mpd.Uptime(), 1);
	*w << '\n';
	*w << fmtBold << U("Time playing: ") << fmtBoldEnd << MPD::Song::ShowTime(Mpd.PlayTime()) << '\n';
	*w << '\n';
	*w << fmtBold << U("Total playtime: ") << fmtBoldEnd;
	ShowTime(*w, Mpd.DBPlayTime(), 1);
	*w << '\n';
	*w << fmtBold << U("Artist names: ") << fmtBoldEnd << Mpd.NumberOfArtists() << '\n';
	*w << fmtBold << U("Album names: ") << fmtBoldEnd << Mpd.NumberOfAlbums() << '\n';
	*w << fmtBold << U("Songs in database: ") << fmtBoldEnd << Mpd.NumberOfSongs() << '\n';
	*w << '\n';
	*w << fmtBold << U("Last DB update: ") << fmtBoldEnd << Timestamp(Mpd.DBUpdateTime()) << '\n';
	*w << '\n';
	*w << fmtBold << U("URL Handlers:") << fmtBoldEnd;
	for (MPD::TagList::const_iterator it = itsURLHandlers.begin(); it != itsURLHandlers.end(); ++it)
		*w << (it != itsURLHandlers.begin() ? U(", ") : U(" ")) << *it;
	*w << U("\n\n");
	*w << fmtBold << U("Tag Types:") << fmtBoldEnd;
	for (MPD::TagList::const_iterator it = itsTagTypes.begin(); it != itsTagTypes.end(); ++it)
		*w << (it != itsTagTypes.begin() ? U(", ") : U(" ")) << *it;
	
	w->Flush();
	w->Refresh();
}

void ServerInfo::SetDimensions()
{
	itsWidth = COLS*0.6;
	itsHeight = std::min(size_t(LINES*0.7), MainHeight);
}

