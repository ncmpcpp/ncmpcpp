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

#ifndef HAVE_SETTINGS_H
#define HAVE_SETTINGS_H

#include <fstream>

#include "ncmpcpp.h"

const int null_key = 0x0fffffff;

struct ncmpcpp_keys
{
	int Up[2];
	int Down[2];
	int PageUp[2];
	int PageDown[2];
	int Home[2];
	int End[2];
	int Space[2];
	int Enter[2];
	int Delete[2];
	int VolumeUp[2];
	int VolumeDown[2];
	int ScreenSwitcher[2];
	int Help[2];
	int Playlist[2];
	int Browser[2];
	int SearchEngine[2];
	int MediaLibrary[2];
	int PlaylistEditor[2];
	int TagEditor[2];
	int Stop[2];
	int Pause[2];
	int Next[2];
	int Prev[2];
	int SeekForward[2];
	int SeekBackward[2];
	int ToggleRepeat[2];
	int ToggleRepeatOne[2];
	int ToggleRandom[2];
	int ToggleSpaceMode[2];
	int Shuffle[2];
	int ToggleCrossfade[2];
	int SetCrossfade[2];
	int UpdateDB[2];
	int FindForward[2];
	int FindBackward[2];
	int NextFoundPosition[2];
	int PrevFoundPosition[2];
	int ToggleFindMode[2];
	int EditTags[2];
	int SongInfo[2];
	int ArtistInfo[2];
	int GoToPosition[2];
	int Lyrics[2];
	int ReverseSelection[2];
	int DeselectAll[2];
	int AddSelected[2];
	int Clear[2];
	int Crop[2];
	int MvSongUp[2];
	int MvSongDown[2];
	int Add[2];
	int SavePlaylist[2];
	int GoToNowPlaying[2];
	int GoToContainingDir[2];
	int StartSearching[2];
	int ToggleAutoCenter[2];
	int TogglePlaylistDisplayMode[2];
	int GoToParentDir[2];
	int Quit[2];
};

struct ncmpcpp_config
{
	string mpd_music_dir;
	string song_list_format;
	string song_columns_list_format;
	string song_status_format;
	string song_window_title_format;
	string song_library_format;
	string tag_editor_album_format;
	string browser_playlist_prefix;
	
	string pattern;
	
	string selected_item_prefix;
	string selected_item_suffix;
	
	string color1;
	string color2;
	
	Color empty_tags_color;
	Color header_color;
	Color volume_color;
	Color state_line_color;
	Color state_flags_color;
	Color main_color;
	Color main_highlight_color;
	Color progressbar_color;
	Color statusbar_color;
	Color active_column_color;
	
	Border window_border;
	Border active_window_border;
	
	bool colors_enabled;
	bool fancy_scrolling;
	bool columns_in_playlist;
	bool set_window_title;
	bool header_visibility;
	bool statusbar_visibility;
	bool autocenter_mode;
	bool repeat_one_mode;
	bool wrapped_search;
	bool space_selects;
	bool albums_in_tag_editor;
	bool incremental_seeking;
	bool now_playing_lyrics;
	
	int mpd_connection_timeout;
	int crossfade_time;
	int playlist_disable_highlight_delay;
	int message_delay_time;
};

void DefaultKeys(ncmpcpp_keys &);
void DefaultConfiguration(ncmpcpp_config &);
void GetKeys(string, int *);
string GetLineValue(const string &, char = '"', char = '"');
string IntoStr(Color);
Color IntoColor(const string &);
void ReadKeys(ncmpcpp_keys &);
void ReadConfiguration(ncmpcpp_config &);

#endif

