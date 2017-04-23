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

#ifndef NCMPCPP_WINDOW_H
#define NCMPCPP_WINDOW_H

#define NCURSES_NOMACROS 1

#include "config.h"

#include "curses.h"
#include "gcc.h"

#include <boost/optional.hpp>
#include <functional>
#include <list>
#include <stack>
#include <vector>
#include <string>
#include <tuple>
#include <queue>

#if NCURSES_MOUSE_VERSION == 1
# define BUTTON5_PRESSED (1U << 27)
#endif // NCURSES_MOUSE_VERSION == 1

/// NC namespace provides set of easy-to-use
/// wrappers over original curses library.
namespace NC {

namespace Key {

typedef uint64_t Type;

const Type None = -1;

// modifier masks
const Type Special = Type{1} << 63;
const Type Alt     = Type{1} << 62;
const Type Ctrl    = Type{1} << 61;
const Type Shift   = Type{1} << 60;

// useful names
const Type Null      = 0;
const Type Space     = 32;
const Type Backspace = 127;

// ctrl-?
const Type Ctrl_A            = 1;
const Type Ctrl_B            = 2;
const Type Ctrl_C            = 3;
const Type Ctrl_D            = 4;
const Type Ctrl_E            = 5;
const Type Ctrl_F            = 6;
const Type Ctrl_G            = 7;
const Type Ctrl_H            = 8;
const Type Ctrl_I            = 9;
const Type Ctrl_J            = 10;
const Type Ctrl_K            = 11;
const Type Ctrl_L            = 12;
const Type Ctrl_M            = 13;
const Type Ctrl_N            = 14;
const Type Ctrl_O            = 15;
const Type Ctrl_P            = 16;
const Type Ctrl_Q            = 17;
const Type Ctrl_R            = 18;
const Type Ctrl_S            = 19;
const Type Ctrl_T            = 20;
const Type Ctrl_U            = 21;
const Type Ctrl_V            = 22;
const Type Ctrl_W            = 23;
const Type Ctrl_X            = 24;
const Type Ctrl_Y            = 25;
const Type Ctrl_Z            = 26;
const Type Ctrl_LeftBracket  = 27;
const Type Ctrl_Backslash    = 28;
const Type Ctrl_RightBracket = 29;
const Type Ctrl_Caret        = 30;
const Type Ctrl_Underscore   = 31;

// useful duplicates
const Type Tab    = 9;
const Type Enter  = 13;
const Type Escape = 27;

// special values, beyond one byte
const Type Insert   = Special | 256;
const Type Delete   = Special | 257;
const Type Home     = Special | 258;
const Type End      = Special | 259;
const Type PageUp   = Special | 260;
const Type PageDown = Special | 261;
const Type Up       = Special | 262;
const Type Down     = Special | 263;
const Type Left     = Special | 264;
const Type Right    = Special | 265;
const Type F1       = Special | 266;
const Type F2       = Special | 267;
const Type F3       = Special | 268;
const Type F4       = Special | 269;
const Type F5       = Special | 270;
const Type F6       = Special | 271;
const Type F7       = Special | 272;
const Type F8       = Special | 273;
const Type F9       = Special | 274;
const Type F10      = Special | 275;
const Type F11      = Special | 276;
const Type F12      = Special | 277;
const Type Mouse    = Special | 278;
const Type EoF      = Special | 279;

}

/// Thrown if Ctrl-C or Ctrl-G is pressed during the call to Window::getString()
/// @see Window::getString()
struct PromptAborted : std::exception
{
	PromptAborted() { }

	template <typename ArgT>
	PromptAborted(ArgT &&prompt)
		: m_prompt(std::forward<ArgT>(prompt)) { }

	virtual const char *what() const noexcept override { return m_prompt.c_str(); }

private:
	std::string m_prompt;
};

struct Color
{
	friend struct Window;

	static const short transparent;
	static const short current;

	Color() : m_impl(0, 0, true, false) { }
	Color(short foreground_value, short background_value,
			 bool is_default = false, bool is_end = false)
	: m_impl(foreground_value, background_value, is_default, is_end)
	{
		if (isDefault() && isEnd())
			throw std::logic_error("Color flag can't be marked as both 'default' and 'end'");
	}

	bool operator==(const Color &rhs) const { return m_impl == rhs.m_impl; }
	bool operator!=(const Color &rhs) const { return m_impl != rhs.m_impl; }
	bool operator<(const Color &rhs) const { return m_impl < rhs.m_impl; }

	bool isDefault() const { return std::get<2>(m_impl); }
	bool isEnd() const { return std::get<3>(m_impl); }

	int pairNumber() const;

	static Color Default;
	static Color Black;
	static Color Red;
	static Color Green;
	static Color Yellow;
	static Color Blue;
	static Color Magenta;
	static Color Cyan;
	static Color White;
	static Color End;

private:
	short foreground() const { return std::get<0>(m_impl); }
	short background() const { return std::get<1>(m_impl); }
	bool currentBackground() const { return background() == current; }

	std::tuple<short, short, bool, bool> m_impl;
};

std::istream &operator>>(std::istream &is, Color &f);

typedef boost::optional<Color> Border;

/// Terminal manipulation functions
enum class TermManip { ClearToEOL };

/// Format flags used by NCurses
enum class Format {
	Bold, NoBold,
	Underline, NoUnderline,
	Reverse, NoReverse,
	AltCharset, NoAltCharset
};

NC::Format reverseFormat(NC::Format fmt);

/// This indicates how much the window has to be scrolled
enum class Scroll { Up, Down, PageUp, PageDown, Home, End };

namespace Mouse {

void enable();
void disable();

}

/// Initializes curses screen and sets some additional attributes
/// @param enable_colors enables colors
void initScreen(bool enable_colors, bool enable_mouse);

/// Destroys the screen
void destroyScreen();

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
	/// Helper function that is periodically invoked
	// inside Window::getString() function
	/// @see Window::getString()
	typedef std::function<bool(const char *)> PromptHook;

	/// Sets helper to a specific value for the current scope
	struct ScopedPromptHook
	{
		template <typename HelperT>
		ScopedPromptHook(Window &w, HelperT &&helper) noexcept
		: m_w(w), m_hook(std::move(w.m_prompt_hook)) {
			m_w.m_prompt_hook = std::forward<HelperT>(helper);
		}
		~ScopedPromptHook() noexcept {
			m_w.m_prompt_hook = std::move(m_hook);
		}

	private:
		Window &m_w;
		PromptHook m_hook;
	};

	struct ScopedTimeout
	{
		ScopedTimeout(Window &w, int new_timeout)
			: m_w(w)
		{
			m_timeout = w.getTimeout();
			w.setTimeout(new_timeout);
		}

		~ScopedTimeout()
		{
			m_w.setTimeout(m_timeout);
		}

	private:
		Window &m_w;
		int m_timeout;
	};

	Window() : m_window(nullptr) { }
	
	/// Constructs an empty window with given parameters
	/// @param startx X position of left upper corner of constructed window
	/// @param starty Y position of left upper corner of constructed window
	/// @param width width of constructed window
	/// @param height height of constructed window
	/// @param title title of constructed window
	/// @param color base color of constructed window
	/// @param border border of constructed window
	Window(size_t startx, size_t starty, size_t width, size_t height,
			std::string title, Color color, Border border);
	
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
	const Color &getColor() const;
	
	/// @return current window's border
	const Border &getBorder() const;
	
	/// @return current window's timeout
	int getTimeout() const;
	
	/// @return current mouse event if readKey() returned KEY_MOUSE
	const MEVENT &getMouseEvent();

	/// Reads the string from standard input using readline library.
	/// @param base base string that has to be edited
	/// @param length max length of the string, unlimited by default
	/// @param width width of area that entry field can take. if it's reached, string
	/// will be scrolled. if value is 0, field will be from cursor position to the end
	/// of current line wide.
	/// @param encrypted if set to true, '*' characters will be displayed instead of
	/// actual text.
	/// @return edited string
	///
	/// @see setPromptHook()
	/// @see SetTimeout()
	std::string prompt(const std::string &base = "", size_t width = -1, bool encrypted = false);
	
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
	
	/// Sets hook used in getString()
	/// @param hook pointer to function that matches getStringHelper prototype
	/// @see getString()
	template <typename HookT>
	void setPromptHook(HookT &&hook) {
		m_prompt_hook = std::forward<HookT>(hook);
	}

	/// Run current GetString helper function (if defined).
	/// @see getString()
	/// @return true if helper was run, false otherwise
	bool runPromptHook(const char *arg, bool *done) const;

	/// Sets window's base color
	/// @param fg foregound base color
	/// @param bg background base color
	void setBaseColor(const Color &color);
	
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

	/// Refreshes window's border
	void refreshBorder() const;

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
	/// readKey() along with stdin and callback that will be invoked
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
	Key::Type readKey();
	
	/// Push single character into input queue, so it can get consumed by ReadKey
	void pushChar(const NC::Key::Type ch);
	
	/// @return const reference to internal input queue
	const std::queue<NC::Key::Type> &inputQueue() { return m_input_queue; }
	
	/// Scrolls the window by amount of lines given in its parameter
	/// @param where indicates how many lines it has to scroll
	virtual void scroll(Scroll where);
	
	Window &operator<<(TermManip tm);
	Window &operator<<(const Color &color);
	Window &operator<<(Format format);
	Window &operator<<(const XY &coords);
	Window &operator<<(const char *s);
	Window &operator<<(char c);
	Window &operator<<(const wchar_t *ws);
	Window &operator<<(wchar_t wc);
	Window &operator<<(int i);
	Window &operator<<(double d);
	Window &operator<<(size_t s);
	Window &operator<<(const std::string &s);
	Window &operator<<(const std::wstring &ws);
protected:
	/// Sets colors of window (interal use only)
	/// @param fg foregound color
	/// @param bg background color
	///
	void setColor(Color c);
	
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
	
	/// start points and dimensions
	size_t m_start_x;
	size_t m_start_y;
	size_t m_width;
	size_t m_height;
	
	/// window timeout
	int m_window_timeout;
	
	/// current colors
	Color m_color;
	
	/// base colors
	Color m_base_color;
	
	/// current border
	Border m_border;
	
private:
	Key::Type getInputChar(int key);

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
	PromptHook m_prompt_hook;
	
	/// window title
	std::string m_title;
	
	/// stack of colors
	std::stack<Color> m_color_stack;
	
	/// input queue of a window. you can put characters there using
	/// PushChar and they will be immediately consumed and
	/// returned by ReadKey
	std::queue<Key::Type> m_input_queue;
	
	/// containter used for additional file descriptors that have
	/// to be polled in ReadKey() and correspondent callbacks that
	/// are invoked if there is data available in them
	typedef std::vector< std::pair<int, void (*)()> > FDCallbacks;
	FDCallbacks m_fds;
	
	MEVENT m_mouse_event;
	bool m_escape_terminal_sequences;

	/// counters for format flags
	int m_bold_counter;
	int m_underline_counter;
	int m_reverse_counter;
	int m_alt_charset_counter;
};

}

#endif // NCMPCPP_WINDOW_H
