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

namespace Actions {//

void validateScreenSize();
void initializeScreens();
void setResizeFlags();
void resizeScreen(bool reload_main_window);
void setWindowsDimensions();

bool connectToMPD();
bool askYesNoQuestion(const std::string &question, void (*callback)());
bool isMPDMusicDirSet();

struct BaseAction *get(ActionType at);
struct BaseAction *get(const std::string &name);

extern bool OriginalStatusbarVisibility;
extern bool ExitMainLoop;

extern size_t HeaderHeight;
extern size_t FooterHeight;
extern size_t FooterStartY;

struct BaseAction
{
	BaseAction(ActionType type, const char *name) : m_type(type), m_name(name) { }
	
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
	
protected:
	virtual void run() = 0;
	
private:
	ActionType m_type;
	const char *m_name;
};

struct Dummy : public BaseAction
{
	Dummy() : BaseAction(aDummy, "dummy") { }
	
protected:
	virtual void run() { }
};

struct MouseEvent : public BaseAction
{
	MouseEvent() : BaseAction(aMouseEvent, "mouse_event") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
	
	private:
		MEVENT m_mouse_event;
		MEVENT m_old_mouse_event;
};

struct ScrollUp : public BaseAction
{
	ScrollUp() : BaseAction(aScrollUp, "scroll_up") { }
	
protected:
	virtual void run();
};

struct ScrollDown : public BaseAction
{
	ScrollDown() : BaseAction(aScrollDown, "scroll_down") { }
	
protected:
	virtual void run();
};

struct ScrollUpArtist : public BaseAction
{
	ScrollUpArtist() : BaseAction(aScrollUpArtist, "scroll_up_artist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollUpAlbum : public BaseAction
{
	ScrollUpAlbum() : BaseAction(aScrollUpAlbum, "scroll_up_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollDownArtist : public BaseAction
{
	ScrollDownArtist() : BaseAction(aScrollDownArtist, "scroll_down_artist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollDownAlbum : public BaseAction
{
	ScrollDownAlbum() : BaseAction(aScrollDownAlbum, "scroll_down_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PageUp : public BaseAction
{
	PageUp() : BaseAction(aPageUp, "page_up") { }
	
protected:
	virtual void run();
};

struct PageDown : public BaseAction
{
	PageDown() : BaseAction(aPageDown, "page_down") { }
	
protected:
	virtual void run();
};

struct MoveHome : public BaseAction
{
	MoveHome() : BaseAction(aMoveHome, "move_home") { }
	
protected:
	virtual void run();
};

struct MoveEnd : public BaseAction
{
	MoveEnd() : BaseAction(aMoveEnd, "move_end") { }
	
protected:
	virtual void run();
};

struct ToggleInterface : public BaseAction
{
	ToggleInterface() : BaseAction(aToggleInterface, "toggle_interface") { }
	
protected:
	virtual void run();
};

struct JumpToParentDirectory : public BaseAction
{
	JumpToParentDirectory() : BaseAction(aJumpToParentDirectory, "jump_to_parent_directory") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PressEnter : public BaseAction
{
	PressEnter() : BaseAction(aPressEnter, "press_enter") { }
	
protected:
	virtual void run();
};

struct PressSpace : public BaseAction
{
	PressSpace() : BaseAction(aPressSpace, "press_space") { }
	
protected:
	virtual void run();
};

struct PreviousColumn : public BaseAction
{
	PreviousColumn() : BaseAction(aPreviousColumn, "previous_column") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct NextColumn : public BaseAction
{
	NextColumn() : BaseAction(aNextColumn, "next_column") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MasterScreen : public BaseAction
{
	MasterScreen() : BaseAction(aMasterScreen, "master_screen") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SlaveScreen : public BaseAction
{
	SlaveScreen() : BaseAction(aSlaveScreen, "slave_screen") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct VolumeUp : public BaseAction
{
	VolumeUp() : BaseAction(aVolumeUp, "volume_up") { }
	
protected:
	virtual void run();
};

struct VolumeDown : public BaseAction
{
	VolumeDown() : BaseAction(aVolumeDown, "volume_down") { }
	
protected:
	virtual void run();
};

struct DeletePlaylistItems : public BaseAction
{
	DeletePlaylistItems() : BaseAction(aDeletePlaylistItems, "delete_playlist_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct DeleteStoredPlaylist : public BaseAction
{
	DeleteStoredPlaylist() : BaseAction(aDeleteStoredPlaylist, "delete_stored_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct DeleteBrowserItems : public BaseAction
{
	DeleteBrowserItems() : BaseAction(aDeleteBrowserItems, "delete_browser_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReplaySong : public BaseAction
{
	ReplaySong() : BaseAction(aReplaySong, "replay_song") { }
	
protected:
	virtual void run();
};

struct PreviousSong : public BaseAction
{
	PreviousSong() : BaseAction(aPrevious, "previous") { }
	
protected:
	virtual void run();
};

struct NextSong : public BaseAction
{
	NextSong() : BaseAction(aNext, "next") { }
	
protected:
	virtual void run();
};

struct Pause : public BaseAction
{
	Pause() : BaseAction(aPause, "pause") { }
	
protected:
	virtual void run();
};

struct Stop : public BaseAction
{
	Stop() : BaseAction(aStop, "stop") { }
	
protected:
	virtual void run();
};

struct ExecuteCommand : public BaseAction
{
	ExecuteCommand() : BaseAction(aExecuteCommand, "execute_command") { }
	
protected:
	virtual void run();
};

struct SavePlaylist : public BaseAction
{
	SavePlaylist() : BaseAction(aSavePlaylist, "save_playlist") { }
	
protected:
	virtual void run();
};

struct MoveSortOrderUp : public BaseAction
{
	MoveSortOrderUp() : BaseAction(aMoveSortOrderUp, "move_sort_order_up") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSortOrderDown : public BaseAction
{
	MoveSortOrderDown() : BaseAction(aMoveSortOrderDown, "move_sort_order_down") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsUp : public BaseAction
{
	MoveSelectedItemsUp() : BaseAction(aMoveSelectedItemsUp, "move_selected_items_up") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsDown : public BaseAction
{
	MoveSelectedItemsDown() : BaseAction(aMoveSelectedItemsDown, "move_selected_items_down") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsTo : public BaseAction
{
	MoveSelectedItemsTo() : BaseAction(aMoveSelectedItemsTo, "move_selected_items_to") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct Add : public BaseAction
{
	Add() : BaseAction(aAdd, "add") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SeekForward : public BaseAction
{
	SeekForward() : BaseAction(aSeekForward, "seek_forward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SeekBackward : public BaseAction
{
	SeekBackward() : BaseAction(aSeekBackward, "seek_backward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleDisplayMode : public BaseAction
{
	ToggleDisplayMode() : BaseAction(aToggleDisplayMode, "toggle_display_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleSeparatorsBetweenAlbums : public BaseAction
{
	ToggleSeparatorsBetweenAlbums()
	: BaseAction(aToggleSeparatorsBetweenAlbums, "toggle_separators_between_albums") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleLyricsFetcher : public BaseAction
{
	ToggleLyricsFetcher() : BaseAction(aToggleLyricsFetcher, "toggle_lyrics_fetcher") { }
	
protected:
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void run();
};

struct ToggleFetchingLyricsInBackground : public BaseAction
{
	ToggleFetchingLyricsInBackground()
	: BaseAction(aToggleFetchingLyricsInBackground, "toggle_fetching_lyrics_in_background") { }
	
protected:
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void run();
};

struct TogglePlayingSongCentering : public BaseAction
{
	TogglePlayingSongCentering()
	: BaseAction(aTogglePlayingSongCentering, "toggle_playing_song_centering") { }
	
protected:
	virtual void run();
};

struct UpdateDatabase : public BaseAction
{
	UpdateDatabase() : BaseAction(aUpdateDatabase, "update_database") { }
	
protected:
	virtual void run();
};

struct JumpToPlayingSong : public BaseAction
{
	JumpToPlayingSong() : BaseAction(aJumpToPlayingSong, "jump_to_playing_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleRepeat : public BaseAction
{
	ToggleRepeat() : BaseAction(aToggleRepeat, "toggle_repeat") { }
	
protected:
	virtual void run();
};

struct Shuffle : public BaseAction
{
	Shuffle() : BaseAction(aShuffle, "shuffle") { }
	
protected:
	virtual void run();
};

struct ToggleRandom : public BaseAction
{
	ToggleRandom() : BaseAction(aToggleRandom, "toggle_random") { }
	
protected:
	virtual void run();
};

struct StartSearching : public BaseAction
{
	StartSearching() : BaseAction(aStartSearching, "start_searching") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SaveTagChanges : public BaseAction
{
	SaveTagChanges() : BaseAction(aSaveTagChanges, "save_tag_changes") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleSingle : public BaseAction
{
	ToggleSingle() : BaseAction(aToggleSingle, "toggle_single") { }
	
protected:
	virtual void run();
};

struct ToggleConsume : public BaseAction
{
	ToggleConsume() : BaseAction(aToggleConsume, "toggle_consume") { }
	
protected:
	virtual void run();
};

struct ToggleCrossfade : public BaseAction
{
	ToggleCrossfade() : BaseAction(aToggleCrossfade, "toggle_crossfade") { }
	
protected:
	virtual void run();
};

struct SetCrossfade : public BaseAction
{
	SetCrossfade() : BaseAction(aSetCrossfade, "set_crossfade") { }
	
protected:
	virtual void run();
};

struct EditSong : public BaseAction
{
	EditSong() : BaseAction(aEditSong, "edit_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLibraryTag : public BaseAction
{
	EditLibraryTag() : BaseAction(aEditLibraryTag, "edit_library_tag") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLibraryAlbum : public BaseAction
{
	EditLibraryAlbum() : BaseAction(aEditLibraryAlbum, "edit_library_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditDirectoryName : public BaseAction
{
	EditDirectoryName() : BaseAction(aEditDirectoryName, "edit_directory_name") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditPlaylistName : public BaseAction
{
	EditPlaylistName() : BaseAction(aEditPlaylistName, "edit_playlist_name") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLyrics : public BaseAction
{
	EditLyrics() : BaseAction(aEditLyrics, "edit_lyrics") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToBrowser : public BaseAction
{
	JumpToBrowser() : BaseAction(aJumpToBrowser, "jump_to_browser") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToMediaLibrary : public BaseAction
{
	JumpToMediaLibrary() : BaseAction(aJumpToMediaLibrary, "jump_to_media_library") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToPlaylistEditor : public BaseAction
{
	JumpToPlaylistEditor() : BaseAction(aJumpToPlaylistEditor, "jump_to_playlist_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleScreenLock : public BaseAction
{
	ToggleScreenLock() : BaseAction(aToggleScreenLock, "toggle_screen_lock") { }
	
protected:
	virtual void run();
};

struct JumpToTagEditor : public BaseAction
{
	JumpToTagEditor() : BaseAction(aJumpToTagEditor, "jump_to_tag_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToPositionInSong : public BaseAction
{
	JumpToPositionInSong() : BaseAction(aJumpToPositionInSong, "jump_to_position_in_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReverseSelection : public BaseAction
{
	ReverseSelection() : BaseAction(aReverseSelection, "reverse_selection") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct RemoveSelection : public BaseAction
{
	RemoveSelection() : BaseAction(aRemoveSelection, "remove_selection") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SelectAlbum : public BaseAction
{
	SelectAlbum() : BaseAction(aSelectAlbum, "select_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct AddSelectedItems : public BaseAction
{
	AddSelectedItems() : BaseAction(aAddSelectedItems, "add_selected_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct CropMainPlaylist : public BaseAction
{
	CropMainPlaylist() : BaseAction(aCropMainPlaylist, "crop_main_playlist") { }
	
protected:
	virtual void run();
};

struct CropPlaylist : public BaseAction
{
	CropPlaylist() : BaseAction(aCropPlaylist, "crop_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ClearMainPlaylist : public BaseAction
{
	ClearMainPlaylist() : BaseAction(aClearMainPlaylist, "clear_main_playlist") { }
	
protected:
	virtual void run();
};

struct ClearPlaylist : public BaseAction
{
	ClearPlaylist() : BaseAction(aClearPlaylist, "clear_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SortPlaylist : public BaseAction
{
	SortPlaylist() : BaseAction(aSortPlaylist, "sort_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReversePlaylist : public BaseAction
{
	ReversePlaylist() : BaseAction(aReversePlaylist, "reverse_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ApplyFilter : public BaseAction
{
	ApplyFilter() : BaseAction(aApplyFilter, "apply_filter") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct Find : public BaseAction
{
	Find() : BaseAction(aFind, "find") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FindItemForward : public BaseAction
{
	FindItemForward() : BaseAction(aFindItemForward, "find_item_forward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FindItemBackward : public BaseAction
{
	FindItemBackward() : BaseAction(aFindItemBackward, "find_item_backward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct NextFoundItem : public BaseAction
{
	NextFoundItem() : BaseAction(aNextFoundItem, "next_found_item") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PreviousFoundItem : public BaseAction
{
	PreviousFoundItem() : BaseAction(aPreviousFoundItem, "previous_found_item") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleFindMode : public BaseAction
{
	ToggleFindMode() : BaseAction(aToggleFindMode, "toggle_find_mode") { }
	
protected:
	virtual void run();
};

struct ToggleReplayGainMode : public BaseAction
{
	ToggleReplayGainMode() : BaseAction(aToggleReplayGainMode, "toggle_replay_gain_mode") { }
	
protected:
	virtual void run();
};

struct ToggleSpaceMode : public BaseAction
{
	ToggleSpaceMode() : BaseAction(aToggleSpaceMode, "toggle_space_mode") { }
	
protected:
	virtual void run();
};

struct ToggleAddMode : public BaseAction
{
	ToggleAddMode() : BaseAction(aToggleAddMode, "toggle_add_mode") { }
	
protected:
	virtual void run();
};

struct ToggleMouse : public BaseAction
{
	ToggleMouse() : BaseAction(aToggleMouse, "toggle_mouse") { }
	
protected:
	virtual void run();
};

struct ToggleBitrateVisibility : public BaseAction
{
	ToggleBitrateVisibility() : BaseAction(aToggleBitrateVisibility, "toggle_bitrate_visibility") { }
	
protected:
	virtual void run();
};

struct AddRandomItems : public BaseAction
{
	AddRandomItems() : BaseAction(aAddRandomItems, "add_random_items") { }
	
protected:
	virtual void run();
};

struct ToggleBrowserSortMode : public BaseAction
{
	ToggleBrowserSortMode() : BaseAction(aToggleBrowserSortMode, "toggle_browser_sort_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleLibraryTagType : public BaseAction
{
	ToggleLibraryTagType() : BaseAction(aToggleLibraryTagType, "toggle_library_tag_type") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleMediaLibrarySortMode : public BaseAction
{
	ToggleMediaLibrarySortMode()
	: BaseAction(aToggleMediaLibrarySortMode, "toggle_media_library_sort_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct RefetchLyrics : public BaseAction
{
	RefetchLyrics() : BaseAction(aRefetchLyrics, "refetch_lyrics") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct RefetchArtistInfo : public BaseAction
{
	RefetchArtistInfo() : BaseAction(aRefetchArtistInfo, "refetch_artist_info") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SetSelectedItemsPriority : public BaseAction
{
	SetSelectedItemsPriority()
	: BaseAction(aSetSelectedItemsPriority, "set_selected_items_priority") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FilterPlaylistOnPriorities : public BaseAction
{
	FilterPlaylistOnPriorities()
	: BaseAction(aFilterPlaylistOnPriorities, "filter_playlist_on_priorities") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowSongInfo : public BaseAction
{
	ShowSongInfo() : BaseAction(aShowSongInfo, "show_song_info") { }
	
protected:
	virtual void run();
};

struct ShowArtistInfo : public BaseAction
{
	ShowArtistInfo() : BaseAction(aShowArtistInfo, "show_artist_info") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowLyrics : public BaseAction
{
	ShowLyrics() : BaseAction(aShowLyrics, "show_lyrics") { }
	
protected:
	virtual void run();
};

struct Quit : public BaseAction
{
	Quit() : BaseAction(aQuit, "quit") { }
	
protected:
	virtual void run();
};

struct NextScreen : public BaseAction
{
	NextScreen() : BaseAction(aNextScreen, "next_screen") { }
	
protected:
	virtual void run();
};

struct PreviousScreen : public BaseAction
{
	PreviousScreen() : BaseAction(aPreviousScreen, "previous_screen") { }
	
protected:
	virtual void run();
};

struct ShowHelp : public BaseAction
{
	ShowHelp() : BaseAction(aShowHelp, "show_help") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowPlaylist : public BaseAction
{
	ShowPlaylist() : BaseAction(aShowPlaylist, "show_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowBrowser : public BaseAction
{
	ShowBrowser() : BaseAction(aShowBrowser, "show_browser") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ChangeBrowseMode : public BaseAction
{
	ChangeBrowseMode() : BaseAction(aChangeBrowseMode, "change_browse_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowSearchEngine : public BaseAction
{
	ShowSearchEngine() : BaseAction(aShowSearchEngine, "show_search_engine") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ResetSearchEngine : public BaseAction
{
	ResetSearchEngine() : BaseAction(aResetSearchEngine, "reset_search_engine") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowMediaLibrary : public BaseAction
{
	ShowMediaLibrary() : BaseAction(aShowMediaLibrary, "show_media_library") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleMediaLibraryColumnsMode : public BaseAction
{
	ToggleMediaLibraryColumnsMode()
	: BaseAction(aToggleMediaLibraryColumnsMode, "toggle_media_library_columns_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowPlaylistEditor : public BaseAction
{
	ShowPlaylistEditor() : BaseAction(aShowPlaylistEditor, "show_playlist_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowTagEditor : public BaseAction
{
	ShowTagEditor() : BaseAction(aShowTagEditor, "show_tag_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowOutputs : public BaseAction
{
	ShowOutputs() : BaseAction(aShowOutputs, "show_outputs") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowVisualizer : public BaseAction
{
	ShowVisualizer() : BaseAction(aShowVisualizer, "show_visualizer") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowClock : public BaseAction
{
	ShowClock() : BaseAction(aShowClock, "show_clock") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowServerInfo : public BaseAction
{
	ShowServerInfo() : BaseAction(aShowServerInfo, "show_server_info") { }
	
protected:
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void run();
};

}

#endif // NCMPCPP_ACTIONS_H
