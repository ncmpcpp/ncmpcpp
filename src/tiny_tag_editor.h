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

#ifndef _TINY_TAG_EDITOR_H
#define _TINY_TAG_EDITOR_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_TAGLIB_H

// taglib includes
#include "tfile.h"

#include "screen.h"

class TinyTagEditor : public Screen< Menu<Buffer> >
{
	public:
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed() { }
		virtual void MouseButtonPressed(MEVENT);
		
		virtual bool allowsSelection() { return false; }
		
		virtual List *GetList() { return 0; }
		
		virtual bool isMergable() { return true; }
		
		bool SetEdited(MPD::Song *);
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		bool GetTags();
		MPD::Song itsEdited;
		
		static bool extendedTagsSupported(TagLib::File *);
};

extern TinyTagEditor *myTinyTagEditor;

#endif // HAVE_TAGLIB_H

#endif

