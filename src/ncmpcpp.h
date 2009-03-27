/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#ifndef _NCMPCPP_H
#define _NCMPCPP_H

#include <cstdlib>

#include "window.h"
#include "menu.h"
#include "scrollpad.h"

#ifdef WIN32
# define HOME_ENV "USERPROFILE"
#else
# define HOME_ENV "HOME"
#endif // WIN32

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#else
# define pthread_mutex_lock(x);
# define pthread_mutex_unlock(x);
# define pthread_exit(x) return (x)
#endif // HAVE_PTHREAD_H

using namespace NCurses;

typedef std::pair<std::string, std::string> string_pair;

const int ncmpcpp_window_timeout = 250;

const std::string home_path = getenv(HOME_ENV) ? getenv(HOME_ENV) : "";

#undef HOME_ENV

#endif

