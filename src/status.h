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

#ifndef NCMPCPP_STATUS_CHECKER_H
#define NCMPCPP_STATUS_CHECKER_H

#include "interfaces.h"
#include "mpdpp.h"

namespace Status {//

void handleClientError(MPD::ClientError &e);
void handleServerError(MPD::ServerError &e);

void trace(bool update_timer, bool update_window_timeout);
inline void trace() { trace(true, false); }
void update(int event);
void clear();

const MPD::Status &get();

// get current elapsed time (the one from
// the status is outdated most of the time).
unsigned elapsedTime();

namespace Changes {//

void playlist(unsigned previous_version);
void storedPlaylists();
void database();
void playerState();
void songID();
void elapsedTime(bool update_elapsed);
void repeat(bool show_msg);
void random(bool show_msg);
void single(bool show_msg);
void consume(bool show_msg);
void crossfade(bool show_msg);
void dbUpdateState(bool show_msg);
void flags();
void mixer();
void outputs();

}

}

#endif // NCMPCPP_STATUS_CHECKER_H
