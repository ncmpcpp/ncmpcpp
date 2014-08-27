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

#include "global.h"
#include "helpers.h"
#include "song_info.h"
#include "tag_editor.h"
#include "tags.h"
#include "title.h"
#include "screen_switcher.h"

#ifdef HAVE_TAGLIB_H
# include "fileref.h"
# include "tag.h"
# include "boost/lexical_cast.hpp"
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
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border::None))
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

void SongInfo::PrepareSong(MPD::Song &s)
{
	w << NC::Format::Bold << Config.color1 << "Filename: " << NC::Format::NoBold << Config.color2 << s.getName() << '\n' << NC::Color::End;
	w << NC::Format::Bold << "Directory: " << NC::Format::NoBold << Config.color2;
	ShowTag(w, s.getDirectory());
	w << "\n\n" << NC::Color::End;
	w << NC::Format::Bold << "Length: " << NC::Format::NoBold << Config.color2 << s.getLength() << '\n' << NC::Color::End;
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
			std::string channels;
			switch (f.audioProperties()->channels())
			{
				case 1:
					channels = "Mono";
					break;
				case 2:
					channels = "Stereo";
					break;
				default:
					channels = boost::lexical_cast<std::string>(f.audioProperties()->channels());
					break;
			}
			w << NC::Format::Bold << "Bitrate: " << NC::Format::NoBold << Config.color2 << f.audioProperties()->bitrate() << " kbps\n" << NC::Color::End;
			w << NC::Format::Bold << "Sample rate: " << NC::Format::NoBold << Config.color2 << f.audioProperties()->sampleRate() << " Hz\n" << NC::Color::End;
			w << NC::Format::Bold << "Channels: " << NC::Format::NoBold << Config.color2 << channels << NC::Color::End << "\n";
			
			auto rginfo = Tags::readReplayGain(f.file());
			if (!rginfo.empty())
			{
				w << NC::Format::Bold << "\nReference loudness: " << NC::Format::NoBold << Config.color2 << rginfo.referenceLoudness() << NC::Color::End << "\n";
				w << NC::Format::Bold << "Track gain: " << NC::Format::NoBold << Config.color2 << rginfo.trackGain() << NC::Color::End << "\n";
				w << NC::Format::Bold << "Track peak: " << NC::Format::NoBold << Config.color2 << rginfo.trackPeak() << NC::Color::End << "\n";
				w << NC::Format::Bold << "Album gain: " << NC::Format::NoBold << Config.color2 << rginfo.albumGain() << NC::Color::End << "\n";
				w << NC::Format::Bold << "Album peak: " << NC::Format::NoBold << Config.color2 << rginfo.albumPeak() << NC::Color::End << "\n";
			}
		}
	}
#	endif // HAVE_TAGLIB_H
	w << NC::Color::Default;
	
	for (const Metadata *m = Tags; m->Name; ++m)
	{
		w << NC::Format::Bold << '\n' << m->Name << ": " << NC::Format::NoBold;
		ShowTag(w, s.getTags(m->Get, Config.tags_separator));
	}
}
