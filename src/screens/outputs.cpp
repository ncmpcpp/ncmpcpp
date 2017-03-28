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

#include "screens/outputs.h"

#ifdef ENABLE_OUTPUTS

#include "curses/menu_impl.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "screens/screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

Outputs *myOutputs;

Outputs::Outputs()
: Screen(NC::Menu<MPD::Output>(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border()))
{
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	setHighlightFixes(w);
	w.setItemDisplayer([](NC::Menu<MPD::Output> &menu) {
			auto &output = menu.drawn()->value();
			if (output.enabled())
				menu << NC::Format::Bold;
			menu << Charset::utf8ToLocale(output.name());
			if (output.enabled())
				menu << NC::Format::NoBold;
	});
}

void Outputs::switchTo()
{
	SwitchTo::execute(this);
	drawHeader();
}

void Outputs::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::wstring Outputs::title()
{
	return L"Outputs";
}

void Outputs::mouseButtonPressed(MEVENT me)
{
	if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
		return;
	if (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED)
	{
		w.Goto(me.y);
		if (me.bstate & BUTTON3_PRESSED)
			toggleOutput();
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

void Outputs::fetchList()
{
	w.clear();
	for (MPD::OutputIterator out = Mpd.GetOutputs(), end; out != end; ++out)
		w.addItem(std::move(*out));
}

void Outputs::toggleOutput()
{
	if (w.current()->value().enabled())
	{
		Mpd.DisableOutput(w.choice());
		Statusbar::printf("Output \"%s\" disabled", w.current()->value().name());
	}
	else
	{
		Mpd.EnableOutput(w.choice());
		Statusbar::printf("Output \"%s\" enabled", w.current()->value().name());
	}
}

#endif // ENABLE_OUTPUTS
