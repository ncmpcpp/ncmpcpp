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

#include <sstream>

#include "macro_utilities.h"
#include "screens/screen_switcher.h"
#include "statusbar.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

Artwork* myArtwork;
int ueberzug_fd;
std::string current_artwork_path;

int writen(const int fd, const char *buf, const size_t count)
{
	size_t n = count;
	while (n > 0)
	{
		ssize_t bytes = write(fd, buf, n);
		if (-1 == bytes)
		{
			break;
		}

		n -= bytes;
		buf += bytes;
	}

	return n == 0 ? 0 : -1;
}

Artwork::Artwork()
: Screen(NC::Window(0, MainStartY, COLS, MainHeight, "", NC::Color::Default, NC::Border()))
{
	int ret;

	// create pipe from ncmpcpp to ueberzug
	int pipefd[2];
	ret = pipe(pipefd);
	if (ret == -1)
	{
		throw std::runtime_error("Unable to create pipe");
	}

	// fork and exec ueberzug
	pid_t child_pid = fork();
	if (child_pid == -1)
	{
		throw std::runtime_error("Unable to fork ueberzug process");
	}
	else if (child_pid == 0)
	{
		// redirect pipe output to ueberzug stdin
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);

		const char* const argv[] = {"ueberzug", "layer", "--silent", "--parser", "simple", nullptr};
		// const_cast ok, exec doesn't modify argv
		execvp(argv[0], const_cast<char *const *>(argv));
	}

	ueberzug_fd = pipefd[1];
	close(pipefd[0]);

	std::atexit(stopUeberzug);
}

void Artwork::stopUeberzug()
{
	removeArtwork();
	close(ueberzug_fd);
}

void Artwork::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	updateArtwork(current_artwork_path);
}

void Artwork::switchTo(){
	SwitchTo::execute(this);
	drawHeader();
}

std::wstring Artwork::title() { return L"Artwork"; }

void Artwork::showArtwork(std::string path, int x_offset, int y_offset, int width, int height)
{
	std::string dir = Config.mpd_music_dir;

	std::stringstream stream;
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

	writen(ueberzug_fd, stream.str().c_str(), stream.str().length());
}

void Artwork::removeArtwork()
{
	std::stringstream stream;
	stream << "action\tremove\t";
	stream << "identifier\talbumart\n";

	writen(ueberzug_fd, stream.str().c_str(), stream.str().length());
}

void Artwork::updateArtwork(std::string path)
{
	if (isVisible(myArtwork))
	{
		size_t x_offset, width;
		myArtwork->getWindowResizeParams(x_offset, width);
		std::string fullpath = Config.mpd_music_dir + path + "/cover.jpg";
		showArtwork(fullpath, x_offset, MainStartY, width, MainHeight);
		current_artwork_path = path;
	}
}
