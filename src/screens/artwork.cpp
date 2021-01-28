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
std::thread Artwork::t;

namespace
{
	ArtworkBackend* backend;
	std::string current_artwork_path = "";
	std::string prev_uri = "";
}

// For signaling worker thread
namespace
{
	std::condition_variable worker_cv;
	std::mutex worker_mtx;
	bool worker_exit = false;
	std::queue<std::function<void()>> worker_queue;
}

Artwork::Artwork()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border()))
{
	namespace fs = boost::filesystem;
	temp_file_name = (fs::temp_directory_path() / fs::unique_path()).string();

	backend = new UeberzugBackend;
	backend->init();

	// Spawn worker thread
	t = std::thread(worker);
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
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_exit = true;
	}
	worker_cv.notify_all();
	std::remove(temp_file_name.c_str());
	t.join();
}

void Artwork::removeArtwork(bool reset_artwork)
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace([=] { worker_removeArtwork(reset_artwork); });
	}
	worker_cv.notify_all();
}

void Artwork::updateArtwork()
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace([] { worker_updateArtwork(); });
	}
	worker_cv.notify_all();
}

void Artwork::updateArtwork(std::string uri)
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace([=] { worker_updateArtwork(uri); });
	}
	worker_cv.notify_all();
}


// Worker thread methods

void Artwork::worker_drawArtwork(std::string path, int x_offset, int y_offset, int width, int height)
{
	backend->updateArtwork(path, x_offset, y_offset, width, height);
	drawn = true;
	current_artwork_path = path;
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

void Artwork::worker_updateArtwork(const std::string &uri)
{
	size_t x_offset, width;
	myArtwork->getWindowResizeParams(x_offset, width);

	// Draw same file if uri is unchanged
	if (prev_uri == uri)
	{
		worker_drawArtwork(temp_file_name, x_offset, MainStartY, width, MainHeight);
		return;
	}

	auto buffer = worker_fetchArtwork(uri);

	// Draw default artwork if MPD doesn't return anything
	if (buffer.empty()) {
		worker_drawArtwork(Config.albumart_default_path, x_offset, MainStartY, width, MainHeight);
		return;
	}

	// Write artwork to a temporary file and draw
	prev_uri = uri;
	temp_file.open(temp_file_name, std::ios::trunc | std::ios::binary);
	temp_file.write(reinterpret_cast<char *>(buffer.data()), buffer.size());
	temp_file.close();
	worker_drawArtwork(temp_file_name, x_offset, MainStartY, width, MainHeight);
}

std::vector<uint8_t> Artwork::worker_fetchArtwork(const std::string &uri)
{
	// Check if MPD connection is still alive
	try
	{
		Mpd_artwork.getStatus();
	}
	catch (const MPD::ClientError &e)
	{
		// Reconnect to MPD if closed and try again
		if (e.code() == MPD_ERROR_CLOSED)
		{
			Mpd_artwork.Disconnect();
			Mpd_artwork.Connect();
		}
	}

	// Get artwork
	// TODO: make this order configurable
	const std::vector<std::string> art_sources = { "albumart", "readpicture" };
	std::vector<uint8_t> buffer;
	for (const auto &source : art_sources)
	{
		buffer = Mpd_artwork.GetArtwork(uri, source);
		if (!buffer.empty())
			return buffer;
	}

	return buffer;
}

void Artwork::worker()
{
	while (true)
	{
		std::function<void()> op;
		{
			// Wait for signal
			std::unique_lock<std::mutex> lck(worker_mtx);
			worker_cv.wait(lck, [] { return worker_exit || !worker_queue.empty(); });

			if (worker_exit)
			{
				worker_exit = false;
				return;
			}

			// Take the first operation in queue
			op = worker_queue.front();
			worker_queue.pop();
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

			// Do the requested operation
			op();
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
