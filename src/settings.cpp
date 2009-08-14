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

#ifdef WIN32
# include <io.h>
#else
# include <sys/stat.h>
#endif // WIN32
#include <fstream>
#include <stdexcept>

#include "global.h"
#include "helpers.h"
#include "lyrics.h"
#include "settings.h"

const std::string config_file = config_dir + "config";
const std::string keys_config_file = config_dir + "keys";

ncmpcpp_config Config;
ncmpcpp_keys Key;

namespace
{
	void GetKeys(std::string &line, int *key)
	{
		size_t i = line.find("=")+1;
		line = line.substr(i, line.length()-i);
		i = 0;
		if (line[i] == ' ')
			while (line[++i] == ' ') { }
		line = line.substr(i, line.length()-i);
		i = line.find(" ");
		std::string one;
		std::string two;
		if (i != std::string::npos)
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
	
	void ValidateSongFormat(const std::string &type, const std::string &s)
	{
		int braces = 0;
		for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
		{
			if (*it == '{')
				++braces;
			else if (*it == '}')
				--braces;
		}
		if (braces)
			throw std::runtime_error(type + ": number of opening and closing braces does not equal!");
	}
	
	Border IntoBorder(const std::string &color)
	{
		return Border(IntoColor(color));
	}
}

void CreateConfigDir()
{
	mkdir(config_dir.c_str()
#	ifndef WIN32
	, 0755
#	endif // !WIN32
	);
}

void SetWindowsDimensions(size_t &header_height, size_t &footer_start_y, size_t &footer_height)
{
	Global::MainStartY = Config.new_design ? 5 : 2;
	Global::MainHeight = LINES-(Config.new_design ? 7 : 4);
	
	if (!Config.header_visibility)
	{
		Global::MainStartY -= 2;
		Global::MainHeight += 2;
	}
	if (!Config.statusbar_visibility)
		Global::MainHeight++;
	
	header_height = Config.new_design ? 5 : 1;
	footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
	footer_height = Config.statusbar_visibility ? 2 : 1;
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
	keys.Outputs[0] = '8';
	keys.Clock[0] = '0';
	keys.Stop[0] = 's';
	keys.Pause[0] = 'P';
	keys.Next[0] = '>';
	keys.Prev[0] = '<';
	keys.SeekForward[0] = 'f';
	keys.SeekBackward[0] = 'b';
	keys.ToggleRepeat[0] = 'r';
	keys.ToggleRandom[0] = 'z';
	keys.ToggleSingle[0] = 'y';
	keys.ToggleConsume[0] = 'R';
	keys.ToggleSpaceMode[0] = 't';
	keys.ToggleAddMode[0] = 'T';
	keys.ToggleMouse[0] = '|';
	keys.Shuffle[0] = 'Z';
	keys.ToggleCrossfade[0] = 'x';
	keys.SetCrossfade[0] = 'X';
	keys.UpdateDB[0] = 'u';
	keys.SortPlaylist[0] = 22;
	keys.ApplyFilter[0] = 6;
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
	keys.MoveTo[0] = 'M';
	keys.Add[0] = 'a';
	keys.SavePlaylist[0] = 'S';
	keys.GoToNowPlaying[0] = 'o';
	keys.GoToContainingDir[0] = 'G';
	keys.ToggleAutoCenter[0] = 'U';
	keys.ToggleDisplayMode[0] = 'p';
	keys.ToggleInterface[0] = '\\';
	keys.ToggleLyricsDB[0] = 'L';
	keys.GoToParentDir[0] = KEY_BACKSPACE;
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
	keys.Outputs[1] = 272;
	keys.Clock[1] = 274;
	keys.Stop[1] = null_key;
	keys.Pause[1] = null_key;
	keys.Next[1] = null_key;
	keys.Prev[1] = null_key;
	keys.SeekForward[1] = null_key;
	keys.SeekBackward[1] = null_key;
	keys.ToggleRepeat[1] = null_key;
	keys.ToggleRandom[1] = null_key;
	keys.ToggleSingle[1] = null_key;
	keys.ToggleConsume[1] = null_key;
	keys.ToggleSpaceMode[1] = null_key;
	keys.ToggleAddMode[1] = null_key;
	keys.ToggleMouse[1] = null_key;
	keys.Shuffle[1] = null_key;
	keys.ToggleCrossfade[1] = null_key;
	keys.SetCrossfade[1] = null_key;
	keys.UpdateDB[1] = null_key;
	keys.SortPlaylist[1] = null_key;
	keys.ApplyFilter[1] = null_key;
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
	keys.MoveTo[1] = null_key;
	keys.Add[1] = null_key;
	keys.SavePlaylist[1] = null_key;
	keys.GoToNowPlaying[1] = null_key;
	keys.GoToContainingDir[1] = null_key;
	keys.ToggleAutoCenter[1] = null_key;
	keys.ToggleDisplayMode[1] = null_key;
	keys.ToggleInterface[1] = null_key;
	keys.ToggleLyricsDB[1] = null_key;
	keys.GoToParentDir[1] = 127;
	keys.SwitchTagTypeList[1] = null_key;
	keys.Quit[1] = 'Q';
}

void DefaultConfiguration(ncmpcpp_config &conf)
{
	conf.mpd_host = "localhost";
	conf.empty_tag = "<empty>";
	conf.song_list_columns_format = "(7f)[green]{l} (25)[cyan]{a} (40)[]{t} (30)[red]{b}";
	conf.song_list_format = "{{%a - }{%t}|{$8%f$9}$R{$3(%l)$9}}";
	conf.song_status_format = "{{(%l) }{%a - }{%t}|{%f}}";
	conf.song_window_title_format = "{{%a - }{%t}|{%f}}";
	conf.song_library_format = "{{%n - }{%t}|{%f}}";
	conf.tag_editor_album_format = "{{(%y) }%b}";
	conf.new_header_first_line = "{$b$1$aqqu$/a$9 {%t}|{%f} $1$atqq$/a$9$/b}";
	conf.new_header_second_line = "{{{$4$b%a$/b$9}{ - $7%b$9}{ ($4%y$9)}}|{%D}}";
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
	conf.playlist_show_remaining_time = false;
	conf.columns_in_playlist = false;
	conf.columns_in_browser = false;
	conf.columns_in_search_engine = false;
	conf.header_visibility = true;
	conf.header_text_scrolling = true;
	conf.statusbar_visibility = true;
	conf.autocenter_mode = false;
	conf.wrapped_search = true;
	conf.space_selects = false;
	conf.ncmpc_like_songs_adding = false;
	conf.albums_in_tag_editor = false;
	conf.incremental_seeking = true;
	conf.now_playing_lyrics = false;
	conf.local_browser = false;
	conf.local_browser_show_hidden_files = false;
	conf.search_in_db = true;
	conf.display_screens_numbers_on_start = true;
	conf.clock_display_seconds = false;
	conf.ignore_leading_the = false;
	conf.block_search_constraints_change = true;
	conf.use_console_editor = false;
	conf.use_cyclic_scrolling = false;
	conf.allow_physical_files_deletion = false;
	conf.allow_physical_directories_deletion = false;
	conf.mouse_support = true;
	conf.new_design = false;
	conf.set_window_title = true;
	conf.mpd_port = 6600;
	conf.mpd_connection_timeout = 15;
	conf.crossfade_time = 5;
	conf.seek_time = 1;
	conf.playlist_disable_highlight_delay = 5;
	conf.message_delay_time = 4;
	conf.lyrics_db = 0;
	conf.regex_type = 0;
	conf.lines_scrolled = 2;
}

void ReadKeys(ncmpcpp_keys &keys)
{
	std::ifstream f(keys_config_file.c_str());
	std::string key;
	
	if (!f.is_open())
		return;
	
	while (!f.eof())
	{
		getline(f, key);
		if (!key.empty() && key[0] != '#')
		{
			if (key.find("key_up ") != std::string::npos)
				GetKeys(key, keys.Up);
			else if (key.find("key_down ") != std::string::npos)
				GetKeys(key, keys.Down);
			else if (key.find("key_page_up ") != std::string::npos)
				GetKeys(key, keys.PageUp);
			else if (key.find("key_page_down ") != std::string::npos)
				GetKeys(key, keys.PageDown);
			else if (key.find("key_home ") != std::string::npos)
				GetKeys(key, keys.Home);
			else if (key.find("key_end ") != std::string::npos)
				GetKeys(key, keys.End);
			else if (key.find("key_space ") != std::string::npos)
				GetKeys(key, keys.Space);
			else if (key.find("key_enter ") != std::string::npos)
				GetKeys(key, keys.Enter);
			else if (key.find("key_delete ") != std::string::npos)
				GetKeys(key, keys.Delete);
			else if (key.find("key_volume_up ") != std::string::npos)
				GetKeys(key, keys.VolumeUp);
			else if (key.find("key_volume_down ") != std::string::npos)
				GetKeys(key, keys.VolumeDown);
			else if (key.find("key_screen_switcher ") != std::string::npos)
				GetKeys(key, keys.ScreenSwitcher);
			else if (key.find("key_help ") != std::string::npos)
				GetKeys(key, keys.Help);
			else if (key.find("key_playlist ") != std::string::npos)
				GetKeys(key, keys.Playlist);
			else if (key.find("key_browser ") != std::string::npos)
				GetKeys(key, keys.Browser);
			else if (key.find("key_search_engine ") != std::string::npos)
				GetKeys(key, keys.SearchEngine);
			else if (key.find("key_media_library ") != std::string::npos)
				GetKeys(key, keys.MediaLibrary);
			else if (key.find("key_playlist_editor ") != std::string::npos)
				GetKeys(key, keys.PlaylistEditor);
			else if (key.find("key_tag_editor ") != std::string::npos)
				GetKeys(key, keys.TagEditor);
			else if (key.find("key_outputs ") != std::string::npos)
				GetKeys(key, keys.Outputs);
			else if (key.find("key_clock ") != std::string::npos)
				GetKeys(key, keys.Clock);
			else if (key.find("key_stop ") != std::string::npos)
				GetKeys(key, keys.Stop);
			else if (key.find("key_pause ") != std::string::npos)
				GetKeys(key, keys.Pause);
			else if (key.find("key_next ") != std::string::npos)
				GetKeys(key, keys.Next);
			else if (key.find("key_prev ") != std::string::npos)
				GetKeys(key, keys.Prev);
			else if (key.find("key_seek_forward ") != std::string::npos)
				GetKeys(key, keys.SeekForward);
			else if (key.find("key_seek_backward ") != std::string::npos)
				GetKeys(key, keys.SeekBackward);
			else if (key.find("key_toggle_repeat ") != std::string::npos)
				GetKeys(key, keys.ToggleRepeat);
			else if (key.find("key_toggle_random ") != std::string::npos)
				GetKeys(key, keys.ToggleRandom);
			else if (key.find("key_toggle_single ") != std::string::npos)
				GetKeys(key, keys.ToggleSingle);
			else if (key.find("key_toggle_consume ") != std::string::npos)
				GetKeys(key, keys.ToggleConsume);
			else if (key.find("key_toggle_space_mode ") != std::string::npos)
				GetKeys(key, keys.ToggleSpaceMode);
			else if (key.find("key_toggle_add_mode ") != std::string::npos)
				GetKeys(key, keys.ToggleAddMode);
			else if (key.find("key_toggle_mouse ") != std::string::npos)
				GetKeys(key, keys.ToggleMouse);
			else if (key.find("key_shuffle ") != std::string::npos)
				GetKeys(key, keys.Shuffle);
			else if (key.find("key_toggle_crossfade ") != std::string::npos)
				GetKeys(key, keys.ToggleCrossfade);
			else if (key.find("key_set_crossfade ") != std::string::npos)
				GetKeys(key, keys.SetCrossfade);
			else if (key.find("key_update_db ") != std::string::npos)
				GetKeys(key, keys.UpdateDB);
			else if (key.find("key_sort_playlist ") != std::string::npos)
				GetKeys(key, keys.SortPlaylist);
			else if (key.find("key_apply_filter ") != std::string::npos)
				GetKeys(key, keys.ApplyFilter);
			else if (key.find("key_find_forward ") != std::string::npos)
				GetKeys(key, keys.FindForward);
			else if (key.find("key_find_backward ") != std::string::npos)
				GetKeys(key, keys.FindBackward);
			else if (key.find("key_next_found_position ") != std::string::npos)
				GetKeys(key, keys.NextFoundPosition);
			else if (key.find("key_prev_found_position ") != std::string::npos)
				GetKeys(key, keys.PrevFoundPosition);
			else if (key.find("key_toggle_find_mode ") != std::string::npos)
				GetKeys(key, keys.ToggleFindMode);
			else if (key.find("key_edit_tags ") != std::string::npos)
				GetKeys(key, keys.EditTags);
			else if (key.find("key_go_to_position ") != std::string::npos)
				GetKeys(key, keys.GoToPosition);
			else if (key.find("key_song_info ") != std::string::npos)
				GetKeys(key, keys.SongInfo);
			else if (key.find("key_artist_info ") != std::string::npos)
				GetKeys(key, keys.ArtistInfo);
			else if (key.find("key_lyrics ") != std::string::npos)
				GetKeys(key, keys.Lyrics);
			else if (key.find("key_reverse_selection ") != std::string::npos)
				GetKeys(key, keys.ReverseSelection);
			else if (key.find("key_deselect_all ") != std::string::npos)
				GetKeys(key, keys.DeselectAll);
			else if (key.find("key_add_selected_items ") != std::string::npos)
				GetKeys(key, keys.AddSelected);
			else if (key.find("key_clear ") != std::string::npos)
				GetKeys(key, keys.Clear);
			else if (key.find("key_crop ") != std::string::npos)
				GetKeys(key, keys.Crop);
			else if (key.find("key_move_song_up ") != std::string::npos)
				GetKeys(key, keys.MvSongUp);
			else if (key.find("key_move_song_down ") != std::string::npos)
				GetKeys(key, keys.MvSongDown);
			else if (key.find("key_move_to ") != std::string::npos)
				GetKeys(key, keys.MoveTo);
			else if (key.find("key_add ") != std::string::npos)
				GetKeys(key, keys.Add);
			else if (key.find("key_save_playlist ") != std::string::npos)
				GetKeys(key, keys.SavePlaylist);
			else if (key.find("key_go_to_now_playing ") != std::string::npos)
				GetKeys(key, keys.GoToNowPlaying);
			else if (key.find("key_toggle_auto_center ") != std::string::npos)
				GetKeys(key, keys.ToggleAutoCenter);
			else if (key.find("key_toggle_display_mode ") != std::string::npos)
				GetKeys(key, keys.ToggleDisplayMode);
			else if (key.find("key_toggle_lyrics_db ") != std::string::npos)
				GetKeys(key, keys.ToggleLyricsDB);
			else if (key.find("key_go_to_containing_directory ") != std::string::npos)
				GetKeys(key, keys.GoToContainingDir);
			else if (key.find("key_go_to_parent_dir ") != std::string::npos)
				GetKeys(key, keys.GoToParentDir);
			else if (key.find("key_switch_tag_type_list ") != std::string::npos)
				GetKeys(key, keys.SwitchTagTypeList);
			else if (key.find("key_quit ") != std::string::npos)
				GetKeys(key, keys.Quit);
		}
	}
	f.close();
}

void ReadConfiguration(ncmpcpp_config &conf)
{
	std::ifstream f(config_file.c_str());
	std::string cl, v;
	
	while (f.is_open() && !f.eof())
	{
		getline(f, cl);
		if (!cl.empty() && cl[0] != '#')
		{
			v = GetLineValue(cl);
			if (cl.find("mpd_host") != std::string::npos)
			{
				if (!v.empty())
					conf.mpd_host = v;
			}
			else if (cl.find("mpd_music_dir") != std::string::npos)
			{
				if (!v.empty())
				{
					 // if ~ is used at the beginning, replace it with user's home folder
					if (v[0] == '~')
						v.replace(0, 1, home_path);
					conf.mpd_music_dir = v + "/";
				}
			}
			else if (cl.find("mpd_port") != std::string::npos)
			{
				if (StrToInt(v))
					conf.mpd_port = StrToInt(v);
			}
			else if (cl.find("mpd_connection_timeout") != std::string::npos)
			{
				if (StrToInt(v))
					conf.mpd_connection_timeout = StrToInt(v);
			}
			else if (cl.find("mpd_crossfade_time") != std::string::npos)
			{
				if (StrToInt(v) > 0)
					conf.crossfade_time = StrToInt(v);
			}
			else if (cl.find("seek_time") != std::string::npos)
			{
				if (StrToInt(v) > 0)
					conf.seek_time = StrToInt(v);
			}
			else if (cl.find("playlist_disable_highlight_delay") != std::string::npos)
			{
				if (StrToInt(v) >= 0)
					conf.playlist_disable_highlight_delay = StrToInt(v);
			}
			else if (cl.find("message_delay_time") != std::string::npos)
			{
				if (StrToInt(v) > 0)
					conf.message_delay_time = StrToInt(v);
			}
			else if (cl.find("song_list_format") != std::string::npos)
			{
				if (!v.empty())
				{
					ValidateSongFormat("song_list_format", v);
					conf.song_list_format = '{';
					conf.song_list_format += v;
					conf.song_list_format += '}';
				}
			}
			else if (cl.find("song_columns_list_format") != std::string::npos)
			{
				if (!v.empty())
					conf.song_list_columns_format = v;
			}
			else if (cl.find("song_status_format") != std::string::npos)
			{
				if (!v.empty())
				{
					ValidateSongFormat("song_status_format", v);
					conf.song_status_format = '{';
					conf.song_status_format += v;
					conf.song_status_format += '}';
				}
			}
			else if (cl.find("song_library_format") != std::string::npos)
			{
				if (!v.empty())
				{
					ValidateSongFormat("song_library_format", v);
					conf.song_library_format = '{';
					conf.song_library_format += v;
					conf.song_library_format += '}';
				}
			}
			else if (cl.find("tag_editor_album_format") != std::string::npos)
			{
				if (!v.empty())
				{
					ValidateSongFormat("tag_editor_album_format", v);
					conf.tag_editor_album_format = '{';
					conf.tag_editor_album_format += v;
					conf.tag_editor_album_format += '}';
				}
			}
			else if (cl.find("external_editor") != std::string::npos)
			{
				if (!v.empty())
					conf.external_editor = v;
			}
			else if (cl.find("system_encoding") != std::string::npos)
			{
				if (!v.empty())
					conf.system_encoding = v + "//TRANSLIT";
			}
			else if (cl.find("execute_on_song_change") != std::string::npos)
			{
				if (!v.empty())
					conf.execute_on_song_change = v;
			}
			else if (cl.find("alternative_header_first_line_format") != std::string::npos)
			{
				if (!v.empty())
				{
					ValidateSongFormat("alternative_header_first_line_format", v);
					conf.new_header_first_line = '{';
					conf.new_header_first_line += v;
					conf.new_header_first_line += '}';
				}
			}
			else if (cl.find("alternative_header_second_line_format") != std::string::npos)
			{
				if (!v.empty())
				{
					ValidateSongFormat("alternative_header_second_line_format", v);
					conf.new_header_second_line = '{';
					conf.new_header_second_line += v;
					conf.new_header_second_line += '}';
				}
			}
			else if (cl.find("browser_playlist_prefix") != std::string::npos)
			{
				if (!v.empty())
				{
					conf.browser_playlist_prefix.Clear();
					String2Buffer(v, conf.browser_playlist_prefix);
				}
			}
			else if (cl.find("default_tag_editor_pattern") != std::string::npos)
			{
				if (!v.empty())
					conf.pattern = v;
			}
			else if (cl.find("selected_item_prefix") != std::string::npos)
			{
				if (!v.empty())
				{
					conf.selected_item_prefix.Clear();
					String2Buffer(v, conf.selected_item_prefix);
				}
			}
			else if (cl.find("selected_item_suffix") != std::string::npos)
			{
				if (!v.empty())
				{
					conf.selected_item_suffix.Clear();
					String2Buffer(v, conf.selected_item_suffix);
				}
			}
			else if (cl.find("color1") != std::string::npos)
			{
				if (!v.empty())
					conf.color1 = IntoColor(v);
			}
			else if (cl.find("color2") != std::string::npos)
			{
				if (!v.empty())
					conf.color2 = IntoColor(v);
			}
			else if (cl.find("colors_enabled") != std::string::npos)
			{
				conf.colors_enabled = v == "yes";
			}
			else if (cl.find("fancy_scrolling") != std::string::npos)
			{
				conf.fancy_scrolling = v == "yes";
			}
			else if (cl.find("cyclic_scrolling") != std::string::npos)
			{
				conf.use_cyclic_scrolling = v == "yes";
			}
			else if (cl.find("playlist_show_remaining_time") != std::string::npos)
			{
				conf.playlist_show_remaining_time = v == "yes";
			}
			else if (cl.find("playlist_display_mode") != std::string::npos)
			{
				conf.columns_in_playlist = v == "columns";
			}
			else if (cl.find("browser_display_mode") != std::string::npos)
			{
				conf.columns_in_browser = v == "columns";
			}
			else if (cl.find("search_engine_display_mode") != std::string::npos)
			{
				conf.columns_in_search_engine = v == "columns";
			}
			else if (cl.find("header_visibility") != std::string::npos)
			{
				conf.header_visibility = v == "yes";
			}
			else if (cl.find("header_text_scrolling") != std::string::npos)
			{
				conf.header_text_scrolling = v == "yes";
			}
			else if (cl.find("statusbar_visibility") != std::string::npos)
			{
				conf.statusbar_visibility = v == "yes";
			}
			else if (cl.find("autocenter_mode") != std::string::npos)
			{
				conf.autocenter_mode = v == "yes";
			}
			else if (cl.find("default_find_mode") != std::string::npos)
			{
				conf.wrapped_search = v == "wrapped";
			}
			else if (cl.find("default_space_mode") != std::string::npos)
			{
				conf.space_selects = v == "select";
			}
			else if (cl.find("default_tag_editor_left_col") != std::string::npos)
			{
				conf.albums_in_tag_editor = v == "albums";
			}
			else if (cl.find("incremental_seeking") != std::string::npos)
			{
				conf.incremental_seeking = v == "yes";
			}
			else if (cl.find("show_hidden_files_in_local_browser") != std::string::npos)
			{
				conf.local_browser_show_hidden_files = v == "yes";
			}
			else if (cl.find("follow_now_playing_lyrics") != std::string::npos)
			{
				conf.now_playing_lyrics = v == "yes";
			}
			else if (cl.find("ncmpc_like_songs_adding") != std::string::npos)
			{
				conf.ncmpc_like_songs_adding = v == "yes";
			}
			else if (cl.find("default_place_to_search_in") != std::string::npos)
			{
				conf.search_in_db = v == "database";
			}
			else if (cl.find("display_screens_numbers_on_start") != std::string::npos)
			{
				conf.display_screens_numbers_on_start = v == "yes";
			}
			else if (cl.find("clock_display_seconds") != std::string::npos)
			{
				conf.clock_display_seconds = v == "yes";
			}
			else if (cl.find("ignore_leading_the") != std::string::npos)
			{
				conf.ignore_leading_the = v == "yes";
			}
			else if (cl.find("use_console_editor") != std::string::npos)
			{
				conf.use_console_editor = v == "yes";
			}
			else if (cl.find("block_search_constraints_change_if_items_found") != std::string::npos)
			{
				conf.block_search_constraints_change = v == "yes";
			}
			else if (cl.find("allow_physical_files_deletion") != std::string::npos)
			{
				conf.allow_physical_files_deletion = v == "yes";
			}
			else if (cl.find("allow_physical_directories_deletion") != std::string::npos)
			{
				conf.allow_physical_directories_deletion = v == "yes";
			}
			else if (cl.find("mouse_support") != std::string::npos)
			{
				conf.mouse_support = v == "yes";
			}
			else if (cl.find("user_interface") != std::string::npos)
			{
				conf.new_design = v == "alternative";
			}
			else if (cl.find("enable_window_title") != std::string::npos)
			{
				conf.set_window_title = v == "yes";
			}
			else if (cl.find("regular_expressions") != std::string::npos)
			{
				conf.regex_type = REG_EXTENDED * (v != "basic");
			}
			else if (cl.find("lyrics_database") != std::string::npos)
			{
				if (!v.empty())
				{
					unsigned n = StrToInt(v)-1;
					conf.lyrics_db = n < Lyrics::DBs ? n : 0;
				}
			}
			else if (cl.find("lines_scrolled") != std::string::npos)
			{
				if (!v.empty())
					conf.lines_scrolled = StrToInt(v);
			}
			else if (cl.find("song_window_title_format") != std::string::npos)
			{
				if (!v.empty())
				{
					ValidateSongFormat("song_window_title_format", v);
					conf.song_window_title_format = '{';
					conf.song_window_title_format += v;
					conf.song_window_title_format += '}';
				}
			}
			else if (cl.find("empty_tag") != std::string::npos)
			{
				conf.empty_tag = v; // is this case empty string is allowed
			}
			else if (cl.find("empty_tag_color") != std::string::npos)
			{
				if (!v.empty())
					conf.empty_tags_color = IntoColor(v);
			}
			else if (cl.find("header_window_color") != std::string::npos)
			{
				if (!v.empty())
					conf.header_color = IntoColor(v);
			}
			else if (cl.find("volume_color") != std::string::npos)
			{
				if (!v.empty())
					conf.volume_color = IntoColor(v);
			}
			else if (cl.find("state_line_color") != std::string::npos)
			{
				if (!v.empty())
					conf.state_line_color = IntoColor(v);
			}
			else if (cl.find("state_flags_color") != std::string::npos)
			{
				if (!v.empty())
					conf.state_flags_color = IntoColor(v);
			}
			else if (cl.find("main_window_color") != std::string::npos)
			{
				if (!v.empty())
					conf.main_color = IntoColor(v);
			}
			else if (cl.find("main_window_highlight_color") != std::string::npos)
			{
				if (!v.empty())
					conf.main_highlight_color = IntoColor(v);
			}
			else if (cl.find("progressbar_color") != std::string::npos)
			{
				if (!v.empty())
					conf.progressbar_color = IntoColor(v);
			}
			else if (cl.find("statusbar_color") != std::string::npos)
			{
				if (!v.empty())
					conf.statusbar_color = IntoColor(v);
			}
			else if (cl.find("active_column_color") != std::string::npos)
			{
				if (!v.empty())
					conf.active_column_color = IntoColor(v);
			}
			else if (cl.find("window_border_color ") != std::string::npos)
			{
				if (!v.empty())
					conf.window_border = IntoBorder(v);
			}
			else if (cl.find("active_window_border") != std::string::npos)
			{
				if (!v.empty())
					conf.active_window_border = IntoBorder(v);
			}
			else if (cl.find("media_library_left_column") != std::string::npos)
			{
				if (!v.empty())
					conf.media_lib_primary_tag = IntoTagItem(v[0]);
			}
		}
	}
	f.close();
	
	for (std::string width = GetLineValue(conf.song_list_columns_format, '(', ')', 1); !width.empty(); width = GetLineValue(conf.song_list_columns_format, '(', ')', 1))
	{
		Column col;
		col.color = IntoColor(GetLineValue(conf.song_list_columns_format, '[', ']', 1));
		std::string tag_type = GetLineValue(conf.song_list_columns_format, '{', '}', 1);
		col.type = tag_type.at(0);
		col.fixed = *width.rbegin() == 'f';
		col.right_alignment = tag_type.length() > 1 && tag_type[1] == 'r';
		col.width = StrToInt(width);
		conf.columns.push_back(col);
	}
}

