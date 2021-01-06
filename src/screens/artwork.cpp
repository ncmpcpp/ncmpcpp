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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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
static pid_t child_pid;
static int ueberzug_fd;
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
	drawArtwork(it->c_str(), x_offset, MainStartY, width, MainHeight);
}


// UeberzugBackend

bool UeberzugBackend::process_ok = false;

int writen(const int fd, const char *buf, const size_t count)
{
	size_t n = count;
	while (n > 0)
	{
		ssize_t ret = write(fd, buf, n);
		if ((ret == -1 && errno == EINTR) || (errno == EWOULDBLOCK) || (errno == EAGAIN))
		{
			continue;
		}
		else
		{
			break;
		}

		n -= ret;
		buf += ret;
	}

	return n == 0 ? 0 : -1;
}

void UeberzugBackend::init()
{
	int ret;

	// create pipe from ncmpcpp to ueberzug
	int pipefd[2];
	if (pipe(pipefd))
	{
		throw std::runtime_error("Unable to create pipe");
	}

	// self-pipe trick to check if fork-exec succeeded
	// https://stackoverflow.com/a/1586277
	// https://cr.yp.to/docs/selfpipe.html
	// https://lkml.org/lkml/2006/7/10/300
	int exec_check_pipefd[2];
	if (pipe(exec_check_pipefd))
	{
		throw std::runtime_error("Unable to create pipe");
	}
	if (fcntl(exec_check_pipefd[1], F_SETFD, fcntl(exec_check_pipefd[1], F_GETFD) | FD_CLOEXEC))
	{
		throw std::runtime_error("Unable to set FD_CLOEXEC on pipe");
	}

	// fork and exec ueberzug
	child_pid = fork();
	if (child_pid == -1)
	{
		throw std::runtime_error("Unable to fork ueberzug process");
	}
	else if (child_pid == 0)
	{
		// redirect pipe output to ueberzug stdin
		while (-1 == dup2(pipefd[0], STDIN_FILENO) && errno == EINTR);
		//close pipes
		close(pipefd[0]);
		close(pipefd[1]);

		// close self-pipe
		close(exec_check_pipefd[0]);

		// exec ueberzug
		const char* const argv[] = {"ueberzug", "layer", "--silent", "--parser", "simple", nullptr};
		// const_cast ok, exec doesn't modify argv
		execvp(argv[0], const_cast<char *const *>(argv));

		// if exec() fails, write to self-pipe and abort
		write(exec_check_pipefd[1], &errno, sizeof(errno));
		close(exec_check_pipefd[1]);
		std::abort();
	}

	// ueberzug pipes
	ueberzug_fd = pipefd[1];
	close(pipefd[0]);

	// check self-pipe to see if ueberzug was successfully started
	int err;
	close(exec_check_pipefd[1]);
	while ((ret = read(exec_check_pipefd[0], &err, sizeof(errno))) == -1)
	{
		if (errno != EAGAIN && errno != EINTR) break;
	}
	if (ret)
	{
		// reap the failed ueberzug process
		waitpid(child_pid, nullptr, 0);
	}
	else
	{
		process_ok = true;
		// set ueberzug cleanup routine
		std::atexit(UeberzugBackend::stop);
	}
	close(exec_check_pipefd[0]);
}

void UeberzugBackend::updateArtwork(std::string path, int x_offset, int y_offset, int width, int height)
{
	if (!process_ok)
		return;

	std::stringstream stream;
	stream << "action\tadd\t";
	stream << "identifier\talbumart\t";
	stream << "synchronously_draw\tTrue\t";
	stream << "scaling_position_x\t0.5\t";
	stream << "scaling_position_y\t0.5\t";
	stream << "scaler\t" << Config.albumart_scaler << "\t";
	stream << "path\t" << path << "\t";
	stream << "x\t" << x_offset << "\t";
	stream << "y\t" << y_offset << "\t";
	stream << "width\t" << width << "\t";
	stream << "height\t" << height << "\n";
	writen(ueberzug_fd, stream.str().c_str(), stream.str().length());
}

void UeberzugBackend::removeArtwork()
{
	if (!process_ok)
		return;

	std::stringstream stream;
	stream << "action\tremove\t";
	stream << "identifier\talbumart\n";
	writen(ueberzug_fd, stream.str().c_str(), stream.str().length());
}

void UeberzugBackend::stop()
{
	// close the ueberzug pipe, which causes ueberzug to exit
	close(ueberzug_fd);
	waitpid(child_pid, nullptr, 0);
}

#endif // ENABLE_ARTWORK
