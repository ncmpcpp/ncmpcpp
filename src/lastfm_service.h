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

#ifndef NCMPCPP_LASTFM_SERVICE_H
#define NCMPCPP_LASTFM_SERVICE_H

#include "config.h"

#ifdef HAVE_CURL_CURL_H

#include <map>
#include <string>

#include "scrollpad.h"

namespace LastFm {

struct Service
{
	typedef std::map<std::string, std::string> Arguments;
	typedef std::pair<bool, std::string> Result;
	
	Service(Arguments args) : m_arguments(args) { }
	
	virtual const char *name() = 0;
	virtual Result fetch();
	
	virtual void beautifyOutput(NC::Scrollpad &w) = 0;
	
protected:
	virtual bool argumentsOk() = 0;
	virtual bool actionFailed(const std::string &data);
	
	virtual Result processData(const std::string &data) = 0;
	
	virtual const char *methodName() = 0;
	
	Arguments m_arguments;
};

struct ArtistInfo : public Service
{
	ArtistInfo(std::string artist, std::string lang)
	: Service({{"artist", artist}, {"lang", lang}}) { }
	
	virtual const char *name() { return "Artist info"; }
	
	virtual void beautifyOutput(NC::Scrollpad &w);
	
	bool operator==(const ArtistInfo &ai) const { return m_arguments == ai.m_arguments; }
	
protected:
	virtual bool argumentsOk();
	virtual Result processData(const std::string &data);
	
	virtual const char *methodName() { return "artist.getinfo"; }
};

}

#endif // HAVE_CURL_CURL_H

#endif // NCMPCPP_LASTFM_SERVICE_H
