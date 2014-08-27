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

#include <iostream>

#include "cmdargs.h"
#include "config.h"
#include "mpdpp.h"
#include "settings.h"

void ParseArgv(int argc, char **argv)
{
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
			;
			exit(0);
		}
		
		if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--screen"))
		{
			if (++i == argc)
			{
				std::cerr << "No screen specified" << std::endl;
				exit(1);
			}
			Config.startup_screen_type = stringtoStartupScreenType(argv[i]);
			if (Config.startup_screen_type == ScreenType::Unknown)
			{
				std::cerr << "Invalid screen: " << argv[i] << std::endl;
				exit(1);
			}
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
	}
}
