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

#ifndef NCMPCPP_TAG_EDITOR_H
#define NCMPCPP_TAG_EDITOR_H

#include "config.h"

#ifdef HAVE_TAGLIB_H

#include <list>

#include "interfaces.h"
#include "mutable_song.h"
#include "regex_filter.h"
#include "screen.h"

struct TagEditor: Screen<NC::Window *>, Filterable, HasColumns, HasSongs, Searchable, Tabbable
{
	TagEditor();
	
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::TagEditor; }
	
	virtual void refresh() OVERRIDE;
	virtual void update() OVERRIDE;
	
	virtual void enterPressed() OVERRIDE;
	virtual void spacePressed() OVERRIDE;
	virtual void mouseButtonPressed(MEVENT) OVERRIDE;
	
	virtual bool isMergable() OVERRIDE { return true; }
	
	// Filterable implementation
	virtual bool allowsFiltering() OVERRIDE;
	virtual std::string currentFilter() OVERRIDE;
	virtual void applyFilter(const std::string &filter) OVERRIDE;
	
	// Searchable implementation
	virtual bool allowsSearching() OVERRIDE;
	virtual bool search(const std::string &constraint) OVERRIDE;
	virtual void nextFound(bool wrap) OVERRIDE;
	virtual void prevFound(bool wrap) OVERRIDE;
	
	// HasSongs implementation
	virtual ProxySongList proxySongList() OVERRIDE;
	
	virtual bool allowsSelection() OVERRIDE;
	virtual void reverseSelection() OVERRIDE;
	virtual MPD::SongList getSelectedSongs() OVERRIDE;
	
	// HasColumns implementation
	virtual bool previousColumnAvailable() OVERRIDE;
	virtual void previousColumn() OVERRIDE;
	
	virtual bool nextColumnAvailable() OVERRIDE;
	virtual void nextColumn() OVERRIDE;
	
	// private members
	bool ifAnyModifiedAskForDiscarding();
	void LocateSong(const MPD::Song &s);
	const std::string &CurrentDir() { return itsBrowsedDir; }
	
	NC::Menu< std::pair<std::string, std::string> > *Dirs;
	NC::Menu<std::string> *TagTypes;
	NC::Menu<MPD::MutableSong> *Tags;
	
protected:
	virtual bool isLockable() OVERRIDE { return true; }
	
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

#endif // HAVE_TAGLIB_H

#endif // NCMPCPP_TAG_EDITOR_H

