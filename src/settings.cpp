/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
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
#include <fstream>
#include <stdexcept>

#include "configuration.h"
#include "helpers.h"
#include "settings.h"
#include "utility/conversion.h"
#include "utility/option_parser.h"
#include "utility/type_conversions.h"

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

Configuration Config;

namespace {

std::string remove_dollar_formatting(const std::string &s)
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

std::pair<std::vector<Column>, std::string> generate_columns(const std::string &format)
{
	std::vector<Column> result;
	std::string width;
	size_t pos = 0;
	while (!(width = getEnclosedString(format, '(', ')', &pos)).empty())
	{
		Column col;
		col.color = stringToColor(getEnclosedString(format, '[', ']', &pos));
		std::string tag_type = getEnclosedString(format, '{', '}', &pos);

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

	std::string string_format;
	// generate format for converting tags in columns to string for Playlist::SongInColumnsToString()
	char tag[] = "{% }|";
	string_format = "{";
	for (auto it = result.begin(); it != result.end(); ++it)
	{
		for (std::string::const_iterator j = it->type.begin(); j != it->type.end(); ++j)
		{
			tag[2] = *j;
			string_format += tag;
		}
		*string_format.rbegin() = ' ';
	}
	if (string_format.length() == 1) // only '{'
		string_format += '}';
	else
		*string_format.rbegin() = '}';

	return std::make_pair(std::move(result), std::move(string_format));
}

void add_slash_at_the_end(std::string &s)
{
	if (s.empty() || *s.rbegin() != '/')
	{
		s.resize(s.size()+1);
		*s.rbegin() = '/';
	}
}

std::string adjust_directory(std::string &&s)
{
	add_slash_at_the_end(s);
	expand_home(s);
	return s;
}

std::string adjust_and_validate_format(std::string &&format)
{
	MPD::Song::validateFormat(format);
	format = "{" + format + "}";
	return format;
}

// parser worker for buffer
template <typename ValueT, typename TransformT>
option_parser::worker buffer(NC::Buffer &arg, ValueT &&value, TransformT &&map)
{
	return option_parser::worker(assign<std::string>(arg, [&arg, map](std::string &&s) {
		return map(stringToBuffer(s));
	}), defaults_to(arg, map(std::forward<ValueT>(value))));
}

}

bool Configuration::read(const std::string &config_path)
{
	std::string mpd_host;
	unsigned mpd_port;
	std::string columns_format;

	// keep the same order of variables as in configuration file
	option_parser p;
	p.add("ncmpcpp_directory", assign_default<std::string>(
		ncmpcpp_directory, "~/.ncmpcpp/", adjust_directory
	));
	p.add("lyrics_directory", assign_default<std::string>(
		lyrics_directory, "~/.lyrics/", adjust_directory
	));
	p.add("mpd_host", assign_default<std::string>(
		mpd_host, "localhost", [](std::string &&host) {
			Mpd.SetHostname(host);
			return host;
	}));
	p.add("mpd_port", assign_default<unsigned>(
		mpd_port, 6600, [](unsigned port) {
			Mpd.SetPort(port);
			return port;
	}));
	p.add("mpd_music_dir", assign_default<std::string>(
		mpd_music_dir, "~/music", adjust_directory
	));
	p.add("mpd_connection_timeout", assign_default(
		mpd_connection_timeout, 5
	));
	p.add("mpd_crossfade_time", assign_default(
		crossfade_time, 5
	));
	p.add("visualizer_fifo_path", assign_default(
		visualizer_fifo_path, "/tmp/mpd.fifo"
	));
	p.add("visualizer_output_name", assign_default(
		visualizer_output_name, "Visualizer feed"
	));
	p.add("visualizer_in_stereo", yes_no(
		visualizer_in_stereo, true
	));
	p.add("visualizer_sample_multiplier", assign_default<double>(
		visualizer_sample_multiplier, 1.0, [](double v) {
			lowerBoundCheck(v, 0.0);
			return v;
	}));
	p.add("visualizer_sync_interval", assign_default<unsigned>(
		visualizer_sync_interval, 30, [](unsigned v) {
			return boost::posix_time::seconds(v);
	}));
	p.add("visualizer_type", option_parser::worker([this](std::string &&v) {
		visualizer_type = stringToVisualizerType( v );
	}, defaults_to(visualizer_type, VisualizerType::Wave)
	));
	p.add("visualizer_look", assign_default<std::string>(
		visualizer_chars, "●▮", [](std::string &&s) {
			auto result = ToWString(std::move(s));
			typedef std::wstring::size_type size_type;
			boundsCheck(result.size(), size_type(2), size_type(2));
			return result;
	}));
	p.add("system_encoding", assign_default<std::string>(
		system_encoding, "", [](std::string &&enc) {
#			ifdef HAVE_LANGINFO_H
			// try to autodetect system encoding
			if (enc.empty())
			{
				enc = nl_langinfo(CODESET);
				if (enc == "UTF-8") // mpd uses utf-8 by default so no need to convert
					enc.clear();
			}
#			endif // HAVE_LANGINFO_H
			return enc;
	}));
	p.add("playlist_disable_highlight_delay", assign_default<unsigned>(
		playlist_disable_highlight_delay, 5, [](unsigned v) {
			return boost::posix_time::seconds(v);
	}));
	p.add("message_delay_time", assign_default(
		message_delay_time, 5
	));
	p.add("song_list_format", assign_default<std::string>(
		song_list_format, "{%a - }{%t}|{$8%f$9}$R{$3(%l)$9}", [this](std::string &&s) {
			auto result = adjust_and_validate_format(std::move(s));
			song_list_format_dollar_free = remove_dollar_formatting(result);
			return result;
	}));
	p.add("song_status_format", assign_default<std::string>(
		song_status_format, "{{%a{ \"%b\"{ (%y)}} - }{%t}}|{%f}", [this](std::string &&s) {
			auto result = adjust_and_validate_format(std::move(s));
			if (result.find("$") != std::string::npos)
				song_status_format_no_colors = stringToBuffer(result).str();
			else
				song_status_format_no_colors = result;
			return result;
	}));
	p.add("song_library_format", assign_default<std::string>(
		song_library_format, "{%n - }{%t}|{%f}", adjust_and_validate_format
	));
	p.add("tag_editor_album_format", assign_default<std::string>(
		tag_editor_album_format, "{(%y) }%b", adjust_and_validate_format
	));
	p.add("browser_sort_mode", assign_default(
		browser_sort_mode, SortMode::Name
	));
	p.add("browser_sort_format", assign_default<std::string>(
		browser_sort_format, "{%a - }{%t}|{%f} {(%l)}", adjust_and_validate_format
	));
	p.add("alternative_header_first_line_format", assign_default<std::string>(
		new_header_first_line, "$b$1$aqqu$/a$9 {%t}|{%f} $1$atqq$/a$9$/b", adjust_and_validate_format
	));
	p.add("alternative_header_second_line_format", assign_default<std::string>(
		new_header_second_line, "{{$4$b%a$/b$9}{ - $7%b$9}{ ($4%y$9)}}|{%D}", adjust_and_validate_format
	));
	p.add("now_playing_prefix", buffer(
		now_playing_prefix, NC::Buffer(NC::Format::Bold), [this](NC::Buffer &&buf) {
			now_playing_prefix_length = wideLength(ToWString(buf.str()));
			return buf;
	}));
	p.add("now_playing_suffix", buffer(
		now_playing_suffix, NC::Buffer(NC::Format::NoBold), [this](NC::Buffer &&buf) {
			now_playing_suffix_length = wideLength(ToWString(buf.str()));
			return buf;
	}));
	p.add("browser_playlist_prefix", buffer(
		browser_playlist_prefix, NC::Buffer(NC::Color::Red, "playlist", NC::Color::End, ' '), id_()
	));
	p.add("selected_item_prefix", buffer(
		selected_item_prefix, NC::Buffer(NC::Color::Magenta), [this](NC::Buffer &&buf) {
			selected_item_prefix_length = wideLength(ToWString(buf.str()));
			return buf;
	}));
	p.add("selected_item_suffix", buffer(
		selected_item_suffix, NC::Buffer(NC::Color::End), [this](NC::Buffer &&buf) {
			selected_item_suffix_length = wideLength(ToWString(buf.str()));
			return buf;
	}));
	p.add("modified_item_prefix", buffer(
		modified_item_prefix, NC::Buffer(NC::Color::Green, "> ", NC::Color::End), id_()
	));
	p.add("song_window_title_format", assign_default<std::string>(
		song_window_title_format, "{%a - }{%t}|{%f}", adjust_and_validate_format
	));
	p.add("song_columns_list_format", assign_default<std::string>(
		columns_format, "(20)[]{a} (6f)[green]{NE} (50)[white]{t|f:Title} (20)[cyan]{b} (7f)[magenta]{l}",
			[this](std::string &&v) {
				boost::tie(columns, song_in_columns_to_string_format) = generate_columns(v);
				return v;
	}));
	p.add("execute_on_song_change", assign_default(
		execute_on_song_change, ""
	));
	p.add("playlist_show_remaining_time", yes_no(
		playlist_show_remaining_time, false
	));
	p.add("playlist_shorten_total_times", yes_no(
		playlist_shorten_total_times, false
	));
	p.add("playlist_separate_albums", yes_no(
		playlist_separate_albums, false
	));
	p.add("playlist_display_mode", assign_default(
		playlist_display_mode, DisplayMode::Columns
	));
	p.add("browser_display_mode", assign_default(
		browser_display_mode, DisplayMode::Classic
	));
	p.add("search_engine_display_mode", assign_default(
		search_engine_display_mode, DisplayMode::Classic
	));
	p.add("playlist_editor_display_mode", assign_default(
		playlist_editor_display_mode, DisplayMode::Classic
	));
	p.add("discard_colors_if_item_is_selected", yes_no(
		discard_colors_if_item_is_selected, true
	));
	p.add("incremental_seeking", yes_no(
		incremental_seeking, true
	));
	p.add("seek_time", assign_default(
		seek_time, 1
	));
	p.add("volume_change_step", assign_default(
		volume_change_step, 2
	));
	p.add("autocenter_mode", yes_no(
		autocenter_mode, false
	));
	p.add("centered_cursor", yes_no(
		centered_cursor, false
	));
	p.add("progressbar_look", assign_default<std::string>(
		progressbar, "=>", [](std::string &&s) {
			auto result = ToWString(std::move(s));
			typedef std::wstring::size_type size_type;
			boundsCheck(result.size(), size_type(2), size_type(3));
			// if two characters were specified, add third one (null)
			result.resize(3);
			return result;
	}));
	p.add("progressbar_boldness", yes_no(
		progressbar_boldness, true
	));
	p.add("default_place_to_search_in", option_parser::worker([this](std::string &&v) {
		if (v == "database")
			search_in_db = true;
		else if (v == "playlist")
			search_in_db = true;
		else
			throw std::runtime_error("invalid argument: " + v);
	}, defaults_to(search_in_db, true)
	));
	p.add("user_interface", assign_default(
		design, Design::Classic
	));
	p.add("data_fetching_delay", yes_no(
		data_fetching_delay, true
	));
	p.add("media_library_primary_tag", option_parser::worker([this](std::string &&v) {
		if (v == "artist")
			media_lib_primary_tag = MPD_TAG_ARTIST;
		else if (v == "album_artist")
			media_lib_primary_tag = MPD_TAG_ALBUM_ARTIST;
		else if (v == "date")
			media_lib_primary_tag = MPD_TAG_DATE;
		else if (v == "genre")
			media_lib_primary_tag = MPD_TAG_GENRE;
		else if (v == "composer")
			media_lib_primary_tag = MPD_TAG_COMPOSER;
		else if (v == "performer")
			media_lib_primary_tag = MPD_TAG_PERFORMER;
		else
			throw std::runtime_error("invalid argument: " + v);
		regex_type |= boost::regex::icase;
	}, defaults_to(regex_type, boost::regex::literal | boost::regex::icase)
	));
	p.add("default_find_mode", option_parser::worker([this](std::string &&v) {
		if (v == "wrapped")
			wrapped_search = true;
		else if (v == "normal")
			wrapped_search = false;
		else
			throw std::runtime_error("invalid argument: " + v);
	}, defaults_to(wrapped_search, true)
	));
	p.add("default_space_mode", option_parser::worker([this](std::string &&v) {
		if (v == "add")
			space_selects = false;
		else if (v == "select")
			space_selects = true;
		else
			throw std::runtime_error("invalid argument: " + v);
	}, defaults_to(wrapped_search, true)
	));
	p.add("default_tag_editor_pattern", assign_default(
		pattern, "%n - %t"
	));
	p.add("header_visibility", yes_no(
		header_visibility, true
	));
	p.add("statusbar_visibility", yes_no(
		statusbar_visibility, true
	));
	p.add("titles_visibility", yes_no(
		titles_visibility, true
	));
	p.add("header_text_scrolling", yes_no(
		header_text_scrolling, true
	));
	p.add("cyclic_scrolling", yes_no(
		use_cyclic_scrolling, false
	));
	p.add("lines_scrolled", assign_default(
		lines_scrolled, 2
	));
	p.add("follow_now_playing_lyrics", yes_no(
		now_playing_lyrics, false
	));
	p.add("fetch_lyrics_for_current_song_in_background", yes_no(
		fetch_lyrics_in_background, false
	));
	p.add("store_lyrics_in_song_dir", yes_no(
		store_lyrics_in_song_dir, false
	));
	p.add("generate_win32_compatible_filenames", yes_no(
		generate_win32_compatible_filenames, true
	));
	p.add("allow_for_physical_item_deletion", yes_no(
		allow_for_physical_item_deletion, false
	));
	p.add("lastfm_preferred_language", assign_default(
		lastfm_preferred_language, "en"
	));
	p.add("space_add_mode", assign_default(
		space_add_mode, SpaceAddMode::AlwaysAdd
	));
	p.add("show_hidden_files_in_local_browser", yes_no(
		local_browser_show_hidden_files, false
	));
	p.add("screen_switcher_mode", option_parser::worker([this](std::string &&v) {
		if (v == "previous")
			screen_switcher_previous = true;
		else
		{
			screen_switcher_previous = false;
			boost::sregex_token_iterator i(v.begin(), v.end(), boost::regex("\\w+")), j;
			for (; i != j; ++i)
			{
				auto screen = stringtoStartupScreenType(*i);
				if (screen != ScreenType::Unknown)
					screen_sequence.push_back(screen);
				else
					throw std::runtime_error("unknown screen: " + *i);
			}
		}
	}, [this] {
		screen_switcher_previous = false;
		screen_sequence = { ScreenType::Playlist, ScreenType::Browser };
	}));
	p.add("startup_screen", option_parser::worker([this](std::string &&v) {
		startup_screen_type = stringtoStartupScreenType(v);
		if (startup_screen_type == ScreenType::Unknown)
			throw std::runtime_error("unknown screen: " + v);
	}, defaults_to(startup_screen_type, ScreenType::Playlist)
	));
	p.add("locked_screen_width_part", assign_default<double>(
		locked_screen_width_part, 50.0, [](double v) {
			return v / 100;
	}));
	p.add("ask_for_locked_screen_width_part", yes_no(
		ask_for_locked_screen_width_part, true
	));
	p.add("jump_to_now_playing_song_at_start", yes_no(
		jump_to_now_playing_song_at_start, true
	));
	p.add("ask_before_clearing_playlists", yes_no(
		ask_before_clearing_playlists, true
	));
	p.add("clock_display_seconds", yes_no(
		clock_display_seconds, false
	));
	p.add("display_volume_level", yes_no(
		display_volume_level, true
	));
	p.add("display_bitrate", yes_no(
		display_bitrate, false
	));
	p.add("display_remaining_time", yes_no(
		display_remaining_time, false
	));
	p.add("regular_expressions", option_parser::worker([this](std::string &&v) {
		if (v == "none")
			regex_type = boost::regex::literal;
		else if (v == "basic")
			regex_type = boost::regex::basic;
		else if (v == "extended")
			regex_type = boost::regex::extended;
		else
			throw std::runtime_error("invalid argument: " + v);
		regex_type |= boost::regex::icase;
	}, defaults_to(regex_type, boost::regex::literal | boost::regex::icase)
	));
	p.add("ignore_leading_the", yes_no(
		ignore_leading_the, false
	));
	p.add("block_search_constraints_change_if_items_found", yes_no(
		block_search_constraints_change, true
	));
	p.add("mouse_support", yes_no(
		mouse_support, true
	));
	p.add("mouse_list_scroll_whole_page", yes_no(
		mouse_list_scroll_whole_page, true
	));
	p.add("empty_tag_marker", assign_default(
		empty_tag, "<empty>"
	));
	p.add("tags_separator", assign_default(
		tags_separator, " | "
	));
	p.add("tag_editor_extended_numeration", yes_no(
		tag_editor_extended_numeration, false
	));
	p.add("media_library_sort_by_mtime", yes_no(
		media_library_sort_by_mtime, false
	));
	p.add("enable_window_title", yes_no(
		set_window_title, true
	));
	p.add("search_engine_default_search_mode", assign_default<unsigned>(
		search_engine_default_search_mode, 1, [](unsigned v) {
			boundsCheck(v, 1u, 3u);
			return --v;
	}));
	p.add("external_editor", assign_default(
		external_editor, "nano"
	));
	p.add("use_console_editor", yes_no(
		use_console_editor, true
	));
	p.add("colors_enabled", yes_no(
		colors_enabled, true
	));
	p.add("empty_tag_color", assign_default(
		empty_tags_color, NC::Color::Cyan
	));
	p.add("header_window_color", assign_default(
		header_color, NC::Color::Default
	));
	p.add("volume_color", assign_default(
		volume_color, NC::Color::Default
	));
	p.add("state_line_color", assign_default(
		state_line_color, NC::Color::Default
	));
	p.add("state_flags_color", assign_default(
		state_flags_color, NC::Color::Default
	));
	p.add("main_window_color", assign_default(
		main_color, NC::Color::Yellow
	));
	p.add("color1", assign_default(
		color1, NC::Color::White
	));
	p.add("color2", assign_default(
		color2, NC::Color::Green
	));
	p.add("main_window_highlight_color", assign_default(
		main_highlight_color, NC::Color::Yellow
	));
	p.add("progressbar_color", assign_default(
		progressbar_color, NC::Color::Black
	));
	p.add("progressbar_elapsed_color", assign_default(
		progressbar_elapsed_color, NC::Color::Green
	));
	p.add("statusbar_color", assign_default(
		statusbar_color, NC::Color::Default
	));
	p.add("alternative_ui_separator_color", assign_default(
		alternative_ui_separator_color, NC::Color::Black
	));
	p.add("active_column_color", assign_default(
		active_column_color, NC::Color::Red
	));
	p.add("visualizer_color", option_parser::worker([this](std::string &&v) {
		boost::sregex_token_iterator i(v.begin(), v.end(), boost::regex("\\w+")), j;
		for (; i != j; ++i)
		{
			auto color = stringToColor(*i);
			visualizer_colors.push_back(color);
		}
	}, [this] {
			visualizer_colors = { NC::Color::Blue, NC::Color::Cyan, NC::Color::Green, NC::Color::Yellow, NC::Color::Red };
	}));
	p.add("window_border_color", assign_default(
		window_border, NC::Border::Green
	));
	p.add("active_window_border", assign_default(
		active_window_border, NC::Border::Red
	));

	std::ifstream f(config_path);
	return p.run(f);
}

/* vim: set tabstop=4 softtabstop=4 shiftwidth=4 noexpandtab : */
