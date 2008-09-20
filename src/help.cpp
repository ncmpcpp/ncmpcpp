/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "help.h"
#include "settings.h"

extern ncmpcpp_keys Key;

string DisplayKeys(int *key, int size = 2)
{
	bool backspace = 1;
	string result = "\t";
	for (int i = 0; i < size; i++)
	{
		if (key[i] == null_key);
		else if (key[i] == 259)
			result += "Up";
		else if (key[i] == 258)
			result += "Down";
		else if (key[i] == 339)
			result += "Page Up";
		else if (key[i] == 338)
			result += "Page Down";
		else if (key[i] == 262)
			result += "Home";
		else if (key[i] == 360)
			result += "End";
		else if (key[i] == 32)
			result += "Space";
		else if (key[i] == 10)
			result += "Enter";
		else if (key[i] == 330)
			result += "Delete";
		else if (key[i] == 261)
			result += "Right";
		else if (key[i] == 260)
			result += "Left";
		else if (key[i] == 9)
			result += "Tab";
		else if (key[i] >= 1 && key[i] <= 26)
		{
			result += "Ctrl-";
			result += key[i]+64;
		}
		else if (key[i] >= 265 && key[i] <= 276)
		{
			result += "F";
			result += key[i]-216;
		}
		else if ((key[i] == 263 || key[i] == 127) && !backspace);
		else if ((key[i] == 263 || key[i] == 127) && backspace)
		{
			result += "Backspace";
			backspace = 0;
		}
		else
			result += key[i];
		result += " ";
	}
	if (result.length() > 12)
		result = result.substr(0, 12);
	for (int i = result.length(); i <= 12; result += " ", i++);
	result += ": ";
	return result;
}

string GetKeybindings()
{
	string result;
	
	result += "   [.b]Keys - Movement\n -----------------------------------------[/b]\n";
	result += DisplayKeys(Key.Up) + "Move Cursor up\n";
	result += DisplayKeys(Key.Down) + "Move Cursor down\n";
	result += DisplayKeys(Key.PageUp) + "Page up\n";
	result += DisplayKeys(Key.PageDown) + "Page down\n";
	result += DisplayKeys(Key.Home) + "Home\n";
	result += DisplayKeys(Key.End) + "End\n\n";
	
	result += DisplayKeys(Key.ScreenSwitcher) + "Switch between playlist and browser\n";
	result += DisplayKeys(Key.Help) + "Help screen\n";
	result += DisplayKeys(Key.Playlist) + "Playlist screen\n";
	result += DisplayKeys(Key.Browser) + "Browse screen\n";
	result += DisplayKeys(Key.SearchEngine) + "Search engine\n";
	result += DisplayKeys(Key.MediaLibrary) + "Media library\n";
	result += DisplayKeys(Key.PlaylistEditor) + "Playlist editor\n";
	result += DisplayKeys(Key.TagEditor) + "Tag editor\n\n\n";
	
	
	result += "   [.b]Keys - Global\n -----------------------------------------[/b]\n";
	result += DisplayKeys(Key.Stop) + "Stop\n";
	result += DisplayKeys(Key.Pause) + "Pause\n";
	result += DisplayKeys(Key.Next) + "Next track\n";
	result += DisplayKeys(Key.Prev) + "Previous track\n";
	result += DisplayKeys(Key.SeekForward) + "Seek forward\n";
	result += DisplayKeys(Key.SeekBackward) + "Seek backward\n";
	result += DisplayKeys(Key.VolumeDown) + "Decrease volume\n";
	result += DisplayKeys(Key.VolumeUp) + "Increase volume\n\n";
	
	result += DisplayKeys(Key.ToggleSpaceMode) + "Toggle space mode (select/add)\n";
	result += DisplayKeys(Key.ReverseSelection) + "Reverse selection\n";
	result += DisplayKeys(Key.DeselectAll) + "Deselect all items\n";
	result += DisplayKeys(Key.AddSelected) + "Add selected items to playlist/m3u file\n\n";
	
	result += DisplayKeys(Key.ToggleRepeat) + "Toggle repeat mode\n";
	result += DisplayKeys(Key.ToggleRepeatOne) + "Toggle \"repeat one\" mode\n";
	result += DisplayKeys(Key.ToggleRandom) + "Toggle random mode\n";
	result += DisplayKeys(Key.Shuffle) + "Shuffle playlist\n";
	result += DisplayKeys(Key.ToggleCrossfade) + "Toggle crossfade mode\n";
	result += DisplayKeys(Key.SetCrossfade) + "Set crossfade\n";
	result += DisplayKeys(Key.UpdateDB) + "Start a music database update\n\n";
	
	result += DisplayKeys(Key.FindForward) + "Forward find\n";
	result += DisplayKeys(Key.FindBackward) + "Backward find\n";
	result += DisplayKeys(Key.PrevFoundPosition) + "Go to previous found position\n";
	result += DisplayKeys(Key.NextFoundPosition) + "Go to next found position\n";
	result += DisplayKeys(Key.ToggleFindMode) + "Toggle find mode (normal/wrapped)\n";
	result += DisplayKeys(Key.GoToContainingDir) + "Go to directory containing current item\n";
#	ifdef HAVE_TAGLIB_H
	result += DisplayKeys(Key.EditTags) + "Edit song's tags/playlist's name\n";
#	endif // HAVE_TAGLIB_H
	result += DisplayKeys(Key.GoToPosition) + "Go to chosen position in current song\n";
	result += DisplayKeys(Key.SongInfo) + "Show song's info\n";
#	ifdef HAVE_CURL_CURL_H
	result += DisplayKeys(Key.ArtistInfo) + "Show artist's info\n";
#	endif // HAVE_CURL_CURL_H
	result += DisplayKeys(Key.Lyrics) + "Show/hide song's lyrics\n\n";
	
	result += DisplayKeys(Key.Quit) + "Quit\n\n\n";
	
	
	result += "   [.b]Keys - Playlist screen\n -----------------------------------------[/b]\n";
	result += DisplayKeys(Key.Enter) + "Play\n";
	result += DisplayKeys(Key.Delete) + "Delete item/selected items from playlist\n";
	result += DisplayKeys(Key.Clear) + "Clear playlist\n";
	result += DisplayKeys(Key.Crop) + "Clear playlist but hold currently playing/selected items\n";
	result += DisplayKeys(Key.MvSongUp) + "Move item/group of items up\n";
	result += DisplayKeys(Key.MvSongDown) + "Move item/group of items down\n";
	result += DisplayKeys(Key.Add) + "Add url/file/directory to playlist\n";
	result += DisplayKeys(Key.SavePlaylist) + "Save playlist\n";
	result += DisplayKeys(Key.GoToNowPlaying) + "Go to currently playing position\n";
	result += DisplayKeys(Key.TogglePlaylistDisplayMode) + "Toggle playlist display mode\n";
	result += DisplayKeys(Key.ToggleAutoCenter) + "Toggle auto center mode\n\n\n";
	
	result += "   [.b]Keys - Browse screen\n -----------------------------------------[/b]\n";
	result += DisplayKeys(Key.Enter) + "Enter directory/Add item to playlist and play\n";
	result += DisplayKeys(Key.Space) + "Add item to playlist\n";
	result += DisplayKeys(Key.GoToParentDir) + "Go to parent directory\n";
	result += DisplayKeys(Key.Delete) + "Delete playlist\n\n\n";
	
	
	result += "   [.b]Keys - Search engine\n -----------------------------------------[/b]\n";
	result += DisplayKeys(Key.Enter) + "Add item to playlist and play/change option\n";
	result += DisplayKeys(Key.Space) + "Add item to playlist\n";
	result += DisplayKeys(Key.StartSearching) + "Start searching immediately\n\n\n";
	
	
	result += "   [.b]Keys - Media library\n -----------------------------------------[/b]\n";
	result += DisplayKeys(&Key.VolumeDown[0], 1) + "Previous column\n";
	result += DisplayKeys(&Key.VolumeUp[0], 1) + "Next column\n";
	result += DisplayKeys(Key.Enter) + "Add to playlist and play song/album/artist's songs\n";
	result += DisplayKeys(Key.Space) + "Add to playlist song/album/artist's songs\n\n\n";
	
	
	result += "   [.b]Keys - Playlist Editor\n -----------------------------------------[/b]\n";
	result += DisplayKeys(&Key.VolumeDown[0], 1) + "Previous column\n";
	result += DisplayKeys(&Key.VolumeUp[0], 1) + "Next column\n";
	result += DisplayKeys(Key.Enter) + "Add item to playlist and play\n";
	result += DisplayKeys(Key.Space) + "Add to playlist/select item\n";
#	ifndef HAVE_TAGLIB_H
	result += DisplayKeys(Key.EditTags) + "Edit playlist's name\n";
#	endif // ! HAVE_TAGLIB_H
	result += DisplayKeys(Key.MvSongUp) + "Move item/group of items up\n";
	result += DisplayKeys(Key.MvSongDown) + "Move item/group of items down\n";
	
	result += "\n\n   [.b]Keys - Lyrics\n -----------------------------------------[/b]\n";
	result += DisplayKeys(Key.Space) + "Switch for following lyrics of now playing song\n";
	
#	ifdef HAVE_TAGLIB_H
	result += "\n\n   [.b]Keys - Tag editor\n -----------------------------------------[/b]\n";
	result += DisplayKeys(Key.Enter) + "Change tag/filename for one song (left column)\n";
	result += DisplayKeys(Key.Enter) + "Perform operation on all/selected songs (middle column)\n";
	result += DisplayKeys(Key.Space) + "Switch to albums/directories view (left column)\n";
	result += DisplayKeys(Key.Space) + "Select/deselect song (right column)\n";
	result += DisplayKeys(&Key.VolumeDown[0], 1) + "Previous column\n";
	result += DisplayKeys(&Key.VolumeUp[0], 1) + "Next column\n";
#	endif // HAVE_TAGLIB_H
	
	return result;
}

