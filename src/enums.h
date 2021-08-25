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

#ifndef NCMPCPP_ENUMS_H
#define NCMPCPP_ENUMS_H

#include "config.h"
#include <iostream>

enum class SearchDirection { Backward, Forward };
std::ostream &operator<<(std::ostream &os, SearchDirection sd);
std::istream &operator>>(std::istream &is, SearchDirection &sd);

enum class SpaceAddMode { AddRemove, AlwaysAdd };
std::ostream &operator<<(std::ostream &os, SpaceAddMode sam);
std::istream &operator>>(std::istream &is, SpaceAddMode &sam);

enum class SortMode { Type, Name, ModificationTime, CustomFormat, None };
std::ostream &operator<<(std::ostream &os, SortMode sm);
std::istream &operator>>(std::istream &is, SortMode &sm);

enum class DisplayMode { Classic, Columns };
std::ostream &operator<<(std::ostream &os, DisplayMode dm);
std::istream &operator>>(std::istream &is, DisplayMode &dm);

enum class Design { Classic, Alternative };
std::ostream &operator<<(std::ostream &os, Design ui);
std::istream &operator>>(std::istream &is, Design &ui);

enum class VisualizerType {
	Wave,
	WaveFilled,
#	ifdef HAVE_FFTW3_H
	Spectrum,
#	endif // HAVE_FFTW3_H
	Ellipse
};
std::ostream &operator<<(std::ostream &os, VisualizerType vt);
std::istream &operator>>(std::istream &is, VisualizerType &vt);

#endif // NCMPCPP_ENUMS_H
