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

#ifndef _SEARCH_ENGINE_H
#define _SEARCH_ENGINE_H

#include "mpdpp.h"
#include "ncmpcpp.h"

class SearchEngine : public Screen< Menu< std::pair<Buffer *, MPD::Song *> > >
{
	public:
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed();
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		virtual MPD::Song *CurrentSong();
		virtual MPD::Song *GetSong(size_t pos) { return !w->isSeparator(pos) ? w->at(pos).second : 0; }
		
		virtual bool allowsSelection() { return w->Choice() >= StaticOptions; }
		virtual void ReverseSelection() { w->ReverseSelection(StaticOptions); }
		virtual void GetSelectedSongs(MPD::SongList &);
		
		virtual void ApplyFilter(const std::string &);
		
		virtual List *GetList() { return w->Size() >= StaticOptions ? w : 0; }
		
		virtual bool isMergable() { return true; }
		
		void UpdateFoundList();
		void Scroll(int);
		void SelectAlbum();
		
		static size_t StaticOptions;
		static size_t SearchButton;
		static size_t ResetButton;
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		void Prepare();
		void Search();
		void Reset();
		
		const char **SearchMode;
		
		static std::string SearchEngineOptionToString(const std::pair<Buffer *, MPD::Song *> &, void *);
		
		static const char *SearchModes[];
		
		static const size_t ConstraintsNumber = 10;
		static const char *ConstraintsNames[];
		std::string itsConstraints[ConstraintsNumber];
		
		static bool MatchToPattern;
};

extern SearchEngine *mySearcher;

#endif

