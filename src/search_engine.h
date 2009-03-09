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

#ifndef _SEARCH_ENGINE_H
#define _SEARCH_ENGINE_H

#include "mpdpp.h"
#include "ncmpcpp.h"

class SearchEngine : public Screen< Menu< std::pair<Buffer *, MPD::Song *> > >
{
	class SearchPattern : public MPD::Song
	{
		public:
			const std::string &Any() { return itsAnyField; }
			const std::string &Any(const std::string &s) { itsAnyField = s; return itsAnyField; }
			
			void Clear() { Song::Clear(); itsAnyField.clear(); }
			bool Empty() { return Song::Empty() && itsAnyField.empty(); }
			
		protected:
			std::string itsAnyField;
	};
	
	public:
		virtual void Init();
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::string Title();
		
		virtual void EnterPressed();
		virtual void SpacePressed();
		
		virtual MPD::Song *CurrentSong();
		
		virtual bool allowsSelection() { return w->Choice() >= StaticOptions; }
		virtual void ReverseSelection() { w->ReverseSelection(StaticOptions); }
		virtual void GetSelectedSongs(MPD::SongList &);
		
		virtual void ApplyFilter(const std::string &);
		
		virtual List *GetList() { return w->Size() >= StaticOptions ? w : 0; }
		
		void UpdateFoundList();
		
		static size_t StaticOptions;
		static size_t SearchButton;
		static size_t ResetButton;
		
		static const char *NormalMode;
		static const char *StrictMode;
		
	private:
		void Prepare();
		void Search();
		
		static std::string SearchEngineOptionToString(const std::pair<Buffer *, MPD::Song *> &, void *);
		
		SearchPattern itsPattern;
		
		static bool MatchToPattern;
		static int CaseSensitive;
};

extern SearchEngine *mySearcher;

#endif

