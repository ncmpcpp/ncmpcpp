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
#include "song_list.h"

struct TagsWindow: NC::Menu<MPD::MutableSong>, SongList
{
	TagsWindow() { }
	TagsWindow(NC::Menu<MPD::MutableSong> &&base)
	: NC::Menu<MPD::MutableSong>(std::move(base)) { }

	virtual SongIterator currentS() OVERRIDE;
	virtual ConstSongIterator currentS() const OVERRIDE;
	virtual SongIterator beginS() OVERRIDE;
	virtual ConstSongIterator beginS() const OVERRIDE;
	virtual SongIterator endS() OVERRIDE;
	virtual ConstSongIterator endS() const OVERRIDE;

	virtual std::vector<MPD::Song> getSelectedSongs() OVERRIDE;
};

struct TagEditor: Screen<NC::Window *>, HasActions, HasColumns, HasSongs, Searchable, Tabbable
{
	TagEditor();
	
	virtual void resize() OVERRIDE;
	virtual void switchTo() OVERRIDE;
	
	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::TagEditor; }
	
	virtual void refresh() OVERRIDE;
	virtual void update() OVERRIDE;
	
	virtual void mouseButtonPressed(MEVENT) OVERRIDE;
	
	virtual bool isLockable() OVERRIDE { return true; }
	virtual bool isMergable() OVERRIDE { return true; }
	
	// Searchable implementation
	virtual bool allowsSearching() OVERRIDE;
	virtual void setSearchConstraint(const std::string &constraint) OVERRIDE;
	virtual void clearConstraint() OVERRIDE;
	virtual bool find(SearchDirection direction, bool wrap, bool skip_current) OVERRIDE;

	// HasActions implementation
	virtual bool actionRunnable() OVERRIDE;
	virtual void runAction() OVERRIDE;

	// HasSongs implementation
	virtual bool itemAvailable() OVERRIDE;
	virtual bool addItemToPlaylist(bool play) OVERRIDE;
	virtual std::vector<MPD::Song> getSelectedSongs() OVERRIDE;
	
	// HasColumns implementation
	virtual bool previousColumnAvailable() OVERRIDE;
	virtual void previousColumn() OVERRIDE;
	
	virtual bool nextColumnAvailable() OVERRIDE;
	virtual void nextColumn() OVERRIDE;
	
	// private members
	bool enterDirectory();
	void LocateSong(const MPD::Song &s);
	const std::string &CurrentDir() { return itsBrowsedDir; }
	
	NC::Menu< std::pair<std::string, std::string> > *Dirs;
	NC::Menu<std::string> *TagTypes;
	TagsWindow *Tags;
	
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

	Regex::Filter<std::pair<std::string, std::string>> m_directories_search_predicate;
	Regex::Filter<MPD::MutableSong> m_songs_search_predicate;
};

extern TagEditor *myTagEditor;

#endif // HAVE_TAGLIB_H

#endif // NCMPCPP_TAG_EDITOR_H

