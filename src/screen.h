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

#ifndef _SCREEN_H
#define _SCREEN_H

#include "window.h"
#include "menu.h"
#include "mpdpp.h"
#include "helpers.h"
#include "settings.h"
#include "status.h"

void ApplyToVisibleWindows(void (BasicScreen::*f)());
void UpdateInactiveScreen(BasicScreen *);
bool isVisible(BasicScreen *);

/// An interface for various instantiations of Screen template class. Since C++ doesn't like
/// comparison of two different instantiations of the same template class we need the most
/// basic class to be non-template to allow it.
///
class BasicScreen
{
	public:
		/// Initializes all variables to zero
		///
		BasicScreen() : hasToBeResized(0), isInitialized(0) { }
		
		virtual ~BasicScreen() { }
		
		/// @see Screen::ActiveWindow()
		///
		virtual Window *ActiveWindow() = 0;
		
		/// Method used for switching to screen
		///
		virtual void SwitchTo() = 0;
		
		/// Method that should resize screen
		/// if requested by hasToBeResized
		///
		virtual void Resize() = 0;
		
		/// @return title of the screen
		///
		virtual std::basic_string<my_char_t> Title() = 0;
		
		/// If the screen contantly has to update itself
		/// somehow, it should be called by this function.
		///
		virtual void Update() { }
		
		/// @see Screen::Refresh()
		///
		virtual void Refresh() = 0;
		
		/// @see Screen::RefreshWindow()
		///
		virtual void RefreshWindow() = 0;
		
		/// see Screen::ReadKey()
		///
		virtual void ReadKey(int &key) = 0;
		
		/// @see Screen::Scroll()
		///
		virtual void Scroll(Where where, const int key[2] = 0) = 0;
		
		/// Invoked after Enter was pressed
		///
		virtual void EnterPressed() = 0;
		
		/// Invoked after Space was pressed
		///
		virtual void SpacePressed() = 0;
		
		/// @see Screen::MouseButtonPressed()
		///
		virtual void MouseButtonPressed(MEVENT) { }
		
		/// @return pointer to currently selected song in the screen
		/// (if screen provides one) or null pointer otherwise.
		///
		virtual MPD::Song *CurrentSong() { return 0; }
		
		/// @return pointer to song at given position in the screen
		/// (if screen is provides one) or null pointer otherwise.
		///
		virtual MPD::Song *GetSong(GNUC_UNUSED size_t pos) { return 0; }
		
		/// @return true if the screen allows selecting items, false otherwise
		///
		virtual bool allowsSelection() = 0;
		
		/// Reverses selection. Does nothing by default since pure
		/// virtual allowsSelection() should remind of this function
		/// to be defined
		///
		virtual void ReverseSelection() { }
		
		/// Gets selected songs' positions from the screen
		/// @param v vector to be filled with positions
		///
		virtual void GetSelectedSongs(GNUC_UNUSED MPD::SongList &v) { }
		
		/// Applies a filter to the screen
		virtual void ApplyFilter(GNUC_UNUSED const std::string &filter) { }
		
		/// @return pointer to instantiation of Menu template class
		/// cast to List if available or null pointer otherwise
		///
		virtual List *GetList() = 0;

		/// When this is overwritten with a function returning true, the
		/// screen will be used in tab switching.
		///
		virtual bool isTabbable() { return false; }
		
		/// @return true if screen is mergable, ie. can be "proper" subwindow
		/// if one of the screens is locked. Screens that somehow resemble popups
		/// will want to return false here.
		virtual bool isMergable() = 0;
		
		/// Locks current screen.
		/// @return true if lock was successful, false otherwise. Note that
		/// if there is already locked screen, it'll not overwrite it.
		bool Lock();
		
		/// Should be set to true each time screen needs resize
		///
		bool hasToBeResized;
		
		/// Unlocks a screen, ie. hides merged window (if there is one set).
		static void Unlock();
		
	protected:
		/// Since screens initialization is lazy, we don't want to do
		/// this in the constructor. This function should be invoked
		/// only once and after that isInitialized flag has to be set
		/// to true to somehow avoid next attempt of initialization.
		///
		virtual void Init() = 0;
		
		/// @return true if screen can be locked. Note that returning
		/// false here doesn't neccesarily mean that screen is also not
		/// mergable (eg. lyrics screen is not lockable since that wouldn't
		/// make much sense, but it's perfectly fine to merge it).
		virtual bool isLockable() = 0;
		
		/// Gets X offset and width of current screen to be used eg. in Resize() function.
		/// @param adjust_locked_screen indicates whether this function should
		/// automatically adjust locked screen's dimensions (is there is one set)
		/// if current screen is going to be subwindow.
		void GetWindowResizeParams(size_t &x_offset, size_t &width, bool adjust_locked_screen = true);
		
		/// Flag that inditates whether the screen is initialized or not
		///
		bool isInitialized;
};

/// Class that all screens should derive from. It provides basic interface
/// for the screen to be working properly and assumes that we didn't forget
/// about anything vital.
///
template <typename WindowType> class Screen : public BasicScreen
{
	public:
		Screen() : w(0) { }
		virtual ~Screen() { }
		
		/// Since some screens contain more that one window
		/// it's useful to determine the one that is being
		/// active
		/// @return address to window object cast to void *
		///
		virtual Window *ActiveWindow();
		
		/// @return pointer to currently active window
		///
		WindowType *Main();
		
		/// Refreshes whole screen
		///
		virtual void Refresh();
		
		/// Refreshes active window of the screen
		///
		virtual void RefreshWindow();
		
		/// Reads a key from the screen
		///
		virtual void ReadKey(int &key);
		
		/// Scrolls the screen by given amount of lines and
		/// if fancy scrolling feature is disabled, enters the
		/// loop that holds main loop until user releases the key
		/// @param where indicates where one wants to scroll
		/// @param key needed if fancy scrolling is disabled to
		/// define the conditional for while loop
		///
		virtual void Scroll(Where where, const int key[2] = 0);
		
		/// Invoked after there was one of mouse buttons pressed
		/// @param me struct that contains coords of where the click
		/// had its place and button actions
		///
		virtual void MouseButtonPressed(MEVENT me);
		
	protected:
		/// Template parameter that should indicate the main type
		/// of window used by the screen. What is more, it should
		/// always be assigned to the currently active window (if
		/// acreen contains more that one)
		///
		WindowType *w;
};

template <typename WindowType> Window *Screen<WindowType>::ActiveWindow()
{
	return w;
}

template <typename WindowType> WindowType *Screen<WindowType>::Main()
{
	return w;
}

template <typename WindowType> void Screen<WindowType>::Refresh()
{
	w->Display();
}

template <typename WindowType> void Screen<WindowType>::RefreshWindow()
{
	w->Display();
}

template <typename WindowType> void Screen<WindowType>::ReadKey(int &key)
{
	w->ReadKey(key);
}

template <typename WindowType> void Screen<WindowType>::Scroll(Where where, const int key[2])
{
	if (!Config.fancy_scrolling && key)
	{
		int in = key[0];
		w->SetTimeout(50);
		while (Keypressed(in, key))
		{
			TraceMpdStatus();
			w->Scroll(where);
			w->Refresh();
			ReadKey(in);
		}
		w->SetTimeout(ncmpcpp_window_timeout);
	}
	else
		w->Scroll(where);
}

template <typename WindowType> void Screen<WindowType>::MouseButtonPressed(MEVENT me)
{
	if (me.bstate & BUTTON2_PRESSED)
	{
		if (Config.mouse_list_scroll_whole_page)
			Scroll(wPageDown);
		else
			for (size_t i = 0; i < Config.lines_scrolled; ++i)
				Scroll(wDown);
	}
	else if (me.bstate & BUTTON4_PRESSED)
	{
		if (Config.mouse_list_scroll_whole_page)
			Scroll(wPageUp);
		else
			for (size_t i = 0; i < Config.lines_scrolled; ++i)
				Scroll(wUp);
	}
}

/// Specialization for Screen<Scrollpad>::MouseButtonPressed, that should
/// not scroll whole page, but rather a few lines (the number of them is
/// defined in the config)
///
template <> inline void Screen<Scrollpad>::MouseButtonPressed(MEVENT me)
{
	if (me.bstate & BUTTON2_PRESSED)
	{
		for (size_t i = 0; i < Config.lines_scrolled; ++i)
			Scroll(wDown);
	}
	else if (me.bstate & BUTTON4_PRESSED)
	{
		for (size_t i = 0; i < Config.lines_scrolled; ++i)
			Scroll(wUp);
	}
}

#endif

