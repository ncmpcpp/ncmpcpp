/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#ifndef NCMPCPP_OUTPUTS_H
#define NCMPCPP_OUTPUTS_H

#include "config.h"

#ifdef ENABLE_OUTPUTS

#include "interfaces.h"
#include "menu.h"
#include "mpdpp.h"
#include "screens/screen.h"

struct Outputs: Screen<NC::Menu<MPD::Output>>, Tabbable
{
	Outputs();
	
	// Screen< NC::Menu<MPD::Output> > implementation
	virtual void switchTo() override;
	virtual void resize() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::Outputs; }
	
	virtual void update() override { }
	
	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override { return true; }
	virtual bool isMergable() override { return true; }
	
	// private members
	void fetchList();
	void toggleOutput();
};

extern Outputs *myOutputs;

#endif // ENABLE_OUTPUTS

#endif // NCMPCPP_OUTPUTS_H

