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

#include <boost/tuple/tuple.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <stdexcept>

#include "configuration.h"
#include "format_impl.h"
#include "helpers.h"
#include "settings.h"
#include "utility/conversion.h"
#include "utility/option_parser.h"
#include "utility/type_conversions.h"

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

namespace ph = std::placeholders;

Configuration Config;

namespace {

std::vector<Column> generate_columns(const std::string &format)
{
	std::vector<Column> result;
	std::string width;
	size_t pos = 0;
	while (!(width = getEnclosedString(format, '(', ')', &pos)).empty())
	{
		Column col;
		auto scolor = getEnclosedString(format, '[', ']', &pos);
		if (scolor.empty())
			col.color = NC::Color::Default;
		else
			col.color = boost::lexical_cast<NC::Color>(scolor);

		if (*width.rbegin() == 'f')
		{
			col.fixed = true;
			width.resize(width.size()-1);
		}
		else
			col.fixed = false;

		auto tag_type = getEnclosedString(format, '{', '}', &pos);
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
		result.push_back(col);
	}

	// calculate which column is the last one to have relative width and stretch it accordingly
	if (!result.empty())
	{
		int stretch_limit = 0;
		auto it = result.rbegin();
		for (; it != result.rend(); ++it)
		{
			if (it->fixed)
				stretch_limit += it->width;
			else
				break;
		}
		// if it's false, all columns are fixed
		if (it != result.rend())
			it->stretch_limit = stretch_limit;
	}

	return result;
}

Format::AST<char> columns_to_format(const std::vector<Column> &columns)
{
	std::vector<Format::Expression<char>> result;

	auto column = columns.begin();
	while (true)
	{
		Format::FirstOf<char> first_of;
		for (const auto &type : column->type)
		{
			auto f = charToGetFunction(type);
			assert(f != nullptr);
			first_of.base().push_back(f);
		}
		result.push_back(std::move(first_of));

		if (++column != columns.end())
			result.push_back(" ");
		else
			break;
	}

	return Format::AST<char>(std::move(result));
}

void add_slash_at_the_end(std::string &s)
{
	if (s.empty() || *s.rbegin() != '/')
	{
		s.resize(s.size()+1);
		*s.rbegin() = '/';
	}
}

std::string adjust_directory(std::string s)
{
	add_slash_at_the_end(s);
	expand_home(s);
	return s;
}

std::string adjust_path(std::string s)
{
	expand_home(s);
	return s;
}

NC::Buffer buffer(const std::string &v)
{
	NC::Buffer result;
	Format::print(
		Format::parse(v, Format::Flags::Color | Format::Flags::Format),
		result,
		nullptr);
	return result;
}

NC::Buffer buffer_wlength(const NC::Buffer *target,
                          size_t &wlength,
                          const std::string &v)
{
	// Compatibility layer between highlight color and new highlight prefix and
	// suffix. Basically, for older configurations if highlight color is provided,
	// we don't want to override it with default prefix and suffix.
	if (target == nullptr || target->empty())
	{
		NC::Buffer result = buffer(v);
		wlength = wideLength(ToWString(result.str()));
		return result;
	}
	else
		return *target;
}

void deprecated(const char *option, double version_removal, const std::string &advice)
{
	std::cerr << "WARNING: Variable '" << option
	          << "' is deprecated and will be removed in "
	          << version_removal;
	if (!advice.empty())
		std::cerr << " (" << advice << ")";
	std::cerr << ".\n";
}

}

bool Configuration::read(const std::vector<std::string> &config_paths, bool ignore_errors)
{
	option_parser p;

	// Deprecated options.
	p.add<void>("visualizer_sample_multiplier", nullptr, "", [](std::string v) {
			if (!v.empty())
				deprecated(
					"visualizer_sample_multiplier",
					0.9,
					"visualizer scales automatically");
		});
	p.add<void>("progressbar_boldness", nullptr, "", [](std::string v) {
			if (!v.empty())
				deprecated(
					"progressbar_boldness",
					0.9,
					"use extended progressbar_color and progressbar_elapsed_color instead");
		});

	p.add<void>("main_window_highlight_color", nullptr, "", [this](std::string v) {
			if (!v.empty())
			{
				const std::string current_item_prefix_str = "$(" + v + ")$r";
				const std::string current_item_suffix_str = "$/r$(end)";
				current_item_prefix = buffer_wlength(
					nullptr,
					current_item_prefix_length,
					current_item_prefix_str);
				current_item_suffix = buffer_wlength(
					nullptr,
					current_item_suffix_length,
					current_item_suffix_str);
				deprecated(
					"main_window_highlight_color",
					0.9,
					"set current_item_prefix = \""
					+ current_item_prefix_str
					+ "\" and current_item_suffix = \""
					+ current_item_suffix_str
					+ "\" to preserve current behavior");
			};
		});
	p.add<void>("active_column_color", nullptr, "", [this](std::string v) {
			if (!v.empty())
			{
				deprecated(
					"active_column_color",
					0.9,
					"replaced by current_item_inactive_column_prefix"
					" and current_item_inactive_column_suffix");
			};
		});

	// keep the same order of variables as in configuration file
	p.add("ncmpcpp_directory", &ncmpcpp_directory, "~/.ncmpcpp/", adjust_directory);
	p.add("lyrics_directory", &lyrics_directory, "~/.lyrics/", adjust_directory);
	p.add<void>("mpd_host", nullptr, "localhost", [](std::string host) {
			expand_home(host);
			Mpd.SetHostname(host);
		});
	p.add<void>("mpd_port", nullptr, "6600", [](std::string port) {
			Mpd.SetPort(verbose_lexical_cast<unsigned>(port));
		});
	p.add("mpd_music_dir", &mpd_music_dir, "~/music", adjust_directory);
	p.add("mpd_connection_timeout", &mpd_connection_timeout, "5");
	p.add("mpd_crossfade_time", &crossfade_time, "5");
	p.add("random_exclude_pattern", &random_exclude_pattern, "");
	p.add("visualizer_fifo_path", &visualizer_fifo_path, "/tmp/mpd.fifo", adjust_path);
	p.add("visualizer_output_name", &visualizer_output_name, "Visualizer feed");
	p.add("visualizer_in_stereo", &visualizer_in_stereo, "yes", yes_no);
	p.add("visualizer_sync_interval", &visualizer_sync_interval, "30",
	      [](std::string v) {
		      unsigned sync_interval = verbose_lexical_cast<unsigned>(v);
		      lowerBoundCheck<unsigned>(sync_interval, 10);
		      return boost::posix_time::seconds(sync_interval);
	});
	p.add("visualizer_type", &visualizer_type, "wave");
	p.add("visualizer_look", &visualizer_chars, "●▮", [](std::string s) {
			auto result = ToWString(std::move(s));
			boundsCheck<std::wstring::size_type>(result.size(), 2, 2);
			return result;
	});
	p.add("visualizer_color", &visualizer_colors,
	      "blue, cyan, green, yellow, magenta, red", list_of<NC::FormattedColor>);
	p.add("system_encoding", &system_encoding, "", [](std::string encoding) {
#ifdef HAVE_LANGINFO_H
			// try to autodetect system encoding
			if (encoding.empty())
			{
				encoding = nl_langinfo(CODESET);
				if (encoding == "UTF-8") // mpd uses utf-8 by default so no need to convert
					encoding.clear();
			}
#endif // HAVE_LANGINFO_H
			return encoding;
		});
	p.add("playlist_disable_highlight_delay", &playlist_disable_highlight_delay,
	      "5", [](std::string v) {
		      return boost::posix_time::seconds(verbose_lexical_cast<unsigned>(v));
	      });
	p.add("message_delay_time", &message_delay_time, "5");
	p.add("song_list_format", &song_list_format,
	      "{%a - }{%t}|{$8%f$9}$R{$3(%l)$9}", [](std::string v) {
		      return Format::parse(v);
	      });
	p.add("song_status_format", &song_status_format,
	      "{{%a{ \"%b\"{ (%y)}} - }{%t}}|{%f}", [this](std::string v) {
		      auto flags = Format::Flags::All ^ Format::Flags::OutputSwitch;
		      // precompute wide format for status display
		      song_status_wformat = Format::parse(ToWString(v), flags);
		      return Format::parse(v, flags);
	});
	p.add("song_library_format", &song_library_format,
	      "{%n - }{%t}|{%f}", [](std::string v) {
		      return Format::parse(v);
	      });
	p.add("alternative_header_first_line_format", &new_header_first_line,
	      "$b$1$aqqu$/a$9 {%t}|{%f} $1$atqq$/a$9$/b", [](std::string v) {
		      return Format::parse(ToWString(std::move(v)),
		                           Format::Flags::All ^ Format::Flags::OutputSwitch);
	});
	p.add("alternative_header_second_line_format", &new_header_second_line,
	      "{{$4$b%a$/b$9}{ - $7%b$9}{ ($4%y$9)}}|{%D}", [](std::string v) {
		      return Format::parse(ToWString(std::move(v)),
		                           Format::Flags::All ^ Format::Flags::OutputSwitch);
	});
	p.add("current_item_prefix", &current_item_prefix, "$(yellow)$r",
	      std::bind(buffer_wlength,
	                &current_item_prefix,
	                std::ref(current_item_prefix_length),
	                ph::_1));
	p.add("current_item_suffix", &current_item_suffix, "$/r$(end)",
	      std::bind(buffer_wlength,
	                &current_item_suffix,
	                std::ref(current_item_suffix_length),
	                ph::_1));
	p.add("current_item_inactive_column_prefix", &current_item_inactive_column_prefix,
	      "$(white)$r",
	      std::bind(buffer_wlength,
	                &current_item_inactive_column_prefix,
	                std::ref(current_item_inactive_column_prefix_length),
	                ph::_1));
	p.add("current_item_inactive_column_suffix", &current_item_inactive_column_suffix,
	      "$/r$(end)",
	      std::bind(buffer_wlength,
	                &current_item_inactive_column_suffix,
	                std::ref(current_item_inactive_column_suffix_length),
	                ph::_1));
	p.add("now_playing_prefix", &now_playing_prefix, "$b",
	      std::bind(buffer_wlength,
	                nullptr,
	                std::ref(now_playing_prefix_length),
	                ph::_1));
	p.add("now_playing_suffix", &now_playing_suffix, "$/b",
	      std::bind(buffer_wlength,
	                nullptr,
	                std::ref(now_playing_suffix_length),
	                ph::_1));
	p.add("browser_playlist_prefix", &browser_playlist_prefix, "$2playlist$9 ", buffer);
	p.add("selected_item_prefix", &selected_item_prefix, "$6",
	      std::bind(buffer_wlength,
	                nullptr,
	                std::ref(selected_item_prefix_length),
	                ph::_1));
	p.add("selected_item_suffix", &selected_item_suffix, "$9",
	      std::bind(buffer_wlength,
	                nullptr,
	                std::ref(selected_item_suffix_length),
	                ph::_1));
	p.add("modified_item_prefix", &modified_item_prefix, "$3>$9 ", buffer);
	p.add("song_window_title_format", &song_window_title_format,
	      "{%a - }{%t}|{%f}", [](std::string v) {
		      return Format::parse(v, Format::Flags::Tag);
	      });
	p.add("browser_sort_mode", &browser_sort_mode, "name");
	p.add("browser_sort_format", &browser_sort_format,
	      "{%a - }{%t}|{%f} {(%l)}", [](std::string v) {
		      return Format::parse(v, Format::Flags::Tag);
	      });
	p.add("song_columns_list_format", &song_columns_mode_format,
	      "(20)[]{a} (6f)[green]{NE} (50)[white]{t|f:Title} (20)[cyan]{b} (7f)[magenta]{l}",
	      [this](std::string v) {
		      columns = generate_columns(v);
		      return columns_to_format(columns);
	      });
	p.add("execute_on_song_change", &execute_on_song_change, "", adjust_path);
	p.add("execute_on_player_state_change", &execute_on_player_state_change,
	      "", adjust_path);
	p.add("playlist_show_mpd_host", &playlist_show_mpd_host, "no", yes_no);
	p.add("playlist_show_remaining_time", &playlist_show_remaining_time, "no", yes_no);
	p.add("playlist_shorten_total_times", &playlist_shorten_total_times, "no", yes_no);
	p.add("playlist_separate_albums", &playlist_separate_albums, "no", yes_no);
	p.add("playlist_display_mode", &playlist_display_mode, "columns");
	p.add("browser_display_mode", &browser_display_mode, "classic");
	p.add("search_engine_display_mode", &search_engine_display_mode, "classic");
	p.add("playlist_editor_display_mode", &playlist_editor_display_mode, "classic");
	p.add("discard_colors_if_item_is_selected", &discard_colors_if_item_is_selected,
	      "yes", yes_no);
	p.add("show_duplicate_tags", &MPD::Song::ShowDuplicateTags, "yes", yes_no);
	p.add("incremental_seeking", &incremental_seeking, "yes", yes_no);
	p.add("seek_time", &seek_time, "1");
	p.add("volume_change_step", &volume_change_step, "2");
	p.add("autocenter_mode", &autocenter_mode, "no", yes_no);
	p.add("centered_cursor", &centered_cursor, "no", yes_no);
	p.add("progressbar_look", &progressbar, "=>", [](std::string v) {
			auto result = ToWString(std::move(v));
			boundsCheck<std::wstring::size_type>(result.size(), 2, 5);
			// If less than 5 characters were specified, fill \0 as the remaining.
			result.resize(5);
			return result;
	});
	p.add("default_place_to_search_in", &search_in_db, "database", [](std::string v) {
			if (v == "database")
				return true;
			else if (v == "playlist")
				return false;
			else
				invalid_value(v);
	});
	p.add("user_interface", &design, "classic");
	p.add("data_fetching_delay", &data_fetching_delay, "yes", yes_no);
	p.add("media_library_primary_tag", &media_lib_primary_tag, "artist", [](std::string v) {
			if (v == "artist")
				return MPD_TAG_ARTIST;
			else if (v == "album_artist")
				return MPD_TAG_ALBUM_ARTIST;
			else if (v == "date")
				return MPD_TAG_DATE;
			else if (v == "genre")
				return MPD_TAG_GENRE;
			else if (v == "composer")
				return MPD_TAG_COMPOSER;
			else if (v == "performer")
				return MPD_TAG_PERFORMER;
			else
				invalid_value(v);
		});
	p.add("media_library_albums_split_by_date", &media_library_albums_split_by_date, "yes", yes_no);
	p.add("default_find_mode", &wrapped_search, "wrapped", [](std::string v) {
			if (v == "wrapped")
				return true;
			else if (v == "normal")
				return false;
			else
				invalid_value(v);
		});
	p.add("default_tag_editor_pattern", &pattern, "%n - %t");
	p.add("header_visibility", &header_visibility, "yes", yes_no);
	p.add("statusbar_visibility", &statusbar_visibility, "yes", yes_no);
	p.add("connected_message_on_startup", &connected_message_on_startup, "yes", yes_no);
	p.add("titles_visibility", &titles_visibility, "yes", yes_no);
	p.add("header_text_scrolling", &header_text_scrolling, "yes", yes_no);
	p.add("cyclic_scrolling", &use_cyclic_scrolling, "no", yes_no);
	p.add("lines_scrolled", &lines_scrolled, "2");
	p.add("lyrics_fetchers", &lyrics_fetchers,
	      "lyricwiki, azlyrics, genius, sing365, lyricsmania, metrolyrics, justsomelyrics, jahlyrics, plyrics, tekstowo, zeneszoveg, internet",
	      list_of<LyricsFetcher_>);
	p.add("follow_now_playing_lyrics", &now_playing_lyrics, "no", yes_no);
	p.add("fetch_lyrics_for_current_song_in_background", &fetch_lyrics_in_background,
	      "no", yes_no);
	p.add("store_lyrics_in_song_dir", &store_lyrics_in_song_dir, "no", yes_no);
	p.add("generate_win32_compatible_filenames", &generate_win32_compatible_filenames,
	      "yes", yes_no);
	p.add("allow_for_physical_item_deletion", &allow_for_physical_item_deletion,
	      "no", yes_no);
	p.add("lastfm_preferred_language", &lastfm_preferred_language, "en");
	p.add("space_add_mode", &space_add_mode, "add_remove");
	p.add("show_hidden_files_in_local_browser", &local_browser_show_hidden_files,
	      "no", yes_no);
	p.add<void>(
		"screen_switcher_mode", nullptr, "playlist, browser",
		[this](std::string v) {
			if (v == "previous")
				screen_switcher_previous = true;
			else
			{
				screen_switcher_previous = false;
				screen_sequence = list_of<ScreenType>(v, [](std::string s) {
						auto screen = stringtoStartupScreenType(s);
						if (screen == ScreenType::Unknown)
							invalid_value(s);
						return screen;
					});
			}
		});
	p.add("startup_screen", &startup_screen_type, "playlist", [](std::string v) {
			auto screen = stringtoStartupScreenType(v);
			if (screen == ScreenType::Unknown)
				invalid_value(v);
			return screen;
		});
	p.add("startup_slave_screen", &startup_slave_screen_type, "", [](std::string v) {
			boost::optional<ScreenType> screen;
			if (!v.empty())
			{
				screen = stringtoStartupScreenType(v);
				if (screen == ScreenType::Unknown)
					invalid_value(v);
			}
			return screen;
		});
	p.add("startup_slave_screen_focus", &startup_slave_screen_focus, "no", yes_no);
	p.add("locked_screen_width_part", &locked_screen_width_part,
	      "50", [](std::string v) {
		      return verbose_lexical_cast<double>(v) / 100;
	      });
	p.add("ask_for_locked_screen_width_part", &ask_for_locked_screen_width_part,
	      "yes", yes_no);
	p.add("jump_to_now_playing_song_at_start", &jump_to_now_playing_song_at_start,
	      "yes", yes_no);
	p.add("ask_before_clearing_playlists", &ask_before_clearing_playlists,
	      "yes", yes_no);
	p.add("ask_before_shuffling_playlists", &ask_before_shuffling_playlists,
	      "yes", yes_no);
	p.add("clock_display_seconds", &clock_display_seconds, "no", yes_no);
	p.add("display_volume_level", &display_volume_level, "yes", yes_no);
	p.add("display_bitrate", &display_bitrate, "no", yes_no);
	p.add("display_remaining_time", &display_remaining_time, "no", yes_no);
	p.add("regular_expressions", &regex_type, "perl", [](std::string v) {
			if (v == "none")
				return boost::regex::icase | boost::regex::literal;
			else if (v == "basic")
				return boost::regex::icase | boost::regex::basic;
			else if (v == "extended")
				return boost::regex::icase |  boost::regex::extended;
			else if (v == "perl")
				return boost::regex::icase | boost::regex::perl;
			else
				invalid_value(v);
	});
	p.add("ignore_leading_the", &ignore_leading_the, "no", yes_no);
	p.add("ignore_diacritics", &ignore_diacritics, "no", yes_no);
	p.add("block_search_constraints_change_if_items_found",
	      &block_search_constraints_change, "yes", yes_no);
	p.add("mouse_support", &mouse_support, "yes", yes_no);
	p.add("mouse_list_scroll_whole_page", &mouse_list_scroll_whole_page, "yes", yes_no);
	p.add("empty_tag_marker", &empty_tag, "<empty>");
	p.add("tags_separator", &MPD::Song::TagsSeparator, " | ");
	p.add("tag_editor_extended_numeration", &tag_editor_extended_numeration, "no", yes_no);
	p.add("media_library_sort_by_mtime", &media_library_sort_by_mtime, "no", yes_no);
	p.add("enable_window_title", &set_window_title, "yes", [](std::string v) {
			// Consider this variable only if TERM variable is available and we're not
			// in emacs terminal nor tty (through any wrapper like screen).
			auto term = getenv("TERM");
			if (term != nullptr
			    && strstr(term, "linux") == nullptr
			    && strncmp(term, "eterm", const_strlen("eterm")))
				return yes_no(v);
			else
			{
				std::clog << "Terminal doesn't support window title, skipping 'enable_window_title'.\n";
				return false;
			}
		});
	p.add("search_engine_default_search_mode", &search_engine_default_search_mode,
	      "1", [](std::string v) {
		      auto mode = verbose_lexical_cast<unsigned>(v);
		      boundsCheck<unsigned>(mode, 1, 3);
		      return --mode;
	      });
	p.add("external_editor", &external_editor, "nano", adjust_path);
	p.add("use_console_editor", &use_console_editor, "yes", yes_no);
	p.add("colors_enabled", &colors_enabled, "yes", yes_no);
	p.add("empty_tag_color", &empty_tags_color, "cyan");
	p.add("header_window_color", &header_color, "default");
	p.add("volume_color", &volume_color, "default");
	p.add("state_line_color", &state_line_color, "default");
	p.add("state_flags_color", &state_flags_color, "default:b");
	p.add("main_window_color", &main_color, "yellow");
	p.add("color1", &color1, "white");
	p.add("color2", &color2, "green");
	p.add("progressbar_color", &progressbar_color, "black:b");
	p.add("progressbar_elapsed_color", &progressbar_elapsed_color, "green:b");
	p.add("statusbar_color", &statusbar_color, "default");
	p.add("statusbar_time_color", &statusbar_time_color, "default:b");
	p.add("player_state_color", &player_state_color, "default:b");
	p.add("alternative_ui_separator_color", &alternative_ui_separator_color, "black:b");
	p.add("window_border_color", &window_border, "green", verbose_lexical_cast<NC::Color>);
	p.add("active_window_border", &active_window_border, "red",
	      verbose_lexical_cast<NC::Color>);

	return std::all_of(
		config_paths.begin(),
		config_paths.end(),
		[&](const std::string &config_path) {
			std::ifstream f(config_path);
			if (f.is_open())
				std::clog << "Reading configuration from " << config_path << "...\n";
			return p.run(f, ignore_errors);
		}
	) && p.initialize_undefined(ignore_errors);
}
