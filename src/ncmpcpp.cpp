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
#include "ncmpcpp.h"

#include "actions.h"
#include "browser.h"
#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "playlist.h"
#include "settings.h"
#include "status.h"
#include "visualizer.h"

using namespace Global;
using namespace MPD;

BasicScreen *Global::myScreen;
BasicScreen *Global::myOldScreen;
BasicScreen *Global::myPrevScreen;
BasicScreen *Global::myLockedScreen;
BasicScreen *Global::myInactiveScreen;

Window *Global::wHeader;
Window *Global::wFooter;

size_t Global::MainStartY;
size_t Global::MainHeight;

bool Global::MessagesAllowed = 0;
bool Global::SeekingInProgress = 0;
bool Global::RedrawHeader = 1;

namespace
{
	std::ofstream errorlog;
	std::streambuf *cerr_buffer;
	
#	if !defined(WIN32)
	void sighandler(int signal)
	{
		if (signal == SIGPIPE)
		{
			ShowMessage("SIGPIPE (broken pipe signal) received");
		}
		else if (signal == SIGWINCH)
		{
			Action::OrderResize = true;
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
		DestroyScreen();
#		endif // USE_PDCURSES
		WindowTitle("");
	}
}

int main(int argc, char **argv)
{
	srand(time(0));
	setlocale(LC_ALL, "");
	
	Config.CheckForCommandLineConfigFilePath(argv, argc);
	
	Config.SetDefaults();
	Config.Read();
	
	Config.GenerateColumns();
	Key.GenerateKeybindings();
	
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
	
	if (!ConnectToMPD())
		exit(1);
	
	CreateDir(Config.ncmpcpp_directory);
	
	// always execute these commands, even if ncmpcpp use exit function
	atexit(do_at_exit);
	
	// redirect std::cerr output to ~/.ncmpcpp/error.log file
	errorlog.open((Config.ncmpcpp_directory + "error.log").c_str(), std::ios::app);
	cerr_buffer = std::cerr.rdbuf();
	std::cerr.rdbuf(errorlog.rdbuf());
	
	InitScreen("ncmpcpp ver. "VERSION, Config.colors_enabled);
	
	Action::OriginalStatusbarVisibility = Config.statusbar_visibility;
	
	if (!Config.titles_visibility)
		wattron(stdscr, COLOR_PAIR(Config.main_color));
	
	if (Config.new_design)
		Config.statusbar_visibility = 0;
	
	Action::SetWindowsDimensions();
	Action::ValidateScreenSize();
	
	wHeader = new Window(0, 0, COLS, Action::HeaderHeight, "", Config.header_color, brNone);
	if (Config.header_visibility || Config.new_design)
		wHeader->Display();
	
	wFooter = new Window(0, Action::FooterStartY, COLS, Action::FooterHeight, "", Config.statusbar_color, brNone);
	wFooter->SetTimeout(ncmpcpp_window_timeout);
	wFooter->SetGetStringHelper(StatusbarGetStringHelper);
	if (Mpd.SupportsIdle())
		wFooter->AddFDCallback(Mpd.GetFD(), StatusbarMPDCallback);
	wFooter->CreateHistory();
	
	// initialize screens to browser as default previous screen
	myScreen = myBrowser;
	myPrevScreen = myBrowser;
	myOldScreen = myBrowser;
	
	// go to playlist
	myPlaylist->SwitchTo();
	myPlaylist->UpdateTimer();
	
	// go to startup screen
	if (Config.startup_screen != myScreen)
		Config.startup_screen->SwitchTo();
	
	Mpd.SetStatusUpdater(NcmpcppStatusChanged, 0);
	Mpd.SetErrorHandler(NcmpcppErrorCallback, 0);
	
	// local variables
	int input = 0;
	
	timeval past = { 0, 0 };
	// local variables end
	
#	ifndef WIN32
	signal(SIGPIPE, sighandler);
	signal(SIGWINCH, sighandler);
#	endif // !WIN32
	
	mouseinterval(0);
	if (Config.mouse_support)
		mousemask(ALL_MOUSE_EVENTS, 0);
	
	Mpd.OrderDataFetching();
	if (Config.jump_to_now_playing_song_at_start)
	{
		TraceMpdStatus();
		int curr_pos = Mpd.GetCurrentSongPos();
		if  (curr_pos >= 0)
			myPlaylist->Items->Highlight(curr_pos);
	}
	
	while (!Action::ExitMainLoop)
	{
		if (!Mpd.Connected())
		{
			if (!wFooter->FDCallbacksListEmpty())
				wFooter->ClearFDCallbacksList();
			ShowMessage("Attempting to reconnect...");
			if (Mpd.Connect())
			{
				ShowMessage("Connected to %s", Mpd.GetHostname().c_str());
				if (Mpd.SupportsIdle())
				{
					wFooter->AddFDCallback(Mpd.GetFD(), StatusbarMPDCallback);
					Mpd.OrderDataFetching(); // we need info about new connection
				}
				MessagesAllowed = 0;
#				ifdef ENABLE_VISUALIZER
				myVisualizer->ResetFD();
				if (myScreen == myVisualizer)
					myVisualizer->SetFD();
				myVisualizer->FindOutputID();
#				endif // ENABLE_VISUALIZER
			}
		}
		
		TraceMpdStatus();
		
		MessagesAllowed = 1;
		
		if (Action::OrderResize)
			Action::ResizeScreen();
		
		// header stuff
		if (((Timer.tv_sec == past.tv_sec && Timer.tv_usec >= past.tv_usec+500000) || Timer.tv_sec > past.tv_sec)
		&&   (myScreen == myPlaylist || myScreen == myBrowser || myScreen == myLyrics)
		   )
		{
			RedrawHeader = 1;
			gettimeofday(&past, 0);
		}
		if (Config.header_visibility && RedrawHeader)
		{
			if (Config.new_design)
			{
				std::basic_string<my_char_t> title = myScreen->Title();
				*wHeader << XY(0, 3) << wclrtoeol;
				*wHeader << fmtBold << Config.alternative_ui_separator_color;
				mvwhline(wHeader->Raw(), 2, 0, 0, COLS);
				mvwhline(wHeader->Raw(), 4, 0, 0, COLS);
				*wHeader << XY((COLS-Window::Length(title))/2, 3);
				*wHeader << Config.header_color << title << clEnd;
				*wHeader << clEnd << fmtBoldEnd;
			}
			else
			{
				*wHeader << XY(0, 0) << wclrtoeol << fmtBold << myScreen->Title() << fmtBoldEnd;
				*wHeader << Config.volume_color;
				*wHeader << XY(wHeader->GetWidth()-VolumeState.length(), 0) << VolumeState;
				*wHeader << clEnd;
			}
			wHeader->Refresh();
			RedrawHeader = 0;
		}
		// header stuff end
		
		if (input != ERR)
			myScreen->RefreshWindow();
		wFooter->ReadKey(input);
		
		if (input == ERR)
			continue;
		
		NcmpcppKeys::Binding k = Key.Bindings.equal_range(input);
		for (; k.first != k.second; ++k.first)
			if (k.first->second->Execute())
				break;
		
		if (myScreen == myPlaylist)
			myPlaylist->EnableHighlighting();
		
#		ifdef ENABLE_VISUALIZER
		// visualizer sets timeout to 40ms, but since only it needs such small
		// value, we should restore defalt one after switching to another screen.
		if (wFooter->GetTimeout() < ncmpcpp_window_timeout
		&&  !(myScreen == myVisualizer || myLockedScreen == myVisualizer || myInactiveScreen == myVisualizer)
		   )
			wFooter->SetTimeout(ncmpcpp_window_timeout);
#		endif // ENABLE_VISUALIZER
	}
	return 0;
}
