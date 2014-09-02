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

#ifndef NCMPCPP_ACTIONS_H
#define NCMPCPP_ACTIONS_H

#include <boost/format.hpp>
#include <map>
#include <string>
#include "window.h"

namespace Actions {//

enum class Type
{
	MacroUtility = 0,
	Dummy, MouseEvent, ScrollUp, ScrollDown, ScrollUpArtist, ScrollUpAlbum,
	ScrollDownArtist, ScrollDownAlbum, PageUp, PageDown, MoveHome, MoveEnd,
	ToggleInterface, JumpToParentDirectory, PressEnter, PressSpace, PreviousColumn,
	NextColumn, MasterScreen, SlaveScreen, VolumeUp, VolumeDown, DeletePlaylistItems,
	DeleteStoredPlaylist, DeleteBrowserItems, ReplaySong, Previous, Next, Pause,
	Stop, ExecuteCommand, SavePlaylist, MoveSortOrderUp, MoveSortOrderDown,
	MoveSelectedItemsUp, MoveSelectedItemsDown, MoveSelectedItemsTo, Add,
	SeekForward, SeekBackward, ToggleDisplayMode, ToggleSeparatorsBetweenAlbums,
	ToggleLyricsFetcher, ToggleFetchingLyricsInBackground, TogglePlayingSongCentering,
	UpdateDatabase, JumpToPlayingSong, ToggleRepeat, Shuffle, ToggleRandom,
	StartSearching, SaveTagChanges, ToggleSingle, ToggleConsume, ToggleCrossfade,
	SetCrossfade, SetVolume, EditSong, EditLibraryTag, EditLibraryAlbum, EditDirectoryName,
	EditPlaylistName, EditLyrics, JumpToBrowser, JumpToMediaLibrary,
	JumpToPlaylistEditor, ToggleScreenLock, JumpToTagEditor, JumpToPositionInSong,
	ReverseSelection, RemoveSelection, SelectAlbum, AddSelectedItems,
	CropMainPlaylist, CropPlaylist, ClearMainPlaylist, ClearPlaylist, SortPlaylist,
	ReversePlaylist, ApplyFilter, Find, FindItemForward, FindItemBackward,
	NextFoundItem, PreviousFoundItem, ToggleFindMode, ToggleReplayGainMode,
	ToggleSpaceMode, ToggleAddMode, ToggleMouse, ToggleBitrateVisibility,
	AddRandomItems, ToggleBrowserSortMode, ToggleLibraryTagType,
	ToggleMediaLibrarySortMode, RefetchLyrics,
	SetSelectedItemsPriority, SetVisualizerSampleMultiplier, FilterPlaylistOnPriorities,
	ShowSongInfo, ShowArtistInfo, ShowLyrics, Quit, NextScreen, PreviousScreen,
	ShowHelp, ShowPlaylist, ShowBrowser, ChangeBrowseMode, ShowSearchEngine,
	ResetSearchEngine, ShowMediaLibrary, ToggleMediaLibraryColumnsMode,
	ShowPlaylistEditor, ShowTagEditor, ShowOutputs, ShowVisualizer,
	ShowClock, ShowServerInfo,
	_numberOfActions // needed to dynamically calculate size of action array
};

void validateScreenSize();
void initializeScreens();
void setResizeFlags();
void resizeScreen(bool reload_main_window);
void setWindowsDimensions();

bool askYesNoQuestion(const boost::format &question, void (*callback)());
inline bool askYesNoQuestion(const std::string &question, void (*callback)())
{
	return askYesNoQuestion(boost::format(question), callback);
}

bool isMPDMusicDirSet();

extern bool OriginalStatusbarVisibility;
extern bool ExitMainLoop;

extern size_t HeaderHeight;
extern size_t FooterHeight;
extern size_t FooterStartY;

struct BaseAction
{
	BaseAction(Type type_, const char *name_) : m_type(type_), m_name(name_) { }
	
	const char *name() const { return m_name; }
	Type type() const { return m_type; }
	
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
	Type m_type;
	const char *m_name;
};

BaseAction &get(Type at);
BaseAction *get(const std::string &name);

struct Dummy : public BaseAction
{
	Dummy() : BaseAction(Type::Dummy, "dummy") { }
	
protected:
	virtual void run() { }
};

struct MouseEvent : public BaseAction
{
	MouseEvent() : BaseAction(Type::MouseEvent, "mouse_event") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
	
	private:
		MEVENT m_mouse_event;
		MEVENT m_old_mouse_event;
};

struct ScrollUp : public BaseAction
{
	ScrollUp() : BaseAction(Type::ScrollUp, "scroll_up") { }
	
protected:
	virtual void run();
};

struct ScrollDown : public BaseAction
{
	ScrollDown() : BaseAction(Type::ScrollDown, "scroll_down") { }
	
protected:
	virtual void run();
};

struct ScrollUpArtist : public BaseAction
{
	ScrollUpArtist() : BaseAction(Type::ScrollUpArtist, "scroll_up_artist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollUpAlbum : public BaseAction
{
	ScrollUpAlbum() : BaseAction(Type::ScrollUpAlbum, "scroll_up_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollDownArtist : public BaseAction
{
	ScrollDownArtist() : BaseAction(Type::ScrollDownArtist, "scroll_down_artist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ScrollDownAlbum : public BaseAction
{
	ScrollDownAlbum() : BaseAction(Type::ScrollDownAlbum, "scroll_down_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PageUp : public BaseAction
{
	PageUp() : BaseAction(Type::PageUp, "page_up") { }
	
protected:
	virtual void run();
};

struct PageDown : public BaseAction
{
	PageDown() : BaseAction(Type::PageDown, "page_down") { }
	
protected:
	virtual void run();
};

struct MoveHome : public BaseAction
{
	MoveHome() : BaseAction(Type::MoveHome, "move_home") { }
	
protected:
	virtual void run();
};

struct MoveEnd : public BaseAction
{
	MoveEnd() : BaseAction(Type::MoveEnd, "move_end") { }
	
protected:
	virtual void run();
};

struct ToggleInterface : public BaseAction
{
	ToggleInterface() : BaseAction(Type::ToggleInterface, "toggle_interface") { }
	
protected:
	virtual void run();
};

struct JumpToParentDirectory : public BaseAction
{
	JumpToParentDirectory() : BaseAction(Type::JumpToParentDirectory, "jump_to_parent_directory") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PressEnter : public BaseAction
{
	PressEnter() : BaseAction(Type::PressEnter, "press_enter") { }
	
protected:
	virtual void run();
};

struct PressSpace : public BaseAction
{
	PressSpace() : BaseAction(Type::PressSpace, "press_space") { }
	
protected:
	virtual void run();
};

struct PreviousColumn : public BaseAction
{
	PreviousColumn() : BaseAction(Type::PreviousColumn, "previous_column") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct NextColumn : public BaseAction
{
	NextColumn() : BaseAction(Type::NextColumn, "next_column") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MasterScreen : public BaseAction
{
	MasterScreen() : BaseAction(Type::MasterScreen, "master_screen") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SlaveScreen : public BaseAction
{
	SlaveScreen() : BaseAction(Type::SlaveScreen, "slave_screen") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct VolumeUp : public BaseAction
{
	VolumeUp() : BaseAction(Type::VolumeUp, "volume_up") { }
	
protected:
	virtual void run();
};

struct VolumeDown : public BaseAction
{
	VolumeDown() : BaseAction(Type::VolumeDown, "volume_down") { }
	
protected:
	virtual void run();
};

struct DeletePlaylistItems : public BaseAction
{
	DeletePlaylistItems() : BaseAction(Type::DeletePlaylistItems, "delete_playlist_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct DeleteStoredPlaylist : public BaseAction
{
	DeleteStoredPlaylist() : BaseAction(Type::DeleteStoredPlaylist, "delete_stored_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct DeleteBrowserItems : public BaseAction
{
	DeleteBrowserItems() : BaseAction(Type::DeleteBrowserItems, "delete_browser_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReplaySong : public BaseAction
{
	ReplaySong() : BaseAction(Type::ReplaySong, "replay_song") { }
	
protected:
	virtual void run();
};

struct PreviousSong : public BaseAction
{
	PreviousSong() : BaseAction(Type::Previous, "previous") { }
	
protected:
	virtual void run();
};

struct NextSong : public BaseAction
{
	NextSong() : BaseAction(Type::Next, "next") { }
	
protected:
	virtual void run();
};

struct Pause : public BaseAction
{
	Pause() : BaseAction(Type::Pause, "pause") { }
	
protected:
	virtual void run();
};

struct Stop : public BaseAction
{
	Stop() : BaseAction(Type::Stop, "stop") { }
	
protected:
	virtual void run();
};

struct ExecuteCommand : public BaseAction
{
	ExecuteCommand() : BaseAction(Type::ExecuteCommand, "execute_command") { }
	
protected:
	virtual void run();
};

struct SavePlaylist : public BaseAction
{
	SavePlaylist() : BaseAction(Type::SavePlaylist, "save_playlist") { }
	
protected:
	virtual void run();
};

struct MoveSortOrderUp : public BaseAction
{
	MoveSortOrderUp() : BaseAction(Type::MoveSortOrderUp, "move_sort_order_up") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSortOrderDown : public BaseAction
{
	MoveSortOrderDown() : BaseAction(Type::MoveSortOrderDown, "move_sort_order_down") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsUp : public BaseAction
{
	MoveSelectedItemsUp() : BaseAction(Type::MoveSelectedItemsUp, "move_selected_items_up") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsDown : public BaseAction
{
	MoveSelectedItemsDown() : BaseAction(Type::MoveSelectedItemsDown, "move_selected_items_down") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct MoveSelectedItemsTo : public BaseAction
{
	MoveSelectedItemsTo() : BaseAction(Type::MoveSelectedItemsTo, "move_selected_items_to") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct Add : public BaseAction
{
	Add() : BaseAction(Type::Add, "add") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SeekForward : public BaseAction
{
	SeekForward() : BaseAction(Type::SeekForward, "seek_forward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SeekBackward : public BaseAction
{
	SeekBackward() : BaseAction(Type::SeekBackward, "seek_backward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleDisplayMode : public BaseAction
{
	ToggleDisplayMode() : BaseAction(Type::ToggleDisplayMode, "toggle_display_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleSeparatorsBetweenAlbums : public BaseAction
{
	ToggleSeparatorsBetweenAlbums()
	: BaseAction(Type::ToggleSeparatorsBetweenAlbums, "toggle_separators_between_albums") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleLyricsFetcher : public BaseAction
{
	ToggleLyricsFetcher() : BaseAction(Type::ToggleLyricsFetcher, "toggle_lyrics_fetcher") { }
	
protected:
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void run();
};

struct ToggleFetchingLyricsInBackground : public BaseAction
{
	ToggleFetchingLyricsInBackground()
	: BaseAction(Type::ToggleFetchingLyricsInBackground, "toggle_fetching_lyrics_in_background") { }
	
protected:
#	ifndef HAVE_CURL_CURL_H
	virtual bool canBeRun() const;
#	endif // NOT HAVE_CURL_CURL_H
	virtual void run();
};

struct TogglePlayingSongCentering : public BaseAction
{
	TogglePlayingSongCentering()
	: BaseAction(Type::TogglePlayingSongCentering, "toggle_playing_song_centering") { }
	
protected:
	virtual void run();
};

struct UpdateDatabase : public BaseAction
{
	UpdateDatabase() : BaseAction(Type::UpdateDatabase, "update_database") { }
	
protected:
	virtual void run();
};

struct JumpToPlayingSong : public BaseAction
{
	JumpToPlayingSong() : BaseAction(Type::JumpToPlayingSong, "jump_to_playing_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleRepeat : public BaseAction
{
	ToggleRepeat() : BaseAction(Type::ToggleRepeat, "toggle_repeat") { }
	
protected:
	virtual void run();
};

struct Shuffle : public BaseAction
{
	Shuffle() : BaseAction(Type::Shuffle, "shuffle") { }
	
protected:
	virtual void run();
};

struct ToggleRandom : public BaseAction
{
	ToggleRandom() : BaseAction(Type::ToggleRandom, "toggle_random") { }
	
protected:
	virtual void run();
};

struct StartSearching : public BaseAction
{
	StartSearching() : BaseAction(Type::StartSearching, "start_searching") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SaveTagChanges : public BaseAction
{
	SaveTagChanges() : BaseAction(Type::SaveTagChanges, "save_tag_changes") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleSingle : public BaseAction
{
	ToggleSingle() : BaseAction(Type::ToggleSingle, "toggle_single") { }
	
protected:
	virtual void run();
};

struct ToggleConsume : public BaseAction
{
	ToggleConsume() : BaseAction(Type::ToggleConsume, "toggle_consume") { }
	
protected:
	virtual void run();
};

struct ToggleCrossfade : public BaseAction
{
	ToggleCrossfade() : BaseAction(Type::ToggleCrossfade, "toggle_crossfade") { }
	
protected:
	virtual void run();
};

struct SetCrossfade : public BaseAction
{
	SetCrossfade() : BaseAction(Type::SetCrossfade, "set_crossfade") { }
	
protected:
	virtual void run();
};

struct SetVolume : public BaseAction
{
	SetVolume() : BaseAction(Type::SetVolume, "set_volume") { }
	
protected:
	virtual void run();
};

struct EditSong : public BaseAction
{
	EditSong() : BaseAction(Type::EditSong, "edit_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLibraryTag : public BaseAction
{
	EditLibraryTag() : BaseAction(Type::EditLibraryTag, "edit_library_tag") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLibraryAlbum : public BaseAction
{
	EditLibraryAlbum() : BaseAction(Type::EditLibraryAlbum, "edit_library_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditDirectoryName : public BaseAction
{
	EditDirectoryName() : BaseAction(Type::EditDirectoryName, "edit_directory_name") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditPlaylistName : public BaseAction
{
	EditPlaylistName() : BaseAction(Type::EditPlaylistName, "edit_playlist_name") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct EditLyrics : public BaseAction
{
	EditLyrics() : BaseAction(Type::EditLyrics, "edit_lyrics") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToBrowser : public BaseAction
{
	JumpToBrowser() : BaseAction(Type::JumpToBrowser, "jump_to_browser") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToMediaLibrary : public BaseAction
{
	JumpToMediaLibrary() : BaseAction(Type::JumpToMediaLibrary, "jump_to_media_library") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToPlaylistEditor : public BaseAction
{
	JumpToPlaylistEditor() : BaseAction(Type::JumpToPlaylistEditor, "jump_to_playlist_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleScreenLock : public BaseAction
{
	ToggleScreenLock() : BaseAction(Type::ToggleScreenLock, "toggle_screen_lock") { }
	
protected:
	virtual void run();
};

struct JumpToTagEditor : public BaseAction
{
	JumpToTagEditor() : BaseAction(Type::JumpToTagEditor, "jump_to_tag_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct JumpToPositionInSong : public BaseAction
{
	JumpToPositionInSong() : BaseAction(Type::JumpToPositionInSong, "jump_to_position_in_song") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReverseSelection : public BaseAction
{
	ReverseSelection() : BaseAction(Type::ReverseSelection, "reverse_selection") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct RemoveSelection : public BaseAction
{
	RemoveSelection() : BaseAction(Type::RemoveSelection, "remove_selection") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SelectAlbum : public BaseAction
{
	SelectAlbum() : BaseAction(Type::SelectAlbum, "select_album") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct AddSelectedItems : public BaseAction
{
	AddSelectedItems() : BaseAction(Type::AddSelectedItems, "add_selected_items") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct CropMainPlaylist : public BaseAction
{
	CropMainPlaylist() : BaseAction(Type::CropMainPlaylist, "crop_main_playlist") { }
	
protected:
	virtual void run();
};

struct CropPlaylist : public BaseAction
{
	CropPlaylist() : BaseAction(Type::CropPlaylist, "crop_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ClearMainPlaylist : public BaseAction
{
	ClearMainPlaylist() : BaseAction(Type::ClearMainPlaylist, "clear_main_playlist") { }
	
protected:
	virtual void run();
};

struct ClearPlaylist : public BaseAction
{
	ClearPlaylist() : BaseAction(Type::ClearPlaylist, "clear_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SortPlaylist : public BaseAction
{
	SortPlaylist() : BaseAction(Type::SortPlaylist, "sort_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ReversePlaylist : public BaseAction
{
	ReversePlaylist() : BaseAction(Type::ReversePlaylist, "reverse_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ApplyFilter : public BaseAction
{
	ApplyFilter() : BaseAction(Type::ApplyFilter, "apply_filter") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct Find : public BaseAction
{
	Find() : BaseAction(Type::Find, "find") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FindItemForward : public BaseAction
{
	FindItemForward() : BaseAction(Type::FindItemForward, "find_item_forward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FindItemBackward : public BaseAction
{
	FindItemBackward() : BaseAction(Type::FindItemBackward, "find_item_backward") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct NextFoundItem : public BaseAction
{
	NextFoundItem() : BaseAction(Type::NextFoundItem, "next_found_item") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct PreviousFoundItem : public BaseAction
{
	PreviousFoundItem() : BaseAction(Type::PreviousFoundItem, "previous_found_item") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleFindMode : public BaseAction
{
	ToggleFindMode() : BaseAction(Type::ToggleFindMode, "toggle_find_mode") { }
	
protected:
	virtual void run();
};

struct ToggleReplayGainMode : public BaseAction
{
	ToggleReplayGainMode() : BaseAction(Type::ToggleReplayGainMode, "toggle_replay_gain_mode") { }
	
protected:
	virtual void run();
};

struct ToggleSpaceMode : public BaseAction
{
	ToggleSpaceMode() : BaseAction(Type::ToggleSpaceMode, "toggle_space_mode") { }
	
protected:
	virtual void run();
};

struct ToggleAddMode : public BaseAction
{
	ToggleAddMode() : BaseAction(Type::ToggleAddMode, "toggle_add_mode") { }
	
protected:
	virtual void run();
};

struct ToggleMouse : public BaseAction
{
	ToggleMouse() : BaseAction(Type::ToggleMouse, "toggle_mouse") { }
	
protected:
	virtual void run();
};

struct ToggleBitrateVisibility : public BaseAction
{
	ToggleBitrateVisibility() : BaseAction(Type::ToggleBitrateVisibility, "toggle_bitrate_visibility") { }
	
protected:
	virtual void run();
};

struct AddRandomItems : public BaseAction
{
	AddRandomItems() : BaseAction(Type::AddRandomItems, "add_random_items") { }
	
protected:
	virtual void run();
};

struct ToggleBrowserSortMode : public BaseAction
{
	ToggleBrowserSortMode() : BaseAction(Type::ToggleBrowserSortMode, "toggle_browser_sort_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleLibraryTagType : public BaseAction
{
	ToggleLibraryTagType() : BaseAction(Type::ToggleLibraryTagType, "toggle_library_tag_type") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleMediaLibrarySortMode : public BaseAction
{
	ToggleMediaLibrarySortMode()
	: BaseAction(Type::ToggleMediaLibrarySortMode, "toggle_media_library_sort_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct RefetchLyrics : public BaseAction
{
	RefetchLyrics() : BaseAction(Type::RefetchLyrics, "refetch_lyrics") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SetSelectedItemsPriority : public BaseAction
{
	SetSelectedItemsPriority()
	: BaseAction(Type::SetSelectedItemsPriority, "set_selected_items_priority") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct SetVisualizerSampleMultiplier : public BaseAction
{
	SetVisualizerSampleMultiplier()
	: BaseAction(Type::SetVisualizerSampleMultiplier, "set_visualizer_sample_multiplier") { }

protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct FilterPlaylistOnPriorities : public BaseAction
{
	FilterPlaylistOnPriorities()
	: BaseAction(Type::FilterPlaylistOnPriorities, "filter_playlist_on_priorities") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowSongInfo : public BaseAction
{
	ShowSongInfo() : BaseAction(Type::ShowSongInfo, "show_song_info") { }
	
protected:
	virtual void run();
};

struct ShowArtistInfo : public BaseAction
{
	ShowArtistInfo() : BaseAction(Type::ShowArtistInfo, "show_artist_info") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowLyrics : public BaseAction
{
	ShowLyrics() : BaseAction(Type::ShowLyrics, "show_lyrics") { }
	
protected:
	virtual void run();
};

struct Quit : public BaseAction
{
	Quit() : BaseAction(Type::Quit, "quit") { }
	
protected:
	virtual void run();
};

struct NextScreen : public BaseAction
{
	NextScreen() : BaseAction(Type::NextScreen, "next_screen") { }
	
protected:
	virtual void run();
};

struct PreviousScreen : public BaseAction
{
	PreviousScreen() : BaseAction(Type::PreviousScreen, "previous_screen") { }
	
protected:
	virtual void run();
};

struct ShowHelp : public BaseAction
{
	ShowHelp() : BaseAction(Type::ShowHelp, "show_help") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowPlaylist : public BaseAction
{
	ShowPlaylist() : BaseAction(Type::ShowPlaylist, "show_playlist") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowBrowser : public BaseAction
{
	ShowBrowser() : BaseAction(Type::ShowBrowser, "show_browser") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ChangeBrowseMode : public BaseAction
{
	ChangeBrowseMode() : BaseAction(Type::ChangeBrowseMode, "change_browse_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowSearchEngine : public BaseAction
{
	ShowSearchEngine() : BaseAction(Type::ShowSearchEngine, "show_search_engine") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ResetSearchEngine : public BaseAction
{
	ResetSearchEngine() : BaseAction(Type::ResetSearchEngine, "reset_search_engine") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowMediaLibrary : public BaseAction
{
	ShowMediaLibrary() : BaseAction(Type::ShowMediaLibrary, "show_media_library") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ToggleMediaLibraryColumnsMode : public BaseAction
{
	ToggleMediaLibraryColumnsMode()
	: BaseAction(Type::ToggleMediaLibraryColumnsMode, "toggle_media_library_columns_mode") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowPlaylistEditor : public BaseAction
{
	ShowPlaylistEditor() : BaseAction(Type::ShowPlaylistEditor, "show_playlist_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowTagEditor : public BaseAction
{
	ShowTagEditor() : BaseAction(Type::ShowTagEditor, "show_tag_editor") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowOutputs : public BaseAction
{
	ShowOutputs() : BaseAction(Type::ShowOutputs, "show_outputs") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowVisualizer : public BaseAction
{
	ShowVisualizer() : BaseAction(Type::ShowVisualizer, "show_visualizer") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowClock : public BaseAction
{
	ShowClock() : BaseAction(Type::ShowClock, "show_clock") { }
	
protected:
	virtual bool canBeRun() const;
	virtual void run();
};

struct ShowServerInfo : public BaseAction
{
	ShowServerInfo() : BaseAction(Type::ShowServerInfo, "show_server_info") { }
	
protected:
#	ifdef HAVE_TAGLIB_H
	virtual bool canBeRun() const;
#	endif // HAVE_TAGLIB_H
	virtual void run();
};

}

#endif // NCMPCPP_ACTIONS_H
