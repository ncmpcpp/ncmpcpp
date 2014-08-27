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

#ifndef NCMPCPP_LYRICS_H
#define NCMPCPP_LYRICS_H

#include <pthread.h>
#include <queue>

#include "interfaces.h"
#include "lyrics_fetcher.h"
#include "screen.h"
#include "song.h"

struct Lyrics: Screen<NC::Scrollpad>, Tabbable
{
	Lyrics();
	
	// Screen<NC::Scrollpad> implementation
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::Lyrics; }
	
	virtual void update() OVERRIDE;
	
	virtual void enterPressed() OVERRIDE { }
	virtual void spacePressed() OVERRIDE;
	
	virtual bool isMergable() OVERRIDE { return true; }
	
	// private members
	void Edit();
	
#	ifdef HAVE_CURL_CURL_H
	void Refetch();
	
	static void ToggleFetcher();
	static void DownloadInBackground(const MPD::Song &s);
#	endif // HAVE_CURL_CURL_H
	
	bool ReloadNP;
	
protected:
	virtual bool isLockable() OVERRIDE { return false; }
	
private:
	void Load();
	
#	ifdef HAVE_CURL_CURL_H
	static void *DownloadInBackgroundImpl(void *song_ptr);
	static void DownloadInBackgroundImplHelper(const MPD::Song &s);
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
#	endif // HAVE_CURL_CURL_H
	
	size_t itsScrollBegin;
	MPD::Song itsSong;
	std::string itsFilename;
	
	static std::string GenerateFilename(const MPD::Song &s);
};

extern Lyrics *myLyrics;

#endif // NCMPCPP_LYRICS_H

