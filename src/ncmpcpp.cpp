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

#include <cerrno>
#include <clocale>
#include <csignal>
#include <cstring>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/locale.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include "mpdpp.h"

#include "actions.h"
#include "bindings.h"
#include "screens/browser.h"
#include "charset.h"
#include "configuration.h"
#include "global.h"
#include "helpers.h"
#include "screens/lyrics.h"
#include "screens/outputs.h"
#include "screens/playlist.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "screens/visualizer.h"
#include "title.h"
#include "utility/conversion.h"

namespace ph = std::placeholders;

namespace {

std::ofstream errorlog;
std::streambuf *cerr_buffer;
std::streambuf *clog_buffer;

volatile bool run_resize_screen = false;
	
void sighandler(int sig)
{
	if (sig == SIGWINCH)
		run_resize_screen = true;
#if defined(__sun) && defined(__SVR4)
	// in solaris it is needed to reinstall the handler each time it's executed
	signal(sig, sighandler);
#endif // __sun && __SVR4
}

void do_at_exit()
{
	// restore old cerr & clog buffers
	std::cerr.rdbuf(cerr_buffer);
	std::clog.rdbuf(clog_buffer);
	errorlog.close();
	Mpd.Disconnect();
	NC::destroyScreen();
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
	
	std::setlocale(LC_ALL, "");
	std::locale::global(Charset::internalLocale());

	// clog might be overriden in configure, so preserve the original buffer.
	clog_buffer = std::clog.rdbuf();

	if (!configure(argc, argv))
		return 0;
	
	// always execute these commands, even if ncmpcpp use exit function
	atexit(do_at_exit);
	
	// redirect std::cerr output to ~/.ncmpcpp/error.log file
	errorlog.open((Config.ncmpcpp_directory + "error.log").c_str(), std::ios::app);
	cerr_buffer = std::cerr.rdbuf();
	std::cerr.rdbuf(errorlog.rdbuf());
	
	sigignore(SIGPIPE);
	signal(SIGWINCH, sighandler);

	Mpd.setNoidleCallback(Status::update);

	NC::initScreen(Config.colors_enabled, Config.mouse_support);
	
	Actions::OriginalStatusbarVisibility = Config.statusbar_visibility;

	if (Config.design == Design::Alternative)
		Config.statusbar_visibility = 0;
	
	Actions::setWindowsDimensions();
	Actions::validateScreenSize();
	Actions::initializeScreens();
	
	wHeader = new NC::Window(0, 0, COLS, Actions::HeaderHeight, "", Config.header_color, NC::Border());
	if (Config.header_visibility || Config.design == Design::Alternative)
		wHeader->display();
	
	wFooter = new NC::Window(0, Actions::FooterStartY, COLS, Actions::FooterHeight, "", Config.statusbar_color, NC::Border());
	wFooter->setPromptHook(Statusbar::Helpers::mainHook);
	
	// initialize global timer
	Timer = boost::posix_time::microsec_clock::local_time();

	// initialize global random number generator
	Global::RNG.seed(std::random_device()());
	
	// initialize playlist
	myPlaylist->switchTo();

	// go to startup screen
	if (Config.startup_screen_type != myScreen->type())
	{
		auto startup_screen = toScreen(Config.startup_screen_type);
		assert(startup_screen != nullptr);
		startup_screen->switchTo();
	}

	// lock current screen and go to the slave one if applicable
	if (Config.startup_slave_screen_type)
	{
		auto slave_screen_type = *Config.startup_slave_screen_type;
		bool screen_locked = myScreen->lock();
		if (screen_locked && slave_screen_type != myScreen->type())
		{
			auto slave_screen = toScreen(slave_screen_type);
			assert(slave_screen != nullptr);
			slave_screen->switchTo();
			if (!Config.startup_slave_screen_focus)
				Actions::get(Actions::Type::MasterScreen).execute();
		}
	}

	// local variables
	bool key_pressed = false;
	auto input = NC::Key::None;
	auto connect_attempt = boost::posix_time::from_time_t(0);
	auto update_environment = static_cast<Actions::UpdateEnvironment &>(
		Actions::get(Actions::Type::UpdateEnvironment));
	
	while (!Actions::ExitMainLoop)
	{
		try
		{
			if (!Mpd.Connected() && Timer - connect_attempt > boost::posix_time::seconds(1))
			{
				connect_attempt = Timer;
				// reset local status info
				Status::clear();
				// clear mpd callback
				wFooter->clearFDCallbacksList();
				try
				{
					Mpd.Connect();
					if (Mpd.Version() < 16)
					{
							Mpd.Disconnect();
							throw MPD::ClientError(MPD_ERROR_STATE, "MPD < 0.16.0 is not supported", false);
					}
				}
				catch (MPD::ClientError &e)
				{
					Status::handleClientError(e);
				}
			}

			if (run_resize_screen)
			{
				Actions::resizeScreen(true);
				run_resize_screen = false;
			}

			update_environment.run(!key_pressed, key_pressed, false);

			input = readKey(*wFooter);
			key_pressed = input != NC::Key::None;
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
				std::any_of(k.first, k.second, std::bind(&Binding::execute, ph::_1));
			}
			catch (ConversionError &e)
			{
				Statusbar::printf("Invalid value: %1%", e.value());
			}
			catch (OutOfBounds &e)
			{
				Statusbar::printf("Error: %1%", e.errorMessage());
			}
			catch (NC::PromptAborted &)
			{
				Statusbar::printf("Action aborted");
			}

			if (myScreen == myPlaylist)
				myPlaylist->enableHighlighting();
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
