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

#ifndef _H_INFO
#define _H_INFO

#include "ncmpcpp.h"
#include "mpdpp.h"
#include "screen.h"

class Info : public Screen<Scrollpad>
{
	public:
		virtual void Init();
		virtual void SwitchTo() { }
		virtual void Resize();
		
		virtual std::string Title();
		
#		ifdef HAVE_PTHREAD_H
		virtual void Update();
#		endif // HAVE_PTHREAD_H
		
		virtual void EnterPressed() { }
		virtual void SpacePressed() { }
		
		virtual bool allowsSelection() { return false; }
		
		virtual List *GetList() { return 0; }
		
		void GetSong();
#		ifdef HAVE_CURL_CURL_H
		void GetArtist();
#		endif // HAVE_CURL_CURL_H
		
	private:
		void PrepareSong(MPD::Song &);
		static const basic_buffer<my_char_t> &ShowTag(const std::string &);
		
#		ifdef HAVE_CURL_CURL_H
		static void *PrepareArtist(void *);
		
		static const std::string Folder;
		static bool ArtistReady;
		
#		ifdef HAVE_PTHREAD_H
		static pthread_t *Downloader;
#		endif // HAVE_PTHREAD_H
		
#		endif // HAVE_CURL_CURL_H
		
		std::string itsTitle;
};

extern Info *myInfo;

#endif

