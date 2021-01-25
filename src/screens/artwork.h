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

	static void removeArtwork(bool reset_artwork = false);
	static void updateArtwork();
	static void updateArtwork(std::string uri);

	static bool drawn;

private:
	static void stop();

	static void worker();
	static void worker_drawArtwork(std::string path, int x_offset, int y_offset, int width, int height);
	static void worker_removeArtwork(bool reset_artwork = false);
	static void worker_updateArtwork();
	static void worker_updateArtwork(const std::string &uri);
	static std::vector<uint8_t> worker_fetchArtwork(const std::string &uri);

	static std::string temp_file_name;
	static std::ofstream temp_file;
	static std::thread t;
};

class ArtworkBackend
{
public:
	virtual void init() { }

	// draw artwork, path relative to mpd_music_dir, units in terminal characters
	virtual void updateArtwork(std::string path, int x_offset, int y_offset, int width, int height) = 0;

	// clear artwork from screen
	virtual void removeArtwork() = 0;
};

class UeberzugBackend : public ArtworkBackend
{
public:
	virtual void init() override;
	virtual void updateArtwork(std::string path, int x_offset, int y_offset, int width, int height) override;
	virtual void removeArtwork() override;

private:
	static void stop();
	static boost::process::child process;
	static boost::process::opstream stream;
};

extern Artwork *myArtwork;

#endif // ENABLE_ARTWORK

#endif // NCMPCPP_ARTWORK_H
