/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/regex.hpp>

#ifdef HAVE_TAGLIB_H
#include <fileref.h>
#include <tpropertymap.h>
#endif // HAVE_TAGLIB_H

#include "charset.h"
#include "curl_handle.h"
#include "lyrics_fetcher.h"
#include "settings.h"
#include "utility/html.h"
#include "utility/string.h"

namespace
{
std::string letrasSlug(const std::string &input)
{
	struct Accent { const char *from; char to; };
	static const Accent accents[] = {
		{ "á", 'a' }, { "à", 'a' }, { "ä", 'a' }, { "â", 'a' }, { "ã", 'a' },
		{ "é", 'e' }, { "è", 'e' }, { "ë", 'e' }, { "ê", 'e' },
		{ "í", 'i' }, { "ì", 'i' }, { "ï", 'i' }, { "î", 'i' },
		{ "ó", 'o' }, { "ò", 'o' }, { "ö", 'o' }, { "ô", 'o' }, { "õ", 'o' },
		{ "ú", 'u' }, { "ù", 'u' }, { "ü", 'u' }, { "û", 'u' },
		{ "ñ", 'n' }, { "ç", 'c' }, { "ÿ", 'y' }, { "œ", 'o' }
	};
	std::string result;
	result.reserve(input.size());
	size_t i = 0;
	while (i < input.size())
	{
		unsigned char c = input[i];
		if (c < 0x80)
		{
			char lc = std::tolower(c);
			if ((lc >= 'a' && lc <= 'z') || (lc >= '0' && lc <= '9'))
				result += lc;
			else if (c == '&')
				result += "and";
			else if (!result.empty() && result.back() != '-')
				result += '-';
			++i;
		}
		else
		{
			bool matched = false;
			for (const Accent &a : accents)
			{
				size_t len = std::strlen(a.from);
				if (i + len <= input.size() && input.compare(i, len, a.from) == 0)
				{
					result += a.to;
					i += len;
					matched = true;
					break;
				}
			}
			if (!matched)
			{
				if (!result.empty() && result.back() != '-')
					result += '-';
				++i;
			}
		}
	}
while (!result.empty() && result.back() == '-')
		result.pop_back();
	return result;
}

const char *letrasPartTokens[] = { "pt", "pts", "part", "parts", "parte", "partes", "par" };

const char *letrasRomanNums[] = { "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix", "x", "xi", "xii" };

bool isLetrasPartToken(const std::string &s)
{
	for (const char *t : letrasPartTokens)
		if (s == t)
			return true;
	return false;
}

int letrasRomanValue(const std::string &s)
{
	for (size_t i = 0; i < 12; ++i)
		if (s == letrasRomanNums[i])
			return static_cast<int>(i)+1;
	return 0;
}

std::vector<std::string> letrasTokens(const std::string &s)
{
	std::string slug = letrasSlug(s);
	std::vector<std::string> tokens;
	boost::split(tokens, slug, boost::is_any_of("-"), boost::token_compress_off);
	return tokens;
}

// Normaliza el título para comparar canónico (letras.com) vs tag (biblioteca): "part 1" == "pt. 1" == "parte i".
std::string letrasNormTitle(const std::string &s)
{
	std::vector<std::string> tokens = letrasTokens(s);
	std::string result;
	for (auto &t : tokens)
	{
		if (isLetrasPartToken(t))
			t = "p";
		else if (int v = letrasRomanValue(t))
			t = std::to_string(v);
		if (!result.empty())
			result += '-';
		result += t;
	}
	return result;
}

// Tokens a usar en la búsqueda Solr: descarta "part N"/"pt. II" (matan el ranking).
std::vector<std::string> letrasQueryTokens(const std::string &s)
{
	std::vector<std::string> tokens = letrasTokens(s);
	tokens.erase(std::remove_if(tokens.begin(), tokens.end(), [](const std::string &t) {
		return isLetrasPartToken(t) || letrasRomanValue(t) != 0
		       || (t.find_first_not_of("0123456789") == std::string::npos);
	}), tokens.end());
	return tokens;
}

}
std::istream &operator>>(std::istream &is, LyricsFetcher_ &fetcher)
{
	std::string s;
	is >> s;
	if (s == "justsomelyrics")
		fetcher = std::make_unique<JustSomeLyricsFetcher>();
	else if (s == "jahlyrics")
		fetcher = std::make_unique<JahLyricsFetcher>();
	else if (s == "plyrics")
		fetcher = std::make_unique<PLyricsFetcher>();
	else if (s == "tekstowo")
		fetcher = std::make_unique<TekstowoFetcher>();
	else if (s == "zeneszoveg")
		fetcher = std::make_unique<ZeneszovegFetcher>();
	else if (s == "letras")
		fetcher = std::make_unique<LetrasFetcher>();
	else if (s == "internet")
		fetcher = std::make_unique<InternetLyricsFetcher>();
#ifdef HAVE_TAGLIB_H
	else if (s == "tags")
		fetcher = std::make_unique<TagsLyricsFetcher>();
#endif // HAVE_TAGLIB_H
	else
		is.setstate(std::ios::failbit);
	return is;
}

const char LyricsFetcher::msgNotFound[] = "Not found";

LyricsFetcher::Result LyricsFetcher::fetch(const std::string &artist,
                                           const std::string &title,
                                           [[maybe_unused]] const MPD::Song &song)
{
	Result result;
	result.first = false;
	
	std::string url = buildURL(artist, title);
	if (url.empty())
	{
		result.second = msgNotFound;
		return result;
	}
	
	std::string data;
	CURLcode code = Curl::perform(data, url, "", true);
	
	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}

	auto lyrics = getContent(regex(), data);

	//std::cerr << "URL: " << url << "\n";
	//std::cerr << "Data: " << data << "\n";

	if (lyrics.empty() || notLyrics(data))
	{
		//std::cerr << "Empty: " << lyrics.empty() << "\n";
		//std::cerr << "Not Lyrics: " << notLyrics(data) << "\n";
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
			if (it != lyrics.end() - 1)
				data += "\n\n";
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
	boost::regex rx(regex_, boost::regex::perl);
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

std::string LyricsFetcher::buildURL(const std::string &artist, const std::string &title) const
{
	std::string url = urlTemplate();
	boost::replace_all(url, "%artist%", Curl::escape(artist));
	boost::replace_all(url, "%title%", Curl::escape(title));
	return url;
}

std::string LetrasFetcher::buildURL(const std::string &artist, const std::string &title) const
{
	// Las URLs "bonitas" de letras.com mapean mal las canciones con varias
	// partes (part-1 -> PT.2, part-2 -> PT.1). Se resuelve la página canónica
	// a través de la búsqueda Solr del propio sitio y se empareja el título
	// normalizado (part/pt/romanos) contra el tag de la biblioteca.
	std::vector<std::string> q = letrasTokens(artist);
	std::vector<std::string> tt = letrasQueryTokens(title);
	q.insert(q.end(), tt.begin(), tt.end());
	if (q.empty())
		return "";

	std::string searchUrl = "https://solr.sscdn.co/letras/m1/?q=" + boost::algorithm::join(q, "+") + "&wt=json";
	std::string data;
	if (Curl::perform(data, searchUrl, "") != CURLE_OK)
		return "";

	size_t begin = data.find('(');
	size_t end = data.rfind(')');
	if (begin == std::string::npos || end == std::string::npos || end <= begin)
		return "";

	boost::property_tree::ptree root;
	try
	{
		std::istringstream is(data.substr(begin+1, end-begin-1));
		boost::property_tree::read_json(is, root);
	}
	catch (...)
	{
		return "";
	}

	const std::string expectedArtist = letrasSlug(artist);
	const std::string expectedTitle = letrasNormTitle(title);
	std::string fallback;

	if (auto docs = root.get_child_optional("response.docs"))
	{
		for (const auto &kv : *docs)
		{
			const boost::property_tree::ptree &doc = kv.second;
			std::string art = doc.get<std::string>("art", "");
			std::string txt = doc.get<std::string>("txt", "");
			std::string url = doc.get<std::string>("url", "");
			std::string dns = doc.get<std::string>("dns", "");
			if (art.empty() || txt.empty() || dns.empty() || url.empty())
				continue;
			if (letrasSlug(art) != expectedArtist)
				continue;
			if (fallback.empty())
				fallback = "https://www.letras.com/" + dns + "/" + url + "/";
			if (letrasNormTitle(txt) == expectedTitle)
				return "https://www.letras.com/" + dns + "/" + url + "/";
		}
	}

	return fallback;
}

void LyricsFetcher::postProcess(std::string &data) const
{
	data = unescapeHtmlUtf8(data);
	stripHtmlTags(data);
	// Remove indentation from each line and collapse multiple newlines into one.
	std::vector<std::string> lines;
	boost::split(lines, data, boost::is_any_of("\r\n"));
	for (auto &line : lines)
		boost::trim(line);
	auto last = std::unique(
		lines.begin(),
		lines.end(),
		[](std::string &a, std::string &b) { return a.empty() && b.empty(); });
	lines.erase(last, lines.end());
	data = boost::algorithm::join(lines, "\n");
	boost::trim(data);
}

/**********************************************************************/

LyricsFetcher::Result GoogleLyricsFetcher::fetch(const std::string &artist,
                                                 const std::string &title,
                                                 const MPD::Song &song)
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
	return LyricsFetcher::fetch("", "", song);
}

bool GoogleLyricsFetcher::isURLOk(const std::string &url)
{
	return url.find(siteKeyword()) != std::string::npos;
}

/**********************************************************************/

LyricsFetcher::Result InternetLyricsFetcher::fetch(const std::string &artist,
                                                   const std::string &title,
                                                   const MPD::Song &song)
{
	GoogleLyricsFetcher::fetch(artist, title, song);
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

#ifdef HAVE_TAGLIB_H
LyricsFetcher::Result TagsLyricsFetcher::fetch([[maybe_unused]] const std::string &artist,
                                               [[maybe_unused]] const std::string &title,
                                               const MPD::Song &song)
{
	LyricsFetcher::Result result;
	result.first = false;

	std::string path;
	if (song.isFromDatabase())
		path += Config.mpd_music_dir;
	path += song.getURI();

	TagLib::FileRef f(path.c_str());
	if (f.isNull())
	{
		result.second = "Could not open file";
		return result;
	}

	TagLib::PropertyMap properties = f.file()->properties();

	if (properties.contains("LYRICS"))
	{
		result.first = true;
		result.second = properties["LYRICS"].toString("\n\n").to8Bit(true);
	}
	else if (properties.contains("UNSYNCEDLYRICS"))
	{
		result.first = true;
		result.second = properties["UNSYNCEDLYRICS"].toString("\n\n").to8Bit(true);
	}
	else
		result.second = "No lyrics in tags";

	return result;
}
#endif // HAVE_TAGLIB_H
