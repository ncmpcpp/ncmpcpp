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

#include "keys.h"

KeyConfiguration Keys;

Key Key::noOp = Key(ERR, NCurses);

void KeyConfiguration::generateBindings()
{
	bind_(KEY_MOUSE, Key::NCurses, aMouseEvent);
	bind_(KEY_UP, Key::NCurses, aScrollUp);
	bind_(KEY_DOWN, Key::NCurses, aScrollDown);
	bind_('[', Key::Standard, aScrollUpAlbum);
	bind_(']', Key::Standard, aScrollDownAlbum);
	bind_('{', Key::Standard, aScrollUpArtist);
	bind_('}', Key::Standard, aScrollDownArtist);
	bind_(KEY_PPAGE, Key::NCurses, aPageUp);
	bind_(KEY_NPAGE, Key::NCurses, aPageDown);
	bind_(KEY_HOME, Key::NCurses, aMoveHome);
	bind_(KEY_END, Key::NCurses, aMoveEnd);
	bind_(KEY_SPACE, Key::Standard, aPressSpace);
	bind_(KEY_ENTER, Key::Standard, aPressEnter);
	bind_(KEY_DC, Key::NCurses, aDelete);
	bind_(KEY_RIGHT, Key::NCurses, aNextColumn);
	bind_(KEY_RIGHT, Key::NCurses, aSlaveScreen);
	bind_(KEY_RIGHT, Key::NCurses, aVolumeUp);
	bind_(KEY_LEFT, Key::NCurses, aPreviousColumn);
	bind_(KEY_LEFT, Key::NCurses, aMasterScreen);
	bind_(KEY_LEFT, Key::NCurses, aVolumeDown);
	bind_(KEY_TAB, Key::Standard, aNextScreen);
	bind_(KEY_SHIFT_TAB, Key::NCurses, aPreviousScreen);
	bind_('1', Key::Standard, aShowHelp);
	bind_('2', Key::Standard, aShowPlaylist);
	bind_('3', Key::Standard, aShowBrowser);
	bind_('4', Key::Standard, aShowSearchEngine);
	bind_('5', Key::Standard, aShowMediaLibrary);
	bind_('6', Key::Standard, aShowPlaylistEditor);
	bind_('7', Key::Standard, aShowTagEditor);
	bind_('8', Key::Standard, aShowOutputs);
	bind_('9', Key::Standard, aShowVisualizer);
	bind_('0', Key::Standard, aShowClock);
	bind_('@', Key::Standard, aShowServerInfo);
	bind_('s', Key::Standard, aStop);
	bind_('P', Key::Standard, aPause);
	bind_('>', Key::Standard, aNextSong);
	bind_('<', Key::Standard, aPreviousSong);
	bind_(KEY_CTRL_H, Key::Standard, aJumpToParentDir);
	bind_(KEY_CTRL_H, Key::Standard, aReplaySong);
	bind_(KEY_BACKSPACE, Key::NCurses, aJumpToParentDir);
	bind_(KEY_BACKSPACE, Key::NCurses, aReplaySong);
	bind_(KEY_BACKSPACE_2, Key::Standard, aJumpToParentDir);
	bind_(KEY_BACKSPACE_2, Key::Standard, aReplaySong);
	bind_('f', Key::Standard, aSeekForward);
	bind_('b', Key::Standard, aSeekBackward);
	bind_('r', Key::Standard, aToggleRepeat);
	bind_('z', Key::Standard, aToggleRandom);
	bind_('y', Key::Standard, aSaveTagChanges);
	bind_('y', Key::Standard, aStartSearching);
	bind_('y', Key::Standard, aToggleSingle);
	bind_('R', Key::Standard, aToggleConsume);
	bind_('Y', Key::Standard, aToggleReplayGainMode);
	bind_('t', Key::Standard, aToggleSpaceMode);
	bind_('T', Key::Standard, aToggleAddMode);
	bind_('|', Key::Standard, aToggleMouse);
	bind_('#', Key::Standard, aToggleBitrateVisibility);
	bind_('Z', Key::Standard, aShuffle);
	bind_('x', Key::Standard, aToggleCrossfade);
	bind_('X', Key::Standard, aSetCrossfade);
	bind_('u', Key::Standard, aUpdateDatabase);
	bind_(KEY_CTRL_V, Key::Standard, aSortPlaylist);
	bind_(KEY_CTRL_R, Key::Standard, aReversePlaylist);
	bind_(KEY_CTRL_F, Key::Standard, aApplyFilter);
	bind_(KEY_CTRL_G, Key::Standard, aDisableFilter);
	bind_('/', Key::Standard, aFind);
	bind_('/', Key::Standard, aFindItemForward);
	bind_('?', Key::Standard, aFind);
	bind_('?', Key::Standard, aFindItemBackward);
	bind_('.', Key::Standard, aNextFoundItem);
	bind_(',', Key::Standard, aPreviousFoundItem);
	bind_('w', Key::Standard, aToggleFindMode);
	bind_('e', Key::Standard, aEditSong);
	bind_('e', Key::Standard, aEditLibraryTag);
	bind_('e', Key::Standard, aEditLibraryAlbum);
	bind_('e', Key::Standard, aEditDirectoryName);
	bind_('e', Key::Standard, aEditPlaylistName);
	bind_('e', Key::Standard, aEditLyrics);
	bind_('i', Key::Standard, aShowSongInfo);
	bind_('I', Key::Standard, aShowArtistInfo);
	bind_('g', Key::Standard, aJumpToPositionInSong);
	bind_('l', Key::Standard, aShowLyrics);
	bind_('v', Key::Standard, aReverseSelection);
	bind_('V', Key::Standard, aDeselectItems);
	bind_('B', Key::Standard, aSelectAlbum);
	bind_('a', Key::Standard, aAddSelectedItems);
	bind_('c', Key::Standard, aClearPlaylist);
	bind_('c', Key::Standard, aClearMainPlaylist);
	bind_('C', Key::Standard, aCropPlaylist);
	bind_('C', Key::Standard, aCropMainPlaylist);
	bind_('m', Key::Standard, aMoveSortOrderUp);
	bind_('m', Key::Standard, aMoveSelectedItemsUp);
	bind_('n', Key::Standard, aMoveSortOrderDown);
	bind_('n', Key::Standard, aMoveSelectedItemsDown);
	bind_('M', Key::Standard, aMoveSelectedItemsTo);
	bind_('A', Key::Standard, aAdd);
	bind_('S', Key::Standard, aSavePlaylist);
	bind_('o', Key::Standard, aJumpToPlayingSong);
	bind_('G', Key::Standard, aJumpToBrowser);
	bind_('G', Key::Standard, aJumpToPlaylistEditor);
	bind_('~', Key::Standard, aJumpToMediaLibrary);
	bind_('E', Key::Standard, aJumpToTagEditor);
	bind_('U', Key::Standard, aToggleAutoCenter);
	bind_('p', Key::Standard, aToggleDisplayMode);
	bind_('\\', Key::Standard, aToggleInterface);
	bind_('!', Key::Standard, aToggleSeparatorsInPlaylist);
	bind_('L', Key::Standard, aToggleLyricsFetcher);
	bind_('F', Key::Standard, aToggleFetchingLyricsInBackground);
	bind_(KEY_CTRL_L, Key::Standard, aToggleScreenLock);
	bind_('`', Key::Standard, aToggleBrowserSortMode);
	bind_('`', Key::Standard, aToggleLibraryTagType);
	bind_('`', Key::Standard, aRefetchLyrics);
	bind_('`', Key::Standard, aRefetchArtistInfo);
	bind_('`', Key::Standard, aAddRandomItems);
	bind_(KEY_CTRL_P, Key::Standard, aSetSelectedItemsPriority);
	bind_('q', Key::Standard, aQuit);
	
	bind_('k', Key::Standard, aScrollUp);
	bind_('j', Key::Standard, aScrollDown);
	bind_('d', Key::Standard, aDelete);
	bind_('+', Key::Standard, aVolumeUp);
	bind_('-', Key::Standard, aVolumeDown);
	bind_(KEY_F1, Key::NCurses, aShowHelp);
	bind_(KEY_F2, Key::NCurses, aShowPlaylist);
	bind_(KEY_F3, Key::NCurses, aShowBrowser);
	bind_(KEY_F4, Key::NCurses, aShowSearchEngine);
	bind_(KEY_F5, Key::NCurses, aShowMediaLibrary);
	bind_(KEY_F6, Key::NCurses, aShowPlaylistEditor);
	bind_(KEY_F7, Key::NCurses, aShowTagEditor);
	bind_(KEY_F8, Key::NCurses, aShowOutputs);
	bind_(KEY_F9, Key::NCurses, aShowVisualizer);
	bind_(KEY_F10, Key::NCurses, aShowClock);
	bind_('Q', Key::Standard, aQuit);
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
