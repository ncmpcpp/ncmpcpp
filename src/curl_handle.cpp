/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

namespace
{
	size_t write_data(char *buffer, size_t size, size_t nmemb, void *data)
	{
		size_t result = size*nmemb;
		static_cast<std::string *>(data)->append(buffer, result);
		return result;
	}
}

CURLcode Curl::perform(const std::string &URL, std::string &data, unsigned timeout)
{
#	ifdef HAVE_PTHREAD_H
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&lock);
#	endif
	CURLcode result;
	CURL *c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_URL, URL.c_str());
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, timeout);
	curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1);
	result = curl_easy_perform(c);
	curl_easy_cleanup(c);
#	ifdef HAVE_PTHREAD_H
	pthread_mutex_unlock(&lock);
#	endif
	return result;
}

std::string Curl::escape(const std::string &s)
{
	char *cs = curl_easy_escape(0, s.c_str(), s.length());
	std::string result(cs);
	curl_free(cs);
	return result;
}

#endif // HAVE_CURL_CURL_H

