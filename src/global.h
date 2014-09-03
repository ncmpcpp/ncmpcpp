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

#ifndef NCMPCPP_GLOBAL_H
#define NCMPCPP_GLOBAL_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "mpdpp.h"
#include "screen.h"

namespace Global {//

// currently active screen (displayed in main window)
extern BaseScreen *myScreen;

// points at the screen that was locked (or is null if no screen is locked)
extern BaseScreen *myLockedScreen;

// points at inactive screen, if locking was enabled and two screens are displayed
extern BaseScreen *myInactiveScreen;

// header window (above main window)
extern NC::Window *wHeader;

// footer window (below main window)
extern NC::Window *wFooter;

// Y coordinate of top of main window
extern size_t MainStartY;

// height of main window
extern size_t MainHeight;

// indicates whether seeking action in currently in progress
extern bool SeekingInProgress;

// string that represents volume in right top corner. being global
// to be used for calculating width offsets in various files.
extern std::string VolumeState;

// global timer
extern boost::posix_time::ptime Timer;

}

#endif // NCMPCPP_GLOBAL_H
