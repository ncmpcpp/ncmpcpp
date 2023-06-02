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
#include <boost/thread/barrier.hpp>
#include <Magick++.h>

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
	~Artwork();

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
	void resetArtworkPosition();
	void updatedVisibility();

	enum struct ArtBackend { UEBERZUG, KITTY };
	enum struct ArtSource { LOCAL, MPD_ALBUMART, MPD_READPICTURE };
	enum struct ArtAlign { N, NE, E, SE, S, SW, W, NW, CENTER };

	// fd to signal main thread to print image
	int pipefd_read;

private:
	void stop();

	// worker thread methods
	void worker();
	void worker_updateArtBuffer(std::string path);
	void worker_drawArtwork(int x_offset, int y_offset, int width, int height);
	void worker_removeArtwork(bool reset_artwork = false);
	void worker_updateArtwork();
	void worker_updateArtwork(const std::string &uri);
	void worker_resetArtworkPosition();
	void worker_updatedVisibility();
	int worker_calcXOffset(int width, int img_width, int char_xpixel);
	int worker_calcYOffset(int height, int img_height, int char_ypixel);
	std::vector<uint8_t> worker_fetchArtwork(const std::string &uri, const std::string &cmd);
	std::string worker_fetchLocalArtwork(const std::string &uri);
	std::string worker_checkFileAccess(const std::string &path_glob);

	static winsize getWinSize();

	// worker thread variables
	std::thread t;
	std::vector<uint8_t> orig_art_buffer;
	Magick::Blob art_buffer;
	ArtworkBackend* backend;
	std::string prev_uri = "";
	bool drawn = false;
	bool before_inital_draw = true;
	const std::map<Artwork::ArtSource, std::string> art_source_cmd_map = {
		{ Artwork::ArtSource::MPD_ALBUMART, "albumart" },
		{ Artwork::ArtSource::MPD_READPICTURE, "readpicture" },
	};

	// For giving tasks to worker thread
	// Types of operations the worker thread can do
	enum struct WorkerOp {
		UPDATE,
		UPDATE_URI,
		MOVE,
		REMOVE,
		REMOVE_RESET,
		UPDATED_VIS,
	};
	std::condition_variable worker_cv;
	std::mutex worker_mtx;
	std::chrono::time_point<std::chrono::steady_clock> update_time;
	bool worker_exit = false;
	std::vector<std::pair<WorkerOp, std::function<void()>>> worker_queue;

	// For main thread signaling it has finished writing output to terminal
	std::mutex terminal_drawn_mtx;
	std::condition_variable terminal_drawn_cv;
	bool terminal_drawn = true;

	// store window dimensions
	typedef struct winsize_s {
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
	} winsize_t;
	winsize_t prev_winsize;
};

std::istream &operator>>(std::istream &is, Artwork::ArtSource &source);
std::istream &operator>>(std::istream &is, Artwork::ArtBackend &backend);
std::istream &operator>>(std::istream &is, Artwork::ArtAlign &align);

class ArtworkBackend
{
public:
	virtual ~ArtworkBackend() {}

	// draw artwork, path relative to mpd_music_dir, units in terminal characters
	virtual void updateArtwork(const Magick::Blob& buffer, int x_offset, int y_offset) = 0;
	virtual void resetArtworkPosition() {}

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
	~UeberzugBackend() override;
	virtual void updateArtwork(const Magick::Blob& buffer, int x_offset, int y_offset) override;
	virtual void removeArtwork() override;

private:
	void stop();
	boost::process::child process;
	boost::process::opstream stream;
	std::string temp_file_name;
};

class KittyBackend : public ArtworkBackend
{
// Kitty backend has an issue when in slave screen mode, the image scrolls
// when scrolling the other screen
public:
	KittyBackend(std::mutex &terminal_drawn_mtx, std::condition_variable &terminal_drawn_cv, bool &terminal_drawn, int fd)
		: pipefd_write(fd),
		terminal_drawn_mtx(terminal_drawn_mtx),
		terminal_drawn_cv(terminal_drawn_cv),
		terminal_drawn(terminal_drawn) {}
	virtual void updateArtwork(const Magick::Blob& buffer, int x_offset, int y_offset) override;
	virtual void resetArtworkPosition() override;
	virtual void removeArtwork() override;
	virtual std::tuple<std::vector<uint8_t>, int, int> takeOutput() override;

private:
	std::vector<uint8_t> serializeGrCmd(std::map<std::string, std::string> cmd,
			const std::vector<uint8_t> &payload, size_t chunk_begin,
			size_t chunk_end);
	std::vector<uint8_t> writeChunked(std::map<std::string, std::string> cmd,
			const Magick::Blob &data);
	virtual void setOutput(std::vector<uint8_t> buffer, int x_offset, int y_offset) override;

	std::vector<uint8_t> output;
	int output_x_offset;
	int output_y_offset;
	int pipefd_write;
	std::mutex &terminal_drawn_mtx;
	std::condition_variable &terminal_drawn_cv;
	bool &terminal_drawn;
};

extern Artwork *myArtwork;

#endif // ENABLE_ARTWORK

#endif // NCMPCPP_ARTWORK_H
