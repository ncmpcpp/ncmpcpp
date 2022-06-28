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
#include "mpdpp.h"
#include "screens/screen_switcher.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

Artwork* myArtwork;

Magick::Blob Artwork::art_buffer;
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

namespace {
	std::mutex worker_output_mtx;
}

std::istream &operator>>(std::istream &is, Artwork::ArtSource &source)
{
	std::string s;
	is >> s;
	boost::algorithm::to_lower(s);
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
	boost::algorithm::to_lower(s);
	if (s == "ueberzug")
		backend = Artwork::ArtBackend::UEBERZUG;
	else if (s == "kitty")
		backend = Artwork::ArtBackend::KITTY;
	else
		is.setstate(std::ios::failbit);
	return is;
}

std::istream &operator>>(std::istream &is, Artwork::ArtAlign &align)
{
	std::string s;
	is >> s;
	boost::algorithm::to_lower(s);
	if (s == "n")
		align = Artwork::ArtAlign::N;
	else if (s == "ne")
		align = Artwork::ArtAlign::NE;
	else if (s == "e")
		align = Artwork::ArtAlign::E;
	else if (s == "se")
		align = Artwork::ArtAlign::SE;
	else if (s == "s")
		align = Artwork::ArtAlign::S;
	else if (s == "sw")
		align = Artwork::ArtAlign::SW;
	else if (s == "w")
		align = Artwork::ArtAlign::W;
	else if (s == "nw")
		align = Artwork::ArtAlign::NW;
	else if (s == "center")
		align = Artwork::ArtAlign::CENTER;
	return is;
}

Artwork::Artwork()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border()))
, prev_winsize({})
{
	if (!Config.albumart)
		return;

	// ncurses doesn't play well with writing concurrently, set up a self-pipe
	// so the worker thread can signal the main select() loop to print
	int pipefd[2];
	if (-1 == pipe(pipefd)) {
		throw MPD::Error("Can't create to artwork pipe", false);
	}
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
	if (!Config.albumart)
		return;

	myArtwork->drawToScreen();
}

void Artwork::drawToScreen()
{
	if (!Config.albumart)
		return;

	// consume data in self-pipe
	const size_t BUF_SIZE = 100;
	char buf[BUF_SIZE];
	if (-1 == read(pipefd_read, buf, BUF_SIZE)) {
		throw MPD::Error("Can't read from artwork pipe", false);
	}

	std::vector<uint8_t> output;
	int x_offset, y_offset;
	std::tie(output, x_offset, y_offset) = backend->takeOutput();
	if (output.empty())
	{
		return;
	}

	// save current cursor position
	std::cout << "\0337";
	// move cursor, terminal coordinates are 1-indexed
	std::cout << boost::format("\033[%1%;%2%H") % (y_offset+1) % (x_offset+1);
	// write image data
	std::copy(output.begin(), output.end(), std::ostreambuf_iterator<char>(std::cout));
	// restore cursor position
	std::cout << "\0338";
	std::cout.flush();

	// redraw active window
	Global::myScreen->activeWindow()->display();
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
	if (!Config.albumart)
		return;

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
	if (!Config.albumart)
		return;

	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(WorkerOp::UPDATE, [] { worker_updateArtwork(); }));
	}
	worker_cv.notify_all();
}

void Artwork::updateArtwork(std::string uri)
{
	if (!Config.albumart)
		return;

	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(WorkerOp::UPDATE_URI, [=] { worker_updateArtwork(uri); }));
	}
	worker_cv.notify_all();
}

void Artwork::updatedVisibility()
{
	if (!Config.albumart)
		return;

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

	// retrieve and calculate terminal character size
	const auto sz = getWinSize();
	const auto char_xpixel = Config.font_width == 0
			? sz.ws_xpixel / sz.ws_col
			: Config.font_width;
	const auto char_ypixel = Config.font_height == 0
			? sz.ws_ypixel / sz.ws_row
			: Config.font_height;
	const auto pixel_width = char_xpixel * width;
	const auto pixel_height = char_ypixel * height;
	const auto out_geom = Geometry(pixel_width, pixel_height);
	if (char_xpixel == 0 || char_ypixel == 0)
	{
		std::cerr << "Couldn't detect font size, set font_width/font_height" << std::endl;
		return;
	}

	try
	{
		// read and process artwork
		const Blob in_blob(orig_art_buffer.data(), orig_art_buffer.size() * sizeof(orig_art_buffer[0]));
		Image img(in_blob);
		img.resize(out_geom);

		// write output to buffer
		img.write(&art_buffer, "PNG");

		// center image
		x_offset += worker_calcXOffset(width, img.columns(), char_xpixel);
		x_offset += Config.albumart_xoffset;
		y_offset += worker_calcYOffset(height, img.rows(), char_ypixel);
		y_offset += Config.albumart_yoffset;
	}
	catch (Magick::Exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	backend->updateArtwork(art_buffer, x_offset, y_offset);
	drawn = true;
	before_inital_draw = false;
}

int Artwork::worker_calcXOffset(int width, int img_width, int char_xpixel)
{
	switch (Config.albumart_align)
	{
		case ArtAlign::SW:
		case ArtAlign::W:
		case ArtAlign::NW:
			return 0;
		case ArtAlign::N:
		case ArtAlign::S:
		case ArtAlign::CENTER:
			return (width - (static_cast<double>(img_width) / char_xpixel)) / 2;
		case ArtAlign::NE:
		case ArtAlign::E:
		case ArtAlign::SE:
			return width - (static_cast<double>(img_width) / char_xpixel);
	}
	return 0;
}

int Artwork::worker_calcYOffset(int height, int img_height, int char_ypixel)
{
	switch (Config.albumart_align)
	{
		case ArtAlign::N:
		case ArtAlign::NE:
		case ArtAlign::NW:
			return 0;
		case ArtAlign::E:
		case ArtAlign::W:
		case ArtAlign::CENTER:
			return (height - (static_cast<double>(img_height) / char_ypixel)) / 2;
		case ArtAlign::SE:
		case ArtAlign::S:
		case ArtAlign::SW:
			return height - (static_cast<double>(img_height) / char_ypixel);
	}
	return 0;
}

void Artwork::worker_removeArtwork(bool reset_artwork)
{
	backend->removeArtwork();
	drawn = false;
	if (reset_artwork)
	{
		art_buffer = Magick::Blob();
	}
}

void Artwork::worker_updateArtwork()
{
	if (0 == art_buffer.length())
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
					const auto it = std::find_if(
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

					if (orig_art_buffer.empty())
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
	return Mpd_artwork.GetArtwork(uri, cmd);
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
// https://github.com/seebye/ueberzug

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

void UeberzugBackend::updateArtwork(const Magick::Blob& buffer, int x_offset, int y_offset)
{
	if (!process.running())
		return;

	std::ofstream temp_file(temp_file_name, std::ios::trunc | std::ios::binary);
	temp_file.write(reinterpret_cast<const char *>(buffer.data()), buffer.length());
	temp_file.close();

	boost::property_tree::ptree pt;
	pt.put("action", "add");
	pt.put("identifier", "albumart");
	pt.put("synchronously_draw", true);
	pt.put("path", temp_file_name);
	pt.put("x", x_offset);
	pt.put("y", y_offset);

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
// https://sw.kovidgoyal.net/kitty/graphics-protocol.html

void KittyBackend::updateArtwork(const Magick::Blob& buffer, int x_offset, int y_offset)
{
	std::map<std::string, std::string> cmd;
	cmd["a"] = "T";   // transfer and display
	cmd["f"] = "100"; // PNG
	cmd["i"] = "1";   // image ID
	cmd["p"] = "1";   // placement ID
	cmd["q"] = "1";   // suppress output

	setOutput(writeChunked(cmd, buffer), x_offset, y_offset);
}

void KittyBackend::removeArtwork()
{
	std::map<std::string, std::string> cmd;
	cmd["a"] = "d";   // delete
	cmd["i"] = "1";   // image ID
	cmd["q"] = "1";   // suppress output

	setOutput(writeChunked(cmd, {}), 0, 0);
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

std::vector<uint8_t> KittyBackend::writeChunked(std::map<std::string, std::string> cmd, const Magick::Blob& data)
{
	using namespace Magick;

	std::vector<uint8_t> output;

	if (0 != data.length())
	{
		// Create reference-counted copy to bypasee IM6 or earlier Blob::base64() being non-const
		Blob data_copy(data);
		std::string b64_data_str = data_copy.base64();
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
	const auto final_chunk = serializeGrCmd(cmd, {}, 0, 0);
	output.insert(output.end(), final_chunk.begin(), final_chunk.end());
	return output;
}

std::tuple<std::vector<uint8_t>, int, int> KittyBackend::takeOutput()
{
	std::lock_guard<std::mutex> lck(worker_output_mtx);
	std::tuple<std::vector<uint8_t>, int, int> ret;
	ret = {output, output_x_offset, output_y_offset};
	output.clear();
	return ret;
}

void KittyBackend::setOutput(std::vector<uint8_t> buffer, int x_offset, int y_offset)
{
	{
		std::lock_guard<std::mutex> lck(worker_output_mtx);
		output = buffer;
		output_x_offset = x_offset;
		output_y_offset = y_offset;
	}

	// write dummy character to self-pipe, wakes up main thread to write output
	const char dummy = 'x';
	if (-1 == write(pipefd_write, &dummy, 1)) {
		throw MPD::Error("Can't write to artwork pipe", false);
	}
}

#endif // ENABLE_ARTWORK
