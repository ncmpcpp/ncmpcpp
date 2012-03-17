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

#ifndef _LYRICS_H
#define _LYRICS_H

#include <pthread.h>
#include <queue>

#include "ncmpcpp.h"
#include "mpdpp.h"
#include "screen.h"
#include "lyrics_fetcher.h"

class Lyrics : public Screen<Scrollpad>
{
	public:
		Lyrics() : ReloadNP(0),
#		ifdef HAVE_CURL_CURL_H
		isReadyToTake(0), isDownloadInProgress(0),
#		endif // HAVE_CURL_CURL_H
		itsScrollBegin(0) { }
		
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void Update();
		
		virtual void EnterPressed() { }
		virtual void SpacePressed();
		
		virtual bool allowsSelection() { return false; }
		
		virtual List *GetList() { return 0; }
		
		virtual bool isMergable() { return true; }
		
		void Edit();
		void Refetch();
		
#		ifdef HAVE_CURL_CURL_H
		static void ToggleFetcher();
		static void DownloadInBackground(const MPD::Song *s);
#		endif // HAVE_CURL_CURL_H
		
		bool ReloadNP;
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return false; }
		
	private:
		void Load();
		
#		ifdef HAVE_CURL_CURL_H
		static void *DownloadInBackgroundImpl(void *);
		static void DownloadInBackgroundImplHelper(MPD::Song *);
		// lock for allowing exclusive access to itsToDownload and itsWorkersNumber
		static pthread_mutex_t itsDIBLock;
		// storage for songs for which lyrics are scheduled to be downloaded
		static std::queue<MPD::Song *> itsToDownload;
		// current worker threads (ie. downloading lyrics)
		static size_t itsWorkersNumber;
		// maximum number of worker threads. if it's reached, next lyrics requests
		// are put into itsToDownload queue.
		static const size_t itsMaxWorkersNumber = 4;
		
		void *Download();
		static void *DownloadWrapper(void *);
		static void Save(const std::string &filename, const std::string &lyrics);
		
		void Take();
		bool isReadyToTake;
		bool isDownloadInProgress;
		pthread_t itsDownloader;
		
		static LyricsFetcher **itsFetcher;
#		endif // HAVE_CURL_CURL_H
		
		size_t itsScrollBegin;
		MPD::Song itsSong;
		std::string itsFilename;
		
		static std::string GenerateFilename(const MPD::Song &s);
};

extern Lyrics *myLyrics;

#endif

