/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <sys/stat.h>
#include "helpers.h"
#include "lyrics.h"
#include "settings.h"
#include "song.h"

extern ncmpcpp_config Config;

const string artists_folder = home_folder + "/" + ".ncmpcpp/artists";
const string lyrics_folder = home_folder + "/" + ".lyrics";

#ifdef HAVE_CURL_CURL_H
pthread_mutex_t curl = PTHREAD_MUTEX_INITIALIZER;
bool data_ready = 0;
#endif

namespace
{
	size_t write_data(char *buffer, size_t size, size_t nmemb, string data)
	{
		int result = 0;
		if (buffer)
		{
			data += buffer;
			result = size*nmemb;
		}
		return result;
	}

	void EscapeHtml(string &str)
	{
		for (int i = str.find("<"); i != string::npos; i = str.find("<"))
		{
			int j = str.find(">")+1;
			str.replace(i, j-i, "");
		}
		for (int i = str.find("&#039;"); i != string::npos; i = str.find("&#039;"))
			str.replace(i, 6, "'");
		for (int i = str.find("&quot;"); i != string::npos; i = str.find("&quot;"))
			str.replace(i, 6, "\"");
		for (int i = str.find("&amp;"); i != string::npos; i = str.find("&amp;"))
			str.replace(i, 5, "&");
	}
}

#ifdef HAVE_CURL_CURL_H
void * GetArtistInfo(void *ptr)
{
	string *strptr = static_cast<string *>(ptr);
	string artist = *strptr;
	delete strptr;
	
	string filename = artist + ".txt";
	ToLower(filename);
	EscapeUnallowedChars(filename);
	
	const string fullpath = artists_folder + "/" + filename;
	mkdir(artists_folder.c_str(), 0755);
	
	string *result = new string();
	std::ifstream input(fullpath.c_str());
	
	if (input.is_open())
	{
		string line;
		while (getline(input, line))
			*result += line + "\n";
		input.close();
		*result = result->substr(0, result->length()-1);
		data_ready = 1;
		pthread_exit(result);
	}
	
	for (string::iterator it = artist.begin(); it != artist.end(); it++)
		if (*it == ' ')
			*it = '+';
	
	CURLcode code;
	
	string url = "http://ws.audioscrobbler.com/2.0/?method=artist.getinfo&artist=" + artist + "&api_key=d94e5b6e26469a2d1ffae8ef20131b79";
	
	pthread_mutex_lock(&curl);
	CURL *info = curl_easy_init();
	curl_easy_setopt(info, CURLOPT_URL, url.c_str());
	curl_easy_setopt(info, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(info, CURLOPT_WRITEDATA, result);
	curl_easy_setopt(info, CURLOPT_CONNECTTIMEOUT, 10);
	code = curl_easy_perform(info);
	curl_easy_cleanup(info);
	pthread_mutex_unlock(&curl);
	
	if (code != CURLE_OK)
	{
		*result = "Error while fetching artist's info: " + string(curl_easy_strerror(code));
		data_ready = 1;
		pthread_exit(result);
	}
	
	int a, b;
	bool erase = 0;
	bool save = 1;
	
	a = result->find("status=\"failed\"");
	
	if (a != string::npos)
	{
		EscapeHtml(*result);
		*result = "Last.fm returned an error message: " + *result;
		data_ready = 1;
		pthread_exit(result);
	}
	
	vector<string> similar;
	for (int i = result->find("<name>"); i != string::npos; i = result->find("<name>"))
	{
		(*result)[i] = '.';
		int j = result->find("</name>");
		(*result)[j] = '.';
		i += 6;
		similar.push_back(result->substr(i, j-i));
		EscapeHtml(similar.back());
	}
	vector<string> urls;
	for (int i = result->find("<url>"); i != string::npos; i = result->find("<url>"))
	{
		(*result)[i] = '.';
		int j = result->find("</url>");
		(*result)[j] = '.';
		i += 5;
		urls.push_back(result->substr(i, j-i));
	}
	
	a = result->find("<content>")+9;
	b = result->find("</content>");
	
	if (a == b)
	{
		*result = "No description available for this artist.";
		save = 0;
	}
	else
	{
		a += 9; // for <![CDATA[
		b -= 3; // for ]]>
		*result = result->substr(a, b-a);
	}
	
	EscapeHtml(*result);
	for (int i = 0; i < result->length(); i++)
	{
		if (erase)
		{
			result->erase(result->begin()+i);
			erase = 0;
		}
		if ((*result)[i] == 13)
		{
			(*result)[i] = '\n';
			erase = 1;
		}
		else if ((*result)[i] == '\t')
			(*result)[i] = ' ';
	}
	
	int i = result->length();
	if (!isgraph((*result)[i-1]))
	{
		while (!isgraph((*result)[--i]));
		*result = result->substr(0, i+1);
	}
	
	*result += "\n\n[.b]Similar artists:[/b]\n";
	for (int i = 1; i < similar.size(); i++)
		*result += "\n [." + Config.color2 + "]*[/" + Config.color2 + "] " + similar[i] + " (" + urls[i] + ")";
	
	*result += "\n\n" + urls.front();
	
	if (save)
	{
		std::ofstream output(fullpath.c_str());
		if (output.is_open())
		{
			output << *result;
			output.close();
		}
	}
	
	data_ready = 1;
	pthread_exit(result);
}
#endif // HAVE_CURL_CURL_H

void * GetLyrics(void *song)
{
	string artist = static_cast<Song *>(song)->GetArtist();
	string title = static_cast<Song *>(song)->GetTitle();
	
	const string filename = artist + " - " + title + ".txt";
	const string fullpath = lyrics_folder + "/" + filename;
	mkdir(lyrics_folder.c_str(), 0755);
	
	string *result = new string();
	
	std::ifstream input(fullpath.c_str());
	
	if (input.is_open())
	{
		string line;
		while (getline(input, line))
			*result += line + "\n";
		input.close();
		*result = result->substr(0, result->length()-1);
#		ifdef HAVE_CURL_CURL_H
		data_ready = 1;
		pthread_exit(result);
#		endif
	}
	
#	ifdef HAVE_CURL_CURL_H
	for (string::iterator it = artist.begin(); it != artist.end(); it++)
		if (*it == ' ')
			*it = '+';

	for (string::iterator it = title.begin(); it != title.end(); it++)
		if (*it == ' ')
			*it = '+';
	
	CURLcode code;
	
	string url = "http://lyricwiki.org/api.php?artist=" + artist + "&song=" + title + "&fmt=xml";
	
	pthread_mutex_lock(&curl);
	CURL *lyrics = curl_easy_init();
	curl_easy_setopt(lyrics, CURLOPT_URL, url.c_str());
	curl_easy_setopt(lyrics, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(lyrics, CURLOPT_WRITEDATA, result);
	curl_easy_setopt(lyrics, CURLOPT_CONNECTTIMEOUT, 10);
	code = curl_easy_perform(lyrics);
	curl_easy_cleanup(lyrics);
	pthread_mutex_unlock(&curl);
	
	if (code != CURLE_OK)
	{
		*result = "Error while fetching lyrics: " + string(curl_easy_strerror(code));
		data_ready = 1;
		pthread_exit(result);
	}
	
	int a, b;
	a = result->find("<lyrics>")+8;
	b = result->find("</lyrics>");
	
	*result = result->substr(a, b-a);
	
	if (*result == "Not found")
	{
		data_ready = 1;
		pthread_exit(result);
	}
	
	for (int i = result->find("&lt;"); i != string::npos; i = result->find("&lt;"))
		result->replace(i, 4, "<");
	for (int i = result->find("&gt;"); i != string::npos; i = result->find("&gt;"))
		result->replace(i, 4, ">");
	
	EscapeHtml(*result);
	
	std::ofstream output(fullpath.c_str());
	if (output.is_open())
	{
		output << *result;
		output.close();
	}
	
	data_ready = 1;
	pthread_exit(result);
#	else
	else
		*result = "Local lyrics not found. As ncmpcpp has been compiled without curl support, you can put appropriate lyrics into ~/.lyrics directory (file syntax is \"ARTIST - TITLE.txt\") or recompile ncmpcpp with curl support.";
	return result;
#	endif
}

