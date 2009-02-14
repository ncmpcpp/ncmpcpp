/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include <sys/stat.h>
#include <fstream>

#include "lyrics.h"

#include "browser.h"
#include "charset.h"
#include "global.h"
#include "helpers.h"

#include "media_library.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "settings.h"
#include "song.h"
#include "status_checker.h"
#include "tag_editor.h"

using namespace Global;
using std::vector;
using std::string;

const std::string Lyrics::Folder = home_folder + "/.lyrics";

bool Lyrics::Reload = 0;

#ifdef HAVE_CURL_CURL_H
pthread_mutex_t Global::curl = PTHREAD_MUTEX_INITIALIZER;

bool Lyrics::Ready = 0;
pthread_t Lyrics::Downloader = 0;
#endif

Lyrics *myLyrics = new Lyrics;

void Lyrics::Init()
{
	w = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	w->SetTimeout(ncmpcpp_window_timeout);
}

void Lyrics::Resize()
{
	w->Resize(COLS, main_height);
}

void Lyrics::Update()
{
#	ifdef HAVE_CURL_CURL_H
	if (myLyrics->Ready)
		myLyrics->Take();
#	endif // HAVE_CURL_CURL_H
	
	if (!Reload)
		return;
	
	const MPD::Song &s = myPlaylist->NowPlayingSong();
	if (!s.GetArtist().empty() && !s.GetTitle().empty())
		SwitchTo();
	else
		Reload = 0;
}

void Lyrics::SwitchTo()
{
	if (wCurrent == w && !Reload)
	{
		wCurrent->Hide();
		current_screen = prev_screen;
		wCurrent = wPrev;
//		redraw_screen = 1;
		redraw_header = 1;
		if (current_screen == csLibrary)
		{
			myLibrary->Refresh();
		}
		else if (current_screen == csPlaylistEditor)
		{
			myPlaylistEditor->Refresh();
		}
#		ifdef HAVE_TAGLIB_H
		else if (current_screen == csTagEditor)
		{
			myTagEditor->Refresh();
		}
#		endif // HAVE_TAGLIB_H
	}
	else if (
	    Reload
	||  (wCurrent == myPlaylist->Main() && !myPlaylist->Main()->Empty())
	||  (wCurrent == myBrowser->Main() && myBrowser->Main()->Current().type == MPD::itSong)
	||  (wCurrent == mySearcher->Main() && !mySearcher->Main()->Current().first)
	||  (wCurrent == myLibrary->Songs && !myLibrary->Songs->Empty())
	||  (wCurrent == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
#	ifdef HAVE_TAGLIB_H
	||  (wCurrent == myTagEditor->Tags && !myTagEditor->Tags->Empty())
#	endif // HAVE_TAGLIB_H
		)
	{
#		ifdef HAVE_CURL_CURL_H
		if (Downloader && !Ready)
		{
			ShowMessage("Lyrics are being downloaded...");
			return;
		}
		else if (Ready)
		{
			Take();
			return;
		}
#		endif
		
		MPD::Song *s = 0;
		int id;
		
		if (Reload)
		{
			current_screen = csPlaylist;
			wCurrent = myPlaylist->Main();
			Reload = 0;
			id = myPlaylist->NowPlaying;
		}
		else
			id = ((Menu<MPD::Song> *)wCurrent)->Choice();
		
		switch (current_screen)
		{
			case csPlaylist:
				s = &myPlaylist->Main()->at(id);
				break;
			case csBrowser:
				s = myBrowser->Main()->at(id).song;
				break;
			case csSearcher:
				s = mySearcher->Main()->at(id).second;
				break;
			case csLibrary:
				s = &myLibrary->Songs->at(id);
				break;
			case csPlaylistEditor:
				s = &myPlaylistEditor->Content->at(id);
				break;
#				ifdef HAVE_TAGLIB_H
			case csTagEditor:
				s = &myTagEditor->Tags->at(id);
				break;
#				endif // HAVE_TAGLIB_H
			default:
				break;
		}
		if (!s->GetArtist().empty() && !s->GetTitle().empty())
		{
			itsScrollBegin = 0;
			itsSong = *s;
			wPrev = wCurrent;
			prev_screen = current_screen;
			wCurrent = w;
			current_screen = csLyrics;
			redraw_header = 1;
			w->Clear();
			w->WriteXY(0, 0, 0, "Fetching lyrics...");
			w->Refresh();
#			ifdef HAVE_CURL_CURL_H
			if (!Downloader)
			{
				pthread_create(&Downloader, NULL, Get, &itsSong);
			}
#			else
			Get(s);
			w->Flush();
#			endif
		}
	}
}

std::string Lyrics::Title()
{
	string result = "Lyrics: ";
	result += TO_STRING(Scroller(itsSong.toString("%a - %t"), COLS-result.length()-volume_state.length(), itsScrollBegin));
	return result;
}

void *Lyrics::Get(void *song)
{
	string artist = static_cast<MPD::Song *>(song)->GetArtist();
	string title = static_cast<MPD::Song *>(song)->GetTitle();
	
	locale_to_utf(artist);
	locale_to_utf(title);
	
	string filename = artist + " - " + title + ".txt";
	EscapeUnallowedChars(filename);
	const string fullpath = Folder + "/" + filename;
	
	mkdir(Folder.c_str(), 0755);
	
	std::ifstream input(fullpath.c_str());
	
	if (input.is_open())
	{
		bool first = 1;
		string line;
		while (getline(input, line))
		{
			if (!first)
				*myLyrics->Main() << "\n";
			utf_to_locale(line);
			*myLyrics->Main() << line;
			first = 0;
		}
#		ifdef HAVE_CURL_CURL_H
		Ready = 1;
		pthread_exit(NULL);
#		endif
	}
#	ifdef HAVE_CURL_CURL_H
	CURLcode code;
	
	const Plugin *my_lyrics = ChoosePlugin(Config.lyrics_db);
	
	string result;
	
	char *c_artist = curl_easy_escape(0, artist.c_str(), artist.length());
	char *c_title = curl_easy_escape(0, title.c_str(), title.length());
	
	string url = my_lyrics->url;
	url.replace(url.find("%artist%"), 8, c_artist);
	url.replace(url.find("%title%"), 7, c_title);
	
	pthread_mutex_lock(&curl);
	CURL *lyrics = curl_easy_init();
	curl_easy_setopt(lyrics, CURLOPT_URL, url.c_str());
	curl_easy_setopt(lyrics, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(lyrics, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(lyrics, CURLOPT_CONNECTTIMEOUT, 10);
	curl_easy_setopt(lyrics, CURLOPT_NOSIGNAL, 1);
	code = curl_easy_perform(lyrics);
	curl_easy_cleanup(lyrics);
	pthread_mutex_unlock(&curl);
	
	curl_free(c_artist);
	curl_free(c_title);
	
	if (code != CURLE_OK)
	{
		*myLyrics->Main() << "Error while fetching lyrics: " << curl_easy_strerror(code);
		Ready = 1;
		pthread_exit(NULL);
	}
	
	size_t a, b;
	a = result.find(my_lyrics->tag_open)+strlen(my_lyrics->tag_open);
	b = result.find(my_lyrics->tag_close, a);
	result = result.substr(a, b-a);
	
	if (my_lyrics->not_found(result))
	{
		*myLyrics->Main() << "Not found";
		Ready = 1;
		pthread_exit(NULL);
	}
	
	for (size_t i = result.find("&lt;"); i != string::npos; i = result.find("&lt;"))
		result.replace(i, 4, "<");
	for (size_t i = result.find("&gt;"); i != string::npos; i = result.find("&gt;"))
		result.replace(i, 4, ">");
	
	EscapeHtml(result);
	Trim(result);
	
	*myLyrics->Main() << utf_to_locale_cpy(result);
	
	std::ofstream output(fullpath.c_str());
	if (output.is_open())
	{
		output << result;
		output.close();
	}
	Ready = 1;
	pthread_exit(NULL);
#	else
	else
		*myLyrics->Main() << "Local lyrics not found. As ncmpcpp has been compiled without curl support, you can put appropriate lyrics into ~/.lyrics directory (file syntax is \"ARTIST - TITLE.txt\") or recompile ncmpcpp with curl support.";
	return NULL;
#	endif
}

#ifdef HAVE_CURL_CURL_H

void Lyrics::Take()
{
	if (!Ready)
		return;
	pthread_join(Downloader, NULL);
	w->Flush();
	Downloader = 0;
	Ready = 0;
}

const char *Lyrics::GetPluginName(int offset)
{
	return PluginsList[offset];
}

bool Lyrics::LyricWiki_NotFound(const string &s)
{
	return s == "Not found";
}

bool Lyrics::LyricsPlugin_NotFound(const string &s)
{
	if  (s.empty())
		return true;
	for (string::const_iterator it = s.begin(); it != s.end(); it++)
		if (isprint(*it))
			return false;
	return true;
}

const Lyrics::Plugin Lyrics::LyricWiki =
{
	"http://lyricwiki.org/api.php?artist=%artist%&song=%title%&fmt=xml",
	"<lyrics>",
	"</lyrics>",
	LyricWiki_NotFound
};

const Lyrics::Plugin Lyrics::LyricsPlugin =
{
	"http://www.lyricsplugin.com/winamp03/plugin/?artist=%artist%&title=%title%",
	"<div id=\"lyrics\">",
	"</div>",
	LyricsPlugin_NotFound
};

const char *Lyrics::PluginsList[] =
{
	"lyricwiki.org",
	"lyricsplugin.com",
	0
};

const Lyrics::Plugin *Lyrics::ChoosePlugin(int i)
{
	switch (i)
	{
		case 0:
			return &LyricWiki;
		case 1:
			return &LyricsPlugin;
		default:
			return &LyricWiki;
	}
}

#endif // HAVE_CURL_CURL_H

