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

#include "settings.h"

const string config_file = home_folder + "/.ncmpcpprc";

using std::ifstream;

void DefaultConfiguration(ncmpcpp_config &conf)
{
	conf.mpd_music_dir = "/var/lib/mpd/music";
	conf.song_list_format = "[green](%l)[/green] {%a - }{%t}|{[white]%f[/white]}";
	conf.song_status_format = "(%l) {%a - }{%t}|{%f}";
	conf.song_window_title_format = "{%a - }{%t}|{%f}";
	conf.song_library_format = "{%n - }{%t}|{%f}";
	conf.browser_playlist_prefix = "[red](playlist)[/red] ";
	conf.empty_tags_color = clCyan;
	conf.header_color = clDefault;
	conf.volume_color = clDefault;
	conf.state_line_color = clDefault;
	conf.state_flags_color = clDefault;
	conf.main_color = clYellow;
	conf.main_highlight_color = conf.main_color;
	conf.progressbar_color = clDefault;
	conf.statusbar_color = clDefault;
	conf.library_active_column_color = clRed;
	conf.colors_enabled = true;
	conf.header_visibility = true;
	conf.statusbar_visibility = true;
	conf.set_window_title = true;
	conf.mpd_connection_timeout = 15;
	conf.crossfade_time = 5;
	conf.playlist_disable_highlight_delay = 5;
	conf.message_delay_time = 4;
}

string GetConfigLineValue(const string &line)
{
	int i = 0;
	int begin = -1, end = -1;
	for (string::const_iterator it = line.begin(); it != line.end(); i++, it++)
	{
		if (*it == '"')
		{
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

string IntoStr(COLOR color)
{
	string result = "";
	
	if (color == clBlack)
		result = "black";
	if (color == clRed)
		result = "red";
	if (color == clGreen)
		result = "green";
	if (color == clYellow)
		result = "yellow";
	if (color == clBlue)
		result = "blue";
	if (color == clMagenta)
		result = "magenta";
	if (color == clCyan)
		result = "cyan";
	if (color == clWhite)
		result = "white";
	
	return result;
}

COLOR IntoColor(const string &color)
{
	COLOR result = clDefault;
	
	if (color == "black")
		result = clBlack;
	if (color == "red")
		result = clRed;
	if (color == "green")
		result = clGreen;
	if (color == "yellow")
		result = clYellow;
	if (color == "blue")
		result = clBlue;
	if (color == "magenta")
		result = clMagenta;
	if (color == "cyan")
		result = clCyan;
	if (color == "white")
		result = clWhite;
	
	return result;
}

void ReadConfiguration(ncmpcpp_config &conf)
{
	ifstream f(config_file.c_str());
	
	string config_line;
	string v;
	vector<string> config_sets;
	
	if (f.is_open())
	{
		while (!f.eof())
		{
			getline(f, config_line);
			if (!config_line.empty() && config_line[0] != '#')
				config_sets.push_back(config_line);
		}
		for (vector<string>::const_iterator it = config_sets.begin(); it != config_sets.end(); it++)
		{
			v = GetConfigLineValue(*it);
			
			if (it->find("mpd_music_dir") != string::npos)
				if (!v.empty())
					conf.mpd_music_dir = v;
			
			if (it->find("mpd_connection_timeout") != string::npos)
				if (StrToInt(v))
					conf.mpd_connection_timeout = StrToInt(v);
			
			if (it->find("mpd_crossfade_time") != string::npos)
				if (StrToInt(v) > 0)
					conf.crossfade_time = StrToInt(v);
			
			if (it->find("playlist_disable_highlight_delay") != string::npos)
				if (StrToInt(v) >= 0)
					conf.playlist_disable_highlight_delay = StrToInt(v);
			
			if (it->find("message_delay_time") != string::npos)
				if (StrToInt(v) > 0)
					conf.message_delay_time = StrToInt(v);
			
			if (it->find("song_list_format") != string::npos)
				 if (!v.empty())
					conf.song_list_format = v;
			
			if (it->find("song_status_format") != string::npos)
				if (!v.empty())
					conf.song_status_format = v;
			
			if (it->find("song_library_format") != string::npos)
				if (!v.empty())
					conf.song_library_format = v;
			
			if (it->find("browser_playlist_prefix") != string::npos)
				if (!v.empty())
					conf.browser_playlist_prefix = v;
			
			if (it->find("colors_enabled") != string::npos)
				conf.colors_enabled = v == "yes";
			
			if (it->find("header_visibility") != string::npos)
				conf.header_visibility = v == "yes";
			
			if (it->find("statusbar_visibility") != string::npos)
				conf.statusbar_visibility = v == "yes";
			
			if (it->find("enable_window_title") != string::npos)
				conf.set_window_title = v == "yes";
			
			if (it->find("song_window_title_format") != string::npos)
				if (!v.empty())
					conf.song_window_title_format = v;
			
			if (it->find("empty_tag_color") != string::npos)
				if (!v.empty())
					conf.empty_tags_color = IntoColor(v);
			
			if (it->find("header_window_color") != string::npos)
				if (!v.empty())
					conf.header_color = IntoColor(v);
			
			if (it->find("volume_color") != string::npos)
				if (!v.empty())
					conf.volume_color = IntoColor(v);
			
			if (it->find("state_line_color") != string::npos)
				if (!v.empty())
					conf.state_line_color = IntoColor(v);
			
			if (it->find("state_flags_color") != string::npos)
				if (!v.empty())
					conf.state_flags_color = IntoColor(v);
			
			if (it->find("main_window_color") != string::npos)
				if (!v.empty())
					conf.main_color = IntoColor(v);
			
			if (it->find("main_window_highlight_color") != string::npos)
				if (!v.empty())
					conf.main_highlight_color = IntoColor(v);
			
			if (it->find("progressbar_color") != string::npos)
				if (!v.empty())
					conf.progressbar_color = IntoColor(v);
			
			if (it->find("statusbar_color") != string::npos)
				if (!v.empty())
					conf.statusbar_color = IntoColor(v);
			
			if (it->find("library_active_column_color") != string::npos)
				if (!v.empty())
					conf.library_active_column_color = IntoColor(v);
		}
		f.close();
	}
}

