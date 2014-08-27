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

#include "tiny_tag_editor.h"

#ifdef HAVE_TAGLIB_H

#include <boost/locale/conversion.hpp>

// taglib includes
#include <fileref.h>
#include <tag.h>

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "global.h"
#include "song_info.h"
#include "playlist.h"
#include "search_engine.h"
#include "statusbar.h"
#include "tag_editor.h"
#include "title.h"
#include "tags.h"
#include "screen_switcher.h"
#include "utility/string.h"

using Global::MainHeight;
using Global::MainStartY;

TinyTagEditor *myTinyTagEditor;

TinyTagEditor::TinyTagEditor()
: Screen(NC::Menu<NC::Buffer>(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border::None))
{
	w.setHighlightColor(Config.main_highlight_color);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer([](NC::Menu<NC::Buffer> &menu) {
		menu << menu.drawn()->value();
	});
}

void TinyTagEditor::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

void TinyTagEditor::switchTo()
{
	using Global::myScreen;
	if (itsEdited.isStream())
	{
		Statusbar::print("Streams can't be edited");
	}
	else if (getTags())
	{
		m_previous_screen = myScreen;
		SwitchTo::execute(this);
		drawHeader();
	}
	else
	{
		std::string full_path;
		if (itsEdited.isFromDatabase())
			full_path += Config.mpd_music_dir;
		full_path += itsEdited.getURI();
		
		const char msg[] = "Couldn't read file \"%1%\"";
		Statusbar::printf(msg, wideShorten(full_path, COLS-const_strlen(msg)));
	}
}

std::wstring TinyTagEditor::title()
{
	return L"Tiny tag editor";
}

void TinyTagEditor::enterPressed()
{
	size_t option = w.choice();
	Statusbar::lock();
	if (option < 19) // separator after comment
	{
		size_t pos = option-8;
		Statusbar::put() << NC::Format::Bold << SongInfo::Tags[pos].Name << ": " << NC::Format::NoBold;
		itsEdited.setTags(SongInfo::Tags[pos].Set, Global::wFooter->getString(
			itsEdited.getTags(SongInfo::Tags[pos].Get, Config.tags_separator)), Config.tags_separator);
		w.at(option).value().clear();
		w.at(option).value() << NC::Format::Bold << SongInfo::Tags[pos].Name << ':' << NC::Format::NoBold << ' ';
		ShowTag(w.at(option).value(), itsEdited.getTags(SongInfo::Tags[pos].Get, Config.tags_separator));
	}
	else if (option == 20)
	{
		Statusbar::put() << NC::Format::Bold << "Filename: " << NC::Format::NoBold;
		std::string filename = itsEdited.getNewName().empty() ? itsEdited.getName() : itsEdited.getNewName();
		size_t dot = filename.rfind(".");
		std::string extension = filename.substr(dot);
		filename = filename.substr(0, dot);
		std::string new_name = Global::wFooter->getString(filename);
		if (!new_name.empty())
		{
			itsEdited.setNewName(new_name + extension);
			w.at(option).value().clear();
			w.at(option).value() << NC::Format::Bold << "Filename:" << NC::Format::NoBold << ' ' << (itsEdited.getNewName().empty() ? itsEdited.getName() : itsEdited.getNewName());
		}
	}
	Statusbar::unlock();
	
	if (option == 22)
	{
		Statusbar::print("Updating tags...");
		if (Tags::write(itsEdited))
		{
			Statusbar::print("Tags updated");
			if (itsEdited.isFromDatabase())
				Mpd.UpdateDirectory(itsEdited.getDirectory());
			else
			{
				if (m_previous_screen == myPlaylist)
					myPlaylist->main().current().value() = itsEdited;
				else if (m_previous_screen == myBrowser)
					myBrowser->GetDirectory(myBrowser->CurrentDir());
			}
		}
		else
			Statusbar::print("Error while writing tags");
	}
	if (option > 21)
		m_previous_screen->switchTo();
}

void TinyTagEditor::mouseButtonPressed(MEVENT me)
{
	if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		if (!w.Goto(me.y))
			return;
		if (me.bstate & BUTTON3_PRESSED)
		{
			w.refresh();
			enterPressed();
		}
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

void TinyTagEditor::SetEdited(const MPD::Song &s)
{
	if (auto ms = dynamic_cast<const MPD::MutableSong *>(&s))
		itsEdited = *ms;
	else
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
	
	std::string ext = itsEdited.getURI();
	ext = boost::locale::to_lower(ext.substr(ext.rfind(".")+1));
	
	w.clear();
	w.reset();
	
	w.resizeList(24);
	
	for (size_t i = 0; i < 7; ++i)
		w.at(i).setInactive(true);
	
	w.at(7).setSeparator(true);
	w.at(19).setSeparator(true);
	w.at(21).setSeparator(true);
	
	if (!Tags::extendedSetSupported(f.file()))
	{
		w.at(10).setInactive(true);
		for (size_t i = 15; i <= 17; ++i)
			w.at(i).setInactive(true);
	}
	
	w.highlight(8);
	
	w.at(0).value() << NC::Format::Bold << Config.color1 << "Song name: " << NC::Format::NoBold << Config.color2 << itsEdited.getName() << NC::Color::End;
	w.at(1).value() << NC::Format::Bold << Config.color1 << "Location in DB: " << NC::Format::NoBold << Config.color2;
	ShowTag(w.at(1).value(), itsEdited.getDirectory());
	w.at(1).value() << NC::Color::End;
	w.at(3).value() << NC::Format::Bold << Config.color1 << "Length: " << NC::Format::NoBold << Config.color2 << itsEdited.getLength() << NC::Color::End;
	w.at(4).value() << NC::Format::Bold << Config.color1 << "Bitrate: " << NC::Format::NoBold << Config.color2 << f.audioProperties()->bitrate() << " kbps" << NC::Color::End;
	w.at(5).value() << NC::Format::Bold << Config.color1 << "Sample rate: " << NC::Format::NoBold << Config.color2 << f.audioProperties()->sampleRate() << " Hz" << NC::Color::End;
	w.at(6).value() << NC::Format::Bold << Config.color1 << "Channels: " << NC::Format::NoBold << Config.color2 << (f.audioProperties()->channels() == 1 ? "Mono" : "Stereo") << NC::Color::Default;
	
	unsigned pos = 8;
	for (const SongInfo::Metadata *m = SongInfo::Tags; m->Name; ++m, ++pos)
	{
		w.at(pos).value() << NC::Format::Bold << m->Name << ":" << NC::Format::NoBold << ' ';
		ShowTag(w.at(pos).value(), itsEdited.getTags(m->Get, Config.tags_separator));
	}
	
	w.at(20).value() << NC::Format::Bold << "Filename:" << NC::Format::NoBold << ' ' << itsEdited.getName();
	
	w.at(22).value() << "Save";
	w.at(23).value() << "Cancel";
	return true;
}

#endif // HAVE_TAGLIB_H

