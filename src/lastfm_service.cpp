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

#include "lastfm_service.h"

#ifdef HAVE_CURL_CURL_H

#include <boost/algorithm/string/trim.hpp>
#include <boost/locale/conversion.hpp>
#include <fstream>
#include "charset.h"
#include "curl_handle.h"
#include "settings.h"
#include "utility/html.h"
#include "utility/string.h"

namespace {

const char *apiUrl = "http://ws.audioscrobbler.com/2.0/?api_key=d94e5b6e26469a2d1ffae8ef20131b79&method=";
const char *msgInvalidResponse = "Invalid response";

}

namespace LastFm {

Service::Result Service::fetch()
{
	Result result;
	result.first = false;
	
	std::string url = apiUrl;
	url += methodName();
	for (auto &arg : m_arguments)
	{
		url += "&";
		url += arg.first;
		url += "=";
		url += Curl::escape(arg.second);
	}
	
	std::string data;
	CURLcode code = Curl::perform(data, url);
	
	if (code != CURLE_OK)
		result.second = curl_easy_strerror(code);
	else if (actionFailed(data))
		result.second = msgInvalidResponse;
	else
	{
		result = processData(data);
		
		// if relevant part of data was not found and one of arguments
		// was language, try to fetch it again without that parameter.
		// otherwise just report failure.
		if (!result.first && !m_arguments["lang"].empty())
		{
			m_arguments.erase("lang");
			result = fetch();
		}
	}
	
	return result;
}

bool Service::actionFailed(const std::string &data)
{
	return data.find("status=\"failed\"") != std::string::npos;
}

/***********************************************************************/

bool ArtistInfo::argumentsOk()
{
	return !m_arguments["artist"].empty();
}

void ArtistInfo::beautifyOutput(NC::Scrollpad &w)
{
	w.setProperties(NC::Format::Bold, "\n\nSimilar artists:\n", NC::Format::NoBold, 0);
	w.setProperties(NC::Format::Bold, "\n\nSimilar tags:\n", NC::Format::NoBold, 0);
	w.setProperties(Config.color2, "\n * ", NC::Color::End, 0, boost::regex::literal);
}

Service::Result ArtistInfo::processData(const std::string &data)
{
	size_t a, b;
	Service::Result result;
	result.first = false;
	
	boost::regex rx("<content>(.*?)</content>");
	boost::smatch what;
	if (boost::regex_search(data, what, rx))
	{
		std::string desc = what[1];
		// if there is a description...
		if (desc.length() > 0)
		{
			// ...locate the link to wiki on last.fm...
			rx.assign("<link rel=\"original\" href=\"(.*?)\"");
			if (boost::regex_search(data, what, rx))
			{
				// ...try to get the content of it...
				std::string wiki;
				CURLcode code = Curl::perform(wiki, what[1]);
				
				if (code != CURLE_OK)
				{
					result.second = curl_easy_strerror(code);
					return result;
				}
				else
				{
					// ...and filter it to get the whole description.
					rx.assign("<div id=\"wiki\">(.*?)</div>");
					if (boost::regex_search(wiki, what, rx))
						desc = unescapeHtmlUtf8(what[1]);
				}
			}
			else
			{
				// otherwise, get rid of CDATA wrapper.
				rx.assign("<!\\[CDATA\\[(.*)\\]\\]>");
				desc = boost::regex_replace(desc, rx, "\\1");
			}
			stripHtmlTags(desc);
			boost::trim(desc);
			result.second += desc;
		}
		else
			result.second += "No description available for this artist.";
	}
	else
	{
		result.second = msgInvalidResponse;
		return result;
	}
	
	auto add_similars = [&result](boost::sregex_iterator &it, const boost::sregex_iterator &last) {
		for (; it != last; ++it)
		{
			std::string name = it->str(1);
			std::string url = it->str(2);
			stripHtmlTags(name);
			stripHtmlTags(url);
			result.second += "\n * ";
			result.second += name;
			result.second += " (";
			result.second += url;
			result.second += ")";
		}
	};
	
	a = data.find("<similar>");
	b = data.find("</similar>");
	if (a != std::string::npos && b != std::string::npos)
	{
		rx.assign("<artist>.*?<name>(.*?)</name>.*?<url>(.*?)</url>.*?</artist>");
		auto it = boost::sregex_iterator(data.begin()+a, data.begin()+b, rx);
		auto last = boost::sregex_iterator();
		if (it != last)
			result.second += "\n\nSimilar artists:\n";
		add_similars(it, last);
	}
	
	a = data.find("<tags>");
	b = data.find("</tags>");
	if (a != std::string::npos && b != std::string::npos)
	{
		rx.assign("<tag>.*?<name>(.*?)</name>.*?<url>(.*?)</url>.*?</tag>");
		auto it = boost::sregex_iterator(data.begin()+a, data.begin()+b, rx);
		auto last = boost::sregex_iterator();
		if (it != last)
			result.second += "\n\nSimilar tags:\n";
		add_similars(it, last);
	}
	
	// get artist we look for, it's the one before similar artists
	rx.assign("<name>.*?</name>.*?<url>(.*?)</url>.*?<similar>");
	
	if (boost::regex_search(data, what, rx))
	{
		std::string url = what[1];
		stripHtmlTags(url);
		result.second += "\n\n";
		// add only url
		result.second += url;
	}
	
	result.first = true;
	return result;
}

}

#endif

