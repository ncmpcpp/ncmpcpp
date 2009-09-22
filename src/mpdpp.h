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

#ifndef _MPDPP_H
#define _MPDPP_H

#include <vector>

#include <mpd/client.h>
#include "song.h"

namespace MPD
{
	namespace Message
	{
		extern const char *PartOfSongsAdded;
		extern const char *FullPlaylist;
		extern const char *FunctionDisabledFilteringEnabled;
	}
	
	enum ItemType { itDirectory, itSong, itPlaylist };
	enum PlayerState { psUnknown, psStop, psPlay, psPause };

	struct Item
	{
		Item() : song(0) { }
		Song *song;
		ItemType type;
		std::string name;
	};
	
	struct StatusChanges
	{
		StatusChanges() : Playlist(0), SongID(0), Database(0), DBUpdating(0), Volume(0), ElapsedTime(0), Crossfade(0), Random(0), Repeat(0), Single(0), Consume(0), PlayerState(0), StatusFlags(0) { }
		bool Playlist:1;
		bool SongID:1;
		bool Database:1;
		bool DBUpdating:1;
		bool Volume:1;
		bool ElapsedTime:1;
		bool Crossfade:1;
		bool Random:1;
		bool Repeat:1;
		bool Single:1;
		bool Consume:1;
		bool PlayerState:1;
		bool StatusFlags:1;
	};
	
	typedef std::pair<std::string, bool> Output;
	
	typedef std::vector<Item> ItemList;
	typedef std::vector<Song *> SongList;
	typedef std::vector<std::string> TagList;
	typedef std::vector<Output> OutputList;

	void FreeSongList(SongList &);
	void FreeItemList(ItemList &);

	class Connection
	{
		typedef void (*StatusUpdater) (Connection *, StatusChanges, void *);
		typedef void (*ErrorHandler) (Connection *, int, const char *, void *);
		
		public:
			Connection();
			~Connection();
			
			bool Connect();
			bool Connected() const;
			void Disconnect();
			
			const std::string & GetHostname() { return itsHost; }
			int GetPort() { return itsPort; }
			
			float Version() const;
			
			void SetHostname(const std::string &);
			void SetPort(int port) { itsPort = port; }
			void SetTimeout(int timeout) { itsTimeout = timeout; }
			void SetPassword(const std::string &password) { itsPassword = password; }
			bool SendPassword() const;
			
			void SetStatusUpdater(StatusUpdater, void *);
			void SetErrorHandler(ErrorHandler, void *);
			void UpdateStatus();
			bool UpdateDirectory(const std::string &);
			
			bool Execute(const std::string &) const;
			
			void Play() const;
			void Play(int) const;
			void PlayID(int) const;
			void Pause() const;
			void Stop() const;
			void Next() const;
			void Prev() const;
			void Move(unsigned, unsigned) const;
			void Swap(unsigned, unsigned) const;
			void Seek(unsigned) const;
			void Shuffle() const;
			void ClearPlaylist() const;
			
			bool isPlaying() const { return GetState() > psStop; }
			
			PlayerState GetState() const{ return itsCurrentStatus ? PlayerState(mpd_status_get_state(itsCurrentStatus)) : psUnknown; }
			bool GetRepeat() const { return itsCurrentStatus ? mpd_status_get_repeat(itsCurrentStatus) : 0; }
			bool GetRandom() const { return itsCurrentStatus ? mpd_status_get_random(itsCurrentStatus) : 0; }
			bool GetSingle() const { return itsCurrentStatus ? mpd_status_get_single(itsCurrentStatus) : 0; }
			bool GetConsume() const { return itsCurrentStatus ? mpd_status_get_consume(itsCurrentStatus) : 0; }
			bool GetDBIsUpdating() const { return itsCurrentStatus ? mpd_status_get_update_id(itsCurrentStatus) : 0; }
			int GetVolume() const { return itsCurrentStatus ? mpd_status_get_volume(itsCurrentStatus) : -1; }
			unsigned GetCrossfade() const { return itsCurrentStatus ? mpd_status_get_crossfade(itsCurrentStatus) : 0; }
			unsigned GetPlaylistID() const { return itsCurrentStatus ? mpd_status_get_queue_version(itsCurrentStatus) : 0; }
			unsigned GetOldPlaylistID() const { return itsOldStatus ? mpd_status_get_queue_version(itsOldStatus) : 0; }
			unsigned GetElapsedTime() const { return itsCurrentStatus ? mpd_status_get_elapsed_time(itsCurrentStatus) : 0; }
			int GetTotalTime() const { return itsCurrentStatus ? mpd_status_get_total_time(itsCurrentStatus) : 0; }
			unsigned GetBitrate() const { return itsCurrentStatus ? mpd_status_get_kbit_rate(itsCurrentStatus) : 0; }
			
			size_t GetMaxPlaylistLength() const { return itsMaxPlaylistLength; }
			size_t GetPlaylistLength() const { return itsCurrentStatus ? mpd_status_get_queue_length(itsCurrentStatus) : 0; }
			void GetPlaylistChanges(unsigned, SongList &) const;
			
			const std::string & GetErrorMessage() const { return itsErrorMessage; }
			int GetErrorCode() const { return itsErrorCode; }
			
			Song GetCurrentSong() const;
			int GetCurrentSongPos() const;
			Song GetSong(const std::string &) const;
			void GetPlaylistContent(const std::string &, SongList &) const;
			
			void SetRepeat(bool) const;
			void SetRandom(bool) const;
			void SetSingle(bool) const;
			void SetConsume(bool) const;
			void SetCrossfade(unsigned) const;
			void SetVolume(unsigned);
			
			int AddSong(const std::string &); // returns id of added song
			int AddSong(const Song &); // returns id of added song
			bool AddRandomSongs(size_t);
			void Add(const std::string &path) const;
			void Delete(unsigned) const;
			void DeleteID(unsigned) const;
			void Delete(const std::string &, unsigned) const;
			void StartCommandsList();
			bool CommitCommandsList();
			
			void DeletePlaylist(const std::string &) const;
			bool SavePlaylist(const std::string &) const;
			void ClearPlaylist(const std::string &) const;
			void AddToPlaylist(const std::string &, const Song &) const;
			void AddToPlaylist(const std::string &, const std::string &) const;
			void Move(const std::string &, int, int) const;
			void Rename(const std::string &, const std::string &) const;
			
			void StartSearch(bool) const;
			void StartFieldSearch(mpd_tag_type);
			void AddSearch(mpd_tag_type, const std::string &) const;
			void CommitSearch(SongList &) const;
			void CommitSearch(TagList &) const;
			
			void GetPlaylists(TagList &) const;
			void GetList(TagList &, mpd_tag_type) const;
			void GetAlbums(const std::string &, TagList &) const;
			void GetDirectory(const std::string &, ItemList &) const;
			void GetDirectoryRecursive(const std::string &, SongList &) const;
			void GetSongs(const std::string &, SongList &) const;
			void GetDirectories(const std::string &, TagList &) const;
			
			void GetOutputs(OutputList &) const;
			bool EnableOutput(int);
			bool DisableOutput(int);
			
		private:
			int CheckForErrors();
			
			mpd_connection *itsConnection;
			bool isConnected;
			bool isCommandsListEnabled;
			
			std::string itsErrorMessage;
			int itsErrorCode;
			size_t itsMaxPlaylistLength;
			
			std::string itsHost;
			int itsPort;
			int itsTimeout;
			std::string itsPassword;
			
			mpd_status *itsCurrentStatus;
			mpd_status *itsOldStatus;
			
			StatusChanges itsChanges;
			
			StatusUpdater itsUpdater;
			void *itsStatusUpdaterUserdata;
			ErrorHandler itsErrorHandler;
			void *itsErrorHandlerUserdata;
			
			mpd_tag_type itsSearchedField;
	};
}

extern MPD::Connection Mpd;

#endif

