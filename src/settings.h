/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <cassert>
#include <vector>
#include <mpd/client.h>

#include "curses/formatted_color.h"
#include "curses/strbuffer.h"
#include "enums.h"
#include "format.h"
#include "lyrics_fetcher.h"
#include "screens/screen_type.h"

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

struct Configuration
{
	Configuration()
	: playlist_disable_highlight_delay(0), visualizer_sync_interval(0)
	{ }

	bool read(const std::vector<std::string> &config_paths, bool ignore_errors);

	std::string ncmpcpp_directory;
	std::string lyrics_directory;

	std::string mpd_music_dir;
	std::string visualizer_fifo_path;
	std::string visualizer_output_name;
	std::string empty_tag;

	Format::AST<char> song_list_format;
	Format::AST<char> song_window_title_format;
	Format::AST<char> song_library_format;
	Format::AST<char> song_columns_mode_format;
	Format::AST<char> browser_sort_format;
	Format::AST<char> song_status_format;
	Format::AST<wchar_t> song_status_wformat;
	Format::AST<wchar_t> new_header_first_line;
	Format::AST<wchar_t> new_header_second_line;

	std::string external_editor;
	std::string system_encoding;
	std::string execute_on_song_change;
	std::string execute_on_player_state_change;
	std::string lastfm_preferred_language;
	std::wstring progressbar;
	std::wstring visualizer_chars;

	std::string pattern;

	std::vector<Column> columns;

	DisplayMode playlist_display_mode;
	DisplayMode browser_display_mode;
	DisplayMode search_engine_display_mode;
	DisplayMode playlist_editor_display_mode;

	NC::Buffer browser_playlist_prefix;
	NC::Buffer selected_item_prefix;
	NC::Buffer selected_item_suffix;
	NC::Buffer now_playing_prefix;
	NC::Buffer now_playing_suffix;
	NC::Buffer modified_item_prefix;
	NC::Buffer current_item_prefix;
	NC::Buffer current_item_suffix;
	NC::Buffer current_item_inactive_column_prefix;
	NC::Buffer current_item_inactive_column_suffix;

	NC::Color header_color;
	NC::Color main_color;
	NC::Color statusbar_color;

	NC::FormattedColor color1;
	NC::FormattedColor color2;
	NC::FormattedColor empty_tags_color;
	NC::FormattedColor volume_color;
	NC::FormattedColor state_line_color;
	NC::FormattedColor state_flags_color;
	NC::FormattedColor progressbar_color;
	NC::FormattedColor progressbar_elapsed_color;
	NC::FormattedColor player_state_color;
	NC::FormattedColor statusbar_time_color;
	NC::FormattedColor alternative_ui_separator_color;

	std::vector<NC::FormattedColor> visualizer_colors;
	VisualizerType visualizer_type;

	NC::Border window_border;
	NC::Border active_window_border;

	Design design;

	SpaceAddMode space_add_mode;

	mpd_tag_type media_lib_primary_tag;

	bool colors_enabled;
	bool playlist_show_mpd_host;
	bool playlist_show_remaining_time;
	bool playlist_shorten_total_times;
	bool playlist_separate_albums;
	bool set_window_title;
	bool header_visibility;
	bool header_text_scrolling;
	bool statusbar_visibility;
	bool titles_visibility;
	bool centered_cursor;
	bool screen_switcher_previous;
	bool autocenter_mode;
	bool wrapped_search;
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
	bool ignore_diacritics;
	bool block_search_constraints_change;
	bool use_console_editor;
	bool use_cyclic_scrolling;
	bool ask_before_clearing_playlists;
	bool ask_before_shuffling_playlists;
	bool mouse_support;
	bool mouse_list_scroll_whole_page;
	bool visualizer_in_stereo;
	bool data_fetching_delay;
	bool media_library_sort_by_mtime;
	bool tag_editor_extended_numeration;
	bool discard_colors_if_item_is_selected;
	bool store_lyrics_in_song_dir;
	bool generate_win32_compatible_filenames;
	bool ask_for_locked_screen_width_part;
	bool allow_for_physical_item_deletion;
	bool media_library_albums_split_by_date;
	bool startup_slave_screen_focus;

	unsigned mpd_connection_timeout;
	unsigned crossfade_time;
	unsigned seek_time;
	unsigned volume_change_step;
	unsigned message_delay_time;
	unsigned lyrics_db;
	unsigned lines_scrolled;
	unsigned search_engine_default_search_mode;

	boost::regex::flag_type regex_type;

	boost::posix_time::seconds playlist_disable_highlight_delay;
	boost::posix_time::seconds visualizer_sync_interval;

	double locked_screen_width_part;

	size_t selected_item_prefix_length;
	size_t selected_item_suffix_length;
	size_t now_playing_prefix_length;
	size_t now_playing_suffix_length;
	size_t current_item_prefix_length;
	size_t current_item_suffix_length;
	size_t current_item_inactive_column_prefix_length;
	size_t current_item_inactive_column_suffix_length;

	ScreenType startup_screen_type;
	boost::optional<ScreenType> startup_slave_screen_type;
	std::vector<ScreenType> screen_sequence;

	SortMode browser_sort_mode;

	LyricsFetchers lyrics_fetchers;
};

extern Configuration Config;

#endif // NCMPCPP_SETTINGS_H
