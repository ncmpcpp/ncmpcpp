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

#ifdef WIN32
# include <io.h>
# define _WIN32_IE 0x0400
# include <shlobj.h>
#else
# include <sys/stat.h>
#endif // WIN32
#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <regex.h>

#include "actions.h"
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
#include "utility/type_conversions.h"

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

Configuration Config;

namespace
{
	ScreenRef intToScreen(int n)
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
				return ScreenRef();
		}
	}
	
	std::string GetOptionName(const std::string &s)
	{
		size_t equal = s.find('=');
		if (equal == std::string::npos)
			return "";
		std::string result = s.substr(0, equal);
		boost::trim(result);
		return result;
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

void CreateDir(const std::string &dir)
{
	mkdir(dir.c_str()
#	ifndef WIN32
	, 0755
#	endif // !WIN32
	);
}

void Configuration::SetDefaults()
{
	mpd_host = "localhost";
	empty_tag = "<empty>";
	tags_separator = " | ";
	song_list_columns_format = "(7f)[green]{l} (25)[cyan]{a} (40)[]{t|f} (30)[red]{b}";
	song_list_format = "{{%a - }{%t}|{$8%f$9}$R{$3(%l)$9}}";
	song_list_format_dollar_free = RemoveDollarFormatting(song_list_format);
	song_status_format = "{{{%a{ \"%b\"{ (%y)}} - }{%t}}|{%f}}";
	song_status_format_no_colors = song_status_format;
	song_window_title_format = "{{%a - }{%t}|{%f}}";
	song_library_format = "{{%n - }{%t}|{%f}}";
	browser_sort_format = "{{%a - }{%t}|{%f} {(%l)}}";
	tag_editor_album_format = "{{(%y) }%b}";
	new_header_first_line = "{$b$1$aqqu$/a$9 {%t}|{%f} $1$atqq$/a$9$/b}";
	new_header_second_line = "{{{$4$b%a$/b$9}{ - $7%b$9}{ ($4%y$9)}}|{%D}}";
	browser_playlist_prefix << NC::Color::Red << "playlist" << NC::Color::End << ' ';
	progressbar = L"=>\0";
	visualizer_chars = L"◆│";
	pattern = "%n - %t";
	selected_item_prefix << NC::Color::Magenta;
	selected_item_suffix << NC::Color::End;
	now_playing_prefix << NC::Format::Bold;
	now_playing_suffix << NC::Format::NoBold;
	modified_item_prefix << NC::Color::Green << "> " << NC::Color::End;
	color1 = NC::Color::White;
	color2 = NC::Color::Green;
	empty_tags_color = NC::Color::Cyan;
	header_color = NC::Color::Default;
	volume_color = NC::Color::Default;
	state_line_color = NC::Color::Default;
	state_flags_color = NC::Color::Default;
	main_color = NC::Color::Yellow;
	main_highlight_color = main_color;
	progressbar_color = NC::Color::Default;
	progressbar_elapsed_color = NC::Color::Default;
	statusbar_color = NC::Color::Default;
	alternative_ui_separator_color = NC::Color::Black;
	active_column_color = NC::Color::Red;
	window_border = NC::Border::Green;
	active_window_border = NC::Border::Red;
	visualizer_color = NC::Color::Yellow;
	media_lib_primary_tag = MPD_TAG_ARTIST;
	colors_enabled = true;
	playlist_show_remaining_time = false;
	playlist_shorten_total_times = false;
	playlist_separate_albums = false;
	columns_in_playlist = false;
	columns_in_browser = false;
	columns_in_search_engine = false;
	columns_in_playlist_editor = false;
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
	incremental_seeking = true;
	now_playing_lyrics = false;
	fetch_lyrics_in_background = false;
	local_browser_show_hidden_files = false;
	search_in_db = true;
	jump_to_now_playing_song_at_start = true;
	clock_display_seconds = false;
	display_volume_level = true;
	display_bitrate = false;
	display_remaining_time = false;
	ignore_leading_the = false;
	block_search_constraints_change = true;
	use_console_editor = false;
	use_cyclic_scrolling = false;
	ask_before_clearing_main_playlist = false;
	mouse_support = true;
	mouse_list_scroll_whole_page = true;
	new_design = false;
	visualizer_use_wave = true;
	visualizer_in_stereo = false;
	media_library_sort_by_mtime = false;
	tag_editor_extended_numeration = false;
	discard_colors_if_item_is_selected = true;
	store_lyrics_in_song_dir = false;
	generate_win32_compatible_filenames = true;
	ask_for_locked_screen_width_part = true;
	progressbar_boldness = true;
	set_window_title = true;
	mpd_port = 6600;
	mpd_connection_timeout = 15;
	crossfade_time = 5;
	seek_time = 1;
	volume_change_step = 1;
	playlist_disable_highlight_delay = 5;
	message_delay_time = 4;
	lyrics_db = 0;
	regex_type = boost::regex::literal | boost::regex::icase;
	lines_scrolled = 2;
	search_engine_default_search_mode = 0;
	visualizer_sync_interval = 30;
	locked_screen_width_part = 0.5;
	selected_item_prefix_length = 0;
	selected_item_suffix_length = 0;
	now_playing_suffix_length = 0;
#	ifdef HAVE_LANGINFO_H
	system_encoding = nl_langinfo(CODESET);
	if (system_encoding == "UTF-8") // mpd uses utf-8 by default so no need to convert
		system_encoding.clear();
#	endif // HAVE_LANGINFO_H
	startup_screen = myPlaylist;
	browser_sort_mode = smName;
	// default screens sequence
	screens_seq.push_back(myPlaylist);
	screens_seq.push_back(myBrowser);
}

Configuration::Configuration()
{
#	ifdef WIN32
	ncmpcpp_directory = GetHomeDirectory() + "ncmpcpp/";
	lyrics_directory = ncmpcpp_directory + "lyrics/";
#	else
	ncmpcpp_directory = GetHomeDirectory() + ".ncmpcpp/";
	lyrics_directory = GetHomeDirectory() + ".lyrics/";
#	endif // WIN32
	config_file_path = ncmpcpp_directory + "config";
}

const std::string &Configuration::GetHomeDirectory()
{
	if (!home_directory.empty())
		return home_directory;
#	ifdef WIN32
	char path[MAX_PATH];
	SHGetSpecialFolderPath(0, path, CSIDL_PERSONAL, 0);
	home_directory = path ? path : "";
	replace(home_directory.begin(), home_directory.end(), '\\', '/');
#	else
	char *home = getenv("HOME");
	home_directory = home ? home : "<unknown>";
#	endif // WIN32
	if (!home_directory.empty() && *home_directory.rbegin() != '/')
		home_directory += '/';
	return home_directory;
}

void Configuration::CheckForCommandLineConfigFilePath(char **argv, int argc)
{
	if (argc < 3)
		return;
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--config"))
		{
			if (++i >= argc)
				continue;
			config_file_path = argv[i];
		}
	}
}

void Configuration::Read()
{
	std::ifstream f(config_file_path.c_str());
	std::string cl, v, name;
	
	if (!f.is_open())
		return;
	
	while (!f.eof())
	{
		getline(f, cl);
		if (!cl.empty() && cl[0] != '#')
		{
			name = GetOptionName(cl);
			v = getEnclosedString(cl, '"', '"', 0);
 			
			if (name == "ncmpcpp_directory")
			{
				if (!v.empty())
				{
					MakeProperPath(v);
					ncmpcpp_directory = v;
				}
			}
			else if (name == "lyrics_directory")
			{
				if (!v.empty())
				{
					MakeProperPath(v);
					lyrics_directory = v;
				}
			}
			else if (name == "mpd_host")
			{
				if (!v.empty())
					mpd_host = v;
			}
			else if (name == "mpd_music_dir")
			{
				if (!v.empty())
				{
					MakeProperPath(v);
					mpd_music_dir = v;
				}
			}
			else if (name == "visualizer_fifo_path")
			{
				if (!v.empty())
					visualizer_fifo_path = v;
			}
			else if (name == "visualizer_output_name")
			{
				if (!v.empty())
					visualizer_output_name = v;
			}
			else if (name == "mpd_port")
			{
				if (boost::lexical_cast<int>(v))
					mpd_port = boost::lexical_cast<int>(v);
			}
			else if (name == "mpd_connection_timeout")
			{
				if (boost::lexical_cast<int>(v))
					mpd_connection_timeout = boost::lexical_cast<int>(v);
			}
			else if (name == "mpd_crossfade_time")
			{
				if (boost::lexical_cast<int>(v) > 0)
					crossfade_time = boost::lexical_cast<int>(v);
			}
			else if (name == "seek_time")
			{
				if (boost::lexical_cast<int>(v) > 0)
					seek_time = boost::lexical_cast<int>(v);
			}
			else if (name == "volume_change_step")
			{
				if (boost::lexical_cast<int>(v) > 0)
					volume_change_step = boost::lexical_cast<int>(v);
			}
			else if (name == "playlist_disable_highlight_delay")
			{
				if (boost::lexical_cast<int>(v) >= 0)
					playlist_disable_highlight_delay = boost::lexical_cast<int>(v);
			}
			else if (name == "message_delay_time")
			{
				if (boost::lexical_cast<int>(v) > 0)
					message_delay_time = boost::lexical_cast<int>(v);
			}
			else if (name == "song_list_format")
			{
				if (!v.empty() && MPD::Song::isFormatOk("song_list_format", v))
				{
					song_list_format = '{';
					song_list_format += v;
					song_list_format += '}';
					song_list_format_dollar_free = RemoveDollarFormatting(song_list_format);
				}
			}
			else if (name == "song_columns_list_format")
			{
				if (!v.empty())
					song_list_columns_format = v;
			}
			else if (name == "song_status_format")
			{
				if (!v.empty() && MPD::Song::isFormatOk("song_status_format", v))
				{
					song_status_format = '{';
					song_status_format += v;
					song_status_format += '}';
					// make version without colors
					if (song_status_format.find("$") != std::string::npos)
					{
						NC::Buffer status_no_colors;
						stringToBuffer(song_status_format, status_no_colors);
						song_status_format_no_colors = status_no_colors.str();
					}
					else
						song_status_format_no_colors = song_status_format;
				}
			}
			else if (name == "song_library_format")
			{
				if (!v.empty() && MPD::Song::isFormatOk("song_library_format", v))
				{
					song_library_format = '{';
					song_library_format += v;
					song_library_format += '}';
				}
			}
			else if (name == "tag_editor_album_format")
			{
				if (!v.empty() && MPD::Song::isFormatOk("tag_editor_album_format", v))
				{
					tag_editor_album_format = '{';
					tag_editor_album_format += v;
					tag_editor_album_format += '}';
				}
			}
			else if (name == "browser_sort_format")
			{
				if (!v.empty() && MPD::Song::isFormatOk("browser_sort_format", v))
				{
					browser_sort_format = '{';
					browser_sort_format += v;
					browser_sort_format += '}';
				}
			}
			else if (name == "external_editor")
			{
				if (!v.empty())
					external_editor = v;
			}
			else if (name == "system_encoding")
			{
				if (!v.empty())
					system_encoding = v;
			}
			else if (name == "execute_on_song_change")
			{
				if (!v.empty())
					execute_on_song_change = v;
			}
			else if (name == "alternative_header_first_line_format")
			{
				if (!v.empty() && MPD::Song::isFormatOk("alternative_header_first_line_format", v))
				{
					new_header_first_line = '{';
					new_header_first_line += v;
					new_header_first_line += '}';
				}
			}
			else if (name == "alternative_header_second_line_format")
			{
				if (!v.empty() && MPD::Song::isFormatOk("alternative_header_second_line_format", v))
				{
					new_header_second_line = '{';
					new_header_second_line += v;
					new_header_second_line += '}';
				}
			}
			else if (name == "lastfm_preferred_language")
			{
				if (!v.empty() && v != "en")
					lastfm_preferred_language = v;
			}
			else if (name == "browser_playlist_prefix")
			{
				if (!v.empty())
				{
					browser_playlist_prefix.clear();
					stringToBuffer(v, browser_playlist_prefix);
				}
			}
			else if (name == "progressbar_look")
			{
				std::wstring pb = ToWString(v);
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
			else if (name == "visualizer_look")
			{
				std::wstring vc = ToWString(v);
				if (vc.length() != 2)
				{
					std::cerr << "Warning: length of visualizer_look should be 2, but it's " << vc.length() << ", discarding.\n";
				}
				else
					visualizer_chars = vc;
			}
			else if (name == "default_tag_editor_pattern")
			{
				if (!v.empty())
					pattern = v;
			}
			else if (name == "selected_item_prefix")
			{
				if (!v.empty())
				{
					selected_item_prefix.clear();
					stringToBuffer(v, selected_item_prefix);
					selected_item_prefix_length = wideLength(ToWString(selected_item_prefix.str()));
				}
			}
			else if (name == "selected_item_suffix")
			{
				if (!v.empty())
				{
					selected_item_suffix.clear();
					stringToBuffer(v, selected_item_suffix);
					selected_item_suffix_length = wideLength(ToWString(selected_item_suffix.str()));
				}
			}
			else if (name == "now_playing_prefix")
			{
				if (!v.empty())
				{
					now_playing_prefix.clear();
					stringToBuffer(v, now_playing_prefix);
					now_playing_prefix_length = wideLength(ToWString(now_playing_prefix.str()));
				}
			}
			else if (name == "now_playing_suffix")
			{
				if (!v.empty())
				{
					now_playing_suffix.clear();
					stringToBuffer(v, now_playing_suffix);
					now_playing_suffix_length = wideLength(ToWString(now_playing_suffix.str()));
				}
			}
			else if (name == "modified_item_prefix")
			{
				if (!v.empty())
				{
					modified_item_prefix.clear();
					stringToBuffer(v, modified_item_prefix);
				}
			}
			else if (name == "color1")
			{
				if (!v.empty())
					color1 = stringToColor(v);
			}
			else if (name == "color2")
			{
				if (!v.empty())
					color2 = stringToColor(v);
			}
			else if (name == "colors_enabled")
			{
				colors_enabled = v == "yes";
			}
			else if (name == "cyclic_scrolling")
			{
				use_cyclic_scrolling = v == "yes";
			}
			else if (name == "playlist_show_remaining_time")
			{
				playlist_show_remaining_time = v == "yes";
			}
			else if (name == "playlist_shorten_total_times")
			{
				playlist_shorten_total_times = v == "yes";
			}
			else if (name == "playlist_separate_albums")
			{
				playlist_separate_albums = v == "yes";
			}
			else if (name == "playlist_display_mode")
			{
				columns_in_playlist = v == "columns";
			}
			else if (name == "browser_display_mode")
			{
				columns_in_browser = v == "columns";
			}
			else if (name == "search_engine_display_mode")
			{
				columns_in_search_engine = v == "columns";
			}
			else if (name == "playlist_editor_display_mode")
			{
				columns_in_playlist_editor = v == "columns";
			}
			else if (name == "header_visibility")
			{
				header_visibility = v == "yes";
			}
			else if (name == "header_text_scrolling")
			{
				header_text_scrolling = v == "yes";
			}
			else if (name == "statusbar_visibility")
			{
				statusbar_visibility = v == "yes";
			}
			else if (name == "titles_visibility")
			{
				titles_visibility = v == "yes";
			}
			else if (name == "screen_switcher_mode")
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
						if (auto screen = intToScreen(atoi(&*it)))
							screens_seq.push_back(screen);
						while (it != v.end() && isdigit(*it))
							++it;
					}
					// throw away duplicates
					screens_seq.unique();
				}
			}
			else if (name == "startup_screen")
			{
				startup_screen = intToScreen(atoi(v.c_str()));
				if (!startup_screen)
					startup_screen = myPlaylist;
			}
			else if (name == "autocenter_mode")
			{
				autocenter_mode = v == "yes";
			}
			else if (name == "centered_cursor")
			{
				centered_cursor = v == "yes";
			}
			else if (name == "default_find_mode")
			{
				wrapped_search = v == "wrapped";
			}
			else if (name == "default_space_mode")
			{
				space_selects = v == "select";
			}
			else if (name == "incremental_seeking")
			{
				incremental_seeking = v == "yes";
			}
			else if (name == "show_hidden_files_in_local_browser")
			{
				local_browser_show_hidden_files = v == "yes";
			}
			else if (name == "follow_now_playing_lyrics")
			{
				now_playing_lyrics = v == "yes";
			}
			else if (name == "fetch_lyrics_for_current_song_in_background")
			{
				fetch_lyrics_in_background = v == "yes";
			}
			else if (name == "ncmpc_like_songs_adding")
			{
				ncmpc_like_songs_adding = v == "yes";
			}
			else if (name == "default_place_to_search_in")
			{
				search_in_db = v == "database";
			}
			else if (name == "jump_to_now_playing_song_at_start")
			{
				jump_to_now_playing_song_at_start = v == "yes";
			}
			else if (name == "clock_display_seconds")
			{
				clock_display_seconds = v == "yes";
			}
			else if (name == "display_volume_level")
			{
				display_volume_level = v == "yes";
			}
			else if (name == "display_bitrate")
			{
				display_bitrate = v == "yes";
			}
			else if (name == "display_remaining_time")
			{
				display_remaining_time = v == "yes";
			}
			else if (name == "ignore_leading_the")
			{
				ignore_leading_the = v == "yes";
			}
			else if (name == "use_console_editor")
			{
				use_console_editor = v == "yes";
			}
			else if (name == "block_search_constraints_change_if_items_found")
			{
				block_search_constraints_change = v == "yes";
			}
			else if (name == "ask_before_clearing_main_playlist")
			{
				ask_before_clearing_main_playlist = v == "yes";
			}
			else if (name == "visualizer_type")
			{
				visualizer_use_wave = v == "wave";
			}
			else if (name == "visualizer_in_stereo")
			{
				visualizer_in_stereo = v == "yes";
			}
			else if (name == "mouse_support")
			{
				mouse_support = v == "yes";
			}
			else if (name == "mouse_list_scroll_whole_page")
			{
				mouse_list_scroll_whole_page = v == "yes";
			}
			else if (name == "user_interface")
			{
				new_design = v == "alternative";
			}
			else if (name == "tag_editor_extended_numeration")
			{
				tag_editor_extended_numeration = v == "yes";
			}
			else if (name == "discard_colors_if_item_is_selected")
			{
				discard_colors_if_item_is_selected = v == "yes";
			}
			else if (name == "store_lyrics_in_song_dir")
			{
				if (mpd_music_dir.empty())
				{
					std::cerr << "Warning: store_lyrics_in_song_dir = \"yes\" is ";
					std::cerr << "not allowed without mpd_music_dir set, discarding.\n";
				}
				else
					store_lyrics_in_song_dir = v == "yes";
			}
			else if (name == "generate_win32_compatible_filenames")
			{
				generate_win32_compatible_filenames = v == "yes";
			}
			else if (name == "enable_window_title")
			{
				set_window_title = v == "yes";
			}
			else if (name == "regular_expressions")
			{
				if (v == "none")
					regex_type = boost::regex::literal | boost::regex::icase;
				else if (v == "basic")
					regex_type = boost::regex::basic | boost::regex::icase;
				else if (v == "extended")
					regex_type = boost::regex::extended | boost::regex::icase;
			}
			else if (name == "lines_scrolled")
			{
				if (!v.empty())
					lines_scrolled = boost::lexical_cast<int>(v);
			}
			else if (name == "search_engine_default_search_mode")
			{
				if (!v.empty())
				{
					unsigned mode = boost::lexical_cast<unsigned>(v);
					if (--mode < 3)
						search_engine_default_search_mode = mode;
				}
			}
			else if (name == "visualizer_sync_interval")
			{
				unsigned interval = boost::lexical_cast<unsigned>(v);
				if (interval)
					visualizer_sync_interval = interval;
			}
			else if (name == "browser_sort_mode")
			{
				if (v == "mtime")
					browser_sort_mode = smMTime;
				else if (v == "format")
					browser_sort_mode = smCustomFormat;
				else if (v == "unsorted")
					browser_sort_mode = smUnsorted;
				else
					browser_sort_mode = smName; // "name" or invalid
			}
			else if (name == "locked_screen_width_part")
			{
				int part = boost::lexical_cast<int>(v);
				if (part)
					locked_screen_width_part = part/100.0;
			}
			else if (name == "ask_for_locked_screen_width_part")
			{
				if (!v.empty())
					ask_for_locked_screen_width_part = v == "yes";
			}
			else if (name == "progressbar_boldness")
			{
				if (!v.empty())
					progressbar_boldness = v == "yes";
			}
			else if (name == "song_window_title_format")
			{
				if (!v.empty() && MPD::Song::isFormatOk("song_window_title_format", v))
				{
					song_window_title_format = '{';
					song_window_title_format += v;
					song_window_title_format += '}';
				}
			}
			else if (name == "empty_tag_marker")
			{
				empty_tag = v; // is this case empty string is allowed
			}
			else if (name == "tags_separator")
			{
				if (!v.empty())
					tags_separator = v;
			}
			else if (name == "empty_tag_color")
			{
				if (!v.empty())
					empty_tags_color = stringToColor(v);
			}
			else if (name == "header_window_color")
			{
				if (!v.empty())
					header_color = stringToColor(v);
			}
			else if (name == "volume_color")
			{
				if (!v.empty())
					volume_color = stringToColor(v);
			}
			else if (name == "state_line_color")
			{
				if (!v.empty())
					state_line_color = stringToColor(v);
			}
			else if (name == "state_flags_color")
			{
				if (!v.empty())
					state_flags_color = stringToColor(v);
			}
			else if (name == "main_window_color")
			{
				if (!v.empty())
					main_color = stringToColor(v);
			}
			else if (name == "main_window_highlight_color")
			{
				if (!v.empty())
					main_highlight_color = stringToColor(v);
			}
			else if (name == "progressbar_color")
			{
				if (!v.empty())
					progressbar_color = stringToColor(v);
			}
			else if (name == "progressbar_elapsed_color")
			{
				if (!v.empty())
					progressbar_elapsed_color = stringToColor(v);
			}
			else if (name == "statusbar_color")
			{
				if (!v.empty())
					statusbar_color = stringToColor(v);
			}
			else if (name == "alternative_ui_separator_color")
			{
				if (!v.empty())
					alternative_ui_separator_color = stringToColor(v);
			}
			else if (name == "active_column_color")
			{
				if (!v.empty())
					active_column_color = stringToColor(v);
			}
			else if (name == "visualizer_color")
			{
				if (!v.empty())
					visualizer_color = stringToColor(v);
			}
			else if (name == "window_border_color")
			{
				if (!v.empty())
					window_border = stringToBorder(v);
			}
			else if (name == "active_window_border")
			{
				if (!v.empty())
					active_window_border = stringToBorder(v);
			}
			else if (name == "media_library_left_column")
			{
				if (!v.empty())
					media_lib_primary_tag = charToTagType(v[0]);
			}
			else if (name == "media_library_sort_by_mtime")
			{
				media_library_sort_by_mtime = v == "yes";
			}
			else
				std::cout << "Unknown option: " << name << ", ignoring.\n";
		}
	}
	f.close();
}

void Configuration::GenerateColumns()
{
	columns.clear();
	std::string width;
	size_t pos = 0;
	while (!(width = getEnclosedString(song_list_columns_format, '(', ')', &pos)).empty())
	{
		Column col;
		col.color = stringToColor(getEnclosedString(song_list_columns_format, '[', ']', &pos));
		std::string tag_type = getEnclosedString(song_list_columns_format, '{', '}', &pos);
		
		if (*width.rbegin() == 'f')
		{
			col.fixed = true;
			width.resize(width.size()-1);
		}
		else
			col.fixed = false;
		
		// alternative name
		size_t tag_type_colon_pos = tag_type.find(':');
		if (tag_type_colon_pos != std::string::npos)
		{
			col.name = ToWString(tag_type.substr(tag_type_colon_pos+1));
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
		
		col.width = boost::lexical_cast<int>(width);
		columns.push_back(col);
	}
	
	// calculate which column is the last one to have relative width and stretch it accordingly
	if (!columns.empty())
	{
		int stretch_limit = 0;
		auto it = columns.rbegin();
		for (; it != columns.rend(); ++it)
		{
			if (it->fixed)
				stretch_limit += it->width;
			else
				break;
		}
		// if it's false, all columns are fixed
		if (it != columns.rend())
			it->stretch_limit = stretch_limit;
	}
	
	// generate format for converting tags in columns to string for Playlist::SongInColumnsToString()
	char tag[] = "{% }|";
	song_in_columns_to_string_format = "{";
	for (auto it = columns.begin(); it != columns.end(); ++it)
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

void Configuration::MakeProperPath(std::string &dir)
{
	if (dir.length() < 2)
		return;
	if (dir[0] == '~' && dir[1] == '/')
		dir.replace(0, 2, home_directory);
	std::replace(dir.begin(), dir.end(), '\\', '/');
	if (*dir.rbegin() != '/')
		dir += '/';
}
