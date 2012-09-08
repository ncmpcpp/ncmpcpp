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

#include <fstream>
#include <iostream>
#include "global.h"
#include "bindings.h"
#include "utility/string.h"
#include "utility/wide_string.h"

BindingsConfiguration Bindings;

Key Key::noOp = Key(ERR, NCurses);

namespace {//

Key stringToSpecialKey(const std::string &s)
{
	Key result = Key::noOp;
	if (!s.compare("mouse"))
		result = Key(KEY_MOUSE, Key::NCurses);
	else if (!s.compare("up"))
		result = Key(KEY_UP, Key::NCurses);
	else if (!s.compare("down"))
		result = Key(KEY_DOWN, Key::NCurses);
	else if (!s.compare("page_up"))
		result = Key(KEY_PPAGE, Key::NCurses);
	else if (!s.compare("page_down"))
		result = Key(KEY_NPAGE, Key::NCurses);
	else if (!s.compare("home"))
		result = Key(KEY_HOME, Key::NCurses);
	else if (!s.compare("end"))
		result = Key(KEY_END, Key::NCurses);
	else if (!s.compare("space"))
		result = Key(KEY_SPACE, Key::Standard);
	else if (!s.compare("enter"))
		result = Key(KEY_ENTER, Key::Standard);
	else if (!s.compare("insert"))
		result = Key(KEY_IC, Key::NCurses);
	else if (!s.compare("delete"))
		result = Key(KEY_DC, Key::NCurses);
	else if (!s.compare("left"))
		result = Key(KEY_LEFT, Key::NCurses);
	else if (!s.compare("right"))
		result = Key(KEY_RIGHT, Key::NCurses);
	else if (!s.compare("tab"))
		result = Key(KEY_TAB, Key::Standard);
	else if (!s.compare("shift_tab"))
		result = Key(KEY_SHIFT_TAB, Key::NCurses);
	else if (!s.compare(0, 5, "ctrl_") && s.length() > 5 && s[5] >= 'a' && s[5] <= 'z')
		result = Key(KEY_CTRL_A + (s[5] - 'a'), Key::Standard);
	else if (s.length() > 1 && s[0] == 'f')
	{
		int n = atoi(s.c_str() + 1);
		if (n >= 1 && n <= 12)
			result = Key(KEY_F1 + n - 1, Key::NCurses);
	}
	else if (!s.compare("backspace"))
		result = Key(KEY_BACKSPACE, Key::NCurses);
	else if (!s.compare("backspace_2"))
		result = Key(KEY_BACKSPACE_2, Key::Standard);
	return result;
}

Key stringToKey(const std::string &s)
{
	Key result = stringToSpecialKey(s);
	if (result == Key::noOp)
	{
		std::wstring ws = ToWString(s);
		if (ws.length() == 1)
			result = Key(ws[0], Key::Standard);
	}
	return result;
}

template <typename F>
Action *parseActionLine(const std::string &line, F error)
{
	Action *result = 0;
	size_t i = 0;
	for (; i < line.size() && !isspace(line[i]); ++i) { }
	if (i == line.size()) // only action name
		result = Action::Get(line);
	else // there is something else
	{
		std::string action_name = line.substr(0, i);
		if (action_name == "push_character")
		{
			// push single character into input queue
			std::string arg = getEnclosedString(line, '"', '"', 0);
			Key k = stringToSpecialKey(arg);
			if (k != Key::noOp)
				result = new PushCharacters(&Global::wFooter, { k.getChar() });
			else
				error() << "invalid character passed to push_character: '" << arg << "'\n";
		}
		else if (action_name == "push_characters")
		{
			// push sequence of characters into input queue
			std::string arg = getEnclosedString(line, '"', '"', 0);
			if (!arg.empty())
			{
				std::vector<int> queue(arg.begin(), arg.end());
				// if char is signed, cancel 1s from char -> int conversion
				for (auto it = arg.begin(); it != arg.end(); ++it)
					*it &= 0xff;
				result = new PushCharacters(&Global::wFooter, std::move(queue));
			}
			else
				error() << "empty argument passed to push_characters\n";
		}
		else if (action_name == "require_runnable")
		{
			// require that given action is runnable
			std::string arg = getEnclosedString(line, '"', '"', 0);
			Action *action = Action::Get(arg);
			if (action)
				result = new RequireRunnable(action);
			else
				error() << "unknown action passed to require_runnable: '" << arg << "'\n";
		}
	}
	return result;
}

}

Key Key::read(NC::Window &w)
{
	Key result = noOp;
	std::string tmp;
	int input;
	while (true)
	{
		input = w.readKey();
		if (input == ERR)
			break;
		if (input > 255)
		{
			result = Key(input, NCurses);
			break;
		}
		else
		{
			wchar_t wc;
			tmp += input;
			size_t conv_res = mbrtowc(&wc, tmp.c_str(), MB_CUR_MAX, 0);
			if (conv_res == size_t(-1)) // incomplete multibyte character
				continue;
			else if (conv_res == size_t(-2)) // garbage character sequence
				break;
			else // character complete
			{
				result = Key(wc, Standard);
				break;
			}
		}
	}
	return result;
}

bool BindingsConfiguration::read(const std::string &file)
{
	bool result = true;
	
	std::ifstream f(file);
	if (!f.is_open())
		return result;
	
	size_t line_no = 0;
	bool key_def_in_progress = false;
	Key key = Key::noOp;
	Binding::ActionChain actions;
	std::string line, strkey;
	
	auto error = [&]() -> std::ostream & {
		std::cerr << file << ":" << line_no << ": ";
		key_def_in_progress = false;
		result = false;
		return std::cerr;
	};
	
	auto bind_key_def = [&]() -> bool {
		if (!actions.empty())
		{
			bind(key, actions);
			actions.clear();
			return true;
		}
		else
		{
			error() << "definition of key '" << strkey << "' cannot be empty.\n";
			return false;
		}
	};
	
	while (!f.eof() && ++line_no)
	{
		getline(f, line);
		if (line.empty() || line[0] == '#')
			continue;
		
		if (!line.compare(0, 7, "def_key")) // beginning of key definition
		{
			if (key_def_in_progress)
			{
				if (!bind_key_def())
					break;
			}
			key_def_in_progress = true;
			strkey = getEnclosedString(line, '"', '"', 0);
			key = stringToKey(strkey);
			if (key == Key::noOp)
			{
				error() << "invalid key: '" << strkey << "'\n";
				break;
			}
		}
		else if (isspace(line[0])) // name of action to be bound
		{
			trim(line);
			Action *action = parseActionLine(line, error);
			if (action)
				actions.push_back(action);
			else
			{
				error() << "unknown action: '" << line << "'\n";
				break;
			}
		}
	}
	if (key_def_in_progress)
		bind_key_def();
	f.close();
	return result;
}

void BindingsConfiguration::generateDefaults()
{
	Key k = Key::noOp;
	if (notBound(k = stringToKey("mouse")))
		bind(k, aMouseEvent);
	if (notBound(k = stringToKey("up")))
		bind(k, aScrollUp);
	if (notBound(k = stringToKey("down")))
		bind(k, aScrollDown);
	if (notBound(k = stringToKey("[")))
		bind(k, aScrollUpAlbum);
	if (notBound(k = stringToKey("]")))
		bind(k, aScrollDownAlbum);
	if (notBound(k = stringToKey("{")))
		bind(k, aScrollUpArtist);
	if (notBound(k = stringToKey("}")))
		bind(k, aScrollDownArtist);
	if (notBound(k = stringToKey("page_up")))
		bind(k, aPageUp);
	if (notBound(k = stringToKey("page_down")))
		bind(k, aPageDown);
	if (notBound(k = stringToKey("home")))
		bind(k, aMoveHome);
	if (notBound(k = stringToKey("end")))
		bind(k, aMoveEnd);
	if (notBound(k = stringToKey("space")))
		bind(k, aPressSpace);
	if (notBound(k = stringToKey("enter")))
		bind(k, aPressEnter);
	if (notBound(k = stringToKey("delete")))
		bind(k, aDelete);
	if (notBound(k = stringToKey("right")))
	{
		bind(k, aNextColumn);
		bind(k, aSlaveScreen);
		bind(k, aVolumeUp);
	}
	if (notBound(k = stringToKey("+")))
		bind(k, aVolumeUp);
	if (notBound(k = stringToKey("left")))
	{
		bind(k, aPreviousColumn);
		bind(k, aMasterScreen);
		bind(k, aVolumeDown);
	}
	if (notBound(k = stringToKey("-")))
		bind(k, aVolumeDown);
	if (notBound(k = stringToKey("tab")))
		bind(k, aNextScreen);
	if (notBound(k = stringToKey("shift_tab")))
		bind(k, aPreviousScreen);
	if (notBound(k = stringToKey("f1")))
		bind(k, aShowHelp);
	if (notBound(k = stringToKey("1")))
		bind(k, aShowPlaylist);
	if (notBound(k = stringToKey("2")))
		bind(k, aShowBrowser);
	if (notBound(k = stringToKey("3")))
		bind(k, aShowSearchEngine);
	if (notBound(k = stringToKey("4")))
		bind(k, aShowMediaLibrary);
	if (notBound(k = stringToKey("5")))
		bind(k, aShowPlaylistEditor);
	if (notBound(k = stringToKey("6")))
		bind(k, aShowTagEditor);
	if (notBound(k = stringToKey("7")))
		bind(k, aShowOutputs);
	if (notBound(k = stringToKey("8")))
		bind(k, aShowVisualizer);
	if (notBound(k = stringToKey("=")))
		bind(k, aShowClock);
	if (notBound(k = stringToKey("@")))
		bind(k, aShowServerInfo);
	if (notBound(k = stringToKey("s")))
		bind(k, aStop);
	if (notBound(k = stringToKey("p")))
		bind(k, aPause);
	if (notBound(k = stringToKey(">")))
		bind(k, aNext);
	if (notBound(k = stringToKey("<")))
		bind(k, aPrevious);
	if (notBound(k = stringToKey("ctrl_h")))
	{
		bind(k, aJumpToParentDirectory);
		bind(k, aReplaySong);
	}
	if (notBound(k = stringToKey("backspace")))
	{
		bind(k, aJumpToParentDirectory);
		bind(k, aReplaySong);
	}
	if (notBound(k = stringToKey("backspace_2")))
	{
		bind(k, aJumpToParentDirectory);
		bind(k, aReplaySong);
	}
	if (notBound(k = stringToKey("f")))
		bind(k, aSeekForward);
	if (notBound(k = stringToKey("b")))
		bind(k, aSeekBackward);
	if (notBound(k = stringToKey("r")))
		bind(k, aToggleRepeat);
	if (notBound(k = stringToKey("z")))
		bind(k, aToggleRandom);
	if (notBound(k = stringToKey("y")))
	{
		bind(k, aSaveTagChanges);
		bind(k, aStartSearching);
		bind(k, aToggleSingle);
	}
	if (notBound(k = stringToKey("R")))
		bind(k, aToggleConsume);
	if (notBound(k = stringToKey("Y")))
		bind(k, aToggleReplayGainMode);
	if (notBound(k = stringToKey("t")))
		bind(k, aToggleSpaceMode);
	if (notBound(k = stringToKey("T")))
		bind(k, aToggleAddMode);
	if (notBound(k = stringToKey("|")))
		bind(k, aToggleMouse);
	if (notBound(k = stringToKey("#")))
		bind(k, aToggleBitrateVisibility);
	if (notBound(k = stringToKey("Z")))
		bind(k, aShuffle);
	if (notBound(k = stringToKey("x")))
		bind(k, aToggleCrossfade);
	if (notBound(k = stringToKey("X")))
		bind(k, aSetCrossfade);
	if (notBound(k = stringToKey("u")))
		bind(k, aUpdateDatabase);
	if (notBound(k = stringToKey("ctrl_v")))
		bind(k, aSortPlaylist);
	if (notBound(k = stringToKey("ctrl_r")))
		bind(k, aReversePlaylist);
	if (notBound(k = stringToKey("ctrl_f")))
		bind(k, aApplyFilter);
	if (notBound(k = stringToKey("/")))
	{
		bind(k, aFind);
		bind(k, aFindItemForward);
	}
	if (notBound(k = stringToKey("?")))
	{
		bind(k, aFind);
		bind(k, aFindItemBackward);
	}
	if (notBound(k = stringToKey(".")))
		bind(k, aNextFoundItem);
	if (notBound(k = stringToKey(",")))
		bind(k, aPreviousFoundItem);
	if (notBound(k = stringToKey("w")))
		bind(k, aToggleFindMode);
	if (notBound(k = stringToKey("e")))
	{
		bind(k, aEditSong);
		bind(k, aEditLibraryTag);
		bind(k, aEditLibraryAlbum);
		bind(k, aEditDirectoryName);
		bind(k, aEditPlaylistName);
		bind(k, aEditLyrics);
	}
	if (notBound(k = stringToKey("i")))
		bind(k, aShowSongInfo);
	if (notBound(k = stringToKey("I")))
		bind(k, aShowArtistInfo);
	if (notBound(k = stringToKey("g")))
		bind(k, aJumpToPositionInSong);
	if (notBound(k = stringToKey("l")))
		bind(k, aShowLyrics);
	if (notBound(k = stringToKey("v")))
		bind(k, aReverseSelection);
	if (notBound(k = stringToKey("V")))
		bind(k, aRemoveSelection);
	if (notBound(k = stringToKey("B")))
		bind(k, aSelectAlbum);
	if (notBound(k = stringToKey("a")))
		bind(k, aAddSelectedItems);
	if (notBound(k = stringToKey("c")))
	{
		bind(k, aClearPlaylist);
		bind(k, aClearMainPlaylist);
	}
	if (notBound(k = stringToKey("C")))
	{
		bind(k, aCropPlaylist);
		bind(k, aCropMainPlaylist);
	}
	if (notBound(k = stringToKey("m")))
	{
		bind(k, aMoveSortOrderUp);
		bind(k, aMoveSelectedItemsUp);
	}
	if (notBound(k = stringToKey("n")))
	{
		bind(k, aMoveSortOrderDown);
		bind(k, aMoveSelectedItemsDown);
	}
	if (notBound(k = stringToKey("M")))
		bind(k, aMoveSelectedItemsTo);
	if (notBound(k = stringToKey("A")))
		bind(k, aAdd);
	if (notBound(k = stringToKey("S")))
		bind(k, aSavePlaylist);
	if (notBound(k = stringToKey("o")))
		bind(k, aJumpToPlayingSong);
	if (notBound(k = stringToKey("G")))
	{
		bind(k, aJumpToBrowser);
		bind(k, aJumpToPlaylistEditor);
	}
	if (notBound(k = stringToKey("~")))
		bind(k, aJumpToMediaLibrary);
	if (notBound(k = stringToKey("E")))
		bind(k, aJumpToTagEditor);
	if (notBound(k = stringToKey("U")))
		bind(k, aTogglePlayingSongCentering);
	if (notBound(k = stringToKey("P")))
		bind(k, aToggleDisplayMode);
	if (notBound(k = stringToKey("\\")))
		bind(k, aToggleInterface);
	if (notBound(k = stringToKey("!")))
		bind(k, aToggleSeparatorsBetweenAlbums);
	if (notBound(k = stringToKey("L")))
		bind(k, aToggleLyricsFetcher);
	if (notBound(k = stringToKey("F")))
		bind(k, aToggleFetchingLyricsInBackground);
	if (notBound(k = stringToKey("ctrl_l")))
		bind(k, aToggleScreenLock);
	if (notBound(k = stringToKey("`")))
	{
		bind(k, aToggleBrowserSortMode);
		bind(k, aToggleLibraryTagType);
		bind(k, aRefetchLyrics);
		bind(k, aRefetchArtistInfo);
		bind(k, aAddRandomItems);
	}
	if (notBound(k = stringToKey("ctrl_p")))
		bind(k, aSetSelectedItemsPriority);
	if (notBound(k = stringToKey("q")))
		bind(k, aQuit);
	
	if (notBound(k = stringToKey("-")))
		bind(k, aVolumeDown);
}
