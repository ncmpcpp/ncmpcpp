/***************************************************************************
 *   Copyright (C) 2008-2010 by Andrzej Rybczak                            *
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
#ifdef WIN32
# include <io.h>
#else
# include <sys/stat.h>
#endif // WIN32
#include <fstream>

#include "browser.h"
#include "charset.h"
#include "curl_handle.h"
#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "lyrics_fetcher.h"
#include "playlist.h"
#include "settings.h"
#include "song.h"

#ifdef WIN32
# define LYRICS_FOLDER HOME_FOLDER"\\lyrics\\"
#else
# define LYRICS_FOLDER "/.lyrics"
#endif // WIN32

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;
using Global::myOldScreen;

const std::string Lyrics::Folder = home_path + LYRICS_FOLDER;

Lyrics *myLyrics = new Lyrics;

void Lyrics::Init()
{
	w = new Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	isInitialized = 1;
}

void Lyrics::Resize()
{
	w->Resize(COLS, MainHeight);
	w->MoveTo(0, MainStartY);
	hasToBeResized = 0;
}

void Lyrics::Update()
{
#	ifdef HAVE_CURL_CURL_H
	if (ReadyToTake)
		Take();
	
	if (DownloadInProgress)
	{
		w->Flush();
		w->Refresh();
	}
#	endif // HAVE_CURL_CURL_H
	if (ReloadNP)
	{
		if (const MPD::Song *s = myPlaylist->NowPlayingSong())
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
	if (myScreen == this)
		return myOldScreen->SwitchTo();
	
	if (!isInitialized)
		Init();
	
	if (hasToBeResized)
		Resize();
	
	itsScrollBegin = 0;
	
	myOldScreen = myScreen;
	myScreen = this;
	
	// for taking lyrics if they were downloaded
	Update();
	
	if (const MPD::Song *s = myOldScreen->CurrentSong())
	{
		itsSong = *s;
		Load();
	}
	
	Global::RedrawHeader = 1;
}

std::basic_string<my_char_t> Lyrics::Title()
{
	std::basic_string<my_char_t> result = U("Lyrics: ");
	result += Scroller(TO_WSTRING(itsSong.toString("{%a - %t}")), itsScrollBegin, w->GetWidth()-result.length()-(Config.new_design ? 2 : Global::VolumeState.length()));
	return result;
}

void Lyrics::SpacePressed()
{
	Config.now_playing_lyrics = !Config.now_playing_lyrics;
	ShowMessage("Reload lyrics if song changes: %s", Config.now_playing_lyrics ? "On" : "Off");
}

#ifdef HAVE_CURL_CURL_H
void *Lyrics::DownloadWrapper(void *this_ptr)
{
	return static_cast<Lyrics *>(this_ptr)->Download();
}

void *Lyrics::Download()
{
	std::string artist = Curl::escape(locale_to_utf_cpy(itsSong.GetArtist()));
	std::string title = Curl::escape(locale_to_utf_cpy(itsSong.GetTitle()));
	
	LyricsFetcher::Result result;
	
	for (LyricsFetcher **plugin = lyricsPlugins; *plugin != 0; ++plugin)
	{
		*w << "Fetching lyrics from " << fmtBold << (*plugin)->name() << fmtBoldEnd << "... ";
		result = (*plugin)->fetch(artist, title);
		if (result.first == false)
			*w << clRed << result.second << clEnd << "\n";
		else
			break;
	}
	
	if (result.first == true)
	{
		Save(result.second);
		
		utf_to_locale(result.second);
		w->Clear();
		*w << result.second;
	}
	else
		*w << "\nLyrics weren't found.";
	
	ReadyToTake = 1;
	pthread_exit(0);
}
#endif // HAVE_CURL_CURL_H

void Lyrics::Load()
{
#	ifdef HAVE_CURL_CURL_H
	if (DownloadInProgress)
		return;
#	endif // HAVE_CURL_CURL_H
	if (itsSong.GetArtist().empty() || itsSong.GetTitle().empty())
		return;
	
	itsSong.Localize();
	std::string file = locale_to_utf_cpy(itsSong.GetArtist()) + " - " + locale_to_utf_cpy(itsSong.GetTitle()) + ".txt";
	EscapeUnallowedChars(file);
	itsFilenamePath = Folder + "/" + file;
	
	mkdir(Folder.c_str()
#	ifndef WIN32
	, 0755
#	endif // !WIN32
	);
	
	w->Clear();
	w->Reset();
	
	std::ifstream input(itsFilenamePath.c_str());
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
		pthread_create(&Downloader, 0, DownloadWrapper, this);
		DownloadInProgress = 1;
#		else
		*w << "Local lyrics not found. As ncmpcpp has been compiled without curl support, you can put appropriate lyrics into " << Folder << " directory (file syntax is \"$ARTIST - $TITLE.txt\") or recompile ncmpcpp with curl support.";
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
		system(("/bin/sh -c \"" + Config.external_editor + " \\\"" + itsFilenamePath + "\\\"\"").c_str());
		// below is needed as screen gets cleared, but apparently
		// ncurses doesn't know about it, so we need to reload main screen
		endwin();
		initscr();
		curs_set(0);
	}
	else
		system(("nohup " + Config.external_editor + " \"" + itsFilenamePath + "\" > /dev/null 2>&1 &").c_str());
}

void Lyrics::Save(const std::string &lyrics)
{
	std::ofstream output(itsFilenamePath.c_str());
	if (output.is_open())
	{
		output << lyrics;
		output.close();
	}
}

void Lyrics::Refetch()
{
	std::string path = Folder + "/" + locale_to_utf_cpy(itsSong.GetArtist()) + " - " + locale_to_utf_cpy(itsSong.GetTitle()) + ".txt";
	if (!remove(path.c_str()))
	{
		Load();
	}
	else
	{
		static const char msg[] = "Couldn't remove \"%s\": %s";
		ShowMessage(msg, Shorten(TO_WSTRING(path), COLS-static_strlen(msg)-25).c_str(), strerror(errno));
	}
}

#ifdef HAVE_CURL_CURL_H
void Lyrics::Take()
{
	assert(ReadyToTake);
	pthread_join(Downloader, 0);
	w->Flush();
	w->Refresh();
	DownloadInProgress = 0;
	ReadyToTake = 0;
}
#endif // HAVE_CURL_CURL_H

