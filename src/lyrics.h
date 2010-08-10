/***************************************************************************
 *   Copyright (C) 2008-2010 by Andrzej Rybczak                            *
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

#ifndef _LYRICS_H
#define _LYRICS_H

#include "ncmpcpp.h"
#include "mpdpp.h"
#include "screen.h"
#include "lyrics_fetcher.h"

class Lyrics : public Screen<Scrollpad>
{
	public:
		Lyrics() : ReloadNP(0),
#		ifdef HAVE_CURL_CURL_H
		ReadyToTake(0), DownloadInProgress(0), Fetcher(0),
#		endif // HAVE_CURL_CURL_H
		itsScrollBegin(0) { }
		
		~Lyrics() { }
		
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void Update();
		
		virtual void EnterPressed() { }
		virtual void SpacePressed();
		
		virtual bool allowsSelection() { return false; }
		
		virtual List *GetList() { return 0; }
		
		void Edit();
		void Save(const std::string &lyrics);
		void Refetch();
#		ifdef HAVE_CURL_CURL_H
		void ToggleFetcher();
#		endif // HAVE_CURL_CURL_H
		
		bool ReloadNP;
		
	protected:
		virtual void Init();
		
	private:
		void Load();
		
		std::string itsFilenamePath;
		static const std::string Folder;
		
#		ifdef HAVE_CURL_CURL_H
		void *Download();
		static void *DownloadWrapper(void *);
		
		void Take();
		bool ReadyToTake;
		bool DownloadInProgress;
		pthread_t Downloader;
		LyricsFetcher **Fetcher;
#		endif // HAVE_CURL_CURL_H
		
		size_t itsScrollBegin;
		MPD::Song itsSong;
};

extern Lyrics *myLyrics;

#endif

