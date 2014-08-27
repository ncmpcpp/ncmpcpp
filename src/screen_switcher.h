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

#ifndef NCMPCPP_SCREEN_SWITCHER_H
#define NCMPCPP_SCREEN_SWITCHER_H

#include "global.h"

class SwitchTo
{
	template <bool ToBeExecuted, typename ScreenT> struct TabbableAction_ {
		static void execute(ScreenT *) { }
	};
	template <typename ScreenT> struct TabbableAction_<true, ScreenT> {
		static void execute(ScreenT *screen) {
			using Global::myScreen;
			// it has to work only if both current and previous screens are Tabbable
			if (dynamic_cast<Tabbable *>(myScreen))
				screen->setPreviousScreen(myScreen);
		}
	};
	
public:
	template <typename ScreenT>
	static void execute(ScreenT *screen)
	{
		using Global::myScreen;
		using Global::myLockedScreen;
		
		const bool isScreenMergable = screen->isMergable() && myLockedScreen;
		const bool isScreenTabbable = std::is_base_of<Tabbable, ScreenT>::value;
		
		assert(myScreen != screen);
		if (isScreenMergable)
			updateInactiveScreen(screen);
		if (screen->hasToBeResized || isScreenMergable)
			screen->resize();
		TabbableAction_<isScreenTabbable, ScreenT>::execute(screen);
		myScreen = screen;
	}
};

#endif // NCMPCPP_SCREEN_SWITCHER_H
