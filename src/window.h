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

#ifndef _WINDOW_H
#define _WINDOW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_PDCURSES
# define NCURSES_MOUSE_VERSION 1
#endif

#include "curses.h"

#include <list>
#include <stack>
#include <vector>
#include <string>

#if defined(__GNUC__) && __GNUC__ >= 3
# define GNUC_UNUSED __attribute__((unused))
# define GNUC_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
# define GNUC_UNUSED
# define GNUC_PRINTF(a, b)
#endif

#ifdef USE_PDCURSES
# undef KEY_BACKSPACE
# define KEY_BACKSPACE 8
#else
// NOTICE: redefine BUTTON2_PRESSED as it doesn't always work, I noticed
// that it sometimes returns 134217728 (2^27) instead of expected mask, so the
// modified define does it right but is rather experimental.
# undef BUTTON2_PRESSED
# define BUTTON2_PRESSED (NCURSES_MOUSE_MASK(2, NCURSES_BUTTON_PRESSED) | (1U << 27))
#endif // USE_PDCURSES

#ifdef _UTF8
# define my_char_t wchar_t
# define U(x) L##x
# define TO_STRING(x) ToString(x)
# define TO_WSTRING(x) ToWString(x)
#else
# define my_char_t char
# define U(x) x
# define TO_STRING(x) (x)
# define TO_WSTRING(x) (x)
#endif

// workaraund for win32
#ifdef WIN32
# define wcwidth(x) 1
#endif

/// Converts wide string to narrow string
/// @param ws wide string
/// @return narrow string
///
std::string ToString(const std::wstring &ws);

/// Converts narrow string to wide string
/// @param s narrow string
/// @return wide string
///
std::wstring ToWString(const std::string &s);

/// NCurses namespace provides set of easy-to-use
/// wrappers over original curses library
///
namespace NCurses
{
	/// Colors used by NCurses
	///
	enum Color { clDefault, clBlack, clRed, clGreen, clYellow, clBlue, clMagenta, clCyan, clWhite, clEnd };
	
	/// Format flags used by NCurses
	///
	enum Format {
		fmtNone = clEnd+1,
		fmtBold, fmtBoldEnd,
		fmtUnderline, fmtUnderlineEnd,
		fmtReverse, fmtReverseEnd,
		fmtAltCharset, fmtAltCharsetEnd
	};
	
	/// Available border colors for window
	///
	enum Border { brNone, brBlack, brRed, brGreen, brYellow, brBlue, brMagenta, brCyan, brWhite };
	
	/// This indicates how much the window has to be scrolled
	///
	enum Where { wUp, wDown, wPageUp, wPageDown, wHome, wEnd };
	
	/// Helper function that is invoked each time one will want
	/// to obtain string from Window::GetString() function
	/// @see Window::GetString()
	///
	typedef void (*GetStringHelper)(const std::wstring &);
	
	/// Initializes curses screen and sets some additional attributes
	/// @param window_title title of the window (has an effect only if pdcurses lib is used)
	/// @param enable_colors enables colors
	///
	void InitScreen(const char *window_title, bool enable_colors);
	
	/// Destroys the screen
	///
	void DestroyScreen();
	
	/// Struct used to set color of both foreground and background of window
	/// @see Window::operator<<()
	///
	struct Colors
	{
		Colors(Color one, Color two = clDefault) : fg(one), bg(two) { }
		Color fg;
		Color bg;
	};
	
	/// Struct used for going to given coordinates
	/// @see Window::operator<<()
	///
	struct XY
	{
		XY(int xx, int yy) : x(xx), y(yy) { }
		int x;
		int y;
	};
	
	/// Main class of NCurses namespace, used as base for other specialized windows
	///
	class Window
	{
		public:
			/// Constructs an empty window with given parameters
			/// @param startx X position of left upper corner of constructed window
			/// @param starty Y position of left upper corner of constructed window
			/// @param width width of constructed window
			/// @param height height of constructed window
			/// @param title title of constructed window
			/// @param color base color of constructed window
			/// @param border border of constructed window
			///
			Window(size_t startx, size_t starty, size_t width, size_t height,
			       const std::string &title, Color color, Border border);
			
			/// Copies thw window
			/// @param w copied window
			///
			Window(const Window &w);
			
			/// Destroys the window and frees memory
			///
			virtual ~Window();
			
			/// Allows for direct access to internal WINDOW pointer in case there
			/// is no wrapper for a function from curses library one may want to use
			/// @return internal WINDOW pointer
			///
			WINDOW *Raw() const { return itsWindow; }
			
			/// @return window's width
			///
			size_t GetWidth() const;
			
			/// @return window's height
			///
			size_t GetHeight() const;
			
			/// @return X position of left upper window's corner
			///
			size_t GetStartX() const;
			
			/// @return Y position of left upper window's corner
			///
			size_t GetStartY() const;
			
			/// @return window's title
			///
			const std::string &GetTitle() const;
			
			/// @return current window's color
			///
			Color GetColor() const;
			
			/// @return current window's border
			///
			Border GetBorder() const;
			
			/// @return current window's timeout
			///
			int GetTimeout() const;
			
			/// Reads the string from standard input. Note that this is much more complex
			/// function than getstr() from curses library. It allows for moving through
			/// letters with arrows, supports scrolling if string's length is bigger than
			/// given area, supports history of previous strings and each time it receives
			/// an input from the keyboard or the timeout is reached, it calls helper function
			/// (if it's set) that takes as an argument currently edited string.
			/// @param base base string that has to be edited
			/// @param length max length of string, unlimited by default
			/// @param width width of area that entry field can take. if it's reached, string
			/// will be scrolled. if value is 0, field will be from cursor position to the end
			/// of current line wide.
			/// @param encrypted if set to true, '*' characters will be displayed instead of
			/// actual text.
			/// @return edited string
			///
			/// @see SetGetStringHelper()
			/// @see SetTimeout()
			/// @see CreateHistory()
			///
			std::string GetString(const std::string &base, size_t length = -1,
					      size_t width = 0, bool encrypted = 0) const;
			
			/// Wrapper for above function that doesn't take base string (it will be empty).
			/// Taken parameters are the same as for above.
			///
			std::string GetString(size_t length = -1, size_t width = 0, bool encrypted = 0) const
			{
				return GetString("", length, width, encrypted);
			}
			
			/// Gets present position of cursor's coordinates
			/// @param x variable that will be set to current X position
			/// @param y variable that will be set to current Y position
			///
			void GetXY(int &x, int &y);
			
			/// Moves cursor to given coordinates
			/// @param x given X position
			/// @param y given Y position
			///
			void GotoXY(int x, int y);
			
			/// @return last X position that was set with GetXY(). Note that it most
			/// likely won't report correct position unless called after GetXY() and
			/// before any function that can print anything to window.
			/// @see GetXY()
			///
			int X() const;
			
			/// @return last Y position that was set with GetXY(). Since Y position
			/// doesn't change so frequently as X does, it can be called (with some
			/// caution) after something was printed to window and still may report
			/// correct position, but if you're not sure, obtain Y pos with GetXY()
			/// instead.
			/// @see GetXY()
			///
			int Y() const;
			
			/// Used to indicate whether given coordinates of main screen lies within
			/// window area or not and if they do, transform them into in-window coords.
			/// Otherwise function doesn't modify its arguments.
			/// @param x X position of main screen to be checked
			/// @param y Y position of main screen to be checked
			/// @return true if it transformed variables, false otherwise
			///
			bool hasCoords(int &x, int &y);
			
			/// Sets helper function used in GetString()
			/// @param helper pointer to function that matches GetStringHelper prototype
			/// @see GetString()
			///
			void SetGetStringHelper(GetStringHelper helper) { itsGetStringHelper = helper; }
			
			/// Sets window's base color
			/// @param fg foregound base color
			/// @param bg background base color
			///
			void SetBaseColor(Color fg, Color bg = clDefault);
			
			/// Sets window's border
			/// @param border new window's border
			///
			void SetBorder(Border border);
			
			/// Sets window's timeout
			/// @param timeout window's timeout
			///
			void SetTimeout(int timeout);
			
			/// Sets window's title
			/// @param new_title new title for window
			///
			void SetTitle(const std::string &new_title);
			
			/// Creates internal container that stores all previous
			/// strings that were edited using this window.
			///
			void CreateHistory();
			
			/// Deletes container with all previous history entries
			///
			void DeleteHistory();
			
			/// "Hides" the window by filling its area with given character
			/// @param ch character to fill the area
			/// @see Clear()
			///
			void Hide(char ch = 32) const;
			
			/// Refreshed whole window and its border
			/// @see Refresh()
			///
			void Display();
			
			/// Refreshes whole window, but not the border
			/// @see Display()
			///
			virtual void Refresh();
			
			/// Moves the window to new coordinates
			/// @param new_x new X position of left upper corner of window
			/// @param new_y new Y position of left upper corner of window
			///
			virtual void MoveTo(size_t new_x, size_t new_y);
			
			/// Resizes the window
			/// @param new_width new window's width
			/// @param new_height new window's height
			///
			virtual void Resize(size_t new_width, size_t new_height);
			
			/// Cleares the window
			///
			virtual void Clear();
			
			/// Adds given file descriptor to the list that will be polled in
			/// ReadKey() along with stdin and callback that will be invoked
			/// when there is data waiting for reading in it
			/// @param fd file descriptor
			/// @param callback callback
			///
			void AddFDCallback(int fd, void (*callback)());
			
			/// Clears list of file descriptors and their callbacks
			///
			void ClearFDCallbacksList();
			
			/// Checks if list of file descriptors is empty
			/// @return true if list is empty, false otherwise
			///
			bool FDCallbacksListEmpty() const;
			
			/// Reads key from standard input and writes it into read_key variable
			/// @param read_key variable for read key to be written into it
			///
			void ReadKey(int &read_key) const;
			
			/// Waits until user press a key.
			///
			void ReadKey() const;
			
			/// Scrolls the window by amount of lines given in its parameter
			/// @param where indicates how many lines it has to scroll
			///
			virtual void Scroll(Where where);
			
			/// Applies function of compatible prototype to internal WINDOW pointer
			/// The mostly used function in this case seem to be wclrtoeol(), which
			/// clears the window from current cursor position to the end of line.
			/// Note that delwin() also matches that prototype, but I wouldn't
			/// recommend anyone passing this pointer here ;)
			/// @param f pointer to function to call with internal WINDOW pointer
			/// @return reference to itself
			///
			Window &operator<<(int (*f)(WINDOW *));
			
			/// Applies foreground and background colors to window
			/// @param colors struct that holds new colors information
			/// @return reference to itself
			///
			Window &operator<<(Colors colors);
			
			/// Applies foregound color to window. Note that colors applied
			/// that way are stacked, i.e if you applied clRed, then clGreen
			/// and clEnd, current color would be clRed. If you want to discard
			/// all colors and fall back to base one, pass clDefault.
			/// @param color new color value
			/// @return reference to itself
			///
			Window &operator<<(Color color);
			
			/// Applies format flag to window. Note that these attributes are
			/// also stacked, so if you applied fmtBold twice, to get rid of
			/// it you have to pass fmtBoldEnd also twice.
			/// @param format format flag
			/// @return reference to itself
			///
			Window &operator<<(Format format);
			
			/// Moves current cursor position to given coordinates.
			/// @param coords struct that holds information about new coordinations
			/// @return reference to itself
			///
			Window &operator<<(XY coords);
			
			/// Prints string to window
			/// @param s const pointer to char array to be printed
			/// @return reference to itself
			///
			Window &operator<<(const char *s);
			
			/// Prints single character to window
			/// @param c character to be printed
			/// @return reference to itself
			///
			Window &operator<<(char c);
			
			/// Prints wide string to window
			/// @param ws const pointer to wchar_t array to be printed
			/// @return reference to itself
			///
			Window &operator<<(const wchar_t *ws);
			
			/// Prints single wide character to window
			/// @param wc wide character to be printed
			/// @return reference to itself
			///
			Window &operator<<(wchar_t wc);
			
			/// Prints int to window
			/// @param i integer value to be printed
			/// @return reference to itself
			///
			Window &operator<<(int i);
			
			/// Prints double to window
			/// @param d double value to be printed
			/// @return reference to itself
			///
			Window &operator<<(double d);
			
			/// Prints size_t to window
			/// @param s size value to be printed
			/// @return reference to itself
			///
			Window &operator<<(size_t s);
			
			/// Prints std::string to window
			/// @param s string to be printed
			/// @return reference to itself
			///
			Window &operator<<(const std::string &s);
			
			/// Prints std::wstring to window
			/// @param ws wide string to be printed
			/// @return reference to itself
			///
			Window &operator<<(const std::wstring &ws);
			
			/// Fallback for Length() for wide strings used if unicode support is disabled
			/// @param s string that real length has to be measured
			/// @return standard std::string::length() result since it's only fallback
			///
			static size_t Length(const std::string &s) { return s.length(); }
			
			/// Measures real length of wide string (required if e.g. asian characters are used)
			/// @param ws wide string that real length has to be measured
			/// @return real length of wide string
			///
			static size_t Length(const std::wstring &ws);
			
		protected:
			/// Sets colors of window (interal use only)
			/// @param fg foregound color
			/// @param bg background color
			///
			void SetColor(Color fg, Color bg = clDefault);
			
			/// Refreshes window's border
			///
			void ShowBorder() const;
			
			/// Changes dimensions of window, called from Resize()
			/// @param width new window's width
			/// @param height new window's height
			/// @see Resize()
			///
			void AdjustDimensions(size_t width, size_t height);
			
			/// Deletes old window and creates new. It's called by Resize(),
			/// SetBorder() or SetTitle() since internally windows are
			/// handled as curses pads and change in size requires to delete
			/// them and create again, there is no way to change size of pad.
			/// @see SetBorder()
			/// @see SetTitle()
			/// @see Resize()
			///
			virtual void Recreate(size_t width, size_t height);
			
			/// internal WINDOW pointers
			WINDOW *itsWindow;
			WINDOW *itsWinBorder;
			
			/// start points and dimensions
			size_t itsStartX;
			size_t itsStartY;
			size_t itsWidth;
			size_t itsHeight;
			
			/// window timeout
			int itsWindowTimeout;
			
			/// current colors
			Color itsColor;
			Color itsBgColor;
			
			/// base colors
			Color itsBaseColor;
			Color itsBaseBgColor;
			
			/// current border
			Border itsBorder;
			
		private:
			/// Sets state of bold attribute (internal use only)
			/// @param bold_state state of bold attribute
			///
			void Bold(bool bold_state) const;
			
			/// Sets state of underline attribute (internal use only)
			/// @param underline_state state of underline attribute
			///
			void Underline(bool underline_state) const;
			
			/// Sets state of reverse attribute (internal use only)
			/// @param reverse_state state of reverse attribute
			///
			void Reverse(bool reverse_state) const;
			
			/// Sets state of altcharset attribute (internal use only)
			/// @param altcharset_state state of altcharset attribute
			///
			void AltCharset(bool altcharset_state) const;
			
			/// pointer to helper function used by GetString()
			/// @see GetString()
			///
			GetStringHelper itsGetStringHelper;
			
			/// temporary coordinates
			/// @see X()
			/// @see Y()
			///
			int itsX;
			int itsY;
			
			/// window's title
			std::string itsTitle;
			
			/// stack of colors
			std::stack<Colors> itsColors;
			
			/// containter used for additional file descriptors that have
			/// to be polled in ReadKey() and correspondent callbacks that
			/// are invoked if there is data available in them
			typedef std::vector< std::pair<int, void (*)()> > FDCallbacks;
			FDCallbacks itsFDs;
			
			/// pointer to container used as history
			std::list<std::wstring> *itsHistory;
			
			/// counters for format flags
			int itsBoldCounter;
			int itsUnderlineCounter;
			int itsReverseCounter;
			int itsAltCharsetCounter;
	};
}

#endif
