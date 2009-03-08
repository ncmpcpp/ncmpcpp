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

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include "ncmpcpp.h"
#include "mpdpp.h"
#include "screen.h"

namespace Global
{
	extern BasicScreen *myScreen;
	extern BasicScreen *myOldScreen;
	
	extern Window *wHeader;
	extern Window *wFooter;
	
	extern MPD::Connection *Mpd;
	
	extern size_t MainStartY;
	extern size_t MainHeight;
	
	extern time_t Timer;
	
#	ifdef HAVE_CURL_CURL_H
	extern pthread_mutex_t CurlLock;
#	endif
	
	extern bool BlockItemListUpdate;
	
	extern bool MessagesAllowed;
	extern bool RedrawHeader;
	
	extern std::string VolumeState;
}

#endif

