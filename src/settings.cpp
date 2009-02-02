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

#include <sys/stat.h>
#include <fstream>

#include "settings.h"

const string config_file = config_dir + "config";
const string keys_config_file = config_dir + "keys";

using std::ifstream;

namespace
{
	void GetKeys(string &line, int *key)
	{
		size_t i = line.find("=")+1;
		line = line.substr(i, line.length()-i);
		i = 0;
		if (line[i] == ' ')
			while (line[++i] == ' ') { }
		line = line.substr(i, line.length()-i);
		i = line.find(" ");
		string one;
		string two;
		if (i != string::npos)
		{
			one = line.substr(0, i);
			i++;
			two = line.substr(i, line.length()-i);
		}
		else
			one = line;
		key[0] = !one.empty() && one[0] == '\'' ? one[1] : (atoi(one.c_str()) == 0 ? null_key : atoi(one.c_str()));
		key[1] = !two.empty() && two[0] == '\'' ? two[1] : (atoi(two.c_str()) == 0 ? null_key : atoi(two.c_str()));
	}
	
	void String2Buffer(const string &s, Buffer &buf)
	{
		for (string::const_iterator it = s.begin(); it != s.end(); it++)
		{
			if (*it != '$')
				buf << *it;
			else
				buf << Color(*++it-'0');
		}
	}
	
	Border IntoBorder(const string &color)
	{
		return (Border) IntoColor(color);
	}
}

Color IntoColor(const string &color)
{
	Color result = clDefault;
	
	if (color == "black")
		result = clBlack;
	else if (color == "red")
		result = clRed;
	else if (color == "green")
		result = clGreen;
	else if (color == "yellow")
		result = clYellow;
	else if (color == "blue")
		result = clBlue;
	else if (color == "magenta")
		result = clMagenta;
	else if (color == "cyan")
		result = clCyan;
	else if (color == "white")
		result = clWhite;
	
	return result;
}

void CreateConfigDir()
{
	mkdir(config_dir.c_str(), 0755);
}

void DefaultKeys(ncmpcpp_keys &keys)
{
	keys.Up[0] = KEY_UP;
	keys.Down[0] = KEY_DOWN;
	keys.PageUp[0] = KEY_PPAGE;
	keys.PageDown[0] = KEY_NPAGE;
	keys.Home[0] = KEY_HOME;
	keys.End[0] = KEY_END;
	keys.Space[0] = 32;
	keys.Enter[0] = 10;
	keys.Delete[0] = KEY_DC;
	keys.VolumeUp[0] = KEY_RIGHT;
	keys.VolumeDown[0] = KEY_LEFT;
	keys.ScreenSwitcher[0] = 9;
	keys.Help[0] = '1';
	keys.Playlist[0] = '2';
	keys.Browser[0] = '3';
	keys.SearchEngine[0] = '4';
	keys.MediaLibrary[0] = '5';
	keys.PlaylistEditor[0] = '6';
	keys.TagEditor[0] = '7';
	keys.Clock[0] = '0';
	keys.Stop[0] = 's';
	keys.Pause[0] = 'P';
	keys.Next[0] = '>';
	keys.Prev[0] = '<';
	keys.SeekForward[0] = 'f';
	keys.SeekBackward[0] = 'b';
	keys.ToggleRepeat[0] = 'r';
	keys.ToggleRepeatOne[0] = 'R';
	keys.ToggleRandom[0] = 'z';
	keys.ToggleSpaceMode[0] = 't';
	keys.ToggleAddMode[0] = 'T';
	keys.Shuffle[0] = 'Z';
	keys.ToggleCrossfade[0] = 'x';
	keys.SetCrossfade[0] = 'X';
	keys.UpdateDB[0] = 'u';
	keys.FindForward[0] = '/';
	keys.FindBackward[0] = '?';
	keys.NextFoundPosition[0] = '.';
	keys.PrevFoundPosition[0] = ',';
	keys.ToggleFindMode[0] = 'w';
	keys.EditTags[0] = 'e';
	keys.SongInfo[0] = 'i';
	keys.ArtistInfo[0] = 'I';
	keys.GoToPosition[0] = 'g';
	keys.Lyrics[0] = 'l';
	keys.ReverseSelection[0] = 'v';
	keys.DeselectAll[0] = 'V';
	keys.AddSelected[0] = 'A';
	keys.Clear[0] = 'c';
	keys.Crop[0] = 'C';
	keys.MvSongUp[0] = 'm';
	keys.MvSongDown[0] = 'n';
	keys.Add[0] = 'a';
	keys.SavePlaylist[0] = 'S';
	keys.GoToNowPlaying[0] = 'o';
	keys.GoToContainingDir[0] = 'G';
	keys.StartSearching[0] = 'y';
	keys.ToggleAutoCenter[0] = 'U';
	keys.ToggleDisplayMode[0] = 'p';
	keys.GoToParentDir[0] = 263;
	keys.SwitchTagTypeList[0] = '`';
	keys.Quit[0] = 'q';
	
	keys.Up[1] = 'k';
	keys.Down[1] = 'j';
	keys.PageUp[1] = null_key;
	keys.PageDown[1] = null_key;
	keys.Home[1] = null_key;
	keys.End[1] = null_key;
	keys.Space[1] = null_key;
	keys.Enter[1] = null_key;
	keys.Delete[1] = 'd';
	keys.VolumeUp[1] = '+';
	keys.VolumeDown[1] = '-';
	keys.ScreenSwitcher[1] = null_key;
	keys.Help[1] = 265;
	keys.Playlist[1] = 266;
	keys.Browser[1] = 267;
	keys.SearchEngine[1] = 268;
	keys.MediaLibrary[1] = 269;
	keys.PlaylistEditor[1] = 270;
	keys.TagEditor[1] = 271;
	keys.Clock[1] = 274;
	keys.Stop[1] = null_key;
	keys.Pause[1] = null_key;
	keys.Next[1] = null_key;
	keys.Prev[1] = null_key;
	keys.SeekForward[1] = null_key;
	keys.SeekBackward[1] = null_key;
	keys.ToggleRepeat[1] = null_key;
	keys.ToggleRepeatOne[1] = null_key;
	keys.ToggleRandom[1] = null_key;
	keys.ToggleSpaceMode[1] = null_key;
	keys.ToggleAddMode[1] = null_key;
	keys.Shuffle[1] = null_key;
	keys.ToggleCrossfade[1] = null_key;
	keys.SetCrossfade[1] = null_key;
	keys.UpdateDB[1] = null_key;
	keys.FindForward[1] = null_key;
	keys.FindBackward[1] = null_key;
	keys.NextFoundPosition[1] = null_key;
	keys.PrevFoundPosition[1] = null_key;
	keys.ToggleFindMode[1] = null_key;
	keys.EditTags[1] = null_key;
	keys.SongInfo[1] = null_key;
	keys.ArtistInfo[1] = null_key;
	keys.GoToPosition[1] = null_key;
	keys.Lyrics[1] = null_key;
	keys.ReverseSelection[1] = null_key;
	keys.DeselectAll[1] = null_key;
	keys.AddSelected[1] = null_key;
	keys.Clear[1] = null_key;
	keys.Crop[1] = null_key;
	keys.MvSongUp[1] = null_key;
	keys.MvSongDown[1] = null_key;
	keys.Add[1] = null_key;
	keys.SavePlaylist[1] = null_key;
	keys.GoToNowPlaying[1] = null_key;
	keys.GoToContainingDir[1] = null_key;
	keys.StartSearching[1] = null_key;
	keys.ToggleAutoCenter[1] = null_key;
	keys.ToggleDisplayMode[1] = null_key;
	keys.GoToParentDir[1] = 127;
	keys.SwitchTagTypeList[1] = null_key;
	keys.Quit[1] = 'Q';
}

void DefaultConfiguration(ncmpcpp_config &conf)
{
	conf.mpd_host = "localhost";
	conf.empty_tag = "<empty>";
	conf.song_list_format = "{%a - }{%t}|{$8%f$9}%r{$3(%l)$9}";
	conf.song_columns_list_format = "(7)[green]{l} (25)[cyan]{a} (40)[]{t} (30)[red]{b}";
	conf.song_status_format = "{(%l) }{%a - }{%t}|{%f}";
	conf.song_window_title_format = "{%a - }{%t}|{%f}";
	conf.song_library_format = "{%n - }{%t}|{%f}";
	conf.media_lib_album_format = "{(%y) }%b";
	conf.tag_editor_album_format = "{(%y) }%b";
	conf.browser_playlist_prefix << clRed << "(playlist)" << clEnd << ' ';
	conf.pattern = "%n - %t";
	conf.selected_item_prefix << clMagenta;
	conf.selected_item_suffix << clEnd;
	conf.color1 = clWhite;
	conf.color2 = clGreen;
	conf.empty_tags_color = clCyan;
	conf.header_color = clDefault;
	conf.volume_color = clDefault;
	conf.state_line_color = clDefault;
	conf.state_flags_color = clDefault;
	conf.main_color = clYellow;
	conf.main_highlight_color = conf.main_color;
	conf.progressbar_color = clDefault;
	conf.statusbar_color = clDefault;
	conf.active_column_color = clRed;
	conf.window_border = brGreen;
	conf.active_window_border = brRed;
	conf.media_lib_primary_tag = MPD_TAG_ITEM_ARTIST;
	conf.colors_enabled = true;
	conf.fancy_scrolling = true;
	conf.columns_in_playlist = false;
	conf.columns_in_browser = false;
	conf.columns_in_search_engine = false;
	conf.header_visibility = true;
	conf.statusbar_visibility = true;
	conf.autocenter_mode = false;
	conf.repeat_one_mode = false;
	conf.wrapped_search = true;
	conf.space_selects = false;
	conf.ncmpc_like_songs_adding = false;
	conf.albums_in_tag_editor = false;
	conf.incremental_seeking = true;
	conf.now_playing_lyrics = false;
	conf.local_browser = false;
	conf.search_in_db = true;
	conf.display_screens_numbers_on_start = true;
	conf.clock_display_seconds = false;
	conf.set_window_title = true;
	conf.mpd_port = 6600;
	conf.mpd_connection_timeout = 15;
	conf.crossfade_time = 5;
	conf.seek_time = 1;
	conf.playlist_disable_highlight_delay = 5;
	conf.message_delay_time = 4;
	conf.lyrics_db = 1;
}

string GetLineValue(string &line, char a, char b, bool once)
{
	int i = 0;
	int begin = -1, end = -1;
	for (string::iterator it = line.begin(); it != line.end() && (begin == -1 || end == -1); i++, it++)
	{
		if (*it == a || *it == b)
		{
			if (once)
				*it = 0;
			if (begin < 0)
				begin = i+1;
			else
				end = i;
		}
	}
	if (begin >= 0 && end >= 0)
		return line.substr(begin, end-begin);
	else
		return "";
}

string IntoStr(Color color)
{
	string result;
	
	if (color == clDefault)
		result = "default";
	else if (color == clBlack)
		result = "black";
	else if (color == clRed)
		result = "red";
	else if (color == clGreen)
		result = "green";
	else if (color == clYellow)
		result = "yellow";
	else if (color == clBlue)
		result = "blue";
	else if (color == clMagenta)
		result = "magenta";
	else if (color == clCyan)
		result = "cyan";
	else if (color == clWhite)
		result = "white";
	
	return result;
}

mpd_TagItems IntoTagItem(char c)
{
	switch (c)
	{
		case 'a':
			return MPD_TAG_ITEM_ARTIST;
		case 'y':
			return MPD_TAG_ITEM_DATE;
		case 'g':
			return MPD_TAG_ITEM_GENRE;
		case 'c':
			return MPD_TAG_ITEM_COMPOSER;
		case 'p':
			return MPD_TAG_ITEM_PERFORMER;
		default:
			return MPD_TAG_ITEM_ARTIST;
	}
}

void ReadKeys(ncmpcpp_keys &keys)
{
	ifstream f(keys_config_file.c_str());
	string key;
	
	if (!f.is_open())
		return;
	
	while (!f.eof())
	{
		getline(f, key);
		if (!key.empty() && key[0] != '#')
		{
			if (key.find("key_up ") != string::npos)
				GetKeys(key, keys.Up);
			else if (key.find("key_down ") != string::npos)
				GetKeys(key, keys.Down);
			else if (key.find("key_page_up ") != string::npos)
				GetKeys(key, keys.PageUp);
			else if (key.find("key_page_down ") != string::npos)
				GetKeys(key, keys.PageDown);
			else if (key.find("key_home ") != string::npos)
				GetKeys(key, keys.Home);
			else if (key.find("key_end ") != string::npos)
				GetKeys(key, keys.End);
			else if (key.find("key_space ") != string::npos)
				GetKeys(key, keys.Space);
			else if (key.find("key_enter ") != string::npos)
				GetKeys(key, keys.Enter);
			else if (key.find("key_delete ") != string::npos)
				GetKeys(key, keys.Delete);
			else if (key.find("key_volume_up ") != string::npos)
				GetKeys(key, keys.VolumeUp);
			else if (key.find("key_volume_down ") != string::npos)
				GetKeys(key, keys.VolumeDown);
			else if (key.find("key_screen_switcher ") != string::npos)
				GetKeys(key, keys.ScreenSwitcher);
			else if (key.find("key_help ") != string::npos)
				GetKeys(key, keys.Help);
			else if (key.find("key_playlist ") != string::npos)
				GetKeys(key, keys.Playlist);
			else if (key.find("key_browser ") != string::npos)
				GetKeys(key, keys.Browser);
			else if (key.find("key_search_engine ") != string::npos)
				GetKeys(key, keys.SearchEngine);
			else if (key.find("key_media_library ") != string::npos)
				GetKeys(key, keys.MediaLibrary);
			else if (key.find("key_playlist_editor ") != string::npos)
				GetKeys(key, keys.PlaylistEditor);
			else if (key.find("key_tag_editor ") != string::npos)
				GetKeys(key, keys.TagEditor);
			else if (key.find("key_clock ") != string::npos)
				GetKeys(key, keys.Clock);
			else if (key.find("key_stop ") != string::npos)
				GetKeys(key, keys.Stop);
			else if (key.find("key_pause ") != string::npos)
				GetKeys(key, keys.Pause);
			else if (key.find("key_next ") != string::npos)
				GetKeys(key, keys.Next);
			else if (key.find("key_prev ") != string::npos)
				GetKeys(key, keys.Prev);
			else if (key.find("key_seek_forward ") != string::npos)
				GetKeys(key, keys.SeekForward);
			else if (key.find("key_seek_backward ") != string::npos)
				GetKeys(key, keys.SeekBackward);
			else if (key.find("key_toggle_repeat ") != string::npos)
				GetKeys(key, keys.ToggleRepeat);
			else if (key.find("key_toggle_repeat_one ") != string::npos)
				GetKeys(key, keys.ToggleRepeatOne);
			else if (key.find("key_toggle_random ") != string::npos)
				GetKeys(key, keys.ToggleRandom);
			else if (key.find("key_toggle_space_mode ") != string::npos)
				GetKeys(key, keys.ToggleSpaceMode);
			else if (key.find("key_toggle_add_mode ") != string::npos)
				GetKeys(key, keys.ToggleAddMode);
			else if (key.find("key_shuffle ") != string::npos)
				GetKeys(key, keys.Shuffle);
			else if (key.find("key_toggle_crossfade ") != string::npos)
				GetKeys(key, keys.ToggleCrossfade);
			else if (key.find("key_set_crossfade ") != string::npos)
				GetKeys(key, keys.SetCrossfade);
			else if (key.find("key_update_db ") != string::npos)
				GetKeys(key, keys.UpdateDB);
			else if (key.find("key_find_forward ") != string::npos)
				GetKeys(key, keys.FindForward);
			else if (key.find("key_find_backward ") != string::npos)
				GetKeys(key, keys.FindBackward);
			else if (key.find("key_next_found_position ") != string::npos)
				GetKeys(key, keys.NextFoundPosition);
			else if (key.find("key_prev_found_position ") != string::npos)
				GetKeys(key, keys.PrevFoundPosition);
			else if (key.find("key_toggle_find_mode ") != string::npos)
				GetKeys(key, keys.ToggleFindMode);
			else if (key.find("key_edit_tags ") != string::npos)
				GetKeys(key, keys.EditTags);
			else if (key.find("key_go_to_position ") != string::npos)
				GetKeys(key, keys.GoToPosition);
			else if (key.find("key_song_info ") != string::npos)
				GetKeys(key, keys.SongInfo);
			else if (key.find("key_artist_info ") != string::npos)
				GetKeys(key, keys.ArtistInfo);
			else if (key.find("key_lyrics ") != string::npos)
				GetKeys(key, keys.Lyrics);
			else if (key.find("key_reverse_selection ") != string::npos)
				GetKeys(key, keys.ReverseSelection);
			else if (key.find("key_deselect_all ") != string::npos)
				GetKeys(key, keys.DeselectAll);
			else if (key.find("key_add_selected_items ") != string::npos)
				GetKeys(key, keys.AddSelected);
			else if (key.find("key_clear ") != string::npos)
				GetKeys(key, keys.Clear);
			else if (key.find("key_crop ") != string::npos)
				GetKeys(key, keys.Crop);
			else if (key.find("key_move_song_up ") != string::npos)
				GetKeys(key, keys.MvSongUp);
			else if (key.find("key_move_song_down ") != string::npos)
				GetKeys(key, keys.MvSongDown);
			else if (key.find("key_add ") != string::npos)
				GetKeys(key, keys.Add);
			else if (key.find("key_save_playlist ") != string::npos)
				GetKeys(key, keys.SavePlaylist);
			else if (key.find("key_go_to_now_playing ") != string::npos)
				GetKeys(key, keys.GoToNowPlaying);
			else if (key.find("key_toggle_auto_center ") != string::npos)
				GetKeys(key, keys.ToggleAutoCenter);
			else if (key.find("key_toggle_display_mode ") != string::npos)
				GetKeys(key, keys.ToggleDisplayMode);
			else if (key.find("key_go_to_containing_directory ") != string::npos)
				GetKeys(key, keys.GoToContainingDir);
			else if (key.find("key_start_searching ") != string::npos)
				GetKeys(key, keys.StartSearching);
			else if (key.find("key_go_to_parent_dir ") != string::npos)
				GetKeys(key, keys.GoToParentDir);
			else if (key.find("key_switch_tag_type_list ") != string::npos)
				GetKeys(key, keys.SwitchTagTypeList);
			else if (key.find("key_quit ") != string::npos)
				GetKeys(key, keys.Quit);
		}
	}
	f.close();
}

void ReadConfiguration(ncmpcpp_config &conf)
{
	ifstream f(config_file.c_str());
	string cl, v;
	
	if (!f.is_open())
		return;

	while (!f.eof())
	{
		getline(f, cl);
		if (!cl.empty() && cl[0] != '#')
		{
			v = GetLineValue(cl);
			if (cl.find("mpd_host") != string::npos)
			{
				if (!v.empty())
					conf.mpd_host = v;
			}
			else if (cl.find("mpd_music_dir") != string::npos)
			{
				if (!v.empty())
				{
					 // if ~ is used at the beginning, replace it with user's home folder
					if (v[0] == '~')
						v.replace(0, 1, home_folder);
					conf.mpd_music_dir = v + "/";
				}
			}
			else if (cl.find("mpd_port") != string::npos)
			{
				if (StrToInt(v))
					conf.mpd_port = StrToInt(v);
			}
			else if (cl.find("mpd_connection_timeout") != string::npos)
			{
				if (StrToInt(v))
					conf.mpd_connection_timeout = StrToInt(v);
			}
			else if (cl.find("mpd_crossfade_time") != string::npos)
			{
				if (StrToInt(v) > 0)
					conf.crossfade_time = StrToInt(v);
			}
			else if (cl.find("seek_time") != string::npos)
			{
				if (StrToInt(v) > 0)
					conf.seek_time = StrToInt(v);
			}
			else if (cl.find("playlist_disable_highlight_delay") != string::npos)
			{
				if (StrToInt(v) >= 0)
					conf.playlist_disable_highlight_delay = StrToInt(v);
			}
			else if (cl.find("message_delay_time") != string::npos)
			{
				if (StrToInt(v) > 0)
					conf.message_delay_time = StrToInt(v);
			}
			else if (cl.find("song_list_format") != string::npos)
			{
				if (!v.empty())
					conf.song_list_format = v;
			}
			else if (cl.find("song_columns_list_format") != string::npos)
			{
				if (!v.empty())
					conf.song_columns_list_format = v;
			}
			else if (cl.find("song_status_format") != string::npos)
			{
				if (!v.empty())
					conf.song_status_format = v;
			}
			else if (cl.find("song_library_format") != string::npos)
			{
				if (!v.empty())
					conf.song_library_format = v;
			}
			else if (cl.find("media_library_album_format") != string::npos)
			{
				if (!v.empty())
					conf.media_lib_album_format = v;
			}
			else if (cl.find("tag_editor_album_format") != string::npos)
			{
				if (!v.empty())
					conf.tag_editor_album_format = v;
			}
			else if (cl.find("browser_playlist_prefix") != string::npos)
			{
				if (!v.empty())
				{
					conf.browser_playlist_prefix.Clear();
					String2Buffer(v, conf.browser_playlist_prefix);
				}
			}
			else if (cl.find("default_tag_editor_pattern") != string::npos)
			{
				if (!v.empty())
					conf.pattern = v;
			}
			else if (cl.find("selected_item_prefix") != string::npos)
			{
				if (!v.empty())
				{
					conf.selected_item_prefix.Clear();
					String2Buffer(v, conf.selected_item_prefix);
				}
			}
			else if (cl.find("selected_item_suffix") != string::npos)
			{
				if (!v.empty())
				{
					conf.selected_item_suffix.Clear();
					String2Buffer(v, conf.selected_item_suffix);
				}
			}
			else if (cl.find("color1") != string::npos)
			{
				if (!v.empty())
					conf.color1 = IntoColor(v);
			}
			else if (cl.find("color2") != string::npos)
			{
				if (!v.empty())
					conf.color2 = IntoColor(v);
			}
			else if (cl.find("colors_enabled") != string::npos)
			{
				conf.colors_enabled = v == "yes";
			}
			else if (cl.find("fancy_scrolling") != string::npos)
			{
				conf.fancy_scrolling = v == "yes";
			}
			else if (cl.find("playlist_display_mode") != string::npos)
			{
				conf.columns_in_playlist = v == "columns";
			}
			else if (cl.find("browser_display_mode") != string::npos)
			{
				conf.columns_in_browser = v == "columns";
			}
			else if (cl.find("search_engine_display_mode") != string::npos)
			{
				conf.columns_in_search_engine = v == "columns";
			}
			else if (cl.find("header_visibility") != string::npos)
			{
				conf.header_visibility = v == "yes";
			}
			else if (cl.find("statusbar_visibility") != string::npos)
			{
				conf.statusbar_visibility = v == "yes";
			}
			else if (cl.find("autocenter_mode") != string::npos)
			{
				conf.autocenter_mode = v == "yes";
			}
			else if (cl.find("repeat_one_mode") != string::npos)
			{
				conf.repeat_one_mode = v == "yes";
			}
			else if (cl.find("default_find_mode") != string::npos)
			{
				conf.wrapped_search = v == "wrapped";
			}
			else if (cl.find("default_space_mode") != string::npos)
			{
				conf.space_selects = v == "select";
			}
			else if (cl.find("default_tag_editor_left_col") != string::npos)
			{
				conf.albums_in_tag_editor = v == "albums";
			}
			else if (cl.find("incremental_seeking") != string::npos)
			{
				conf.incremental_seeking = v == "yes";
			}
			else if (cl.find("follow_now_playing_lyrics") != string::npos)
			{
				conf.now_playing_lyrics = v == "yes";
			}
			else if (cl.find("ncmpc_like_songs_adding") != string::npos)
			{
				conf.ncmpc_like_songs_adding = v == "yes";
			}
			else if (cl.find("default_place_to_search_in") != string::npos)
			{
				conf.search_in_db = v == "database";
			}
			else if (cl.find("display_screens_numbers_on_start") != string::npos)
			{
				conf.display_screens_numbers_on_start = v == "yes";
			}
			else if (cl.find("clock_display_seconds") != string::npos)
			{
				conf.clock_display_seconds = v == "yes";
			}
			else if (cl.find("enable_window_title") != string::npos)
			{
				conf.set_window_title = v == "yes";
			}
			else if (cl.find("lyrics_database") != string::npos)
			{
				if (!v.empty())
					conf.lyrics_db = StrToInt(v);
			}
			else if (cl.find("song_window_title_format") != string::npos)
			{
				if (!v.empty())
					conf.song_window_title_format = v;
			}
			else if (cl.find("empty_tag_color") != string::npos)
			{
				if (!v.empty())
					conf.empty_tags_color = IntoColor(v);
			}
			else if (cl.find("header_window_color") != string::npos)
			{
				if (!v.empty())
					conf.header_color = IntoColor(v);
			}
			else if (cl.find("volume_color") != string::npos)
			{
				if (!v.empty())
					conf.volume_color = IntoColor(v);
			}
			else if (cl.find("state_line_color") != string::npos)
			{
				if (!v.empty())
					conf.state_line_color = IntoColor(v);
			}
			else if (cl.find("state_flags_color") != string::npos)
			{
				if (!v.empty())
					conf.state_flags_color = IntoColor(v);
			}
			else if (cl.find("main_window_color") != string::npos)
			{
				if (!v.empty())
					conf.main_color = IntoColor(v);
			}
			else if (cl.find("main_window_highlight_color") != string::npos)
			{
				if (!v.empty())
					conf.main_highlight_color = IntoColor(v);
			}
			else if (cl.find("progressbar_color") != string::npos)
			{
				if (!v.empty())
					conf.progressbar_color = IntoColor(v);
			}
			else if (cl.find("statusbar_color") != string::npos)
			{
				if (!v.empty())
					conf.statusbar_color = IntoColor(v);
			}
			else if (cl.find("active_column_color") != string::npos)
			{
				if (!v.empty())
					conf.active_column_color = IntoColor(v);
			}
			else if (cl.find("window_border_color ") != string::npos)
			{
				if (!v.empty())
					conf.window_border = IntoBorder(v);
			}
			else if (cl.find("active_window_border") != string::npos)
			{
				if (!v.empty())
					conf.active_window_border = IntoBorder(v);
			}
			else if (cl.find("media_library_left_column") != string::npos)
			{
				if (!v.empty())
					conf.media_lib_primary_tag = IntoTagItem(v[0]);
			}
		}
	}
	f.close();
}


