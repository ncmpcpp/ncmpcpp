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

#include "libmpdclient.h"
#include "ncmpcpp.h"
#include "song.h"

enum QueueCommandType { qctAdd, qctAddToPlaylist, qctDelete, qctDeleteID, qctMove, qctPlaylistMove, qctDeleteFromPlaylist };
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
	QueueCommand() : id(0), id2(0) { }
	QueueCommandType type;
	string playlist_path;
	string item_path;
	int id;
	int id2;
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
		bool Connected() const;
		void Disconnect();
		
		void SetHostname(const string &);
		void SetPort(int port) { MPD_PORT = port; }
		void SetTimeout(int timeout) { MPD_TIMEOUT = timeout; }
		void SetPassword(const string &password) { MPD_PASSWORD = password; }
		void SendPassword() const;
		
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
		
		unsigned int GetMaxPlaylistLength() const { return itsMaxPlaylistLength; }
		int GetPlaylistLength() const { return isConnected && itsCurrentStatus ? itsCurrentStatus->playlistLength : 0; }
		void GetPlaylistChanges(long long, SongList &) const;
		
		string GetLastErrorMessage() const { return itsLastErrorMessage; }
		int GetErrorCode() const { return itsErrorCode; }
		
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
		void QueueAddToPlaylist(const string &, const string &);
		void QueueAddToPlaylist(const string &, const Song &);
		void QueueDeleteSong(int);
		void QueueDeleteSongId(int);
		void QueueMove(int, int);
		void QueueMove(const string &, int, int);
		void QueueDeleteFromPlaylist(const string &, int);
		bool CommitQueue();
		
		void DeletePlaylist(const string &) const;
		bool SavePlaylist(const string &) const;
		void ClearPlaylist(const string &) const;
		void AddToPlaylist(const string &, const Song &) const;
		void AddToPlaylist(const string &, const string &) const;
		void Move(const string &, int, int) const;
		void Rename(const string &, const string &) const;
		
		void StartSearch(bool) const;
		void StartFieldSearch(mpd_TagItems);
		void AddSearch(mpd_TagItems, const string &) const;
		void CommitSearch(SongList &) const;
		void CommitSearch(TagList &) const;
		
		void GetPlaylists(TagList &) const;
		void GetList(TagList &, mpd_TagItems) const;
		void GetArtists(TagList &) const;
		void GetAlbums(string, TagList &) const;
		void GetDirectory(const string &, ItemList &) const;
		void GetDirectoryRecursive(const string &, SongList &) const;
		void GetSongs(const string &, SongList &) const;
		void GetDirectories(const string &, TagList &) const;
	
	private:
		int CheckForErrors();
		void ClearQueue();
		
		mpd_Connection *itsConnection;
		bool isConnected;
		
		string itsLastErrorMessage;
		int itsErrorCode;
		unsigned int itsMaxPlaylistLength;
		
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
		
		mpd_TagItems itsSearchedField;
		std::vector<QueueCommand *> itsQueue;
};

#endif

