/***************************************************************************
 *   Copyright (C) 2008-2013 by Andrzej Rybczak                            *
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

#ifndef NCMPCPP_SETTINGS_H
#define NCMPCPP_SETTINGS_H

#include <boost/regex.hpp>
#include <cassert>
#include <vector>
#include <mpd/client.h>
#include "actions.h"
#include "strbuffer.h"

struct BaseScreen; // forward declaration for screens sequence

enum SortMode { smName, smMTime, smCustomFormat, smUnsorted };

struct Column
{
	Column() : stretch_limit(-1), right_alignment(0), display_empty_tag(1) { }
	
	std::wstring name;
	std::string type;
	int width;
	int stretch_limit;
	NC::Color color;
	bool fixed;
	bool right_alignment;
	bool display_empty_tag;
};

// FIXME: temporary hack
struct ScreenRef
{
	ScreenRef() : m_ptr(0) { }
	template <typename ScreenT>
	ScreenRef(ScreenT *&ptr) : m_ptr(reinterpret_cast<BaseScreen **>(&ptr)) { }
	
	BaseScreen &operator*() const { return **m_ptr; }
	BaseScreen *operator->() const { return *m_ptr; }
	
	bool operator==(const ScreenRef &rhs) const { return m_ptr == rhs.m_ptr; }
	bool operator!=(const ScreenRef &rhs) const { return m_ptr != rhs.m_ptr; }
	bool operator==(const BaseScreen *rhs) const { return *m_ptr == rhs; }
	bool operator!=(const BaseScreen *rhs) const { return *m_ptr != rhs; }
	
	operator bool() { return m_ptr != 0; }
	
private:
	BaseScreen **m_ptr;
};

struct Configuration
{
	Configuration();
	
	const std::string &GetHomeDirectory();
	void CheckForCommandLineConfigFilePath(char **argv, int argc);
	
	void SetDefaults();
	void Read();
	void GenerateColumns();
	
	std::string ncmpcpp_directory;
	std::string lyrics_directory;
	
	std::string mpd_host;
	std::string mpd_music_dir;
	std::string visualizer_fifo_path;
	std::string visualizer_output_name;
	std::string empty_tag;
	std::string tags_separator;
	std::string song_list_columns_format;
	std::string song_list_format;
	std::string song_list_format_dollar_free;
	std::string song_status_format;
	std::string song_status_format_no_colors;
	std::string song_window_title_format;
	std::string song_library_format;
	std::string tag_editor_album_format;
	std::string song_in_columns_to_string_format;
	std::string browser_sort_format;
	std::string external_editor;
	std::string system_encoding;
	std::string execute_on_song_change;
	std::string new_header_first_line;
	std::string new_header_second_line;
	std::string lastfm_preferred_language;
	std::wstring progressbar;
	std::wstring visualizer_chars;
	
	std::string pattern;
	
	std::vector<Column> columns;
	
	NC::Buffer browser_playlist_prefix;
	NC::Buffer selected_item_prefix;
	NC::Buffer selected_item_suffix;
	NC::Buffer now_playing_prefix;
	NC::Buffer now_playing_suffix;
	NC::Buffer modified_item_prefix;
	
	NC::Color color1;
	NC::Color color2;
	NC::Color empty_tags_color;
	NC::Color header_color;
	NC::Color volume_color;
	NC::Color state_line_color;
	NC::Color state_flags_color;
	NC::Color main_color;
	NC::Color main_highlight_color;
	NC::Color progressbar_color;
	NC::Color progressbar_elapsed_color;
	NC::Color statusbar_color;
	NC::Color alternative_ui_separator_color;
	NC::Color active_column_color;
	NC::Color visualizer_color;
	
	NC::Border window_border;
	NC::Border active_window_border;
	
	mpd_tag_type media_lib_primary_tag;
	
	bool colors_enabled;
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
	bool incremental_seeking;
	bool now_playing_lyrics;
	bool fetch_lyrics_in_background;
	bool local_browser_show_hidden_files;
	bool search_in_db;
	bool jump_to_now_playing_song_at_start;
	bool clock_display_seconds;
	bool display_volume_level;
	bool display_bitrate;
	bool display_remaining_time;
	bool ignore_leading_the;
	bool block_search_constraints_change;
	bool use_console_editor;
	bool use_cyclic_scrolling;
	bool ask_before_clearing_main_playlist;
	bool mouse_support;
	bool mouse_list_scroll_whole_page;
	bool new_design;
	bool visualizer_use_wave;
	bool visualizer_in_stereo;
	bool media_library_sort_by_mtime;
	bool tag_editor_extended_numeration;
	bool discard_colors_if_item_is_selected;
	bool store_lyrics_in_song_dir;
	bool generate_win32_compatible_filenames;
	bool ask_for_locked_screen_width_part;
	bool progressbar_boldness;
	
	int mpd_port;
	int mpd_connection_timeout;
	int crossfade_time;
	int seek_time;
	int volume_change_step;
	int playlist_disable_highlight_delay;
	int message_delay_time;
	int lyrics_db;
	
	boost::regex::flag_type regex_type;
	
	unsigned lines_scrolled;
	unsigned search_engine_default_search_mode;
	unsigned visualizer_sync_interval;
	
	double locked_screen_width_part;
	
	size_t selected_item_prefix_length;
	size_t selected_item_suffix_length;
	size_t now_playing_prefix_length;
	size_t now_playing_suffix_length;
	
	ScreenRef startup_screen;
	std::list<ScreenRef> screens_seq;
	
	SortMode browser_sort_mode;
	
private:
	void MakeProperPath(std::string &dir);
	
	std::string home_directory;
	std::string config_file_path;
};

extern Configuration Config;

void CreateDir(const std::string &dir);

#endif // NCMPCPP_SETTINGS_H

