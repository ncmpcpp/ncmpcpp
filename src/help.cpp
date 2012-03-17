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

std::string Help::DisplayKeys(int *key, int size)
{
	bool backspace = 1;
	std::string result = "\t";
	for (int i = 0; i < size; i++)
	{
		if (key[i] == NcmpcppKeys::NullKey);
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
		else if (key[i] == 353)
			result += "Shift-Tab";
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
	for (size_t i = result.length(); i <= 12; result += " ", ++i) { }
	result += ": ";
	return result;
}

void Help::GetKeybindings()
{
	*w << "   " << fmtBold << "Keys - Movement\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Up)			<< "Move Cursor up\n";
	*w << DisplayKeys(Key.Down)			<< "Move Cursor down\n";
	*w << DisplayKeys(Key.UpAlbum)			<< "Move Cursor up one album\n";
	*w << DisplayKeys(Key.DownAlbum)		<< "Move Cursor down one album\n";
	*w << DisplayKeys(Key.UpArtist)			<< "Move Cursor up one artist\n";
	*w << DisplayKeys(Key.DownArtist)		<< "Move Cursor down one artist\n";
	*w << DisplayKeys(Key.PageUp)			<< "Page up\n";
	*w << DisplayKeys(Key.PageDown)			<< "Page down\n";
	*w << DisplayKeys(Key.Home)			<< "Home\n";
	*w << DisplayKeys(Key.End)			<< "End\n";
	*w << "\n";
	if (Config.screen_switcher_previous)
		*w << DisplayKeys(Key.ScreenSwitcher)   << "Switch between current and last screen\n";
	else
	{
		*w << DisplayKeys(Key.ScreenSwitcher)   << "Switch to next screen in sequence\n";
		*w << DisplayKeys(Key.BackwardScreenSwitcher) << "Switch to previous screen in sequence\n";
	}
	*w << DisplayKeys(Key.Help)			<< "Help screen\n";
	*w << DisplayKeys(Key.Playlist)			<< "Playlist screen\n";
	*w << DisplayKeys(Key.Browser)			<< "Browse screen\n";
	*w << DisplayKeys(Key.SearchEngine)		<< "Search engine\n";
	*w << DisplayKeys(Key.MediaLibrary)		<< "Media library\n";
	*w << DisplayKeys(Key.PlaylistEditor)		<< "Playlist editor\n";
#	ifdef HAVE_TAGLIB_H
	*w << DisplayKeys(Key.TagEditor)		<< "Tag editor\n";
#	endif // HAVE_TAGLIB_H
#	ifdef ENABLE_OUTPUTS
	*w << DisplayKeys(Key.Outputs)			<< "Outputs\n";
#	endif // ENABLE_OUTPUTS
#	ifdef ENABLE_VISUALIZER
	*w << DisplayKeys(Key.Visualizer)		<< "Music visualizer\n";
#	endif // ENABLE_VISUALIZER
#	ifdef ENABLE_CLOCK
	*w << DisplayKeys(Key.Clock)			<< "Clock screen\n";
#	endif // ENABLE_CLOCK
	*w << "\n";
	*w << DisplayKeys(Key.ServerInfo)		<< "MPD server info\n";
	
	*w << "\n\n   " << fmtBold << "Keys - Global\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Stop)			<< "Stop\n";
	*w << DisplayKeys(Key.Pause)			<< "Pause\n";
	*w << DisplayKeys(Key.Next)			<< "Next track\n";
	*w << DisplayKeys(Key.Prev)			<< "Previous track\n";
	*w << DisplayKeys(Key.Replay)			<< "Play current track from the beginning\n";
	*w << DisplayKeys(Key.SeekForward)		<< "Seek forward\n";
	*w << DisplayKeys(Key.SeekBackward)		<< "Seek backward\n";
	*w << DisplayKeys(Key.VolumeDown)		<< "Decrease volume\n";
	*w << DisplayKeys(Key.VolumeUp)			<< "Increase volume\n";
	*w << "\n";
	*w << DisplayKeys(Key.ToggleSpaceMode)		<< "Toggle space mode (select/add)\n";
	*w << DisplayKeys(Key.ToggleAddMode)		<< "Toggle add mode\n";
	*w << DisplayKeys(Key.ToggleMouse)		<< "Toggle mouse support\n";
	*w << DisplayKeys(Key.ReverseSelection)		<< "Reverse selection\n";
	*w << DisplayKeys(Key.DeselectAll)		<< "Deselect all items\n";
	*w << DisplayKeys(Key.SelectAlbum)		<< "Select songs of album around cursor\n";
	*w << DisplayKeys(Key.AddSelected)		<< "Add selected items to playlist/m3u file\n";
	*w << "\n";
	*w << DisplayKeys(Key.ToggleRepeat)		<< "Toggle repeat mode\n";
	*w << DisplayKeys(Key.ToggleRandom)		<< "Toggle random mode\n";
	*w << DisplayKeys(Key.ToggleSingle)		<< "Toggle single mode\n";
	*w << DisplayKeys(Key.ToggleConsume)		<< "Toggle consume mode\n";
	if (Mpd.Version() >= 16)
		*w << DisplayKeys(Key.ToggleReplayGainMode)	<< "Toggle replay gain mode\n";
	*w << DisplayKeys(Key.ToggleBitrateVisibility)	<< "Toggle bitrate visibility\n";
	*w << DisplayKeys(Key.Shuffle)			<< "Shuffle playlist\n";
	*w << DisplayKeys(Key.ToggleCrossfade)		<< "Toggle crossfade mode\n";
	*w << DisplayKeys(Key.SetCrossfade)		<< "Set crossfade\n";
	*w << DisplayKeys(Key.UpdateDB)			<< "Start a music database update\n";
	*w << "\n";
	*w << DisplayKeys(Key.ApplyFilter)		<< "Apply filter\n";
	*w << DisplayKeys(Key.FindForward)		<< "Forward find\n";
	*w << DisplayKeys(Key.FindBackward)		<< "Backward find\n";
	*w << DisplayKeys(Key.PrevFoundPosition)	<< "Go to previous found position\n";
	*w << DisplayKeys(Key.NextFoundPosition)	<< "Go to next found position\n";
	*w << DisplayKeys(Key.ToggleFindMode)		<< "Toggle find mode (normal/wrapped)\n";
	*w << DisplayKeys(Key.GoToContainingDir)	<< "Locate song in browser\n";
	*w << DisplayKeys(Key.GoToMediaLibrary)		<< "Locate current song in media library\n";
	*w << DisplayKeys(Key.ToggleScreenLock)		<< "Lock/unlock current screen\n";
#	ifdef HAVE_TAGLIB_H
	*w << DisplayKeys(Key.GoToTagEditor)		<< "Locate current song in tag editor\n";
#	endif // HAVE_TAGLIB_H
	*w << DisplayKeys(Key.ToggleDisplayMode)	<< "Toggle display mode\n";
	*w << DisplayKeys(Key.ToggleInterface)		<< "Toggle user interface\n";
	*w << DisplayKeys(Key.ToggleSeparatorsInPlaylist) << "Toggle displaying separators between albums\n";
	*w << DisplayKeys(Key.GoToPosition)		<< "Go to given position in current song (in % by default)\n";
	*w << DisplayKeys(Key.SongInfo)			<< "Show song info\n";
#	ifdef HAVE_CURL_CURL_H
	*w << DisplayKeys(Key.ArtistInfo)		<< "Show artist info\n";
	*w << DisplayKeys(Key.ToggleLyricsDB)		<< "Toggle lyrics database\n";
	*w << DisplayKeys(Key.ToggleFetchingLyricsInBackground) << "Toggle fetching lyrics for current song in background\n";
#	endif // HAVE_CURL_CURL_H
	*w << DisplayKeys(Key.Lyrics)			<< "Show/hide song's lyrics\n";
	*w << "\n";
	*w << DisplayKeys(Key.Quit)			<< "Quit\n";
	
	
	*w << "\n\n   " << fmtBold << "Keys - Playlist\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Enter)			<< "Play\n";
	*w << DisplayKeys(Key.SwitchTagTypeList)	<< "Add random songs/artists/albums to playlist\n";
	*w << DisplayKeys(Key.Delete)			<< "Delete item/selected items from playlist\n";
	*w << DisplayKeys(Key.Clear)			<< "Clear playlist\n";
	*w << DisplayKeys(Key.Crop)			<< "Clear playlist but hold currently playing/selected items\n";
	*w << DisplayKeys(Key.MvSongUp)			<< "Move item(s) up\n";
	*w << DisplayKeys(Key.MvSongDown)		<< "Move item(s) down\n";
	*w << DisplayKeys(Key.MoveTo)			<< "Move selected item(s) to cursor position\n";
	*w << DisplayKeys(Key.MoveBefore)		<< "Move selected item(s) before cursor position\n";
	*w << DisplayKeys(Key.MoveAfter)		<< "Move selected item(s) after cursor position\n";
	*w << DisplayKeys(Key.Add)			<< "Add url/file/directory to playlist\n";
#	ifdef HAVE_TAGLIB_H
	*w << DisplayKeys(Key.EditTags)			<< "Edit song's tags\n";
#	endif // HAVE_TAGLIB_H
	*w << DisplayKeys(Key.SavePlaylist)		<< "Save playlist\n";
	*w << DisplayKeys(Key.SortPlaylist)		<< "Sort/reverse playlist\n";
	*w << DisplayKeys(Key.GoToNowPlaying)		<< "Go to currently playing position\n";
	*w << DisplayKeys(Key.ToggleAutoCenter)		<< "Toggle auto center mode\n";
	
	
	*w << "\n\n   " << fmtBold << "Keys - Browser\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Enter)			<< "Enter directory/Add item to playlist and play\n";
	*w << DisplayKeys(Key.Space)			<< "Add item to playlist\n";
#	ifdef HAVE_TAGLIB_H
	*w << DisplayKeys(Key.EditTags)			<< "Edit song's tags/Rename playlist/directory\n";
#	else
	*w << DisplayKeys(Key.EditTags)			<< "Rename playlist/directory\n";
#	endif // HAVE_TAGLIB_H
	if (Mpd.GetHostname()[0] == '/') // are we connected to unix socket?
		*w << DisplayKeys(Key.Browser)		<< "Browse MPD database/local filesystem\n";
	*w << DisplayKeys(Key.SwitchTagTypeList)	<< "Toggle sort order\n";
	*w << DisplayKeys(Key.GoToNowPlaying)		<< "Locate currently playing song\n";
	*w << DisplayKeys(Key.GoToParentDir)		<< "Go to parent directory\n";
	*w << DisplayKeys(Key.Delete)			<< "Delete playlist/file/directory\n";
	*w << DisplayKeys(Key.GoToContainingDir)	<< "Jump to playlist editor (playlists only)\n";
	
	
	*w << "\n\n   " << fmtBold << "Keys - Search engine\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Enter)			<< "Add item to playlist and play/change option\n";
	*w << DisplayKeys(Key.Space)			<< "Add item to playlist\n";
#	ifdef HAVE_TAGLIB_H
	*w << DisplayKeys(Key.EditTags)			<< "Edit song's tags\n";
#	endif // HAVE_TAGLIB_H
	*w << DisplayKeys(Key.ToggleSingle)		<< "Start searching immediately\n";
	*w << DisplayKeys(Key.SearchEngine)		<< "Reset search engine\n";
	
	
	*w << "\n\n   " << fmtBold << "Keys - Media library\n -----------------------------------------\n" << fmtBoldEnd;
	if (!Config.media_library_disable_two_column_mode)
		*w << DisplayKeys(Key.MediaLibrary)	<< "Switch between two/three columns\n";
	*w << DisplayKeys(Key.PrevColumn)		<< "Previous column\n";
	*w << DisplayKeys(Key.NextColumn)		<< "Next column\n";
	*w << DisplayKeys(Key.Enter)			<< "Add to playlist and play song/album/artist's songs\n";
	*w << DisplayKeys(Key.Space)			<< "Add to playlist song/album/artist's songs\n";
#	ifdef HAVE_TAGLIB_H
	*w << DisplayKeys(Key.EditTags)			<< "Edit main tag/album/song's tags\n";
#	endif // HAVE_TAGLIB_H
	*w << DisplayKeys(Key.SwitchTagTypeList)	<< "Tag type list switcher (left column)\n";
	
	
	*w << "\n\n   " << fmtBold << "Keys - Playlist Editor\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.PrevColumn)		<< "Previous column\n";
	*w << DisplayKeys(Key.NextColumn)		<< "Next column\n";
	*w << DisplayKeys(Key.Enter)			<< "Add item to playlist and play\n";
	*w << DisplayKeys(Key.Space)			<< "Add to playlist/select item\n";
#	ifdef HAVE_TAGLIB_H
	*w << DisplayKeys(Key.EditTags)			<< "Edit playlist's name/song's tags\n";
#	else
	*w << DisplayKeys(Key.EditTags)			<< "Edit playlist's name\n";
#	endif // HAVE_TAGLIB_H
	*w << DisplayKeys(Key.MvSongUp)			<< "Move item(s) up\n";
	*w << DisplayKeys(Key.MvSongDown)		<< "Move item(s) down\n";
	*w << DisplayKeys(Key.Clear)			<< "Clear current playlist\n";
	
	
	*w << "\n\n   " << fmtBold << "Keys - Lyrics\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Space)			<< "Switch for following lyrics of now playing song\n";
	*w << DisplayKeys(Key.EditTags)			<< "Open lyrics in external editor\n";
	*w << DisplayKeys(Key.SwitchTagTypeList)	<< "Refetch lyrics\n";
	
	
	*w << "\n\n   " << fmtBold << "Keys - Artist info\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.SwitchTagTypeList)	<< "Refetch artist info\n";
	
	
#	ifdef HAVE_TAGLIB_H
	*w << "\n\n   " << fmtBold << "Keys - Tiny tag editor\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Enter)			<< "Edit tag\n";
	*w << DisplayKeys(Key.ToggleSingle)		<< "Save\n";
	
	
	*w << "\n\n   " << fmtBold << "Keys - Tag editor\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Enter)			<< "Change tag/filename for one song (left column)\n";
	*w << DisplayKeys(Key.Enter)			<< "Perform operation on all/selected songs (middle column)\n";
	*w << DisplayKeys(Key.Space)			<< "Switch to albums/directories view (left column)\n";
	*w << DisplayKeys(Key.Space)			<< "Select/deselect song (right column)\n";
	*w << DisplayKeys(Key.PrevColumn)		<< "Previous column\n";
	*w << DisplayKeys(Key.NextColumn)		<< "Next column\n";
	*w << DisplayKeys(Key.GoToParentDir)		<< "Go to parent directory (left column, directories view)\n";
#	endif // HAVE_TAGLIB_H
	
	
#	ifdef ENABLE_OUTPUTS
	*w << "\n\n   " << fmtBold << "Keys - Outputs\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Enter)			<< "Enable/disable output\n";
#	endif // ENABLE_OUTPUTS
	
	
#	if defined(ENABLE_VISUALIZER) && defined(HAVE_FFTW3_H)
	*w << "\n\n   " << fmtBold << "Keys - Music visualizer\n -----------------------------------------\n" << fmtBoldEnd;
	*w << DisplayKeys(Key.Space)			<< "Toggle visualization type\n";
#	endif // ENABLE_VISUALIZER && HAVE_FFTW3_H
	
	
	*w << "\n\n   " << fmtBold << "Mouse - Global\n -----------------------------------------\n" << fmtBoldEnd;
	*w << "\tLeft click on \"Playing/Paused\"	"	<< ": Play/pause\n";
	*w << "\tLeft click on progressbar	"		<< ": Go to chosen position in played track\n";
	*w << "\n";
	*w << "\tMouse wheel on \"Volume: xx\"	"		<< ": Change volume\n";
	*w << "\tMouse wheel on main window	"		<< ": Scroll\n";
	
	
	*w << "\n\n   " << fmtBold << "Mouse - Playlist\n -----------------------------------------\n" << fmtBoldEnd;
	*w << "\tLeft click			"		<< ": Highlight\n";
	*w << "\tRight click			"		<< ": Play\n";
	
	
	*w << "\n\n   " << fmtBold << "Mouse - Browser\n -----------------------------------------\n" << fmtBoldEnd;
	*w << "\tLeft click on directory		"	<< ": Enter directory\n";
	*w << "\tRight click on directory	"		<< ": Add to playlist\n";
	*w << "\n";
	*w << "\tLeft click on song/playlist	"		<< ": Add to playlist\n";
	*w << "\tRight click on song/playlist	"		<< ": Add to playlist and play\n";
	
	
	*w << "\n\n   " << fmtBold << "Mouse - Search engine\n -----------------------------------------\n" << fmtBoldEnd;
	*w << "\tLeft click			"		<< ": Highlight/switch value\n";
	*w << "\tRight click			"		<< ": Change value\n";
	
	
	*w << "\n\n   " << fmtBold << "Mouse - Media library\n -----------------------------------------\n" << fmtBoldEnd;
	*w << fmtBold << "\tLeft/middle column:\n" << fmtBoldEnd;
	*w << "\t\tLeft Click		"			<< ": Highlight\n";
	*w << "\t\tRight Click		"			<< ": Add to playlist\n";
	*w << "\n";
	*w << fmtBold << "\tRight column:\n" << fmtBoldEnd;
	*w << "\t\tLeft Click		"			<< ": Add to playlist\n";
	*w << "\t\tRight Click		"			<< ": Add to playlist and play\n";
	
	
	*w << "\n\n   " << fmtBold << "Mouse - Playlist editor\n -----------------------------------------\n" << fmtBoldEnd;
	*w << fmtBold << "\tLeft column:\n" << fmtBoldEnd;
	*w << "\t\tLeft Click		"			<< ": Highlight\n";
	*w << "\t\tRight Click		"			<< ": Add to playlist\n";
	*w << "\n";
	*w << fmtBold << "\tRight column:\n" << fmtBoldEnd;
	*w << "\t\tLeft Click		"			<< ": Add to playlist\n";
	*w << "\t\tRight Click		"			<< ": Add to playlist and play\n";
	
	
#	ifdef HAVE_TAGLIB_H
	*w << "\n\n   " << fmtBold << "Mouse - Tiny tag editor\n -----------------------------------------\n" << fmtBoldEnd;
	*w << "\tLeft click			"		<< ": Highlight\n";
	*w << "\tRight click			"		<< ": Change value/execute command\n";
	
	
	*w << "\n\n   " << fmtBold << "Mouse - Tag editor\n -----------------------------------------\n" << fmtBoldEnd;
	*w << fmtBold << "\tLeft column:\n" << fmtBoldEnd;
	*w << "\t\tLeft Click		"			<< ": Enter directory/highlight album\n";
	*w << "\t\tRight Click		"			<< ": Switch to directories/albums view\n";
	*w << "\n";
	*w << fmtBold << "\tMiddle column:\n" << fmtBoldEnd;
	*w << "\t\tLeft Click		"			<< ": Highlight\n";
	*w << "\t\tRight Click		"			<< ": Change value/execute command\n";
	*w << "\n";
	*w << fmtBold << "\tRight column:\n" << fmtBoldEnd;
	*w << "\t\tLeft Click		"			<< ": Highlight\n";
	*w << "\t\tRight Click		"			<< ": Change value\n";
#	endif // HAVE_TAGLIB_H
	
	
#	ifdef ENABLE_OUTPUTS
	*w << "\n\n   " << fmtBold << "Mouse - Outputs\n -----------------------------------------\n" << fmtBoldEnd;
	*w << "\tLeft click			"		<< ": Highlight\n";
	*w << "\tRight click			"		<< ": Enable/disable output\n";
#	endif // ENABLE_OUTPUTS
}

