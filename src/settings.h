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

#ifndef HAVE_SETTINGS_H
#define HAVE_SETTINGS_H

#include <fstream>

#include "ncmpcpp.h"

const string home_folder = getenv("HOME");

struct ncmpcpp_config
{
	string mpd_music_dir;
	string song_list_format;
	string song_status_format;
	string song_window_title_format;
	
	COLOR empty_tags_color;
	COLOR header_color;
	COLOR volume_color;
	COLOR state_line_color;
	COLOR state_flags_color;
	COLOR main_color;
	COLOR progressbar_color;
	COLOR statusbar_color;
	
	bool set_window_title;
	bool header_visibility;
	bool statusbar_visibility;
	
	int mpd_connection_timeout;
	int crossfade_time;
	int playlist_disable_highlight_delay;
	int message_delay_time;
};

void DefaultConfiguration(ncmpcpp_config &);
string GetLineValue(const string &);
string IntoStr(COLOR);
COLOR IntoColor(const string &);
void ReadConfiguration(ncmpcpp_config &);

#endif

