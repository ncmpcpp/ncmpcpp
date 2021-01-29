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

std::string Artwork::temp_file_name;
std::ofstream Artwork::temp_file;
std::thread Artwork::t;
ArtworkBackend* Artwork::backend;
std::string Artwork::current_artwork_path = "";
std::string Artwork::prev_uri = "";
bool Artwork::drawn = false;
bool Artwork::before_inital_draw = true;

// For signaling worker thread
std::condition_variable Artwork::worker_cv;
std::mutex Artwork::worker_mtx;
std::chrono::time_point<std::chrono::steady_clock> Artwork::query_time;
bool Artwork::worker_exit = false;
std::vector<std::pair<Artwork::WorkerOp, std::function<void()>>> Artwork::worker_queue;


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
		// Need to specify distinct operations for reset_artwork true/false to
		// guarantee that both are executed if they are called within one loop
		// iteration of the worker thread
		WorkerOp op = reset_artwork ? OP_REMOVE_RESET : OP_REMOVE;
		worker_queue.emplace_back(std::make_pair(op, [=] { worker_removeArtwork(reset_artwork); }));
	}
	worker_cv.notify_all();
}

void Artwork::updateArtwork()
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(OP_UPDATE, [] { worker_updateArtwork(); }));
	}
	worker_cv.notify_all();
}

void Artwork::updateArtwork(std::string uri)
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(OP_UPDATE_URI, [=] { worker_updateArtwork(uri); }));
	}
	worker_cv.notify_all();
}

void Artwork::updatedVisibility()
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(OP_UPDATED_VIS, [=] { worker_updatedVisibility(); }));
	}
	worker_cv.notify_all();
}


// Worker thread methods

void Artwork::worker_drawArtwork(std::string path, int x_offset, int y_offset, int width, int height)
{
	backend->updateArtwork(path, x_offset, y_offset, width, height);
	drawn = true;
	before_inital_draw = false;
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

void Artwork::worker_updatedVisibility()
{
	if (!isVisible(myArtwork) && drawn)
	{
		worker_removeArtwork();
	}
	else if (isVisible(myArtwork) && !before_inital_draw && !drawn)
	{
		worker_updateArtwork();
	}
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
		query_time = std::chrono::steady_clock::now();
		if (!buffer.empty())
			return buffer;
	}

	return buffer;
}

void Artwork::worker()
{
	while (true)
	{
		std::deque<std::function<void()>> ops;
		{
			// Wait for work
			std::unique_lock<std::mutex> lck(worker_mtx);
			worker_cv.wait(lck, [] { return worker_exit || !worker_queue.empty(); });

			if (worker_exit)
			{
				worker_exit = false;
				return;
			}

			// Only run the latest queued operation for each type so we won't fall behind
			std::set<WorkerOp> found_set;
			for (auto it = worker_queue.rbegin(); it != worker_queue.rend(); ++it)
			{
				if (found_set.end() != found_set.find(it->first))
					continue;
				found_set.insert(it->first);
				ops.push_front(it->second);
			}
			worker_queue.clear();

			// If next work iteration requires MPD and we have previously queried
			// MPD, wait a bit to avoid overwhelming MPD
			if (found_set.end() != found_set.find(OP_UPDATE_URI))
			{
				if (worker_cv.wait_until(lck, query_time + std::chrono::milliseconds(500), [] { return worker_exit; }))
				{
					worker_exit = false;
					return;
				}
			}
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

			// Do the requested operations
			for (auto &op : ops)
			{
				op();
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
