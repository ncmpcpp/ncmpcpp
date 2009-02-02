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
#include "helpers.h"
#include "lyrics.h"
#include "settings.h"
#include "song.h"

extern Window *wCurrent;
extern Scrollpad *sLyrics;
extern Scrollpad *sInfo;

const string artists_folder = home_folder + "/.ncmpcpp/artists";
const string lyrics_folder = home_folder + "/.lyrics";

#ifdef HAVE_CURL_CURL_H
extern pthread_t lyrics_downloader;
extern pthread_t artist_info_downloader;
extern bool lyrics_ready;
extern bool artist_info_ready;
pthread_mutex_t curl = PTHREAD_MUTEX_INITIALIZER;

namespace
{
	size_t write_data(char *buffer, size_t size, size_t nmemb, string data)
	{
		size_t result = size * nmemb;
		data.append(buffer, result);
		return result;
	}

	void EscapeHtml(string &s)
	{
		bool erase = 0;
		for (size_t i = s.find("<"); i != string::npos; i = s.find("<"))
		{
			size_t j = s.find(">")+1;
			s.replace(i, j-i, "");
		}
		for (size_t i = s.find("&#039;"); i != string::npos; i = s.find("&#039;"))
			s.replace(i, 6, "'");
		for (size_t i = s.find("&quot;"); i != string::npos; i = s.find("&quot;"))
			s.replace(i, 6, "\"");
		for (size_t i = s.find("&amp;"); i != string::npos; i = s.find("&amp;"))
			s.replace(i, 5, "&");
		for (size_t i = 0; i < s.length(); i++)
		{
			if (erase)
			{
				s.erase(s.begin()+i);
				erase = 0;
			}
			if (s[i] == 13) // ascii code for windows line ending, get rid of this shit
			{
				s[i] = '\n';
				erase = 1;
			}
			else if (s[i] == '\t')
				s[i] = ' ';
		}
	}
	
	void Trim(string &s)
	{
		if (s.empty())
			return;
		
		size_t b = 0;
		size_t e = s.length()-1;
		
		while (!isprint(s[b]))
			b++;
		while (!isprint(s[e]))
			e--;
		e++;
		
		if (b != 0 || e != s.length()-1)
			s = s.substr(b, e-b);
	}
}

void * GetArtistInfo(void *ptr)
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
	
	const LyricsPlugin *ChooseLyricsPlugin(int i)
	{
		switch (i)
		{
			case 1:
				return &lyricwiki;
			case 2:
				return &lyricsplugin;
			default:
				return &lyricwiki;
		}
	}
}

#endif // HAVE_CURL_CURL_H

void *GetLyrics(void *song)
{
	string artist = static_cast<Song *>(song)->GetArtist();
	string title = static_cast<Song *>(song)->GetTitle();
	
	locale_to_utf(artist);
	locale_to_utf(title);
	
	const string fullpath = lyrics_folder + "/" + artist + " - " + title + ".txt";
	
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

