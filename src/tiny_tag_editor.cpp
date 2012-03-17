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

#include "tiny_tag_editor.h"

#ifdef HAVE_TAGLIB_H

// taglib includes
#include "mpegfile.h"
#include "vorbisfile.h"
#include "flacfile.h"

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "song_info.h"
#include "playlist.h"
#include "search_engine.h"
#include "tag_editor.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myOldScreen;

TinyTagEditor *myTinyTagEditor = new TinyTagEditor;

void TinyTagEditor::Init()
{
	w = new Menu<Buffer>(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	w->HighlightColor(Config.main_highlight_color);
	w->CyclicScrolling(Config.use_cyclic_scrolling);
	w->CenteredCursor(Config.centered_cursor);
	w->SetItemDisplayer(Display::Generic);
	isInitialized = 1;
}

void TinyTagEditor::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->Resize(width, MainHeight);
	w->MoveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

void TinyTagEditor::SwitchTo()
{
	using Global::myScreen;
	using Global::myLockedScreen;
	
	if (itsEdited.isStream())
	{
		ShowMessage("Streams cannot be edited!");
	}
	else if (GetTags())
	{
		if (myLockedScreen)
			UpdateInactiveScreen(this);
		
		if (hasToBeResized || myLockedScreen)
			Resize();
		
		myOldScreen = myScreen;
		myScreen = this;
		Global::RedrawHeader = 1;
	}
	else
	{
		std::string full_path;
		if (itsEdited.isFromDB())
			full_path += Config.mpd_music_dir;
		full_path += itsEdited.GetFile();
		
		std::string message = "Couldn't read file \"";
		message += Shorten(TO_WSTRING(full_path), COLS-message.length()-3);
		message += "\"!";
		ShowMessage("%s", message.c_str());
	}
}

std::basic_string<my_char_t> TinyTagEditor::Title()
{
	return U("Tiny tag editor");
}

void TinyTagEditor::EnterPressed()
{
	size_t option = w->Choice();
	MPD::Song &s = itsEdited;
	
	LockStatusbar();
	if (option < 19) // separator after comment
	{
		size_t pos = option-8;
		Statusbar() << fmtBold << SongInfo::Tags[pos].Name << ": " << fmtBoldEnd;
		s.SetTags(SongInfo::Tags[pos].Set, Global::wFooter->GetString(s.GetTags(SongInfo::Tags[pos].Get)));
		w->at(option).Clear();
		w->at(option) << fmtBold << SongInfo::Tags[pos].Name << ':' << fmtBoldEnd << ' ';
		ShowTag(w->at(option), s.GetTags(SongInfo::Tags[pos].Get));
	}
	else if (option == 20)
	{
		Statusbar() << fmtBold << "Filename: " << fmtBoldEnd;
		std::string filename = s.GetNewName().empty() ? s.GetName() : s.GetNewName();
		size_t dot = filename.rfind(".");
		std::string extension = filename.substr(dot);
		filename = filename.substr(0, dot);
		std::string new_name = Global::wFooter->GetString(filename);
		s.SetNewName(new_name + extension);
		w->at(option).Clear();
		w->at(option) << fmtBold << "Filename:" << fmtBoldEnd << ' ' << (s.GetNewName().empty() ? s.GetName() : s.GetNewName());
	}
	UnlockStatusbar();
	
	if (option == 22)
	{
		ShowMessage("Updating tags...");
		if (TagEditor::WriteTags(s))
		{
			ShowMessage("Tags updated!");
			if (s.isFromDB())
			{
				Mpd.UpdateDirectory(locale_to_utf_cpy(s.GetDirectory()));
				if (myOldScreen == mySearcher) // songs from search engine are not updated automatically
					*mySearcher->Main()->Current().second = s;
			}
			else
			{
				if (myOldScreen == myPlaylist)
					myPlaylist->Items->Current() = s;
				else if (myOldScreen == myBrowser)
					myBrowser->GetDirectory(myBrowser->CurrentDir());
			}
		}
		else
			ShowMessage("Error while writing tags!");
	}
	if (option > 21)
		myOldScreen->SwitchTo();
}

void TinyTagEditor::MouseButtonPressed(MEVENT me)
{
	if (w->Empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->Size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		if (!w->Goto(me.y))
			return;
		if (me.bstate & BUTTON3_PRESSED)
		{
			w->Refresh();
			EnterPressed();
		}
	}
	else
		Screen< Menu<Buffer> >::MouseButtonPressed(me);
}

bool TinyTagEditor::SetEdited(MPD::Song *s)
{
	if (!s)
		return false;
	itsEdited = *s;
	return true;
}

bool TinyTagEditor::GetTags()
{
	MPD::Song &s = itsEdited;
	
	std::string path_to_file;
	if (s.isFromDB())
		path_to_file += Config.mpd_music_dir;
	path_to_file += s.GetFile();
	
	TagLib::FileRef f(path_to_file.c_str());
	if (f.isNull())
		return false;
	s.SetComment(f.tag()->comment().to8Bit(1));
	
	std::string ext = s.GetFile();
	ext = ext.substr(ext.rfind(".")+1);
	ToLower(ext);
	
	if (!isInitialized)
		Init();
	
	w->Clear();
	w->Reset();
	
	w->ResizeList(24);
	
	for (size_t i = 0; i < 7; ++i)
		w->Static(i, 1);
	
	w->IntoSeparator(7);
	w->IntoSeparator(19);
	w->IntoSeparator(21);
	
	if (!extendedTagsSupported(f.file()))
	{
		w->Static(10, 1);
		for (size_t i = 15; i <= 17; ++i)
			w->Static(i, 1);
	}
	
	w->Highlight(8);
	
	w->at(0) << fmtBold << Config.color1 << "Song name: " << fmtBoldEnd << Config.color2 << s.GetName() << clEnd;
	w->at(1) << fmtBold << Config.color1 << "Location in DB: " << fmtBoldEnd << Config.color2;
	ShowTag(w->at(1), s.GetDirectory());
	w->at(1) << clEnd;
	w->at(3) << fmtBold << Config.color1 << "Length: " << fmtBoldEnd << Config.color2 << s.GetLength() << clEnd;
	w->at(4) << fmtBold << Config.color1 << "Bitrate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps" << clEnd;
	w->at(5) << fmtBold << Config.color1 << "Sample rate: " << fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz" << clEnd;
	w->at(6) << fmtBold << Config.color1 << "Channels: " << fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << clDefault;
	
	unsigned pos = 8;
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m, ++pos)
	{
		w->at(pos) << fmtBold << m->Name << ":" << fmtBoldEnd << ' ';
		ShowTag(w->at(pos), s.GetTags(m->Get));
	}
	
	w->at(20) << fmtBold << "Filename:" << fmtBoldEnd << ' ' << s.GetName();
	
	w->at(22) << "Save";
	w->at(23) << "Cancel";
	return true;
}

bool TinyTagEditor::extendedTagsSupported(TagLib::File *f)
{
	return	dynamic_cast<TagLib::MPEG::File *>(f)
	||	dynamic_cast<TagLib::Ogg::Vorbis::File *>(f)
	||	dynamic_cast<TagLib::FLAC::File *>(f);
}

#endif // HAVE_TAGLIB_H

