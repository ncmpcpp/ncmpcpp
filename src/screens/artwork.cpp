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

#include <Magick++.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <sstream>

#include "macro_utilities.h"
#include "screens/screen_switcher.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

Artwork* myArtwork;

std::vector<uint8_t> Artwork::art_buffer;
std::vector<uint8_t> Artwork::orig_art_buffer;
std::thread Artwork::t;
ArtworkBackend* Artwork::backend;
std::string Artwork::prev_uri = "";
bool Artwork::drawn = false;
bool Artwork::before_inital_draw = true;
const std::map<Artwork::ArtSource, std::string> Artwork::art_source_cmd_map = {
	{ ArtSource::MPD_ALBUMART, "albumart" },
	{ ArtSource::MPD_READPICTURE, "readpicture" },
};

// For signaling worker thread
std::condition_variable Artwork::worker_cv;
std::mutex Artwork::worker_mtx;
std::chrono::time_point<std::chrono::steady_clock> Artwork::update_time;
bool Artwork::worker_exit = false;
std::vector<std::pair<Artwork::WorkerOp, std::function<void()>>> Artwork::worker_queue;
int Artwork::pipefd_read;
Artwork::winsize_t Artwork::prev_winsize;

namespace {
	std::mutex worker_output_mtx;
}

std::istream &operator>>(std::istream &is, Artwork::ArtSource &source)
{
	std::string s;
	is >> s;
	if (s == "local")
		source = Artwork::ArtSource::LOCAL;
	else if (s == "mpd_albumart")
		source = Artwork::ArtSource::MPD_ALBUMART;
	else if (s == "mpd_readpicture")
		source = Artwork::ArtSource::MPD_READPICTURE;
	else
		is.setstate(std::ios::failbit);
	return is;
}

std::istream &operator>>(std::istream &is, Artwork::ArtBackend &backend)
{
	std::string s;
	is >> s;
	if (s == "ueberzug")
		backend = Artwork::ArtBackend::UEBERZUG;
	else if (s == "kitty")
		backend = Artwork::ArtBackend::KITTY;
	else
		is.setstate(std::ios::failbit);
	return is;
}

Artwork::Artwork()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border()))
{
	// ncurses doesn't play well with writing concurrently, set up a self-pipe
	// so the worker thread can signal the main select() loop to print
	int pipefd[2];
	pipe(pipefd);
	pipefd_read = pipefd[0];

	// Initialize the selected albumart backend
	switch (Config.albumart_backend)
	{
		case ArtBackend::UEBERZUG:
			backend = new UeberzugBackend;
			break;
		case ArtBackend::KITTY:
			backend = new KittyBackend(pipefd[1]);
			break;
	}

	// Spawn worker thread
	t = std::thread(worker);
	std::atexit(stop);
}

// callback for the main select() loop, draws artwork on screen
void ArtworkHelper::drawToScreen()
{
	myArtwork->drawToScreen();
}

void Artwork::drawToScreen()
{
	// consume data in self-pipe
	const size_t BUF_SIZE = 100;
	char buf[BUF_SIZE];
	read(pipefd_read, buf, BUF_SIZE);

	auto output = backend->getOutput();
	if (output.empty())
	{
		return;
	}

	size_t beg_y, beg_x;
	getbegyx(w.raw(), beg_y, beg_x);

	// save current cursor position
	std::cout << "\0337";
	// move cursor
	std::cout << boost::format("\033[%1%;%2%H") % beg_y % beg_x;
	// write image data
	std::copy(output.begin(), output.end(), std::ostreambuf_iterator<char>(std::cout));
	// restore cursor position
	std::cout << "\0338";
	std::cout.flush();
}

void Artwork::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	w.clear();
	refreshWindow();
	hasToBeResized = 0;

	winsize_t cur_winsize =  {
		x_offset,
		MainStartY,
		width,
		MainHeight
	};

	if (cur_winsize != prev_winsize)
	{
		updateArtwork();
	}

	prev_winsize = cur_winsize;
}

void Artwork::switchTo()
{
	SwitchTo::execute(this);
	drawHeader();
}

std::wstring Artwork::title() { return L"Artwork"; }

winsize Artwork::getWinSize()
{
	struct winsize sz;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &sz);
	return sz;
}

void Artwork::stop()
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_exit = true;
	}
	worker_cv.notify_all();
	t.join();
}

void Artwork::removeArtwork(bool reset_artwork)
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		// Need to specify distinct operations for reset_artwork true/false to
		// guarantee that both are executed if they are called within one loop
		// iteration of the worker thread
		WorkerOp op = reset_artwork ? WorkerOp::REMOVE_RESET : WorkerOp::REMOVE;
		worker_queue.emplace_back(std::make_pair(op, [=] { worker_removeArtwork(reset_artwork); }));
	}
	worker_cv.notify_all();
}

void Artwork::updateArtwork()
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(WorkerOp::UPDATE, [] { worker_updateArtwork(); }));
	}
	worker_cv.notify_all();
}

void Artwork::updateArtwork(std::string uri)
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(WorkerOp::UPDATE_URI, [=] { worker_updateArtwork(uri); }));
	}
	worker_cv.notify_all();
}

void Artwork::updatedVisibility()
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(WorkerOp::UPDATED_VIS, [=] { worker_updatedVisibility(); }));
	}
	worker_cv.notify_all();
}


// Worker thread methods

void Artwork::worker_updateArtBuffer(std::string path)
{
	std::ifstream s(path, std::ios::in | std::ios::binary);
	std::vector<uint8_t> tmp_buf((std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());
	orig_art_buffer = std::move(tmp_buf);
}

void Artwork::worker_drawArtwork(int x_offset, int y_offset, int width, int height)
{
	using namespace Magick;

	if (backend->postprocess())
	{
		// retrieve and calculate terminal character size
		auto sz = getWinSize();
		auto char_xpixel = sz.ws_xpixel / sz.ws_col;
		auto char_ypixel = sz.ws_ypixel / sz.ws_row;
		auto pixel_width = char_xpixel * width;
		auto pixel_height = char_ypixel * height;

		auto out_geom = Geometry(pixel_width, pixel_height);
		Image out_img(out_geom, "transparent");
		Image in_img;
		try
		{
			// read un-processed artwork
			Blob in_blob(orig_art_buffer.data(), orig_art_buffer.size() * sizeof(orig_art_buffer[0]));
			in_img.read(in_blob);

			// composite artwork onto output image
			in_img.scale(out_geom);
			out_img.composite(in_img, CenterGravity, OverCompositeOp);
			out_img.magick("PNG");
			out_img.depth(8);

			// write output to buffer
			Blob out_blob;
			out_img.write(&out_blob);
			art_buffer.resize(out_blob.length());
			std::memcpy(art_buffer.data(), out_blob.data(), out_blob.length());
		}
		catch (Magick::Exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
	else
	{
		art_buffer = orig_art_buffer;
	}

	backend->updateArtwork(art_buffer, x_offset, y_offset, width, height);
	drawn = true;
	before_inital_draw = false;
}

void Artwork::worker_removeArtwork(bool reset_artwork)
{
	backend->removeArtwork();
	drawn = false;
	if (reset_artwork)
	{
		art_buffer.clear();
	}
}

void Artwork::worker_updateArtwork()
{
	if (art_buffer.empty())
		return;

	size_t x_offset, width;
	myArtwork->getWindowResizeParams(x_offset, width, false);
	worker_drawArtwork(x_offset, MainStartY, width, MainHeight);
}

void Artwork::worker_updateArtwork(const std::string &uri)
{
	update_time = std::chrono::steady_clock::now();
	size_t x_offset, width;
	myArtwork->getWindowResizeParams(x_offset, width, false);

	// Draw same file if uri is unchanged
	if (prev_uri == uri)
	{
		worker_drawArtwork(x_offset, MainStartY, width, MainHeight);
		return;
	}

	for (const auto &source : Config.albumart_sources)
	{
		switch (source)
		{
			case ArtSource::LOCAL:
				{
					// find the first suitable image to draw
					std::string dir = uri.substr(0, uri.find_last_of('/'));
					std::vector<std::string> candidate_paths = {
						Config.mpd_music_dir + dir + "/cover.png",
						Config.mpd_music_dir + dir + "/cover.jpg",
						Config.mpd_music_dir + dir + "/cover.tiff",
						Config.mpd_music_dir + dir + "/cover.bmp",
					};
					auto it = std::find_if(
							candidate_paths.begin(), candidate_paths.end(),
							[](const std::string &s) { return 0 == access(s.c_str(), R_OK); });
					if (it == candidate_paths.end())
						continue;


					// draw the image
					worker_updateArtBuffer(*it);
					worker_drawArtwork(x_offset, MainStartY, width, MainHeight);
					return;
				}
			case ArtSource::MPD_ALBUMART:
			case ArtSource::MPD_READPICTURE:
				{
					const std::string &cmd = art_source_cmd_map.at(source);
					orig_art_buffer = worker_fetchArtwork(uri, cmd);

					if (art_buffer.empty())
						continue;

					// Write artwork to a temporary file and draw
					prev_uri = uri;
					worker_drawArtwork(x_offset, MainStartY, width, MainHeight);
					return;
				}
		}
	}

	// Draw default artwork if MPD doesn't return anything
	worker_updateArtBuffer(Config.albumart_default_path);
	worker_drawArtwork(x_offset, MainStartY, width, MainHeight);
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

std::vector<uint8_t> Artwork::worker_fetchArtwork(const std::string &uri, const std::string &cmd)
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
	std::vector<uint8_t> buffer;
	buffer = Mpd_artwork.GetArtwork(uri, cmd);
	if (!buffer.empty())
		return buffer;

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

			// Avoid updating artwork too often
			if (found_set.end() != found_set.find(WorkerOp::UPDATE_URI))
			{
				if (worker_cv.wait_until(lck, update_time + std::chrono::milliseconds(500), [] { return worker_exit; }))
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
std::string UeberzugBackend::temp_file_name;

UeberzugBackend::UeberzugBackend()
{
	namespace bp = boost::process;
	namespace fs = boost::filesystem;

	temp_file_name = (fs::temp_directory_path() / fs::unique_path()).string();

	std::vector<std::string> args = {"layer", "--silent", "--parser", "json"};
	try
	{
		process = bp::child(bp::search_path("ueberzug"), bp::args(args), bp::std_in < stream);
		std::atexit(stop);
	}
	catch (const bp::process_error &e) {}
}

bool UeberzugBackend::postprocess() { return false; }

void UeberzugBackend::updateArtwork(const std::vector<uint8_t>& buffer, int x_offset, int y_offset, int width, int height)
{
	if (!process.running())
		return;

	std::ofstream temp_file(temp_file_name, std::ios::trunc | std::ios::binary);
	temp_file.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
	temp_file.close();

	boost::property_tree::ptree pt;
	pt.put("action", "add");
	pt.put("identifier", "albumart");
	pt.put("synchronously_draw", true);
	pt.put("scaling_position_x", 0.5);
	pt.put("scaling_position_y", 0.5);
	pt.put("scaler", Config.albumart_scaler);
	pt.put("path", temp_file_name);
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
	std::remove(temp_file_name.c_str());
	// close the underlying pipe, which causes ueberzug to exit
	stream.pipe().close();
	process.wait();
}


// KittyBackend

bool KittyBackend::postprocess() { return true; }

void KittyBackend::updateArtwork(const std::vector<uint8_t>& buffer, int x_offset, int y_offset, int width, int height)
{
	const auto sz = Artwork::getWinSize();
	const auto pixel_width = sz.ws_xpixel / sz.ws_col;
	const auto pixel_height = sz.ws_ypixel / sz.ws_row;

	std::map<std::string, std::string> cmd;
	cmd["a"] = "T";   // transfer and display
	cmd["f"] = "100"; // PNG
	cmd["i"] = "1";   // image ID
	cmd["p"] = "1";   // placement ID
	cmd["q"] = "1";   // suppress ouput

	writeChunked(cmd, buffer);
}

void KittyBackend::removeArtwork()
{
	std::map<std::string, std::string> cmd;
	cmd["a"] = "d";   // delete
	cmd["i"] = "1";   // image ID
	cmd["q"] = "1";   // suppress ouput

	writeChunked(cmd, {});
}

std::vector<uint8_t> KittyBackend::serializeGrCmd(std::map<std::string, std::string> cmd,
		const std::vector<uint8_t> &payload,
		size_t chunk_begin, size_t chunk_end)
{
	std::string cmd_str;
	for (auto const &it : cmd)
	{
		cmd_str += it.first + "=" + it.second + ",";
	}
	if (!cmd_str.empty())
	{
		cmd_str.resize(cmd_str.size() - 1);
	}

	std::vector<uint8_t> ret;
	const std::string prefix = "\033_G" + cmd_str;
	const std::string suffix = "\033\\";

	ret.insert(ret.end(), prefix.begin(), prefix.end());
	if (chunk_begin != chunk_end) {
		ret.emplace_back(';');
		const auto it_beg = payload.begin() + chunk_begin;
		const auto it_end = chunk_end > payload.size() ? payload.end() : payload.begin() + chunk_end;
		ret.insert(ret.end(), it_beg, it_end);
	}
	ret.insert(ret.end(), suffix.begin(), suffix.end());
	return ret;
}

void KittyBackend::writeChunked(std::map<std::string, std::string> cmd, const std::vector<uint8_t>& data)
{
	using namespace Magick;

	std::vector<uint8_t> output;

	if (!data.empty())
	{
		Blob blob(data.data(), data.size() * sizeof(data[0]));
		std::string b64_data_str = blob.base64();
		std::vector<uint8_t> b64_data(b64_data_str.begin(), b64_data_str.end());

		const size_t CHUNK_SIZE = 4096;
		for (size_t i = 0; i < b64_data.size(); i += CHUNK_SIZE)
		{
			cmd["m"] = "1";
			auto chunk = serializeGrCmd(cmd, b64_data, i, i+CHUNK_SIZE);
			output.insert(output.end(), chunk.begin(), chunk.end());
		}
	}
	cmd["m"] = "0";
	auto final_chunk = serializeGrCmd(cmd, {}, 0, 0);
	output.insert(output.end(), final_chunk.begin(), final_chunk.end());

	setOutput(output);
}

std::vector<uint8_t> KittyBackend::getOutput()
{
	std::lock_guard<std::mutex> lck(worker_output_mtx);
	auto temp = output;
	output.clear();
	return temp;
}

void KittyBackend::setOutput(std::vector<uint8_t> buffer)
{
	{
		std::lock_guard<std::mutex> lck(worker_output_mtx);
		output = buffer;
	}
	char dummy[2] = "x";
	write(pipefd_write, dummy, 1);
}

#endif // ENABLE_ARTWORK
