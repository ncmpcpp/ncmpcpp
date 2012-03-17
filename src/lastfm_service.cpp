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

#include "lastfm_service.h"

#ifdef HAVE_CURL_CURL_H

#include "conv.h"
#include "curl_handle.h"
#include "settings.h"

const char *LastfmService::baseURL = "http://ws.audioscrobbler.com/2.0/?api_key=d94e5b6e26469a2d1ffae8ef20131b79&method=";

const char *LastfmService::msgParseFailed = "Fetched data could not be parsed";

LastfmService::Result LastfmService::fetch(Args &args)
{
	Result result;
	
	std::string url = baseURL;
	url += methodName();
	for (Args::const_iterator it = args.begin(); it != args.end(); ++it)
	{
		url += "&";
		url += it->first;
		url += "=";
		url += Curl::escape(it->second);
	}
	
	std::string data;
	CURLcode code = Curl::perform(data, url);
	
	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}
	
	if (actionFailed(data))
	{
		StripHtmlTags(data);
		result.second = data;
		return result;
	}
	
	if (!parse(data))
	{
		// if relevant part of data was not found and one of arguments
		// was language, try to fetch it again without that parameter.
		// otherwise just report failure.
		Args::iterator lang = args.find("lang");
		if (lang != args.end())
		{
			args.erase(lang);
			return fetch(args);
		}
		else
		{
			// parse should change data to error msg, if it fails
			result.second = data;
			return result;
		}
	}
	
	result.first = true;
	result.second = data;
	return result;
}

bool LastfmService::actionFailed(const std::string &data)
{
	return data.find("status=\"failed\"") != std::string::npos;
}

void LastfmService::postProcess(std::string &data)
{
	StripHtmlTags(data);
	Trim(data);
}

/***********************************************************************/

bool ArtistInfo::checkArgs(const Args &args)
{
	return args.find("artist") != args.end();
}

void ArtistInfo::colorizeOutput(NCurses::Scrollpad &w)
{
	w.SetFormatting(fmtBold, U("\n\nSimilar artists:\n"), fmtBoldEnd, false);
	w.SetFormatting(Config.color2, U("\n * "), clEnd, true);
	// below is used so format won't be removed using RemoveFormatting() by accident.
	w.ForgetFormatting();
}

bool ArtistInfo::parse(std::string &data)
{
	size_t a, b;
	bool parse_failed = false;
	
	if ((a = data.find("<content>")) != std::string::npos)
	{
		a += static_strlen("<content>");
		if ((b = data.find("</content>")) == std::string::npos)
			parse_failed = true;
	}
	else
		parse_failed = true;
	
	if (parse_failed)
	{
		data = msgParseFailed;
		return false;
	}
	
	if (a == b)
	{
		data = "No description available for this artist.";
		return false;
	}
	
	std::vector< std::pair<std::string, std::string> > similars;
	for (size_t i = data.find("<name>"), j, k = data.find("<url>"), l;
		    i != std::string::npos; i = data.find("<name>", i), k = data.find("<url>", k))
	{
		j = data.find("</name>", i);
		i += static_strlen("<name>");
		
		l = data.find("</url>", k);
		k += static_strlen("<url>");
		
		similars.push_back(std::make_pair(data.substr(i, j-i), data.substr(k, l-k)));
		StripHtmlTags(similars.back().first);
	}
	
	a += static_strlen("<![CDATA[");
	b -= static_strlen("]]>");
	data = data.substr(a, b-a);
	
	postProcess(data);
	
	data += "\n\nSimilar artists:\n";
	for (size_t i = 1; i < similars.size(); ++i)
	{
		 data += "\n * ";
		 data += similars[i].first;
		 data += " (";
		 data += similars[i].second;
		 data += ")";
	}
	data += "\n\n";
	data += similars.front().second;
	
	return true;
}

#endif

