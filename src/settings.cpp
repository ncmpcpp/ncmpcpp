/***************************************************************************
 *   Copyright (C) 2008-2011 by Andrzej Rybczak                            *
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
#include <iostream>
#include <stdexcept>

#include "browser.h"
#include "clock.h"
#include "global.h"
#include "help.h"
#include "helpers.h"
#include "lyrics.h"
#include "media_library.h"
#include "outputs.h"
#include "playlist.h"
#include "playlist_editor.h"
#include "search_engine.h"
#include "settings.h"
#include "tag_editor.h"
#include "visualizer.h"

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

const std::string config_file = config_dir + "config";
const std::string keys_config_file = config_dir + "keys";

NcmpcppConfig Config;
NcmpcppKeys Key;

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
		key[0] = !one.empty() && one[0] == '\''
				? one[1]
				: (atoi(one.c_str()) == 0 ? NcmpcppKeys::NullKey : atoi(one.c_str()));
		
		key[1] = !two.empty() && two[0] == '\''
				? two[1]
				: (atoi(two.c_str()) == 0 ? NcmpcppKeys::NullKey : atoi(two.c_str()));
	}
	
	Border IntoBorder(const std::string &color)
	{
		return Border(IntoColor(color));
	}
	
	BasicScreen *IntoScreen(int n)
	{
		switch (n)
		{
			case 1:
				return myHelp;
			case 2:
				return myPlaylist;
			case 3:
				return myBrowser;
			case 4:
				return mySearcher;
			case 5:
				return myLibrary;
			case 6:
				return myPlaylistEditor;
#			ifdef HAVE_TAGLIB_H
			case 7:
				return myTagEditor;
#			endif // HAVE_TAGLIB_H
#			ifdef ENABLE_OUTPUTS
			case 8:
				return myOutputs;
#			endif // ENABLE_OUTPUTS
#			ifdef ENABLE_VISUALIZER
			case 9:
				return myVisualizer;
#			endif // ENABLE_VISUALIZER
#			ifdef ENABLE_CLOCK
			case 10:
				return myClock;
#			endif // ENABLE_CLOCK
			default:
				return 0;
		}
	}
	
	std::string RemoveDollarFormatting(const std::string &s)
	{
		std::string result;
		for (size_t i = 0; i < s.size(); ++i)
		{
			if (s[i] != '$')
				result += s[i];
			else
				++i;
		}
		return result;
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
	
	header_height = Config.new_design ? (Config.header_visibility ? 5 : 3) : 1;
	footer_start_y = LINES-(Config.statusbar_visibility ? 2 : 1);
	footer_height = Config.statusbar_visibility ? 2 : 1;
}

void NcmpcppKeys::SetDefaults()
{
	Up[0] = KEY_UP;
	Down[0] = KEY_DOWN;
	UpAlbum[0] = '[';
	DownAlbum[0] = ']';
	UpArtist[0] = '{';
	DownArtist[0] = '}';
	PageUp[0] = KEY_PPAGE;
	PageDown[0] = KEY_NPAGE;
	Home[0] = KEY_HOME;
	End[0] = KEY_END;
	Space[0] = 32;
	Enter[0] = 10;
	Delete[0] = KEY_DC;
	VolumeUp[0] = KEY_RIGHT;
	VolumeDown[0] = KEY_LEFT;
	PrevColumn[0] = KEY_LEFT;
	NextColumn[0] = KEY_RIGHT;
	ScreenSwitcher[0] = 9;
	BackwardScreenSwitcher[0] = 353;
	Help[0] = '1';
	Playlist[0] = '2';
	Browser[0] = '3';
	SearchEngine[0] = '4';
	MediaLibrary[0] = '5';
	PlaylistEditor[0] = '6';
	TagEditor[0] = '7';
	Outputs[0] = '8';
	Visualizer[0] = '9';
	Clock[0] = '0';
	ServerInfo[0] = '@';
	Stop[0] = 's';
	Pause[0] = 'P';
	Next[0] = '>';
	Prev[0] = '<';
	Replay[0] = KEY_BACKSPACE;
	SeekForward[0] = 'f';
	SeekBackward[0] = 'b';
	ToggleRepeat[0] = 'r';
	ToggleRandom[0] = 'z';
	ToggleSingle[0] = 'y';
	ToggleConsume[0] = 'R';
	ToggleReplayGainMode[0] = 'Y';
	ToggleSpaceMode[0] = 't';
	ToggleAddMode[0] = 'T';
	ToggleMouse[0] = '|';
	ToggleBitrateVisibility[0] = '#';
	Shuffle[0] = 'Z';
	ToggleCrossfade[0] = 'x';
	SetCrossfade[0] = 'X';
	UpdateDB[0] = 'u';
	SortPlaylist[0] = 22;
	ApplyFilter[0] = 6;
	FindForward[0] = '/';
	FindBackward[0] = '?';
	NextFoundPosition[0] = '.';
	PrevFoundPosition[0] = ',';
	ToggleFindMode[0] = 'w';
	EditTags[0] = 'e';
	SongInfo[0] = 'i';
	ArtistInfo[0] = 'I';
	GoToPosition[0] = 'g';
	Lyrics[0] = 'l';
	ReverseSelection[0] = 'v';
	DeselectAll[0] = 'V';
	SelectAlbum[0] = 'B';
	AddSelected[0] = 'A';
	Clear[0] = 'c';
	Crop[0] = 'C';
	MvSongUp[0] = 'm';
	MvSongDown[0] = 'n';
	MoveTo[0] = 'M';
	MoveBefore[0] = NullKey;
	MoveAfter[0] = NullKey;
	Add[0] = 'a';
	SavePlaylist[0] = 'S';
	GoToNowPlaying[0] = 'o';
	GoToContainingDir[0] = 'G';
	GoToMediaLibrary[0] = '~';
	GoToTagEditor[0] = 'E';
	ToggleAutoCenter[0] = 'U';
	ToggleDisplayMode[0] = 'p';
	ToggleInterface[0] = '\\';
	ToggleSeparatorsInPlaylist[0] = '!';
	ToggleLyricsDB[0] = 'L';
	ToggleFetchingLyricsInBackground[0] = 'F';
	GoToParentDir[0] = KEY_BACKSPACE;
	SwitchTagTypeList[0] = '`';
	Quit[0] = 'q';

	Up[1] = 'k';
	Down[1] = 'j';
	UpAlbum[1] = NullKey;
	DownAlbum[1] = NullKey;
	UpArtist[1] = NullKey;
	DownArtist[1] = NullKey;
	PageUp[1] = NullKey;
	PageDown[1] = NullKey;
	Home[1] = NullKey;
	End[1] = NullKey;
	Space[1] = NullKey;
	Enter[1] = NullKey;
	Delete[1] = 'd';
	VolumeUp[1] = '+';
	VolumeDown[1] = '-';
	PrevColumn[1] = NullKey;
	NextColumn[1] = NullKey;
	ScreenSwitcher[1] = NullKey;
	BackwardScreenSwitcher[1] = NullKey;
	Help[1] = 265;
	Playlist[1] = 266;
	Browser[1] = 267;
	SearchEngine[1] = 268;
	MediaLibrary[1] = 269;
	PlaylistEditor[1] = 270;
	TagEditor[1] = 271;
	Outputs[1] = 272;
	Visualizer[1] = 273;
	Clock[1] = 274;
	ServerInfo[1] = NullKey;
	Stop[1] = NullKey;
	Pause[1] = NullKey;
	Next[1] = NullKey;
	Prev[1] = NullKey;
	Replay[1] = 127;
	SeekForward[1] = NullKey;
	SeekBackward[1] = NullKey;
	ToggleRepeat[1] = NullKey;
	ToggleRandom[1] = NullKey;
	ToggleSingle[1] = NullKey;
	ToggleConsume[1] = NullKey;
	ToggleReplayGainMode[1] = NullKey;
	ToggleSpaceMode[1] = NullKey;
	ToggleAddMode[1] = NullKey;
	ToggleMouse[1] = NullKey;
	ToggleBitrateVisibility[1] = NullKey;
	Shuffle[1] = NullKey;
	ToggleCrossfade[1] = NullKey;
	SetCrossfade[1] = NullKey;
	UpdateDB[1] = NullKey;
	SortPlaylist[1] = NullKey;
	ApplyFilter[1] = NullKey;
	FindForward[1] = NullKey;
	FindBackward[1] = NullKey;
	NextFoundPosition[1] = NullKey;
	PrevFoundPosition[1] = NullKey;
	ToggleFindMode[1] = NullKey;
	EditTags[1] = NullKey;
	SongInfo[1] = NullKey;
	ArtistInfo[1] = NullKey;
	GoToPosition[1] = NullKey;
	Lyrics[1] = NullKey;
	ReverseSelection[1] = NullKey;
	DeselectAll[1] = NullKey;
	SelectAlbum[1] = NullKey;
	AddSelected[1] = NullKey;
	Clear[1] = NullKey;
	Crop[1] = NullKey;
	MvSongUp[1] = NullKey;
	MvSongDown[1] = NullKey;
	MoveTo[1] = NullKey;
	MoveBefore[1] = NullKey;
	MoveAfter[1] = NullKey;
	Add[1] = NullKey;
	SavePlaylist[1] = NullKey;
	GoToNowPlaying[1] = NullKey;
	GoToContainingDir[1] = NullKey;
	GoToMediaLibrary[1] = NullKey;
	GoToTagEditor[1] = NullKey;
	ToggleAutoCenter[1] = NullKey;
	ToggleDisplayMode[1] = NullKey;
	ToggleInterface[1] = NullKey;
	ToggleSeparatorsInPlaylist[1] = NullKey;
	ToggleLyricsDB[1] = NullKey;
	ToggleFetchingLyricsInBackground[1] = NullKey;
	GoToParentDir[1] = 127;
	SwitchTagTypeList[1] = NullKey;
	Quit[1] = 'Q';
}

void NcmpcppConfig::SetDefaults()
{
	mpd_host = "localhost";
	empty_tag = "<empty>";
	song_list_columns_format = "(7f)[green]{l} (25)[cyan]{a} (40)[]{t|f} (30)[red]{b}";
	song_list_format = "{{%a - }{%t}|{$8%f$9}$R{$3(%l)$9}}";
	song_list_format_dollar_free = RemoveDollarFormatting(song_list_format);
	song_status_format = "{{{%a{ \"%b\"{ (%y)}} - }{%t}}|{%f}}";
	song_status_format_no_colors = song_status_format;
	song_window_title_format = "{{%a - }{%t}|{%f}}";
	song_library_format = "{{%n - }{%t}|{%f}}";
	tag_editor_album_format = "{{(%y) }%b}";
	new_header_first_line = "{$b$1$aqqu$/a$9 {%t}|{%f} $1$atqq$/a$9$/b}";
	new_header_second_line = "{{{$4$b%a$/b$9}{ - $7%b$9}{ ($4%y$9)}}|{%D}}";
	browser_playlist_prefix << clRed << "(playlist)" << clEnd << ' ';
	progressbar = U("=>\0");
	pattern = "%n - %t";
	selected_item_prefix << clMagenta;
	selected_item_suffix << clEnd;
	now_playing_prefix << fmtBold;
	now_playing_suffix << fmtBoldEnd;
	color1 = clWhite;
	color2 = clGreen;
	empty_tags_color = clCyan;
	header_color = clDefault;
	volume_color = clDefault;
	state_line_color = clDefault;
	state_flags_color = clDefault;
	main_color = clYellow;
	main_highlight_color = main_color;
	progressbar_color = clDefault;
	statusbar_color = clDefault;
	alternative_ui_separator_color = clBlack;
	active_column_color = clRed;
	window_border = brGreen;
	active_window_border = brRed;
	visualizer_color = clYellow;
	media_lib_primary_tag = MPD_TAG_ARTIST;
	enable_idle_notifications = true;
	colors_enabled = true;
	fancy_scrolling = true;
	playlist_show_remaining_time = false;
	playlist_shorten_total_times = false;
	playlist_separate_albums = false;
	columns_in_playlist = false;
	columns_in_browser = false;
	columns_in_search_engine = false;
	header_visibility = true;
	header_text_scrolling = true;
	statusbar_visibility = true;
	titles_visibility = true;
	centered_cursor = false;
	screen_switcher_previous = false;
	autocenter_mode = false;
	wrapped_search = true;
	space_selects = false;
	ncmpc_like_songs_adding = false;
	albums_in_tag_editor = false;
	incremental_seeking = true;
	now_playing_lyrics = false;
	fetch_lyrics_in_background = false;
	local_browser_show_hidden_files = false;
	search_in_db = true;
	display_screens_numbers_on_start = true;
	jump_to_now_playing_song_at_start = true;
	clock_display_seconds = false;
	display_volume_level = true;
	display_bitrate = false;
	display_remaining_time = false;
	ignore_leading_the = false;
	block_search_constraints_change = true;
	use_console_editor = false;
	use_cyclic_scrolling = false;
	allow_physical_files_deletion = false;
	allow_physical_directories_deletion = false;
	ask_before_clearing_main_playlist = false;
	mouse_support = true;
	mouse_list_scroll_whole_page = true;
	new_design = false;
	visualizer_use_wave = true;
	visualizer_in_stereo = false;
	browser_sort_by_mtime = false;
	tag_editor_extended_numeration = false;
	media_library_display_date = true;
	media_library_display_empty_tag = true;
	media_library_disable_two_column_mode = false;
	discard_colors_if_item_is_selected = true;
	store_lyrics_in_song_dir = false;
	set_window_title = true;
	mpd_port = 6600;
	mpd_connection_timeout = 15;
	crossfade_time = 5;
	seek_time = 1;
	playlist_disable_highlight_delay = 5;
	message_delay_time = 4;
	lyrics_db = 0;
	regex_type = 0;
	lines_scrolled = 2;
	search_engine_default_search_mode = 0;
	visualizer_sync_interval = 30;
	selected_item_suffix_length = 0;
	now_playing_suffix_length = 0;
#	ifdef HAVE_LANGINFO_H
	system_encoding = nl_langinfo(CODESET);
	if (system_encoding == "UTF-8") // mpd uses utf-8 by default so no need to convert
		system_encoding.clear();
#	endif // HAVE_LANGINFO_H
	startup_screen = myPlaylist;
	// default screens sequence
	screens_seq.push_back(myPlaylist);
	screens_seq.push_back(myBrowser);
}

void NcmpcppKeys::Read()
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
				GetKeys(key, Up);
			else if (key.find("key_down ") != std::string::npos)
				GetKeys(key, Down);
			else if (key.find("key_up_album ") != std::string::npos)
				GetKeys(key, UpAlbum);
			else if (key.find("key_down_album ") != std::string::npos)
				GetKeys(key, DownAlbum);
			else if (key.find("key_up_artist ") != std::string::npos)
				GetKeys(key, UpArtist);
			else if (key.find("key_down_artist ") != std::string::npos)
				GetKeys(key, DownArtist);
			else if (key.find("key_page_up ") != std::string::npos)
				GetKeys(key, PageUp);
			else if (key.find("key_page_down ") != std::string::npos)
				GetKeys(key, PageDown);
			else if (key.find("key_home ") != std::string::npos)
				GetKeys(key, Home);
			else if (key.find("key_end ") != std::string::npos)
				GetKeys(key, End);
			else if (key.find("key_space ") != std::string::npos)
				GetKeys(key, Space);
			else if (key.find("key_enter ") != std::string::npos)
				GetKeys(key, Enter);
			else if (key.find("key_delete ") != std::string::npos)
				GetKeys(key, Delete);
			else if (key.find("key_volume_up ") != std::string::npos)
				GetKeys(key, VolumeUp);
			else if (key.find("key_volume_down ") != std::string::npos)
				GetKeys(key, VolumeDown);
			else if (key.find("key_prev_column ") != std::string::npos)
				GetKeys(key, PrevColumn);
			else if (key.find("key_next_column ") != std::string::npos)
				GetKeys(key, NextColumn);
			else if (key.find("key_screen_switcher ") != std::string::npos)
				GetKeys(key, ScreenSwitcher);
			else if (key.find("key_backward_screen_switcher ") != std::string::npos)
				GetKeys(key, BackwardScreenSwitcher);
			else if (key.find("key_help ") != std::string::npos)
				GetKeys(key, Help);
			else if (key.find("key_playlist ") != std::string::npos)
				GetKeys(key, Playlist);
			else if (key.find("key_browser ") != std::string::npos)
				GetKeys(key, Browser);
			else if (key.find("key_search_engine ") != std::string::npos)
				GetKeys(key, SearchEngine);
			else if (key.find("key_media_library ") != std::string::npos)
				GetKeys(key, MediaLibrary);
			else if (key.find("key_playlist_editor ") != std::string::npos)
				GetKeys(key, PlaylistEditor);
			else if (key.find("key_tag_editor ") != std::string::npos)
				GetKeys(key, TagEditor);
			else if (key.find("key_outputs ") != std::string::npos)
				GetKeys(key, Outputs);
			else if (key.find("key_music_visualizer ") != std::string::npos)
				GetKeys(key, Visualizer);
			else if (key.find("key_clock ") != std::string::npos)
				GetKeys(key, Clock);
			else if (key.find("key_server_info ") != std::string::npos)
				GetKeys(key, ServerInfo);
			else if (key.find("key_stop ") != std::string::npos)
				GetKeys(key, Stop);
			else if (key.find("key_pause ") != std::string::npos)
				GetKeys(key, Pause);
			else if (key.find("key_next ") != std::string::npos)
				GetKeys(key, Next);
			else if (key.find("key_prev ") != std::string::npos)
				GetKeys(key, Prev);
			else if (key.find("key_replay ") != std::string::npos)
				GetKeys(key, Replay);
			else if (key.find("key_seek_forward ") != std::string::npos)
				GetKeys(key, SeekForward);
			else if (key.find("key_seek_backward ") != std::string::npos)
				GetKeys(key, SeekBackward);
			else if (key.find("key_toggle_repeat ") != std::string::npos)
				GetKeys(key, ToggleRepeat);
			else if (key.find("key_toggle_random ") != std::string::npos)
				GetKeys(key, ToggleRandom);
			else if (key.find("key_toggle_single ") != std::string::npos)
				GetKeys(key, ToggleSingle);
			else if (key.find("key_toggle_consume ") != std::string::npos)
				GetKeys(key, ToggleConsume);
			else if (key.find("key_toggle_replay_gain_mode ") != std::string::npos)
				GetKeys(key, ToggleReplayGainMode);
			else if (key.find("key_toggle_space_mode ") != std::string::npos)
				GetKeys(key, ToggleSpaceMode);
			else if (key.find("key_toggle_add_mode ") != std::string::npos)
				GetKeys(key, ToggleAddMode);
			else if (key.find("key_toggle_mouse ") != std::string::npos)
				GetKeys(key, ToggleMouse);
			else if (key.find("key_toggle_bitrate_visibility ") != std::string::npos)
				GetKeys(key, ToggleBitrateVisibility);
			else if (key.find("key_shuffle ") != std::string::npos)
				GetKeys(key, Shuffle);
			else if (key.find("key_toggle_crossfade ") != std::string::npos)
				GetKeys(key, ToggleCrossfade);
			else if (key.find("key_set_crossfade ") != std::string::npos)
				GetKeys(key, SetCrossfade);
			else if (key.find("key_update_db ") != std::string::npos)
				GetKeys(key, UpdateDB);
			else if (key.find("key_sort_playlist ") != std::string::npos)
				GetKeys(key, SortPlaylist);
			else if (key.find("key_apply_filter ") != std::string::npos)
				GetKeys(key, ApplyFilter);
			else if (key.find("key_find_forward ") != std::string::npos)
				GetKeys(key, FindForward);
			else if (key.find("key_find_backward ") != std::string::npos)
				GetKeys(key, FindBackward);
			else if (key.find("key_next_found_position ") != std::string::npos)
				GetKeys(key, NextFoundPosition);
			else if (key.find("key_prev_found_position ") != std::string::npos)
				GetKeys(key, PrevFoundPosition);
			else if (key.find("key_toggle_find_mode ") != std::string::npos)
				GetKeys(key, ToggleFindMode);
			else if (key.find("key_edit_tags ") != std::string::npos)
				GetKeys(key, EditTags);
			else if (key.find("key_go_to_position ") != std::string::npos)
				GetKeys(key, GoToPosition);
			else if (key.find("key_song_info ") != std::string::npos)
				GetKeys(key, SongInfo);
			else if (key.find("key_artist_info ") != std::string::npos)
				GetKeys(key, ArtistInfo);
			else if (key.find("key_lyrics ") != std::string::npos)
				GetKeys(key, Lyrics);
			else if (key.find("key_reverse_selection ") != std::string::npos)
				GetKeys(key, ReverseSelection);
			else if (key.find("key_deselect_all ") != std::string::npos)
				GetKeys(key, DeselectAll);
			else if (key.find("key_select_album ") != std::string::npos)
				GetKeys(key, SelectAlbum);
			else if (key.find("key_add_selected_items ") != std::string::npos)
				GetKeys(key, AddSelected);
			else if (key.find("key_clear ") != std::string::npos)
				GetKeys(key, Clear);
			else if (key.find("key_crop ") != std::string::npos)
				GetKeys(key, Crop);
			else if (key.find("key_move_song_up ") != std::string::npos)
				GetKeys(key, MvSongUp);
			else if (key.find("key_move_song_down ") != std::string::npos)
				GetKeys(key, MvSongDown);
			else if (key.find("key_move_to ") != std::string::npos)
				GetKeys(key, MoveTo);
			else if (key.find("key_move_before ") != std::string::npos)
				GetKeys(key, MoveBefore);
			else if (key.find("key_move_after ") != std::string::npos)
				GetKeys(key, MoveAfter);
			else if (key.find("key_add ") != std::string::npos)
				GetKeys(key, Add);
			else if (key.find("key_save_playlist ") != std::string::npos)
				GetKeys(key, SavePlaylist);
			else if (key.find("key_go_to_now_playing ") != std::string::npos)
				GetKeys(key, GoToNowPlaying);
			else if (key.find("key_toggle_auto_center ") != std::string::npos)
				GetKeys(key, ToggleAutoCenter);
			else if (key.find("key_toggle_display_mode ") != std::string::npos)
				GetKeys(key, ToggleDisplayMode);
			else if (key.find("key_toggle_separators_in_playlist ") != std::string::npos)
				GetKeys(key, ToggleSeparatorsInPlaylist);
			else if (key.find("key_toggle_lyrics_db ") != std::string::npos)
				GetKeys(key, ToggleLyricsDB);
			else if (key.find("key_toggle_fetching_lyrics_for_current_song_in_background ") != std::string::npos)
				GetKeys(key, ToggleFetchingLyricsInBackground);
			else if (key.find("key_go_to_containing_directory ") != std::string::npos)
				GetKeys(key, GoToContainingDir);
			else if (key.find("key_go_to_media_library ") != std::string::npos)
				GetKeys(key, GoToMediaLibrary);
			else if (key.find("key_go_to_parent_dir ") != std::string::npos)
				GetKeys(key, GoToParentDir);
			else if (key.find("key_switch_tag_type_list ") != std::string::npos)
				GetKeys(key, SwitchTagTypeList);
			else if (key.find("key_quit ") != std::string::npos)
				GetKeys(key, Quit);
		}
	}
	f.close();
}

void NcmpcppConfig::Read()
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
					mpd_host = v;
			}
			else if (cl.find("mpd_music_dir") != std::string::npos)
			{
				if (!v.empty())
				{
					 // if ~ is used at the beginning, replace it with user's home folder
					if (v[0] == '~')
						v.replace(0, 1, home_path);
					mpd_music_dir = v + "/";
				}
			}
			else if (cl.find("visualizer_fifo_path") != std::string::npos)
			{
				if (!v.empty())
					visualizer_fifo_path = v;
			}
			else if (cl.find("visualizer_output_name") != std::string::npos)
			{
				if (!v.empty())
					visualizer_output_name = v;
			}
			else if (cl.find("mpd_port") != std::string::npos)
			{
				if (StrToInt(v))
					mpd_port = StrToInt(v);
			}
			else if (cl.find("mpd_connection_timeout") != std::string::npos)
			{
				if (StrToInt(v))
					mpd_connection_timeout = StrToInt(v);
			}
			else if (cl.find("mpd_crossfade_time") != std::string::npos)
			{
				if (StrToInt(v) > 0)
					crossfade_time = StrToInt(v);
			}
			else if (cl.find("seek_time") != std::string::npos)
			{
				if (StrToInt(v) > 0)
					seek_time = StrToInt(v);
			}
			else if (cl.find("playlist_disable_highlight_delay") != std::string::npos)
			{
				if (StrToInt(v) >= 0)
					playlist_disable_highlight_delay = StrToInt(v);
			}
			else if (cl.find("message_delay_time") != std::string::npos)
			{
				if (StrToInt(v) > 0)
					message_delay_time = StrToInt(v);
			}
			else if (cl.find("song_list_format") != std::string::npos)
			{
				if (!v.empty() && MPD::Song::isFormatOk("song_list_format", v))
				{
					song_list_format = '{';
					song_list_format += v;
					song_list_format += '}';
					song_list_format_dollar_free = RemoveDollarFormatting(song_list_format);
				}
			}
			else if (cl.find("song_columns_list_format") != std::string::npos)
			{
				if (!v.empty())
					song_list_columns_format = v;
			}
			else if (cl.find("song_status_format") != std::string::npos)
			{
				if (!v.empty() && MPD::Song::isFormatOk("song_status_format", v))
				{
					song_status_format = '{';
					song_status_format += v;
					song_status_format += '}';
					// make version without colors
					if (song_status_format.find("$") != std::string::npos)
					{
						Buffer status_no_colors;
						String2Buffer(song_status_format, status_no_colors);
						song_status_format_no_colors = status_no_colors.Str();
					}
					else
						song_status_format_no_colors = song_status_format;
				}
			}
			else if (cl.find("song_library_format") != std::string::npos)
			{
				if (!v.empty() && MPD::Song::isFormatOk("song_library_format", v))
				{
					song_library_format = '{';
					song_library_format += v;
					song_library_format += '}';
				}
			}
			else if (cl.find("tag_editor_album_format") != std::string::npos)
			{
				if (!v.empty() && MPD::Song::isFormatOk("tag_editor_album_format", v))
				{
					tag_editor_album_format = '{';
					tag_editor_album_format += v;
					tag_editor_album_format += '}';
				}
			}
			else if (cl.find("external_editor") != std::string::npos)
			{
				if (!v.empty())
					external_editor = v;
			}
			else if (cl.find("system_encoding") != std::string::npos)
			{
				if (!v.empty())
					system_encoding = v;
			}
			else if (cl.find("execute_on_song_change") != std::string::npos)
			{
				if (!v.empty())
					execute_on_song_change = v;
			}
			else if (cl.find("alternative_header_first_line_format") != std::string::npos)
			{
				if (!v.empty() && MPD::Song::isFormatOk("alternative_header_first_line_format", v))
				{
					new_header_first_line = '{';
					new_header_first_line += v;
					new_header_first_line += '}';
				}
			}
			else if (cl.find("alternative_header_second_line_format") != std::string::npos)
			{
				if (!v.empty() && MPD::Song::isFormatOk("alternative_header_second_line_format", v))
				{
					new_header_second_line = '{';
					new_header_second_line += v;
					new_header_second_line += '}';
				}
			}
			else if (cl.find("lastfm_preferred_language") != std::string::npos)
			{
				if (!v.empty() && v != "en")
					lastfm_preferred_language = v;
			}
			else if (cl.find("browser_playlist_prefix") != std::string::npos)
			{
				if (!v.empty())
				{
					browser_playlist_prefix.Clear();
					String2Buffer(v, browser_playlist_prefix);
				}
			}
			else if (cl.find("progressbar_look") != std::string::npos)
			{
				std::basic_string<my_char_t> pb = TO_WSTRING(v);
				if (pb.length() < 2 || pb.length() > 3)
				{
					std::cerr << "Warning: length of progressbar_look should be either ";
					std::cerr << "2 or 3, but it's " << pb.length() << ", discarding.\n";
				}
				else
					progressbar = pb;
				// if two characters were specified, add third one as null
				progressbar.resize(3);
			}
			else if (cl.find("default_tag_editor_pattern") != std::string::npos)
			{
				if (!v.empty())
					pattern = v;
			}
			else if (cl.find("selected_item_prefix") != std::string::npos)
			{
				if (!v.empty())
				{
					selected_item_prefix.Clear();
					String2Buffer(v, selected_item_prefix);
				}
			}
			else if (cl.find("selected_item_suffix") != std::string::npos)
			{
				if (!v.empty())
				{
					selected_item_suffix.Clear();
					String2Buffer(v, selected_item_suffix);
					selected_item_suffix_length = Window::Length(TO_WSTRING(selected_item_suffix.Str()));
				}
			}
			else if (cl.find("now_playing_prefix") != std::string::npos)
			{
				if (!v.empty())
				{
					now_playing_prefix.Clear();
					String2Buffer(v, now_playing_prefix);
				}
			}
			else if (cl.find("now_playing_suffix") != std::string::npos)
			{
				if (!v.empty())
				{
					now_playing_suffix.Clear();
					String2Buffer(TO_WSTRING(v), now_playing_suffix);
					now_playing_suffix_length = Window::Length(now_playing_suffix.Str());
				}
			}
			else if (cl.find("color1") != std::string::npos)
			{
				if (!v.empty())
					color1 = IntoColor(v);
			}
			else if (cl.find("color2") != std::string::npos)
			{
				if (!v.empty())
					color2 = IntoColor(v);
			}
			else if (cl.find("mpd_communication_mode") != std::string::npos)
			{
				enable_idle_notifications = v == "notifications";
			}
			else if (cl.find("colors_enabled") != std::string::npos)
			{
				colors_enabled = v == "yes";
			}
			else if (cl.find("fancy_scrolling") != std::string::npos)
			{
				fancy_scrolling = v == "yes";
			}
			else if (cl.find("cyclic_scrolling") != std::string::npos)
			{
				use_cyclic_scrolling = v == "yes";
			}
			else if (cl.find("playlist_show_remaining_time") != std::string::npos)
			{
				playlist_show_remaining_time = v == "yes";
			}
			else if (cl.find("playlist_shorten_total_times") != std::string::npos)
			{
				playlist_shorten_total_times = v == "yes";
			}
			else if (cl.find("playlist_separate_albums") != std::string::npos)
			{
				playlist_separate_albums = v == "yes";
			}
			else if (cl.find("playlist_display_mode") != std::string::npos)
			{
				columns_in_playlist = v == "columns";
			}
			else if (cl.find("browser_display_mode") != std::string::npos)
			{
				columns_in_browser = v == "columns";
			}
			else if (cl.find("search_engine_display_mode") != std::string::npos)
			{
				columns_in_search_engine = v == "columns";
			}
			else if (cl.find("header_visibility") != std::string::npos)
			{
				header_visibility = v == "yes";
			}
			else if (cl.find("header_text_scrolling") != std::string::npos)
			{
				header_text_scrolling = v == "yes";
			}
			else if (cl.find("statusbar_visibility") != std::string::npos)
			{
				statusbar_visibility = v == "yes";
			}
			else if (cl.find("titles_visibility") != std::string::npos)
			{
				titles_visibility = v == "yes";
			}
			else if (cl.find("screen_switcher_mode") != std::string::npos)
			{
				if (v.find("previous") != std::string::npos)
				{
					screen_switcher_previous = true;
				}
				else if (v.find("sequence") != std::string::npos)
				{
					screen_switcher_previous = false;
					screens_seq.clear();
					for (std::string::const_iterator it = v.begin(); it != v.end(); )
					{
						while (it != v.end() && !isdigit(*it))
							++it;
						if (it == v.end())
							break;
						if (BasicScreen *screen = IntoScreen(atoi(&*it)))
							screens_seq.push_back(screen);
						while (it != v.end() && isdigit(*it))
							++it;
					}
					// throw away duplicates
					screens_seq.unique();
				}
			}
			else if (cl.find("startup_screen") != std::string::npos)
			{
				startup_screen = IntoScreen(atoi(v.c_str()));
				if (!startup_screen)
					startup_screen = myPlaylist;
			}
			else if (cl.find("autocenter_mode") != std::string::npos)
			{
				autocenter_mode = v == "yes";
			}
			else if (cl.find("centered_cursor") != std::string::npos)
			{
				centered_cursor = v == "yes";
			}
			else if (cl.find("default_find_mode") != std::string::npos)
			{
				wrapped_search = v == "wrapped";
			}
			else if (cl.find("default_space_mode") != std::string::npos)
			{
				space_selects = v == "select";
			}
			else if (cl.find("default_tag_editor_left_col") != std::string::npos)
			{
				albums_in_tag_editor = v == "albums";
			}
			else if (cl.find("incremental_seeking") != std::string::npos)
			{
				incremental_seeking = v == "yes";
			}
			else if (cl.find("show_hidden_files_in_local_browser") != std::string::npos)
			{
				local_browser_show_hidden_files = v == "yes";
			}
			else if (cl.find("follow_now_playing_lyrics") != std::string::npos)
			{
				now_playing_lyrics = v == "yes";
			}
			else if (cl.find("fetch_lyrics_for_current_song_in_background") != std::string::npos)
			{
				fetch_lyrics_in_background = v == "yes";
			}
			else if (cl.find("ncmpc_like_songs_adding") != std::string::npos)
			{
				ncmpc_like_songs_adding = v == "yes";
			}
			else if (cl.find("default_place_to_search_in") != std::string::npos)
			{
				search_in_db = v == "database";
			}
			else if (cl.find("display_screens_numbers_on_start") != std::string::npos)
			{
				display_screens_numbers_on_start = v == "yes";
			}
			else if (cl.find("jump_to_now_playing_song_at_start") != std::string::npos)
			{
				jump_to_now_playing_song_at_start = v == "yes";
			}
			else if (cl.find("clock_display_seconds") != std::string::npos)
			{
				clock_display_seconds = v == "yes";
			}
			else if (cl.find("display_volume_level") != std::string::npos)
			{
				display_volume_level = v == "yes";
			}
			else if (cl.find("display_bitrate") != std::string::npos)
			{
				display_bitrate = v == "yes";
			}
			else if (cl.find("display_remaining_time") != std::string::npos)
			{
				display_remaining_time = v == "yes";
			}
			else if (cl.find("ignore_leading_the") != std::string::npos)
			{
				ignore_leading_the = v == "yes";
			}
			else if (cl.find("use_console_editor") != std::string::npos)
			{
				use_console_editor = v == "yes";
			}
			else if (cl.find("block_search_constraints_change_if_items_found") != std::string::npos)
			{
				block_search_constraints_change = v == "yes";
			}
			else if (cl.find("allow_physical_files_deletion") != std::string::npos)
			{
				allow_physical_files_deletion = v == "yes";
			}
			else if (cl.find("allow_physical_directories_deletion") != std::string::npos)
			{
				allow_physical_directories_deletion = v == "yes";
			}
			else if (cl.find("ask_before_clearing_main_playlist") != std::string::npos)
			{
				ask_before_clearing_main_playlist = v == "yes";
			}
			else if (cl.find("visualizer_type") != std::string::npos)
			{
				visualizer_use_wave = v == "wave";
			}
			else if (cl.find("visualizer_in_stereo") != std::string::npos)
			{
				visualizer_in_stereo = v == "yes";
			}
			else if (cl.find("mouse_support") != std::string::npos)
			{
				mouse_support = v == "yes";
			}
			else if (cl.find("mouse_list_scroll_whole_page") != std::string::npos)
			{
				mouse_list_scroll_whole_page = v == "yes";
			}
			else if (cl.find("user_interface") != std::string::npos)
			{
				new_design = v == "alternative";
			}
			else if (cl.find("tag_editor_extended_numeration") != std::string::npos)
			{
				tag_editor_extended_numeration = v == "yes";
			}
			else if (cl.find("media_library_display_date") != std::string::npos)
			{
				media_library_display_date = v == "yes";
			}
			else if (cl.find("media_library_display_empty_tag") != std::string::npos)
			{
				media_library_display_empty_tag = v == "yes";
			}
			else if (cl.find("media_library_disable_two_column_mode") != std::string::npos)
			{
				media_library_disable_two_column_mode = v == "yes";
			}
			else if (cl.find("discard_colors_if_item_is_selected") != std::string::npos)
			{
				discard_colors_if_item_is_selected = v == "yes";
			}
			else if (cl.find("store_lyrics_in_song_dir") != std::string::npos)
			{
				if (mpd_music_dir.empty())
				{
					std::cerr << "Warning: store_lyrics_in_song_dir = \"yes\" is ";
					std::cerr << "not allowed without mpd_music_dir set, discarding.\n";
				}
				else
					store_lyrics_in_song_dir = v == "yes";
			}
			else if (cl.find("enable_window_title") != std::string::npos)
			{
				set_window_title = v == "yes";
			}
			else if (cl.find("regular_expressions") != std::string::npos)
			{
				regex_type = REG_EXTENDED * (v != "basic");
			}
			else if (cl.find("lines_scrolled") != std::string::npos)
			{
				if (!v.empty())
					lines_scrolled = StrToInt(v);
			}
			else if (cl.find("search_engine_default_search_mode") != std::string::npos)
			{
				if (!v.empty())
				{
					unsigned mode = StrToInt(v);
					if (--mode < 3)
						search_engine_default_search_mode = mode;
				}
			}
			else if (cl.find("visualizer_sync_interval") != std::string::npos)
			{
				unsigned interval = StrToInt(v);
				if (interval)
					visualizer_sync_interval = interval;
			}
			else if (cl.find("song_window_title_format") != std::string::npos)
			{
				if (!v.empty() && MPD::Song::isFormatOk("song_window_title_format", v))
				{
					song_window_title_format = '{';
					song_window_title_format += v;
					song_window_title_format += '}';
				}
			}
			else if (cl.find("empty_tag_marker") != std::string::npos)
			{
				empty_tag = v; // is this case empty string is allowed
			}
			else if (cl.find("empty_tag_color") != std::string::npos)
			{
				if (!v.empty())
					empty_tags_color = IntoColor(v);
			}
			else if (cl.find("header_window_color") != std::string::npos)
			{
				if (!v.empty())
					header_color = IntoColor(v);
			}
			else if (cl.find("volume_color") != std::string::npos)
			{
				if (!v.empty())
					volume_color = IntoColor(v);
			}
			else if (cl.find("state_line_color") != std::string::npos)
			{
				if (!v.empty())
					state_line_color = IntoColor(v);
			}
			else if (cl.find("state_flags_color") != std::string::npos)
			{
				if (!v.empty())
					state_flags_color = IntoColor(v);
			}
			else if (cl.find("main_window_color") != std::string::npos)
			{
				if (!v.empty())
					main_color = IntoColor(v);
			}
			else if (cl.find("main_window_highlight_color") != std::string::npos)
			{
				if (!v.empty())
					main_highlight_color = IntoColor(v);
			}
			else if (cl.find("progressbar_color") != std::string::npos)
			{
				if (!v.empty())
					progressbar_color = IntoColor(v);
			}
			else if (cl.find("statusbar_color") != std::string::npos)
			{
				if (!v.empty())
					statusbar_color = IntoColor(v);
			}
			else if (cl.find("alternative_ui_separator_color") != std::string::npos)
			{
				if (!v.empty())
					alternative_ui_separator_color = IntoColor(v);
			}
			else if (cl.find("active_column_color") != std::string::npos)
			{
				if (!v.empty())
					active_column_color = IntoColor(v);
			}
			else if (cl.find("visualizer_color") != std::string::npos)
			{
				if (!v.empty())
					visualizer_color = IntoColor(v);
			}
			else if (cl.find("window_border_color ") != std::string::npos)
			{
				if (!v.empty())
					window_border = IntoBorder(v);
			}
			else if (cl.find("active_window_border") != std::string::npos)
			{
				if (!v.empty())
					active_window_border = IntoBorder(v);
			}
			else if (cl.find("media_library_left_column") != std::string::npos)
			{
				if (!v.empty())
					media_lib_primary_tag = IntoTagItem(v[0]);
			}
		}
	}
	f.close();
	
	std::string width;
	while (!(width = GetLineValue(song_list_columns_format, '(', ')', 1)).empty())
	{
		Column col;
		col.color = IntoColor(GetLineValue(song_list_columns_format, '[', ']', 1));
		std::string tag_type = GetLineValue(song_list_columns_format, '{', '}', 1);
		
		col.fixed = *width.rbegin() == 'f';
		
		// alternative name
		size_t tag_type_colon_pos = tag_type.find(':');
		if (tag_type_colon_pos != std::string::npos)
		{
			col.name = TO_WSTRING(tag_type.substr(tag_type_colon_pos+1));
			tag_type.resize(tag_type_colon_pos);
		}
		
		if (!tag_type.empty())
		{
			size_t i = -1;
			
			// extract tag types in format a|b|c etc.
			do
				col.type += tag_type[(++i)++]; // nice one.
			while (tag_type[i] == '|');
			
			// apply attributes
			for (; i < tag_type.length(); ++i)
			{
				switch (tag_type[i])
				{
					case 'r':
						col.right_alignment = 1;
						break;
					case 'E':
						col.display_empty_tag = 0;
						break;
				}
			}
		}
		else // empty column
			col.display_empty_tag = 0;
		
		col.width = StrToInt(width);
		columns.push_back(col);
	}
	
	// generate format for converting tags in columns to string for Playlist::SongInColumnsToString()
	char tag[] = "{% }|";
	song_in_columns_to_string_format = "{";
	for (std::vector<Column>::const_iterator it = columns.begin(); it != columns.end(); ++it)
	{
		for (std::string::const_iterator j = it->type.begin(); j != it->type.end(); ++j)
		{
			tag[2] = *j;
			song_in_columns_to_string_format += tag;
		}
		*song_in_columns_to_string_format.rbegin() = ' ';
	}
	if (song_in_columns_to_string_format.length() == 1) // only '{'
		song_in_columns_to_string_format += '}';
	else
		*song_in_columns_to_string_format.rbegin() = '}';
}

