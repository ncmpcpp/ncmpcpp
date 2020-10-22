/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#include <cstdlib>
#include <cstring>
#include <numeric>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>

#include "charset.h"
#include "lyrics_fetcher.h"
#include "utility/html.h"
#include "utility/string.h"

std::istream &operator>>(std::istream &is, LyricsFetcher_ &fetcher)
{
	std::string s;
	is >> s;
	if (s == "azlyrics")
		fetcher = std::make_unique<AzLyricsFetcher>();
	else if (s == "genius")
		fetcher = std::make_unique<GeniusFetcher>();
	else if (s == "sing365")
		fetcher = std::make_unique<Sing365Fetcher>();
	else if (s == "lyricsmania")
		fetcher = std::make_unique<LyricsmaniaFetcher>();
	else if (s == "metrolyrics")
		fetcher = std::make_unique<MetrolyricsFetcher>();
	else if (s == "justsomelyrics")
		fetcher = std::make_unique<JustSomeLyricsFetcher>();
	else if (s == "jahlyrics")
		fetcher = std::make_unique<JahLyricsFetcher>();
	else if (s == "plyrics")
		fetcher = std::make_unique<PLyricsFetcher>();
	else if (s == "tekstowo")
		fetcher = std::make_unique<TekstowoFetcher>();
	else if (s == "zeneszoveg")
		fetcher = std::make_unique<ZeneszovegFetcher>();
	else if (s == "internet")
		fetcher = std::make_unique<InternetLyricsFetcher>();
	else
		is.setstate(std::ios::failbit);
	return is;
}

const char LyricsFetcher::msgNotFound[] = "Not found";

LyricsFetcher::Result LyricsFetcher::fetch(const std::string &artist,
                                           const std::string &title)
{
	Result result;
	result.first = false;
	
	std::string url = urlTemplate();
	boost::replace_all(url, "%artist%", Curl::escape(artist));
	boost::replace_all(url, "%title%", Curl::escape(title));
	
	std::string data;
	CURLcode code = Curl::perform(data, url, "", true);
	
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

std::vector<std::string> LyricsFetcher::getContent(const char *regex_,
                                                   const std::string &data)
{
	std::vector<std::string> result;
	boost::regex rx(regex_);
	auto first = boost::sregex_iterator(data.begin(), data.end(), rx);
	auto last = boost::sregex_iterator();
	for (; first != last; ++first)
	{
		std::string content;
		for (size_t i = 1; i < first->size(); ++i)
			content += first->str(i);
		result.push_back(std::move(content));
	}
	return result;
}

void LyricsFetcher::postProcess(std::string &data) const
{
	data = unescapeHtmlUtf8(data);
	stripHtmlTags(data);
	// Remove indentation from each line and collapse multiple newlines into one.
	std::vector<std::string> lines;
	boost::split(lines, data, boost::is_any_of("\n"));
	for (auto &line : lines)
		boost::trim(line);
	std::unique(lines.begin(), lines.end(), [](std::string &a, std::string &b) {
		return a.empty() && b.empty();
	});
	data = boost::algorithm::join(lines, "\n");
	boost::trim(data);
}

/**********************************************************************/

LyricsFetcher::Result GoogleLyricsFetcher::fetch(const std::string &artist,
                                                 const std::string &title)
{
	Result result;
	result.first = false;
	
	std::string search_str;
	if (siteKeyword() != nullptr)
	{
		search_str += "site:";
		search_str += Curl::escape(siteKeyword());
	}
	else
		search_str = "lyrics";
	search_str += "+";
	search_str += Curl::escape(artist);
	search_str += "+";
	search_str += Curl::escape(title);
	
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

	auto urls = getContent("<A HREF=\"http://www.google.com/url\\?q=(.*?)\">here</A>", data);

	if (urls.empty() || !isURLOk(urls[0]))
	{
		result.second = msgNotFound;
		return result;
	}

	data = unescapeHtmlUtf8(urls[0]);

	URL = data.c_str();

	std::string data2;
	CURLcode code2 = Curl::perform(data2, URL, google_url, true);

	if (code2 != CURLE_OK) {
		result.second = curl_easy_strerror(code2);
		return result;
	}

	{
		auto content = getContent(regex(), data2);
		auto lyricsStr = std::accumulate(content.begin(), content.end(), std::string(""));
		postProcess(lyricsStr);
		result.first = true;
		result.second = lyricsStr;
	}

	return result;

}

bool GoogleLyricsFetcher::isURLOk(const std::string &url)
{
	return url.find(siteKeyword()) != std::string::npos;
}

/**********************************************************************/

bool MetrolyricsFetcher::isURLOk(const std::string &url)
{
	// it sometimes return link to sitemap.xml, which is huge so we need to discard it
	return GoogleLyricsFetcher::isURLOk(url) && url.find("sitemap") == std::string::npos;
}

/**********************************************************************/

LyricsFetcher::Result InternetLyricsFetcher::fetch(const std::string &artist,
                                                   const std::string &title)
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
