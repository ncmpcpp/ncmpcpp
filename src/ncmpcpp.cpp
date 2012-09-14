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

#include <cerrno>
#include <clocale>
#include <csignal>
#include <cstring>
#include <sys/time.h>

#include <iostream>
#include <fstream>
#include <stdexcept>

#include "mpdpp.h"

#include "actions.h"
#include "bindings.h"
#include "browser.h"
#include "cmdargs.h"
#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "playlist.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "visualizer.h"
#include "title.h"

namespace
{
	std::ofstream errorlog;
	std::streambuf *cerr_buffer;
	
#	if !defined(WIN32)
	void sighandler(int signal)
	{
		if (signal == SIGPIPE)
		{
			Statusbar::msg("SIGPIPE (broken pipe signal) received");
		}
		else if (signal == SIGWINCH)
		{
			Action::ResizeScreen(true);
		}
	}
#	endif // !WIN32
	
	void do_at_exit()
	{
		// restore old cerr buffer
		std::cerr.rdbuf(cerr_buffer);
		errorlog.close();
		Mpd.Disconnect();
#		ifndef USE_PDCURSES // destroying screen somehow crashes pdcurses
		NC::destroyScreen();
#		endif // USE_PDCURSES
		windowTitle("");
	}
}

int main(int argc, char **argv)
{
	using Global::myScreen;
	using Global::myOldScreen;
	using Global::myPrevScreen;
	using Global::myLockedScreen;
	using Global::myInactiveScreen;
	
	using Global::wHeader;
	using Global::wFooter;
	
	using Global::ShowMessages;
	using Global::VolumeState;
	using Global::Timer;
	
	srand(time(0));
	setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));
	
	Config.CheckForCommandLineConfigFilePath(argv, argc);
	
	Config.SetDefaults();
	Config.Read();
	Config.GenerateColumns();
	
	if (!Bindings.read(Config.ncmpcpp_directory + "bindings"))
		return 1;
	Bindings.generateDefaults();
	
	if (getenv("MPD_HOST"))
		Mpd.SetHostname(getenv("MPD_HOST"));
	if (getenv("MPD_PORT"))
		Mpd.SetPort(atoi(getenv("MPD_PORT")));
	
	if (Config.mpd_host != "localhost")
		Mpd.SetHostname(Config.mpd_host);
	if (Config.mpd_port != 6600)
		Mpd.SetPort(Config.mpd_port);
	
	Mpd.SetTimeout(Config.mpd_connection_timeout);
	Mpd.SetIdleEnabled(Config.enable_idle_notifications);
	
	if (argc > 1)
		ParseArgv(argc, argv);
	
	if (!Action::ConnectToMPD())
		exit(1);
	
	if (Mpd.Version() < 16)
	{
		std::cout << "MPD < 0.16.0 is not supported, please upgrade.\n";
		exit(1);
	}
	
	CreateDir(Config.ncmpcpp_directory);
	
	// always execute these commands, even if ncmpcpp use exit function
	atexit(do_at_exit);
	
	// redirect std::cerr output to ~/.ncmpcpp/error.log file
	errorlog.open((Config.ncmpcpp_directory + "error.log").c_str(), std::ios::app);
	cerr_buffer = std::cerr.rdbuf();
	std::cerr.rdbuf(errorlog.rdbuf());
	
	NC::initScreen("ncmpcpp ver. " VERSION, Config.colors_enabled);
	
	Action::OriginalStatusbarVisibility = Config.statusbar_visibility;
	
	if (!Config.titles_visibility)
		wattron(stdscr, COLOR_PAIR(Config.main_color));
	
	if (Config.new_design)
		Config.statusbar_visibility = 0;
	
	Action::SetWindowsDimensions();
	Action::ValidateScreenSize();
	Action::InitializeScreens();
	
	wHeader = new NC::Window(0, 0, COLS, Action::HeaderHeight, "", Config.header_color, NC::brNone);
	if (Config.header_visibility || Config.new_design)
		wHeader->display();
	
	wFooter = new NC::Window(0, Action::FooterStartY, COLS, Action::FooterHeight, "", Config.statusbar_color, NC::brNone);
	wFooter->setTimeout(500);
	wFooter->setGetStringHelper(Statusbar::Helpers::getString);
	if (Mpd.SupportsIdle())
		wFooter->addFDCallback(Mpd.GetFD(), Statusbar::Helpers::mpd);
	wFooter->createHistory();
	
	// initialize screens to browser as default previous screen
	myScreen = myBrowser;
	myPrevScreen = myBrowser;
	myOldScreen = myBrowser;
	
	// initialize global timer
	gettimeofday(&Timer, 0);
	
	// go to playlist
	myPlaylist->switchTo();
	myPlaylist->UpdateTimer();
	
	// go to startup screen
	if (Config.startup_screen != myScreen)
		Config.startup_screen->switchTo();
	
	Mpd.SetStatusUpdater(Status::update, 0);
	Mpd.SetErrorHandler(Status::handleError, 0);
	
	// local variables
	Key input(0, Key::Standard);
	timeval past = { 0, 0 };
	// local variables end
	
	mouseinterval(0);
	if (Config.mouse_support)
		mousemask(ALL_MOUSE_EVENTS, 0);
	
	Mpd.OrderDataFetching();
	if (Config.jump_to_now_playing_song_at_start)
	{
		Status::trace();
		int curr_pos = Mpd.GetCurrentSongPos();
		if  (curr_pos >= 0)
			myPlaylist->main().highlight(curr_pos);
	}
	
#	ifndef WIN32
	signal(SIGPIPE, sighandler);
	signal(SIGWINCH, sighandler);
#	endif // !WIN32
	
	while (!Action::ExitMainLoop)
	{
		if (!Mpd.Connected())
		{
			if (!wFooter->FDCallbacksListEmpty())
				wFooter->clearFDCallbacksList();
			Statusbar::msg("Attempting to reconnect...");
			if (Mpd.Connect())
			{
				Statusbar::msg("Connected to %s", Mpd.GetHostname().c_str());
				if (Mpd.SupportsIdle())
				{
					wFooter->addFDCallback(Mpd.GetFD(), Statusbar::Helpers::mpd);
					Mpd.OrderDataFetching(); // we need info about new connection
				}
				ShowMessages = false;
#				ifdef ENABLE_VISUALIZER
				myVisualizer->ResetFD();
				if (myScreen == myVisualizer)
					myVisualizer->SetFD();
				myVisualizer->FindOutputID();
#				endif // ENABLE_VISUALIZER
			}
		}
		
		Status::trace();
		
		ShowMessages = true;
		
		// header stuff
		if (((Timer.tv_sec == past.tv_sec && Timer.tv_usec >= past.tv_usec+500000) || Timer.tv_sec > past.tv_sec)
		&&   (myScreen == myPlaylist || myScreen == myBrowser || myScreen == myLyrics)
		   )
		{
			drawHeader();
			past = Timer;
		}
		
		// header stuff end
		
		if (input != Key::noOp)
			myScreen->refreshWindow();
		input = Key::read(*wFooter);
		
		if (input == Key::noOp)
			continue;
		
		auto k = Bindings.get(input);
		for (; k.first != k.second; ++k.first)
		{
			Binding &b = k.first->second;
			if (b.isSingle())
			{
				if (b.action()->Execute())
					break;
			}
			else
			{
				auto chain = b.chain();
				for (auto it = chain->begin(); it != chain->end(); ++it)
					if (!(*it)->Execute())
						break;
				break;
			}
		}
		
		if (myScreen == myPlaylist)
			myPlaylist->EnableHighlighting();
		
#		ifdef ENABLE_VISUALIZER
		// visualizer sets timeout to 40ms, but since only it needs such small
		// value, we should restore defalt one after switching to another screen.
		if (wFooter->getTimeout() < 500
		&&  !(myScreen == myVisualizer || myLockedScreen == myVisualizer || myInactiveScreen == myVisualizer)
		   )
			wFooter->setTimeout(500);
#		endif // ENABLE_VISUALIZER
	}
	return 0;
}
