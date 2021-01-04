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

#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>

#include "macro_utilities.h"
#include "screens/screen_switcher.h"
#include "statusbar.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

Artwork* myArtwork;
std::string ueberzug_fifo;
pid_t child_pid = 0;
std::string current_artwork_path;

extern char** environ;

Artwork::Artwork()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border()))
{
	pid_t parent_pid = getpid();
	int ret;

	// generate ueberzug fifo name
	char fifo_fmt[] = "/tmp/ncmpcpp_ueberzug.%1%.fifo";
	ueberzug_fifo = boost::str(boost::format(fifo_fmt) % parent_pid);
	mkfifo(ueberzug_fifo.c_str(), 0666);
	assert(ret == 0);

	std::atexit(stopUeberzug);

	// start ueberzug process
	{
		// argv
		char argv1[] = "bash";
		char argv2[] = "-c";
		std::string cmd = boost::str(boost::format("tail --follow %1% | ueberzug layer --silent --parser simple") % ueberzug_fifo);
		char* argv3 = strdup(cmd.c_str());
		char* argv[] = {argv1, argv2, argv3, nullptr};

		// attrp
		posix_spawnattr_t attr;
		ret = posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP);
		assert(ret == 0);
		ret = posix_spawnattr_setpgroup(&attr, 0);
		assert(ret == 0);

		// spawn process
		ret = posix_spawnp(&child_pid, "bash", nullptr, &attr, argv, environ);
		assert(ret == 0);
		free(argv3);
	}
}

void Artwork::stopUeberzug()
{
	removeArtwork();
	std::remove(ueberzug_fifo.c_str());
	kill(-1*child_pid, SIGTERM);
}

void Artwork::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	updateArtwork(current_artwork_path);
}

void Artwork::switchTo() {
	SwitchTo::execute(this);
	drawHeader();
}

std::wstring Artwork::title() { return L"Artwork"; }

void Artwork::showArtwork(std::string path, int x_offset, int y_offset, int width, int height)
{
	std::string dir = Config.mpd_music_dir;

	std::ofstream stream;
	stream.open(ueberzug_fifo, std::ios_base::app);
	stream << "action\tadd\t";
	stream << "identifier\talbumart\t";
	stream << "synchronously_draw\tTrue\t";
	stream << "scaler\tcontain\t";
	stream << "scaling_position_x\t0.5\t";
	stream << "scaling_position_y\t0.5\t";
	stream << "path\t" << path << "\t";
	stream << "x\t" << x_offset << "\t";
	stream << "y\t" << y_offset << "\t";
	stream << "width\t" << width << "\t";
	stream << "height\t" << height << "\n";
}

void Artwork::removeArtwork()
{
	std::ofstream stream;
	stream.open(ueberzug_fifo, std::ios_base::app);
	stream << "action\tremove\t"
		"identifier\talbumart\n";
}

void Artwork::updateArtwork(std::string path) {
	if (isVisible(myArtwork))
	{
		size_t x_offset, width;
		myArtwork->getWindowResizeParams(x_offset, width);
		std::string fullpath = Config.mpd_music_dir + path + "/cover.jpg";
		showArtwork(fullpath, x_offset, MainStartY, width, MainHeight);
		current_artwork_path = path;
	}
}
