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

#ifndef NCMPCPP_PROXY_SONG_LIST_H
#define NCMPCPP_PROXY_SONG_LIST_H

#include "menu.h"
#include "song.h"

/// This fancy class provides a way to use NC::Menu<T> template instantiations
/// with different Ts in a uniform manner. Note that since it provides methods
/// such as getSong or currentSong, it's biased towards menus that somehow
/// contain songs.
///
/// Dependent types are T and F. T is type of items stored inside Menu, whereas
/// F is a function that takes Menu<T>::Item and converts it into MPD::Song pointer.
/// For Menu<MPD::Song> this is just taking address of given MPD::Song, but e.g.
/// Menu<MPD::Item> returns not null pointer only if requested position actually
/// contains a song and not directory or playlist.
class ProxySongList
{
	struct Interface
	{
		virtual ~Interface() { }
		
		virtual bool empty() = 0;
		virtual size_t size() = 0;
		virtual size_t choice() = 0;
		virtual void highlight(size_t pos) = 0;
		
		virtual bool isSelected(size_t pos) = 0;
		virtual void setSelected(size_t pos, bool selected) = 0;
		
		virtual bool isBold(size_t pos) = 0;
		virtual void setBold(size_t pos, bool bold) = 0;
		
		virtual MPD::Song *getSong(size_t pos) = 0;
		virtual MPD::Song *currentSong() = 0;
	};
	
	template <typename T, typename F> struct Impl : Interface
	{
		typedef typename NC::Menu<T> Menu;
		
		Impl(Menu &menu, F f) : m_menu(menu), m_song_getter(f) { }
		virtual ~Impl() { }
		
		virtual bool empty() { return m_menu.empty(); }
		virtual size_t size() { return m_menu.size(); }
		virtual size_t choice() { return m_menu.choice(); }
		virtual void highlight(size_t pos) { m_menu.highlight(pos); }
		
		virtual bool isSelected(size_t pos) {
			assert(pos < m_menu.size());
			return m_menu[pos].isSelected();
		}
		virtual void setSelected(size_t pos, bool selected) {
			assert(pos < m_menu.size());
			m_menu[pos].setSelected(selected);
		}
		
		virtual bool isBold(size_t pos) {
			assert(pos < m_menu.size());
			return m_menu[pos].isBold();
		}
		virtual void setBold(size_t pos, bool bold) {
			assert(pos < m_menu.size());
			m_menu[pos].setBold(bold);
		}
		
		virtual MPD::Song *getSong(size_t pos) {
			assert(pos < m_menu.size());
			return m_song_getter(m_menu[pos]);
		}
		virtual MPD::Song *currentSong() {
			if (!m_menu.empty())
				return getSong(m_menu.choice());
			else
				return 0;
		}
		
	private:
		Menu &m_menu;
		F m_song_getter;
	};
	
	std::shared_ptr<Interface> m_impl;
	
public:
	ProxySongList() { }
	
	template <typename T, typename F>
	ProxySongList(typename NC::Menu<T> &menu, F f) : m_impl(new Impl<T, F>(menu, f)) { }
	
	bool empty() const { return m_impl->empty(); }
	size_t size() const { return m_impl->size(); }
	size_t choice() const { return m_impl->choice(); }
	void highlight(size_t pos) const { m_impl->highlight(pos); }
	
	bool isSelected(size_t pos) const { return m_impl->isSelected(pos); }
	void setSelected(size_t pos, bool selected) const { m_impl->setSelected(pos, selected); }
	
	bool isBold(size_t pos) const { return m_impl->isBold(pos); }
	void setBold(size_t pos, bool bold) const{ m_impl->setBold(pos, bold); }
	
	MPD::Song *getSong(size_t pos) const { return m_impl->getSong(pos); }
	MPD::Song *currentSong() const { return m_impl->currentSong(); }
	
	/// @return true if there is no underlying menu object, false otherwise
	operator bool() const { return m_impl.get() != 0; }
};

#endif // NCMPCPP_PROXY_SONG_LIST_H
