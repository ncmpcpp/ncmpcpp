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

#include "menu.h"
#include "scrollpad.h"

void GenericMouseButtonPressed(NC::Window *w, MEVENT me);
void ScrollpadMouseButtonPressed(NC::Scrollpad *w, MEVENT me);

/// An interface for various instantiations of Screen template class. Since C++ doesn't like
/// comparison of two different instantiations of the same template class we need the most
/// basic class to be non-template to allow it.
class BasicScreen
{
	public:
		/// Initializes all variables to zero
		BasicScreen() : hasToBeResized(0), isInitialized(0) { }
		
		virtual ~BasicScreen() { }
		
		/// @see Screen::ActiveWindow()
		virtual NC::Window *ActiveWindow() = 0;
		
		/// @see Screen::Refresh()
		virtual void Refresh() = 0;
		
		/// @see Screen::RefreshWindow()
		virtual void RefreshWindow() = 0;
		
		/// @see Screen::Scroll()
		virtual void Scroll(NC::Where where) = 0;
		
		/// Method used for switching to screen
		virtual void SwitchTo() = 0;
		
		/// Method that should resize screen
		/// if requested by hasToBeResized
		virtual void Resize() = 0;
		
		/// @return title of the screen
		virtual std::wstring Title() = 0;
		
		/// If the screen contantly has to update itself
		/// somehow, it should be called by this function.
		virtual void Update() = 0;
		
		/// Invoked after Enter was pressed
		virtual void EnterPressed() = 0;
		
		/// Invoked after Space was pressed
		virtual void SpacePressed() = 0;
		
		/// @see Screen::MouseButtonPressed()
		virtual void MouseButtonPressed(MEVENT me) = 0;
		
		/// When this is overwritten with a function returning true, the
		/// screen will be used in tab switching.
		virtual bool isTabbable() = 0;
		
		/// @return true if screen is mergable, ie. can be "proper" subwindow
		/// if one of the screens is locked. Screens that somehow resemble popups
		/// will want to return false here.
		virtual bool isMergable() = 0;
		
		/// Locks current screen.
		/// @return true if lock was successful, false otherwise. Note that
		/// if there is already locked screen, it'll not overwrite it.
		bool Lock();
		
		/// Should be set to true each time screen needs resize
		bool hasToBeResized;
		
		/// Unlocks a screen, ie. hides merged window (if there is one set).
		static void Unlock();
		
	protected:
		/// Since screens initialization is lazy, we don't want to do
		/// this in the constructor. This function should be invoked
		/// only once and after that isInitialized flag has to be set
		/// to true to somehow avoid next attempt of initialization.
		virtual void Init() = 0;
		
		/// @return true if screen can be locked. Note that returning
		/// false here doesn't neccesarily mean that screen is also not
		/// mergable (eg. lyrics screen is not lockable since that wouldn't
		/// make much sense, but it's perfectly fine to merge it).
		virtual bool isLockable() = 0;
		
		/// Gets X offset and width of current screen to be used eg. in Resize() function.
		/// @param adjust_locked_screen indicates whether this function should
		/// automatically adjust locked screen's dimensions (if there is one set)
		/// if current screen is going to be subwindow.
		void GetWindowResizeParams(size_t &x_offset, size_t &width, bool adjust_locked_screen = true);
		
		/// Flag that inditates whether the screen is initialized or not
		bool isInitialized;
};

void ApplyToVisibleWindows(void (BasicScreen::*f)());
void UpdateInactiveScreen(BasicScreen *screen_to_be_set);
bool isVisible(BasicScreen *screen);

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
		virtual NC::Window *ActiveWindow() OVERRIDE {
			return w;
		}
		
		/// Refreshes whole screen
		virtual void Refresh() OVERRIDE {
			w->display();
		}
		
		/// Refreshes active window of the screen
		virtual void RefreshWindow() OVERRIDE {
			w->display();
		}
		
		/// Scrolls the screen by given amount of lines and
		/// if fancy scrolling feature is disabled, enters the
		/// loop that holds main loop until user releases the key
		/// @param where indicates where one wants to scroll
		virtual void Scroll(NC::Where where) OVERRIDE {
			w->scroll(where);
		}
		
		/// Invoked after there was one of mouse buttons pressed
		/// @param me struct that contains coords of where the click
		/// had its place and button actions
		virtual void MouseButtonPressed(MEVENT me) OVERRIDE {
			GenericMouseButtonPressed(w, me);
		}
		
		/// @return pointer to currently active window
		WindowType *Main() {
			return w;
		}
		
	protected:
		/// Template parameter that should indicate the main type
		/// of window used by the screen. What is more, it should
		/// always be assigned to the currently active window (if
		/// acreen contains more that one)
		WindowType *w;
};

/// Specialization for Screen<Scrollpad>::MouseButtonPressed, that should
/// not scroll whole page, but rather a few lines (the number of them is
/// defined in the config)
template <> inline void Screen<NC::Scrollpad>::MouseButtonPressed(MEVENT me)
{
	ScrollpadMouseButtonPressed(w, me);
}

#endif

