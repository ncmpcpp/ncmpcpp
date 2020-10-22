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

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>
#include <iomanip>
#include <iostream>
#include <fstream>

#include "bindings.h"
#include "configuration.h"
#include "config.h"
#include "mpdpp.h"
#include "format_impl.h"
#include "settings.h"
#include "utility/string.h"

namespace po = boost::program_options;

using std::cerr;
using std::cout;

namespace {

const char *env_home;

std::string xdg_config_home()
{
	std::string result;
	const char *env_xdg_config_home = getenv("XDG_CONFIG_HOME");
	if (env_xdg_config_home == nullptr)
		result = "~/.config/";
	else
	{
		result = env_xdg_config_home;
		if (!result.empty() && result.back() != '/')
			result += "/";
	}
	return result;
}

}

void expand_home(std::string &path)
{
	assert(env_home != nullptr);
	if (!path.empty())
	{
		size_t i = path.find("~");
		if (i != std::string::npos && (i == 0 || path[i - 1] == '@'))
			path.replace(i, 1, env_home);
	}
}

bool configure(int argc, char **argv)
{
	const std::vector<std::string> default_config_paths = {
		"~/.ncmpcpp/config",
		xdg_config_home() + "ncmpcpp/config"
	};

	const std::vector<std::string> default_bindings_paths = {
		"~/.ncmpcpp/bindings",
		xdg_config_home() + "ncmpcpp/bindings"
	};

	std::vector<std::string> bindings_paths;
	std::vector<std::string> config_paths;

	po::options_description options("Options");
	options.add_options()
		("host,h", po::value<std::string>()->value_name("HOST")->default_value("localhost"), "connect to server at host")
		("port,p", po::value<int>()->value_name("PORT")->default_value(6600), "connect to server at port")
		("current-song", po::value<std::string>()->value_name("FORMAT")->implicit_value("{{{(%l) }{{%a - }%t}}|{%f}}"), "print current song using given format and exit")
		("config,c", po::value<std::vector<std::string>>(&config_paths)->value_name("PATH")->default_value(default_config_paths, join<std::string>(default_config_paths, " AND ")), "specify configuration file(s)")
		("ignore-config-errors", "ignore unknown and invalid options in configuration files")
		("test-lyrics-fetchers", "check if lyrics fetchers work")
		("bindings,b", po::value<std::vector<std::string>>(&bindings_paths)->value_name("PATH")->default_value(default_bindings_paths, join<std::string>(default_bindings_paths, " AND ")), "specify bindings file(s)")
		("screen,s", po::value<std::string>()->value_name("SCREEN"), "specify the startup screen")
		("slave-screen,S", po::value<std::string>()->value_name("SCREEN"), "specify the startup slave screen")
		("help,?", "show help message")
		("version,v", "display version information")
		("quiet,q", "suppress logs and excess output")
	;

	po::variables_map vm;
	try
	{
		po::store(po::parse_command_line(argc, argv, options), vm);

		// suppress messages from std::clog
		if (vm.count("quiet"))
			std::clog.rdbuf(nullptr);

		if (vm.count("help"))
		{
			cout << "Usage: " << argv[0] << " [options]...\n" << options << "\n";
			return false;
		}
		if (vm.count("version"))
		{
			std::cout << "ncmpcpp " << VERSION << "\n\n"
			<< "optional screens compiled-in:\n"
	#		ifdef HAVE_TAGLIB_H
			<< " - tag editor\n"
			<< " - tiny tag editor\n"
	#		endif
	#		ifdef ENABLE_OUTPUTS
			<< " - outputs\n"
	#		endif
	#		ifdef ENABLE_VISUALIZER
			<< " - visualizer\n"
	#		endif
	#		ifdef ENABLE_CLOCK
			<< " - clock\n"
	#		endif
			<< "\nencoding detection: "
	#		ifdef HAVE_LANGINFO_H
			<< "enabled"
	#		else
			<< "disabled"
	#		endif // HAVE_LANGINFO_H
			<< "\nbuilt with support for:"
	#		ifdef HAVE_FFTW3_H
			<< " fftw"
	#		endif
			<< " ncurses"
	#		ifdef HAVE_TAGLIB_H
			<< " taglib"
	#		endif
			<< "\n";
			return false;
		}

		po::notify(vm);

		if (vm.count("test-lyrics-fetchers"))
		{
			std::vector<std::tuple<std::string, std::string, std::string>> fetcher_data = {
				std::make_tuple("azlyrics", "rihanna", "umbrella"),
				std::make_tuple("genius", "rihanna", "umbrella"),
				std::make_tuple("sing365", "rihanna", "umbrella"),
				std::make_tuple("lyricsmania", "rihanna", "umbrella"),
				std::make_tuple("metrolyrics", "rihanna", "umbrella"),
				std::make_tuple("justsomelyrics", "rihanna", "umbrella"),
				std::make_tuple("jahlyrics", "sean kingston", "dry your eyes"),
				std::make_tuple("plyrics", "offspring", "genocide"),
				std::make_tuple("tekstowo", "rihanna", "umbrella"),
				std::make_tuple("zeneszoveg", "rihanna", "umbrella"),
			};
			for (auto &data : fetcher_data)
			{
				auto fetcher = boost::lexical_cast<LyricsFetcher_>(std::get<0>(data));
				std::cout << std::setw(20)
				          << std::left
				          << fetcher->name()
				          << " : "
				          << std::flush;
				auto result = fetcher->fetch(std::get<1>(data), std::get<2>(data));
				std::cout << (result.first ? "ok" : "failed")
				          << "\n";
			}
			exit(0);
		}

		// get home directory
		env_home = getenv("HOME");
		if (env_home == nullptr)
		{
			cerr << "Fatal error: HOME environment variable is not defined\n";
			return false;
		}

		// read configuration
		std::for_each(config_paths.begin(), config_paths.end(), expand_home);
		if (Config.read(config_paths, vm.count("ignore-config-errors")) == false)
			exit(1);

		// read bindings
		std::for_each(bindings_paths.begin(), bindings_paths.end(), expand_home);
		if (Bindings.read(bindings_paths) == false)
			exit(1);
		Bindings.generateDefaults();

		// create directories
		boost::filesystem::create_directories(Config.ncmpcpp_directory);
		boost::filesystem::create_directory(Config.lyrics_directory);

		// try to get MPD connection details from environment variables
		// as they take precedence over these from the configuration.
		auto env_host = getenv("MPD_HOST");
		auto env_port = getenv("MPD_PORT");
		if (env_host != nullptr)
			Mpd.SetHostname(env_host);
		if (env_port != nullptr)
		{
			auto trimmed_env_port = boost::trim_copy<std::string>(env_port);
			try {
				Mpd.SetPort(boost::lexical_cast<int>(trimmed_env_port));
			} catch (boost::bad_lexical_cast &) {
				throw std::runtime_error("MPD_PORT environment variable ("
				                         + std::string(env_port)
				                         + ") is not a number");
			}
		}

		// if MPD connection details are provided as command line
		// parameters, use them as their priority is the highest.
		if (!vm["host"].defaulted())
			Mpd.SetHostname(vm["host"].as<std::string>());
		if (!vm["port"].defaulted())
			Mpd.SetPort(vm["port"].as<int>());
		Mpd.SetTimeout(Config.mpd_connection_timeout);

		// print current song
		if (vm.count("current-song"))
		{
			Mpd.Connect();
			auto s = Mpd.GetCurrentSong();
			if (!s.empty())
			{
				auto format = Format::parse(vm["current-song"].as<std::string>(), Format::Flags::Tag);
				std::cout << Format::stringify<char>(format, &s);
			}
			return false;
		}

		// custom startup screen
		if (vm.count("screen"))
		{
			auto screen = vm["screen"].as<std::string>();
			Config.startup_screen_type = stringtoStartupScreenType(screen);
			if (Config.startup_screen_type == ScreenType::Unknown)
			{
				std::cerr << "Unknown screen: " << screen << "\n";
				exit(1);
			}
		}

		// custom startup slave screen
		if (vm.count("slave-screen"))
		{
			auto screen = vm["slave-screen"].as<std::string>();
			Config.startup_slave_screen_type = stringtoStartupScreenType(screen);
			if (Config.startup_slave_screen_type == ScreenType::Unknown)
			{
				std::cerr << "Unknown slave screen: " << screen << "\n";
				exit(1);
			}
		}
	}
	catch (std::exception &e)
	{
		cerr << "Error while processing configuration: " << e.what() << "\n";
		exit(1);
	}
	return true;
}
