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

#ifndef _LASTFM_SERVICE_H
#define _LASTFM_SERVICE_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_CURL_CURL_H

#include <map>
#include <string>

#include "scrollpad.h"

struct LastfmService
{
	typedef std::map<std::string, std::string> Args;
	typedef std::pair<bool, std::string> Result;
	
	virtual const char *name() = 0;
	virtual Result fetch(Args &args);
	
	virtual bool checkArgs(const Args &args) = 0;
	virtual void colorizeOutput(NCurses::Scrollpad &w) = 0;
	
	protected:
		virtual bool actionFailed(const std::string &data);
		
		virtual bool parse(std::string &data) = 0;
		virtual void postProcess(std::string &data);
		
		virtual const char *methodName() = 0;
		
		static const char *baseURL;
		static const char *msgParseFailed;
};

struct ArtistInfo : public LastfmService
{
	virtual const char *name() { return "Artist info"; }
	
	virtual bool checkArgs(const Args &args);
	virtual void colorizeOutput(NCurses::Scrollpad &w);
	
	protected:
		virtual bool parse(std::string &data);
		
		virtual const char *methodName() { return "artist.getinfo"; }
};

#endif // HAVE_CURL_CURL_H

#endif
