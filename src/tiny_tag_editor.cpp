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
#include "fileref.h"

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "helpers.h"
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
	w = new NC::Menu<NC::Buffer>(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone);
	w->setHighlightColor(Config.main_highlight_color);
	w->cyclicScrolling(Config.use_cyclic_scrolling);
	w->centeredCursor(Config.centered_cursor);
	w->setItemDisplayer(Display::Default<NC::Buffer>);
	isInitialized = 1;
}

void TinyTagEditor::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->resize(width, MainHeight);
	w->moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

void TinyTagEditor::SwitchTo()
{
	using Global::myScreen;
	using Global::myLockedScreen;
	
	if (itsEdited.isStream())
	{
		ShowMessage("Streams can't be edited");
	}
	else if (getTags())
	{
		if (myLockedScreen)
			UpdateInactiveScreen(this);
		
		if (hasToBeResized || myLockedScreen)
			Resize();
		
		myOldScreen = myScreen;
		myScreen = this;
		DrawHeader();
	}
	else
	{
		std::string full_path;
		if (itsEdited.isFromDatabase())
			full_path += Config.mpd_music_dir;
		full_path += itsEdited.getURI();
		
		const char msg[] = "Couldn't read file \"%ls\"";
		ShowMessage(msg, wideShorten(ToWString(full_path), COLS-const_strlen(msg)).c_str());
	}
}

std::wstring TinyTagEditor::Title()
{
	return L"Tiny tag editor";
}

void TinyTagEditor::EnterPressed()
{
	size_t option = w->choice();
	LockStatusbar();
	if (option < 19) // separator after comment
	{
		size_t pos = option-8;
		Statusbar() << NC::fmtBold << SongInfo::Tags[pos].Name << ": " << NC::fmtBoldEnd;
		itsEdited.setTag(SongInfo::Tags[pos].Set, Global::wFooter->getString(itsEdited.getTags(SongInfo::Tags[pos].Get)));
		w->at(option).value().clear();
		w->at(option).value() << NC::fmtBold << SongInfo::Tags[pos].Name << ':' << NC::fmtBoldEnd << ' ';
		ShowTag(w->at(option).value(), itsEdited.getTags(SongInfo::Tags[pos].Get));
	}
	else if (option == 20)
	{
		Statusbar() << NC::fmtBold << "Filename: " << NC::fmtBoldEnd;
		std::string filename = itsEdited.getNewURI().empty() ? itsEdited.getName() : itsEdited.getNewURI();
		size_t dot = filename.rfind(".");
		std::string extension = filename.substr(dot);
		filename = filename.substr(0, dot);
		std::string new_name = Global::wFooter->getString(filename);
		itsEdited.setNewURI(new_name + extension);
		w->at(option).value().clear();
		w->at(option).value() << NC::fmtBold << "Filename:" << NC::fmtBoldEnd << ' ' << (itsEdited.getNewURI().empty() ? itsEdited.getName() : itsEdited.getNewURI());
	}
	UnlockStatusbar();
	
	if (option == 22)
	{
		ShowMessage("Updating tags...");
		if (TagEditor::WriteTags(itsEdited))
		{
			ShowMessage("Tags updated");
			if (itsEdited.isFromDatabase())
				Mpd.UpdateDirectory(itsEdited.getDirectory());
			else
			{
				if (myOldScreen == myPlaylist)
					myPlaylist->Items->current().value() = itsEdited;
				else if (myOldScreen == myBrowser)
					myBrowser->GetDirectory(myBrowser->CurrentDir());
			}
		}
		else
			ShowMessage("Error while writing tags");
	}
	if (option > 21)
		myOldScreen->SwitchTo();
}

void TinyTagEditor::MouseButtonPressed(MEVENT me)
{
	if (w->empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		if (!w->Goto(me.y))
			return;
		if (me.bstate & BUTTON3_PRESSED)
		{
			w->refresh();
			EnterPressed();
		}
	}
	else
		Screen< NC::Menu<NC::Buffer> >::MouseButtonPressed(me);
}

void TinyTagEditor::SetEdited(const MPD::Song &s)
{
	itsEdited = s;
}

bool TinyTagEditor::getTags()
{
	std::string path_to_file;
	if (itsEdited.isFromDatabase())
		path_to_file += Config.mpd_music_dir;
	path_to_file += itsEdited.getURI();
	
	TagLib::FileRef f(path_to_file.c_str());
	if (f.isNull())
		return false;
	itsEdited.setComment(f.tag()->comment().to8Bit(1));
	
	std::string ext = itsEdited.getURI();
	ext = lowercase(ext.substr(ext.rfind(".")+1));
	
	if (!isInitialized)
		Init();
	
	w->clear();
	w->reset();
	
	w->resizeList(24);
	
	for (size_t i = 0; i < 7; ++i)
		w->at(i).setInactive(true);
	
	w->at(7).setSeparator(true);
	w->at(19).setSeparator(true);
	w->at(21).setSeparator(true);
	
	if (!extendedTagsSupported(f.file()))
	{
		w->at(10).setInactive(true);
		for (size_t i = 15; i <= 17; ++i)
			w->at(i).setInactive(true);
	}
	
	w->highlight(8);
	
	w->at(0).value() << NC::fmtBold << Config.color1 << "Song name: " << NC::fmtBoldEnd << Config.color2 << itsEdited.getName() << NC::clEnd;
	w->at(1).value() << NC::fmtBold << Config.color1 << "Location in DB: " << NC::fmtBoldEnd << Config.color2;
	ShowTag(w->at(1).value(), itsEdited.getDirectory());
	w->at(1).value() << NC::clEnd;
	w->at(3).value() << NC::fmtBold << Config.color1 << "Length: " << NC::fmtBoldEnd << Config.color2 << itsEdited.getLength() << NC::clEnd;
	w->at(4).value() << NC::fmtBold << Config.color1 << "Bitrate: " << NC::fmtBoldEnd << Config.color2 << f.audioProperties()->bitrate() << " kbps" << NC::clEnd;
	w->at(5).value() << NC::fmtBold << Config.color1 << "Sample rate: " << NC::fmtBoldEnd << Config.color2 << f.audioProperties()->sampleRate() << " Hz" << NC::clEnd;
	w->at(6).value() << NC::fmtBold << Config.color1 << "Channels: " << NC::fmtBoldEnd << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << NC::clDefault;
	
	unsigned pos = 8;
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m, ++pos)
	{
		w->at(pos).value() << NC::fmtBold << m->Name << ":" << NC::fmtBoldEnd << ' ';
		ShowTag(w->at(pos).value(), itsEdited.getTags(m->Get));
	}
	
	w->at(20).value() << NC::fmtBold << "Filename:" << NC::fmtBoldEnd << ' ' << itsEdited.getName();
	
	w->at(22).value() << "Save";
	w->at(23).value() << "Cancel";
	return true;
}

bool TinyTagEditor::extendedTagsSupported(TagLib::File *f)
{
	return	dynamic_cast<TagLib::MPEG::File *>(f)
	||	dynamic_cast<TagLib::Ogg::Vorbis::File *>(f)
	||	dynamic_cast<TagLib::FLAC::File *>(f);
}

#endif // HAVE_TAGLIB_H

