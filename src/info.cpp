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

#include "info.h"

#ifdef HAVE_CURL_CURL_H
# include <fstream>
# include <sys/stat.h>
# include <pthread.h>
# include "curl/curl.h"
# include "helpers.h"
#endif

#include "browser.h"
#include "charset.h"
#include "global.h"
#include "media_library.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "status_checker.h"
#include "tag_editor.h"

using namespace Global;
using std::string;
using std::vector;

#ifdef HAVE_CURL_CURL_H
const std::string Info::Folder = home_folder + "/.ncmpcpp/artists";
bool Info::ArtistReady = 0;
pthread_t Info::Downloader = 0;
#endif

Info *myInfo = new Info;

void Info::Init()
{
	w = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	w->SetTimeout(ncmpcpp_window_timeout);
}

void Info::Resize()
{
	w->Resize(COLS, main_height);
}

std::string Info::Title()
{
	return itsTitle;
}

void Info::Update()
{
#	ifdef HAVE_CURL_CURL_H
	if (!ArtistReady)
		return;
	
	pthread_join(Downloader, NULL);
	w->Flush();
	Downloader = 0;
	ArtistReady = 0;
#	endif // HAVE_CURL_CURL_H
}

void Info::GetSong()
{
	if (myScreen == this)
	{
		w->Hide();
		current_screen = prev_screen;
		myScreen = myOldScreen;
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
	    (myScreen == myPlaylist && !myPlaylist->Main()->Empty())
	||  (myScreen == myBrowser && myBrowser->Main()->Current().type == MPD::itSong)
	||  (myScreen == mySearcher && !mySearcher->Main()->Current().first)
	||  (myScreen->Cmp() == myLibrary->Songs && !myLibrary->Songs->Empty())
	||  (myScreen->Cmp() == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
#	ifdef HAVE_TAGLIB_H
	||  (myScreen->Cmp() == myTagEditor->Tags && !myTagEditor->Tags->Empty())
#	endif // HAVE_TAGLIB_H
		)
	{
		MPD::Song *s = 0;
		size_t id = reinterpret_cast<Menu<MPD::Song> *>(((Screen<Window> *)myScreen)->Main())->Choice();
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
#			ifdef HAVE_TAGLIB_H
			case csTagEditor:
				s = &myTagEditor->Tags->at(id);
				break;
#			endif // HAVE_TAGLIB_H
			default:
				break;
		}
		myOldScreen = myScreen;
		myScreen = this;
		prev_screen = current_screen;
		current_screen = csInfo;
		redraw_header = 1;
		itsTitle = "Song info";
		w->Clear();
		PrepareSong(*s);
		w->Flush();
		w->Hide();
	}
}

#ifdef HAVE_CURL_CURL_H

void Info::GetArtist()
{
	if (myScreen == this)
	{
		w->Hide();
		current_screen = prev_screen;
		myScreen = myOldScreen;
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
	    (myScreen == myPlaylist && !myPlaylist->Main()->Empty())
	||  (myScreen == myBrowser && myBrowser->Main()->Current().type == MPD::itSong)
	||  (myScreen == mySearcher && !mySearcher->Main()->Current().first)
	||  (myScreen->Cmp() == myLibrary->Artists && !myLibrary->Artists->Empty())
	||  (myScreen->Cmp() == myLibrary->Songs && !myLibrary->Songs->Empty())
	||  (myScreen->Cmp() == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
#	ifdef HAVE_TAGLIB_H
	||  (myScreen->Cmp() == myTagEditor->Tags && !myTagEditor->Tags->Empty())
#	endif // HAVE_TAGLIB_H
		)
	{
		if (Downloader && !ArtistReady)
		{
			ShowMessage("Artist's info is being downloaded...");
			return;
		}
		else if (ArtistReady)
			Update();
		
		string *artist = new string();
		size_t id = reinterpret_cast<Menu<MPD::Song> *>(((Screen<Window> *)myScreen)->Main())->Choice();
		switch (current_screen)
		{
			case csPlaylist:
				*artist = myPlaylist->Main()->at(id).GetArtist();
				break;
			case csBrowser:
				*artist = myBrowser->Main()->at(id).song->GetArtist();
				break;
			case csSearcher:
				*artist = mySearcher->Main()->at(id).second->GetArtist();
				break;
			case csLibrary:
				*artist = myLibrary->Artists->at(id);
				break;
			case csPlaylistEditor:
				*artist = myPlaylistEditor->Content->at(id).GetArtist();
				break;
#			ifdef HAVE_TAGLIB_H
			case csTagEditor:
				*artist = myTagEditor->Tags->at(id).GetArtist();
				break;
#			endif // HAVE_TAGLIB_H
			default:
				break;
		}
		if (!artist->empty())
		{
			myOldScreen = myScreen;
			myScreen = this;
			prev_screen = current_screen;
			current_screen = csInfo;
			redraw_header = 1;
			itsTitle = "Artist's info - " + *artist;
			w->Clear();
			w->WriteXY(0, 0, 0, "Fetching artist's info...");
			w->Refresh();
			if (!Downloader)
			{
				pthread_create(&Downloader, NULL, PrepareArtist, artist);
			}
		}
		else
			delete artist;
	}
}

void *Info::PrepareArtist(void *ptr)
{
	string *strptr = static_cast<string *>(ptr);
	string artist = *strptr;
	delete strptr;
	
	locale_to_utf(artist);
	
	string filename = artist + ".txt";
	ToLower(filename);
	EscapeUnallowedChars(filename);
	
	const string fullpath = Folder + "/" + filename;
	mkdir(Folder.c_str(), 0755);
	
	string result;
	std::ifstream input(fullpath.c_str());
	
	if (input.is_open())
	{
		bool first = 1;
		string line;
		while (getline(input, line))
		{
			if (!first)
				*myInfo->Main() << "\n";
			utf_to_locale(line);
			*myInfo->Main() << line;
			first = 0;
		}
		input.close();
		myInfo->Main()->SetFormatting(fmtBold, "\n\nSimilar artists:\n", fmtBoldEnd, 0);
		myInfo->Main()->SetFormatting(Config.color2, "\n * ", clEnd);
		ArtistReady = 1;
		pthread_exit(NULL);
	}
	
	CURLcode code;
	
	char *c_artist = curl_easy_escape(0, artist.c_str(), artist.length());
	
	string url = "http://ws.audioscrobbler.com/2.0/?method=artist.getinfo&artist=";
	url += c_artist;
	url += "&api_key=d94e5b6e26469a2d1ffae8ef20131b79";
	
	pthread_mutex_lock(&curl);
	CURL *info = curl_easy_init();
	curl_easy_setopt(info, CURLOPT_URL, url.c_str());
	curl_easy_setopt(info, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(info, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(info, CURLOPT_CONNECTTIMEOUT, 10);
	curl_easy_setopt(info, CURLOPT_NOSIGNAL, 1);
	code = curl_easy_perform(info);
	curl_easy_cleanup(info);
	pthread_mutex_unlock(&curl);
	
	curl_free(c_artist);
	
	if (code != CURLE_OK)
	{
		*myInfo->Main() << "Error while fetching artist's info: " << curl_easy_strerror(code);
		ArtistReady = 1;
		pthread_exit(NULL);
	}
	
	size_t a, b;
	bool save = 1;
	
	a = result.find("status=\"failed\"");
	
	if (a != string::npos)
	{
		EscapeHtml(result);
		*myInfo->Main() << "Last.fm returned an error message: " << result;
		ArtistReady = 1;
		pthread_exit(NULL);
	}
	
	vector<string> similar;
	for (size_t i = result.find("<name>"); i != string::npos; i = result.find("<name>"))
	{
		result[i] = '.';
		size_t j = result.find("</name>");
		result[j] = '.';
		i += 6;
		similar.push_back(result.substr(i, j-i));
		EscapeHtml(similar.back());
	}
	vector<string> urls;
	for (size_t i = result.find("<url>"); i != string::npos; i = result.find("<url>"))
	{
		result[i] = '.';
		size_t j = result.find("</url>");
		result[j] = '.';
		i += 5;
		urls.push_back(result.substr(i, j-i));
	}
	
	a = result.find("<content>")+9;
	b = result.find("</content>");
	
	if (a == b)
	{
		result = "No description available for this artist.";
		save = 0;
	}
	else
	{
		a += 9; // for <![CDATA[
		b -= 3; // for ]]>
		result = result.substr(a, b-a);
	}
	
	EscapeHtml(result);
	Trim(result);
	
	Buffer filebuffer;
	if (save)
		filebuffer << result;
	utf_to_locale(result);
	*myInfo->Main() << result;
	
	if (save)
		filebuffer << "\n\nSimilar artists:\n";
	*myInfo->Main() << fmtBold << "\n\nSimilar artists:\n" << fmtBoldEnd;
	for (size_t i = 1; i < similar.size(); i++)
	{
		if (save)
			filebuffer << "\n * " << similar[i] << " (" << urls[i] << ")";
		utf_to_locale(similar[i]);
		utf_to_locale(urls[i]);
		*myInfo->Main() << "\n" << Config.color2 << " * " << clEnd << similar[i] << " (" << urls[i] << ")";
	}
	
	if (save)
		filebuffer << "\n\n" << urls.front();
	utf_to_locale(urls.front());
	*myInfo->Main() << "\n\n" << urls.front();
	
	if (save)
	{
		std::ofstream output(fullpath.c_str());
		if (output.is_open())
		{
			output << filebuffer.Str();
			output.close();
		}
	}
	ArtistReady = 1;
	pthread_exit(NULL);
}
#endif // HVAE_CURL_CURL_H

void Info::PrepareSong(MPD::Song &s)
{
#	ifdef HAVE_TAGLIB_H
	string path_to_file;
	if (s.IsFromDB())
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	TagLib::FileRef f(path_to_file.c_str());
	if (!f.isNull())
		s.SetComment(f.tag()->comment().to8Bit(1));
#	endif // HAVE_TAGLIB_H
	
	*w << fmtBold << Config.color1 << "Filename: " << fmtBoldEnd << Config.color2 << s.GetName() << "\n" << clEnd;
	*w << fmtBold << "Directory: " << fmtBoldEnd << Config.color2 << ShowTag(s.GetDirectory()) << "\n\n" << clEnd;
	*w << fmtBold << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << "\n" << clEnd;
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		*w << fmtBold << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps\n" << clEnd;
		*w << fmtBold << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz\n" << clEnd;
		*w << fmtBold << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << "\n" << clDefault;
	}
	else
		*w << clDefault;
#	endif // HAVE_TAGLIB_H
	*w << fmtBold << "\nTitle: " << fmtBoldEnd << ShowTag(s.GetTitle());
	*w << fmtBold << "\nArtist: " << fmtBoldEnd << ShowTag(s.GetArtist());
	*w << fmtBold << "\nAlbum: " << fmtBoldEnd << ShowTag(s.GetAlbum());
	*w << fmtBold << "\nYear: " << fmtBoldEnd << ShowTag(s.GetYear());
	*w << fmtBold << "\nTrack: " << fmtBoldEnd << ShowTag(s.GetTrack());
	*w << fmtBold << "\nGenre: " << fmtBoldEnd << ShowTag(s.GetGenre());
	*w << fmtBold << "\nComposer: " << fmtBoldEnd << ShowTag(s.GetComposer());
	*w << fmtBold << "\nPerformer: " << fmtBoldEnd << ShowTag(s.GetPerformer());
	*w << fmtBold << "\nDisc: " << fmtBoldEnd << ShowTag(s.GetDisc());
	*w << fmtBold << "\nComment: " << fmtBoldEnd << ShowTag(s.GetComment());
}

const basic_buffer<my_char_t> &Info::ShowTag(const string &tag)
{
#	ifdef _UTF8
	static WBuffer result;
	result.Clear();
	if (tag.empty())
		result << Config.empty_tags_color << ToWString(Config.empty_tag) << clEnd;
	else
		result << ToWString(tag);
	return result;
#	else
	return ::ShowTag(tag);
#	endif
}

