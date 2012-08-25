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

#ifndef _ACTIONS_H
#define _ACTIONS_H

#include <map>
#include <string>
#include "ncmpcpp.h"

enum ActionType
{
	aMacroUtility,
	aMouseEvent, aScrollUp, aScrollDown, aScrollUpArtist, aScrollUpAlbum, aScrollDownArtist,
	aScrollDownAlbum, aPageUp, aPageDown, aMoveHome, aMoveEnd, aToggleInterface, aJumpToParentDir,
	aPressEnter, aPressSpace, aPreviousColumn, aNextColumn, aMasterScreen, aSlaveScreen, aVolumeUp,
	aVolumeDown, aDelete, aReplaySong, aPreviousSong, aNextSong, aPause, aStop, aSavePlaylist,
	aMoveSortOrderUp, aMoveSortOrderDown, aMoveSelectedItemsUp, aMoveSelectedItemsDown,
	aMoveSelectedItemsTo, aAdd, aSeekForward, aSeekBackward, aToggleDisplayMode, aToggleSeparatorsInPlaylist,
	aToggleLyricsFetcher, aToggleFetchingLyricsInBackground, aToggleAutoCenter, aUpdateDatabase,
	aJumpToPlayingSong, aToggleRepeat, aShuffle, aToggleRandom, aStartSearching, aSaveTagChanges,
	aToggleSingle, aToggleConsume, aToggleCrossfade, aSetCrossfade, aEditSong, aEditLibraryTag,
	aEditLibraryAlbum, aEditDirectoryName, aEditPlaylistName, aEditLyrics, aJumpToBrowser,
	aJumpToMediaLibrary, aJumpToPlaylistEditor, aToggleScreenLock, aJumpToTagEditor,
	aJumpToPositionInSong, aReverseSelection, aDeselectItems, aSelectAlbum, aAddSelectedItems,
	aCropMainPlaylist, aCropPlaylist, aClearMainPlaylist, aClearPlaylist, aSortPlaylist, aReversePlaylist,
	aApplyFilter, aDisableFilter, aFind, aFindItemForward, aFindItemBackward, aNextFoundItem,
	aPreviousFoundItem, aToggleFindMode, aToggleReplayGainMode, aToggleSpaceMode, aToggleAddMode,
	aToggleMouse, aToggleBitrateVisibility, aAddRandomItems, aToggleBrowserSortMode, aToggleLibraryTagType,
	aRefetchLyrics, aRefetchArtistInfo, aSetSelectedItemsPriority, aShowSongInfo, aShowArtistInfo,
	aShowLyrics, aQuit, aNextScreen, aPreviousScreen, aShowHelp, aShowPlaylist, aShowBrowser,
	aShowSearchEngine, aShowMediaLibrary, aShowPlaylistEditor, aShowTagEditor, aShowOutputs,
	aShowVisualizer, aShowClock, aShowServerInfo
};

enum CharType { ctStandard, ctNCurses };

struct Action
{
	/// Key for binding actions to it. Supports non-ascii characters.
	struct Key
	{
		Key(wchar_t ch, CharType ct) : Char(ch), Type(ct) { }
		
		wchar_t getChar() const { return Char; }
		CharType getType() const { return Type; }
		
#		define INEQUALITY_OPERATOR(CMP) \
			bool operator CMP (const Key &k) const \
			{ \
				if (Char CMP k.Char) \
					return true; \
				if (Char != k.Char) \
					return false; \
				return Type CMP k.Type; \
			}
		INEQUALITY_OPERATOR(<);
		INEQUALITY_OPERATOR(<=);
		INEQUALITY_OPERATOR(>);
		INEQUALITY_OPERATOR(>=);
#		undef INEQUALITY_OPERATOR
		
		bool operator==(const Key &k) const { return Char == k.Char && Type == k.Type; }
		bool operator!=(const Key &k) const { return !(*this == k); }
		
		private:
			wchar_t Char;
			CharType Type;
	};
	
	enum FindDirection { fdForward, fdBackward };
	
	Action(ActionType type, const char *name) : itsType(type), itsName(name) { }
	
	const char *Name() const { return itsName; }
	ActionType Type() const { return itsType; }
	
	bool Execute()
	{
		if (canBeRun())
		{
			Run();
			return true;
		}
		return false;
	}
	
	static Key ReadKey(Window &w);
	
	static void ValidateScreenSize();
	static void SetResizeFlags();
	static void ResizeScreen();
	static void SetWindowsDimensions();
	
	static bool AskYesNoQuestion(const std::string &question, void (*callback)());
	static bool isMPDMusicDirSet();
	
	static Action *Get(ActionType);
	
	static bool OriginalStatusbarVisibility;
	static bool DesignChanged;
	static bool OrderResize;
	static bool ExitMainLoop;
	
	static size_t HeaderHeight;
	static size_t FooterHeight;
	static size_t FooterStartY;
	
	static Key NoOp;
	
	protected:
		virtual bool canBeRun() const { return true; }
		virtual void Run() = 0;
		
		static void Seek();
		static void FindItem(const FindDirection);
		static void ListsChangeFinisher();
		
	private:
		ActionType itsType;
		const char *itsName;
		
		static void insertAction(Action *a) { Actions[a->Type()] = a; }
		static std::map<ActionType, Action *> Actions;
};

struct MouseEvent : public Action
{
	MouseEvent() : Action(aMouseEvent, "mouse_event") { }
	virtual bool canBeRun() const;
	virtual void Run();
	
	private:
		MEVENT itsMouseEvent;
		MEVENT itsOldMouseEvent;
};

struct ScrollUp : public Action
{
	ScrollUp() : Action(aScrollUp, "scroll_up") { }
	virtual void Run();
};

struct ScrollDown : public Action
{
	ScrollDown() : Action(aScrollDown, "scroll_down") { }
	virtual void Run();
};

struct ScrollUpArtist : public Action
{
	ScrollUpArtist() : Action(aScrollUpArtist, "scroll_up_artist") { }
	virtual void Run();
};

struct ScrollUpAlbum : public Action
{
	ScrollUpAlbum() : Action(aScrollUpAlbum, "scroll_up_album") { }
	virtual void Run();
};

struct ScrollDownArtist : public Action
{
	ScrollDownArtist() : Action(aScrollDownArtist, "scroll_down_artist") { }
	virtual void Run();
};

struct ScrollDownAlbum : public Action
{
	ScrollDownAlbum() : Action(aScrollDownAlbum, "scroll_down_album") { }
	virtual void Run();
};

struct PageUp : public Action
{
	PageUp() : Action(aPageUp, "page_up") { }
	virtual void Run();
};

struct PageDown : public Action
{
	PageDown() : Action(aPageDown, "page_down") { }
	virtual void Run();
};

struct MoveHome : public Action
{
	MoveHome() : Action(aMoveHome, "move_home") { }
	virtual void Run();
};

struct MoveEnd : public Action
{
	MoveEnd() : Action(aMoveEnd, "move_end") { }
	virtual void Run();
};

struct ToggleInterface : public Action
{
	ToggleInterface() : Action(aToggleInterface, "toggle_inferface") { }
	virtual void Run();
};

struct JumpToParentDir : public Action
{
	JumpToParentDir() : Action(aJumpToParentDir, "jump_to_parent_dir") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct PressEnter : public Action
{
	PressEnter() : Action(aPressEnter, "press_enter") { }
	virtual void Run();
};

struct PressSpace : public Action
{
	PressSpace() : Action(aPressSpace, "press_space") { }
	virtual void Run();
};

struct PreviousColumn : public Action
{
	PreviousColumn() : Action(aPreviousColumn, "previous_column") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct NextColumn : public Action
{
	NextColumn() : Action(aNextColumn, "next_column") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct MasterScreen : public Action
{
	MasterScreen() : Action(aMasterScreen, "master_screen") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct SlaveScreen : public Action
{
	SlaveScreen() : Action(aSlaveScreen, "slave_screen") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct VolumeUp : public Action
{
	VolumeUp() : Action(aVolumeUp, "volume_up") { }
	virtual void Run();
};

struct VolumeDown : public Action
{
	VolumeDown() : Action(aVolumeDown, "volume_down") { }
	virtual void Run();
};

struct Delete : public Action
{
	Delete() : Action(aDelete, "delete") { }
	virtual void Run();
};

struct ReplaySong : public Action
{
	ReplaySong() : Action(aReplaySong, "replay_song") { }
	virtual void Run();
};

struct PreviousSong : public Action
{
	PreviousSong() : Action(aPreviousSong, "previous") { }
	virtual void Run();
};

struct NextSong : public Action
{
	NextSong() : Action(aNextSong, "next") { }
	virtual void Run();
};

struct Pause : public Action
{
	Pause() : Action(aPause, "pause") { }
	virtual void Run();
};

struct Stop : public Action
{
	Stop() : Action(aStop, "stop") { }
	virtual void Run();
};

struct SavePlaylist : public Action
{
	SavePlaylist() : Action(aSavePlaylist, "save_playlist") { }
	virtual void Run();
};

struct MoveSortOrderUp : public Action
{
	MoveSortOrderUp() : Action(aMoveSortOrderUp, "move_sort_order_up") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct MoveSortOrderDown : public Action
{
	MoveSortOrderDown() : Action(aMoveSortOrderDown, "move_sort_order_down") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct MoveSelectedItemsUp : public Action
{
	MoveSelectedItemsUp() : Action(aMoveSelectedItemsUp, "move_selected_items_up") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct MoveSelectedItemsDown : public Action
{
	MoveSelectedItemsDown() : Action(aMoveSelectedItemsDown, "move_selected_items_down") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct MoveSelectedItemsTo : public Action
{
	MoveSelectedItemsTo() : Action(aMoveSelectedItemsTo, "move_selected_items_to") { }
	virtual void Run();
};

struct Add : public Action
{
	Add() : Action(aAdd, "add") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct SeekForward : public Action
{
	SeekForward() : Action(aSeekForward, "seek_forward") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct SeekBackward : public Action
{
	SeekBackward() : Action(aSeekBackward, "seek_backward") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ToggleDisplayMode : public Action
{
	ToggleDisplayMode() : Action(aToggleDisplayMode, "toggle_display_mode") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ToggleSeparatorsInPlaylist : public Action
{
	ToggleSeparatorsInPlaylist() : Action(aToggleSeparatorsInPlaylist, "toggle_separators_in_playlist") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ToggleLyricsFetcher : public Action
{
	ToggleLyricsFetcher() : Action(aToggleLyricsFetcher, "toggle_lyrics_fetcher") { }
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void Run();
};

struct ToggleFetchingLyricsInBackground : public Action
{
	ToggleFetchingLyricsInBackground() : Action(aToggleFetchingLyricsInBackground, "toggle_fetching_lyrics_in_background") { }
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void Run();
};

struct ToggleAutoCenter : public Action
{
	ToggleAutoCenter() : Action(aToggleAutoCenter, "toggle_autocentering") { }
	virtual void Run();
};

struct UpdateDatabase : public Action
{
	UpdateDatabase() : Action(aUpdateDatabase, "update_database") { }
	virtual void Run();
};

struct JumpToPlayingSong : public Action
{
	JumpToPlayingSong() : Action(aJumpToPlayingSong, "jump_to_playing_song") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ToggleRepeat : public Action
{
	ToggleRepeat() : Action(aToggleRepeat, "toggle_repeat") { }
	virtual void Run();
};

struct Shuffle : public Action
{
	Shuffle() : Action(aShuffle, "shuffle") { }
	virtual void Run();
};

struct ToggleRandom : public Action
{
	ToggleRandom() : Action(aToggleRandom, "toggle_random") { }
	virtual void Run();
};

struct StartSearching : public Action
{
	StartSearching() : Action(aStartSearching, "start_searching") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct SaveTagChanges : public Action
{
	SaveTagChanges() : Action(aSaveTagChanges, "save_tag_changes") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ToggleSingle : public Action
{
	ToggleSingle() : Action(aToggleSingle, "toggle_single") { }
	virtual void Run();
};

struct ToggleConsume : public Action
{
	ToggleConsume() : Action(aToggleConsume, "toggle_consume") { }
	virtual void Run();
};

struct ToggleCrossfade : public Action
{
	ToggleCrossfade() : Action(aToggleCrossfade, "toggle_crossfade") { }
	virtual void Run();
};

struct SetCrossfade : public Action
{
	SetCrossfade() : Action(aSetCrossfade, "set_crossfade") { }
	virtual void Run();
};

struct EditSong : public Action
{
	EditSong() : Action(aEditSong, "edit_song") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct EditLibraryTag : public Action
{
	EditLibraryTag() : Action(aEditLibraryTag, "edit_library_tag") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct EditLibraryAlbum : public Action
{
	EditLibraryAlbum() : Action(aEditLibraryAlbum, "edit_library_album") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct EditDirectoryName : public Action
{
	EditDirectoryName() : Action(aEditDirectoryName, "edit_directory_name") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct EditPlaylistName : public Action
{
	EditPlaylistName() : Action(aEditPlaylistName, "edit_playlist_name") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct EditLyrics : public Action
{
	EditLyrics() : Action(aEditLyrics, "edit_lyrics") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct JumpToBrowser : public Action
{
	JumpToBrowser() : Action(aJumpToBrowser, "jump_to_browser") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct JumpToMediaLibrary : public Action
{
	JumpToMediaLibrary() : Action(aJumpToMediaLibrary, "jump_to_media_library") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct JumpToPlaylistEditor : public Action
{
	JumpToPlaylistEditor() : Action(aJumpToPlaylistEditor, "jump_to_playlist_editor") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ToggleScreenLock : public Action
{
	ToggleScreenLock() : Action(aToggleScreenLock, "toggle_screen_lock") { }
	virtual void Run();
};

struct JumpToTagEditor : public Action
{
	JumpToTagEditor() : Action(aJumpToTagEditor, "jump_to_tag_editor") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct JumpToPositionInSong : public Action
{
	JumpToPositionInSong() : Action(aJumpToPositionInSong, "jump_to_position_in_song") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ReverseSelection : public Action
{
	ReverseSelection() : Action(aReverseSelection, "reverse_selection") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct DeselectItems : public Action
{
	DeselectItems() : Action(aDeselectItems, "deselect_items") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct SelectAlbum : public Action
{
	SelectAlbum() : Action(aSelectAlbum, "select_album") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct AddSelectedItems : public Action
{
	AddSelectedItems() : Action(aAddSelectedItems, "add_selected_items") { }
	virtual void Run();
};

struct CropMainPlaylist : public Action
{
	CropMainPlaylist() : Action(aCropMainPlaylist, "crop_main_playlist") { }
	virtual void Run();
};

struct CropPlaylist : public Action
{
	CropPlaylist() : Action(aCropPlaylist, "crop_playlist") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ClearMainPlaylist : public Action
{
	ClearMainPlaylist() : Action(aClearMainPlaylist, "clear_main_playlist") { }
	virtual void Run();
};

struct ClearPlaylist : public Action
{
	ClearPlaylist() : Action(aClearPlaylist, "clear_playlist") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct SortPlaylist : public Action
{
	SortPlaylist() : Action(aSortPlaylist, "sort_playlist") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ReversePlaylist : public Action
{
	ReversePlaylist() : Action(aReversePlaylist, "reverse_playlist") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ApplyFilter : public Action
{
	ApplyFilter() : Action(aApplyFilter, "apply_filter") { }
	virtual void Run();
};

struct DisableFilter : public Action
{
	DisableFilter() : Action(aDisableFilter, "disable_filter") { }
	virtual void Run();
};

struct Find : public Action
{
	Find() : Action(aFind, "find") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct FindItemForward : public Action
{
	FindItemForward() : Action(aFindItemForward, "find_item_forward") { }
	virtual void Run();
};

struct FindItemBackward : public Action
{
	FindItemBackward() : Action(aFindItemBackward, "find_item_backward") { }
	virtual void Run();
};

struct NextFoundItem : public Action
{
	NextFoundItem() : Action(aNextFoundItem, "next_found_item") { }
	virtual void Run();
};

struct PreviousFoundItem : public Action
{
	PreviousFoundItem() : Action(aPreviousFoundItem, "previous_found_item") { }
	virtual void Run();
};

struct ToggleFindMode : public Action
{
	ToggleFindMode() : Action(aToggleFindMode, "toggle_find_mode") { }
	virtual void Run();
};

struct ToggleReplayGainMode : public Action
{
	ToggleReplayGainMode() : Action(aToggleReplayGainMode, "toggle_replay_gain_mode") { }
	virtual void Run();
};

struct ToggleSpaceMode : public Action
{
	ToggleSpaceMode() : Action(aToggleSpaceMode, "toggle_space_mode") { }
	virtual void Run();
};

struct ToggleAddMode : public Action
{
	ToggleAddMode() : Action(aToggleAddMode, "toggle_add_mode") { }
	virtual void Run();
};

struct ToggleMouse : public Action
{
	ToggleMouse() : Action(aToggleMouse, "toggle_mouse") { }
	virtual void Run();
};

struct ToggleBitrateVisibility : public Action
{
	ToggleBitrateVisibility() : Action(aToggleBitrateVisibility, "toggle_bitrate_visibility") { }
	virtual void Run();
};

struct AddRandomItems : public Action
{
	AddRandomItems() : Action(aAddRandomItems, "add_random_items") { }
	virtual void Run();
};

struct ToggleBrowserSortMode : public Action
{
	ToggleBrowserSortMode() : Action(aToggleBrowserSortMode, "toggle_browser_sort_mode") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ToggleLibraryTagType : public Action
{
	ToggleLibraryTagType() : Action(aToggleLibraryTagType, "toggle_library_tag_type") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct RefetchLyrics : public Action
{
	RefetchLyrics() : Action(aRefetchLyrics, "refetch_lyrics") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct RefetchArtistInfo : public Action
{
	RefetchArtistInfo() : Action(aRefetchArtistInfo, "refetch_artist_info") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct SetSelectedItemsPriority : public Action
{
	SetSelectedItemsPriority() : Action(aSetSelectedItemsPriority, "set_selected_items_priority") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ShowSongInfo : public Action
{
	ShowSongInfo() : Action(aShowSongInfo, "show_song_info") { }
	virtual void Run();
};

struct ShowArtistInfo : public Action
{
	ShowArtistInfo() : Action(aShowArtistInfo, "show_artist_info") { }
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void Run();
};

struct ShowLyrics : public Action
{
	ShowLyrics() : Action(aShowLyrics, "show_lyrics") { }
	virtual void Run();
};

struct Quit : public Action
{
	Quit() : Action(aQuit, "quit") { }
	virtual void Run();
};

struct NextScreen : public Action
{
	NextScreen() : Action(aNextScreen, "next_screen") { }
	virtual void Run();
};

struct PreviousScreen : public Action
{
	PreviousScreen() : Action(aPreviousScreen, "previous_screen") { }
	virtual void Run();
};

struct ShowHelp : public Action
{
	ShowHelp() : Action(aShowHelp, "show_help") { }
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void Run();
};

struct ShowPlaylist : public Action
{
	ShowPlaylist() : Action(aShowPlaylist, "show_playlist") { }
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void Run();
};

struct ShowBrowser : public Action
{
	ShowBrowser() : Action(aShowBrowser, "show_browser") { }
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void Run();
};

struct ShowSearchEngine : public Action
{
	ShowSearchEngine() : Action(aShowSearchEngine, "show_search_engine") { }
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void Run();
};

struct ShowMediaLibrary : public Action
{
	ShowMediaLibrary() : Action(aShowMediaLibrary, "show_media_library") { }
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void Run();
};

struct ShowPlaylistEditor : public Action
{
	ShowPlaylistEditor() : Action(aShowPlaylistEditor, "show_playlist_editor") { }
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void Run();
};

struct ShowTagEditor : public Action
{
	ShowTagEditor() : Action(aShowTagEditor, "show_tag_editor") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ShowOutputs : public Action
{
	ShowOutputs() : Action(aShowOutputs, "show_outputs") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ShowVisualizer : public Action
{
	ShowVisualizer() : Action(aShowVisualizer, "show_visualizer") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ShowClock : public Action
{
	ShowClock() : Action(aShowClock, "show_clock") { }
	virtual bool canBeRun() const;
	virtual void Run();
};

struct ShowServerInfo : public Action
{
	ShowServerInfo() : Action(aShowServerInfo, "show_server_info") { }
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void Run();
};

#endif // _ACTIONS_H
