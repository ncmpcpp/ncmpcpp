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

#include "lyrics.h"
#include <sys/stat.h>

const string lyrics_folder = home_folder + "/" + ".lyrics";

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

string GetLyrics(string artist, string song)
{
	const string filename = artist + " - " + song + ".txt";
	const string fullpath = lyrics_folder + "/" + filename;
	mkdir(lyrics_folder.c_str(), 0755);
	
	string result;
	std::ifstream input(fullpath.c_str());
	
	if (input.is_open())
	{
		string line;
		while (getline(input, line))
			result += line + "\n";
		return result;
	}
	
#	ifdef HAVE_CURL_CURL_H
	for (string::iterator it = artist.begin(); it != artist.end(); it++)
		if (*it == ' ')
			*it = '+';

	for (string::iterator it = song.begin(); it != song.end(); it++)
		if (*it == ' ')
			*it = '+';
	
	CURLcode code;
	
	string url = "http://lyricwiki.org/api.php?artist=" + artist + "&song=" + song + "&fmt=xml";
	
	CURL *lyrics = curl_easy_init();
	curl_easy_setopt(lyrics, CURLOPT_URL, url.c_str());
	curl_easy_setopt(lyrics, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(lyrics, CURLOPT_WRITEDATA, &result);
	curl_easy_setopt(lyrics, CURLOPT_CONNECTTIMEOUT, 10);
	code = curl_easy_perform(lyrics);
	curl_easy_cleanup(lyrics);
	
	if (code != CURLE_OK)
	{
		result = "Error while fetching lyrics: " + string(curl_easy_strerror(code));
		return result;
	}
	
	int a, b;
	a = result.find("<lyrics>")+8;
	b = result.find("</lyrics>");
	
	result = result.substr(a, b-a);
	
	if (result == "Not found")
		return result;
	
	for (int i = result.find("&#039;"); i != string::npos; i = result.find("&#039;"))
		result.replace(i, 6, "'");
	for (int i = result.find("&quot;"); i != string::npos; i = result.find("&quot;"))
		result.replace(i, 6, "\"");
	for (int i = result.find("&lt;"); i != string::npos; i = result.find("&lt;"))
		result.replace(i, 4, "<");
	for (int i = result.find("&gt;"); i != string::npos; i = result.find("&gt;"))
		result.replace(i, 4, ">");
	for (int i = result.find("<lyric>"); i != string::npos; i = result.find("<lyric>"))
		result.replace(i, 7, "");
	for (int i = result.find("</lyric>"); i != string::npos; i = result.find("</lyric>"))
		result.replace(i, 8, "");
	for (int i = result.find("<b>"); i != string::npos; i = result.find("<b>"))
		result.replace(i, 3, "");
	for (int i = result.find("</b>"); i != string::npos; i = result.find("</b>"))
		result.replace(i, 4, "");
	for (int i = result.find("<i>"); i != string::npos; i = result.find("<i>"))
		result.replace(i, 3, "");
	for (int i = result.find("</i>"); i != string::npos; i = result.find("</i>"))
		result.replace(i, 4, "");
	
	std::ofstream output(fullpath.c_str());
	
	if (output.is_open())
	{
		output << result;
		output.close();
	}
#	else
	result = "Local lyrics not found. As ncmpcpp has been compiled without curl support, you can put appropriate lyrics into ~/.lyrics directory (file syntax is \"ARTIST - TITLE.txt\") or recompile ncmpcpp with curl support.";
#	endif
	return result + '\n';
}

