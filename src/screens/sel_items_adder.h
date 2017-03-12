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

#ifndef NCMPCPP_SEL_ITEMS_ADDER_H
#define NCMPCPP_SEL_ITEMS_ADDER_H

#include "runnable_item.h"
#include "interfaces.h"
#include "regex_filter.h"
#include "screens/screen.h"
#include "song.h"

struct SelectedItemsAdder: Screen<NC::Menu<RunnableItem<std::string, void()>> *>, HasActions, Searchable, Tabbable
{
	typedef SelectedItemsAdder Self;
	typedef typename std::remove_pointer<WindowType>::type Component;
	typedef Component::Item::Type Entry;
	
	SelectedItemsAdder();
	
	virtual void switchTo() override;
	virtual void resize() override;
	virtual void refresh() override;
	
	virtual std::wstring title() override;
	virtual ScreenType type() override { return ScreenType::SelectedItemsAdder; }
	
	virtual void update() override { }
	
	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return false; }

	// HasActions implementation
	virtual bool actionRunnable() override;
	virtual void runAction() override;

	// Searchable implementation
	virtual bool allowsSearching() override;
	virtual const std::string &searchConstraint() override;
	virtual void setSearchConstraint(const std::string &constraint) override;
	virtual void clearSearchConstraint() override;
	virtual bool search(SearchDirection direction, bool wrap, bool skip_current) override;

private:
	void populatePlaylistSelector(BaseScreen *screen);
	
	void addToCurrentPlaylist();
	void addToNewPlaylist() const;
	void addToExistingPlaylist(const std::string &playlist) const;
	void addAtTheEndOfPlaylist() const;
	void addAtTheBeginningOfPlaylist() const;
	void addAfterCurrentSong() const;
	void addAfterCurrentAlbum() const;
	void addAfterHighlightedSong() const;
	void cancel();
	void exitSuccessfully(bool success) const;
	
	void setDimensions();
	
	size_t m_playlist_selector_width;
	size_t m_playlist_selector_height;
	
	size_t m_position_selector_width;
	size_t m_position_selector_height;
	
	Component m_playlist_selector;
	Component m_position_selector;
	
	std::vector<MPD::Song> m_selected_items;

	Regex::ItemFilter<Entry> m_search_predicate;
};

extern SelectedItemsAdder *mySelectedItemsAdder;

#endif // NCMPCPP_SEL_ITEMS_ADDER_H

