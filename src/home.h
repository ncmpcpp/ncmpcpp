/***************************************************************************
 *   Copyright (C) 2008-2011 by Andrzej Rybczak                            *
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

#ifndef _HOME_H
#define _HOME_H

#include <cstdlib>
#include <string>

#ifdef WIN32
# define _WIN32_IE 0x0400
# include <shlobj.h>

inline std::string _GetHomeFolder()
{
	char path[300];
	return SHGetSpecialFolderPath(0, path, CSIDL_PERSONAL, 0) ? path : "";
}

# define GET_HOME_FOLDER _GetHomeFolder()
#else
# define GET_HOME_FOLDER getenv("HOME") ? getenv("HOME") : "";
#endif // WIN32

const std::string home_path = GET_HOME_FOLDER;

#undef GET_HOME_FOLDER

#endif

