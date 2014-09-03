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

#ifndef NCMPCPP_SCREEN_H
#define NCMPCPP_SCREEN_H

#include "menu.h"
#include "scrollpad.h"
#include "screen_type.h"

void drawSeparator(int x);
void genericMouseButtonPressed(NC::Window &w, MEVENT me);
void scrollpadMouseButtonPressed(NC::Scrollpad &w, MEVENT me);

/// An interface for various instantiations of Screen template class. Since C++ doesn't like
/// comparison of two different instantiations of the same template class we need the most
/// basic class to be non-template to allow it.
struct BaseScreen
{
	BaseScreen() : hasToBeResized(false) { }
	virtual ~BaseScreen() { }
	
	/// @see Screen::isActiveWindow()
	virtual bool isActiveWindow(const NC::Window &w_) = 0;
	
	/// @see Screen::activeWindow()
	virtual void *activeWindow() = 0;
	
	/// @see Screen::refresh()
	virtual void refresh() = 0;
	
	/// @see Screen::refreshWindow()
	virtual void refreshWindow() = 0;
	
	/// @see Screen::scroll()
	virtual void scroll(NC::Scroll where) = 0;
	
	/// Method used for switching to screen
	virtual void switchTo() = 0;
	
	/// Method that should resize screen
	/// if requested by hasToBeResized
	virtual void resize() = 0;

	/// @return ncurses timeout parameter for the screen
	virtual int windowTimeout() = 0;
	
	/// @return title of the screen
	virtual std::wstring title() = 0;
	
	/// @return type of the screen
	virtual ScreenType type() = 0;
	
	/// If the screen contantly has to update itself
	/// somehow, it should be called by this function.
	virtual void update() = 0;
	
	/// Invoked after Enter was pressed
	virtual void enterPressed() = 0;
	
	/// Invoked after Space was pressed
	virtual void spacePressed() = 0;
	
	/// @see Screen::mouseButtonPressed()
	virtual void mouseButtonPressed(MEVENT me) = 0;
	
	/// @return true if screen is mergable, ie. can be "proper" subwindow
	/// if one of the screens is locked. Screens that somehow resemble popups
	/// will want to return false here.
	virtual bool isMergable() = 0;
	
	/// Locks current screen.
	/// @return true if lock was successful, false otherwise. Note that
	/// if there is already locked screen, it'll not overwrite it.
	bool lock();
	
	/// Should be set to true each time screen needs resize
	bool hasToBeResized;
	
	/// Unlocks a screen, ie. hides merged window (if there is one set).
	static void unlock();
	
protected:
	/// @return true if screen can be locked. Note that returning
	/// false here doesn't neccesarily mean that screen is also not
	/// mergable (eg. lyrics screen is not lockable since that wouldn't
	/// make much sense, but it's perfectly fine to merge it).
	virtual bool isLockable() = 0;
	
	/// Gets X offset and width of current screen to be used eg. in resize() function.
	/// @param adjust_locked_screen indicates whether this function should
	/// automatically adjust locked screen's dimensions (if there is one set)
	/// if current screen is going to be subwindow.
	void getWindowResizeParams(size_t &x_offset, size_t &width, bool adjust_locked_screen = true);
};

void applyToVisibleWindows(std::function<void(BaseScreen *)> f);
void updateInactiveScreen(BaseScreen *screen_to_be_set);
bool isVisible(BaseScreen *screen);

/// Class that all screens should derive from. It provides basic interface
/// for the screen to be working properly and assumes that we didn't forget
/// about anything vital.
///
template <typename WindowT> struct Screen : public BaseScreen
{
	typedef WindowT WindowType;
	typedef typename std::add_lvalue_reference<WindowType>::type WindowReference;
	
private:
	template <bool IsPointer, typename Result> struct getObject { };
	template <typename Result> struct getObject<true, Result> {
		static Result apply(WindowType w) { return *w; }
	};
	template <typename Result> struct getObject<false, Result> {
		static Result apply(WindowReference w) { return w; }
	};
	
	typedef getObject<
		std::is_pointer<WindowT>::value,
		typename std::add_lvalue_reference<
			typename std::remove_pointer<WindowT>::type
		>::type
	> Accessor;
	
public:
	Screen() { }
	Screen(WindowT w_) : w(w_) { }
	
	virtual ~Screen() { }
	
	virtual bool isActiveWindow(const NC::Window &w_) OVERRIDE {
		return &Accessor::apply(w) == &w_;
	}
	
	/// Since some screens contain more that one window
	/// it's useful to determine the one that is being
	/// active
	/// @return address to window object cast to void *
	virtual void *activeWindow() OVERRIDE {
		return &Accessor::apply(w);
	}
	
	/// Refreshes whole screen
	virtual void refresh() OVERRIDE {
		Accessor::apply(w).display();
	}
	
	/// Refreshes active window of the screen
	virtual void refreshWindow() OVERRIDE {
		Accessor::apply(w).display();
	}
	
	/// Scrolls the screen by given amount of lines and
	/// if fancy scrolling feature is disabled, enters the
	/// loop that holds main loop until user releases the key
	/// @param where indicates where one wants to scroll
	virtual void scroll(NC::Scroll where) OVERRIDE {
		Accessor::apply(w).scroll(where);
	}
	
	/// @return timeout parameter used for the screen (in ms)
	/// @default 500
	virtual int windowTimeout() OVERRIDE {
		return 500;
	}

	/// Invoked after there was one of mouse buttons pressed
	/// @param me struct that contains coords of where the click
	/// had its place and button actions
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE {
		genericMouseButtonPressed(Accessor::apply(w), me);
	}
	
	/// @return currently active window
	WindowReference main() {
		return w;
	}
	
protected:
	/// Template parameter that should indicate the main type
	/// of window used by the screen. What is more, it should
	/// always be assigned to the currently active window (if
	/// acreen contains more that one)
	WindowT w;
};

/// Specialization for Screen<Scrollpad>::mouseButtonPressed that should
/// not scroll whole page, but rather a few lines (the number of them is
/// defined in the config)
template <> inline void Screen<NC::Scrollpad>::mouseButtonPressed(MEVENT me) {
	scrollpadMouseButtonPressed(w, me);
}

#endif // NCMPCPP_SCREEN_H

