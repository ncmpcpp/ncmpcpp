/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef HAVE_MPDPP_H
#define HAVE_MPDPP_H

#include "ncmpcpp.h"
#include "song.h"

enum QueueCommandType { qctAdd, qctDelete, qctDeleteID };
enum ItemType { itDirectory, itSong, itPlaylist };
enum PlayerState { psUnknown, psStop, psPlay, psPause };

struct MPDStatusChanges
{
	MPDStatusChanges() : Playlist(0), SongID(0), Database(0), DBUpdating(0), Volume(0), ElapsedTime(0), Crossfade(0), Random(0), Repeat(0), PlayerState(0) { }
	bool Playlist;
	bool SongID;
	bool Database;
	bool DBUpdating;
	bool Volume;
	bool ElapsedTime;
	bool Crossfade;
	bool Random;
	bool Repeat;
	bool PlayerState;
};

struct QueueCommand
{
	QueueCommand() : id(0) { }
	QueueCommandType type;
	string path;
	int id;
};

struct Item
{
	Item() : song(0) { }
	Song *song;
	ItemType type;
	string name;
};

typedef std::vector<Item> ItemList;
typedef std::vector<Song *> SongList;
typedef std::vector<string> TagList;

void FreeSongList(SongList &);
void FreeItemList(ItemList &);

class MPDConnection
{
	typedef void (*StatusUpdater) (MPDConnection *, MPDStatusChanges, void *);
	typedef void (*ErrorHandler) (MPDConnection *, int, string, void *);
	
	public:
		MPDConnection();
		~MPDConnection();
		
		bool Connect();
		bool Connected();
		void Disconnect();
		
		void SetHostname(string hostname) { MPD_HOST = hostname; }
		void SetPort(int port) { MPD_PORT = port; }
		void SetTimeout(int timeout) { MPD_TIMEOUT = timeout; }
		void SetPassword(string password) { MPD_PASSWORD = password; }
		void SendPassword();
		
		void SetStatusUpdater(StatusUpdater, void *);
		void SetErrorHandler(ErrorHandler, void *);
		void UpdateStatus();
		void UpdateDirectory(const string &) const;
		
		void Play() const;
		void Play(int) const;
		void PlayID(int) const;
		void Pause() const;
		void Stop() const;
		void Next() const;
		void Prev() const;
		void Move(int, int) const;
		void Seek(int) const;
		void Shuffle() const;
		void ClearPlaylist() const;
		
		PlayerState GetState() const { return isConnected && itsCurrentStatus ? (PlayerState)itsCurrentStatus->state : psUnknown; }
		bool GetRepeat() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->repeat : 0; }
		bool GetRandom() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->random : 0; }
		bool GetDBIsUpdating() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->updatingDb : 0; }
		int GetVolume() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->volume : -1; }
		int GetCrossfade() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->crossfade : -1; }
		long long GetPlaylistID() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->playlist : -1; }
		long long GetOldPlaylistID() const { return isConnected && itsOldStatus ? itsOldStatus->playlist : -1; }
		int GetElapsedTime() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->elapsedTime : -1; }
		
		unsigned int GetMaxPlaylistLength() { return itsMaxPlaylistLength; }
		int GetPlaylistLength() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->playlistLength : -1; }
		void GetPlaylistChanges(long long, SongList &) const;
		
		string GetLastErrorMessage() const { return itsLastErrorMessage; }
		
		Song GetCurrentSong() const;
		int GetCurrentSongPos() const;
		Song GetSong(const string &) const;
		void GetPlaylistContent(const string &, SongList &) const;
		
		void SetRepeat(bool) const;
		void SetRandom(bool) const;
		void SetVolume(int) const;
		void SetCrossfade(int) const;
		
		int AddSong(const string &); // returns id of added song
		int AddSong(const Song &); // returns id of added song
		void QueueAddSong(const string &);
		void QueueAddSong(const Song &);
		void QueueDeleteSong(int);
		void QueueDeleteSongId(int);
		bool CommitQueue();
		
		void DeletePlaylist(const string &);
		bool SavePlaylist(const string &);
		
		void StartSearch(bool) const;
		void AddSearch(mpd_TagItems, const string &) const;
		void CommitSearch(SongList &v) const;
		
		void GetArtists(TagList &) const;
		void GetAlbums(string, TagList &) const;
		void GetDirectory(const string &, ItemList &) const;
		void GetDirectoryRecursive(const string &, SongList &) const;
	
	private:
		int CheckForErrors();
		void ClearQueue();
		string itsLastErrorMessage;
		
		mpd_Connection *itsConnection;
		bool isConnected;
		
		string MPD_HOST;
		int MPD_PORT;
		int MPD_TIMEOUT;
		string MPD_PASSWORD;
		
		mpd_Stats *itsOldStats;
		mpd_Stats *itsCurrentStats;
		mpd_Status *itsCurrentStatus;
		mpd_Status *itsOldStatus;
		
		StatusUpdater itsUpdater;
		void *itsStatusUpdaterUserdata;
		ErrorHandler itsErrorHandler;
		void *itsErrorHandlerUserdata;
		
		std::vector<QueueCommand *> itsQueue;
		unsigned int itsMaxPlaylistLength;
};

#endif

