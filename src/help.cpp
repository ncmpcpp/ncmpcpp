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

#include "global.h"
#include "help.h"
#include "settings.h"
#include "tag_editor.h"

using Global::MainHeight;
using Global::MainStartY;

Help *myHelp = new Help;

void Help::Init()
{
	w = new Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	GetKeybindings();
	w->Flush();
	isInitialized = 1;
}

void Help::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->Resize(width, MainHeight);
	w->MoveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

void Help::SwitchTo()
{
	using Global::myScreen;
	using Global::myLockedScreen;
	
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	Global::RedrawHeader = 1;

}

std::basic_string<my_char_t> Help::Title()
{
	return U("Help");
}

std::string Help::DisplayKeys(const ActionType at)
{
	bool backspace = true;
	std::string result;
	for (std::multimap<int, Action *>::const_iterator it = Key.Bindings.begin(); it != Key.Bindings.end(); ++it)
	{
		if (it->second->Type() == at)
		{
			int key = it->first;
			
			if (key == 259)
				result += "Up";
			else if (key == 258)
				result += "Down";
			else if (key == 339)
				result += "PageUp";
			else if (key == 338)
				result += "PageDown";
			else if (key == 262)
				result += "Home";
			else if (key == 360)
				result += "End";
			else if (key == 32)
				result += "Space";
			else if (key == 10)
				result += "Enter";
			else if (key == 330)
				result += "Delete";
			else if (key == 261)
				result += "Right";
			else if (key == 260)
				result += "Left";
			else if (key == 9)
				result += "Tab";
			else if (key == 353)
				result += "Shift-Tab";
			else if (key >= 1 && key <= 26)
			{
				result += "Ctrl-";
				result += key+64;
			}
			else if (key >= 265 && key <= 276)
			{
				result += "F";
				result += IntoStr(key-264);
			}
			else if ((key == 263 || key == 127) && !backspace);
			else if ((key == 263 || key == 127) && backspace)
			{
				result += "Backspace";
				backspace = false;
			}
			else if (isprint(key))
				result += key;
			result += " ";
		}
	}
	result.resize(12, ' ');
	return result;
}

void Help::Section(const char *type, const char *title)
{
	*w << "\n  " << fmtBold << type << " - " << title << fmtBoldEnd << "\n\n";
}

void Help::KeyDesc(const ActionType at, const char *desc)
{
	*w << "    " << DisplayKeys(at) << " : " << desc << "\n";
}

void Help::MouseDesc(std::string action, const char *desc, bool indent)
{
	action.resize(31 - (indent ? 2 : 0), ' ');
	*w << "    " << (indent ? "  " : "") << action << ": " << desc << "\n";
}

void Help::MouseColumn(const char *column)
{
	*w << fmtBold << "    " << column << " column:\n" << fmtBoldEnd;
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
	*w << "\n";
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
	*w << "\n";
	KeyDesc(aShowServerInfo, "Show server info");
	
	KeysSection("Global");
	KeyDesc(aStop, "Stop");
	KeyDesc(aPause, "Pause");
	KeyDesc(aNextSong, "Next track");
	KeyDesc(aPreviousSong, "Previous track");
	KeyDesc(aReplaySong, "Replay playing song");
	KeyDesc(aSeekForward, "Seek forward in playing song");
	KeyDesc(aSeekBackward, "Seek backward in playing song");
	KeyDesc(aVolumeDown, "Decrease volume by 2%");
	KeyDesc(aVolumeUp, "Increase volume by 2%");
	*w << "\n";
	KeyDesc(aToggleSpaceMode, "Toggle space mode (select/add)");
	KeyDesc(aToggleAddMode, "Toggle add mode (add or remove/always add)");
	KeyDesc(aToggleMouse, "Toggle mouse support");
	KeyDesc(aReverseSelection, "Reverse selection");
	KeyDesc(aDeselectItems, "Deselect items");
	KeyDesc(aSelectAlbum, "Select songs of album around the cursor");
	KeyDesc(aAddSelectedItems, "Add selected items to playlist");
	KeyDesc(aAddRandomItems, "Add random items to playlist");
	*w << "\n";
	KeyDesc(aToggleRepeat, "Toggle repeat mode");
	KeyDesc(aToggleRandom, "Toggle random mode");
	KeyDesc(aToggleSingle, "Toggle single mode");
	KeyDesc(aToggleConsume, "Toggle consume mode");
	if (Mpd.Version() >= 16)
		KeyDesc(aToggleReplayGainMode, "Toggle replay gain mode");
	KeyDesc(aToggleBitrateVisibility, "Toggle bitrate visibility");
	KeyDesc(aShuffle, "Shuffle playlist");
	KeyDesc(aToggleCrossfade, "Toggle crossfade mode");
	KeyDesc(aSetCrossfade, "Set crossfade");
	KeyDesc(aUpdateDatabase, "Start music database update");
	*w << "\n";
	KeyDesc(aApplyFilter, "Apply filter");
	KeyDesc(aDisableFilter, "Disable filter");
	KeyDesc(aFindItemForward, "Find item forward");
	KeyDesc(aFindItemBackward, "Find item backward");
	KeyDesc(aPreviousFoundItem, "Go to previous found item");
	KeyDesc(aNextFoundItem, "Go to next found item");
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
	KeyDesc(aToggleSeparatorsInPlaylist, "Toggle displaying separators between albums");
	KeyDesc(aJumpToPositionInSong, "Jump to given position in playing song (formats: mm:ss, x%)");
	KeyDesc(aShowSongInfo, "Show song info");
#	ifdef HAVE_CURL_CURL_H
	KeyDesc(aShowArtistInfo, "Show artist info");
	KeyDesc(aToggleLyricsFetcher, "Toggle lyrics fetcher");
	KeyDesc(aToggleFetchingLyricsInBackground, "Toggle fetching lyrics for playing songs in background");
#	endif // HAVE_CURL_CURL_H
	KeyDesc(aShowLyrics, "Show/hide song lyrics");
	*w << "\n";
	KeyDesc(aQuit, "Quit");
	
	KeysSection("Playlist");
	KeyDesc(aPressEnter, "Play selected item");
	KeyDesc(aDelete, "Delete selected item(s) from playlist");
	KeyDesc(aClearMainPlaylist, "Clear playlist");
	KeyDesc(aCropMainPlaylist, "Clear playlist except playing/selected items");
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
	KeyDesc(aJumpToPlayingSong, "Jump to playing song");
	KeyDesc(aToggleAutoCenter, "Toggle auto center mode");
	
	KeysSection("Browser");
	KeyDesc(aPressEnter, "Enter directory/Add item to playlist and play it");
	KeyDesc(aPressSpace, "Add item to playlist/select it");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(aEditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(aEditDirectoryName, "Edit directory name");
	KeyDesc(aEditPlaylistName, "Edit playlist name");
	if (Mpd.GetHostname()[0] == '/') // are we connected to unix socket?
		KeyDesc(aShowBrowser, "Browse MPD database/local filesystem");
	KeyDesc(aToggleBrowserSortMode, "Toggle sort mode");
	KeyDesc(aJumpToPlayingSong, "Locate playing song");
	KeyDesc(aJumpToParentDir, "Go to parent directory");
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
	KeyDesc(aJumpToParentDir, "Go to parent directory (left column, directories view)");
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
	*w << "\n";
	MouseDesc("Mouse wheel on \"Volume: xx\"", "Play/pause");
	MouseDesc("Mouse wheel on main window", "Scroll");
	
	MouseSection("Playlist");
	MouseDesc("Left click", "Select pointed item");
	MouseDesc("Right click", "Play");
	
	MouseSection("Browser");
	MouseDesc("Left click on directory", "Enter pointed directory");
	MouseDesc("Right click on directory", "Add pointed directory to playlist");
	*w << "\n";
	MouseDesc("Left click on song/playlist", "Add pointed item to playlist");
	MouseDesc("Right click on song/playlist", "Add pointed item to playlist and play it");
	
	MouseSection("Search engine");
	MouseDesc("Left click", "Highlight/switch value");
	MouseDesc("Right click", "Change value");
	
	MouseSection("Media library");
	MouseColumn("Left/middle");
	MouseDesc("Left click", "Select pointed item", true);
	MouseDesc("Right click", "Add item to playlist", true);
	*w << "\n";
	MouseColumn("Right");
	MouseDesc("Left Click", "Add pointed item to playlist", true);
	MouseDesc("Right Click", "Add pointed item to playlist and play it", true);
	
	MouseSection("Playlist editor");
	MouseColumn("Left");
	MouseDesc("Left click", "Select pointed item", true);
	MouseDesc("Right click", "Add item to playlist", true);
	*w << "\n";
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
	*w << "\n";
	MouseColumn("Middle");
	MouseDesc("Left click", "Select option", true);
	MouseDesc("Right click", "Set value/execute", true);
	*w << "\n";
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

