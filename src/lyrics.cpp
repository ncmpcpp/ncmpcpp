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

#include "charset.h"
#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "media_library.h"
#include "playlist_editor.h"
#include "settings.h"
#include "song.h"
#include "status_checker.h"
#include "tag_editor.h"

using namespace Global;
using std::vector;
using std::string;

Scrollpad *Global::sLyrics;

MPD::Song Global::lyrics_song;

const string lyrics_folder = home_folder + "/.lyrics";

#ifdef HAVE_CURL_CURL_H
pthread_mutex_t Global::curl = PTHREAD_MUTEX_INITIALIZER;
#endif

namespace
{
	pthread_t lyrics_downloader;
	bool lyrics_ready;
	
	void *GetLyrics(void *);
}

void Lyrics::Init()
{
	sLyrics = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	sLyrics->SetTimeout(ncmpcpp_window_timeout);
}

void Lyrics::Resize()
{
	sLyrics->Resize(COLS, main_height);
}

void Lyrics::Update()
{
	if (!reload_lyrics)
		return;
	
	const MPD::Song &s = mPlaylist->at(now_playing);
	if (!s.GetArtist().empty() && !s.GetTitle().empty())
		Get();
	else
		reload_lyrics = 0;
}

bool Lyrics::Ready()
{
	if (!lyrics_ready)
		return false;
	pthread_join(lyrics_downloader, NULL);
	sLyrics->Flush();
	lyrics_downloader = 0;
	lyrics_ready = 0;
	return true;
}

void Lyrics::Get()
{
	if (wCurrent == sLyrics && !reload_lyrics)
	{
		wCurrent->Hide();
		current_screen = prev_screen;
		wCurrent = wPrev;
//		redraw_screen = 1;
		redraw_header = 1;
		if (current_screen == csLibrary)
		{
			MediaLibrary::Refresh();
		}
		else if (current_screen == csPlaylistEditor)
		{
			PlaylistEditor::Refresh();
		}
#		ifdef HAVE_TAGLIB_H
		else if (current_screen == csTagEditor)
		{
			TagEditor::Refresh();
		}
#		endif // HAVE_TAGLIB_H
	}
	else if (
	    reload_lyrics
	||  (wCurrent == mPlaylist && !mPlaylist->Empty())
	||  (wCurrent == mBrowser && mBrowser->Current().type == MPD::itSong)
	||  (wCurrent == mSearcher && !mSearcher->Current().first)
	||  (wCurrent == mLibSongs && !mLibSongs->Empty())
	||  (wCurrent == mPlaylistEditor && !mPlaylistEditor->Empty())
#	ifdef HAVE_TAGLIB_H
	||  (wCurrent == mEditorTags && !mEditorTags->Empty())
#	endif // HAVE_TAGLIB_H
		)
	{
#		ifdef HAVE_CURL_CURL_H
		if (lyrics_downloader)
		{
			ShowMessage("Lyrics are being downloaded...");
			return;
		}
#		endif
		
		MPD::Song *s = 0;
		int id;
		
		if (reload_lyrics)
		{
			current_screen = csPlaylist;
			wCurrent = mPlaylist;
			reload_lyrics = 0;
			id = now_playing;
		}
		else
			id = ((Menu<MPD::Song> *)wCurrent)->Choice();
		
		switch (current_screen)
		{
			case csPlaylist:
				s = &mPlaylist->at(id);
				break;
			case csBrowser:
				s = mBrowser->at(id).song;
				break;
			case csSearcher:
				s = mSearcher->at(id).second;
				break;
			case csLibrary:
				s = &mLibSongs->at(id);
				break;
			case csPlaylistEditor:
				s = &mPlaylistEditor->at(id);
				break;
#				ifdef HAVE_TAGLIB_H
			case csTagEditor:
				s = &mEditorTags->at(id);
				break;
#				endif // HAVE_TAGLIB_H
			default:
				break;
		}
		if (!s->GetArtist().empty() && !s->GetTitle().empty())
		{
			lyrics_scroll_begin = 0;
			lyrics_song = *s;
			wPrev = wCurrent;
			prev_screen = current_screen;
			wCurrent = sLyrics;
			current_screen = csLyrics;
			redraw_header = 1;
			sLyrics->Clear();
			sLyrics->WriteXY(0, 0, 0, "Fetching lyrics...");
			sLyrics->Refresh();
#			ifdef HAVE_CURL_CURL_H
			if (!lyrics_downloader)
			{
				pthread_create(&lyrics_downloader, NULL, GetLyrics, s);
			}
#			else
			GetLyrics(s);
			sLyrics->Flush();
#			endif
		}
	}
}

#ifdef HAVE_CURL_CURL_H

namespace
{
	bool lyricwiki_not_found(const string &s)
	{
		return s == "Not found";
	}
	
	bool lyricsplugin_not_found(const string &s)
	{
		if  (s.empty())
			return true;
		for (string::const_iterator it = s.begin(); it != s.end(); it++)
			if (isprint(*it))
				return false;
		return true;
	}
	
	const LyricsPlugin lyricwiki =
	{
		"http://lyricwiki.org/api.php?artist=%artist%&song=%title%&fmt=xml",
		"<lyrics>",
		"</lyrics>",
		lyricwiki_not_found
	};
	
	const LyricsPlugin lyricsplugin =
	{
		"http://www.lyricsplugin.com/winamp03/plugin/?artist=%artist%&title=%title%",
		"<div id=\"lyrics\">",
		"</div>",
		lyricsplugin_not_found
	};
	
	const char *lyricsplugins_list[] =
	{
		"lyricwiki.org",
		"lyricsplugin.com",
		0
	};
	
	const LyricsPlugin *ChooseLyricsPlugin(int i)
	{
		switch (i)
		{
			case 0:
				return &lyricwiki;
			case 1:
				return &lyricsplugin;
			default:
				return &lyricwiki;
		}
	}
}

const char *GetLyricsPluginName(int offset)
{
	return lyricsplugins_list[offset];
}

#endif // HAVE_CURL_CURL_H

namespace {
void *GetLyrics(void *song)
{
	string artist = static_cast<MPD::Song *>(song)->GetArtist();
	string title = static_cast<MPD::Song *>(song)->GetTitle();
	
	locale_to_utf(artist);
	locale_to_utf(title);
	
	string filename = artist + " - " + title + ".txt";
	EscapeUnallowedChars(filename);
	const string fullpath = lyrics_folder + "/" + filename;
	
	mkdir(lyrics_folder.c_str(), 0755);
	
	std::ifstream input(fullpath.c_str());
	
	if (input.is_open())
	{
		bool first = 1;
		string line;
		while (getline(input, line))
		{
			if (!first)
				*sLyrics << "\n";
			utf_to_locale(line);
			*sLyrics << line;
			first = 0;
		}
#		ifdef HAVE_CURL_CURL_H
		lyrics_ready = 1;
		pthread_exit(NULL);
#		endif
	}
#	ifdef HAVE_CURL_CURL_H
	CURLcode code;
	
	const LyricsPlugin *my_lyrics = ChooseLyricsPlugin(Config.lyrics_db);
	
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
		*sLyrics << "Error while fetching lyrics: " << curl_easy_strerror(code);
		lyrics_ready = 1;
		pthread_exit(NULL);
	}
	
	size_t a, b;
	a = result.find(my_lyrics->tag_open)+strlen(my_lyrics->tag_open);
	b = result.find(my_lyrics->tag_close, a);
	result = result.substr(a, b-a);
	
	if (my_lyrics->not_found(result))
	{
		*sLyrics << "Not found";
		lyrics_ready = 1;
		pthread_exit(NULL);
	}
	
	for (size_t i = result.find("&lt;"); i != string::npos; i = result.find("&lt;"))
		result.replace(i, 4, "<");
	for (size_t i = result.find("&gt;"); i != string::npos; i = result.find("&gt;"))
		result.replace(i, 4, ">");
	
	EscapeHtml(result);
	Trim(result);
	
	*sLyrics << utf_to_locale_cpy(result);
	
	std::ofstream output(fullpath.c_str());
	if (output.is_open())
	{
		output << result;
		output.close();
	}
	lyrics_ready = 1;
	pthread_exit(NULL);
#	else
	else
		*sLyrics << "Local lyrics not found. As ncmpcpp has been compiled without curl support, you can put appropriate lyrics into ~/.lyrics directory (file syntax is \"ARTIST - TITLE.txt\") or recompile ncmpcpp with curl support.";
	return NULL;
#	endif
}
}
