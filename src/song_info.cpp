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
#include "song_info.h"
#include "tag_editor.h"

using Global::MainHeight;
using Global::MainStartY;

SongInfo *mySongInfo = new SongInfo;

const SongInfo::Metadata SongInfo::Tags[] =
{
 { "Title",		&MPD::Song::GetTitle,		&MPD::Song::SetTitle		},
 { "Artist",		&MPD::Song::GetArtist,		&MPD::Song::SetArtist		},
 { "Album Artist",	&MPD::Song::GetAlbumArtist,	&MPD::Song::SetAlbumArtist	},
 { "Album",		&MPD::Song::GetAlbum,		&MPD::Song::SetAlbum		},
 { "Year",		&MPD::Song::GetDate,		&MPD::Song::SetDate		},
 { "Track",		&MPD::Song::GetTrack,		&MPD::Song::SetTrack		},
 { "Genre",		&MPD::Song::GetGenre,		&MPD::Song::SetGenre		},
 { "Composer",		&MPD::Song::GetComposer,	&MPD::Song::SetComposer		},
 { "Performer",		&MPD::Song::GetPerformer,	&MPD::Song::SetPerformer	},
 { "Disc",		&MPD::Song::GetDisc,		&MPD::Song::SetDisc		},
 { "Comment",		&MPD::Song::GetComment,		&MPD::Song::SetComment		},
 { 0,			0,				0				}
};

void SongInfo::Init()
{
	w = new Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	isInitialized = 1;
}

void SongInfo::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->Resize(width, MainHeight);
	w->MoveTo(x_offset, MainStartY);
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
	
	MPD::Song *s = myScreen->CurrentSong();
	
	if (!s)
		return;
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	myOldScreen = myScreen;
	myScreen = this;
	
	Global::RedrawHeader = 1;
	
	w->Clear();
	w->Reset();
	PrepareSong(*s);
	w->Flush();
}

void SongInfo::PrepareSong(MPD::Song &s)
{
#	ifdef HAVE_TAGLIB_H
	std::string path_to_file;
	if (s.isFromDB())
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	TagLib::FileRef f(path_to_file.c_str());
	if (!f.isNull())
		s.SetComment(f.tag()->comment().to8Bit(1));
#	endif // HAVE_TAGLIB_H
	
	*w << fmtBold << Config.color1 << "Filename: " << fmtBoldEnd << Config.color2 << s.GetName() << "\n" << clEnd;
	*w << fmtBold << "Directory: " << fmtBoldEnd << Config.color2;
	ShowTag(*w, s.GetDirectory());
	*w << "\n\n" << clEnd;
	*w << fmtBold << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << "\n" << clEnd;
#	ifdef HAVE_TAGLIB_H
	if (!f.isNull())
	{
		*w << fmtBold << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps\n" << clEnd;
		*w << fmtBold << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz\n" << clEnd;
		*w << fmtBold << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << "\n" << clDefault;
	}
#	endif // HAVE_TAGLIB_H
	*w << clDefault;
	
	for (const Metadata *m = Tags; m->Name; ++m)
	{
		*w << fmtBold << "\n" << m->Name << ": " << fmtBoldEnd;
		ShowTag(*w, s.GetTags(m->Get));
	}
}

