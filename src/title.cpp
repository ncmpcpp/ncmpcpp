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

#include <cstring>
#include <iostream>

#include "global.h"
#include "settings.h"
#include "title.h"
#include "utility/wide_string.h"

#ifdef USE_PDCURSES
void windowTitle(const std::string &) { }
#else
void windowTitle(const std::string &status)
{
	if (strcmp(getenv("TERM"), "linux") && Config.set_window_title)
		std::cout << "\033]0;" << status << "\7" << std::flush;
}
#endif // USE_PDCURSES

void drawHeader()
{
	using Global::myScreen;
	using Global::wHeader;
	using Global::VolumeState;
	
	if (!Config.header_visibility)
		return;
	if (Config.new_design)
	{
		std::wstring title = myScreen->title();
		*wHeader << NC::XY(0, 3) << wclrtoeol;
		*wHeader << NC::Format::Bold << Config.alternative_ui_separator_color;
		mvwhline(wHeader->raw(), 2, 0, 0, COLS);
		mvwhline(wHeader->raw(), 4, 0, 0, COLS);
		*wHeader << NC::XY((COLS-wideLength(title))/2, 3);
		*wHeader << Config.header_color << title << NC::Color::End;
		*wHeader << NC::Color::End << NC::Format::NoBold;
	}
	else
	{
		*wHeader << NC::XY(0, 0) << wclrtoeol << NC::Format::Bold << myScreen->title() << NC::Format::NoBold;
		*wHeader << Config.volume_color;
		*wHeader << NC::XY(wHeader->getWidth()-VolumeState.length(), 0) << VolumeState;
		*wHeader << NC::Color::End;
	}
	wHeader->refresh();
}
