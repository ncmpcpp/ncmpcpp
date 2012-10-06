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

#ifndef NCMPCPP_ACTIONS_H
#define NCMPCPP_ACTIONS_H

#include <map>
#include <string>
#include "window.h"

enum ActionType
{
	aMacroUtility,
	aDummy, aMouseEvent, aScrollUp, aScrollDown, aScrollUpArtist, aScrollUpAlbum,
	aScrollDownArtist, aScrollDownAlbum, aPageUp, aPageDown, aMoveHome, aMoveEnd,
	aToggleInterface, aJumpToParentDirectory, aPressEnter, aPressSpace, aPreviousColumn,
	aNextColumn, aMasterScreen, aSlaveScreen, aVolumeUp, aVolumeDown, aDeletePlaylistItems,
	aDeleteStoredPlaylist, aDeleteBrowserItems, aReplaySong, aPrevious, aNext, aPause,
	aStop, aExecuteCommand, aSavePlaylist, aMoveSortOrderUp, aMoveSortOrderDown,
	aMoveSelectedItemsUp, aMoveSelectedItemsDown, aMoveSelectedItemsTo, aAdd,
	aSeekForward, aSeekBackward, aToggleDisplayMode, aToggleSeparatorsBetweenAlbums,
	aToggleLyricsFetcher, aToggleFetchingLyricsInBackground, aTogglePlayingSongCentering,
	aUpdateDatabase, aJumpToPlayingSong, aToggleRepeat, aShuffle, aToggleRandom,
	aStartSearching, aSaveTagChanges, aToggleSingle, aToggleConsume, aToggleCrossfade,
	aSetCrossfade, aEditSong, aEditLibraryTag, aEditLibraryAlbum, aEditDirectoryName,
	aEditPlaylistName, aEditLyrics, aJumpToBrowser, aJumpToMediaLibrary,
	aJumpToPlaylistEditor, aToggleScreenLock, aJumpToTagEditor, aJumpToPositionInSong,
	aReverseSelection, aRemoveSelection, aSelectAlbum, aAddSelectedItems,
	aCropMainPlaylist, aCropPlaylist, aClearMainPlaylist, aClearPlaylist, aSortPlaylist,
	aReversePlaylist, aApplyFilter, aFind, aFindItemForward, aFindItemBackward,
	aNextFoundItem, aPreviousFoundItem, aToggleFindMode, aToggleReplayGainMode,
	aToggleSpaceMode, aToggleAddMode, aToggleMouse, aToggleBitrateVisibility,
	aAddRandomItems, aToggleBrowserSortMode, aToggleLibraryTagType,
	aToggleMediaLibrarySortMode, aRefetchLyrics, aRefetchArtistInfo,
	aSetSelectedItemsPriority, aFilterPlaylistOnPriorities, aShowSongInfo,
	aShowArtistInfo, aShowLyrics, aQuit, aNextScreen, aPreviousScreen, aShowHelp,
	aShowPlaylist, aShowBrowser, aChangeBrowseMode, aShowSearchEngine,
	aResetSearchEngine, aShowMediaLibrary, aToggleMediaLibraryColumnsMode,
	aShowPlaylistEditor, aShowTagEditor, aShowOutputs, aShowVisualizer,
	aShowClock, aShowServerInfo
};

struct Action
{
	enum class Find { Forward, Backward };
	
	Action(ActionType type, const char *name) : m_type(type), m_name(name) { }
	
	const char *name() const { return m_name; }
	ActionType type() const { return m_type; }
	
	virtual bool canBeRun() const { return true; }
	
	bool execute()
	{
		if (canBeRun())
		{
			run();
			return true;
		}
		return false;
	}
	
	static void validateScreenSize();
	static void initializeScreens();
	static void setResizeFlags();
	static void resizeScreen(bool reload_main_window);
	static void setWindowsDimensions();
	
	static bool connectToMPD();
	static bool askYesNoQuestion(const std::string &question, void (*callback)());
	static bool isMPDMusicDirSet();
	
	static Action *get(ActionType at);
	static Action *get(const std::string &name);
	
	static bool OriginalStatusbarVisibility;
	static bool ExitMainLoop;
	
	static size_t HeaderHeight;
	static size_t FooterHeight;
	static size_t FooterStartY;
	
protected:
	virtual void run() = 0;
	
	static void seek();
	static void findItem(const Find direction);
	static void listsChangeFinisher();
	
private:
	ActionType m_type;
	const char *m_name;
};

struct Dummy : public Action
{
	Dummy() : Action(aDummy, "dummy") { }
	
protected:
	virtual void run() { }
};

struct MouseEvent : public Action
{
	MouseEvent() : Action(aMouseEvent, "mouse_event") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
	
	private:
		MEVENT m_mouse_event;
		MEVENT m_old_mouse_event;
};

struct ScrollUp : public Action
{
	ScrollUp() : Action(aScrollUp, "scroll_up") { }
	
protected:
	virtual void run();
};

struct ScrollDown : public Action
{
	ScrollDown() : Action(aScrollDown, "scroll_down") { }
	
protected:
	virtual void run();
};

struct ScrollUpArtist : public Action
{
	ScrollUpArtist() : Action(aScrollUpArtist, "scroll_up_artist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollUpAlbum : public Action
{
	ScrollUpAlbum() : Action(aScrollUpAlbum, "scroll_up_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollDownArtist : public Action
{
	ScrollDownArtist() : Action(aScrollDownArtist, "scroll_down_artist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollDownAlbum : public Action
{
	ScrollDownAlbum() : Action(aScrollDownAlbum, "scroll_down_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PageUp : public Action
{
	PageUp() : Action(aPageUp, "page_up") { }
	
protected:
	virtual void run();
};

struct PageDown : public Action
{
	PageDown() : Action(aPageDown, "page_down") { }
	
protected:
	virtual void run();
};

struct MoveHome : public Action
{
	MoveHome() : Action(aMoveHome, "move_home") { }
	
protected:
	virtual void run();
};

struct MoveEnd : public Action
{
	MoveEnd() : Action(aMoveEnd, "move_end") { }
	
protected:
	virtual void run();
};

struct ToggleInterface : public Action
{
	ToggleInterface() : Action(aToggleInterface, "toggle_interface") { }
	
protected:
	virtual void run();
};

struct JumpToParentDirectory : public Action
{
	JumpToParentDirectory() : Action(aJumpToParentDirectory, "jump_to_parent_directory") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PressEnter : public Action
{
	PressEnter() : Action(aPressEnter, "press_enter") { }
	
protected:
	virtual void run();
};

struct PressSpace : public Action
{
	PressSpace() : Action(aPressSpace, "press_space") { }
	
protected:
	virtual void run();
};

struct PreviousColumn : public Action
{
	PreviousColumn() : Action(aPreviousColumn, "previous_column") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct NextColumn : public Action
{
	NextColumn() : Action(aNextColumn, "next_column") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MasterScreen : public Action
{
	MasterScreen() : Action(aMasterScreen, "master_screen") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SlaveScreen : public Action
{
	SlaveScreen() : Action(aSlaveScreen, "slave_screen") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct VolumeUp : public Action
{
	VolumeUp() : Action(aVolumeUp, "volume_up") { }
	
protected:
	virtual void run();
};

struct VolumeDown : public Action
{
	VolumeDown() : Action(aVolumeDown, "volume_down") { }
	
protected:
	virtual void run();
};

struct DeletePlaylistItems : public Action
{
	DeletePlaylistItems() : Action(aDeletePlaylistItems, "delete_playlist_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct DeleteStoredPlaylist : public Action
{
	DeleteStoredPlaylist() : Action(aDeleteStoredPlaylist, "delete_stored_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct DeleteBrowserItems : public Action
{
	DeleteBrowserItems() : Action(aDeleteBrowserItems, "delete_browser_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReplaySong : public Action
{
	ReplaySong() : Action(aReplaySong, "replay_song") { }
	
protected:
	virtual void run();
};

struct PreviousSong : public Action
{
	PreviousSong() : Action(aPrevious, "previous") { }
	
protected:
	virtual void run();
};

struct NextSong : public Action
{
	NextSong() : Action(aNext, "next") { }
	
protected:
	virtual void run();
};

struct Pause : public Action
{
	Pause() : Action(aPause, "pause") { }
	
protected:
	virtual void run();
};

struct Stop : public Action
{
	Stop() : Action(aStop, "stop") { }
	
protected:
	virtual void run();
};

struct ExecuteCommand : public Action
{
	ExecuteCommand() : Action(aExecuteCommand, "execute_command") { }
	
protected:
	virtual void run();
};

struct SavePlaylist : public Action
{
	SavePlaylist() : Action(aSavePlaylist, "save_playlist") { }
	
protected:
	virtual void run();
};

struct MoveSortOrderUp : public Action
{
	MoveSortOrderUp() : Action(aMoveSortOrderUp, "move_sort_order_up") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSortOrderDown : public Action
{
	MoveSortOrderDown() : Action(aMoveSortOrderDown, "move_sort_order_down") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsUp : public Action
{
	MoveSelectedItemsUp() : Action(aMoveSelectedItemsUp, "move_selected_items_up") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsDown : public Action
{
	MoveSelectedItemsDown() : Action(aMoveSelectedItemsDown, "move_selected_items_down") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsTo : public Action
{
	MoveSelectedItemsTo() : Action(aMoveSelectedItemsTo, "move_selected_items_to") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct Add : public Action
{
	Add() : Action(aAdd, "add") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SeekForward : public Action
{
	SeekForward() : Action(aSeekForward, "seek_forward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SeekBackward : public Action
{
	SeekBackward() : Action(aSeekBackward, "seek_backward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleDisplayMode : public Action
{
	ToggleDisplayMode() : Action(aToggleDisplayMode, "toggle_display_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleSeparatorsBetweenAlbums : public Action
{
	ToggleSeparatorsBetweenAlbums()
	: Action(aToggleSeparatorsBetweenAlbums, "toggle_separators_between_albums") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleLyricsFetcher : public Action
{
	ToggleLyricsFetcher() : Action(aToggleLyricsFetcher, "toggle_lyrics_fetcher") { }
	
protected:
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void run();
};

struct ToggleFetchingLyricsInBackground : public Action
{
	ToggleFetchingLyricsInBackground()
	: Action(aToggleFetchingLyricsInBackground, "toggle_fetching_lyrics_in_background") { }
	
protected:
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void run();
};

struct TogglePlayingSongCentering : public Action
{
	TogglePlayingSongCentering()
	: Action(aTogglePlayingSongCentering, "toggle_playing_song_centering") { }
	
protected:
	virtual void run();
};

struct UpdateDatabase : public Action
{
	UpdateDatabase() : Action(aUpdateDatabase, "update_database") { }
	
protected:
	virtual void run();
};

struct JumpToPlayingSong : public Action
{
	JumpToPlayingSong() : Action(aJumpToPlayingSong, "jump_to_playing_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleRepeat : public Action
{
	ToggleRepeat() : Action(aToggleRepeat, "toggle_repeat") { }
	
protected:
	virtual void run();
};

struct Shuffle : public Action
{
	Shuffle() : Action(aShuffle, "shuffle") { }
	
protected:
	virtual void run();
};

struct ToggleRandom : public Action
{
	ToggleRandom() : Action(aToggleRandom, "toggle_random") { }
	
protected:
	virtual void run();
};

struct StartSearching : public Action
{
	StartSearching() : Action(aStartSearching, "start_searching") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SaveTagChanges : public Action
{
	SaveTagChanges() : Action(aSaveTagChanges, "save_tag_changes") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleSingle : public Action
{
	ToggleSingle() : Action(aToggleSingle, "toggle_single") { }
	
protected:
	virtual void run();
};

struct ToggleConsume : public Action
{
	ToggleConsume() : Action(aToggleConsume, "toggle_consume") { }
	
protected:
	virtual void run();
};

struct ToggleCrossfade : public Action
{
	ToggleCrossfade() : Action(aToggleCrossfade, "toggle_crossfade") { }
	
protected:
	virtual void run();
};

struct SetCrossfade : public Action
{
	SetCrossfade() : Action(aSetCrossfade, "set_crossfade") { }
	
protected:
	virtual void run();
};

struct EditSong : public Action
{
	EditSong() : Action(aEditSong, "edit_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLibraryTag : public Action
{
	EditLibraryTag() : Action(aEditLibraryTag, "edit_library_tag") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLibraryAlbum : public Action
{
	EditLibraryAlbum() : Action(aEditLibraryAlbum, "edit_library_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditDirectoryName : public Action
{
	EditDirectoryName() : Action(aEditDirectoryName, "edit_directory_name") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditPlaylistName : public Action
{
	EditPlaylistName() : Action(aEditPlaylistName, "edit_playlist_name") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLyrics : public Action
{
	EditLyrics() : Action(aEditLyrics, "edit_lyrics") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToBrowser : public Action
{
	JumpToBrowser() : Action(aJumpToBrowser, "jump_to_browser") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToMediaLibrary : public Action
{
	JumpToMediaLibrary() : Action(aJumpToMediaLibrary, "jump_to_media_library") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToPlaylistEditor : public Action
{
	JumpToPlaylistEditor() : Action(aJumpToPlaylistEditor, "jump_to_playlist_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleScreenLock : public Action
{
	ToggleScreenLock() : Action(aToggleScreenLock, "toggle_screen_lock") { }
	
protected:
	virtual void run();
};

struct JumpToTagEditor : public Action
{
	JumpToTagEditor() : Action(aJumpToTagEditor, "jump_to_tag_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToPositionInSong : public Action
{
	JumpToPositionInSong() : Action(aJumpToPositionInSong, "jump_to_position_in_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReverseSelection : public Action
{
	ReverseSelection() : Action(aReverseSelection, "reverse_selection") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct RemoveSelection : public Action
{
	RemoveSelection() : Action(aRemoveSelection, "remove_selection") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SelectAlbum : public Action
{
	SelectAlbum() : Action(aSelectAlbum, "select_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct AddSelectedItems : public Action
{
	AddSelectedItems() : Action(aAddSelectedItems, "add_selected_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct CropMainPlaylist : public Action
{
	CropMainPlaylist() : Action(aCropMainPlaylist, "crop_main_playlist") { }
	
protected:
	virtual void run();
};

struct CropPlaylist : public Action
{
	CropPlaylist() : Action(aCropPlaylist, "crop_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ClearMainPlaylist : public Action
{
	ClearMainPlaylist() : Action(aClearMainPlaylist, "clear_main_playlist") { }
	
protected:
	virtual void run();
};

struct ClearPlaylist : public Action
{
	ClearPlaylist() : Action(aClearPlaylist, "clear_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SortPlaylist : public Action
{
	SortPlaylist() : Action(aSortPlaylist, "sort_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReversePlaylist : public Action
{
	ReversePlaylist() : Action(aReversePlaylist, "reverse_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ApplyFilter : public Action
{
	ApplyFilter() : Action(aApplyFilter, "apply_filter") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct Find : public Action
{
	Find() : Action(aFind, "find") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FindItemForward : public Action
{
	FindItemForward() : Action(aFindItemForward, "find_item_forward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FindItemBackward : public Action
{
	FindItemBackward() : Action(aFindItemBackward, "find_item_backward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct NextFoundItem : public Action
{
	NextFoundItem() : Action(aNextFoundItem, "next_found_item") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PreviousFoundItem : public Action
{
	PreviousFoundItem() : Action(aPreviousFoundItem, "previous_found_item") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleFindMode : public Action
{
	ToggleFindMode() : Action(aToggleFindMode, "toggle_find_mode") { }
	
protected:
	virtual void run();
};

struct ToggleReplayGainMode : public Action
{
	ToggleReplayGainMode() : Action(aToggleReplayGainMode, "toggle_replay_gain_mode") { }
	
protected:
	virtual void run();
};

struct ToggleSpaceMode : public Action
{
	ToggleSpaceMode() : Action(aToggleSpaceMode, "toggle_space_mode") { }
	
protected:
	virtual void run();
};

struct ToggleAddMode : public Action
{
	ToggleAddMode() : Action(aToggleAddMode, "toggle_add_mode") { }
	
protected:
	virtual void run();
};

struct ToggleMouse : public Action
{
	ToggleMouse() : Action(aToggleMouse, "toggle_mouse") { }
	
protected:
	virtual void run();
};

struct ToggleBitrateVisibility : public Action
{
	ToggleBitrateVisibility() : Action(aToggleBitrateVisibility, "toggle_bitrate_visibility") { }
	
protected:
	virtual void run();
};

struct AddRandomItems : public Action
{
	AddRandomItems() : Action(aAddRandomItems, "add_random_items") { }
	
protected:
	virtual void run();
};

struct ToggleBrowserSortMode : public Action
{
	ToggleBrowserSortMode() : Action(aToggleBrowserSortMode, "toggle_browser_sort_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleLibraryTagType : public Action
{
	ToggleLibraryTagType() : Action(aToggleLibraryTagType, "toggle_library_tag_type") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleMediaLibrarySortMode : public Action
{
	ToggleMediaLibrarySortMode()
	: Action(aToggleMediaLibrarySortMode, "toggle_media_library_sort_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct RefetchLyrics : public Action
{
	RefetchLyrics() : Action(aRefetchLyrics, "refetch_lyrics") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct RefetchArtistInfo : public Action
{
	RefetchArtistInfo() : Action(aRefetchArtistInfo, "refetch_artist_info") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SetSelectedItemsPriority : public Action
{
	SetSelectedItemsPriority()
	: Action(aSetSelectedItemsPriority, "set_selected_items_priority") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FilterPlaylistOnPriorities : public Action
{
	FilterPlaylistOnPriorities()
	: Action(aFilterPlaylistOnPriorities, "filter_playlist_on_priorities") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowSongInfo : public Action
{
	ShowSongInfo() : Action(aShowSongInfo, "show_song_info") { }
	
protected:
	virtual void run();
};

struct ShowArtistInfo : public Action
{
	ShowArtistInfo() : Action(aShowArtistInfo, "show_artist_info") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowLyrics : public Action
{
	ShowLyrics() : Action(aShowLyrics, "show_lyrics") { }
	
protected:
	virtual void run();
};

struct Quit : public Action
{
	Quit() : Action(aQuit, "quit") { }
	
protected:
	virtual void run();
};

struct NextScreen : public Action
{
	NextScreen() : Action(aNextScreen, "next_screen") { }
	
protected:
	virtual void run();
};

struct PreviousScreen : public Action
{
	PreviousScreen() : Action(aPreviousScreen, "previous_screen") { }
	
protected:
	virtual void run();
};

struct ShowHelp : public Action
{
	ShowHelp() : Action(aShowHelp, "show_help") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowPlaylist : public Action
{
	ShowPlaylist() : Action(aShowPlaylist, "show_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowBrowser : public Action
{
	ShowBrowser() : Action(aShowBrowser, "show_browser") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ChangeBrowseMode : public Action
{
	ChangeBrowseMode() : Action(aChangeBrowseMode, "change_browse_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowSearchEngine : public Action
{
	ShowSearchEngine() : Action(aShowSearchEngine, "show_search_engine") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ResetSearchEngine : public Action
{
	ResetSearchEngine() : Action(aResetSearchEngine, "reset_search_engine") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowMediaLibrary : public Action
{
	ShowMediaLibrary() : Action(aShowMediaLibrary, "show_media_library") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleMediaLibraryColumnsMode : public Action
{
	ToggleMediaLibraryColumnsMode()
	: Action(aToggleMediaLibraryColumnsMode, "toggle_media_library_columns_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowPlaylistEditor : public Action
{
	ShowPlaylistEditor() : Action(aShowPlaylistEditor, "show_playlist_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowTagEditor : public Action
{
	ShowTagEditor() : Action(aShowTagEditor, "show_tag_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowOutputs : public Action
{
	ShowOutputs() : Action(aShowOutputs, "show_outputs") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowVisualizer : public Action
{
	ShowVisualizer() : Action(aShowVisualizer, "show_visualizer") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowClock : public Action
{
	ShowClock() : Action(aShowClock, "show_clock") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowServerInfo : public Action
{
	ShowServerInfo() : Action(aShowServerInfo, "show_server_info") { }
	
protected:
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void run();
};

#endif // NCMPCPP_ACTIONS_H
