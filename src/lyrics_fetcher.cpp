/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
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

#include "charset.h"
#include "conv.h"
#include "lyrics_fetcher.h"

LyricsFetcher *lyricsPlugins[] =
{
	new LyricwikiFetcher(),
	new LyricsvipFetcher(),
	new Sing365Fetcher(),
	new LoloLyricsFetcher(),
	new LyriczzFetcher(),
	new SonglyricsFetcher(),
	new LyricsmaniaFetcher(),
	new LyricstimeFetcher(),
	new MetrolyricsFetcher(),
	new JustSomeLyricsFetcher(),
	new LyrcComArFetcher(),
	new InternetLyricsFetcher(),
	0
};

const char LyricsFetcher::msgNotFound[] = "Not found";

LyricsFetcher::Result LyricsFetcher::fetch(const std::string &artist, const std::string &title)
{
	Result result;
	result.first = false;
	
	std::string url = getURL();
	Replace(url, "%artist%", artist.c_str());
	Replace(url, "%title%", title.c_str());
	
	std::string data;
	CURLcode code = Curl::perform(data, url);
	
	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}
	
	bool parse_ok = getContent(getOpenTag(), getCloseTag(), data);
	
	if (!parse_ok || notLyrics(data))
	{
		result.second = msgNotFound;
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
	StripHtmlTags(data);
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
		CURLcode code = Curl::perform(data, result.second);
		
		if (code != CURLE_OK)
		{
			result.second = curl_easy_strerror(code);
			return result;
		}
		
		bool parse_ok = getContent("'17'/></a></div>", "<!--", data);
		
		if (!parse_ok)
		{
			result.second = msgNotFound;
			return result;
		}
		data = unescapeHtmlUtf8(data);
		if (data.find("Unfortunately, we are not licensed to display the full lyrics for this song at the moment.") != std::string::npos)
		{
			result.second = "Licence restriction";
			return result;
		}
		
		Replace(data, "<br />", "\n");
		StripHtmlTags(data);
		Trim(data);
		
		result.second = data;
		result.first = true;
	}
	return result;
}

bool LyricwikiFetcher::notLyrics(const std::string &data)
{
	return data.find("action=edit") != std::string::npos;
}

/**********************************************************************/

LyricsFetcher::Result GoogleLyricsFetcher::fetch(const std::string &artist, const std::string &title)
{
	Result result;
	result.first = false;
	
	std::string search_str = artist;
	search_str += "+";
	search_str += title;
	search_str += "+";
	search_str += getSiteKeyword();
	
	std::string google_url = "http://www.google.com/search?hl=en&ie=UTF-8&oe=UTF-8&q=";
	google_url += search_str;
	google_url += "&btnI=I%27m+Feeling+Lucky";
	
	std::string data;
	CURLcode code = Curl::perform(data, google_url, google_url);
	
	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}
	
	bool found_url = getContent("<A HREF=\"", "\">here</A>", data);
	
	if (!found_url || !isURLOk(data))
	{
		result.second = msgNotFound;
		return result;
	}
	
	data = unescapeHtmlUtf8(data);
	//result.second = data;
	//return result;
	
	URL = data.c_str();
	return LyricsFetcher::fetch("", "");
}

bool GoogleLyricsFetcher::isURLOk(const std::string &url)
{
	return url.find(getSiteKeyword()) != std::string::npos;
}

/**********************************************************************/

bool LyricstimeFetcher::isURLOk(const std::string &url)
{
	// it sometimes returns list of all artists that begin
	// with a given letter, e.g. www.lyricstime.com/A.html, which
	// is 25 chars long, so we want longer.
	return GoogleLyricsFetcher::isURLOk(url) && url.length() > 25;
}

void LyricstimeFetcher::postProcess(std::string &data)
{
	// lyricstime.com uses iso-8859-1 as the encoding
	// so we need to convert obtained lyrics to utf-8
	iconv_convert_from_to("iso-8859-1", "utf-8", data);
	LyricsFetcher::postProcess(data);
}

/**********************************************************************/

void MetrolyricsFetcher::postProcess(std::string &data)
{
	// throw away [ from ... ] info
	size_t i = data.find('['), j = data.find(']');
	if (i != std::string::npos && i != std::string::npos)
		data.replace(i, j-i+1, "");
	// some of lyrics have both \n chars and <br />, html tags
	// are always present whereas \n chars are not, so we need to
	// throw them away to avoid having line breaks doubled.
	Replace(data, "&#10;", "");
	Replace(data, "<br />", "\n");
	data = unescapeHtmlUtf8(data);
	LyricsFetcher::postProcess(data);
}

bool MetrolyricsFetcher::isURLOk(const std::string &url)
{
	// it sometimes return link to sitemap.xml, which is huge so we need to discard it
	return GoogleLyricsFetcher::isURLOk(url) && url.find("sitemap") == std::string::npos;
}

/**********************************************************************/

void LyricsmaniaFetcher::postProcess(std::string &data)
{
	// lyricsmania.com uses iso-8859-1 as the encoding
	// so we need to convert obtained lyrics to utf-8
	iconv_convert_from_to("iso-8859-1", "utf-8", data);
	LyricsFetcher::postProcess(data);
}

/**********************************************************************/

void SonglyricsFetcher::postProcess(std::string &data)
{
	// throw away [ ... lyrics are found on www.songlyrics.com ] info.
	// there is +2 instead of +1 in third line because there is extra
	// space after ] we also want to get rid of
	size_t i = data.find('['), j = data.find(']');
	if (i != std::string::npos && i != std::string::npos)
		data.replace(i, j-i+2, "");
	data = unescapeHtmlUtf8(data);
	LyricsFetcher::postProcess(data);
}

/**********************************************************************/

LyricsFetcher::Result InternetLyricsFetcher::fetch(const std::string &artist, const std::string &title)
{
	GoogleLyricsFetcher::fetch(artist, title);
	LyricsFetcher::Result result;
	result.first = false;
	result.second = "The following site may contain lyrics for this song: ";
	result.second += URL;
	return result;
}

bool InternetLyricsFetcher::isURLOk(const std::string &url)
{
	URL = url;
	return false;
}

#endif // HAVE_CURL_CURL_H

