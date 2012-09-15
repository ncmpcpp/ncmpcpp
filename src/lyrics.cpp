/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
 *   electricityispower@gmail.com                                          *
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

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fstream>

#include "browser.h"
#include "charset.h"
#include "curl_handle.h"
#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "playlist.h"
#include "scrollpad.h"
#include "settings.h"
#include "song.h"
#include "statusbar.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

#ifdef HAVE_CURL_CURL_H
LyricsFetcher **Lyrics::itsFetcher = 0;
std::queue<MPD::Song *> Lyrics::itsToDownload;
pthread_mutex_t Lyrics::itsDIBLock = PTHREAD_MUTEX_INITIALIZER;
size_t Lyrics::itsWorkersNumber = 0;
#endif // HAVE_CURL_CURL_H

Lyrics *myLyrics;

Lyrics::Lyrics()
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone))
, ReloadNP(0),
#ifdef HAVE_CURL_CURL_H
isReadyToTake(0), isDownloadInProgress(0),
#endif // HAVE_CURL_CURL_H
	itsScrollBegin(0)
{ }

void Lyrics::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

void Lyrics::update()
{
#	ifdef HAVE_CURL_CURL_H
	if (isReadyToTake)
		Take();
	
	if (isDownloadInProgress)
	{
		w.flush();
		w.refresh();
	}
#	endif // HAVE_CURL_CURL_H
	if (ReloadNP)
	{
		const MPD::Song s = myPlaylist->nowPlayingSong();
		if (!s.empty() && !s.getArtist().empty() && !s.getTitle().empty())
		{
			drawHeader();
			itsScrollBegin = 0;
			itsSong = s;
			Load();
		}
		ReloadNP = 0;
	}
}

void Lyrics::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
	{
#		ifdef HAVE_CURL_CURL_H
		// take lyrics if they were downloaded
		if (isReadyToTake)
			Take();
		
		if (isDownloadInProgress || itsWorkersNumber > 0)
		{
			Statusbar::msg("Lyrics are being downloaded...");
			return;
		}
#		endif // HAVE_CURL_CURL_H
		
		const MPD::Song *s = currentSong(myScreen);
		if (!s)
			return;
		
		if (!s->getArtist().empty() && !s->getTitle().empty())
		{
			SwitchTo::execute(this);
			itsScrollBegin = 0;
			itsSong = *s;
			Load();
			drawHeader();
		}
		else
			Statusbar::msg("Song must have both artist and title tag set");
	}
	else
		switchToPreviousScreen();
}

std::wstring Lyrics::title()
{
	std::wstring result = L"Lyrics: ";
	result += Scroller(ToWString(itsSong.toString("{%a - %t}", ", ")), itsScrollBegin, COLS-result.length()-(Config.new_design ? 2 : Global::VolumeState.length()));
	return result;
}

void Lyrics::spacePressed()
{
	Config.now_playing_lyrics = !Config.now_playing_lyrics;
	Statusbar::msg("Reload lyrics if song changes: %s", Config.now_playing_lyrics ? "On" : "Off");
}

#ifdef HAVE_CURL_CURL_H
void Lyrics::DownloadInBackground(const MPD::Song &s)
{
	if (s.empty() || s.getArtist().empty() || s.getTitle().empty())
		return;
	
	std::string filename = GenerateFilename(s);
	std::ifstream f(filename.c_str());
	if (f.is_open())
	{
		f.close();
		return;
	}
	Statusbar::msg("Fetching lyrics for \"%s\"...", s.toString(Config.song_status_format_no_colors, Config.tags_separator).c_str());
	
	MPD::Song *s_copy = new MPD::Song(s);
	pthread_mutex_lock(&itsDIBLock);
	if (itsWorkersNumber == itsMaxWorkersNumber)
		itsToDownload.push(s_copy);
	else
	{
		++itsWorkersNumber;
		pthread_t t;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&t, &attr, DownloadInBackgroundImpl, s_copy);
	}
	pthread_mutex_unlock(&itsDIBLock);
}

void *Lyrics::DownloadInBackgroundImpl(void *void_ptr)
{
	MPD::Song *s = static_cast<MPD::Song *>(void_ptr);
	DownloadInBackgroundImplHelper(*s);
	delete s;
	
	while (true)
	{
		pthread_mutex_lock(&itsDIBLock);
		if (itsToDownload.empty())
		{
			pthread_mutex_unlock(&itsDIBLock);
			break;
		}
		else
		{
			s = itsToDownload.front();
			itsToDownload.pop();
			pthread_mutex_unlock(&itsDIBLock);
		}
		DownloadInBackgroundImplHelper(*s);
		delete s;
	}
	
	pthread_mutex_lock(&itsDIBLock);
	--itsWorkersNumber;
	pthread_mutex_unlock(&itsDIBLock);
	
	pthread_exit(0);
}

void Lyrics::DownloadInBackgroundImplHelper(const MPD::Song &s)
{
	std::string artist = Curl::escape(s.getArtist());
	std::string title = Curl::escape(s.getTitle());
	
	LyricsFetcher::Result result;
	bool fetcher_defined = itsFetcher && *itsFetcher;
	for (LyricsFetcher **plugin = fetcher_defined ? itsFetcher : lyricsPlugins; *plugin != 0; ++plugin)
	{
		result = (*plugin)->fetch(artist, title);
		if (result.first)
			break;
		if (fetcher_defined)
			break;
	}
	if (result.first == true)
		Save(GenerateFilename(s), result.second);
}

void *Lyrics::Download()
{
	std::string artist = Curl::escape(itsSong.getArtist());
	std::string title_ = Curl::escape(itsSong.getTitle());
	
	LyricsFetcher::Result result;
	
	// if one of plugins is selected, try only this one,
	// otherwise try all of them until one of them succeeds
	bool fetcher_defined = itsFetcher && *itsFetcher;
	for (LyricsFetcher **plugin = fetcher_defined ? itsFetcher : lyricsPlugins; *plugin != 0; ++plugin)
	{
		w << L"Fetching lyrics from " << NC::fmtBold << ToWString((*plugin)->name()) << NC::fmtBoldEnd << L"... ";
		result = (*plugin)->fetch(artist, title_);
		if (result.first == false)
			w << NC::clRed << ToWString(result.second) << NC::clEnd << '\n';
		else
			break;
		if (fetcher_defined)
			break;
	}
	
	if (result.first == true)
	{
		Save(itsFilename, result.second);
		w.clear();
		IConv::utf8ToLocale_(result.second);
		w << result.second;
	}
	else
		w << '\n' << L"Lyrics weren't found.";
	
	isReadyToTake = 1;
	pthread_exit(0);
}

void *Lyrics::DownloadWrapper(void *this_ptr)
{
	return static_cast<Lyrics *>(this_ptr)->Download();
}
#endif // HAVE_CURL_CURL_H

std::string Lyrics::GenerateFilename(const MPD::Song &s)
{
	std::string filename;
	if (Config.store_lyrics_in_song_dir)
	{
		if (s.isFromDatabase())
		{
			filename = Config.mpd_music_dir;
			filename += "/";
			filename += s.getURI();
		}
		else
			filename = s.getURI();
		// replace song's extension with .txt
		size_t dot = filename.rfind('.');
		assert(dot != std::string::npos);
		filename.resize(dot);
		filename += ".txt";
	}
	else
	{
		std::string file = s.getArtist();
		file += " - ";
		file += s.getTitle();
		file += ".txt";
		removeInvalidCharsFromFilename(file);
		filename = Config.lyrics_directory;
		filename += "/";
		filename += file;
	}
	return filename;
}

void Lyrics::Load()
{
#	ifdef HAVE_CURL_CURL_H
	if (isDownloadInProgress)
		return;
#	endif // HAVE_CURL_CURL_H
	
	assert(!itsSong.getArtist().empty());
	assert(!itsSong.getTitle().empty());
	
	itsFilename = GenerateFilename(itsSong);
	
	CreateDir(Config.lyrics_directory);
	
	w.clear();
	w.reset();
	
	std::ifstream input(itsFilename.c_str());
	if (input.is_open())
	{
		bool first = 1;
		std::string line;
		while (getline(input, line))
		{
			if (!first)
				w << '\n';
			IConv::utf8ToLocale_(line);
			w << line;
			first = 0;
		}
		w.flush();
		if (ReloadNP)
			w.refresh();
	}
	else
	{
#		ifdef HAVE_CURL_CURL_H
		pthread_create(&itsDownloader, 0, DownloadWrapper, this);
		isDownloadInProgress = 1;
#		else
		w << U("Local lyrics not found. As ncmpcpp has been compiled without curl support, you can put appropriate lyrics into ") << TO_WSTRING(Config.lyrics_directory) << U(" directory (file syntax is \"$ARTIST - $TITLE.txt\") or recompile ncmpcpp with curl support.");
		w.flush();
#		endif
	}
}

void Lyrics::Edit()
{
	assert(Global::myScreen == this);
	
	if (Config.external_editor.empty())
	{
		Statusbar::msg("Proper external_editor variable has to be set in configuration file");
		return;
	}
	
	Statusbar::msg("Opening lyrics in external editor...");
	
	GNUC_UNUSED int res;
	if (Config.use_console_editor)
	{
		res = system(("/bin/sh -c \"" + Config.external_editor + " \\\"" + itsFilename + "\\\"\"").c_str());
		// below is needed as screen gets cleared, but apparently
		// ncurses doesn't know about it, so we need to reload main screen
		endwin();
		initscr();
		curs_set(0);
	}
	else
		res = system(("nohup " + Config.external_editor + " \"" + itsFilename + "\" > /dev/null 2>&1 &").c_str());
}

#ifdef HAVE_CURL_CURL_H
void Lyrics::Save(const std::string &filename, const std::string &lyrics)
{
	std::ofstream output(filename.c_str());
	if (output.is_open())
	{
		output << lyrics;
		output.close();
	}
}

void Lyrics::Refetch()
{
	if (remove(itsFilename.c_str()) && errno != ENOENT)
	{
		const char msg[] = "Couldn't remove \"%ls\": %s";
		Statusbar::msg(msg, wideShorten(ToWString(itsFilename), COLS-const_strlen(msg)-25).c_str(), strerror(errno));
		return;
	}
	Load();
}

void Lyrics::ToggleFetcher()
{
	if (itsFetcher && *itsFetcher)
		++itsFetcher;
	else
		itsFetcher = &lyricsPlugins[0];
	if (*itsFetcher)
		Statusbar::msg("Using lyrics database: %s", (*itsFetcher)->name());
	else
		Statusbar::msg("Using all lyrics databases");
}

void Lyrics::Take()
{
	assert(isReadyToTake);
	pthread_join(itsDownloader, 0);
	w.flush();
	w.refresh();
	isDownloadInProgress = 0;
	isReadyToTake = 0;
}
#endif // HAVE_CURL_CURL_H

