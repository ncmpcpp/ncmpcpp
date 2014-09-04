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

#include <cerrno>
#include <clocale>
#include <csignal>
#include <cstring>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/locale.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include "mpdpp.h"

#include "actions.h"
#include "bindings.h"
#include "browser.h"
#include "charset.h"
#include "configuration.h"
#include "global.h"
#include "error.h"
#include "helpers.h"
#include "lyrics.h"
#include "outputs.h"
#include "playlist.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "visualizer.h"
#include "title.h"
#include "utility/conversion.h"

namespace
{
	std::ofstream errorlog;
	std::streambuf *cerr_buffer;
	bool run_resize_screen = false;
	
#	if !defined(WIN32)
	void sighandler(int sig)
	{
		if (sig == SIGPIPE)
		{
			Statusbar::print("SIGPIPE (broken pipe signal) received");
		}
		else if (sig == SIGWINCH)
		{
			run_resize_screen = true;
		}
#		if defined(__sun) && defined(__SVR4)
		// in solaris it is needed to reinstall the handler each time it's executed
		signal(sig, sighandler);
#		endif // __sun && __SVR4
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
	
	using Global::wHeader;
	using Global::wFooter;
	
	using Global::VolumeState;
	using Global::Timer;
	
	srand(time(nullptr));
	std::setlocale(LC_ALL, "");
	std::locale::global(Charset::internalLocale());
	
	if (!configure(argc, argv))
		return 0;
	
	// always execute these commands, even if ncmpcpp use exit function
	atexit(do_at_exit);
	
	// redirect std::cerr output to ~/.ncmpcpp/error.log file
	errorlog.open((Config.ncmpcpp_directory + "error.log").c_str(), std::ios::app);
	cerr_buffer = std::cerr.rdbuf();
	std::cerr.rdbuf(errorlog.rdbuf());
	
	NC::initScreen("ncmpcpp ver. " VERSION, Config.colors_enabled);
	
	Actions::OriginalStatusbarVisibility = Config.statusbar_visibility;

	if (Config.design == Design::Alternative)
		Config.statusbar_visibility = 0;
	
	Actions::setWindowsDimensions();
	Actions::validateScreenSize();
	Actions::initializeScreens();
	
	wHeader = new NC::Window(0, 0, COLS, Actions::HeaderHeight, "", Config.header_color, NC::Border::None);
	if (Config.header_visibility || Config.design == Design::Alternative)
		wHeader->display();
	
	wFooter = new NC::Window(0, Actions::FooterStartY, COLS, Actions::FooterHeight, "", Config.statusbar_color, NC::Border::None);
	wFooter->setGetStringHelper(Statusbar::Helpers::getString);
	
	// initialize global timer
	Timer = boost::posix_time::microsec_clock::local_time();
	
	// initialize playlist
	myPlaylist->switchTo();
	
	// local variables
	bool key_pressed = false;
	Key input = Key::noOp;
	auto past = boost::posix_time::from_time_t(0);
	
	/// enable mouse
	mouseinterval(0);
	if (Config.mouse_support)
		mousemask(ALL_MOUSE_EVENTS, 0);
	
#	ifndef WIN32
	signal(SIGPIPE, sighandler);
	signal(SIGWINCH, sighandler);
	// ignore Ctrl-C
	sigignore(SIGINT);
#	endif // !WIN32
	
	while (!Actions::ExitMainLoop)
	{
		try
		{
			if (!Mpd.Connected())
			{
				wFooter->clearFDCallbacksList();
				try
				{
					Mpd.Connect();
					if (Mpd.Version() < 16)
					{
						Mpd.Disconnect();
						throw MPD::ClientError(MPD_ERROR_STATE, "MPD < 0.16.0 is not supported", false);
					}
					wFooter->addFDCallback(Mpd.GetFD(), Statusbar::Helpers::mpd);
					Status::clear(); // reset local status info
					Status::update(-1); // we need info about new connection
					
					if (Config.jump_to_now_playing_song_at_start)
					{
						int curr_pos = Status::get().currentSongPosition();
						if  (curr_pos >= 0)
							myPlaylist->main().highlight(curr_pos);
					}
					
					// Set TCP_NODELAY on the tcp socket as we are using write-write-read pattern
					// a lot (idle - write, noidle - write, then read the result of noidle), which
					// kills the performance.
					int flag = 1;
					setsockopt(Mpd.GetFD(), IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

					// go to startup screen
					if (Config.startup_screen_type != myScreen->type())
						toScreen(Config.startup_screen_type)->switchTo();
					myScreen->refresh();
					
					myBrowser->fetchSupportedExtensions();
#					ifdef ENABLE_OUTPUTS
					myOutputs->FetchList();
#					endif // ENABLE_OUTPUTS
#					ifdef ENABLE_VISUALIZER
					myVisualizer->ResetFD();
					if (myScreen == myVisualizer)
						myVisualizer->SetFD();
					myVisualizer->FindOutputID();
#					endif // ENABLE_VISUALIZER
					
					Statusbar::printf("Connected to %1%", Mpd.GetHostname());
				}
				catch (MPD::ClientError &e)
				{
					Status::handleClientError(e);
				}
			}
			
			// update timer, status if necessary etc.
			Status::trace(!key_pressed, true);

			if (run_resize_screen)
			{
				Actions::resizeScreen(true);
				run_resize_screen = false;
			}
			
			// header stuff
			if ((myScreen == myPlaylist || myScreen == myBrowser || myScreen == myLyrics)
			&&  (Timer - past > boost::posix_time::milliseconds(500))
			)
			{
				drawHeader();
				past = Timer;
			}
			
			if (key_pressed)
				myScreen->refreshWindow();
			input = Key::read(*wFooter);
			key_pressed = input != Key::noOp;
			
			if (!key_pressed)
				continue;

			// The reason we want to update timer here is that if the timer is updated
			// in Status::trace, then Key::read usually blocks for 500ms and if key is
			// pressed 400ms after Key::read was called, we end up with Timer that is
			// ~400ms inaccurate. On the other hand, if keys are being pressed, we don't
			// want to update timer in both Status::trace and here. Therefore we update
			// timer in Status::trace only if there was no recent input.
			Timer = boost::posix_time::microsec_clock::local_time();

			try
			{
				auto k = Bindings.get(input);
				std::any_of(k.first, k.second, boost::bind(&Binding::execute, _1));
			}
			catch (ConversionError &e)
			{
				Statusbar::printf("Couldn't convert value \"%1%\" to target type", e.value());
			}
			catch (OutOfBounds &e)
			{
				Statusbar::printf("Error: %1%", e.errorMessage());
			}

			if (myScreen == myPlaylist)
				myPlaylist->EnableHighlighting();
		}
		catch (MPD::ClientError &e)
		{
			Status::handleClientError(e);
		}
		catch (MPD::ServerError &e)
		{
			Status::handleServerError(e);
		}
		catch (std::exception &e)
		{
			Statusbar::printf("Unexpected error: %1%", e.what());
		}
	}
	return 0;
}
