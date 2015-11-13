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

#include <boost/algorithm/string/trim.hpp>
#include <fstream>
#include <iostream>
#include "global.h"
#include "bindings.h"
#include "utility/string.h"
#include "utility/wide_string.h"

BindingsConfiguration Bindings;

namespace {

void warning(const char *msg)
{
	std::cerr << "WARNING: " << msg << "\n";
}

NC::Key::Type stringToKey(const std::string &s);

NC::Key::Type stringToSpecialKey(const std::string &s)
{
	NC::Key::Type result = NC::Key::None;
	if (!s.compare(0, 4, "ctrl") && s.length() == 6 && (s[4] == '_' || s[4] == '-'))
	{
		if (s[4] == '_')
			warning("prefix 'ctrl_' is deprecated and will be removed in 0.8, use 'ctrl-' instead.");
		if (s[5] >= 'a' && s[5] <= 'z')
			result = NC::Key::Ctrl_A + (s[5] - 'a');
		else if (s[5] == '[')
			result = NC::Key::Ctrl_LeftBracket;
		else if (s[5] == '\\')
			result = NC::Key::Ctrl_Backslash;
		else if (s[5] == ']')
			result = NC::Key::Ctrl_RightBracket;
		else if (s[5] == '^')
			result = NC::Key::Ctrl_Caret;
		else if (s[5] == '_')
			result = NC::Key::Ctrl_Underscore;
	}
	else if (!s.compare(0, 3, "alt") && s.length() > 3 && (s[3] == '_' || s[3] == '-'))
	{
		if (s[3] == '_')
			warning("prefix 'alt_' is deprecated and will be removed in 0.8, use 'alt-' instead.");
		result = NC::Key::Alt | stringToKey(s.substr(4));
	}
	else if (!s.compare(0, 4, "ctrl") && s.length() > 4 && (s[4] == '_' || s[4] == '-'))
	{
		if (s[4] == '_')
			warning("prefix 'ctrl_' is deprecated and will be removed in 0.8, use 'ctrl-' instead.");
		result = NC::Key::Ctrl | stringToKey(s.substr(5));
	}
	else if (!s.compare(0, 5, "shift") && s.length() > 5 && (s[5] == '_' || s[5] == '-'))
	{
		if (s[5] == '_')
			warning("prefix 'shift_' is deprecated and will be removed in 0.8, use 'shift-' instead.");
		result = NC::Key::Shift | stringToKey(s.substr(6));
	}
	else if (!s.compare("escape"))
		result = NC::Key::Escape;
	else if (!s.compare("mouse"))
		result = NC::Key::Mouse;
	else if (!s.compare("up"))
		result = NC::Key::Up;
	else if (!s.compare("down"))
		result = NC::Key::Down;
	else if (!s.compare("page_up"))
		result = NC::Key::PageUp;
	else if (!s.compare("page_down"))
		result = NC::Key::PageDown;
	else if (!s.compare("home"))
		result = NC::Key::Home;
	else if (!s.compare("end"))
		result = NC::Key::End;
	else if (!s.compare("space"))
		result = NC::Key::Space;
	else if (!s.compare("enter"))
		result = NC::Key::Enter;
	else if (!s.compare("insert"))
		result = NC::Key::Insert;
	else if (!s.compare("delete"))
		result = NC::Key::Delete;
	else if (!s.compare("left"))
		result = NC::Key::Left;
	else if (!s.compare("right"))
		result = NC::Key::Right;
	else if (!s.compare("tab"))
		result = NC::Key::Tab;
	else if ((s.length() == 2 || s.length() == 3) && s[0] == 'f')
	{
		int n = atoi(s.c_str() + 1);
		if (n >= 1 && n <= 12)
			result = NC::Key::F1 + n - 1;
	}
	else if (!s.compare("backspace"))
		result = NC::Key::Backspace;
	else if (!s.compare("backspace_2"))
	{
		warning("'backspace_2' is deprecated and will be removed in 0.8, use 'backspace' instead.");
		result = NC::Key::Backspace;
	}
	return result;
}

NC::Key::Type stringToKey(const std::string &s)
{
	NC::Key::Type result = stringToSpecialKey(s);
	if (result == NC::Key::None)
	{
		std::wstring ws = ToWString(s);
		if (ws.length() == 1)
			result = ws[0];
	}
	return result;
}

template <typename F>
Actions::BaseAction *parseActionLine(const std::string &line, F error)
{
	Actions::BaseAction *result = 0;
	size_t i = 0;
	for (; i < line.size() && !isspace(line[i]); ++i) { }
	if (i == line.size()) // only action name
		result = Actions::get(line);
	else // there is something else
	{
		std::string action_name = line.substr(0, i);
		if (action_name == "push_character")
		{
			// push single character into input queue
			std::string arg = getEnclosedString(line, '"', '"', 0);
			NC::Key::Type k = stringToSpecialKey(arg);
			auto queue = std::vector<NC::Key::Type>{ k };
			if (k != NC::Key::None)
				result = new Actions::PushCharacters(&Global::wFooter, std::move(queue));
			else
				error() << "invalid character passed to push_character: '" << arg << "'\n";
		}
		else if (action_name == "push_characters")
		{
			// push sequence of characters into input queue
			std::string arg = getEnclosedString(line, '"', '"', 0);
			if (!arg.empty())
			{
				std::vector<NC::Key::Type> queue(arg.begin(), arg.end());
				// if char is signed, erase 1s from char -> int conversion
				for (auto it = arg.begin(); it != arg.end(); ++it)
					*it &= 0xff;
				result = new Actions::PushCharacters(&Global::wFooter, std::move(queue));
			}
			else
				error() << "empty argument passed to push_characters\n";
		}
		else if (action_name == "require_screen")
		{
			// require screen of given type
			std::string arg = getEnclosedString(line, '"', '"', 0);
			ScreenType screen_type = stringToScreenType(arg);
			if (screen_type != ScreenType::Unknown)
				result = new Actions::RequireScreen(screen_type);
			else
				error() << "unknown screen passed to require_screen: '" << arg << "'\n";
		}
		else if (action_name == "require_runnable")
		{
			// require that given action is runnable
			std::string arg = getEnclosedString(line, '"', '"', 0);
			auto action = Actions::get(arg);
			if (action)
				result = new Actions::RequireRunnable(action);
			else
				error() << "unknown action passed to require_runnable: '" << arg << "'\n";
		}
		else if (action_name == "run_external_command")
		{
			std::string command = getEnclosedString(line, '"', '"', 0);
			if (!command.empty())
				result = new Actions::RunExternalCommand(std::move(command));
			else
				error() << "empty command passed to run_external_command\n";
		}
	}
	return result;
}

}

NC::Key::Type readKey(NC::Window &w)
{
	NC::Key::Type result = NC::Key::None;
	std::string tmp;
	NC::Key::Type input;
	bool alt_pressed = false;
	while (true)
	{
		input = w.readKey();
		if (input == NC::Key::None)
			break;
		if (input & NC::Key::Alt)
		{
			// Complete the key and reapply the mask at the end.
			alt_pressed = true;
			input &= ~NC::Key::Alt;
		}
		if (input > 255) // NC special character
		{
			result = input;
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
				result = wc;
				break;
			}
		}
	}
	if (alt_pressed)
		result |= NC::Key::Alt;
	return result;
}

std::wstring keyToWString(const NC::Key::Type key)
{
	std::wstring result;

	if (key == NC::Key::Tab)
		result += L"Tab";
	else if (key == NC::Key::Enter)
		result += L"Enter";
	else if (key == NC::Key::Escape)
		result += L"Escape";
	else if (key >= NC::Key::Ctrl_A && key <= NC::Key::Ctrl_Z)
	{
		result += L"Ctrl-";
		result += 'A' + (key - NC::Key::Ctrl_A);
	}
	else if (key == NC::Key::Ctrl_LeftBracket)
		result += L"Ctrl-[";
	else if (key == NC::Key::Ctrl_Backslash)
		result += L"Ctrl-\\";
	else if (key == NC::Key::Ctrl_RightBracket)
		result += L"Ctrl-]";
	else if (key == NC::Key::Ctrl_Caret)
		result += L"Ctrl-^";
	else if (key == NC::Key::Ctrl_Underscore)
		result += L"Ctrl-_";
	else if (key & NC::Key::Alt)
	{
		result += L"Alt-";
		result += keyToWString(key & ~NC::Key::Alt);
	}
	else if (key & NC::Key::Ctrl)
	{
		result += L"Ctrl-";
		result += keyToWString(key & ~NC::Key::Ctrl);
	}
	else if (key & NC::Key::Shift)
	{
		result += L"Shift-";
		result += keyToWString(key & ~NC::Key::Shift);
	}
	else if (key == NC::Key::Space)
		result += L"Space";
	else if (key == NC::Key::Backspace)
		result += L"Backspace";
	else if (key == NC::Key::Insert)
		result += L"Insert";
	else if (key == NC::Key::Delete)
		result += L"Delete";
	else if (key == NC::Key::Home)
		result += L"Home";
	else if (key == NC::Key::End)
		result += L"End";
	else if (key == NC::Key::PageUp)
		result += L"PageUp";
	else if (key == NC::Key::PageDown)
		result += L"PageDown";
	else if (key == NC::Key::Up)
		result += L"Up";
	else if (key == NC::Key::Down)
		result += L"Down";
	else if (key == NC::Key::Left)
		result += L"Left";
	else if (key == NC::Key::Right)
		result += L"Right";
	else if (key >= NC::Key::F1 && key <= NC::Key::F9)
	{
		result += L"F";
		result += '1' + (key - NC::Key::F1);
	}
	else if (key >= NC::Key::F10 && key <= NC::Key::F12)
	{
		result += L"F1";
		result += '0' + (key - NC::Key::F10);
	}
	else
		result += std::wstring(1, key);

	return result;
}

bool BindingsConfiguration::read(const std::string &file)
{
	enum class InProgress { None, Command, Key };
	
	bool result = true;
	
	std::ifstream f(file);
	if (!f.is_open())
		return result;
	
	// shared variables
	InProgress in_progress = InProgress::None;
	size_t line_no = 0;
	std::string line;
	Binding::ActionChain actions;
	
	// def_key specific variables
	NC::Key::Type key = NC::Key::None;
	std::string strkey;
	
	// def_command specific variables
	bool cmd_immediate = false;
	std::string cmd_name;
	
	auto error = [&]() -> std::ostream & {
		std::cerr << file << ":" << line_no << ": error: ";
		in_progress = InProgress::None;
		result = false;
		return std::cerr;
	};
	
	auto bind_in_progress = [&]() -> bool {
		if (in_progress == InProgress::Command)
		{
			if (!actions.empty())
			{
				m_commands.insert(std::make_pair(cmd_name, Command(std::move(actions), cmd_immediate)));
				actions.clear();
				return true;
			}
			else
			{
				error() << "definition of command '" << cmd_name << "' cannot be empty\n";
				return false;
			}
		}
		else if (in_progress == InProgress::Key)
		{
			if (!actions.empty())
			{
				bind(key, actions);
				actions.clear();
				return true;
			}
			else
			{
				error() << "definition of key '" << strkey << "' cannot be empty\n";
				return false;
			}
		}
		return true;
	};
	
	const char def_command[] = "def_command";
	const char def_key[] = "def_key";
	
	while (!f.eof() && ++line_no)
	{
		getline(f, line);
		if (line.empty() || line[0] == '#')
			continue;
		
		// beginning of command definition
		if (!line.compare(0, const_strlen(def_command), def_command))
		{
			if (!bind_in_progress())
				break;
			in_progress = InProgress::Command;
			cmd_name = getEnclosedString(line, '"', '"', 0);
			if (cmd_name.empty())
			{
				error() << "command must have non-empty name\n";
				break;
			}
			if (m_commands.find(cmd_name) != m_commands.end())
			{
				error() << "redefinition of command '" << cmd_name << "'\n";
				break;
			}
			std::string cmd_type = getEnclosedString(line, '[', ']', 0);
			if (cmd_type == "immediate")
				cmd_immediate = true;
			else if (cmd_type == "deferred")
				cmd_immediate = false;
			else
			{
				error() << "invalid type of command: '" << cmd_type << "'\n";
				break;
			}
		}
		// beginning of key definition
		else if (!line.compare(0, const_strlen(def_key), def_key))
		{
			if (!bind_in_progress())
				break;
			in_progress = InProgress::Key;
			strkey = getEnclosedString(line, '"', '"', 0);
			key = stringToKey(strkey);
			if (key == NC::Key::None)
			{
				error() << "invalid key: '" << strkey << "'\n";
				break;
			}
		}
		else if (isspace(line[0])) // name of action to be bound
		{
			boost::trim(line);
			auto action = parseActionLine(line, error);
			if (action)
				actions.push_back(action);
			else
			{
				error() << "unknown action: '" << line << "'\n";
				break;
			}
		}
		else
		{
			error() << "invalid line: '" << line << "'\n";
			break;
		}
	}
	bind_in_progress();
	f.close();
	return result;
}

void BindingsConfiguration::generateDefaults()
{
	NC::Key::Type k = NC::Key::None;
	if (notBound(k = stringToKey("mouse")))
		bind(k, Actions::Type::MouseEvent);
	if (notBound(k = stringToKey("up")))
		bind(k, Actions::Type::ScrollUp);
	if (notBound(k = stringToKey("shift-up")))
		bind(k, Binding::ActionChain({ &Actions::get(Actions::Type::SelectItem), &Actions::get(Actions::Type::ScrollUp) }));
	if (notBound(k = stringToKey("down")))
		bind(k, Actions::Type::ScrollDown);
	if (notBound(k = stringToKey("shift-down")))
		bind(k, Binding::ActionChain({ &Actions::get(Actions::Type::SelectItem), &Actions::get(Actions::Type::ScrollDown) }));
	if (notBound(k = stringToKey("[")))
		bind(k, Actions::Type::ScrollUpAlbum);
	if (notBound(k = stringToKey("]")))
		bind(k, Actions::Type::ScrollDownAlbum);
	if (notBound(k = stringToKey("{")))
		bind(k, Actions::Type::ScrollUpArtist);
	if (notBound(k = stringToKey("}")))
		bind(k, Actions::Type::ScrollDownArtist);
	if (notBound(k = stringToKey("page_up")))
		bind(k, Actions::Type::PageUp);
	if (notBound(k = stringToKey("page_down")))
		bind(k, Actions::Type::PageDown);
	if (notBound(k = stringToKey("home")))
		bind(k, Actions::Type::MoveHome);
	if (notBound(k = stringToKey("end")))
		bind(k, Actions::Type::MoveEnd);
	if (notBound(k = stringToKey("insert")))
		bind(k, Actions::Type::SelectItem);
	if (notBound(k = stringToKey("enter")))
	{
		bind(k, Actions::Type::EnterDirectory);
		bind(k, Actions::Type::ToggleOutput);
		bind(k, Actions::Type::RunAction);
		bind(k, Actions::Type::PlayItem);
	}
	if (notBound(k = stringToKey("space")))
	{
		bind(k, Actions::Type::AddItemToPlaylist);
		bind(k, Actions::Type::ToggleLyricsUpdateOnSongChange);
		bind(k, Actions::Type::ToggleVisualizationType);
	}
	if (notBound(k = stringToKey("delete")))
	{
		bind(k, Actions::Type::DeletePlaylistItems);
		bind(k, Actions::Type::DeleteBrowserItems);
		bind(k, Actions::Type::DeleteStoredPlaylist);
	}
	if (notBound(k = stringToKey("right")))
	{
		bind(k, Actions::Type::NextColumn);
		bind(k, Actions::Type::SlaveScreen);
		bind(k, Actions::Type::VolumeUp);
	}
	if (notBound(k = stringToKey("+")))
		bind(k, Actions::Type::VolumeUp);
	if (notBound(k = stringToKey("left")))
	{
		bind(k, Actions::Type::PreviousColumn);
		bind(k, Actions::Type::MasterScreen);
		bind(k, Actions::Type::VolumeDown);
	}
	if (notBound(k = stringToKey("-")))
		bind(k, Actions::Type::VolumeDown);
	if (notBound(k = stringToKey(":")))
		bind(k, Actions::Type::ExecuteCommand);
	if (notBound(k = stringToKey("tab")))
		bind(k, Actions::Type::NextScreen);
	if (notBound(k = stringToKey("shift-tab")))
		bind(k, Actions::Type::PreviousScreen);
	if (notBound(k = stringToKey("f1")))
		bind(k, Actions::Type::ShowHelp);
	if (notBound(k = stringToKey("1")))
		bind(k, Actions::Type::ShowPlaylist);
	if (notBound(k = stringToKey("2")))
	{
		bind(k, Actions::Type::ShowBrowser);
		bind(k, Actions::Type::ChangeBrowseMode);
	}
	if (notBound(k = stringToKey("3")))
	{
		bind(k, Actions::Type::ShowSearchEngine);
		bind(k, Actions::Type::ResetSearchEngine);
	}
	if (notBound(k = stringToKey("4")))
	{
		bind(k, Actions::Type::ShowMediaLibrary);
		bind(k, Actions::Type::ToggleMediaLibraryColumnsMode);
	}
	if (notBound(k = stringToKey("5")))
		bind(k, Actions::Type::ShowPlaylistEditor);
	if (notBound(k = stringToKey("6")))
		bind(k, Actions::Type::ShowTagEditor);
	if (notBound(k = stringToKey("7")))
		bind(k, Actions::Type::ShowOutputs);
	if (notBound(k = stringToKey("8")))
		bind(k, Actions::Type::ShowVisualizer);
	if (notBound(k = stringToKey("=")))
		bind(k, Actions::Type::ShowClock);
	if (notBound(k = stringToKey("@")))
		bind(k, Actions::Type::ShowServerInfo);
	if (notBound(k = stringToKey("s")))
		bind(k, Actions::Type::Stop);
	if (notBound(k = stringToKey("p")))
		bind(k, Actions::Type::Pause);
	if (notBound(k = stringToKey(">")))
		bind(k, Actions::Type::Next);
	if (notBound(k = stringToKey("<")))
		bind(k, Actions::Type::Previous);
	if (notBound(k = stringToKey("ctrl-h")))
	{
		bind(k, Actions::Type::JumpToParentDirectory);
		bind(k, Actions::Type::ReplaySong);
	}
	if (notBound(k = stringToKey("backspace")))
	{
		bind(k, Actions::Type::JumpToParentDirectory);
		bind(k, Actions::Type::ReplaySong);
	}
	if (notBound(k = stringToKey("f")))
		bind(k, Actions::Type::SeekForward);
	if (notBound(k = stringToKey("b")))
		bind(k, Actions::Type::SeekBackward);
	if (notBound(k = stringToKey("r")))
		bind(k, Actions::Type::ToggleRepeat);
	if (notBound(k = stringToKey("z")))
		bind(k, Actions::Type::ToggleRandom);
	if (notBound(k = stringToKey("y")))
	{
		bind(k, Actions::Type::SaveTagChanges);
		bind(k, Actions::Type::StartSearching);
		bind(k, Actions::Type::ToggleSingle);
	}
	if (notBound(k = stringToKey("R")))
		bind(k, Actions::Type::ToggleConsume);
	if (notBound(k = stringToKey("Y")))
		bind(k, Actions::Type::ToggleReplayGainMode);
	if (notBound(k = stringToKey("T")))
		bind(k, Actions::Type::ToggleAddMode);
	if (notBound(k = stringToKey("|")))
		bind(k, Actions::Type::ToggleMouse);
	if (notBound(k = stringToKey("#")))
		bind(k, Actions::Type::ToggleBitrateVisibility);
	if (notBound(k = stringToKey("Z")))
		bind(k, Actions::Type::Shuffle);
	if (notBound(k = stringToKey("x")))
		bind(k, Actions::Type::ToggleCrossfade);
	if (notBound(k = stringToKey("X")))
		bind(k, Actions::Type::SetCrossfade);
	if (notBound(k = stringToKey("u")))
		bind(k, Actions::Type::UpdateDatabase);
	if (notBound(k = stringToKey("ctrl-s")))
	{
		bind(k, Actions::Type::SortPlaylist);
		bind(k, Actions::Type::ToggleBrowserSortMode);
		bind(k, Actions::Type::ToggleMediaLibrarySortMode);
	}
	if (notBound(k = stringToKey("ctrl-r")))
		bind(k, Actions::Type::ReversePlaylist);
	if (notBound(k = stringToKey("ctrl-_")))
		bind(k, Actions::Type::SelectFoundItems);
	if (notBound(k = stringToKey("/")))
	{
		bind(k, Actions::Type::Find);
		bind(k, Actions::Type::FindItemForward);
	}
	if (notBound(k = stringToKey("?")))
	{
		bind(k, Actions::Type::Find);
		bind(k, Actions::Type::FindItemBackward);
	}
	if (notBound(k = stringToKey(".")))
		bind(k, Actions::Type::NextFoundItem);
	if (notBound(k = stringToKey(",")))
		bind(k, Actions::Type::PreviousFoundItem);
	if (notBound(k = stringToKey("w")))
		bind(k, Actions::Type::ToggleFindMode);
	if (notBound(k = stringToKey("e")))
	{
		bind(k, Actions::Type::EditSong);
		bind(k, Actions::Type::EditLibraryTag);
		bind(k, Actions::Type::EditLibraryAlbum);
		bind(k, Actions::Type::EditDirectoryName);
		bind(k, Actions::Type::EditPlaylistName);
		bind(k, Actions::Type::EditLyrics);
	}
	if (notBound(k = stringToKey("i")))
		bind(k, Actions::Type::ShowSongInfo);
	if (notBound(k = stringToKey("I")))
		bind(k, Actions::Type::ShowArtistInfo);
	if (notBound(k = stringToKey("g")))
		bind(k, Actions::Type::JumpToPositionInSong);
	if (notBound(k = stringToKey("l")))
		bind(k, Actions::Type::ShowLyrics);
	if (notBound(k = stringToKey("ctrl-v")))
		bind(k, Actions::Type::SelectRange);
	if (notBound(k = stringToKey("v")))
		bind(k, Actions::Type::ReverseSelection);
	if (notBound(k = stringToKey("V")))
		bind(k, Actions::Type::RemoveSelection);
	if (notBound(k = stringToKey("B")))
		bind(k, Actions::Type::SelectAlbum);
	if (notBound(k = stringToKey("a")))
		bind(k, Actions::Type::AddSelectedItems);
	if (notBound(k = stringToKey("c")))
	{
		bind(k, Actions::Type::ClearPlaylist);
		bind(k, Actions::Type::ClearMainPlaylist);
	}
	if (notBound(k = stringToKey("C")))
	{
		bind(k, Actions::Type::CropPlaylist);
		bind(k, Actions::Type::CropMainPlaylist);
	}
	if (notBound(k = stringToKey("m")))
	{
		bind(k, Actions::Type::MoveSortOrderUp);
		bind(k, Actions::Type::MoveSelectedItemsUp);
		bind(k, Actions::Type::SetVisualizerSampleMultiplier);
	}
	if (notBound(k = stringToKey("n")))
	{
		bind(k, Actions::Type::MoveSortOrderDown);
		bind(k, Actions::Type::MoveSelectedItemsDown);
	}
	if (notBound(k = stringToKey("M")))
		bind(k, Actions::Type::MoveSelectedItemsTo);
	if (notBound(k = stringToKey("A")))
		bind(k, Actions::Type::Add);
	if (notBound(k = stringToKey("S")))
		bind(k, Actions::Type::SavePlaylist);
	if (notBound(k = stringToKey("o")))
		bind(k, Actions::Type::JumpToPlayingSong);
	if (notBound(k = stringToKey("G")))
	{
		bind(k, Actions::Type::JumpToBrowser);
		bind(k, Actions::Type::JumpToPlaylistEditor);
	}
	if (notBound(k = stringToKey("~")))
		bind(k, Actions::Type::JumpToMediaLibrary);
	if (notBound(k = stringToKey("E")))
		bind(k, Actions::Type::JumpToTagEditor);
	if (notBound(k = stringToKey("U")))
		bind(k, Actions::Type::TogglePlayingSongCentering);
	if (notBound(k = stringToKey("P")))
		bind(k, Actions::Type::ToggleDisplayMode);
	if (notBound(k = stringToKey("\\")))
		bind(k, Actions::Type::ToggleInterface);
	if (notBound(k = stringToKey("!")))
		bind(k, Actions::Type::ToggleSeparatorsBetweenAlbums);
	if (notBound(k = stringToKey("L")))
		bind(k, Actions::Type::ToggleLyricsFetcher);
	if (notBound(k = stringToKey("F")))
		bind(k, Actions::Type::ToggleFetchingLyricsInBackground);
	if (notBound(k = stringToKey("ctrl-l")))
		bind(k, Actions::Type::ToggleScreenLock);
	if (notBound(k = stringToKey("`")))
	{
		bind(k, Actions::Type::ToggleLibraryTagType);
		bind(k, Actions::Type::RefetchLyrics);
		bind(k, Actions::Type::AddRandomItems);
	}
	if (notBound(k = stringToKey("ctrl-p")))
		bind(k, Actions::Type::SetSelectedItemsPriority);
	if (notBound(k = stringToKey("q")))
		bind(k, Actions::Type::Quit);
	
	if (notBound(k = stringToKey("-")))
		bind(k, Actions::Type::VolumeDown);
}

const Command *BindingsConfiguration::findCommand(const std::string &name)
{
	const Command *ptr = nullptr;
	auto it = m_commands.find(name);
	if (it != m_commands.end())
		ptr = &it->second;
	return ptr;
}

BindingsConfiguration::BindingIteratorPair BindingsConfiguration::get(const NC::Key::Type &k)
{
	std::pair<BindingIterator, BindingIterator> result;
	auto it = m_bindings.find(k);
	if (it != m_bindings.end()) {
		result.first = it->second.begin();
		result.second = it->second.end();
	} else {
		auto list_end = m_bindings.begin()->second.end();
		result.first = list_end;
		result.second = list_end;
	}
	return result;
}
