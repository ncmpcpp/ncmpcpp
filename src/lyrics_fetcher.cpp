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

#include <cstdlib>

#include "conv.h"
#include "lyrics_fetcher.h"

LyricsFetcher *lyricsPlugins[] =
{
	new LyricwikiFetcher(),
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
	
	bool parse_ok = getContent(getOpenTag(), getCloseTag(), data);
	
	if (!parse_ok || notLyrics(data))
	{
		result.second = "Not found";
		return result;
	}
	
	postProcess(data);
	
	result.second = data;
	result.first = true;
	return result;
}

bool LyricsFetcher::getContent(const char *open_tag, const char *close_tag, std::string &data)
{
	size_t a, b;
	if ((a = data.find(open_tag)) != std::string::npos)
	{
		a += strlen(open_tag);
		if ((b = data.find(close_tag, a)) != std::string::npos)
			data = data.substr(a, b-a);
		else
			return false;
	}
	else
		return false;
	return true;
}

void LyricsFetcher::postProcess(std::string &data)
{
	EscapeHtml(data);
	Trim(data);
}

/***********************************************************************/

LyricsFetcher::Result LyricwikiFetcher::fetch(const std::string &artist, const std::string &title)
{
	LyricsFetcher::Result result = LyricsFetcher::fetch(artist, title);
	if (result.first == true)
	{
		result.first = false;
		
		std::string data;
		CURLcode code = Curl::perform(result.second, data);
		
		if (code != CURLE_OK)
		{
			result.second = "Error while fetching lyrics: ";
			result.second += curl_easy_strerror(code);
			return result;
		}
		
		bool parse_ok = getContent("'17'/></a></div>", "<!--", data);
		
		if (!parse_ok)
		{
			result.second = "Not found";
			return result;
		}
		
		Replace(data, "<br />", "\n");
		
		result.second = unescape(data);
		result.first = true;
	}
	return result;
}

bool LyricwikiFetcher::notLyrics(const std::string &data)
{
	return data.find("action=edit") != std::string::npos;
}

std::string LyricwikiFetcher::unescape(const std::string &data)
{
	std::string result;
	for (size_t i = 0, j; i < data.length(); ++i)
	{
		if (data[i] == '&' && data[i+1] == '#' && (j = data.find(';', i)) != std::string::npos)
		{
			int n = atoi(&data.c_str()[i+2]);
			result += char(n);
			i = j;
		}
		else
			result += data[i];
	}
	return result;
}

/***********************************************************************/

void LyricsflyFetcher::postProcess(std::string &data)
{
	Replace(data, "[br]", "");
	Trim(data);
}

#endif // HAVE_CURL_CURL_H

