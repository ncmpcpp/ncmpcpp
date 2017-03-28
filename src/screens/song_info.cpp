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

#include "global.h"
#include "helpers.h"
#include "screens/song_info.h"
#include "screens/tag_editor.h"
#include "tags.h"
#include "title.h"
#include "screens/screen_switcher.h"

#ifdef HAVE_TAGLIB_H
# include "fileref.h"
# include "tag.h"
# include <boost/lexical_cast.hpp>
#endif // HAVE_TAGLIB_H

using Global::MainHeight;
using Global::MainStartY;

SongInfo *mySongInfo;

const SongInfo::Metadata SongInfo::Tags[] =
{
 { "Title",        &MPD::Song::getTitle,       &MPD::MutableSong::setTitle       },
 { "Artist",       &MPD::Song::getArtist,      &MPD::MutableSong::setArtist      },
 { "Album Artist", &MPD::Song::getAlbumArtist, &MPD::MutableSong::setAlbumArtist },
 { "Album",        &MPD::Song::getAlbum,       &MPD::MutableSong::setAlbum       },
 { "Date",         &MPD::Song::getDate,        &MPD::MutableSong::setDate        },
 { "Track",        &MPD::Song::getTrack,       &MPD::MutableSong::setTrack       },
 { "Genre",        &MPD::Song::getGenre,       &MPD::MutableSong::setGenre       },
 { "Composer",     &MPD::Song::getComposer,    &MPD::MutableSong::setComposer    },
 { "Performer",    &MPD::Song::getPerformer,   &MPD::MutableSong::setPerformer   },
 { "Disc",         &MPD::Song::getDisc,        &MPD::MutableSong::setDisc        },
 { "Comment",      &MPD::Song::getComment,     &MPD::MutableSong::setComment     },
 { 0,              0,                          0                                 }
};

SongInfo::SongInfo()
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border()))
{ }

void SongInfo::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::wstring SongInfo::title()
{
	return L"Song info";
}

void SongInfo::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
	{
		auto s = currentSong(myScreen);
		if (!s)
			return;
		SwitchTo::execute(this);
		w.clear();
		w.reset();
		PrepareSong(*s);
		w.flush();
		// redraw header after we're done with the file, since reading it from disk
		// takes a bit of time and having header updated before content of a window
		// is displayed doesn't look nice.
		drawHeader();
	}
	else
		switchToPreviousScreen();
}

void SongInfo::PrepareSong(const MPD::Song &s)
{
	auto print_key_value = [this](const char *key, const auto &value) {
		w << NC::Format::Bold
		  << Config.color1
		  << key
		  << ":"
		  << NC::FormattedColor::End<>(Config.color1)
		  << NC::Format::NoBold
		  << " "
		  << Config.color2
		  << value
		  << NC::FormattedColor::End<>(Config.color2)
		  << "\n";
	};

	print_key_value("Filename", s.getName());
	print_key_value("Directory", ShowTag(s.getDirectory()));
	w << "\n";
	print_key_value("Length", s.getLength());

#	ifdef HAVE_TAGLIB_H
	if (!Config.mpd_music_dir.empty() && !s.isStream())
	{
		std::string path_to_file;
		if (s.isFromDatabase())
			path_to_file += Config.mpd_music_dir;
		path_to_file += s.getURI();
		TagLib::FileRef f(path_to_file.c_str());
		if (!f.isNull())
		{
			print_key_value(
				"Bitrate",
				boost::lexical_cast<std::string>(f.audioProperties()->bitrate()) + " kbps");
			print_key_value(
				"Sample rate",
				boost::lexical_cast<std::string>(f.audioProperties()->sampleRate()) + " Hz");
			print_key_value("Channels", channelsToString(f.audioProperties()->channels()));
			
			auto rginfo = Tags::readReplayGain(f.file());
			if (!rginfo.empty())
			{
				w << "\n";
				print_key_value("Reference loudness", rginfo.referenceLoudness());
				print_key_value("Track gain", rginfo.trackGain());
				print_key_value("Track peak", rginfo.trackPeak());
				print_key_value("Album gain", rginfo.albumGain());
				print_key_value("Album peak", rginfo.albumPeak());
			}
		}
	}
#	endif // HAVE_TAGLIB_H
	w << NC::Color::Default;
	
	for (const Metadata *m = Tags; m->Name; ++m)
	{
		w << NC::Format::Bold << "\n" << m->Name << ":" << NC::Format::NoBold << " ";
		ShowTag(w, s.getTags(m->Get));
	}
}
