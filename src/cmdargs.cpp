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

#include <cassert>
#include <cstring>
#include <iostream>

#include "actions.h"
#include "charset.h"
#include "cmdargs.h"
#include "config.h"
#include "mpdpp.h"
#include "settings.h"

#include "help.h"
#include "playlist.h"
#include "browser.h"
#include "search_engine.h"
#include "media_library.h"
#include "playlist_editor.h"
#include "tag_editor.h"
#include "outputs.h"
#include "visualizer.h"
#include "clock.h"

void ParseArgv(int argc, char **argv)
{
	std::string now_playing_format = "{{{(%l) }{{%a - }%t}}|{%f}}";
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--host"))
		{
			if (++i >= argc)
				exit(1);
			Mpd.SetHostname(argv[i]);
			continue;
		}
		if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port"))
		{
			if (++i >= argc)
				exit(1);
			Mpd.SetPort(atoi(argv[i]));
			continue;
		}
		else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
		{
			std::cout << "ncmpcpp " << VERSION << "\n\n"
			<< "optional screens compiled-in:\n"
#			ifdef HAVE_TAGLIB_H
			<< " - tag editor\n"
			<< " - tiny tag editor\n"
#			endif
#			ifdef HAVE_CURL_CURL_H
			<< " - artist info\n"
#			endif
#			ifdef ENABLE_OUTPUTS
			<< " - outputs\n"
#			endif
#			ifdef ENABLE_VISUALIZER
			<< " - visualizer\n"
#			endif
#			ifdef ENABLE_CLOCK
			<< " - clock\n"
#			endif
			<< "\nencoding detection: "
#			ifdef HAVE_LANGINFO_H
			<< "enabled"
#			else
			<< "disabled"
#			endif // HAVE_LANGINFO_H
			<< "\nbuilt with support for:"
#			ifdef HAVE_CURL_CURL_H
			<< " curl"
#			endif
#			ifdef HAVE_ICONV_H
			<< " iconv"
#			endif
#			ifdef HAVE_FFTW3_H
			<< " fftw"
#			endif
#			ifdef USE_PDCURSES
			<< " pdcurses"
#			else
			<< " ncurses"
#			endif
#			ifdef HAVE_TAGLIB_H
			<< " taglib"
#			endif
#			ifdef NCMPCPP_UNICODE
			<< " unicode"
#			endif
			<< std::endl;
			exit(0);
		}
		else if (!strcmp(argv[i], "-?") || !strcmp(argv[i], "--help"))
		{
			std::cout
			<< "Usage: ncmpcpp [OPTION]...\n"
			<< "  -h, --host                connect to server at host [localhost]\n"
			<< "  -p, --port                connect to server at port [6600]\n"
			<< "  -c, --config              use alternative configuration file\n"
			<< "  -s, --screen <name>       specify the startup screen\n"
			<< "  -?, --help                show help message\n"
			<< "  -v, --version             display version information\n"
			<< "  --now-playing             display now playing song [" << now_playing_format << "]\n"
			;
			exit(0);
		}
		
		if (!Action::ConnectToMPD())
			exit(1);
		
		if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--screen"))
		{
			if (++i == argc) {
				std::cerr << "No screen specified" << std::endl;
				exit(1);
			}
			if (!strcmp(argv[i], "help"))
				Config.startup_screen = myHelp;
			else if (!strcmp(argv[i], "playlist"))
				Config.startup_screen = myPlaylist;
			else if (!strcmp(argv[i], "browser"))
				Config.startup_screen = myBrowser;
			else if (!strcmp(argv[i], "search-engine"))
				Config.startup_screen = mySearcher;
			else if (!strcmp(argv[i], "media-library"))
				Config.startup_screen = myLibrary;
			else if (!strcmp(argv[i], "playlist-editor"))
				Config.startup_screen = myPlaylistEditor;
#			ifdef HAVE_TAGLIB_H
			else if (!strcmp(argv[i], "tag-editor"))
				Config.startup_screen = myTagEditor;
#			endif // HAVE_TAGLIB_H
#			ifdef ENABLE_OUTPUTS
			else if (!strcmp(argv[i], "outputs"))
				Config.startup_screen = myOutputs;
#			endif // ENABLE_OUTPUTS
#			ifdef ENABLE_VISUALIZER
			else if (!strcmp(argv[i], "visualizer"))
				Config.startup_screen = myVisualizer;
#			endif // ENABLE_VISUALIZER
#			ifdef ENABLE_CLOCK
			else if (!strcmp(argv[i], "clock"))
				Config.startup_screen = myClock;
#			endif // ENABLE_CLOCK
			else {
				std::cerr << "Invalid screen: " << argv[i] << std::endl;
				exit(1);
			}
		}
		else if (!strcmp(argv[i], "--now-playing"))
		{
			Mpd.UpdateStatus();
			if (!Mpd.GetErrorMessage().empty())
			{
				std::cerr << "MPD error: " << Mpd.GetErrorMessage() << std::endl;
				exit(1);
			}
			if (Mpd.isPlaying())
			{
				if (argc > ++i)
				{
					if (MPD::Song::isFormatOk("now-playing format", argv[i]))
					{
						// apply additional pair of braces
						now_playing_format = "{";
						now_playing_format += argv[i];
						now_playing_format += "}";
						replace(now_playing_format, "\\n", "\n");
						replace(now_playing_format, "\\t", "\t");
					}
				}
				std::cout << IConv::utf8ToLocale(
					Mpd.GetCurrentlyPlayingSong().toString(now_playing_format, Config.tags_separator)) << "\n";
			}
			exit(0);
		}
		else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--config"))
		{
			// this is used in Configuration::CheckForCommandLineConfigFilePath, ignoring here.
			++i;
		}
		else
		{
			std::cerr << "Invalid option: " << argv[i] << std::endl;
			exit(1);
		}
		if (!Mpd.GetErrorMessage().empty())
		{
			std::cerr << "Error: " << Mpd.GetErrorMessage() << std::endl;
			exit(1);
		}
	}
}
