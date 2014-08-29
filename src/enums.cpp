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

#include "enums.h"

std::ostream &operator<<(std::ostream &os, SpaceAddMode sam)
{
	switch (sam)
	{
		case SpaceAddMode::AddRemove:
			os << "add_remove";
			break;
		case SpaceAddMode::AlwaysAdd:
			os << "always_add";
			break;
	}
	return os;
}

std::istream &operator>>(std::istream &is, SpaceAddMode &sam)
{
	std::string ssam;
	is >> ssam;
	if (ssam == "add_remove")
		sam = SpaceAddMode::AddRemove;
	else if (ssam == "always_add")
		sam = SpaceAddMode::AlwaysAdd;
	else
		is.setstate(std::ios::failbit);
	return is;
}

std::ostream &operator<<(std::ostream &os, SortMode sm)
{
	switch (sm)
	{
		case SortMode::Name:
			os << "name";
			break;
		case SortMode::ModificationTime:
			os << "mtime";
			break;
		case SortMode::CustomFormat:
			os << "format";
			break;
		case SortMode::NoOp:
			os << "noop";
			break;
	}
	return os;
}

std::istream &operator>>(std::istream &is, SortMode &sm)
{
	std::string ssm;
	is >> ssm;
	if (ssm == "name")
		sm = SortMode::Name;
	else if (ssm == "mtime")
		sm = SortMode::ModificationTime;
	else if (ssm == "format")
		sm = SortMode::CustomFormat;
	else if (ssm == "noop")
		sm = SortMode::NoOp;
	else
		is.setstate(std::ios::failbit);
	return is;
}

std::ostream &operator<<(std::ostream &os, DisplayMode dm)
{
	switch (dm)
	{
		case DisplayMode::Classic:
			os << "classic";
			break;
		case DisplayMode::Columns:
			os << "columns";
			break;
	}
	return os;
}

std::istream &operator>>(std::istream &is, DisplayMode &dm)
{
	std::string sdm;
	is >> sdm;
	if (sdm == "classic")
		dm = DisplayMode::Classic;
	else if (sdm == "columns")
		dm = DisplayMode::Columns;
	else
		is.setstate(std::ios::failbit);
	return is;
}

std::ostream &operator<<(std::ostream &os, Design ui)
{
	switch (ui)
	{
		case Design::Classic:
			os << "classic";
			break;
		case Design::Alternative:
			os << "alternative";
			break;
	}
	return os;
}

std::istream &operator>>(std::istream &is, Design &ui)
{
	std::string sui;
	is >> sui;
	if (sui == "classic")
		ui = Design::Classic;
	else if (sui == "alternative")
		ui = Design::Alternative;
	else
		is.setstate(std::ios::failbit);
	return is;
}
