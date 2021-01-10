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

#include <boost/json.hpp>
#include <sstream>

#include "macro_utilities.h"
#include "screens/screen_switcher.h"
#include "statusbar.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

Artwork* myArtwork;
ArtworkBackend* backend;
bool Artwork::drawn = false;
static std::string current_artwork_dir = "";

Artwork::Artwork()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border()))
{
	backend = new UeberzugBackend;
	backend->init();
}

void Artwork::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	updateArtwork(current_artwork_dir);
}

void Artwork::switchTo()
{
	SwitchTo::execute(this);
	drawHeader();
}

std::wstring Artwork::title() { return L"Artwork"; }

void Artwork::drawArtwork(std::string path, int x_offset, int y_offset, int width, int height)
{
	backend->updateArtwork(path, x_offset, y_offset, width, height);
	drawn = true;
}

void Artwork::removeArtwork(bool reset_artwork)
{
	backend->removeArtwork();
	drawn = false;
	if (reset_artwork)
	{
		current_artwork_dir = "";
	}
}

void Artwork::updateArtwork()
{
	updateArtwork(current_artwork_dir);
}

void Artwork::updateArtwork(std::string dir)
{
	if (dir == "")
		return;

	current_artwork_dir = dir;

	// only draw artwork if artwork screen is visible
	if (!isVisible(myArtwork))
		return;

	// find the first suitable image to draw
	std::vector<std::string> candidate_paths = {
		Config.mpd_music_dir + dir + "/cover.png",
		Config.mpd_music_dir + dir + "/cover.jpg",
		Config.mpd_music_dir + dir + "/cover.tiff",
		Config.mpd_music_dir + dir + "/cover.bmp",
		Config.albumart_default_path,
	};
	auto it = std::find_if(
			candidate_paths.begin(), candidate_paths.end(),
			[](const std::string &s) { return 0 == access(s.c_str(), R_OK); });
	if (it == candidate_paths.end())
		return;

	// draw the image
	size_t x_offset, width;
	myArtwork->getWindowResizeParams(x_offset, width);
	drawArtwork(*it, x_offset, MainStartY, width, MainHeight);
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

	boost::json::value v = {
		{"action", "add"},
		{"identifier", "albumart"},
		{"synchronously_draw", true},
		{"scaling_position_x", 0.5},
		{"scaling_position_y", 0.5},
		{"scaler", Config.albumart_scaler},
		{"path", path},
		{"x", x_offset},
		{"y", y_offset},
		{"width", width},
		{"height", height},
	};

	stream << v << std::endl;
}

void UeberzugBackend::removeArtwork()
{
	if (!process.running())
		return;

	boost::json::value v = {
		{"action", "remove"},
		{"identifier", "albumart"},
	};

	stream << v << std::endl;
}

void UeberzugBackend::stop()
{
	// close the underlying pipe, which causes ueberzug to exit
	stream.pipe().close();
	process.wait();
}

#endif // ENABLE_ARTWORK
