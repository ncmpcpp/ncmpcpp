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
		
		virtual void EnterPressed();
		
		bool SetEdited(MPD::Song *);
		
	protected:
		bool GetTags();
		MPD::Song itsEdited;
};

extern TinyTagEditor *myTinyTagEditor;

class TagEditor : public Screen<Window>
{
	public:
		virtual void Init();
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::string Title();
		
		virtual void Refresh();
		virtual void Update();
		
		virtual void EnterPressed();
		virtual void SpacePressed();
		
		virtual MPD::Song *CurrentSong();
		
		void NextColumn();
		void PrevColumn();
		
		Menu<string_pair> *LeftColumn;
		Menu<string_pair> *Albums;
		Menu<string_pair> *Dirs;
		Menu<std::string> *TagTypes;
		Menu<MPD::Song> *Tags;
		
		static void ReadTags(mpd_Song *);
		static bool WriteTags(MPD::Song &);
		
	protected:
		static std::string CapitalizeFirstLetters(const std::string &);
		static void CapitalizeFirstLetters(MPD::Song &);
		static void LowerAllLetters(MPD::Song &);
		
		static const size_t MiddleColumnWidth;
		static size_t LeftColumnWidth;
		static size_t MiddleColumnStartX;
		static size_t RightColumnWidth;
		static size_t RightColumnStartX;
};

extern TagEditor *myTagEditor;

typedef void (MPD::Song::*SongSetFunction)(const std::string &);
typedef std::string (MPD::Song::*SongGetFunction)() const;

std::string FindSharedDir(Menu<MPD::Song> *);
std::string FindSharedDir(const MPD::SongList &);

SongSetFunction IntoSetFunction(mpd_TagItems);

void DealWithFilenames(MPD::SongList &);

#endif

#endif

