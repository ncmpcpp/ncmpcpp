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

#ifndef NCMPCPP_ARTWORK_H
#define NCMPCPP_ARTWORK_H

#include "config.h"

#ifdef ENABLE_ARTWORK

#include <boost/process.hpp>

#include "curses/window.h"
#include "interfaces.h"
#include "screens/screen.h"

class ArtworkBackend;

namespace ArtworkHelper {
	void drawToScreen();
}

struct Artwork: Screen<NC::Window>, Tabbable
{
	Artwork();

	virtual void resize() override;
	virtual void switchTo() override;

	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::Artwork; }

	virtual void update() override { }
	virtual void scroll(NC::Scroll) override { }

	virtual void mouseButtonPressed(MEVENT) override { }

	virtual bool isLockable() override { return true; }
	virtual bool isMergable() override { return true; }

	void drawToScreen();

	void removeArtwork(bool reset_artwork = false);
	void updateArtwork();
	void updateArtwork(std::string uri);
	void updatedVisibility();

	static winsize getWinSize();

	enum struct ArtBackend { UEBERZUG, KITTY };
	enum struct ArtSource { LOCAL, MPD_ALBUMART, MPD_READPICTURE };
	enum struct ArtAlign { N, NE, E, SE, S, SW, W, NW, CENTER };

	int pipefd_read;

private:
	static void stop();

	// worker thread methods
	static void worker();
	static void worker_updateArtBuffer(std::string path);
	static void worker_drawArtwork(int x_offset, int y_offset, int width, int height);
	static void worker_removeArtwork(bool reset_artwork = false);
	static void worker_updateArtwork();
	static void worker_updateArtwork(const std::string &uri);
	static void worker_updatedVisibility();
	static int worker_calcXOffset(int width, int img_width, int char_xpixel);
	static int worker_calcYOffset(int height, int img_height, int char_ypixel);
	static std::vector<uint8_t> worker_fetchArtwork(const std::string &uri, const std::string &cmd);
	static std::string worker_fetchLocalArtwork(const std::string &uri);

	static std::thread t;  // worker thread
	static std::vector<uint8_t> art_buffer;
	static std::vector<uint8_t> orig_art_buffer;
	static ArtworkBackend *backend;
	static std::string prev_uri;
	static bool drawn;
	static bool before_inital_draw;

	// store window dimensions
	struct winsize_s {
		size_t x_offset;
		size_t y_offset;
		size_t width;
		size_t height;

		bool operator==(const winsize_s& rhs)
		{
			return x_offset == rhs.x_offset &&
				y_offset == rhs.y_offset &&
				width == rhs.width &&
				height == rhs.height;
		}

		bool operator!=(const winsize_s& rhs)
		{
			return !(*this == rhs);
		}
	};
	typedef winsize_s winsize_t;
	winsize_t prev_winsize;

	const static std::map<ArtSource, std::string> art_source_cmd_map;

	// For signaling worker thread
	static std::condition_variable worker_cv;
	static std::mutex worker_mtx;

	// last time worker thread updated artwork
	static std::chrono::time_point<std::chrono::steady_clock> update_time;

	// worker thread should exit
	static bool worker_exit;

	// Types of operations the worker thread can do. Worker thread will only
	// run the latest of each operation per iteration
	enum struct WorkerOp {
		UPDATE,
		UPDATE_URI,
		REMOVE,
		REMOVE_RESET,
		UPDATED_VIS,
	};

	static std::vector<std::pair<WorkerOp, std::function<void()>>> worker_queue;
};

std::istream &operator>>(std::istream &is, Artwork::ArtSource &source);
std::istream &operator>>(std::istream &is, Artwork::ArtBackend &backend);
std::istream &operator>>(std::istream &is, Artwork::ArtAlign &align);

class ArtworkBackend
{
public:
	// draw artwork, path relative to mpd_music_dir, units in terminal characters
	virtual void updateArtwork(const std::vector<uint8_t>& buffer, int x_offset, int y_offset) = 0;

	// clear artwork from screen
	virtual void removeArtwork() = 0;

	virtual void setOutput(std::vector<uint8_t> buffer, int x_offset, int y_offset) {}
	// get output data, returns (output, x_offset, y_offset)
	virtual std::tuple<std::vector<uint8_t>, int, int> takeOutput() { return {{}, 0, 0}; }
};

class UeberzugBackend : public ArtworkBackend
{
public:
	UeberzugBackend();
	virtual void updateArtwork(const std::vector<uint8_t>& buffer, int x_offset, int y_offset) override;
	virtual void removeArtwork() override;

private:
	static void stop();
	static boost::process::child process;
	static boost::process::opstream stream;
	static std::string temp_file_name;
};

class KittyBackend : public ArtworkBackend
{
	// Kitty backend has an issue when in slave screen mode, the image scrolls
	// when scrolling the other screen
public:
	KittyBackend(int fd) : pipefd_write(fd) {}
	virtual void updateArtwork(const std::vector<uint8_t>& buffer, int x_offset, int y_offset) override;
	virtual void removeArtwork() override;
	virtual std::tuple<std::vector<uint8_t>, int, int> takeOutput() override;

private:
	std::vector<uint8_t> serializeGrCmd(std::map<std::string, std::string> cmd,
			const std::vector<uint8_t> &payload, size_t chunk_begin,
			size_t chunk_end);
	std::vector<uint8_t> writeChunked(std::map<std::string, std::string> cmd,
			const std::vector<uint8_t> &data);
	virtual void setOutput(std::vector<uint8_t> buffer, int x_offset, int y_offset) override;

	std::vector<uint8_t> output;
	int output_x_offset;
	int output_y_offset;
	int pipefd_write;
};

extern Artwork *myArtwork;

#endif // ENABLE_ARTWORK

#endif // NCMPCPP_ARTWORK_H
