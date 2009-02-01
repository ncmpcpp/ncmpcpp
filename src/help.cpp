/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include "help.h"
#include "settings.h"

extern MPD::Connection *Mpd;

extern ncmpcpp_keys Key;

namespace
{
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
				result += IntoStr(key[i]-264);
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
		for (size_t i = result.length(); i <= 12; result += " ", i++) { }
		result += ": ";
		return result;
	}
}

void GetKeybindings(Scrollpad &help)
{
	help << "   " << fmtBold << "Keys - Movement\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(Key.Up) << "Move Cursor up\n";
	help << DisplayKeys(Key.Down) << "Move Cursor down\n";
	help << DisplayKeys(Key.PageUp) << "Page up\n";
	help << DisplayKeys(Key.PageDown) << "Page down\n";
	help << DisplayKeys(Key.Home) << "Home\n";
	help << DisplayKeys(Key.End) << "End\n\n";
	
	help << DisplayKeys(Key.ScreenSwitcher) << "Switch between playlist and browser\n";
	help << DisplayKeys(Key.Help) << "Help screen\n";
	help << DisplayKeys(Key.Playlist) << "Playlist screen\n";
	help << DisplayKeys(Key.Browser) << "Browse screen\n";
	help << DisplayKeys(Key.SearchEngine) << "Search engine\n";
	help << DisplayKeys(Key.MediaLibrary) << "Media library\n";
	help << DisplayKeys(Key.PlaylistEditor) << "Playlist editor\n";
#	ifdef HAVE_TAGLIB_H
	help << DisplayKeys(Key.TagEditor) << "Tag editor\n";
#	endif // HAVE_TAGLIB_H
#	ifdef ENABLE_CLOCK
	help << DisplayKeys(Key.Clock) << "Clock screen\n";
#	endif // ENABLE_CLOCK
	help << "\n\n";
	
	help << "   " << fmtBold << "Keys - Global\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(Key.Stop) << "Stop\n";
	help << DisplayKeys(Key.Pause) << "Pause\n";
	help << DisplayKeys(Key.Next) << "Next track\n";
	help << DisplayKeys(Key.Prev) << "Previous track\n";
	help << DisplayKeys(Key.SeekForward) << "Seek forward\n";
	help << DisplayKeys(Key.SeekBackward) << "Seek backward\n";
	help << DisplayKeys(Key.VolumeDown) << "Decrease volume\n";
	help << DisplayKeys(Key.VolumeUp) << "Increase volume\n\n";
	
	help << DisplayKeys(Key.ToggleSpaceMode) << "Toggle space mode (select/add)\n";
	help << DisplayKeys(Key.ToggleAddMode) << "Toggle add mode\n";
	help << DisplayKeys(Key.ReverseSelection) << "Reverse selection\n";
	help << DisplayKeys(Key.DeselectAll) << "Deselect all items\n";
	help << DisplayKeys(Key.AddSelected) << "Add selected items to playlist/m3u file\n\n";
	
	help << DisplayKeys(Key.ToggleRepeat) << "Toggle repeat mode\n";
	help << DisplayKeys(Key.ToggleRepeatOne) << "Toggle \"repeat one\" mode\n";
	help << DisplayKeys(Key.ToggleRandom) << "Toggle random mode\n";
	help << DisplayKeys(Key.Shuffle) << "Shuffle playlist\n";
	help << DisplayKeys(Key.ToggleCrossfade) << "Toggle crossfade mode\n";
	help << DisplayKeys(Key.SetCrossfade) << "Set crossfade\n";
	help << DisplayKeys(Key.UpdateDB) << "Start a music database update\n\n";
	
	help << DisplayKeys(Key.FindForward) << "Forward find\n";
	help << DisplayKeys(Key.FindBackward) << "Backward find\n";
	help << DisplayKeys(Key.PrevFoundPosition) << "Go to previous found position\n";
	help << DisplayKeys(Key.NextFoundPosition) << "Go to next found position\n";
	help << DisplayKeys(Key.ToggleFindMode) << "Toggle find mode (normal/wrapped)\n";
	help << DisplayKeys(Key.GoToContainingDir) << "Go to directory containing current item\n";
	help << DisplayKeys(Key.ToggleDisplayMode) << "Toggle display mode\n";
#	ifdef HAVE_TAGLIB_H
	help << DisplayKeys(Key.EditTags) << "Edit song's tags/playlist's name\n";
#	endif // HAVE_TAGLIB_H
	help << DisplayKeys(Key.GoToPosition) << "Go to chosen position in current song\n";
	help << DisplayKeys(Key.SongInfo) << "Show song's info\n";
#	ifdef HAVE_CURL_CURL_H
	help << DisplayKeys(Key.ArtistInfo) << "Show artist's info\n";
#	endif // HAVE_CURL_CURL_H
	help << DisplayKeys(Key.Lyrics) << "Show/hide song's lyrics\n\n";
	
	help << DisplayKeys(Key.Quit) << "Quit\n\n\n";
	
	
	help << "   " << fmtBold << "Keys - Playlist screen\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(Key.Enter) << "Play\n";
	help << DisplayKeys(Key.Delete) << "Delete item/selected items from playlist\n";
	help << DisplayKeys(Key.Clear) << "Clear playlist\n";
	help << DisplayKeys(Key.Crop) << "Clear playlist but hold currently playing/selected items\n";
	help << DisplayKeys(Key.MvSongUp) << "Move item/group of items up\n";
	help << DisplayKeys(Key.MvSongDown) << "Move item/group of items down\n";
	help << DisplayKeys(Key.Add) << "Add url/file/directory to playlist\n";
	help << DisplayKeys(Key.SavePlaylist) << "Save playlist\n";
	help << DisplayKeys(Key.GoToNowPlaying) << "Go to currently playing position\n";
	help << DisplayKeys(Key.ToggleAutoCenter) << "Toggle auto center mode\n\n\n";
	
	help << "   " << fmtBold << "Keys - Browse screen\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(Key.Enter) << "Enter directory/Add item to playlist and play\n";
	help << DisplayKeys(Key.Space) << "Add item to playlist\n";
	if (Mpd->GetHostname()[0] == '/') // are we connected to unix socket?
		help << DisplayKeys(Key.SwitchTagTypeList) << "Browse MPD database/local filesystem\n";
	help << DisplayKeys(Key.GoToParentDir) << "Go to parent directory\n";
	help << DisplayKeys(Key.Delete) << "Delete playlist\n\n\n";
	
	
	help << "   " << fmtBold << "Keys - Search engine\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(Key.Enter) << "Add item to playlist and play/change option\n";
	help << DisplayKeys(Key.Space) << "Add item to playlist\n";
	help << DisplayKeys(Key.StartSearching) << "Start searching immediately\n\n\n";
	
	
	help << "   " << fmtBold << "Keys - Media library\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(&Key.VolumeDown[0], 1) << "Previous column\n";
	help << DisplayKeys(&Key.VolumeUp[0], 1) << "Next column\n";
	help << DisplayKeys(Key.Enter) << "Add to playlist and play song/album/artist's songs\n";
	help << DisplayKeys(Key.Space) << "Add to playlist song/album/artist's songs\n";
	help << DisplayKeys(Key.SwitchTagTypeList) << "Tag type list switcher (left column)\n\n\n";
	
	help << "   " << fmtBold << "Keys - Playlist Editorz\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(&Key.VolumeDown[0], 1) << "Previous column\n";
	help << DisplayKeys(&Key.VolumeUp[0], 1) << "Next column\n";
	help << DisplayKeys(Key.Enter) << "Add item to playlist and play\n";
	help << DisplayKeys(Key.Space) << "Add to playlist/select item\n";
#	ifndef HAVE_TAGLIB_H
	help << DisplayKeys(Key.EditTags) << "Edit playlist's name\n";
#	endif // ! HAVE_TAGLIB_H
	help << DisplayKeys(Key.MvSongUp) << "Move item/group of items up\n";
	help << DisplayKeys(Key.MvSongDown) << "Move item/group of items down\n";
	
	help << "\n\n   " << fmtBold << "Keys - Lyrics\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(Key.Space) << "Switch for following lyrics of now playing song\n";
	
#	ifdef HAVE_TAGLIB_H
	help << "\n\n   " << fmtBold << "Keys - Tag editor\n -----------------------------------------\n" << fmtBoldEnd;
	help << DisplayKeys(Key.Enter) << "Change tag/filename for one song (left column)\n";
	help << DisplayKeys(Key.Enter) << "Perform operation on all/selected songs (middle column)\n";
	help << DisplayKeys(Key.Space) << "Switch to albums/directories view (left column)\n";
	help << DisplayKeys(Key.Space) << "Select/deselect song (right column)\n";
	help << DisplayKeys(&Key.VolumeDown[0], 1) << "Previous column\n";
	help << DisplayKeys(&Key.VolumeUp[0], 1) << "Next column\n";
#	endif // HAVE_TAGLIB_H
}

