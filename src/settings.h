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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <vector>

#include <mpd/client.h>

#include "ncmpcpp.h"

class BasicScreen; // forward declaration for screens sequence

struct Column
{
	Column() : right_alignment(0), display_empty_tag(1) { }
	
	std::basic_string<my_char_t> name;
	std::string type;
	unsigned width;
	Color color;
	bool fixed;
	bool right_alignment;
	bool display_empty_tag;
};

struct NcmpcppKeys
{
	static const int NullKey = 0;
	
	void SetDefaults();
	void Read();
	
	int Up[2];
	int Down[2];
	int UpAlbum[2];
	int DownAlbum[2];
	int UpArtist[2];
	int DownArtist[2];
	int PageUp[2];
	int PageDown[2];
	int Home[2];
	int End[2];
	int Space[2];
	int Enter[2];
	int Delete[2];
	int VolumeUp[2];
	int VolumeDown[2];
	int PrevColumn[2];
	int NextColumn[2];
	int ScreenSwitcher[2];
	int BackwardScreenSwitcher[2];
	int Help[2];
	int Playlist[2];
	int Browser[2];
	int SearchEngine[2];
	int MediaLibrary[2];
	int PlaylistEditor[2];
	int TagEditor[2];
	int Outputs[2];
	int Visualizer[2];
	int Clock[2];
	int ServerInfo[2];
	int Stop[2];
	int Pause[2];
	int Next[2];
	int Prev[2];
	int Replay[2];
	int SeekForward[2];
	int SeekBackward[2];
	int ToggleRepeat[2];
	int ToggleRandom[2];
	int ToggleSingle[2];
	int ToggleConsume[2];
	int ToggleReplayGainMode[2];
	int ToggleSpaceMode[2];
	int ToggleAddMode[2];
	int ToggleMouse[2];
	int ToggleBitrateVisibility[2];
	int Shuffle[2];
	int ToggleCrossfade[2];
	int SetCrossfade[2];
	int UpdateDB[2];
	int SortPlaylist[2];
	int ApplyFilter[2];
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
	int SelectAlbum[2];
	int AddSelected[2];
	int Clear[2];
	int Crop[2];
	int MvSongUp[2];
	int MvSongDown[2];
	int MoveTo[2];
	int MoveBefore[2];
	int MoveAfter[2];
	int Add[2];
	int SavePlaylist[2];
	int GoToNowPlaying[2];
	int GoToContainingDir[2];
	int GoToMediaLibrary[2];
	int GoToTagEditor[2];
	int ToggleAutoCenter[2];
	int ToggleDisplayMode[2];
	int ToggleInterface[2];
	int ToggleSeparatorsInPlaylist[2];
	int ToggleLyricsDB[2];
	int ToggleFetchingLyricsInBackground[2];
	int ToggleScreenLock[2];
	int GoToParentDir[2];
	int SwitchTagTypeList[2];
	int Quit[2];
};

struct NcmpcppConfig
{
	NcmpcppConfig();
	
	const std::string &GetHomeDirectory();
	void CheckForCommandLineConfigFilePath(char **argv, int argc);
	
	void SetDefaults();
	void Read();
	
	std::string ncmpcpp_directory;
	std::string lyrics_directory;
	
	std::string mpd_host;
	std::string mpd_music_dir;
	std::string visualizer_fifo_path;
	std::string visualizer_output_name;
	std::string empty_tag;
	std::string song_list_columns_format;
	std::string song_list_format;
	std::string song_list_format_dollar_free;
	std::string song_status_format;
	std::string song_status_format_no_colors;
	std::string song_window_title_format;
	std::string song_library_format;
	std::string tag_editor_album_format;
	std::string song_in_columns_to_string_format;
	std::string external_editor;
	std::string system_encoding;
	std::string execute_on_song_change;
	std::string new_header_first_line;
	std::string new_header_second_line;
	std::string lastfm_preferred_language;
	std::basic_string<my_char_t> progressbar;
	std::basic_string<my_char_t> visualizer_chars;
	
	std::string pattern;
	
	std::vector<Column> columns;
	
	Buffer browser_playlist_prefix;
	Buffer selected_item_prefix;
	Buffer selected_item_suffix;
	Buffer now_playing_prefix;
	basic_buffer<my_char_t> now_playing_suffix;
	
	Color color1;
	Color color2;
	Color empty_tags_color;
	Color header_color;
	Color volume_color;
	Color state_line_color;
	Color state_flags_color;
	Color main_color;
	Color main_highlight_color;
	Color progressbar_color;
	Color statusbar_color;
	Color alternative_ui_separator_color;
	Color active_column_color;
	Color visualizer_color;
	
	Border window_border;
	Border active_window_border;
	
	mpd_tag_type media_lib_primary_tag;
	
	bool enable_idle_notifications;
	bool colors_enabled;
	bool fancy_scrolling;
	bool playlist_show_remaining_time;
	bool playlist_shorten_total_times;
	bool playlist_separate_albums;
	bool columns_in_playlist;
	bool columns_in_browser;
	bool columns_in_search_engine;
	bool columns_in_playlist_editor;
	bool set_window_title;
	bool header_visibility;
	bool header_text_scrolling;
	bool statusbar_visibility;
	bool titles_visibility;
	bool centered_cursor;
	bool screen_switcher_previous;
	bool autocenter_mode;
	bool wrapped_search;
	bool space_selects;
	bool ncmpc_like_songs_adding;
	bool albums_in_tag_editor;
	bool incremental_seeking;
	bool now_playing_lyrics;
	bool fetch_lyrics_in_background;
	bool local_browser_show_hidden_files;
	bool search_in_db;
	bool display_screens_numbers_on_start;
	bool jump_to_now_playing_song_at_start;
	bool clock_display_seconds;
	bool display_volume_level;
	bool display_bitrate;
	bool display_remaining_time;
	bool ignore_leading_the;
	bool block_search_constraints_change;
	bool use_console_editor;
	bool use_cyclic_scrolling;
	bool allow_physical_files_deletion;
	bool allow_physical_directories_deletion;
	bool ask_before_clearing_main_playlist;
	bool mouse_support;
	bool mouse_list_scroll_whole_page;
	bool new_design;
	bool visualizer_use_wave;
	bool visualizer_in_stereo;
	bool browser_sort_by_mtime;
	bool tag_editor_extended_numeration;
	bool media_library_display_date;
	bool media_library_display_empty_tag;
	bool media_library_disable_two_column_mode;
	bool discard_colors_if_item_is_selected;
	bool store_lyrics_in_song_dir;
	bool ask_for_locked_screen_width_part;
	
	int mpd_port;
	int mpd_connection_timeout;
	int crossfade_time;
	int seek_time;
	int playlist_disable_highlight_delay;
	int message_delay_time;
	int lyrics_db;
	int regex_type;
	
	unsigned lines_scrolled;
	unsigned search_engine_default_search_mode;
	unsigned visualizer_sync_interval;
	
	double locked_screen_width_part;
	
	size_t selected_item_suffix_length;
	size_t now_playing_suffix_length;
	
	BasicScreen *startup_screen;
	std::list<BasicScreen *> screens_seq;
	
	private:
		void MakeProperPath(std::string &dir);
		
		std::string home_directory;
		std::string config_file_path;
};

extern NcmpcppKeys Key;
extern NcmpcppConfig Config;

void CreateDir(const std::string &dir);
void SetWindowsDimensions(size_t &header_height, size_t &footer_start_y, size_t &footer_height);

#endif

