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

#ifndef HAVE_SEARCH_ENGINE_H
#define HAVE_SEARCH_ENGINE_H

#include "mpdpp.h"
#include "ncmpcpp.h"

const size_t search_engine_static_options = 19;
const size_t seach_engine_search_button = 14;
const size_t seach_engine_reset_button = 15;

void SearchEngineDisplayer(const std::pair<Buffer *, Song *> &, void *, Menu< std::pair<Buffer *, Song *> > *);
void UpdateFoundList();
void PrepareSearchEngine(Song &s);
void Search(Song &);

#endif

