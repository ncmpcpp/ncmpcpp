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

#ifndef NCMPCPP_WINDOW_H
#define NCMPCPP_WINDOW_H

#include "config.h"

#ifdef USE_PDCURSES
# define NCURSES_MOUSE_VERSION 1
#endif

#include "curses.h"
#include "gcc.h"

#include <functional>
#include <list>
#include <stack>
#include <vector>
#include <string>
#include <queue>

// define some Ctrl-? keys
#define KEY_CTRL_A 1
#define KEY_CTRL_B 2
#define KEY_CTRL_C 3
#define KEY_CTRL_D 4
#define KEY_CTRL_E 5
#define KEY_CTRL_F 6
#define KEY_CTRL_G 7
#define KEY_CTRL_H 8
#define KEY_CTRL_I 9
#define KEY_CTRL_J 10
#define KEY_CTRL_K 11
#define KEY_CTRL_L 12
#define KEY_CTRL_M 13
#define KEY_CTRL_N 14
#define KEY_CTRL_O 15
#define KEY_CTRL_P 16
#define KEY_CTRL_Q 17
#define KEY_CTRL_R 18
#define KEY_CTRL_S 19
#define KEY_CTRL_T 20
#define KEY_CTRL_U 21
#define KEY_CTRL_V 22
#define KEY_CTRL_W 23
#define KEY_CTRL_X 24
#define KEY_CTRL_Y 25
#define KEY_CTRL_Z 26

// define F? keys
#define KEY_F1 265
#define KEY_F2 266
#define KEY_F3 267
#define KEY_F4 268
#define KEY_F5 269
#define KEY_F6 270
#define KEY_F7 271
#define KEY_F8 272
#define KEY_F9 273
#define KEY_F10 274
#define KEY_F11 275
#define KEY_F12 276

// other handy keys
#define KEY_ESCAPE 27
#define KEY_SHIFT_TAB 353
#define KEY_SPACE 32
#define KEY_TAB 9

// define alternative KEY_BACKSPACE (used in some terminal emulators)
#define KEY_BACKSPACE_2 127

// KEY_ENTER is 343, which doesn't make any sense. This makes it useful.
#undef KEY_ENTER
#define KEY_ENTER 13

// undefine scroll macro as it collides with Window::scroll
#undef scroll

#ifndef USE_PDCURSES
// NOTICE: redefine BUTTON2_PRESSED as it doesn't always work, I noticed
// that it sometimes returns 134217728 (2^27) instead of expected mask, so the
// modified define does it right but is rather experimental.
# undef BUTTON2_PRESSED
# define BUTTON2_PRESSED (NCURSES_MOUSE_MASK(2, NCURSES_BUTTON_PRESSED) | (1U << 27))
#endif // USE_PDCURSES

// workaraund for win32
#ifdef WIN32
# define wcwidth(x) int(!iscntrl(x))
#endif

/// NC namespace provides set of easy-to-use
/// wrappers over original curses library
namespace NC {//

/// Colors used by NCurses
enum class Color { Default, Black, Red, Green, Yellow, Blue, Magenta, Cyan, White, End };

std::ostream &operator<<(std::ostream &os, Color c);
std::istream &operator>>(std::istream &is, Color &c);

/// Format flags used by NCurses
enum class Format {
	None,
	Bold, NoBold,
	Underline, NoUnderline,
	Reverse, NoReverse,
	AltCharset, NoAltCharset
};

std::ostream &operator<<(std::ostream &os, Format f);

/// Available border colors for window
enum class Border { None, Black, Red, Green, Yellow, Blue, Magenta, Cyan, White };

std::ostream &operator<<(std::ostream &os, Border b);
std::istream &operator>>(std::istream &is, Border &b);

/// This indicates how much the window has to be scrolled
enum class Scroll { Up, Down, PageUp, PageDown, Home, End };

std::ostream &operator<<(std::ostream &os, Scroll s);

/// Helper function that is invoked each time one will want
/// to obtain string from Window::getString() function
/// @see Window::getString()
typedef std::function<bool(const char *)> GetStringHelper;

/// Initializes curses screen and sets some additional attributes
/// @param window_title title of the window (has an effect only if pdcurses lib is used)
/// @param enable_colors enables colors
void initScreen(const char *window_title, bool enable_colors);

/// Destroys the screen
void destroyScreen();

/// Struct used to set color of both foreground and background of window
/// @see Window::operator<<()
struct Colors
{
	Colors(Color one, Color two = Color::Default) : fg(one), bg(two) { }
	Color fg;
	Color bg;
};

/// Struct used for going to given coordinates
/// @see Window::operator<<()
struct XY
{
	XY(int xx, int yy) : x(xx), y(yy) { }
	int x;
	int y;
};

/// Main class of NCurses namespace, used as base for other specialized windows
struct Window
{
	Window() : m_window(0), m_border_window(0) { }
	
	/// Constructs an empty window with given parameters
	/// @param startx X position of left upper corner of constructed window
	/// @param starty Y position of left upper corner of constructed window
	/// @param width width of constructed window
	/// @param height height of constructed window
	/// @param title title of constructed window
	/// @param color base color of constructed window
	/// @param border border of constructed window
	Window(size_t startx, size_t starty, size_t width, size_t height,
			const std::string &title, Color color, Border border);
	
	Window(const Window &rhs);
	Window(Window &&rhs);
	Window &operator=(Window w);
	
	virtual ~Window();
	
	/// Allows for direct access to internal WINDOW pointer in case there
	/// is no wrapper for a function from curses library one may want to use
	/// @return internal WINDOW pointer
	WINDOW *raw() const { return m_window; }
	
	/// @return window's width
	size_t getWidth() const;
	
	/// @return window's height
	size_t getHeight() const;
	
	/// @return X position of left upper window's corner
	size_t getStartX() const;
	
	/// @return Y position of left upper window's corner
	size_t getStarty() const;
	
	/// @return window's title
	const std::string &getTitle() const;
	
	/// @return current window's color
	Color getColor() const;
	
	/// @return current window's border
	Border getBorder() const;
	
	/// @return current window's timeout
	int getTimeout() const;
	
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
	/// @see setGetStringHelper()
	/// @see SetTimeout()
	/// @see CreateHistory()
	std::string getString(const std::string &base, size_t width = 0, bool encrypted = 0);
	
	/// Wrapper for above function that doesn't take base string (it will be empty).
	/// Taken parameters are the same as for above.
	std::string getString(size_t width = 0, bool encrypted = 0)
	{
		return getString("", width, encrypted);
	}
	
	/// Moves cursor to given coordinates
	/// @param x given X position
	/// @param y given Y position
	void goToXY(int x, int y);
	
	/// @return x window coordinate
	int getX();
	
	/// @return y windows coordinate
	int getY();
	
	/// Used to indicate whether given coordinates of main screen lies within
	/// window area or not and if they do, transform them into in-window coords.
	/// Otherwise function doesn't modify its arguments.
	/// @param x X position of main screen to be checked
	/// @param y Y position of main screen to be checked
	/// @return true if it transformed variables, false otherwise
	bool hasCoords(int &x, int &y);
	
	/// Sets helper function used in getString()
	/// @param helper pointer to function that matches getStringHelper prototype
	/// @see getString()
	void setGetStringHelper(GetStringHelper helper) { m_get_string_helper = helper; }

	/// Run current GetString helper function (if defined).
	/// @see getString()
	/// @return true if helper was run, false otherwise
	bool runGetStringHelper(const char *arg, bool *done) const;

	/// Sets window's base color
	/// @param fg foregound base color
	/// @param bg background base color
	void setBaseColor(Color fg, Color bg = Color::Default);
	
	/// Sets window's border
	/// @param border new window's border
	void setBorder(Border border);
	
	/// Sets window's timeout
	/// @param timeout window's timeout
	void setTimeout(int timeout);
	
	/// Sets window's title
	/// @param new_title new title for window
	void setTitle(const std::string &new_title);
	
	/// Refreshed whole window and its border
	/// @see refresh()
	void display();
	
	/// Refreshes whole window, but not the border
	/// @see display()
	virtual void refresh();
	
	/// Moves the window to new coordinates
	/// @param new_x new X position of left upper corner of window
	/// @param new_y new Y position of left upper corner of window
	virtual void moveTo(size_t new_x, size_t new_y);
	
	/// Resizes the window
	/// @param new_width new window's width
	/// @param new_height new window's height
	virtual void resize(size_t new_width, size_t new_height);
	
	/// Cleares the window
	virtual void clear();
	
	/// Adds given file descriptor to the list that will be polled in
	/// ReadKey() along with stdin and callback that will be invoked
	/// when there is data waiting for reading in it
	/// @param fd file descriptor
	/// @param callback callback
	void addFDCallback(int fd, void (*callback)());
	
	/// Clears list of file descriptors and their callbacks
	void clearFDCallbacksList();
	
	/// Checks if list of file descriptors is empty
	/// @return true if list is empty, false otherwise
	bool FDCallbacksListEmpty() const;
	
	/// Reads key from standard input (or takes it from input queue)
	/// and writes it into read_key variable
	int readKey();
	
	/// Push single character into input queue, so it can get consumed by ReadKey
	void pushChar(int ch);
	
	/// @return const reference to internal input queue
	const std::queue<int> &inputQueue() { return m_input_queue; }
	
	/// Scrolls the window by amount of lines given in its parameter
	/// @param where indicates how many lines it has to scroll
	virtual void scroll(Scroll where);
	
	/// Applies function of compatible prototype to internal WINDOW pointer
	/// The mostly used function in this case seem to be wclrtoeol(), which
	/// clears the window from current cursor position to the end of line.
	/// Note that delwin() also matches that prototype, but I wouldn't
	/// recommend anyone passing this pointer here ;)
	/// @param f pointer to function to call with internal WINDOW pointer
	/// @return reference to itself
	Window &operator<<(int (*f)(WINDOW *));
	
	/// Applies foreground and background colors to window
	/// @param colors struct that holds new colors information
	/// @return reference to itself
	Window &operator<<(Colors colors);
	
	/// Applies foregound color to window. Note that colors applied
	/// that way are stacked, i.e if you applied Color::Red, then Color::Green
	/// and Color::End, current color would be Color::Red. If you want to discard
	/// all colors and fall back to base one, pass Color::Default.
	/// @param color new color value
	/// @return reference to itself
	Window &operator<<(Color color);
	
	/// Applies format flag to window. Note that these attributes are
	/// also stacked, so if you applied Format::Bold twice, to get rid of
	/// it you have to pass Format::NoBold also twice.
	/// @param format format flag
	/// @return reference to itself
	Window &operator<<(Format format);
	
	/// Moves current cursor position to given coordinates.
	/// @param coords struct that holds information about new coordinations
	/// @return reference to itself
	Window &operator<<(XY coords);
	
	/// Prints string to window
	/// @param s const pointer to char array to be printed
	/// @return reference to itself
	Window &operator<<(const char *s);
	
	/// Prints single character to window
	/// @param c character to be printed
	/// @return reference to itself
	Window &operator<<(char c);
	
	/// Prints wide string to window
	/// @param ws const pointer to wchar_t array to be printed
	/// @return reference to itself
	Window &operator<<(const wchar_t *ws);
	
	/// Prints single wide character to window
	/// @param wc wide character to be printed
	/// @return reference to itself
	Window &operator<<(wchar_t wc);
	
	/// Prints int to window
	/// @param i integer value to be printed
	/// @return reference to itself
	Window &operator<<(int i);
	
	/// Prints double to window
	/// @param d double value to be printed
	/// @return reference to itself
	Window &operator<<(double d);
	
	/// Prints size_t to window
	/// @param s size value to be printed
	/// @return reference to itself
	Window &operator<<(size_t s);
	
	/// Prints std::string to window
	/// @param s string to be printed
	/// @return reference to itself
	Window &operator<<(const std::string &s);
	
	/// Prints std::wstring to window
	/// @param ws wide string to be printed
	/// @return reference to itself
	Window &operator<<(const std::wstring &ws);
protected:
	/// Sets colors of window (interal use only)
	/// @param fg foregound color
	/// @param bg background color
	///
	void setColor(Color fg, Color bg = Color::Default);
	
	/// Refreshes window's border
	///
	void showBorder() const;
	
	/// Changes dimensions of window, called from resize()
	/// @param width new window's width
	/// @param height new window's height
	/// @see resize()
	///
	void adjustDimensions(size_t width, size_t height);
	
	/// Deletes old window and creates new. It's called by resize(),
	/// SetBorder() or setTitle() since internally windows are
	/// handled as curses pads and change in size requires to delete
	/// them and create again, there is no way to change size of pad.
	/// @see SetBorder()
	/// @see setTitle()
	/// @see resize()
	///
	virtual void recreate(size_t width, size_t height);
	
	/// internal WINDOW pointers
	WINDOW *m_window;
	WINDOW *m_border_window;
	
	/// start points and dimensions
	size_t m_start_x;
	size_t m_start_y;
	size_t m_width;
	size_t m_height;
	
	/// window timeout
	int m_window_timeout;
	
	/// current colors
	Color m_color;
	Color m_bg_color;
	
	/// base colors
	Color m_base_color;
	Color m_base_bg_color;
	
	/// current border
	Border m_border;
	
private:
	/// Sets state of bold attribute (internal use only)
	/// @param bold_state state of bold attribute
	///
	void bold(bool bold_state) const;
	
	/// Sets state of underline attribute (internal use only)
	/// @param underline_state state of underline attribute
	///
	void underline(bool underline_state) const;
	
	/// Sets state of reverse attribute (internal use only)
	/// @param reverse_state state of reverse attribute
	///
	void reverse(bool reverse_state) const;
	
	/// Sets state of altcharset attribute (internal use only)
	/// @param altcharset_state state of altcharset attribute
	///
	void altCharset(bool altcharset_state) const;
	
	/// pointer to helper function used by getString()
	/// @see getString()
	///
	GetStringHelper m_get_string_helper;
	
	/// window title
	std::string m_title;
	
	/// stack of colors
	std::stack<Colors> m_color_stack;
	
	/// input queue of a window. you can put characters there using
	/// PushChar and they will be immediately consumed and
	/// returned by ReadKey
	std::queue<int> m_input_queue;
	
	/// containter used for additional file descriptors that have
	/// to be polled in ReadKey() and correspondent callbacks that
	/// are invoked if there is data available in them
	typedef std::vector< std::pair<int, void (*)()> > FDCallbacks;
	FDCallbacks m_fds;
	
	/// counters for format flags
	int m_bold_counter;
	int m_underline_counter;
	int m_reverse_counter;
	int m_alt_charset_counter;
};

}

#endif // NCMPCPP_WINDOW_H
