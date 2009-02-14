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

Scrollpad *Global::sInfo;

const string artists_folder = home_folder + "/.ncmpcpp/artists";

namespace
{
#	ifdef HAVE_CURL_CURL_H
	pthread_t artist_info_downloader;
	bool artist_info_ready = 0;
	
	void *GetArtistInfo(void *);
#	endif
	
	void GetSongInfo(MPD::Song &, Scrollpad &);
	const basic_buffer<my_char_t> &ShowTagInInfoScreen(const string &);
}

void Info::Init()
{
	sInfo = new Scrollpad(0, main_start_y, COLS, main_height, "", Config.main_color, brNone);
	sInfo->SetTimeout(ncmpcpp_window_timeout);
}

void Info::Resize()
{
	sInfo->Resize(COLS, main_height);
}

void Info::GetSong()
{
	if (wCurrent == sInfo)
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
			TagEditor::Refresh();
		}
#		endif // HAVE_TAGLIB_H
	}
	else if (
	    (wCurrent == myPlaylist->Main() && !myPlaylist->Main()->Empty())
	||  (wCurrent == myBrowser->Main() && myBrowser->Main()->Current().type == MPD::itSong)
	||  (wCurrent == mySearcher->Main() && !mySearcher->Main()->Current().first)
	||  (wCurrent == myLibrary->Songs && !myLibrary->Songs->Empty())
	||  (wCurrent == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
#	ifdef HAVE_TAGLIB_H
	||  (wCurrent == mEditorTags && !mEditorTags->Empty())
#	endif // HAVE_TAGLIB_H
		)
	{
		MPD::Song *s = 0;
		size_t id = ((Menu<MPD::Song> *)wCurrent)->Choice();
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
				s = &mEditorTags->at(id);
				break;
#			endif // HAVE_TAGLIB_H
			default:
				break;
		}
		wPrev = wCurrent;
		wCurrent = sInfo;
		prev_screen = current_screen;
		current_screen = csInfo;
		redraw_header = 1;
		info_title = "Song info";
		sInfo->Clear();
		GetSongInfo(*s, *sInfo);
		sInfo->Flush();
		sInfo->Hide();
	}
}

#ifdef HAVE_CURL_CURL_H
bool Info::Ready()
{
	if (!artist_info_ready)
		return false;
	pthread_join(artist_info_downloader, NULL);
	sInfo->Flush();
	artist_info_downloader = 0;
	artist_info_ready = 0;
	return true;
}

void Info::GetArtist()
{
	if (wCurrent == sInfo)
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
			TagEditor::Refresh();
		}
#		endif // HAVE_TAGLIB_H
	}
	else if (
	    (wCurrent == myPlaylist->Main() && !myPlaylist->Main()->Empty())
	||  (wCurrent == myBrowser->Main() && myBrowser->Main()->Current().type == MPD::itSong)
	||  (wCurrent == mySearcher->Main() && !mySearcher->Main()->Current().first)
	||  (wCurrent == myLibrary->Artists && !myLibrary->Artists->Empty())
	||  (wCurrent == myLibrary->Songs && !myLibrary->Songs->Empty())
	||  (wCurrent == myPlaylistEditor->Content && !myPlaylistEditor->Content->Empty())
#	ifdef HAVE_TAGLIB_H
	||  (wCurrent == mEditorTags && !mEditorTags->Empty())
#	endif // HAVE_TAGLIB_H
		)
	{
		if (artist_info_downloader)
		{
			ShowMessage("Artist's info is being downloaded...");
			return;
		}
		
		string *artist = new string();
		int id = ((Menu<MPD::Song> *)wCurrent)->Choice();
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
#					ifdef HAVE_TAGLIB_H
			case csTagEditor:
				*artist = mEditorTags->at(id).GetArtist();
				break;
#					endif // HAVE_TAGLIB_H
			default:
				break;
		}
		if (!artist->empty())
		{
			wPrev = wCurrent;
			wCurrent = sInfo;
			prev_screen = current_screen;
			current_screen = csInfo;
			redraw_header = 1;
			info_title = "Artist's info - " + *artist;
			sInfo->Clear();
			sInfo->WriteXY(0, 0, 0, "Fetching artist's info...");
			sInfo->Refresh();
			if (!artist_info_downloader)
			{
				pthread_create(&artist_info_downloader, NULL, GetArtistInfo, artist);
			}
		}
		else
			delete artist;
	}
}
#endif // HVAE_CURL_CURL_H

namespace {
#ifdef HAVE_CURL_CURL_H
void *GetArtistInfo(void *ptr)
{
	string *strptr = static_cast<string *>(ptr);
	string artist = *strptr;
	delete strptr;
	
	locale_to_utf(artist);
	
	string filename = artist + ".txt";
	ToLower(filename);
	EscapeUnallowedChars(filename);
	
	const string fullpath = artists_folder + "/" + filename;
	mkdir(artists_folder.c_str(), 0755);
	
	string result;
	std::ifstream input(fullpath.c_str());
	
	if (input.is_open())
	{
		bool first = 1;
		string line;
		while (getline(input, line))
		{
			if (!first)
				*sInfo << "\n";
			utf_to_locale(line);
			*sInfo << line;
			first = 0;
		}
		input.close();
		sInfo->SetFormatting(fmtBold, "\n\nSimilar artists:\n", fmtBoldEnd, 0);
		sInfo->SetFormatting(Config.color2, "\n * ", clEnd);
		artist_info_ready = 1;
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
		*sInfo << "Error while fetching artist's info: " << curl_easy_strerror(code);
		artist_info_ready = 1;
		pthread_exit(NULL);
	}
	
	size_t a, b;
	bool save = 1;
	
	a = result.find("status=\"failed\"");
	
	if (a != string::npos)
	{
		EscapeHtml(result);
		*sInfo << "Last.fm returned an error message: " << result;
		artist_info_ready = 1;
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
	*sInfo << result;
	
	if (save)
		filebuffer << "\n\nSimilar artists:\n";
	*sInfo << fmtBold << "\n\nSimilar artists:\n" << fmtBoldEnd;
	for (size_t i = 1; i < similar.size(); i++)
	{
		if (save)
			filebuffer << "\n * " << similar[i] << " (" << urls[i] << ")";
		utf_to_locale(similar[i]);
		utf_to_locale(urls[i]);
		*sInfo << "\n" << Config.color2 << " * " << clEnd << similar[i] << " (" << urls[i] << ")";
	}
	
	if (save)
		filebuffer << "\n\n" << urls.front();
	utf_to_locale(urls.front());
	*sInfo << "\n\n" << urls.front();
	
	if (save)
	{
		std::ofstream output(fullpath.c_str());
		if (output.is_open())
		{
			output << filebuffer.Str();
			output.close();
		}
	}
	artist_info_ready = 1;
	pthread_exit(NULL);
}
#endif // HVAE_CURL_CURL_H

void GetSongInfo(MPD::Song &s, Scrollpad &info)
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
	
	info << fmtBold << Config.color1 << "Filename: " << fmtBoldEnd << Config.color2 << s.GetName() << "\n" << clEnd;
	info << fmtBold << "Directory: " << fmtBoldEnd << Config.color2 << ShowTagInInfoScreen(s.GetDirectory()) << "\n\n" << clEnd;
	info << fmtBold << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << "\n" << clEnd;
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		info << fmtBold << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps\n" << clEnd;
		info << fmtBold << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz\n" << clEnd;
		info << fmtBold << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << "\n" << clDefault;
	}
	else
		info << clDefault;
#	endif // HAVE_TAGLIB_H
	info << fmtBold << "\nTitle: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetTitle());
	info << fmtBold << "\nArtist: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetArtist());
	info << fmtBold << "\nAlbum: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetAlbum());
	info << fmtBold << "\nYear: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetYear());
	info << fmtBold << "\nTrack: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetTrack());
	info << fmtBold << "\nGenre: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetGenre());
	info << fmtBold << "\nComposer: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetComposer());
	info << fmtBold << "\nPerformer: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetPerformer());
	info << fmtBold << "\nDisc: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetDisc());
	info << fmtBold << "\nComment: " << fmtBoldEnd << ShowTagInInfoScreen(s.GetComment());
}

const basic_buffer<my_char_t> &ShowTagInInfoScreen(const string &tag)
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
	return ShowTag(tag);
#	endif
}
}
