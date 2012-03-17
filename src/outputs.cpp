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

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

Outputs *myOutputs = new Outputs;

void Outputs::Init()
{
	w = new Menu<MPD::Output>(0, MainStartY, COLS, MainHeight, "", Config.main_color, brNone);
	w->CyclicScrolling(Config.use_cyclic_scrolling);
	w->CenteredCursor(Config.centered_cursor);
	w->HighlightColor(Config.main_highlight_color);
	w->SetItemDisplayer(Display::Pairs);
	
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
	w->Window::Clear();
	
	Global::RedrawHeader = 1;
}

void Outputs::Resize()
{
	size_t x_offset, width;
	GetWindowResizeParams(x_offset, width);
	w->Resize(width, MainHeight);
	w->MoveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::basic_string<my_char_t> Outputs::Title()
{
	return U("Outputs");
}

void Outputs::EnterPressed()
{
	if (w->Current().second)
	{
		if (Mpd.DisableOutput(w->Choice()))
			ShowMessage("Output \"%s\" disabled", w->Current().first.c_str());
	}
	else
	{
		if (Mpd.EnableOutput(w->Choice()))
			ShowMessage("Output \"%s\" enabled", w->Current().first.c_str());
	}
	if (!Mpd.SupportsIdle())
		FetchList();
}

void Outputs::MouseButtonPressed(MEVENT me)
{
	if (w->Empty() || !w->hasCoords(me.x, me.y) || size_t(me.y) >= w->Size())
		return;
	if (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED)
	{
		w->Goto(me.y);
		if (me.bstate & BUTTON3_PRESSED)
			EnterPressed();
	}
	else
		Screen< Menu<MPD::Output> >::MouseButtonPressed(me);
}

void Outputs::FetchList()
{
	if (!isInitialized)
		return;
	MPD::OutputList ol;
	Mpd.GetOutputs(ol);
	w->Clear();
	for (MPD::OutputList::const_iterator it = ol.begin(); it != ol.end(); ++it)
		w->AddOption(*it, it->second);
	if (myScreen == this)
		w->Refresh();
}

#endif // ENABLE_OUTPUTS

