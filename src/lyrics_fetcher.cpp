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

#include "curl_handle.h"

#ifdef HAVE_CURL_CURL_H

#include "conv.h"
#include "lyrics_fetcher.h"

LyricsFetcher *lyricsPlugins[] =
{
	new LyrcComArFetcher(),
	new LyricsflyFetcher(),
	0
};

LyricsFetcher::Result LyricsFetcher::fetch(const std::string &artist, const std::string &title)
{
	Result result;
	result.first = false;
	
	std::string url = getURL();
	Replace(url, "%artist%", artist.c_str());
	Replace(url, "%title%", title.c_str());
	
	std::string data;
	CURLcode code = Curl::perform(url, data);
	
	if (code != CURLE_OK)
	{
		result.second = "Error while fetching lyrics: ";
		result.second += curl_easy_strerror(code);
		return result;
	}
	
	size_t a, b;
	bool parse_failed = false;
	if ((a = data.find(getOpenTag())) != std::string::npos)
	{
		a += strlen(getCloseTag())-1;
		if ((b = data.find(getCloseTag(), a)) != std::string::npos)
			data = data.substr(a, b-a);
		else
			parse_failed = true;
	}
	else
		parse_failed = true;
	
	if (parse_failed || notLyrics(data))
	{
		result.second = "Not found";
		return result;
	}
	
	postProcess(data);
	
	result.second = data;
	result.first = true;
	return result;
}

void LyricsFetcher::postProcess(std::string &data)
{
	EscapeHtml(data);
	Trim(data);
}

void LyricsflyFetcher::postProcess(std::string &data)
{
	Replace(data, "[br]", "");
	Trim(data);
}

#endif // HAVE_CURL_CURL_H

