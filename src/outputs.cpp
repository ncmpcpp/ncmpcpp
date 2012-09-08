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

#include "outputs.h"

#ifdef ENABLE_OUTPUTS

#include "display.h"
#include "global.h"
#include "settings.h"
#include "status.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

Outputs *myOutputs = new Outputs;

void Outputs::Init()
{
	w = new NC::Menu<MPD::Output>(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone);
	w->cyclicScrolling(Config.use_cyclic_scrolling);
	w->centeredCursor(Config.centered_cursor);
	w->setHighlightColor(Config.main_highlight_color);
	w->setItemDisplayer(Display::Outputs);
	
	isInitialized = 1;
	FetchList();
}

void Outputs::SwitchTo()
{
	using Global::myLockedScreen;
	
	if (myScreen == this)
		return;
	
	if (!isInitialized)
		Init();
	
	if (myLockedScreen)
		UpdateInactiveScreen(this);
	
	if (hasToBeResized || myLockedScreen)
		Resize();
	
	if (myScreen != this && myScreen->isTabbable())
		Global::myPrevScreen = myScreen;
	myScreen = this;
	w->Window::clear();
	DrawHeader();
}

void Outputs::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->resize(width, MainHeight);
	w->moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::wstring Outputs::Title()
{
	return L"Outputs";
}

void Outputs::EnterPressed()
{
	if (w->current().value().isEnabled())
	{
		if (Mpd.DisableOutput(w->choice()))
			ShowMessage("Output \"%s\" disabled", w->current().value().name().c_str());
	}
	else
	{
		if (Mpd.EnableOutput(w->choice()))
			ShowMessage("Output \"%s\" enabled", w->current().value().name().c_str());
	}
	if (!Mpd.SupportsIdle())
		FetchList();
}

void Outputs::MouseButtonPressed(MEVENT me)
{
	if (w->empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->size())
		return;
	if (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED)
	{
		w->Goto(me.y);
		if (me.bstate & BUTTON3_PRESSED)
			EnterPressed();
	}
	else
		Screen< NC::Menu<MPD::Output> >::MouseButtonPressed(me);
}

void Outputs::FetchList()
{
	if (!isInitialized)
		return;
	w->clear();
	auto outputs = Mpd.GetOutputs();
	for (auto o = outputs.begin(); o != outputs.end(); ++o)
		w->addItem(*o, o->isEnabled());
	if (myScreen == this)
		w->refresh();
}

#endif // ENABLE_OUTPUTS

