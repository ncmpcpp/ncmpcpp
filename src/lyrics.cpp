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
#include <fstream>

#include "browser.h"
#include "charset.h"
#include "curl_handle.h"
#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "playlist.h"
#include "settings.h"
#include "song.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;
using Global::myOldScreen;

#ifdef HAVE_CURL_CURL_H
LyricsFetcher **Lyrics::itsFetcher = 0;
std::queue<MPD::Song *> Lyrics::itsToDownload;
pthread_mutex_t Lyrics::itsDIBLock = PTHREAD_MUTEX_INITIALIZER;
size_t Lyrics::itsWorkersNumber = 0;
#endif // HAVE_CURL_CURL_H

Lyrics *myLyrics = new Lyrics;

void Lyrics::Init()
{
	w = new Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	isInitialized = 1;
}

void Lyrics::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->Resize(width, MainHeight);
	w->MoveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

void Lyrics::Update()
{
#	ifdef HAVE_CURL_CURL_H
	if (isReadyToTake)
		Take();
	
	if (isDownloadInProgress)
	{
		w->Flush();
		w->Refresh();
	}
#	endif // HAVE_CURL_CURL_H
	if (ReloadNP)
	{
		const MPD::Song *s = myPlaylist->NowPlayingSong();
		if (s && !s->GetArtist().empty() && !s->GetTitle().empty())
		{
			Global::RedrawHeader = 1;
			itsScrollBegin = 0;
			itsSong = *s;
			Load();
		}
		ReloadNP = 0;
	}
}

void Lyrics::SwitchTo()
{
	using Global::myLockedScreen;
	using Global::myInactiveScreen;
	
	if (myScreen == this)
		return myOldScreen->SwitchTo();
	
	if (!isInitialized)
		Init();
	
	if (hasToBeResized)
		Resize();
	
	itsScrollBegin = 0;
	
#	ifdef HAVE_CURL_CURL_H
	// take lyrics if they were downloaded
	if (isReadyToTake)
		Take();
	
	if (isDownloadInProgress || itsWorkersNumber > 0)
	{
		ShowMessage("Lyrics are being downloaded...");
		return;
	}
#	endif // HAVE_CURL_CURL_H
	
	if (const MPD::Song *s = myScreen->CurrentSong())
	{
		if (!s->GetArtist().empty() && !s->GetTitle().empty())
		{
			myOldScreen = myScreen;
			myScreen = this;
			
			itsSong = *s;
			Load();
			
			Global::RedrawHeader = 1;
		}
		else
		{
			ShowMessage("Song must have both artist and title tag set!");
			return;
		}
	}
	// if we resize for locked screen, we have to do that in the end since
	// fetching lyrics may fail (eg. if tags are missing) and we don't want
	// to adjust screen size then.
	if (myLockedScreen)
	{
		UpdateInactiveScreen(this);
		Resize();
	}
}

std::basic_string<my_char_t> Lyrics::Title()
{
	std::basic_string<my_char_t> result = U("Lyrics: ");
	result += Scroller(TO_WSTRING(itsSong.toString("{%a - %t}")), itsScrollBegin, COLS-result.length()-(Config.new_design ? 2 : Global::VolumeState.length()));
	return result;
}

void Lyrics::SpacePressed()
{
	Config.now_playing_lyrics = !Config.now_playing_lyrics;
	ShowMessage("Reload lyrics if song changes: %s", Config.now_playing_lyrics ? "On" : "Off");
}

#ifdef HAVE_CURL_CURL_H
void Lyrics::DownloadInBackground(const MPD::Song *s)
{
	if (!s || s->GetArtist().empty() || s->GetTitle().empty())
		return;
	if (!s->Localized())
		const_cast<MPD::Song *>(s)->Localize();
	
	std::string filename = GenerateFilename(*s);
	std::ifstream f(filename.c_str());
	if (f.is_open())
	{
		f.close();
		return;
	}
	ShowMessage("Fetching lyrics for %s...", s->toString(Config.song_status_format_no_colors).c_str());
	
	MPD::Song *s_copy = new MPD::Song(*s);
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
	DownloadInBackgroundImplHelper(s);
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
		DownloadInBackgroundImplHelper(s);
		delete s;
	}
	
	pthread_mutex_lock(&itsDIBLock);
	--itsWorkersNumber;
	pthread_mutex_unlock(&itsDIBLock);
	
	pthread_exit(0);
}

void Lyrics::DownloadInBackgroundImplHelper(MPD::Song *s)
{
	std::string artist = Curl::escape(locale_to_utf_cpy(s->GetArtist()));
	std::string title = Curl::escape(locale_to_utf_cpy(s->GetTitle()));
	
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
		Save(GenerateFilename(*s), result.second);
}

void *Lyrics::Download()
{
	std::string artist = Curl::escape(locale_to_utf_cpy(itsSong.GetArtist()));
	std::string title = Curl::escape(locale_to_utf_cpy(itsSong.GetTitle()));
	
	LyricsFetcher::Result result;
	
	// if one of plugins is selected, try only this one,
	// otherwise try all of them until one of them succeeds
	bool fetcher_defined = itsFetcher && *itsFetcher;
	for (LyricsFetcher **plugin = fetcher_defined ? itsFetcher : lyricsPlugins; *plugin != 0; ++plugin)
	{
		*w << "Fetching lyrics from " << fmtBold << (*plugin)->name() << fmtBoldEnd << "... ";
		result = (*plugin)->fetch(artist, title);
		if (result.first == false)
			*w << clRed << result.second << clEnd << "\n";
		else
			break;
		if (fetcher_defined)
			break;
	}
	
	if (result.first == true)
	{
		Save(itsFilename, result.second);
		
		utf_to_locale(result.second);
		w->Clear();
		*w << result.second;
	}
	else
		*w << "\nLyrics weren't found.";
	
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
		if (s.isFromDB())
		{
			filename = Config.mpd_music_dir;
			filename += "/";
			filename += s.GetFile();
		}
		else
			filename = s.GetFile();
		// replace song's extension with .txt
		size_t dot = filename.rfind('.');
		assert(dot != std::string::npos);
		filename.resize(dot);
		filename += ".txt";
	}
	else
	{
		std::string file = locale_to_utf_cpy(s.GetArtist());
		file += " - ";
		file += locale_to_utf_cpy(s.GetTitle());
		file += ".txt";
		EscapeUnallowedChars(file);
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
	
	assert(!itsSong.GetArtist().empty());
	assert(!itsSong.GetTitle().empty());
	
	itsSong.Localize();
	itsFilename = GenerateFilename(itsSong);
	
	CreateDir(Config.lyrics_directory);
	
	w->Clear();
	w->Reset();
	
	std::ifstream input(itsFilename.c_str());
	if (input.is_open())
	{
		bool first = 1;
		std::string line;
		while (getline(input, line))
		{
			if (!first)
				*w << "\n";
			utf_to_locale(line);
			*w << line;
			first = 0;
		}
		w->Flush();
		if (ReloadNP)
			w->Refresh();
	}
	else
	{
#		ifdef HAVE_CURL_CURL_H
		pthread_create(&itsDownloader, 0, DownloadWrapper, this);
		isDownloadInProgress = 1;
#		else
		*w << "Local lyrics not found. As ncmpcpp has been compiled without curl support, you can put appropriate lyrics into " << Config.lyrics_directory << " directory (file syntax is \"$ARTIST - $TITLE.txt\") or recompile ncmpcpp with curl support.";
		w->Flush();
#		endif
	}
}

void Lyrics::Edit()
{
	if (myScreen != this)
		return;
	
	if (Config.external_editor.empty())
	{
		ShowMessage("External editor is not set!");
		return;
	}
	
	ShowMessage("Opening lyrics in external editor...");
	
	if (Config.use_console_editor)
	{
		system(("/bin/sh -c \"" + Config.external_editor + " \\\"" + itsFilename + "\\\"\"").c_str());
		// below is needed as screen gets cleared, but apparently
		// ncurses doesn't know about it, so we need to reload main screen
		endwin();
		initscr();
		curs_set(0);
	}
	else
		system(("nohup " + Config.external_editor + " \"" + itsFilename + "\" > /dev/null 2>&1 &").c_str());
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
#endif // HAVE_CURL_CURL_H

void Lyrics::Refetch()
{
	if (remove(itsFilename.c_str()) && errno != ENOENT)
	{
		static const char msg[] = "Couldn't remove \"%s\": %s";
		ShowMessage(msg, Shorten(TO_WSTRING(itsFilename), COLS-static_strlen(msg)-25).c_str(), strerror(errno));
		return;
	}
	Load();
}

#ifdef HAVE_CURL_CURL_H
void Lyrics::ToggleFetcher()
{
	if (itsFetcher && *itsFetcher)
		++itsFetcher;
	else
		itsFetcher = &lyricsPlugins[0];
	if (*itsFetcher)
		ShowMessage("Using lyrics database: %s", (*itsFetcher)->name());
	else
		ShowMessage("Using all lyrics databases");
}

void Lyrics::Take()
{
	assert(isReadyToTake);
	pthread_join(itsDownloader, 0);
	w->Flush();
	w->Refresh();
	isDownloadInProgress = 0;
	isReadyToTake = 0;
}
#endif // HAVE_CURL_CURL_H

