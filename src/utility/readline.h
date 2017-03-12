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

#ifndef NCMPCPP_UTILITY_READLINE_H
#define NCMPCPP_UTILITY_READLINE_H

#include "config.h"

#if defined(HAVE_READLINE_HISTORY_H)
# include <readline/history.h>
#endif // HAVE_READLINE_HISTORY

#if defined(HAVE_READLINE_H)
# include <readline.h>
#elif defined(HAVE_READLINE_READLINE_H)
# include <readline/readline.h>
#else
# error "readline is not available"
#endif

#endif // NCMPCPP_READLINE_UTILITY_H
