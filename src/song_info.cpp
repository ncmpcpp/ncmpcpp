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

#ifdef HAVE_TAGLIB_H
# include "fileref.h"
# include "tag.h"
#endif // HAVE_TAGLIB_H

using Global::MainHeight;
using Global::MainStartY;

SongInfo *mySongInfo = new SongInfo;

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
 { 0,              0,                          0                                  }
};

void SongInfo::Init()
{
	w = new NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone);
	isInitialized = 1;
}

void SongInfo::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->resize(width, MainHeight);
	w->moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::basic_string<my_char_t> SongInfo::Title()
{
	return U("Song info");
}

void SongInfo::SwitchTo()
{
	using Global::myScreen;
	using Global::myOldScreen;
	using Global::myLockedScreen;
	
	if (myScreen == this)
		return myOldScreen->SwitchTo();
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	auto s = currentSong(myScreen);
	if (!s)
		return;
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	myOldScreen = myScreen;
	myScreen = this;
	
	Global::RedrawHeader = true;
	
	w->clear();
	w->reset();
	PrepareSong(*s);
	w->flush();
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
	
	*w << NC::fmtBold << Config.color1 << U("Filename: ") << NC::fmtBoldEnd << Config.color2 << s.getName() << '\n' << NC::clEnd;
	*w << NC::fmtBold << U("Directory: ") << NC::fmtBoldEnd << Config.color2;
	ShowTag(*w, s.getDirectory());
	*w << U("\n\n") << NC::clEnd;
	*w << NC::fmtBold << U("Length: ") << NC::fmtBoldEnd << Config.color2 << s.getLength() << '\n' << NC::clEnd;
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		*w << NC::fmtBold << U("Bitrate: ") << NC::fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << U(" kbps\n") << NC::clEnd;
		*w << NC::fmtBold << U("Sample rate: ") << NC::fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << U(" Hz\n") << NC::clEnd;
		*w << NC::fmtBold << U("Channels: ") << NC::fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? U("Mono") : U("Stereo")) << '\n' << NC::clDefault;
	}
#	endif // HAVE_TAGLIB_H
	*w << NC::clDefault;
	
	for (const Metadata *m = Tags; m->Name; ++m)
	{
		*w << NC::fmtBold << '\n' << TO_WSTRING(m->Name) << U(": ") << NC::fmtBoldEnd;
		ShowTag(*w, s.getTags(m->Get));
	}
}
