/***************************************************************************
 *   Copyright (C) 2008-2017 by Andrzej Rybczak                            *
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
#include "screens/screen.h"
#include "song_list.h"

struct TagsWindow: NC::Menu<MPD::MutableSong>, SongList
{
	TagsWindow() { }
	TagsWindow(NC::Menu<MPD::MutableSong> &&base)
	: NC::Menu<MPD::MutableSong>(std::move(base)) { }

	virtual SongIterator currentS() override;
	virtual ConstSongIterator currentS() const override;
	virtual SongIterator beginS() override;
	virtual ConstSongIterator beginS() const override;
	virtual SongIterator endS() override;
	virtual ConstSongIterator endS() const override;

	virtual std::vector<MPD::Song> getSelectedSongs() override;
};

struct TagEditor: Screen<NC::Window *>, HasActions, HasColumns, HasSongs, Searchable, Tabbable
{
	TagEditor();
	
	virtual void resize() override;
	virtual void switchTo() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::TagEditor; }
	
	virtual void refresh() override;
	virtual void update() override;
	
	virtual void mouseButtonPressed(MEVENT) override;
	
	virtual bool isLockable() override { return true; }
	virtual bool isMergable() override { return true; }
	
	// Searchable implementation
	virtual bool allowsSearching() override;
	virtual const std::string &searchConstraint() override;
	virtual void setSearchConstraint(const std::string &constraint) override;
	virtual void clearSearchConstraint() override;
	virtual bool search(SearchDirection direction, bool wrap, bool skip_current) override;

	// HasActions implementation
	virtual bool actionRunnable() override;
	virtual void runAction() override;

	// HasSongs implementation
	virtual bool itemAvailable() override;
	virtual bool addItemToPlaylist(bool play) override;
	virtual std::vector<MPD::Song> getSelectedSongs() override;
	
	// HasColumns implementation
	virtual bool previousColumnAvailable() override;
	virtual void previousColumn() override;
	
	virtual bool nextColumnAvailable() override;
	virtual void nextColumn() override;
	
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

