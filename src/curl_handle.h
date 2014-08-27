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

#ifndef NCMPCPP_CURL_HANDLE_H
#define NCMPCPP_CURL_HANDLE_H

#include "config.h"

#ifdef HAVE_CURL_CURL_H

#include <string>
#include "curl/curl.h"

namespace Curl
{
	CURLcode perform(std::string &data, const std::string &URL, const std::string &referer = "", unsigned timeout = 10);
	
	std::string escape(const std::string &s);
}

#endif // HAVE_CURL_CURL_H

#endif // NCMPCPP_CURL_HANDLE_H

