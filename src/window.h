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

#if NCURSES_MOUSE_VERSION == 1
// NOTICE: define BUTTON5_PRESSED to be BUTTON2_PRESSED with additional mask
// (I noticed that it sometimes returns 134217728 (2^27) instead of expected
// mask, so the modified define does it right.
# define BUTTON5_PRESSED (BUTTON2_PRESSED | (1U << 27))
#endif // NCURSES_MOUSE_VERSION == 1

// undefine macros with colliding names
#undef border
#undef scroll

/// NC namespace provides set of easy-to-use
/// wrappers over original curses library.
namespace NC {

/// Thrown if Ctrl-C or Ctrl-G is pressed during the call to Window::getString()
/// @see Window::getString()
struct PromptAborted : std::exception
{
	template <typename ArgT>
	PromptAborted(ArgT &&prompt)
	: m_prompt(std::forward<ArgT>(prompt)) { }

	virtual const char *what() const noexcept OVERRIDE { return m_prompt.c_str(); }

private:
	std::string m_prompt;
};

struct Color
{
	friend struct Window;

	static const short transparent;
	static const short previous;

	Color() : m_rep(0, transparent, true, false) { }
	Color(short foreground_value, short background_value,
			 bool is_default = false, bool is_end = false)
	: m_rep(foreground_value, background_value, is_default, is_end)
	{
		if (isDefault() && isEnd())
			throw std::logic_error("Color flag can't be marked as both 'default' and 'end'");
	}

	bool operator==(const Color &rhs) const
	{
		return m_rep == rhs.m_rep;
	}
	bool operator!=(const Color &rhs) const
	{
		return m_rep != rhs.m_rep;
	}
	bool operator<(const Color &rhs) const
	{
		return m_rep < rhs.m_rep;
	}

	bool isDefault() const
	{
		return std::get<2>(m_rep);
	}
	bool isEnd() const
	{
		return std::get<3>(m_rep);
	}

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
	short foreground() const
	{
		return std::get<0>(m_rep);
	}
	short background() const
	{
		return std::get<1>(m_rep);
	}
	bool previousBackground() const
	{
		return background() == previous;
	}

	std::tuple<short, short, bool, bool> m_rep;
};

std::istream &operator>>(std::istream &is, Color &f);

typedef boost::optional<Color> Border;

/// Terminal manipulation functions
enum class TermManip { ClearToEOL };

/// Format flags used by NCurses
enum class Format {
	None,
	Bold, NoBold,
	Underline, NoUnderline,
	Reverse, NoReverse,
	AltCharset, NoAltCharset
};

/// This indicates how much the window has to be scrolled
enum class Scroll { Up, Down, PageUp, PageDown, Home, End };

/// Initializes curses screen and sets some additional attributes
/// @param enable_colors enables colors
void initScreen(bool enable_colors);

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
	void setBaseColor(Color c);
	
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
	int readKey();
	
	/// Push single character into input queue, so it can get consumed by ReadKey
	void pushChar(int ch);
	
	/// @return const reference to internal input queue
	const std::queue<int> &inputQueue() { return m_input_queue; }
	
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
	
	/// Refreshes window's border
	///
	void refreshBorder() const;
	
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
