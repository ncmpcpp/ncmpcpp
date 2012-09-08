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

#ifndef _H_LASTFM
#define _H_LASTFM

#include "config.h"

#ifdef HAVE_CURL_CURL_H

#include <memory>
#include <pthread.h>

#include "lastfm_service.h"
#include "screen.h"

class Lastfm : public Screen<NC::Scrollpad>
{
	public:
		Lastfm() : isReadyToTake(0), isDownloadInProgress(0) { }
		
		// Screen<NC::Scrollpad>
		virtual void SwitchTo() OVERRIDE;
		virtual void Resize() OVERRIDE;
		
		virtual std::wstring Title() OVERRIDE;
		
		virtual void Update() OVERRIDE;
		
		virtual void EnterPressed() OVERRIDE { }
		virtual void SpacePressed() OVERRIDE { }
		
		virtual bool isMergable() OVERRIDE { return true; }
		virtual bool isTabbable() OVERRIDE { return false; }
		
		// private members
		void Refetch();
		
		bool isDownloading() { return isDownloadInProgress && !isReadyToTake; }
		bool SetArtistInfoArgs(const std::string &artist, const std::string &lang = "");
		
	protected:
		virtual void Init() OVERRIDE;
		virtual bool isLockable() OVERRIDE { return false; }
		
	private:
		std::wstring itsTitle;
		
		std::string itsArtist;
		std::string itsFilename;
		
		std::string itsFolder;
		
		std::auto_ptr<LastfmService> itsService;
		LastfmService::Args itsArgs;
		
		void Load();
		void Save(const std::string &data);
		void SetTitleAndFolder();
		
		void Download();
		static void *DownloadWrapper(void *);
		
		void Take();
		bool isReadyToTake;
		bool isDownloadInProgress;
		pthread_t itsDownloader;
};

extern Lastfm *myLastfm;

#endif // HAVE_CURL_CURL_H

#endif

