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

#include "global.h"
#include "helpers.h"
#include "song_info.h"
#include "tag_editor.h"
#include "title.h"
#include "screen_switcher.h"

#ifdef HAVE_TAGLIB_H
# include "fileref.h"
# include "tag.h"
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
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone))
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
#	ifdef HAVE_TAGLIB_H
	std::string path_to_file;
	if (s.isFromDatabase())
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.getURI();
	TagLib::FileRef f(path_to_file.c_str());
#	endif // HAVE_TAGLIB_H
	
	w << NC::fmtBold << Config.color1 << L"Filename: " << NC::fmtBoldEnd << Config.color2 << s.getName() << '\n' << NC::clEnd;
	w << NC::fmtBold << L"Directory: " << NC::fmtBoldEnd << Config.color2;
	ShowTag(w, s.getDirectory());
	w << L"\n\n" << NC::clEnd;
	w << NC::fmtBold << L"Length: " << NC::fmtBoldEnd << Config.color2 << s.getLength() << '\n' << NC::clEnd;
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		w << NC::fmtBold << L"Bitrate: " << NC::fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << L" kbps\n" << NC::clEnd;
		w << NC::fmtBold << L"Sample rate: " << NC::fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << L" Hz\n" << NC::clEnd;
		w << NC::fmtBold << L"Channels: " << NC::fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? L"Mono" : L"Stereo") << '\n' << NC::clDefault;
	}
#	endif // HAVE_TAGLIB_H
	w << NC::clDefault;
	
	for (const Metadata *m = Tags; m->Name; ++m)
	{
		w << NC::fmtBold << '\n' << ToWString(m->Name) << L": " << NC::fmtBoldEnd;
		ShowTag(w, s.getTags(m->Get, Config.tags_separator));
	}
}
