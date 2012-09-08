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

#ifndef _TAG_EDITOR_H
#define _TAG_EDITOR_H

#include "config.h"

#ifdef HAVE_TAGLIB_H

#include <list>

#include "interfaces.h"
#include "mpdpp.h"
#include "mutable_song.h"
#include "regex_filter.h"
#include "screen.h"

class TagEditor : public Screen<NC::Window>, public Filterable, public HasSongs, public Searchable
{
	public:
		TagEditor() : FParser(0), FParserHelper(0), FParserLegend(0), FParserPreview(0), itsBrowsedDir("/") { }
		
		virtual void Resize();
		virtual void SwitchTo();
		
		virtual std::wstring Title();
		
		virtual void Refresh();
		virtual void Update();
		
		virtual void EnterPressed();
		virtual void SpacePressed();
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		/// Filterable implementation
		virtual bool allowsFiltering();
		virtual std::string currentFilter();
		virtual void applyFilter(const std::string &filter);
		
		/// Searchable implementation
		virtual bool allowsSearching();
		virtual bool search(const std::string &constraint);
		virtual void nextFound(bool wrap);
		virtual void prevFound(bool wrap);
		
		/// HasSongs implementation
		virtual std::shared_ptr<ProxySongList> getProxySongList();
		
		virtual bool allowsSelection();
		virtual void reverseSelection();
		virtual MPD::SongList getSelectedSongs();
		
		virtual bool isMergable() { return true; }
		
		bool isNextColumnAvailable();
		bool NextColumn();
		bool isPrevColumnAvailable();
		bool PrevColumn();
		
		void LocateSong(const MPD::Song &s);
		
		NC::Menu< std::pair<std::string, std::string> > *Dirs;
		NC::Menu<std::string> *TagTypes;
		NC::Menu<MPD::MutableSong> *Tags;
		
		/// NOTICE: this string is always in utf8, no need to convert it
		const std::string &CurrentDir() { return itsBrowsedDir; }
		
		static void ReadTags(MPD::MutableSong &);
		static bool WriteTags(MPD::MutableSong &);
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		void SetDimensions(size_t, size_t);
		
		std::vector<MPD::MutableSong *> EditedSongs;
		NC::Menu<std::string> *FParserDialog;
		NC::Menu<std::string> *FParser;
		NC::Scrollpad *FParserHelper;
		NC::Scrollpad *FParserLegend;
		NC::Scrollpad *FParserPreview;
		bool FParserUsePreview;
		
		std::string itsBrowsedDir;
		std::string itsHighlightedDir;
};

extern TagEditor *myTagEditor;

#endif

#endif

