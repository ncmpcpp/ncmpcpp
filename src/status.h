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

#ifndef _STATUS_CHECKER_H
#define _STATUS_CHECKER_H

#include "mpdpp.h"
#include "ncmpcpp.h"

#ifndef USE_PDCURSES
 void WindowTitle(const std::string &);
#else
# define WindowTitle(x);
#endif // USE_PDCURSES

void LockProgressbar();
void UnlockProgressbar();

void LockStatusbar();
void UnlockStatusbar();

void TraceMpdStatus();
void NcmpcppStatusChanged(MPD::Connection *, MPD::StatusChanges, void *);
void NcmpcppErrorCallback(MPD::Connection *, int, const char *, void *);

Window &Statusbar();
void DrawProgressbar(unsigned elapsed, unsigned time);
void ShowMessage(const char *, ...) GNUC_PRINTF(1, 2);

void StatusbarMPDCallback();
void StatusbarGetStringHelper(const std::wstring &);
void StatusbarApplyFilterImmediately(const std::wstring &);

#endif

