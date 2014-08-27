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

#include "lastfm.h"

#ifdef HAVE_CURL_CURL_H

#include "helpers.h"
#include "charset.h"
#include "global.h"
#include "statusbar.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

Lastfm *myLastfm;

Lastfm::Lastfm()
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border::None))
{ }

void Lastfm::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::wstring Lastfm::title()
{
	return m_title;
}

void Lastfm::update()
{
	if (m_worker.valid() && m_worker.is_ready())
		getResult();
}

void Lastfm::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
	{
		SwitchTo::execute(this);
		drawHeader();
	}
	else
		switchToPreviousScreen();
}

void Lastfm::getResult()
{
	auto result = m_worker.get();
	if (result.first)
	{
		w.clear();
		w << Charset::utf8ToLocale(result.second);
		m_service->beautifyOutput(w);
	}
	else
		w << " " << NC::Color::Red << result.second << NC::Color::End;
	w.flush();
	w.refresh();
	// reset m_worker so it's no longer valid
	m_worker = boost::future<LastFm::Service::Result>();
}

#endif // HVAE_CURL_CURL_H
