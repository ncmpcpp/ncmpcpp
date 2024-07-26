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
#include <glob.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "mpdpp.h"
#include "screens/screen_switcher.h"
#include "statusbar.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

Artwork* myArtwork;

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
		, cache(4) // keep last few recently rendered artworks in cache for faster rendering
		           // 4 is chosen mostly to speed up switching between split and full windows
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
			backend = new KittyBackend(terminal_drawn_mtx, terminal_drawn_cv, terminal_drawn, pipefd[1]);
			break;
	}

	// Spawn worker thread
	t = std::thread(&Artwork::worker, this);
}

Artwork::~Artwork()
{
	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_exit = true;
	}
	worker_cv.notify_all();
	t.join();
	delete backend;
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

	{
		// unblock worker thread so it can send the next command
		std::unique_lock<std::mutex> lck(terminal_drawn_mtx);
		terminal_drawn = true;
		terminal_drawn_cv.notify_all();
	}

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
		worker_queue.emplace_back(std::make_pair(WorkerOp::UPDATE, [=] { worker_updateArtwork(); }));
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

void Artwork::resetArtworkPosition()
{
	if (!Config.albumart)
		return;

	{
		std::lock_guard<std::mutex> lck(worker_mtx);
		worker_queue.emplace_back(std::make_pair(WorkerOp::MOVE, [=] { worker_resetArtworkPosition(); }));
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

void Artwork::worker_updateArtBuffer(const std::string &uri, const std::string &path)
{
	orig_art.uri = uri;
	try {
		std::ifstream s(path, std::ios::in | std::ios::binary);
		std::vector<uint8_t> tmp_buf((std::istreambuf_iterator<char>(s)), std::istreambuf_iterator<char>());
		orig_art.buffer = std::move(tmp_buf);
	} catch (const std::exception&) {
		orig_art.buffer.clear();
	}
}

void Artwork::worker_drawArtwork(int x_offset, int y_offset, int width, int height)
{
	using namespace Magick;

	if (orig_art.buffer.empty())
	{
		return;
	}

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
		cache_key_t k = { orig_art.uri, pixel_width, pixel_height, x_offset, y_offset };
		if (boost::optional<cache_value_t> v = cache.get(k))
		{
			// cache hit
			art_buffer = v->blob;
			x_offset = v->adj_x_offset;
			y_offset = v->adj_y_offset;
		}
		else
		{
			// cache miss
			// read and process artwork
			const Blob in_blob(orig_art.buffer.data(), orig_art.buffer.size() * sizeof(orig_art.buffer[0]));
			Image img(in_blob);
			img.resize(out_geom);

			// write output to buffer
			img.write(&art_buffer, "PNG");

			// center image
			x_offset += worker_calcXOffset(width, img.columns(), char_xpixel);
			x_offset += Config.albumart_xoffset;
			y_offset += worker_calcYOffset(height, img.rows(), char_ypixel);
			y_offset += Config.albumart_yoffset;

			// save in cache
			cache_value_t new_v = {
				art_buffer,
				x_offset,
				y_offset,
			};
			cache.insert(k, new_v);
		}
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
					std::string dir = uri.substr(0, uri.find_last_of('/')) + '/';
					std::string art_path;
					for (const auto &file : Config.albumart_filenames)
					{
						const std::string path_glob = Config.mpd_music_dir + dir + file;
						const std::string path = worker_checkFileAccess(path_glob);
						if (!path.empty())
						{
							art_path = path;
							break;
						}
					}
					if (art_path.empty())
					{
						continue;
					}

					// draw the image
					worker_updateArtBuffer(uri, art_path);
					worker_drawArtwork(x_offset, MainStartY, width, MainHeight);
					return;
				}
			case ArtSource::MPD_ALBUMART:
			case ArtSource::MPD_READPICTURE:
				{
					const std::string &cmd = art_source_cmd_map.at(source);
					orig_art = {
						uri,
						worker_fetchArtwork(uri, cmd),
					};

					if (orig_art.buffer.empty())
						continue;

					// Write artwork to a temporary file and draw
					prev_uri = uri;
					worker_drawArtwork(x_offset, MainStartY, width, MainHeight);
					return;
				}
		}
	}

	// Draw default artwork if MPD doesn't return anything
	worker_updateArtBuffer(uri, Config.albumart_default_path);
	worker_drawArtwork(x_offset, MainStartY, width, MainHeight);
}

// Expand glob and return first file that exists, otherwise return empty string
std::string Artwork::worker_checkFileAccess(const std::string &path_glob)
{
	glob_t globbuf;
	glob(path_glob.c_str(), 0, nullptr, &globbuf);

	std::string path;
	for (size_t i = 0; i < globbuf.gl_pathc; ++i)
	{
		std::string tmp_path = globbuf.gl_pathv[i];
		if (0 == access(tmp_path.c_str(), R_OK))
		{
			path = tmp_path;
			break;
		}
	}

	globfree(&globbuf);
	return path;
}

void Artwork::worker_resetArtworkPosition()
{
	if (drawn)
		backend->resetArtworkPosition();
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
			worker_cv.wait(lck, [=] { return worker_exit || !worker_queue.empty(); });

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
				if (worker_cv.wait_until(lck, update_time + std::chrono::milliseconds(500), [=] { return worker_exit; }))
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

UeberzugBackend::UeberzugBackend()
{
	namespace bp = boost::process;
	namespace fs = boost::filesystem;

	temp_file_name = (fs::temp_directory_path() / fs::unique_path()).string();

	std::vector<std::string> args = {"layer", "--silent", "--parser", "json"};
	try
	{
		process = bp::child(bp::search_path("ueberzug"), bp::args(args), bp::std_in < stream);
	}
	catch (const bp::process_error &e) {}
}

UeberzugBackend::~UeberzugBackend()
{
	std::remove(temp_file_name.c_str());
	// close the underlying pipe, which causes ueberzug to exit
	stream.pipe().close();
	process.wait();
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


// KittyBackend
// https://sw.kovidgoyal.net/kitty/graphics-protocol.html

void KittyBackend::updateArtwork(const Magick::Blob& buffer, int x_offset, int y_offset)
{
	std::map<std::string, std::string> cmd;
	cmd["a"] = "T";   // transmit and display
	cmd["f"] = "100"; // PNG
	cmd["i"] = "1";   // image ID
	cmd["p"] = "1";   // placement ID
	cmd["q"] = "2";   // suppress output
	cmd["C"] = "1";   // don't move cursor after placing image

	setOutput(writeChunked(cmd, buffer), x_offset, y_offset);
}

void KittyBackend::resetArtworkPosition()
{
	std::map<std::string, std::string> cmd;
	cmd["a"] = "p";   // put
	cmd["i"] = "1";   // image ID
	cmd["p"] = "1";   // placement ID
	cmd["q"] = "2";   // suppress output
	cmd["C"] = "1";   // don't move cursor after placing image

	setOutput(writeChunked(cmd, {}), output_x_offset, output_y_offset);
}

void KittyBackend::removeArtwork()
{
	std::map<std::string, std::string> cmd;
	cmd["a"] = "d";   // delete
	cmd["i"] = "1";   // image ID
	cmd["q"] = "2";   // suppress output

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
		// Create reference-counted copy to bypass IM6 or earlier Blob::base64() being non-const
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
	std::tuple<std::vector<uint8_t>, int, int> ret;
	ret = {output, output_x_offset, output_y_offset};
	output.clear();
	return ret;
}

void KittyBackend::setOutput(std::vector<uint8_t> buffer, int x_offset, int y_offset)
{
	{
		// wait until main thread has written previous command
		std::unique_lock<std::mutex> lck(terminal_drawn_mtx);
		terminal_drawn_cv.wait(lck, [=] { return terminal_drawn; });
		terminal_drawn = false;
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
