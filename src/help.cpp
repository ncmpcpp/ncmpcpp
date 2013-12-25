/***************************************************************************
 *   Copyright (C) 2008-2013 by Andrzej Rybczak                            *
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
		result += boost::lexical_cast<std::string>(key.getChar()-264);
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
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border::None))
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

std::string Help::DisplayKeys(const Actions::Type at)
{
	bool print_backspace = true;
	std::string result;
	for (auto it = Bindings.begin(); it != Bindings.end(); ++it)
	{
		for (auto j = it->second.begin(); j != it->second.end(); ++j)
		{
			if (j->isSingle() && j->action()->type() == at)
			{
				result += keyToString(it->first, &print_backspace);
				result += " ";
			}
		}
	}
	result.resize(16, ' ');
	return result;
}

void Help::Section(const char *type_, const char *title_)
{
	w << "\n  " << NC::Format::Bold << type_ << " - ";
	w << title_ << NC::Format::NoBold << "\n\n";
}

void Help::KeyDesc(const Actions::Type at, const char *desc)
{
	w << "    " << DisplayKeys(at) << " : " << desc << '\n';
}

void Help::MouseDesc(std::string action, const char *desc, bool indent)
{
	action.resize(31 - (indent ? 2 : 0), ' ');
	w << "    " << (indent ? "  " : "") << action;
	w << ": " << desc << '\n';
}

void Help::MouseColumn(const char *column)
{
	w << NC::Format::Bold << "    " << column << " column:\n" << NC::Format::NoBold;
}

void Help::GetKeybindings()
{
	KeysSection("Movement");
	KeyDesc(Actions::Type::ScrollUp, "Move cursor up");
	KeyDesc(Actions::Type::ScrollDown, "Move cursor down");
	KeyDesc(Actions::Type::ScrollUpAlbum, "Move cursor up one album");
	KeyDesc(Actions::Type::ScrollDownAlbum, "Move cursor down one album");
	KeyDesc(Actions::Type::ScrollUpArtist, "Move cursor up one artist");
	KeyDesc(Actions::Type::ScrollDownArtist, "Move cursor down one artist");
	KeyDesc(Actions::Type::PageUp, "Page up");
	KeyDesc(Actions::Type::PageDown, "Page down");
	KeyDesc(Actions::Type::MoveHome, "Home");
	KeyDesc(Actions::Type::MoveEnd, "End");
	w << '\n';
	if (Config.screen_switcher_previous)
	{
		KeyDesc(Actions::Type::NextScreen, "Switch between current and last screen");
		KeyDesc(Actions::Type::PreviousScreen, "Switch between current and last screen");
	}
	else
	{
		KeyDesc(Actions::Type::NextScreen, "Switch to next screen in sequence");
		KeyDesc(Actions::Type::PreviousScreen, "Switch to previous screen in sequence");
	}
	KeyDesc(Actions::Type::ShowHelp, "Show help");
	KeyDesc(Actions::Type::ShowPlaylist, "Show playlist");
	KeyDesc(Actions::Type::ShowBrowser, "Show browser");
	KeyDesc(Actions::Type::ShowSearchEngine, "Show search engine");
	KeyDesc(Actions::Type::ShowMediaLibrary, "Show media library");
	KeyDesc(Actions::Type::ShowPlaylistEditor, "Show playlist editor");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(Actions::Type::ShowTagEditor, "Show tag editor");
#	endif // HAVE_TAGLIB_H
#	ifdef ENABLE_OUTPUTS
	KeyDesc(Actions::Type::ShowOutputs, "Show outputs");
#	endif // ENABLE_OUTPUTS
#	ifdef ENABLE_VISUALIZER
	KeyDesc(Actions::Type::ShowVisualizer, "Show music visualizer");
#	endif // ENABLE_VISUALIZER
#	ifdef ENABLE_CLOCK
	KeyDesc(Actions::Type::ShowClock, "Show clock");
#	endif // ENABLE_CLOCK
	w << '\n';
	KeyDesc(Actions::Type::ShowServerInfo, "Show server info");
	
	KeysSection("Global");
	KeyDesc(Actions::Type::Stop, "Stop");
	KeyDesc(Actions::Type::Pause, "Pause");
	KeyDesc(Actions::Type::Next, "Next track");
	KeyDesc(Actions::Type::Previous, "Previous track");
	KeyDesc(Actions::Type::ReplaySong, "Replay playing song");
	KeyDesc(Actions::Type::SeekForward, "Seek forward in playing song");
	KeyDesc(Actions::Type::SeekBackward, "Seek backward in playing song");
	KeyDesc(Actions::Type::VolumeDown, "Decrease volume by 2%");
	KeyDesc(Actions::Type::VolumeUp, "Increase volume by 2%");
	w << '\n';
	KeyDesc(Actions::Type::ToggleSpaceMode, "Toggle space mode (select/add)");
	KeyDesc(Actions::Type::ToggleAddMode, "Toggle add mode (add or remove/always add)");
	KeyDesc(Actions::Type::ToggleMouse, "Toggle mouse support");
	KeyDesc(Actions::Type::ReverseSelection, "Reverse selection");
	KeyDesc(Actions::Type::RemoveSelection, "Remove selection");
	KeyDesc(Actions::Type::SelectAlbum, "Select songs of album around the cursor");
	KeyDesc(Actions::Type::AddSelectedItems, "Add selected items to playlist");
	KeyDesc(Actions::Type::AddRandomItems, "Add random items to playlist");
	w << '\n';
	KeyDesc(Actions::Type::ToggleRepeat, "Toggle repeat mode");
	KeyDesc(Actions::Type::ToggleRandom, "Toggle random mode");
	KeyDesc(Actions::Type::ToggleSingle, "Toggle single mode");
	KeyDesc(Actions::Type::ToggleConsume, "Toggle consume mode");
	KeyDesc(Actions::Type::ToggleReplayGainMode, "Toggle replay gain mode");
	KeyDesc(Actions::Type::ToggleBitrateVisibility, "Toggle bitrate visibility");
	KeyDesc(Actions::Type::Shuffle, "Shuffle playlist");
	KeyDesc(Actions::Type::ToggleCrossfade, "Toggle crossfade mode");
	KeyDesc(Actions::Type::SetCrossfade, "Set crossfade");
	KeyDesc(Actions::Type::SetVolume, "Set volume");
	KeyDesc(Actions::Type::UpdateDatabase, "Start music database update");
	w << '\n';
	KeyDesc(Actions::Type::ExecuteCommand, "Execute command");
	KeyDesc(Actions::Type::ApplyFilter, "Apply filter");
	KeyDesc(Actions::Type::FindItemForward, "Find item forward");
	KeyDesc(Actions::Type::FindItemBackward, "Find item backward");
	KeyDesc(Actions::Type::PreviousFoundItem, "Jump to previous found item");
	KeyDesc(Actions::Type::NextFoundItem, "Jump to next found item");
	KeyDesc(Actions::Type::ToggleFindMode, "Toggle find mode (normal/wrapped)");
	KeyDesc(Actions::Type::JumpToBrowser, "Locate song in browser");
	KeyDesc(Actions::Type::JumpToMediaLibrary, "Locate song in media library");
	KeyDesc(Actions::Type::ToggleScreenLock, "Lock/unlock current screen");
	KeyDesc(Actions::Type::MasterScreen, "Switch to master screen (left one)");
	KeyDesc(Actions::Type::SlaveScreen, "Switch to slave screen (right one)");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(Actions::Type::JumpToTagEditor, "Locate song in tag editor");
#	endif // HAVE_TAGLIB_H
	KeyDesc(Actions::Type::ToggleDisplayMode, "Toggle display mode");
	KeyDesc(Actions::Type::ToggleInterface, "Toggle user interface");
	KeyDesc(Actions::Type::ToggleSeparatorsBetweenAlbums, "Toggle displaying separators between albums");
	KeyDesc(Actions::Type::JumpToPositionInSong, "Jump to given position in playing song (formats: mm:ss, x%)");
	KeyDesc(Actions::Type::ShowSongInfo, "Show song info");
#	ifdef HAVE_CURL_CURL_H
	KeyDesc(Actions::Type::ShowArtistInfo, "Show artist info");
	KeyDesc(Actions::Type::ToggleLyricsFetcher, "Toggle lyrics fetcher");
	KeyDesc(Actions::Type::ToggleFetchingLyricsInBackground, "Toggle fetching lyrics for playing songs in background");
#	endif // HAVE_CURL_CURL_H
	KeyDesc(Actions::Type::ShowLyrics, "Show/hide song lyrics");
	w << '\n';
	KeyDesc(Actions::Type::Quit, "Quit");
	
	KeysSection("Playlist");
	KeyDesc(Actions::Type::PressEnter, "Play selected item");
	KeyDesc(Actions::Type::DeletePlaylistItems, "Delete selected item(s) from playlist");
	KeyDesc(Actions::Type::ClearMainPlaylist, "Clear playlist");
	KeyDesc(Actions::Type::CropMainPlaylist, "Clear playlist except selected item(s)");
	KeyDesc(Actions::Type::SetSelectedItemsPriority, "Set priority of selected items");
	KeyDesc(Actions::Type::MoveSelectedItemsUp, "Move selected item(s) up");
	KeyDesc(Actions::Type::MoveSelectedItemsDown, "Move selected item(s) down");
	KeyDesc(Actions::Type::MoveSelectedItemsTo, "Move selected item(s) to cursor position");
	KeyDesc(Actions::Type::Add, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(Actions::Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(Actions::Type::SavePlaylist, "Save playlist");
	KeyDesc(Actions::Type::SortPlaylist, "Sort playlist");
	KeyDesc(Actions::Type::ReversePlaylist, "Reverse playlist");
	KeyDesc(Actions::Type::FilterPlaylistOnPriorities, "Filter playlist on priorities");
	KeyDesc(Actions::Type::JumpToPlayingSong, "Jump to playing song");
	KeyDesc(Actions::Type::TogglePlayingSongCentering, "Toggle playing song centering");
	
	KeysSection("Browser");
	KeyDesc(Actions::Type::PressEnter, "Enter directory/Add item to playlist and play it");
	KeyDesc(Actions::Type::PressSpace, "Add item to playlist/select it");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(Actions::Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(Actions::Type::EditDirectoryName, "Edit directory name");
	KeyDesc(Actions::Type::EditPlaylistName, "Edit playlist name");
	KeyDesc(Actions::Type::ChangeBrowseMode, "Browse MPD database/local filesystem");
	KeyDesc(Actions::Type::ToggleBrowserSortMode, "Toggle sort mode");
	KeyDesc(Actions::Type::JumpToPlayingSong, "Locate playing song");
	KeyDesc(Actions::Type::JumpToParentDirectory, "Jump to parent directory");
	KeyDesc(Actions::Type::DeleteBrowserItems, "Delete selected items from disk");
	KeyDesc(Actions::Type::JumpToPlaylistEditor, "Jump to playlist editor (playlists only)");
	
	KeysSection("Search engine");
	KeyDesc(Actions::Type::PressEnter, "Add item to playlist and play it/change option");
	KeyDesc(Actions::Type::PressSpace, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(Actions::Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(Actions::Type::StartSearching, "Start searching");
	KeyDesc(Actions::Type::ResetSearchEngine, "Reset search constraints and clear results");
	
	KeysSection("Media library");
	KeyDesc(Actions::Type::ToggleMediaLibraryColumnsMode, "Switch between two/three columns mode");
	KeyDesc(Actions::Type::PreviousColumn, "Previous column");
	KeyDesc(Actions::Type::NextColumn, "Next column");
	KeyDesc(Actions::Type::PressEnter, "Add item to playlist and play it");
	KeyDesc(Actions::Type::PressSpace, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(Actions::Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(Actions::Type::EditLibraryTag, "Edit tag (left column)/album (middle/right column)");
	KeyDesc(Actions::Type::ToggleLibraryTagType, "Toggle type of tag used in left column");
	KeyDesc(Actions::Type::ToggleMediaLibrarySortMode, "Toggle sort mode");
	
	KeysSection("Playlist editor");
	KeyDesc(Actions::Type::PreviousColumn, "Previous column");
	KeyDesc(Actions::Type::NextColumn, "Next column");
	KeyDesc(Actions::Type::PressEnter, "Add item to playlist and play it");
	KeyDesc(Actions::Type::PressSpace, "Add item to playlist/select it");
#	ifdef HAVE_TAGLIB_H
	KeyDesc(Actions::Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	KeyDesc(Actions::Type::EditPlaylistName, "Edit playlist name");
	KeyDesc(Actions::Type::MoveSelectedItemsUp, "Move selected item(s) up");
	KeyDesc(Actions::Type::MoveSelectedItemsDown, "Move selected item(s) down");
	KeyDesc(Actions::Type::DeleteStoredPlaylist, "Delete selected playlists (left column)");
	KeyDesc(Actions::Type::DeletePlaylistItems, "Delete selected item(s) from playlist (right column)");
	KeyDesc(Actions::Type::ClearPlaylist, "Clear playlist");
	KeyDesc(Actions::Type::CropPlaylist, "Clear playlist except selected items");
	
	KeysSection("Lyrics");
	KeyDesc(Actions::Type::PressSpace, "Toggle reloading lyrics upon song change");
	KeyDesc(Actions::Type::EditLyrics, "Open lyrics in external editor");
	KeyDesc(Actions::Type::RefetchLyrics, "Refetch lyrics");
	
#	ifdef HAVE_TAGLIB_H
	KeysSection("Tiny tag editor");
	KeyDesc(Actions::Type::PressEnter, "Edit tag");
	KeyDesc(Actions::Type::SaveTagChanges, "Save");
	
	KeysSection("Tag editor");
	KeyDesc(Actions::Type::PressEnter, "Edit tag/filename of selected item (left column)");
	KeyDesc(Actions::Type::PressEnter, "Perform operation on all/selected items (middle column)");
	KeyDesc(Actions::Type::PressSpace, "Switch to albums/directories view (left column)");
	KeyDesc(Actions::Type::PressSpace, "Select item (right column)");
	KeyDesc(Actions::Type::PreviousColumn, "Previous column");
	KeyDesc(Actions::Type::NextColumn, "Next column");
	KeyDesc(Actions::Type::JumpToParentDirectory, "Jump to parent directory (left column, directories view)");
#	endif // HAVE_TAGLIB_H
	
#	ifdef ENABLE_OUTPUTS
	KeysSection("Outputs");
	KeyDesc(Actions::Type::PressEnter, "Toggle output");
#	endif // ENABLE_OUTPUTS
	
#	if defined(ENABLE_VISUALIZER) && defined(HAVE_FFTW3_H)
	KeysSection("Music visualizer");
	KeyDesc(Actions::Type::PressSpace, "Toggle visualization type");
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

