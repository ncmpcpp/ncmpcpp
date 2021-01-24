/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#include "screens/artwork.h"

#ifdef ENABLE_ARTWORK

#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>

#include "macro_utilities.h"
#include "screens/screen_switcher.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

Artwork* myArtwork;

bool Artwork::drawn = false;
std::string Artwork::temp_file_name;
std::ofstream Artwork::temp_file;

namespace
{
	ArtworkBackend* backend;
	std::string current_artwork_path = "";
	std::string prev_uri = "";
}

// For signaling worker thread
namespace
{
	std::condition_variable cv;
	std::mutex cv_mtx;
	bool cv_ready = false;
	bool cv_exit_worker = false;

	enum operation { UPDATE, UPDATE_URI, REMOVE };
	operation cv_op;

	std::string cv_uri;
	bool cv_reset_artwork;
}

Artwork::Artwork()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border()))
{
	namespace fs = boost::filesystem;
	temp_file_name = (fs::temp_directory_path() / fs::unique_path()).string();

	backend = new UeberzugBackend;
	backend->init();

	// Spawn worker thread
	std::thread t(worker);
	t.detach();
	std::atexit(stop);
}

void Artwork::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	updateArtwork();
}

void Artwork::switchTo()
{
	SwitchTo::execute(this);
	drawHeader();
}

std::wstring Artwork::title() { return L"Artwork"; }

void Artwork::stop()
{
	{
		std::lock_guard<std::mutex> lck(cv_mtx);
		cv_exit_worker = true;
		cv_ready = true;
		cv.notify_all();
	}
	std::remove(temp_file_name.c_str());
}

void Artwork::removeArtwork(bool reset_artwork)
{
	std::lock_guard<std::mutex> lck(cv_mtx);
	cv_op = REMOVE;
	cv_reset_artwork = reset_artwork;
	cv_ready = true;
	cv.notify_all();
}

void Artwork::updateArtwork()
{
	std::lock_guard<std::mutex> lck(cv_mtx);
	cv_op = UPDATE;
	cv_ready = true;
	cv.notify_all();
}

void Artwork::updateArtwork(std::string uri)
{
	std::lock_guard<std::mutex> lck(cv_mtx);
	cv_op = UPDATE_URI;
	cv_uri = uri;
	cv_ready = true;
	cv.notify_all();
}


// Worker thread methods

void Artwork::worker_drawArtwork(std::string path, int x_offset, int y_offset, int width, int height)
{
	backend->updateArtwork(path, x_offset, y_offset, width, height);
	drawn = true;
}

void Artwork::worker_removeArtwork(bool reset_artwork)
{
	backend->removeArtwork();
	drawn = false;
	if (reset_artwork)
	{
		current_artwork_path = "";
	}
}

void Artwork::worker_updateArtwork()
{
	if (current_artwork_path == "")
		return;

	size_t x_offset, width;
	myArtwork->getWindowResizeParams(x_offset, width);
	worker_drawArtwork(current_artwork_path, x_offset, MainStartY, width, MainHeight);
}

void Artwork::worker_updateArtwork(std::string uri)
{
	size_t x_offset, width;
	myArtwork->getWindowResizeParams(x_offset, width);

	// Draw same file if uri is unchanged
	if (prev_uri == uri)
	{
		worker_drawArtwork(temp_file_name, x_offset, MainStartY, width, MainHeight);
		return;
	}

	// Try to read artwork from MPD
	std::vector<uint8_t> buffer;
	for (size_t i = 0; i < 2; ++i)
	{
		try{
			buffer = Mpd_artwork.GetArtwork(uri);
		}
		catch (const MPD::ClientError &e)
		{
			// Reconnect to MPD if closed and try again
			if (e.code() == MPD_ERROR_CLOSED)
			{
				Mpd_artwork.Disconnect();
				Mpd_artwork.Connect();
				continue;
			}
		}
		break;
	}

	// Draw default artwork if MPD doesn't return anything
	if (buffer.size() == 0) {
		current_artwork_path = Config.albumart_default_path;
		worker_drawArtwork(Config.albumart_default_path, x_offset, MainStartY, width, MainHeight);
		return;
	}

	// Write artwork to a temporary file and draw
	prev_uri = uri;
	temp_file.open(temp_file_name, std::ios::trunc | std::ios::binary);
	temp_file.write(reinterpret_cast<char *>(buffer.data()), buffer.size());
	temp_file.close();
	current_artwork_path = temp_file_name;
	worker_drawArtwork(temp_file_name, x_offset, MainStartY, width, MainHeight);
}

void Artwork::worker()
{
	while (true)
	{
		std::string uri;
		bool reset_artwork;
		operation op;
		{
			// Wait for signal
			std::unique_lock<std::mutex> lck(cv_mtx);
			cv.wait(lck, [] { return cv_ready; });
			cv_ready = false;
			if (cv_exit_worker)
				return;
			op = cv_op;
			uri = cv_uri;
			reset_artwork = cv_reset_artwork;
		}

		try
		{
			if (!Mpd_artwork.Connected())
			{
				Mpd_artwork.SetHostname(Mpd.GetHostname());
				Mpd_artwork.SetPort(Mpd.GetPort());
				Mpd_artwork.SetPassword(Mpd.GetPassword());
				Mpd_artwork.Connect();
			}

			switch (op)
			{
				case UPDATE:
					worker_updateArtwork();
					break;
				case UPDATE_URI:
					worker_updateArtwork(uri);
					break;
				case REMOVE:
					worker_removeArtwork(reset_artwork);
					break;
			}

		}
		catch (std::exception &e)
		{
			Statusbar::printf("Unexpected error: %1%", e.what());
		}
	}
}


// UeberzugBackend

boost::process::child UeberzugBackend::process;
boost::process::opstream UeberzugBackend::stream;

void UeberzugBackend::init()
{
	namespace bp = boost::process;

	std::vector<std::string> args = {"layer", "--silent", "--parser", "json"};
	try
	{
		process = bp::child(bp::search_path("ueberzug"), bp::args(args), bp::std_in < stream);
		std::atexit(stop);
	}
	catch (const bp::process_error &e) {}
}

void UeberzugBackend::updateArtwork(std::string path, int x_offset, int y_offset, int width, int height)
{
	if (!process.running())
		return;

	boost::property_tree::ptree pt;
	pt.put("action", "add");
	pt.put("identifier", "albumart");
	pt.put("synchronously_draw", true);
	pt.put("scaling_position_x", 0.5);
	pt.put("scaling_position_y", 0.5);
	pt.put("scaler", Config.albumart_scaler);
	pt.put("path", path);
	pt.put("x", x_offset);
	pt.put("y", y_offset);
	pt.put("width", width);
	pt.put("height", height);

	boost::property_tree::write_json(stream, pt, /* pretty */ false);
	stream << std::endl;
}

void UeberzugBackend::removeArtwork()
{
	if (!process.running())
		return;

	boost::property_tree::ptree pt;
	pt.put("action", "remove");
	pt.put("identifier", "albumart");

	boost::property_tree::write_json(stream, pt, /* pretty */ false);
	stream << std::endl;
}

void UeberzugBackend::stop()
{
	// close the underlying pipe, which causes ueberzug to exit
	stream.pipe().close();
	process.wait();
}

#endif // ENABLE_ARTWORK
