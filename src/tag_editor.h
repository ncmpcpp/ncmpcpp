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

#include "ncmpcpp.h"

#ifdef HAVE_TAGLIB_H

#ifndef _TAG_EDITOR_H
#define _TAG_EDITOR_H

// taglib headers
#include "fileref.h"
#include "tag.h"

#include "mpdpp.h"
#include "screen.h"
#include "settings.h"

class TinyTagEditor : public Screen< Menu<Buffer> >
{
	public:
		virtual void Init();
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::string Title();
		
		void EnterPressed();
		
	protected:
		bool GetTags();
		MPD::Song itsEdited;
};

extern TinyTagEditor *myTinyTagEditor;

namespace TagEditor
{
	void Init();
	void Resize();
	void Refresh();
	void SwitchTo();
	
	void Update();
	
	void EnterPressed();
}

typedef void (MPD::Song::*SongSetFunction)(const std::string &);
typedef std::string (MPD::Song::*SongGetFunction)() const;

std::string FindSharedDir(Menu<MPD::Song> *);
std::string FindSharedDir(const MPD::SongList &);

SongSetFunction IntoSetFunction(mpd_TagItems);

void ReadTagsFromFile(mpd_Song *);
//bool GetSongTags(MPD::Song &);
bool WriteTags(MPD::Song &);

void __deal_with_filenames(MPD::SongList &);

void CapitalizeFirstLetters(MPD::Song &);
void LowerAllLetters(MPD::Song &);

#endif

#endif

