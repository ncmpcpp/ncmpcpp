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

#include "mpdpp.h"

#include "bindings.h"
#include "global.h"
#include "help.h"
#include "settings.h"
#include "status.h"
#include "utility/wide_string.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

Help *myHelp;

namespace {//

std::string keyToString(const Key &key, bool *print_backspace)
{
	std::string result;
	if (key == Key(KEY_UP, Key::NCurses))
		result += "Up";
	else if (key == Key(KEY_DOWN, Key::NCurses))
		result += "Down";
	else if (key == Key(KEY_PPAGE, Key::NCurses))
		result += "PageUp";
	else if (key == Key(KEY_NPAGE, Key::NCurses))
		result += "PageDown";
	else if (key == Key(KEY_HOME, Key::NCurses))
		result += "Home";
	else if (key == Key(KEY_END, Key::NCurses))
		result += "End";
	else if (key == Key(KEY_SPACE, Key::Standard))
		result += "Space";
	else if (key == Key(KEY_ENTER, Key::Standard))
		result += "Enter";
	else if (key == Key(KEY_IC, Key::NCurses))
		result += "Insert";
	else if (key == Key(KEY_DC, Key::NCurses))
		result += "Delete";
	else if (key == Key(KEY_RIGHT, Key::NCurses))
		result += "Right";
	else if (key == Key(KEY_LEFT, Key::NCurses))
		result += "Left";
	else if (key == Key(KEY_TAB, Key::Standard))
		result += "Tab";
	else if (key == Key(KEY_SHIFT_TAB, Key::NCurses))
		result += "Shift-Tab";
	else if (key >= Key(KEY_CTRL_A, Key::Standard) && key <= Key(KEY_CTRL_Z, Key::Standard))
	{
		result += "Ctrl-";
		result += key.getChar()+64;
	}
	else if (key >= Key(KEY_F1, Key::NCurses) && key <= Key(KEY_F12, Key::NCurses))
	{
		result += "F";
		result += intTo<std::string>::apply(key.getChar()-264);
	}
	else if ((key == Key(KEY_BACKSPACE, Key::NCurses) || key == Key(KEY_BACKSPACE_2, Key::Standard)))
	{
		// since some terminals interpret KEY_BACKSPACE as backspace and other need KEY_BACKSPACE_2,
		// actions have to be bound to either of them, but we want to display "Backspace" only once,
		// hance this 'print_backspace' switch.
		if (!print_backspace || *print_backspace)
		{
			result += "Backspace";
			if (print_backspace)
				*print_backspace = false;
		}
	}
	else
		result += ToString(std::wstring(1, key.getChar()));
	return result;
}

}

Help::Help()
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone))
{
	GetKeybindings();
	w.flush();
}

void Help::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

void Help::switchTo()
{
	SwitchTo::execute(this);
	drawHeader();
}

std::wstring Help::title()
{
	return L"Help";
}

std::string Help::DisplayKeys(const ActionType at)
{
	bool print_backspace = true;
	std::string result;
	for (auto it = Bindings.begin(); it != Bindings.end(); ++it)
	{
		if (it->second.isSingle() && it->second.action()->Type() == at)
		{
			result += keyToString(it->first, &print_backspace);
			result += " ";
		}
	}
	result.resize(16, ' ');
	return result;
}

void Help::Section(const char *type, const char *title_)
{
	w << L"\n  " << NC::fmtBold << ToWString(type) << L" - ";
	w << ToWString(title_) << NC::fmtBoldEnd << L"\n\n";
}

void Help::KeyDesc(const ActionType at, const char *desc)
{
	w << L"    " << DisplayKeys(at) << L" : " << ToWString(desc) << '\n';
}

void Help::MouseDesc(std::string action, const char *desc, bool indent)
{
	action.resize(31 - (indent ? 2 : 0), ' ');
	w << L"    " << (indent ? L"  " : L"") << ToWString(action);
	w << L": " << ToWString(desc) << '\n';
}

void Help::MouseColumn(const char *column)
{
	w << NC::fmtBold << L"    " << ToWString(column) << L" column:\n" << NC::fmtBoldEnd;
}

void Help::GetKeybindings()
{
	KeysSection("Movement");
	KeyDesc(aScrollUp, "Move cursor up");
	KeyDesc(aScrollDown, "Move cursor down");
	KeyDesc(aScrollUpAlbum, "Move cursor up one album");
	KeyDesc(aScrollDownAlbum, "Move cursor down one album");
	KeyDesc(aScrollUpArtist, "Move cursor up one artist");
	KeyDesc(aScrollDownArtist, "Move cursor down one artist");
	KeyDesc(aPageUp, "Page up");
	KeyDesc(aPageDown, "Page down");
	KeyDesc(aMoveHome, "Home");
	KeyDesc(aMoveEnd, "End");
	w << '\n';
	if (Config.screen_switcher_previous)
	{
		KeyDesc(aNextScreen, "Switch between current and last screen");
		KeyDesc(aPreviousScreen, "Switch between current and last screen");
	}
	else
	{
		KeyDesc(aNextScreen, "Switch to next screen in sequence");
		KeyDesc(aPreviousScreen, "Switch to previous screen in sequence");
	}
	KeyDesc(aShowHelp, "Show help");
	KeyDesc(aShowPlaylist, "Show playlist");
	KeyDesc(aShowBrowser, "Show browser");
	KeyDesc(aShowSearchEngine, "Show search engine");
	KeyDesc(aShowMediaLibrary, "Show media library");
	KeyDesc(aShowPlaylistEditor, "Show playlist editor");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(aShowTagEditor, "Show tag editor");
#	endif // HAVE_TAGLIB_H
#	ifdef ENABLE_OUTPUTS
	KeyDesc(aShowOutputs, "Show outputs");
#	endif // ENABLE_OUTPUTS
#	ifdef ENABLE_VISUALIZER
	KeyDesc(aShowVisualizer, "Show music visualizer");
#	endif // ENABLE_VISUALIZER
#	ifdef ENABLE_CLOCK
	KeyDesc(aShowClock, "Show clock");
#	endif // ENABLE_CLOCK
	w << '\n';
	KeyDesc(aShowServerInfo, "Show server info");
	
	KeysSection("Global");
	KeyDesc(aStop, "Stop");
	KeyDesc(aPause, "Pause");
	KeyDesc(aNext, "Next track");
	KeyDesc(aPrevious, "Previous track");
	KeyDesc(aReplaySong, "Replay playing song");
	KeyDesc(aSeekForward, "Seek forward in playing song");
	KeyDesc(aSeekBackward, "Seek backward in playing song");
	KeyDesc(aVolumeDown, "Decrease volume by 2%");
	KeyDesc(aVolumeUp, "Increase volume by 2%");
	w << '\n';
	KeyDesc(aToggleSpaceMode, "Toggle space mode (select/add)");
	KeyDesc(aToggleAddMode, "Toggle add mode (add or remove/always add)");
	KeyDesc(aToggleMouse, "Toggle mouse support");
	KeyDesc(aReverseSelection, "Reverse selection");
	KeyDesc(aRemoveSelection, "Remove selection");
	KeyDesc(aSelectAlbum, "Select songs of album around the cursor");
	KeyDesc(aAddSelectedItems, "Add selected items to playlist");
	KeyDesc(aAddRandomItems, "Add random items to playlist");
	w << '\n';
	KeyDesc(aToggleRepeat, "Toggle repeat mode");
	KeyDesc(aToggleRandom, "Toggle random mode");
	KeyDesc(aToggleSingle, "Toggle single mode");
	KeyDesc(aToggleConsume, "Toggle consume mode");
	KeyDesc(aToggleReplayGainMode, "Toggle replay gain mode");
	KeyDesc(aToggleBitrateVisibility, "Toggle bitrate visibility");
	KeyDesc(aShuffle, "Shuffle playlist");
	KeyDesc(aToggleCrossfade, "Toggle crossfade mode");
	KeyDesc(aSetCrossfade, "Set crossfade");
	KeyDesc(aUpdateDatabase, "Start music database update");
	w << '\n';
	KeyDesc(aApplyFilter, "Apply filter");
	KeyDesc(aFindItemForward, "Find item forward");
	KeyDesc(aFindItemBackward, "Find item backward");
	KeyDesc(aPreviousFoundItem, "Jump to previous found item");
	KeyDesc(aNextFoundItem, "Jump to next found item");
	KeyDesc(aToggleFindMode, "Toggle find mode (normal/wrapped)");
	KeyDesc(aJumpToBrowser, "Locate song in browser");
	KeyDesc(aJumpToMediaLibrary, "Locate song in media library");
	KeyDesc(aToggleScreenLock, "Lock/unlock current screen");
	KeyDesc(aMasterScreen, "Switch to master screen (left one)");
	KeyDesc(aSlaveScreen, "Switch to slave screen (right one)");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(aJumpToTagEditor, "Locate song in tag editor");
#	endif // HAVE_TAGLIB_H
	KeyDesc(aToggleDisplayMode, "Toggle display mode");
	KeyDesc(aToggleInterface, "Toggle user interface");
	KeyDesc(aToggleSeparatorsBetweenAlbums, "Toggle displaying separators between albums");
	KeyDesc(aJumpToPositionInSong, "Jump to given position in playing song (formats: mm:ss, x%)");
	KeyDesc(aShowSongInfo, "Show song info");
#	ifdef HAVE_CURL_CURL_H
	KeyDesc(aShowArtistInfo, "Show artist info");
	KeyDesc(aToggleLyricsFetcher, "Toggle lyrics fetcher");
	KeyDesc(aToggleFetchingLyricsInBackground, "Toggle fetching lyrics for playing songs in background");
#	endif // HAVE_CURL_CURL_H
	KeyDesc(aShowLyrics, "Show/hide song lyrics");
	w << '\n';
	KeyDesc(aQuit, "Quit");
	
	KeysSection("Playlist");
	KeyDesc(aPressEnter, "Play selected item");
	KeyDesc(aDelete, "Delete selected item(s) from playlist");
	KeyDesc(aClearMainPlaylist, "Clear playlist");
	KeyDesc(aCropMainPlaylist, "Clear playlist except selected item(s)");
	KeyDesc(aSetSelectedItemsPriority, "Set priority of selected items");
	KeyDesc(aMoveSelectedItemsUp, "Move selected item(s) up");
	KeyDesc(aMoveSelectedItemsDown, "Move selected item(s) down");
	KeyDesc(aMoveSelectedItemsTo, "Move selected item(s) to cursor position");
	KeyDesc(aAdd, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(aEditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(aSavePlaylist, "Save playlist");
	KeyDesc(aSortPlaylist, "Sort playlist");
	KeyDesc(aReversePlaylist, "Reverse playlist");
	KeyDesc(aFilterPlaylistOnPriorities, "Filter playlist on priorities");
	KeyDesc(aJumpToPlayingSong, "Jump to playing song");
	KeyDesc(aTogglePlayingSongCentering, "Toggle playing song centering");
	
	KeysSection("Browser");
	KeyDesc(aPressEnter, "Enter directory/Add item to playlist and play it");
	KeyDesc(aPressSpace, "Add item to playlist/select it");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(aEditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(aEditDirectoryName, "Edit directory name");
	KeyDesc(aEditPlaylistName, "Edit playlist name");
	KeyDesc(aShowBrowser, "Browse MPD database/local filesystem");
	KeyDesc(aToggleBrowserSortMode, "Toggle sort mode");
	KeyDesc(aJumpToPlayingSong, "Locate playing song");
	KeyDesc(aJumpToParentDirectory, "Jump to parent directory");
	KeyDesc(aDelete, "Delete item");
	KeyDesc(aJumpToPlaylistEditor, "Jump to playlist editor (playlists only)");
	
	KeysSection("Search engine");
	KeyDesc(aPressEnter, "Add item to playlist and play it/change option");
	KeyDesc(aPressSpace, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(aEditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(aStartSearching, "Start searching");
	KeyDesc(aShowSearchEngine, "Reset search constraints and clear results");
	
	KeysSection("Media library");
	if (!Config.media_library_disable_two_column_mode)
		KeyDesc(aShowMediaLibrary, "Switch between two/three columns mode");
	KeyDesc(aPreviousColumn, "Previous column");
	KeyDesc(aNextColumn, "Next column");
	KeyDesc(aPressEnter, "Add item to playlist and play it");
	KeyDesc(aPressSpace, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(aEditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(aEditLibraryTag, "Edit tag (left column)/album (middle/right column)");
	KeyDesc(aToggleLibraryTagType, "Toggle type of tag used in left column");
	
	KeysSection("Playlist editor");
	KeyDesc(aPreviousColumn, "Previous column");
	KeyDesc(aNextColumn, "Next column");
	KeyDesc(aPressEnter, "Add item to playlist and play it");
	KeyDesc(aPressSpace, "Add item to playlist/select it");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(aEditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(aEditPlaylistName, "Edit playlist name");
	KeyDesc(aMoveSelectedItemsUp, "Move selected item(s) up");
	KeyDesc(aMoveSelectedItemsDown, "Move selected item(s) down");
	KeyDesc(aClearPlaylist, "Clear playlist");
	KeyDesc(aCropPlaylist, "Clear playlist except selected items");
	
	KeysSection("Lyrics");
	KeyDesc(aPressSpace, "Toggle reloading lyrics upon song change");
	KeyDesc(aEditLyrics, "Open lyrics in external editor");
	KeyDesc(aRefetchLyrics, "Refetch lyrics");
	
	KeysSection("Artist info");
	KeyDesc(aRefetchArtistInfo, "Refetch artist info");
	
#	ifdef HAVE_TAGLIB_H
	KeysSection("Tiny tag editor");
	KeyDesc(aPressEnter, "Edit tag");
	KeyDesc(aSaveTagChanges, "Save");
	
	KeysSection("Tag editor");
	KeyDesc(aPressEnter, "Edit tag/filename of selected item (left column)");
	KeyDesc(aPressEnter, "Perform operation on all/selected items (middle column)");
	KeyDesc(aPressSpace, "Switch to albums/directories view (left column)");
	KeyDesc(aPressSpace, "Select item (right column)");
	KeyDesc(aPreviousColumn, "Previous column");
	KeyDesc(aNextColumn, "Next column");
	KeyDesc(aJumpToParentDirectory, "Jump to parent directory (left column, directories view)");
#	endif // HAVE_TAGLIB_H
	
#	ifdef ENABLE_OUTPUTS
	KeysSection("Outputs");
	KeyDesc(aPressEnter, "Toggle output");
#	endif // ENABLE_OUTPUTS
	
#	if defined(ENABLE_VISUALIZER) && defined(HAVE_FFTW3_H)
	KeysSection("Music visualizer");
	KeyDesc(aPressSpace, "Toggle visualization type");
#	endif // ENABLE_VISUALIZER && HAVE_FFTW3_H
	
	MouseSection("Global");
	MouseDesc("Left click on \"Playing/Paused\"", "Play/pause");
	MouseDesc("Left click on progressbar", "Jump to pointed position in playing song");
	w << '\n';
	MouseDesc("Mouse wheel on \"Volume: xx\"", "Play/pause");
	MouseDesc("Mouse wheel on main window", "Scroll");
	
	MouseSection("Playlist");
	MouseDesc("Left click", "Select pointed item");
	MouseDesc("Right click", "Play");
	
	MouseSection("Browser");
	MouseDesc("Left click on directory", "Enter pointed directory");
	MouseDesc("Right click on directory", "Add pointed directory to playlist");
	w << '\n';
	MouseDesc("Left click on song/playlist", "Add pointed item to playlist");
	MouseDesc("Right click on song/playlist", "Add pointed item to playlist and play it");
	
	MouseSection("Search engine");
	MouseDesc("Left click", "Highlight/switch value");
	MouseDesc("Right click", "Change value");
	
	MouseSection("Media library");
	MouseColumn("Left/middle");
	MouseDesc("Left click", "Select pointed item", true);
	MouseDesc("Right click", "Add item to playlist", true);
	w << '\n';
	MouseColumn("Right");
	MouseDesc("Left Click", "Add pointed item to playlist", true);
	MouseDesc("Right Click", "Add pointed item to playlist and play it", true);
	
	MouseSection("Playlist editor");
	MouseColumn("Left");
	MouseDesc("Left click", "Select pointed item", true);
	MouseDesc("Right click", "Add item to playlist", true);
	w << '\n';
	MouseColumn("Right");
	MouseDesc("Left click", "Add pointed item to playlist", true);
	MouseDesc("Right click", "Add pointed item to playlist and play it", true);
	
#	ifdef HAVE_TAGLIB_H
	MouseSection("Tiny tag editor");
	MouseDesc("Left click", "Select option");
	MouseDesc("Right click", "Set value/execute");
	
	MouseSection("Tag editor");
	MouseColumn("Left");
	MouseDesc("Left click", "Enter pointed directory/select pointed album", true);
	MouseDesc("Right click", "Toggle view (directories/albums)", true);
	w << '\n';
	MouseColumn("Middle");
	MouseDesc("Left click", "Select option", true);
	MouseDesc("Right click", "Set value/execute", true);
	w << '\n';
	MouseColumn("Right");
	MouseDesc("Left click", "Select pointed item", true);
	MouseDesc("Right click", "Set value", true);
#	endif // HAVE_TAGLIB_H
	
#	ifdef ENABLE_OUTPUTS
	MouseSection("Outputs");
	MouseDesc("Left click", "Select pointed output");
	MouseDesc("Right click", "Toggle output");
#	endif // ENABLE_OUTPUTS

}

