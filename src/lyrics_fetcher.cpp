/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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

#include "config.h"
#include "curl_handle.h"

#ifdef HAVE_CURL_CURL_H

#include <cstdlib>
#include <cstring>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>

#include "charset.h"
#include "lyrics_fetcher.h"
#include "utility/html.h"
#include "utility/string.h"

LyricsFetcher *lyricsPlugins[] =
{
	new LyricwikiFetcher(),
	new AzLyricsFetcher(),
	new Sing365Fetcher(),
	new LyricsmaniaFetcher(),
	new MetrolyricsFetcher(),
	new JustSomeLyricsFetcher(),
	new InternetLyricsFetcher(),
	0
};

const char LyricsFetcher::msgNotFound[] = "Not found";

LyricsFetcher::Result LyricsFetcher::fetch(const std::string &artist, const std::string &title)
{
	Result result;
	result.first = false;
	
	std::string url = urlTemplate();
	boost::replace_all(url, "%artist%", artist);
	boost::replace_all(url, "%title%", title);
	
	std::string data;
	CURLcode code = Curl::perform(data, url);
	
	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}
	
	auto lyrics = getContent(regex(), data);
	
	if (lyrics.empty() || notLyrics(data))
	{
		result.second = msgNotFound;
		return result;
	}
	
	data.clear();
	for (auto it = lyrics.begin(); it != lyrics.end(); ++it)
	{
		postProcess(*it);
		if (!it->empty())
		{
			data += *it;
			if (it != lyrics.end()-1)
				data += "\n\n----------\n\n";
		}
	}
	
	result.second = data;
	result.first = true;
	return result;
}

std::vector<std::string> LyricsFetcher::getContent(const char *regex_, const std::string &data)
{
	std::vector<std::string> result;
	boost::regex rx(regex_);
	auto first = boost::sregex_iterator(data.begin(), data.end(), rx);
	auto last = boost::sregex_iterator();
	for (; first != last; ++first)
		result.push_back(first->str(1));
	return result;
}

void LyricsFetcher::postProcess(std::string &data)
{
	stripHtmlTags(data);
	boost::trim(data);
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
		
		auto lyrics = getContent("<div class='lyricbox'><script>.*?</script>(.*?)<!--", data);
		
		if (lyrics.empty())
		{
			result.second = msgNotFound;
			return result;
		}
		std::transform(lyrics.begin(), lyrics.end(), lyrics.begin(), unescapeHtmlUtf8);
		bool license_restriction = std::any_of(lyrics.begin(), lyrics.end(), [](const std::string &s) {
			return s.find("Unfortunately, we are not licensed to display the full lyrics for this song at the moment.") != std::string::npos;
		});
		if (license_restriction)
		{
			result.second = "Licence restriction";
			return result;
		}
		
		data.clear();
		for (auto it = lyrics.begin(); it != lyrics.end(); ++it)
		{
			boost::replace_all(*it, "<br />", "\n");
			stripHtmlTags(*it);
			boost::trim(*it);
			if (!it->empty())
			{
				data += *it;
				if (it != lyrics.end()-1)
					data += "\n\n----------\n\n";
			}
		}
		
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
	search_str += "+%2B";
	search_str += siteKeyword();
	
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
	
	auto urls = getContent("<A HREF=\"(.*?)\">here</A>", data);
	
	if (urls.empty() || !isURLOk(urls[0]))
	{
		result.second = msgNotFound;
		return result;
	}
	
	data = unescapeHtmlUtf8(urls[0]);
	
	URL = data.c_str();
	return LyricsFetcher::fetch("", "");
}

bool GoogleLyricsFetcher::isURLOk(const std::string &url)
{
	return url.find(siteKeyword()) != std::string::npos;
}

/**********************************************************************/

void Sing365Fetcher::postProcess(std::string &data)
{
	// throw away ad
	data = boost::regex_replace(data, boost::regex("<div.*</div>"), "");
	LyricsFetcher::postProcess(data);
}

/**********************************************************************/

void MetrolyricsFetcher::postProcess(std::string &data)
{
	// some of lyrics have both \n chars and <br />, html tags
	// are always present whereas \n chars are not, so we need to
	// throw them away to avoid having line breaks doubled.
	boost::replace_all(data, "&#10;", "");
	boost::replace_all(data, "<br />", "\n");
	data = unescapeHtmlUtf8(data);
	LyricsFetcher::postProcess(data);
}

bool MetrolyricsFetcher::isURLOk(const std::string &url)
{
	// it sometimes return link to sitemap.xml, which is huge so we need to discard it
	return GoogleLyricsFetcher::isURLOk(url) && url.find("sitemap") == std::string::npos;
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

