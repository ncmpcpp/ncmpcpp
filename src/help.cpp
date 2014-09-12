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

namespace {

std::string key_to_string(const Key &key, bool *print_backspace)
{
	std::string result;
	if (key == Key(KEY_UP, Key::NCurses))
		result += "Up";
	else if (key == Key(KEY_DOWN, Key::NCurses))
		result += "Down";
	else if (key == Key(KEY_PPAGE, Key::NCurses))
		result += "Page Up";
	else if (key == Key(KEY_NPAGE, Key::NCurses))
		result += "Page Down";
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

std::string display_keys(const Actions::Type at)
{
	bool print_backspace = true;
	std::string result, skey;
	for (auto it = Bindings.begin(); it != Bindings.end(); ++it)
	{
		for (auto j = it->second.begin(); j != it->second.end(); ++j)
		{
			if (j->isSingle() && j->action()->type() == at)
			{
				skey = key_to_string(it->first, &print_backspace);
				if (!skey.empty())
				{
					result += std::move(skey);
					result += " ";
				}
			}
		}
	}
	result.resize(16, ' ');
	return result;
}

void section(NC::Scrollpad &w, const char *type_, const char *title_)
{
	w << "\n  " << NC::Format::Bold << type_ << " - ";
	w << title_ << NC::Format::NoBold << "\n\n";
}

/**********************************************************************/

void key_section(NC::Scrollpad &w, const char *title_)
{
	section(w, "Keys", title_);
}

void key(NC::Scrollpad &w, const Actions::Type at, const char *desc)
{
	w << "    " << display_keys(at) << " : " << desc << '\n';
}

void key(NC::Scrollpad &w, const Actions::Type at, const boost::format &desc)
{
	w << "    " << display_keys(at) << " : " << desc.str() << '\n';
}

/**********************************************************************/

void mouse_section(NC::Scrollpad &w, const char *title_)
{
	section(w, "Mouse", title_);
}

void mouse(NC::Scrollpad &w, std::string action, const char *desc, bool indent = false)
{
	action.resize(31 - (indent ? 2 : 0), ' ');
	w << "    " << (indent ? "  " : "") << action;
	w << ": " << desc << '\n';
}

void mouse_column(NC::Scrollpad &w, const char *column)
{
	w << NC::Format::Bold << "    " << column << " column:\n" << NC::Format::NoBold;
}

/**********************************************************************/

void write_bindings(NC::Scrollpad &w)
{
	using Actions::Type;

	key_section(w, "Movement");
	key(w, Type::ScrollUp, "Move cursor up");
	key(w, Type::ScrollDown, "Move cursor down");
	key(w, Type::ScrollUpAlbum, "Move cursor up one album");
	key(w, Type::ScrollDownAlbum, "Move cursor down one album");
	key(w, Type::ScrollUpArtist, "Move cursor up one artist");
	key(w, Type::ScrollDownArtist, "Move cursor down one artist");
	key(w, Type::PageUp, "Page up");
	key(w, Type::PageDown, "Page down");
	key(w, Type::MoveHome, "Home");
	key(w, Type::MoveEnd, "End");
	w << '\n';
	if (Config.screen_switcher_previous)
	{
		key(w, Type::NextScreen, "Switch between current and last screen");
		key(w, Type::PreviousScreen, "Switch between current and last screen");
	}
	else
	{
		key(w, Type::NextScreen, "Switch to next screen in sequence");
		key(w, Type::PreviousScreen, "Switch to previous screen in sequence");
	}
	key(w, Type::ShowHelp, "Show help");
	key(w, Type::ShowPlaylist, "Show playlist");
	key(w, Type::ShowBrowser, "Show browser");
	key(w, Type::ShowSearchEngine, "Show search engine");
	key(w, Type::ShowMediaLibrary, "Show media library");
	key(w, Type::ShowPlaylistEditor, "Show playlist editor");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::ShowTagEditor, "Show tag editor");
#	endif // HAVE_TAGLIB_H
#	ifdef ENABLE_OUTPUTS
	key(w, Type::ShowOutputs, "Show outputs");
#	endif // ENABLE_OUTPUTS
#	ifdef ENABLE_VISUALIZER
	key(w, Type::ShowVisualizer, "Show music visualizer");
#	endif // ENABLE_VISUALIZER
#	ifdef ENABLE_CLOCK
	key(w, Type::ShowClock, "Show clock");
#	endif // ENABLE_CLOCK
	w << '\n';
	key(w, Type::ShowServerInfo, "Show server info");

	key_section(w, "Global");
	key(w, Type::Stop, "Stop");
	key(w, Type::Pause, "Pause");
	key(w, Type::Next, "Next track");
	key(w, Type::Previous, "Previous track");
	key(w, Type::ReplaySong, "Replay playing song");
	key(w, Type::SeekForward, "Seek forward in playing song");
	key(w, Type::SeekBackward, "Seek backward in playing song");
	key(w, Type::VolumeDown,
		boost::format("Decrease volume by %1%%%") % Config.volume_change_step
	);
	key(w, Type::VolumeUp,
		boost::format("Increase volume by %1%%%") % Config.volume_change_step
	);
	w << '\n';
	key(w, Type::ToggleSpaceMode, "Toggle space mode (select/add)");
	key(w, Type::ToggleAddMode, "Toggle add mode (add or remove/always add)");
	key(w, Type::ToggleMouse, "Toggle mouse support");
	key(w, Type::ReverseSelection, "Reverse selection");
	key(w, Type::RemoveSelection, "Remove selection");
	key(w, Type::SelectAlbum, "Select songs of album around the cursor");
	key(w, Type::AddSelectedItems, "Add selected items to playlist");
	key(w, Type::AddRandomItems, "Add random items to playlist");
	w << '\n';
	key(w, Type::ToggleRepeat, "Toggle repeat mode");
	key(w, Type::ToggleRandom, "Toggle random mode");
	key(w, Type::ToggleSingle, "Toggle single mode");
	key(w, Type::ToggleConsume, "Toggle consume mode");
	key(w, Type::ToggleReplayGainMode, "Toggle replay gain mode");
	key(w, Type::ToggleBitrateVisibility, "Toggle bitrate visibility");
	key(w, Type::Shuffle, "Shuffle playlist");
	key(w, Type::ToggleCrossfade, "Toggle crossfade mode");
	key(w, Type::SetCrossfade, "Set crossfade");
	key(w, Type::SetVolume, "Set volume");
	key(w, Type::UpdateDatabase, "Start music database update");
	w << '\n';
	key(w, Type::ExecuteCommand, "Execute command");
	key(w, Type::ApplyFilter, "Apply filter");
	key(w, Type::FindItemForward, "Find item forward");
	key(w, Type::FindItemBackward, "Find item backward");
	key(w, Type::PreviousFoundItem, "Jump to previous found item");
	key(w, Type::NextFoundItem, "Jump to next found item");
	key(w, Type::ToggleFindMode, "Toggle find mode (normal/wrapped)");
	key(w, Type::JumpToBrowser, "Locate song in browser");
	key(w, Type::JumpToMediaLibrary, "Locate song in media library");
	key(w, Type::ToggleScreenLock, "Lock/unlock current screen");
	key(w, Type::MasterScreen, "Switch to master screen (left one)");
	key(w, Type::SlaveScreen, "Switch to slave screen (right one)");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::JumpToTagEditor, "Locate song in tag editor");
#	endif // HAVE_TAGLIB_H
	key(w, Type::ToggleDisplayMode, "Toggle display mode");
	key(w, Type::ToggleInterface, "Toggle user interface");
	key(w, Type::ToggleSeparatorsBetweenAlbums, "Toggle displaying separators between albums");
	key(w, Type::JumpToPositionInSong, "Jump to given position in playing song (formats: mm:ss, x%)");
	key(w, Type::ShowSongInfo, "Show song info");
#	ifdef HAVE_CURL_CURL_H
	key(w, Type::ShowArtistInfo, "Show artist info");
	key(w, Type::ToggleLyricsFetcher, "Toggle lyrics fetcher");
	key(w, Type::ToggleFetchingLyricsInBackground, "Toggle fetching lyrics for playing songs in background");
#	endif // HAVE_CURL_CURL_H
	key(w, Type::ShowLyrics, "Show/hide song lyrics");
	w << '\n';
	key(w, Type::Quit, "Quit");

	key_section(w, "Playlist");
	key(w, Type::PressEnter, "Play selected item");
	key(w, Type::DeletePlaylistItems, "Delete selected item(s) from playlist");
	key(w, Type::ClearMainPlaylist, "Clear playlist");
	key(w, Type::CropMainPlaylist, "Clear playlist except selected item(s)");
	key(w, Type::SetSelectedItemsPriority, "Set priority of selected items");
	key(w, Type::MoveSelectedItemsUp, "Move selected item(s) up");
	key(w, Type::MoveSelectedItemsDown, "Move selected item(s) down");
	key(w, Type::MoveSelectedItemsTo, "Move selected item(s) to cursor position");
	key(w, Type::Add, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::SavePlaylist, "Save playlist");
	key(w, Type::SortPlaylist, "Sort playlist");
	key(w, Type::ReversePlaylist, "Reverse playlist");
	key(w, Type::FilterPlaylistOnPriorities, "Filter playlist on priorities");
	key(w, Type::JumpToPlayingSong, "Jump to current song");
	key(w, Type::TogglePlayingSongCentering, "Toggle playing song centering");

	key_section(w, "Browser");
	key(w, Type::PressEnter, "Enter directory/Add item to playlist and play it");
	key(w, Type::PressSpace, "Add item to playlist/select it");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::EditDirectoryName, "Edit directory name");
	key(w, Type::EditPlaylistName, "Edit playlist name");
	key(w, Type::ChangeBrowseMode, "Browse MPD database/local filesystem");
	key(w, Type::ToggleBrowserSortMode, "Toggle sort mode");
	key(w, Type::JumpToPlayingSong, "Locate playing song");
	key(w, Type::JumpToParentDirectory, "Jump to parent directory");
	key(w, Type::DeleteBrowserItems, "Delete selected items from disk");
	key(w, Type::JumpToPlaylistEditor, "Jump to playlist editor (playlists only)");

	key_section(w, "Search engine");
	key(w, Type::PressEnter, "Add item to playlist and play it/change option");
	key(w, Type::PressSpace, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::StartSearching, "Start searching");
	key(w, Type::ResetSearchEngine, "Reset search constraints and clear results");

	key_section(w, "Media library");
	key(w, Type::ToggleMediaLibraryColumnsMode, "Switch between two/three columns mode");
	key(w, Type::PreviousColumn, "Previous column");
	key(w, Type::NextColumn, "Next column");
	key(w, Type::PressEnter, "Add item to playlist and play it");
	key(w, Type::PressSpace, "Add item to playlist");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::EditLibraryTag, "Edit tag (left column)/album (middle/right column)");
	key(w, Type::ToggleLibraryTagType, "Toggle type of tag used in left column");
	key(w, Type::ToggleMediaLibrarySortMode, "Toggle sort mode");

	key_section(w, "Playlist editor");
	key(w, Type::PreviousColumn, "Previous column");
	key(w, Type::NextColumn, "Next column");
	key(w, Type::PressEnter, "Add item to playlist and play it");
	key(w, Type::PressSpace, "Add item to playlist/select it");
#	ifdef HAVE_TAGLIB_H
	key(w, Type::EditSong, "Edit song");
#	endif // HAVE_TAGLIB_H
	key(w, Type::EditPlaylistName, "Edit playlist name");
	key(w, Type::MoveSelectedItemsUp, "Move selected item(s) up");
	key(w, Type::MoveSelectedItemsDown, "Move selected item(s) down");
	key(w, Type::DeleteStoredPlaylist, "Delete selected playlists (left column)");
	key(w, Type::DeletePlaylistItems, "Delete selected item(s) from playlist (right column)");
	key(w, Type::ClearPlaylist, "Clear playlist");
	key(w, Type::CropPlaylist, "Clear playlist except selected items");

	key_section(w, "Lyrics");
	key(w, Type::PressSpace, "Toggle reloading lyrics upon song change");
	key(w, Type::EditLyrics, "Open lyrics in external editor");
	key(w, Type::RefetchLyrics, "Refetch lyrics");

#	ifdef HAVE_TAGLIB_H
	key_section(w, "Tiny tag editor");
	key(w, Type::PressEnter, "Edit tag");
	key(w, Type::SaveTagChanges, "Save");

	key_section(w, "Tag editor");
	key(w, Type::PressEnter, "Edit tag/filename of selected item (left column)");
	key(w, Type::PressEnter, "Perform operation on all/selected items (middle column)");
	key(w, Type::PressSpace, "Switch to albums/directories view (left column)");
	key(w, Type::PressSpace, "Select item (right column)");
	key(w, Type::PreviousColumn, "Previous column");
	key(w, Type::NextColumn, "Next column");
	key(w, Type::JumpToParentDirectory, "Jump to parent directory (left column, directories view)");
#	endif // HAVE_TAGLIB_H

#	ifdef ENABLE_OUTPUTS
	key_section(w, "Outputs");
	key(w, Type::PressEnter, "Toggle output");
#	endif // ENABLE_OUTPUTS

#	if defined(ENABLE_VISUALIZER) && defined(HAVE_FFTW3_H)
	key_section(w, "Music visualizer");
	key(w, Type::PressSpace, "Toggle visualization type");
	key(w, Type::SetVisualizerSampleMultiplier, "Set visualizer sample multiplier");
#	endif // ENABLE_VISUALIZER && HAVE_FFTW3_H

	mouse_section(w, "Global");
	mouse(w, "Left click on \"Playing/Paused\"", "Play/pause");
	mouse(w, "Left click on progressbar", "Jump to pointed position in playing song");
	w << '\n';
	mouse(w, "Mouse wheel on \"Volume: xx\"", "Play/pause");
	mouse(w, "Mouse wheel on main window", "Scroll");

	mouse_section(w, "Playlist");
	mouse(w, "Left click", "Select pointed item");
	mouse(w, "Right click", "Play");

	mouse_section(w, "Browser");
	mouse(w, "Left click on directory", "Enter pointed directory");
	mouse(w, "Right click on directory", "Add pointed directory to playlist");
	w << '\n';
	mouse(w, "Left click on song/playlist", "Add pointed item to playlist");
	mouse(w, "Right click on song/playlist", "Add pointed item to playlist and play it");

	mouse_section(w, "Search engine");
	mouse(w, "Left click", "Highlight/switch value");
	mouse(w, "Right click", "Change value");

	mouse_section(w, "Media library");
	mouse_column(w, "Left/middle");
	mouse(w, "Left click", "Select pointed item", true);
	mouse(w, "Right click", "Add item to playlist", true);
	w << '\n';
	mouse_column(w, "Right");
	mouse(w, "Left Click", "Add pointed item to playlist", true);
	mouse(w, "Right Click", "Add pointed item to playlist and play it", true);

	mouse_section(w, "Playlist editor");
	mouse_column(w, "Left");
	mouse(w, "Left click", "Select pointed item", true);
	mouse(w, "Right click", "Add item to playlist", true);
	w << '\n';
	mouse_column(w, "Right");
	mouse(w, "Left click", "Add pointed item to playlist", true);
	mouse(w, "Right click", "Add pointed item to playlist and play it", true);

#	ifdef HAVE_TAGLIB_H
	mouse_section(w, "Tiny tag editor");
	mouse(w, "Left click", "Select option");
	mouse(w, "Right click", "Set value/execute");

	mouse_section(w, "Tag editor");
	mouse_column(w, "Left");
	mouse(w, "Left click", "Enter pointed directory/select pointed album", true);
	mouse(w, "Right click", "Toggle view (directories/albums)", true);
	w << '\n';
	mouse_column(w, "Middle");
	mouse(w, "Left click", "Select option", true);
	mouse(w, "Right click", "Set value/execute", true);
	w << '\n';
	mouse_column(w, "Right");
	mouse(w, "Left click", "Select pointed item", true);
	mouse(w, "Right click", "Set value", true);
#	endif // HAVE_TAGLIB_H

#	ifdef ENABLE_OUTPUTS
	mouse_section(w, "Outputs");
	mouse(w, "Left click", "Select pointed output");
	mouse(w, "Right click", "Toggle output");
#	endif // ENABLE_OUTPUTS

}

}

Help::Help()
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border::None))
{
	write_bindings(w);
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
