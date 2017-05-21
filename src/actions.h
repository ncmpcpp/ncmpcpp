/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_ACTIONS_H
#define NCMPCPP_ACTIONS_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/format.hpp>
#include <map>
#include <string>
#include "curses/window.h"
#include "interfaces.h"

// forward declarations
struct SongList;

namespace Actions {

enum class Type
{
	MacroUtility = -1,
	Dummy,
	UpdateEnvironment,
	MouseEvent,
	ScrollUp,
	ScrollDown,
	ScrollUpArtist,
	ScrollUpAlbum,
	ScrollDownArtist,
	ScrollDownAlbum,
	PageUp,
	PageDown,
	MoveHome,
	MoveEnd,
	ToggleInterface,
	JumpToParentDirectory,
	RunAction,
	PreviousColumn,
	NextColumn,
	MasterScreen,
	SlaveScreen,
	VolumeUp,
	VolumeDown,
	AddItemToPlaylist,
	PlayItem,
	DeletePlaylistItems,
	DeleteStoredPlaylist,
	DeleteBrowserItems,
	ReplaySong,
	Previous,
	Next,
	Pause,
	Stop,
	ExecuteCommand,
	SavePlaylist,
	MoveSortOrderUp,
	MoveSortOrderDown,
	MoveSelectedItemsUp,
	MoveSelectedItemsDown,
	MoveSelectedItemsTo,
	Add,
	SeekForward,
	SeekBackward,
	ToggleDisplayMode,
	ToggleSeparatorsBetweenAlbums,
	ToggleLyricsUpdateOnSongChange,
	ToggleLyricsFetcher,
	ToggleFetchingLyricsInBackground,
	TogglePlayingSongCentering,
	UpdateDatabase,
	JumpToPlayingSong,
	ToggleRepeat,
	Shuffle,
	ToggleRandom,
	StartSearching,
	SaveTagChanges,
	ToggleSingle,
	ToggleConsume,
	ToggleCrossfade,
	SetCrossfade,
	SetVolume,
	EnterDirectory,
	EditSong,
	EditLibraryTag,
	EditLibraryAlbum,
	EditDirectoryName,
	EditPlaylistName,
	EditLyrics,
	JumpToBrowser,
	JumpToMediaLibrary,
	JumpToPlaylistEditor,
	ToggleScreenLock,
	JumpToTagEditor,
	JumpToPositionInSong,
	SelectItem,
	SelectRange,
	ReverseSelection,
	RemoveSelection,
	SelectAlbum,
	SelectFoundItems,
	AddSelectedItems,
	CropMainPlaylist,
	CropPlaylist,
	ClearMainPlaylist,
	ClearPlaylist,
	SortPlaylist,
	ReversePlaylist,
	ApplyFilter,
	Find,
	FindItemForward,
	FindItemBackward,
	NextFoundItem,
	PreviousFoundItem,
	ToggleFindMode,
	ToggleReplayGainMode,
	ToggleAddMode,
	ToggleMouse,
	ToggleBitrateVisibility,
	AddRandomItems,
	ToggleBrowserSortMode,
	ToggleLibraryTagType,
	ToggleMediaLibrarySortMode,
	FetchLyricsInBackground,
	RefetchLyrics,
	SetSelectedItemsPriority,
	ToggleOutput,
	ToggleVisualizationType,
	ShowSongInfo,
	ShowArtistInfo,
	ShowLyrics,
	Quit,
	NextScreen,
	PreviousScreen,
	ShowHelp,
	ShowPlaylist,
	ShowBrowser,
	ChangeBrowseMode,
	ShowSearchEngine,
	ResetSearchEngine,
	ShowMediaLibrary,
	ToggleMediaLibraryColumnsMode,
	ShowPlaylistEditor,
	ShowTagEditor,
	ShowOutputs,
	ShowVisualizer,
	ShowClock,
	ShowServerInfo,
	_numberOfActions // needed to dynamically calculate size of action array
};

void validateScreenSize();
void initializeScreens();
void setResizeFlags();
void resizeScreen(bool reload_main_window);
void setWindowsDimensions();

void confirmAction(const boost::format &description);
inline void confirmAction(const std::string &description)
{
	confirmAction(boost::format(description));
}

bool isMPDMusicDirSet();

extern bool OriginalStatusbarVisibility;
extern bool ExitMainLoop;

extern size_t HeaderHeight;
extern size_t FooterHeight;
extern size_t FooterStartY;

struct BaseAction
{
	BaseAction(Type type_, const char *name_): m_name(name_), m_type(type_) { }

	virtual ~BaseAction() { }

	const std::string &name() const { return m_name; }
	Type type() const { return m_type; }
	
	virtual bool canBeRun() { return true; }
	
	bool execute()
	{
		if (canBeRun())
		{
			run();
			return true;
		}
		return false;
	}

protected:
	std::string m_name;

private:
	virtual void run() = 0;

	Type m_type;
};

BaseAction &get(Type at);

std::shared_ptr<BaseAction> get_(Type at);
std::shared_ptr<BaseAction> get_(const std::string &name);

struct Dummy: BaseAction
{
	Dummy(): BaseAction(Type::Dummy, "dummy") { }
	
private:
	virtual void run() override { }
};

struct UpdateEnvironment: BaseAction
{
	UpdateEnvironment();

	void run(bool update_status, bool refresh_window, bool mpd_sync);

private:
	boost::posix_time::ptime m_past;

	virtual void run() override;
};

struct MouseEvent: BaseAction
{
	MouseEvent(): BaseAction(Type::MouseEvent, "mouse_event")
	{
		m_old_mouse_event.bstate = 0;
		m_mouse_event.bstate = 0;
	}
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
	
	MEVENT m_mouse_event;
	MEVENT m_old_mouse_event;
};

struct ScrollUp: BaseAction
{
	ScrollUp(): BaseAction(Type::ScrollUp, "scroll_up") { }
	
private:
	virtual void run() override;
};

struct ScrollDown: BaseAction
{
	ScrollDown(): BaseAction(Type::ScrollDown, "scroll_down") { }
	
private:
	virtual void run() override;
};

struct ScrollUpArtist: BaseAction
{
	ScrollUpArtist(): BaseAction(Type::ScrollUpArtist, "scroll_up_artist") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	const SongList *m_songs;
};

struct ScrollUpAlbum: BaseAction
{
	ScrollUpAlbum(): BaseAction(Type::ScrollUpAlbum, "scroll_up_album") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	const SongList *m_songs;
};

struct ScrollDownArtist: BaseAction
{
	ScrollDownArtist(): BaseAction(Type::ScrollDownArtist, "scroll_down_artist") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	const SongList *m_songs;
};

struct ScrollDownAlbum: BaseAction
{
	ScrollDownAlbum(): BaseAction(Type::ScrollDownAlbum, "scroll_down_album") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	const SongList *m_songs;
};

struct PageUp: BaseAction
{
	PageUp(): BaseAction(Type::PageUp, "page_up") { }
	
private:
	virtual void run() override;
};

struct PageDown: BaseAction
{
	PageDown(): BaseAction(Type::PageDown, "page_down") { }
	
private:
	virtual void run() override;
};

struct MoveHome: BaseAction
{
	MoveHome(): BaseAction(Type::MoveHome, "move_home") { }
	
private:
	virtual void run() override;
};

struct MoveEnd: BaseAction
{
	MoveEnd(): BaseAction(Type::MoveEnd, "move_end") { }
	
private:
	virtual void run() override;
};

struct ToggleInterface: BaseAction
{
	ToggleInterface(): BaseAction(Type::ToggleInterface, "toggle_interface") { }
	
private:
	virtual void run() override;
};

struct JumpToParentDirectory: BaseAction
{
	JumpToParentDirectory(): BaseAction(Type::JumpToParentDirectory, "jump_to_parent_directory") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct RunAction: BaseAction
{
	RunAction(): BaseAction(Type::RunAction, "run_action") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasActions *m_ha;
};

struct PreviousColumn: BaseAction
{
	PreviousColumn(): BaseAction(Type::PreviousColumn, "previous_column") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasColumns *m_hc;
};

struct NextColumn: BaseAction
{
	NextColumn(): BaseAction(Type::NextColumn, "next_column") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasColumns *m_hc;
};

struct MasterScreen: BaseAction
{
	MasterScreen(): BaseAction(Type::MasterScreen, "master_screen") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SlaveScreen: BaseAction
{
	SlaveScreen(): BaseAction(Type::SlaveScreen, "slave_screen") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct VolumeUp: BaseAction
{
	VolumeUp(): BaseAction(Type::VolumeUp, "volume_up") { }
	
private:
	virtual void run() override;
};

struct VolumeDown: BaseAction
{
	VolumeDown(): BaseAction(Type::VolumeDown, "volume_down") { }
	
private:
	virtual void run() override;
};

struct AddItemToPlaylist: BaseAction
{
	AddItemToPlaylist(): BaseAction(Type::AddItemToPlaylist, "add_item_to_playlist") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasSongs *m_hs;
};

struct PlayItem: BaseAction
{
	PlayItem(): BaseAction(Type::PlayItem, "play_item") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasSongs *m_hs;
};

struct DeletePlaylistItems: BaseAction
{
	DeletePlaylistItems(): BaseAction(Type::DeletePlaylistItems, "delete_playlist_items") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct DeleteStoredPlaylist: BaseAction
{
	DeleteStoredPlaylist(): BaseAction(Type::DeleteStoredPlaylist, "delete_stored_playlist") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct DeleteBrowserItems: BaseAction
{
	DeleteBrowserItems(): BaseAction(Type::DeleteBrowserItems, "delete_browser_items") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ReplaySong: BaseAction
{
	ReplaySong(): BaseAction(Type::ReplaySong, "replay_song") { }
	
private:
	virtual void run() override;
};

struct PreviousSong: BaseAction
{
	PreviousSong(): BaseAction(Type::Previous, "previous") { }
	
private:
	virtual void run() override;
};

struct NextSong: BaseAction
{
	NextSong(): BaseAction(Type::Next, "next") { }
	
private:
	virtual void run() override;
};

struct Pause: BaseAction
{
	Pause(): BaseAction(Type::Pause, "pause") { }
	
private:
	virtual void run() override;
};

struct Stop: BaseAction
{
	Stop(): BaseAction(Type::Stop, "stop") { }
	
private:
	virtual void run() override;
};

struct ExecuteCommand: BaseAction
{
	ExecuteCommand(): BaseAction(Type::ExecuteCommand, "execute_command") { }
	
private:
	virtual void run() override;
};

struct SavePlaylist: BaseAction
{
	SavePlaylist(): BaseAction(Type::SavePlaylist, "save_playlist") { }
	
private:
	virtual void run() override;
};

struct MoveSortOrderUp: BaseAction
{
	MoveSortOrderUp(): BaseAction(Type::MoveSortOrderUp, "move_sort_order_up") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct MoveSortOrderDown: BaseAction
{
	MoveSortOrderDown(): BaseAction(Type::MoveSortOrderDown, "move_sort_order_down") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct MoveSelectedItemsUp: BaseAction
{
	MoveSelectedItemsUp(): BaseAction(Type::MoveSelectedItemsUp, "move_selected_items_up") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct MoveSelectedItemsDown: BaseAction
{
	MoveSelectedItemsDown(): BaseAction(Type::MoveSelectedItemsDown, "move_selected_items_down") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct MoveSelectedItemsTo: BaseAction
{
	MoveSelectedItemsTo(): BaseAction(Type::MoveSelectedItemsTo, "move_selected_items_to") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct Add: BaseAction
{
	Add(): BaseAction(Type::Add, "add") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SeekForward: BaseAction
{
	SeekForward(): BaseAction(Type::SeekForward, "seek_forward") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SeekBackward: BaseAction
{
	SeekBackward(): BaseAction(Type::SeekBackward, "seek_backward") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleDisplayMode: BaseAction
{
	ToggleDisplayMode(): BaseAction(Type::ToggleDisplayMode, "toggle_display_mode") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleSeparatorsBetweenAlbums: BaseAction
{
	ToggleSeparatorsBetweenAlbums()
	: BaseAction(Type::ToggleSeparatorsBetweenAlbums, "toggle_separators_between_albums") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleLyricsUpdateOnSongChange: BaseAction
{
	ToggleLyricsUpdateOnSongChange()
	: BaseAction(Type::ToggleLyricsUpdateOnSongChange, "toggle_lyrics_update_on_song_change") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleLyricsFetcher: BaseAction
{
	ToggleLyricsFetcher(): BaseAction(Type::ToggleLyricsFetcher, "toggle_lyrics_fetcher") { }
	
private:
	virtual void run() override;
};

struct ToggleFetchingLyricsInBackground: BaseAction
{
	ToggleFetchingLyricsInBackground()
	: BaseAction(Type::ToggleFetchingLyricsInBackground, "toggle_fetching_lyrics_in_background") { }
	
private:
	virtual void run() override;
};

struct TogglePlayingSongCentering: BaseAction
{
	TogglePlayingSongCentering()
	: BaseAction(Type::TogglePlayingSongCentering, "toggle_playing_song_centering") { }
	
private:
	virtual void run() override;
};

struct UpdateDatabase: BaseAction
{
	UpdateDatabase(): BaseAction(Type::UpdateDatabase, "update_database") { }
	
private:
	virtual void run() override;
};

struct JumpToPlayingSong: BaseAction
{
	JumpToPlayingSong(): BaseAction(Type::JumpToPlayingSong, "jump_to_playing_song") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	MPD::Song m_song;
};

struct ToggleRepeat: BaseAction
{
	ToggleRepeat(): BaseAction(Type::ToggleRepeat, "toggle_repeat") { }
	
private:
	virtual void run() override;
};

struct Shuffle: BaseAction
{
	Shuffle(): BaseAction(Type::Shuffle, "shuffle") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::Menu<MPD::Song>::ConstIterator m_begin;
	NC::Menu<MPD::Song>::ConstIterator m_end;
};

struct ToggleRandom: BaseAction
{
	ToggleRandom(): BaseAction(Type::ToggleRandom, "toggle_random") { }
	
private:
	virtual void run() override;
};

struct StartSearching: BaseAction
{
	StartSearching(): BaseAction(Type::StartSearching, "start_searching") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SaveTagChanges: BaseAction
{
	SaveTagChanges(): BaseAction(Type::SaveTagChanges, "save_tag_changes") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleSingle: BaseAction
{
	ToggleSingle(): BaseAction(Type::ToggleSingle, "toggle_single") { }
	
private:
	virtual void run() override;
};

struct ToggleConsume: BaseAction
{
	ToggleConsume(): BaseAction(Type::ToggleConsume, "toggle_consume") { }
	
private:
	virtual void run() override;
};

struct ToggleCrossfade: BaseAction
{
	ToggleCrossfade(): BaseAction(Type::ToggleCrossfade, "toggle_crossfade") { }
	
private:
	virtual void run() override;
};

struct SetCrossfade: BaseAction
{
	SetCrossfade(): BaseAction(Type::SetCrossfade, "set_crossfade") { }
	
private:
	virtual void run() override;
};

struct SetVolume: BaseAction
{
	SetVolume(): BaseAction(Type::SetVolume, "set_volume") { }
	
private:
	virtual void run() override;
};

struct EnterDirectory: BaseAction
{
	EnterDirectory(): BaseAction(Type::EnterDirectory, "enter_directory") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;
};


struct EditSong: BaseAction
{
	EditSong(): BaseAction(Type::EditSong, "edit_song") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

#ifdef HAVE_TAGLIB_H
	const MPD::Song *m_song;
#endif // HAVE_TAGLIB_H
};

struct EditLibraryTag: BaseAction
{
	EditLibraryTag(): BaseAction(Type::EditLibraryTag, "edit_library_tag") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EditLibraryAlbum: BaseAction
{
	EditLibraryAlbum(): BaseAction(Type::EditLibraryAlbum, "edit_library_album") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EditDirectoryName: BaseAction
{
	EditDirectoryName(): BaseAction(Type::EditDirectoryName, "edit_directory_name") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EditPlaylistName: BaseAction
{
	EditPlaylistName(): BaseAction(Type::EditPlaylistName, "edit_playlist_name") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct EditLyrics: BaseAction
{
	EditLyrics(): BaseAction(Type::EditLyrics, "edit_lyrics") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct JumpToBrowser: BaseAction
{
	JumpToBrowser(): BaseAction(Type::JumpToBrowser, "jump_to_browser") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	const MPD::Song *m_song;
};

struct JumpToMediaLibrary: BaseAction
{
	JumpToMediaLibrary(): BaseAction(Type::JumpToMediaLibrary, "jump_to_media_library") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	const MPD::Song *m_song;
};

struct JumpToPlaylistEditor: BaseAction
{
	JumpToPlaylistEditor(): BaseAction(Type::JumpToPlaylistEditor, "jump_to_playlist_editor") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleScreenLock: BaseAction
{
	ToggleScreenLock(): BaseAction(Type::ToggleScreenLock, "toggle_screen_lock") { }
	
private:
	virtual void run() override;
};

struct JumpToTagEditor: BaseAction
{
	JumpToTagEditor(): BaseAction(Type::JumpToTagEditor, "jump_to_tag_editor") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

#ifdef HAVE_TAGLIB_H
	const MPD::Song *m_song;
#endif // HAVE_TAGLIB_H
};

struct JumpToPositionInSong: BaseAction
{
	JumpToPositionInSong(): BaseAction(Type::JumpToPositionInSong, "jump_to_position_in_song") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SelectItem: BaseAction
{
	SelectItem(): BaseAction(Type::SelectItem, "select_item") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
};

struct SelectRange: BaseAction
{
	SelectRange(): BaseAction(Type::SelectRange, "select_range") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	NC::List::Iterator m_begin;
	NC::List::Iterator m_end;
};

struct ReverseSelection: BaseAction
{
	ReverseSelection(): BaseAction(Type::ReverseSelection, "reverse_selection") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
};

struct RemoveSelection: BaseAction
{
	RemoveSelection(): BaseAction(Type::RemoveSelection, "remove_selection") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
};

struct SelectAlbum: BaseAction
{
	SelectAlbum(): BaseAction(Type::SelectAlbum, "select_album") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	SongList *m_songs;
};

struct SelectFoundItems: BaseAction
{
	SelectFoundItems(): BaseAction(Type::SelectFoundItems, "select_found_items") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::List *m_list;
	Searchable *m_searchable;
};

struct AddSelectedItems: BaseAction
{
	AddSelectedItems(): BaseAction(Type::AddSelectedItems, "add_selected_items") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct CropMainPlaylist: BaseAction
{
	CropMainPlaylist(): BaseAction(Type::CropMainPlaylist, "crop_main_playlist") { }
	
private:
	virtual void run() override;
};

struct CropPlaylist: BaseAction
{
	CropPlaylist(): BaseAction(Type::CropPlaylist, "crop_playlist") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ClearMainPlaylist: BaseAction
{
	ClearMainPlaylist(): BaseAction(Type::ClearMainPlaylist, "clear_main_playlist") { }
	
private:
	virtual void run() override;
};

struct ClearPlaylist: BaseAction
{
	ClearPlaylist(): BaseAction(Type::ClearPlaylist, "clear_playlist") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SortPlaylist: BaseAction
{
	SortPlaylist(): BaseAction(Type::SortPlaylist, "sort_playlist") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ReversePlaylist: BaseAction
{
	ReversePlaylist(): BaseAction(Type::ReversePlaylist, "reverse_playlist") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	NC::Menu<MPD::Song>::ConstIterator m_begin;
	NC::Menu<MPD::Song>::ConstIterator m_end;
};

struct ApplyFilter: public BaseAction
{
	ApplyFilter(): BaseAction(Type::ApplyFilter, "apply_filter") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	Filterable *m_filterable;
};

struct Find: BaseAction
{
	Find(): BaseAction(Type::Find, "find") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct FindItemForward: BaseAction
{
	FindItemForward(): BaseAction(Type::FindItemForward, "find_item_forward") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct FindItemBackward: BaseAction
{
	FindItemBackward(): BaseAction(Type::FindItemBackward, "find_item_backward") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct NextFoundItem: BaseAction
{
	NextFoundItem(): BaseAction(Type::NextFoundItem, "next_found_item") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct PreviousFoundItem: BaseAction
{
	PreviousFoundItem(): BaseAction(Type::PreviousFoundItem, "previous_found_item") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleFindMode: BaseAction
{
	ToggleFindMode(): BaseAction(Type::ToggleFindMode, "toggle_find_mode") { }
	
private:
	virtual void run() override;
};

struct ToggleReplayGainMode: BaseAction
{
	ToggleReplayGainMode(): BaseAction(Type::ToggleReplayGainMode, "toggle_replay_gain_mode") { }
	
private:
	virtual void run() override;
};

struct ToggleAddMode: BaseAction
{
	ToggleAddMode(): BaseAction(Type::ToggleAddMode, "toggle_add_mode") { }
	
private:
	virtual void run() override;
};

struct ToggleMouse: BaseAction
{
	ToggleMouse(): BaseAction(Type::ToggleMouse, "toggle_mouse") { }
	
private:
	virtual void run() override;
};

struct ToggleBitrateVisibility: BaseAction
{
	ToggleBitrateVisibility(): BaseAction(Type::ToggleBitrateVisibility, "toggle_bitrate_visibility") { }
	
private:
	virtual void run() override;
};

struct AddRandomItems: BaseAction
{
	AddRandomItems(): BaseAction(Type::AddRandomItems, "add_random_items") { }
	
private:
	virtual void run() override;
};

struct ToggleBrowserSortMode: BaseAction
{
	ToggleBrowserSortMode(): BaseAction(Type::ToggleBrowserSortMode, "toggle_browser_sort_mode") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleLibraryTagType: BaseAction
{
	ToggleLibraryTagType(): BaseAction(Type::ToggleLibraryTagType, "toggle_library_tag_type") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleMediaLibrarySortMode: BaseAction
{
	ToggleMediaLibrarySortMode()
	: BaseAction(Type::ToggleMediaLibrarySortMode, "toggle_media_library_sort_mode") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct FetchLyricsInBackground: BaseAction
{
	FetchLyricsInBackground()
		: BaseAction(Type::FetchLyricsInBackground, "fetch_lyrics_in_background") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;

	HasSongs *m_hs;
};

struct RefetchLyrics: BaseAction
{
	RefetchLyrics(): BaseAction(Type::RefetchLyrics, "refetch_lyrics") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct SetSelectedItemsPriority: BaseAction
{
	SetSelectedItemsPriority()
	: BaseAction(Type::SetSelectedItemsPriority, "set_selected_items_priority") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleOutput: BaseAction
{
	ToggleOutput(): BaseAction(Type::ToggleOutput, "toggle_output") { }

private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleVisualizationType: BaseAction
{
	ToggleVisualizationType()
	: BaseAction(Type::ToggleVisualizationType, "toggle_visualization_type") { }

private:
	
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowSongInfo: BaseAction
{
	ShowSongInfo(): BaseAction(Type::ShowSongInfo, "show_song_info") { }
	
private:
	virtual void run() override;
};

struct ShowArtistInfo: BaseAction
{
	ShowArtistInfo(): BaseAction(Type::ShowArtistInfo, "show_artist_info") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowLyrics: BaseAction
{
	ShowLyrics(): BaseAction(Type::ShowLyrics, "show_lyrics") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;

	const MPD::Song *m_song;
};

struct Quit: BaseAction
{
	Quit(): BaseAction(Type::Quit, "quit") { }
	
private:
	virtual void run() override;
};

struct NextScreen: BaseAction
{
	NextScreen(): BaseAction(Type::NextScreen, "next_screen") { }
	
private:
	virtual void run() override;
};

struct PreviousScreen: BaseAction
{
	PreviousScreen(): BaseAction(Type::PreviousScreen, "previous_screen") { }
	
private:
	virtual void run() override;
};

struct ShowHelp: BaseAction
{
	ShowHelp(): BaseAction(Type::ShowHelp, "show_help") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowPlaylist: BaseAction
{
	ShowPlaylist(): BaseAction(Type::ShowPlaylist, "show_playlist") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowBrowser: BaseAction
{
	ShowBrowser(): BaseAction(Type::ShowBrowser, "show_browser") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ChangeBrowseMode: BaseAction
{
	ChangeBrowseMode(): BaseAction(Type::ChangeBrowseMode, "change_browse_mode") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowSearchEngine: BaseAction
{
	ShowSearchEngine(): BaseAction(Type::ShowSearchEngine, "show_search_engine") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ResetSearchEngine: BaseAction
{
	ResetSearchEngine(): BaseAction(Type::ResetSearchEngine, "reset_search_engine") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowMediaLibrary: BaseAction
{
	ShowMediaLibrary(): BaseAction(Type::ShowMediaLibrary, "show_media_library") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ToggleMediaLibraryColumnsMode: BaseAction
{
	ToggleMediaLibraryColumnsMode()
	: BaseAction(Type::ToggleMediaLibraryColumnsMode, "toggle_media_library_columns_mode") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowPlaylistEditor: BaseAction
{
	ShowPlaylistEditor(): BaseAction(Type::ShowPlaylistEditor, "show_playlist_editor") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowTagEditor: BaseAction
{
	ShowTagEditor(): BaseAction(Type::ShowTagEditor, "show_tag_editor") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowOutputs: BaseAction
{
	ShowOutputs(): BaseAction(Type::ShowOutputs, "show_outputs") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowVisualizer: BaseAction
{
	ShowVisualizer(): BaseAction(Type::ShowVisualizer, "show_visualizer") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowClock: BaseAction
{
	ShowClock(): BaseAction(Type::ShowClock, "show_clock") { }
	
private:
	virtual bool canBeRun() override;
	virtual void run() override;
};

struct ShowServerInfo: BaseAction
{
	ShowServerInfo(): BaseAction(Type::ShowServerInfo, "show_server_info") { }
	
private:
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() override;
#	endif // HAVE_TAGLIB_H
	virtual void run() override;
};

}

#endif // NCMPCPP_ACTIONS_H
