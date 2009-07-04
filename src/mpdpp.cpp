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

#include <cstdlib>
#include <algorithm>

#include "charset.h"
#include "mpdpp.h"

using namespace MPD;

using std::string;

MPD::Connection Mpd;

const char *MPD::Message::PartOfSongsAdded = "Only part of requested songs' list added to playlist!";
const char *MPD::Message::FullPlaylist = "Playlist is full!";
const char *MPD::Message::FunctionDisabledFilteringEnabled = "Function disabled due to enabled filtering in playlist";

Connection::Connection() : itsConnection(0),
			   isConnected(0),
			   isCommandsListEnabled(0),
			   itsErrorCode(0),
			   itsMaxPlaylistLength(-1),
			   itsHost("localhost"),
			   itsPort(6600),
			   itsTimeout(15),
			   itsCurrentStatus(0),
			   itsOldStatus(0),
			   itsUpdater(0),
			   itsErrorHandler(0)
{
}

Connection::~Connection()
{
	if (itsConnection)
		mpd_closeConnection(itsConnection);
	if (itsOldStatus)
		mpd_freeStatus(itsOldStatus);
	if (itsCurrentStatus)
		mpd_freeStatus(itsCurrentStatus);
}

bool Connection::Connect()
{
	if (!isConnected && !itsConnection)
	{
		itsConnection = mpd_newConnection(itsHost.c_str(), itsPort, itsTimeout);
		isConnected = 1;
		if (CheckForErrors())
			return false;
		if (!itsPassword.empty())
			SendPassword();
		return !CheckForErrors();
	}
	else
		return true;
}

bool Connection::Connected() const
{
	return isConnected;
}

void Connection::Disconnect()
{
	if (itsConnection)
		mpd_closeConnection(itsConnection);
	if (itsOldStatus)
		mpd_freeStatus(itsOldStatus);
	if (itsCurrentStatus)
		mpd_freeStatus(itsCurrentStatus);
	itsConnection = 0;
	itsCurrentStatus = 0;
	itsOldStatus = 0;
	isConnected = 0;
	isCommandsListEnabled = 0;
	itsMaxPlaylistLength = -1;
}

float Connection::Version() const
{
	return itsConnection ? itsConnection->version[1] + itsConnection->version[2]*0.1 : 0;
}

void Connection::SetHostname(const string &host)
{
	size_t at = host.find("@");
	if (at != string::npos)
	{
		itsPassword = host.substr(0, at);
		itsHost = host.substr(at+1);
	}
	else
		itsHost = host;
}

void Connection::SendPassword() const
{
	mpd_sendPasswordCommand(itsConnection, itsPassword.c_str());
	mpd_finishCommand(itsConnection);
}

void Connection::SetStatusUpdater(StatusUpdater updater, void *data)
{
	itsUpdater = updater;
	itsStatusUpdaterUserdata = data;
}

void Connection::SetErrorHandler(ErrorHandler handler, void *data)
{
	itsErrorHandler = handler;
	itsErrorHandlerUserdata = data;
}

void Connection::UpdateStatus()
{
	if (!itsConnection)
		return;
	
	CheckForErrors();
	
	if (!itsConnection)
		return;
	
	if (itsOldStatus)
		mpd_freeStatus(itsOldStatus);
	
	itsOldStatus = itsCurrentStatus;
	itsCurrentStatus = 0;
	
	mpd_sendStatusCommand(itsConnection);
	itsCurrentStatus = mpd_getStatus(itsConnection);
	
	if (!itsMaxPlaylistLength)
		itsMaxPlaylistLength = GetPlaylistLength();
	
	if (CheckForErrors())
		return;
	
	if (itsCurrentStatus && itsUpdater)
	{
		if (!itsOldStatus)
		{
			itsChanges.Playlist = 1;
			itsChanges.SongID = 1;
			itsChanges.Database = 1;
			itsChanges.DBUpdating = 1;
			itsChanges.Volume = 1;
			itsChanges.ElapsedTime = 1;
			itsChanges.Crossfade = 1;
			itsChanges.Random = 1;
			itsChanges.Repeat = 1;
			itsChanges.Single = 1;
			itsChanges.Consume = 1;
			itsChanges.PlayerState = 1;
			itsChanges.StatusFlags = 1;
		}
		else
		{
			itsChanges.Playlist = itsOldStatus->playlist != itsCurrentStatus->playlist;
			itsChanges.SongID = itsOldStatus->songid != itsCurrentStatus->songid;
			itsChanges.Database = itsOldStatus->updatingDb && !itsCurrentStatus->updatingDb;
			itsChanges.DBUpdating = itsOldStatus->updatingDb != itsCurrentStatus->updatingDb;
			itsChanges.Volume = itsOldStatus->volume != itsCurrentStatus->volume;
			itsChanges.ElapsedTime = itsOldStatus->elapsedTime != itsCurrentStatus->elapsedTime;
			itsChanges.Crossfade = itsOldStatus->crossfade != itsCurrentStatus->crossfade;
			itsChanges.Random = itsOldStatus->random != itsCurrentStatus->random;
			itsChanges.Repeat = itsOldStatus->repeat != itsCurrentStatus->repeat;
			itsChanges.Single = itsOldStatus->single != itsCurrentStatus->single;
			itsChanges.Consume = itsOldStatus->consume != itsCurrentStatus->consume;
			itsChanges.PlayerState = itsOldStatus->state != itsCurrentStatus->state;
			itsChanges.StatusFlags = itsChanges.Repeat || itsChanges.Random || itsChanges.Single || itsChanges.Consume || itsChanges.Crossfade || itsChanges.DBUpdating;
		}
		itsUpdater(this, itsChanges, itsErrorHandlerUserdata);
	}
}

void Connection::UpdateDirectory(const string &path)
{
	if (isConnected)
	{
		mpd_sendUpdateCommand(itsConnection, path.c_str());
		mpd_finishCommand(itsConnection);
		if (!itsConnection->error && itsUpdater)
		{
			itsCurrentStatus->updatingDb = 1;
			StatusChanges ch;
			ch.DBUpdating = 1;
			ch.StatusFlags = 1;
			itsUpdater(this, ch, itsErrorHandlerUserdata);
		}
	}
}

void Connection::Execute(const string &command) const
{
	if (isConnected)
	{
		mpd_executeCommand(itsConnection, command.c_str());
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Play() const
{
	if (isConnected)
		PlayID(-1);
}

void Connection::Play(int pos) const
{
	if (isConnected)
	{
		mpd_sendPlayCommand(itsConnection, pos);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::PlayID(int id) const
{
	if (isConnected)
	{
		mpd_sendPlayIdCommand(itsConnection, id);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Pause() const
{
	if (isConnected)
	{
		mpd_sendPauseCommand(itsConnection, itsCurrentStatus->state != psPause);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Stop() const
{
	if (isConnected)
	{
		mpd_sendStopCommand(itsConnection);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Next() const
{
	if (isConnected)
	{
		mpd_sendNextCommand(itsConnection);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Prev() const
{
	if (isConnected)
	{
		mpd_sendPrevCommand(itsConnection);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Move(int from, int to) const
{
	if (isConnected)
	{
		mpd_sendMoveCommand(itsConnection, from, to);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Swap(int from, int to) const
{
	if (isConnected)
	{
		mpd_sendSwapCommand(itsConnection, from, to);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Seek(int where) const
{
	if (isConnected)
	{
		mpd_sendSeekCommand(itsConnection, itsCurrentStatus->song, where);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Shuffle() const
{
	if (isConnected)
	{
		mpd_sendShuffleCommand(itsConnection);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::ClearPlaylist() const
{
	if (isConnected)
	{
		mpd_sendClearCommand(itsConnection);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::ClearPlaylist(const string &playlist) const
{
	if (isConnected)
	{
		mpd_sendPlaylistClearCommand(itsConnection, playlist.c_str());
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::AddToPlaylist(const string &path, const Song &s) const
{
	if (!s.Empty())
		AddToPlaylist(path, s.GetFile());
}

void Connection::AddToPlaylist(const string &path, const string &file) const
{
	if (isConnected)
	{
		mpd_sendPlaylistAddCommand(itsConnection, path.c_str(), file.c_str());
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Move(const string &path, int from, int to) const
{
	if (isConnected)
	{
		mpd_sendPlaylistMoveCommand(itsConnection, path.c_str(), from, to);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Rename(const string &from, const string &to) const
{
	if (isConnected)
	{
		mpd_sendRenameCommand(itsConnection, from.c_str(), to.c_str());
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::GetPlaylistChanges(long long id, SongList &v) const
{
	if (isConnected)
	{
		if (id < 0)
		{
			id = 0;
			v.reserve(GetPlaylistLength());
		}
		mpd_sendPlChangesCommand(itsConnection, id);
		mpd_InfoEntity *item = NULL;
		while ((item = mpd_getNextInfoEntity(itsConnection)) != NULL)
		{
			if (item->type == MPD_INFO_ENTITY_TYPE_SONG)
			{
				Song *s = new Song(item->info.song, 1);
				item->info.song = 0;
				v.push_back(s);
			}
			mpd_freeInfoEntity(item);
		}
		mpd_finishCommand(itsConnection);
	}
}

Song Connection::GetSong(const string &path) const
{
	if (isConnected)
	{
		mpd_sendListallInfoCommand(itsConnection, path.c_str());
		mpd_InfoEntity *item = NULL;
		item = mpd_getNextInfoEntity(itsConnection);
		Song result = item->info.song;
		item->info.song = 0;
		mpd_freeInfoEntity(item);
		mpd_finishCommand(itsConnection);
		return result;
	}
	else
		return Song();
}

int Connection::GetCurrentSongPos() const
{
	return isConnected && itsCurrentStatus ? (itsCurrentStatus->state == psPlay || itsCurrentStatus->state == psPause ? itsCurrentStatus->song : -1) : -1;
}

Song Connection::GetCurrentSong() const
{
	if (isConnected && (GetState() == psPlay || GetState() == psPause))
	{
		mpd_sendCurrentSongCommand(itsConnection);
		mpd_InfoEntity *item = NULL;
		item = mpd_getNextInfoEntity(itsConnection);
		if (item)
		{
			Song result = item->info.song;
			item->info.song = 0;
			mpd_freeInfoEntity(item);
			return result;
		}
		else
			return Song();
		mpd_finishCommand(itsConnection);
	}
	else
		return Song();
}

void Connection::GetPlaylistContent(const string &path, SongList &v) const
{
	if (isConnected)
	{
		mpd_sendListPlaylistInfoCommand(itsConnection, path.c_str());
		mpd_InfoEntity *item = NULL;
		while ((item = mpd_getNextInfoEntity(itsConnection)) != NULL)
		{
			if (item->type == MPD_INFO_ENTITY_TYPE_SONG)
			{
				Song *s = new Song(item->info.song);
				item->info.song = 0;
				v.push_back(s);
			}
			mpd_freeInfoEntity(item);
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::SetRepeat(bool mode) const
{
	if (isConnected)
	{
		mpd_sendRepeatCommand(itsConnection, mode);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::SetRandom(bool mode) const
{
	if (isConnected)
	{
		mpd_sendRandomCommand(itsConnection, mode);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::SetSingle(bool mode) const
{
	if (isConnected)
	{
		mpd_sendSingleCommand(itsConnection, mode);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::SetConsume(bool mode) const
{
	if (isConnected)
	{
		mpd_sendConsumeCommand(itsConnection, mode);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::SetVolume(int vol)
{
	if (isConnected && vol >= 0 && vol <= 100)
	{
		mpd_sendSetvolCommand(itsConnection, vol);
		mpd_finishCommand(itsConnection);
		if (!itsConnection->error && itsUpdater)
		{
			itsCurrentStatus->volume = vol;
			StatusChanges ch;
			ch.Volume = 1;
			itsUpdater(this, ch, itsStatusUpdaterUserdata);
		}
	}
}

void Connection::SetCrossfade(int crossfade) const
{
	if (isConnected)
	{
		mpd_sendCrossfadeCommand(itsConnection, crossfade);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

int Connection::AddSong(const string &path)
{
	int id = -1;
	if (isConnected)
	{
		if (GetPlaylistLength() < itsMaxPlaylistLength)
		{
			id = mpd_sendAddIdCommand(itsConnection, path.c_str());
			if (!isCommandsListEnabled)
			{
				mpd_finishCommand(itsConnection);
				UpdateStatus();
			}
			else
				id = 0;
		}
		else if (itsErrorHandler)
				itsErrorHandler(this, MPD_ACK_ERROR_PLAYLIST_MAX, Message::FullPlaylist, NULL);
	}
	return id;
}

int Connection::AddSong(const Song &s)
{
	return !s.Empty() ? (AddSong((!s.IsFromDB() ? "file://" : "") + (s.Localized() ? locale_to_utf_cpy(s.GetFile()) : s.GetFile()))) : -1;
}

bool Connection::AddRandomSongs(size_t number)
{
	if (!isConnected && !number)
		return false;
	
	TagList files;
	
	mpd_sendListallCommand(itsConnection, "/");
	while (char *file = mpd_getNextTag(itsConnection, MPD_TAG_ITEM_FILENAME))
	{
		files.push_back(file);
		delete [] file;
	}
	mpd_finishCommand(itsConnection);
	
	if (number > files.size())
	{
		if (itsErrorHandler)
			itsErrorHandler(this, 0, "Requested number of random songs is bigger than size of your library!", itsErrorHandlerUserdata);
		return false;
	}
	else
	{
		srand(time(0));
		std::random_shuffle(files.begin(), files.end());
		StartCommandsList();
		TagList::const_iterator it = files.begin()+rand()%(files.size()-number);
		for (size_t i = 0; i < number && it != files.end(); ++i)
			AddSong(*it++);
		CommitCommandsList();
	}
	return true;
}

void Connection::Delete(int pos) const
{
	if (isConnected)
	{
		mpd_sendDeleteCommand(itsConnection, pos);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::DeleteID(int id) const
{
	if (isConnected)
	{
		mpd_sendDeleteIdCommand(itsConnection, id);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::Delete(const string &playlist, int pos) const
{
	if (isConnected)
	{
		mpd_sendPlaylistDeleteCommand(itsConnection, playlist.c_str(), pos);
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

void Connection::StartCommandsList()
{
	if (isConnected)
	{
		mpd_sendCommandListBegin(itsConnection);
		isCommandsListEnabled = 1;
	}
	
}

void Connection::CommitCommandsList()
{
	if (isConnected)
	{
		mpd_sendCommandListEnd(itsConnection);
		mpd_finishCommand(itsConnection);
		UpdateStatus();
		if (GetPlaylistLength() == itsMaxPlaylistLength && itsErrorHandler)
			itsErrorHandler(this, MPD_ACK_ERROR_PLAYLIST_MAX, Message::FullPlaylist, NULL);
		isCommandsListEnabled = 0;
	}
}

void Connection::DeletePlaylist(const string &name) const
{
	if (isConnected)
	{
		mpd_sendRmCommand(itsConnection, name.c_str());
		if (!isCommandsListEnabled)
			mpd_finishCommand(itsConnection);
	}
}

bool Connection::SavePlaylist(const string &name) const
{
	if (isConnected)
	{
		mpd_sendSaveCommand(itsConnection, name.c_str());
		mpd_finishCommand(itsConnection);
		return !(itsConnection->error == MPD_ERROR_ACK && itsConnection->errorCode == MPD_ACK_ERROR_EXIST);
	}
	else
		return false;
}

void Connection::GetPlaylists(TagList &v) const
{
	if (isConnected)
	{
		ItemList list;
		GetDirectory("/", list);
		for (ItemList::const_iterator it = list.begin(); it != list.end(); ++it)
		{
			if (it->type == itPlaylist)
				v.push_back(it->name);
		}
		FreeItemList(list);
	}
}

void Connection::GetList(TagList &v, mpd_TagItems type) const
{
	if (isConnected)
	{
		mpd_sendListCommand(itsConnection, type, NULL);
		char *item;
		while ((item = mpd_getNextTag(itsConnection, type)) != NULL)
		{
			if (item[0] != 0) // do not push empty item
				v.push_back(item);
			delete [] item;
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::GetAlbums(const string &artist, TagList &v) const
{
	if (isConnected)
	{
		mpd_sendListCommand(itsConnection, MPD_TABLE_ALBUM, artist.empty() ? NULL : artist.c_str());
		char *item;
		while ((item = mpd_getNextAlbum(itsConnection)) != NULL)
		{
			if (item[0] != 0) // do not push empty item
				v.push_back(item);
			delete [] item;
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::StartSearch(bool exact_match) const
{
	if (isConnected)
		mpd_startSearch(itsConnection, exact_match);
}

void Connection::StartFieldSearch(mpd_TagItems item)
{
	if (isConnected)
	{
		itsSearchedField = item;
		mpd_startFieldSearch(itsConnection, item);
	}
}

void Connection::AddSearch(mpd_TagItems item, const string &str) const
{
	// mpd version < 0.14.* doesn't support empty search constraints
	if (Version() < 14 && str.empty())
		return;
	if (isConnected)
		mpd_addConstraintSearch(itsConnection, item, str.c_str());
}

void Connection::CommitSearch(SongList &v) const
{
	if (isConnected)
	{
		mpd_commitSearch(itsConnection);
		mpd_InfoEntity *item = NULL;
		while ((item = mpd_getNextInfoEntity(itsConnection)) != NULL)
		{
			if (item->type == MPD_INFO_ENTITY_TYPE_SONG)
			{
				Song *s = new Song(item->info.song);
				item->info.song = 0;
				v.push_back(s);
			}
			mpd_freeInfoEntity(item);
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::CommitSearch(TagList &v) const
{
	if (isConnected)
	{
		mpd_commitSearch(itsConnection);
		char *tag = NULL;
		while ((tag = mpd_getNextTag(itsConnection, itsSearchedField)) != NULL)
		{
			if (tag[0] != 0) // do not push empty item
				v.push_back(tag);
			delete [] tag;
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::GetDirectory(const string &path, ItemList &v) const
{
	if (isConnected)
	{
		mpd_InfoEntity *item = NULL;
		mpd_sendLsInfoCommand(itsConnection, path.c_str());
		while ((item = mpd_getNextInfoEntity(itsConnection)) != NULL)
		{
			Item i;
			switch (item->type)
			{
				case MPD_INFO_ENTITY_TYPE_DIRECTORY:
					i.name = item->info.directory->path;
					i.type = itDirectory;
					break;
				case MPD_INFO_ENTITY_TYPE_SONG:
					i.song = new Song(item->info.song);
					item->info.song = 0;
					i.type = itSong;
					break;
				case MPD_INFO_ENTITY_TYPE_PLAYLISTFILE:
					i.name = item->info.playlistFile->path;
					i.type = itPlaylist;
					break;
			}
			v.push_back(i);
			mpd_freeInfoEntity(item);
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::GetDirectoryRecursive(const string &path, SongList &v) const
{
	if (isConnected)
	{
		mpd_InfoEntity *item = NULL;
		mpd_sendListallInfoCommand(itsConnection, path.c_str());
		while ((item = mpd_getNextInfoEntity(itsConnection)) != NULL)
		{
			if (item->type == MPD_INFO_ENTITY_TYPE_SONG)
			{
				Song *s = new Song(item->info.song);
				item->info.song = 0;
				v.push_back(s);
			}
			mpd_freeInfoEntity(item);
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::GetSongs(const string &path, SongList &v) const
{
	if (isConnected)
	{
		mpd_InfoEntity *item = NULL;
		mpd_sendLsInfoCommand(itsConnection, path.c_str());
		while ((item = mpd_getNextInfoEntity(itsConnection)) != NULL)
		{
			if (item->type == MPD_INFO_ENTITY_TYPE_SONG)
			{
				Song *s = new Song(item->info.song);
				item->info.song = 0;
				v.push_back(s);
			}
			mpd_freeInfoEntity(item);
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::GetDirectories(const string &path, TagList &v) const
{
	if (isConnected)
	{
		mpd_InfoEntity *item = NULL;
		mpd_sendLsInfoCommand(itsConnection, path.c_str());
		while ((item = mpd_getNextInfoEntity(itsConnection)) != NULL)
		{
			if (item->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
				v.push_back(item->info.directory->path);
			mpd_freeInfoEntity(item);
		}
		mpd_finishCommand(itsConnection);
	}
}

void Connection::GetOutputs(OutputList &v) const
{
	if (!isConnected)
		return;
	mpd_sendOutputsCommand(itsConnection);
	while (mpd_OutputEntity *output = mpd_getNextOutput(itsConnection))
	{
		v.push_back(std::make_pair(output->name, output->enabled));
		mpd_freeOutputElement(output);
	}
	mpd_finishCommand(itsConnection);
}

bool Connection::EnableOutput(int id)
{
	if (!isConnected)
		return false;
	mpd_sendEnableOutputCommand(itsConnection, id);
	mpd_finishCommand(itsConnection);
	return !CheckForErrors();
}

bool Connection::DisableOutput(int id)
{
	if (!isConnected)
		return false;
	mpd_sendDisableOutputCommand(itsConnection, id);
	mpd_finishCommand(itsConnection);
	return !CheckForErrors();
}

int Connection::CheckForErrors()
{
	itsErrorCode = 0;
	if (itsConnection->error)
	{
		itsErrorMessage = itsConnection->errorStr;
		if (itsConnection->error == MPD_ERROR_ACK)
		{
			// this is to avoid setting too small max size as we check it before fetching current status
			// setting real max playlist length is in UpdateStatus()
			if (itsConnection->errorCode == MPD_ACK_ERROR_PLAYLIST_MAX && itsMaxPlaylistLength == size_t(-1))
				itsMaxPlaylistLength = 0;
			
			if (itsErrorHandler)
				itsErrorHandler(this, itsConnection->errorCode, itsConnection->errorStr, itsErrorHandlerUserdata);
			itsErrorCode = itsConnection->errorCode;
		}
		else
		{
			if (itsErrorHandler)
				itsErrorHandler(this, itsConnection->error, itsConnection->errorStr, itsErrorHandlerUserdata);
			itsErrorCode = itsConnection->error;
			Disconnect(); // the rest of errors are fatal to connection
		}
		if (itsConnection)
			mpd_clearError(itsConnection);
	}
	return itsErrorCode;
}

void MPD::FreeSongList(SongList &l)
{
	for (SongList::iterator i = l.begin(); i != l.end(); ++i)
		delete *i;
	l.clear();
}

void MPD::FreeItemList(ItemList &l)
{
	for (ItemList::iterator i = l.begin(); i != l.end(); ++i)
		delete i->song;
	l.clear();
}

