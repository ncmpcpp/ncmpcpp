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

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/select.h>
#include <unistd.h>

#include "utility/readline.h"
#include "utility/string.h"
#include "utility/wide_string.h"
#include "window.h"

namespace {

namespace rl {

bool aborted;

NC::Window *w;
size_t start_x;
size_t start_y;
size_t width;
bool encrypted;
const char *base;

int read_key(FILE *)
{
	size_t x;
	bool done;
	int result;
	do
	{
		x = w->getX();
		if (w->runPromptHook(rl_line_buffer, &done))
		{
			// Do not end if readline is in one of its commands, e.g. searching
			// through history, as it doesn't actually make readline exit and it
			// becomes stuck in a loop.
			if (!RL_ISSTATE(RL_STATE_DISPATCHING) && done)
			{
				rl_done = 1;
				return EOF;
			}
			w->goToXY(x, start_y);
		}
		w->refresh();
		result = w->readKey();
		if (!w->FDCallbacksListEmpty())
		{
			w->goToXY(x, start_y);
			w->refresh();
		}
	}
	while (result == ERR);
	return result;
}

void display_string()
{
	auto print_char = [](wchar_t wc) {
		if (encrypted)
			*w << '*';
		else
			*w << wc;
	};
	auto print_string = [](wchar_t *ws, size_t len) {
		if (encrypted)
			for (size_t i = 0; i < len; ++i)
				*w << '*';
		else
			*w << ws;
	};
	auto narrow_to_wide = [](wchar_t *dest, const char *src, size_t n) {
		size_t result = 0;
		// convert the string and substitute invalid multibyte chars with dots.
		for (size_t i = 0; i < n;)
		{
			int ret = mbtowc(&dest[result], &src[i], n-i);
			if (ret > 0)
			{
				i += ret;
				++result;
			}
			else if (ret == -1)
			{
				dest[result] = L'.';
				++i;
				++result;
			}
			else
				throw std::runtime_error("mbtowc: unexpected return value");
		}
		return result;
	};

	// copy the part of the string that is before the cursor to pre_pos
	char pt = rl_line_buffer[rl_point];
	rl_line_buffer[rl_point] = 0;
	wchar_t pre_pos[rl_point+1];
	pre_pos[narrow_to_wide(pre_pos, rl_line_buffer, rl_point)] = 0;
	rl_line_buffer[rl_point] = pt;

	int pos = wcswidth(pre_pos, rl_point);
	if (pos < 0)
		pos = rl_point;

	// clear the area for the string
	mvwhline(w->raw(), start_y, start_x, ' ', width+1);

	w->goToXY(start_x, start_y);
	if (size_t(pos) <= width)
	{
		// if the current position in the string is not bigger than allowed
		// width, print the part of the string before cursor position...

		print_string(pre_pos, pos);

		// ...and then print the rest char-by-char until there is no more area
		wchar_t post_pos[rl_end-rl_point+1];
		post_pos[narrow_to_wide(post_pos, rl_line_buffer+rl_point, rl_end-rl_point)] = 0;

		size_t cpos = pos;
		for (wchar_t *c = post_pos; *c != 0; ++c)
		{
			int n = wcwidth(*c);
			if (n < 0)
			{
				print_char(L'.');
				++cpos;
			}
			else
			{
				if (cpos+n > width)
					break;
				cpos += n;
				print_char(*c);
			}
		}
	}
	else
	{
		// if the current position in the string is bigger than allowed
		// width, we always keep the cursor at the end of the line (it
		// would be nice to have more flexible scrolling, but for now
		// let's stick to that) by cutting the beginning of the part
		// of the string before the cursor until it fits the area.

		wchar_t *mod_pre_pos = pre_pos;
		while (*mod_pre_pos != 0)
		{
			++mod_pre_pos;
			int n = wcwidth(*mod_pre_pos);
			if (n < 0)
				--pos;
			else
				pos -= n;
			if (size_t(pos) <= width)
				break;
		}
		print_string(mod_pre_pos, pos);
	}
	w->goToXY(start_x+pos, start_y);
}

int add_base()
{
	rl_insert_text(base);
	return 0;
}

}

int color_pair_counter;
std::vector<int> color_pair_map;

}

namespace NC {

const short Color::transparent = -1;
const short Color::current = -2;

Color Color::Default(0, 0, true, false);
Color Color::Black(COLOR_BLACK, Color::current);
Color Color::Red(COLOR_RED, Color::current);
Color Color::Green(COLOR_GREEN, Color::current);
Color Color::Yellow(COLOR_YELLOW, Color::current);
Color Color::Blue(COLOR_BLUE, Color::current);
Color Color::Magenta(COLOR_MAGENTA, Color::current);
Color Color::Cyan(COLOR_CYAN, Color::current);
Color Color::White(COLOR_WHITE, Color::current);
Color Color::End(0, 0, false, true);

int Color::pairNumber() const
{
	int result = 0;
	if (isEnd())
		throw std::logic_error("'end' doesn't have a corresponding pair number");
	else if (!isDefault())
	{
		if (!currentBackground())
			result = (background() + 1) % COLORS;
		result *= 256;
		result += foreground() % COLORS;

		assert(result < int(color_pair_map.size()));

		// NCurses allows for a limited number of color pairs to be registered, so
		// in order to be able to support all the combinations we want to, we need
		// to dynamically register only pairs of colors we're actually using.
		if (!color_pair_map[result])
		{
			// Check if there are any unused pairs left and either register the one
			// that was requested or return a default one if there is no space left.
			if (color_pair_counter >= COLOR_PAIRS)
				result = 0;
			else
			{
				init_pair(color_pair_counter, foreground(), background());
				color_pair_map[result] = color_pair_counter;
				++color_pair_counter;
			}
		}
		result = color_pair_map[result];
	}
	return result;
}

std::istream &operator>>(std::istream &is, Color &c)
{
	const short invalid_color_value = -1337;
	auto get_single_color = [](const std::string &s, bool background) {
		short result = invalid_color_value;
		if (s == "black")
			result = COLOR_BLACK;
		else if (s == "red")
			result = COLOR_RED;
		else if (s == "green")
			result = COLOR_GREEN;
		else if (s == "yellow")
			result = COLOR_YELLOW;
		else if (s == "blue")
			result = COLOR_BLUE;
		else if (s == "magenta")
			result = COLOR_MAGENTA;
		else if (s == "cyan")
			result = COLOR_CYAN;
		else if (s == "white")
			result = COLOR_WHITE;
		else if (background && s == "transparent")
			result = NC::Color::transparent;
		else if (background && s == "current")
			result = NC::Color::current;
		else if (std::all_of(s.begin(), s.end(), isdigit))
		{
			result = atoi(s.c_str());
			if (result < (background ? 0 : 1) || result > 256)
				result = invalid_color_value;
			else
				--result;
		}
		return result;
	};

	auto get_color = [](std::istream &is_) {
		std::string result;
		while (!is_.eof() && isalnum(is_.peek()))
			result.push_back(is_.get());
		return result;
	};

	std::string sc = get_color(is);

	if (sc == "default")
		c = Color::Default;
	else if (sc == "end")
		c = Color::End;
	else
	{
		short fg = get_single_color(sc, false);
		if (fg == invalid_color_value)
			is.setstate(std::ios::failbit);
		// Check if there is background color
		else if (!is.eof() && is.peek() == '_')
		{
			is.get();
			sc = get_color(is);
			short bg = get_single_color(sc, true);
			if (bg == invalid_color_value)
				is.setstate(std::ios::failbit);
			else
				c = Color(fg, bg);
		}
		else
			c = Color(fg, NC::Color::current);
	}
	return is;
}

NC::Format reverseFormat(NC::Format fmt)
{
	switch (fmt)
	{
	case NC::Format::Bold:
		return NC::Format::NoBold;
	case NC::Format::NoBold:
		return NC::Format::Bold;
	case NC::Format::Underline:
		return NC::Format::NoUnderline;
	case NC::Format::NoUnderline:
		return NC::Format::Underline;
	case NC::Format::Reverse:
		return NC::Format::NoReverse;
	case NC::Format::NoReverse:
		return NC::Format::Reverse;
	case NC::Format::AltCharset:
		return NC::Format::NoAltCharset;
	case NC::Format::NoAltCharset:
		return NC::Format::AltCharset;
	}
	// Unreachable, silence GCC.
	return fmt;
}

namespace Mouse {

namespace {

bool supportEnabled = false;

}

void enable()
{
	if (!supportEnabled)
		return;
	// save old highlight mouse tracking
	std::printf("\e[?1001s");
	// enable mouse tracking
	std::printf("\e[?1000h");
	// try to enable extended (urxvt) mouse tracking
	std::printf("\e[?1015h");
	// send the above to the terminal immediately
	std::fflush(stdout);
}

void disable()
{
	if (!supportEnabled)
		return;
	// disable extended (urxvt) mouse tracking
	std::printf("\e[?1015l");
	// disable mouse tracking
	std::printf("\e[?1000l");
	// restore old highlight mouse tracking
	std::printf("\e[?1001r");
	// send the above to the terminal immediately
	std::fflush(stdout);
}

}

void initScreen(bool enable_colors, bool enable_mouse)
{
	initscr();
	if (has_colors() && enable_colors)
	{
		start_color();
		use_default_colors();
		color_pair_map.resize(256 * 256, 0);

		// Predefine pairs for colors with transparent background, all the other
		// ones will be dynamically registered in Color::pairNumber when they're
		// used.
		color_pair_counter = 1;
		for (int fg = 0; fg < COLORS; ++fg, ++color_pair_counter)
		{
			init_pair(color_pair_counter, fg, -1);
			color_pair_map[fg] = color_pair_counter;
		}
	}
	raw();
	nonl();
	noecho();
	timeout(0);
	curs_set(0);

	// setup mouse
	Mouse::supportEnabled = enable_mouse;
	Mouse::enable();

	// initialize readline (needed, otherwise we get segmentation
	// fault on SIGWINCH). also, initialize first as doing this
	// later erases keys bound with rl_bind_key for some users.
	rl_initialize();
	// disable autocompletion
	rl_attempted_completion_function = [](const char *, int, int) -> char ** {
		rl_attempted_completion_over = 1;
		return nullptr;
	};
	auto abort_prompt = [](int, int) -> int {
		rl::aborted = true;
		rl_done = 1;
		return 0;
	};
	// if ctrl-c or ctrl-g is pressed, abort the prompt
	rl_bind_key('\3', abort_prompt);
	rl_bind_key('\7', abort_prompt);
	// do not change the state of the terminal
	rl_prep_term_function = nullptr;
	rl_deprep_term_function = nullptr;
	// do not catch signals
	rl_catch_signals = 0;
	rl_catch_sigwinch = 0;
	// overwrite readline callbacks
	rl_getc_function = rl::read_key;
	rl_redisplay_function = rl::display_string;
	rl_startup_hook = rl::add_base;
}

void destroyScreen()
{
	Mouse::disable();
	curs_set(1);
	endwin();
}

Window::Window(size_t startx, size_t starty, size_t width, size_t height,
               std::string title, Color color, Border border)
	: m_window(nullptr),
	  m_start_x(startx),
	  m_start_y(starty),
	  m_width(width),
	  m_height(height),
	  m_window_timeout(-1),
	  m_border(std::move(border)),
	  m_prompt_hook(0),
	  m_title(std::move(title)),
	  m_escape_terminal_sequences(true),
	  m_bold_counter(0),
	  m_underline_counter(0),
	  m_reverse_counter(0),
	  m_alt_charset_counter(0)
{
	if (m_start_x > size_t(COLS)
	    ||  m_start_y > size_t(LINES)
	    ||  m_width+m_start_x > size_t(COLS)
	    ||  m_height+m_start_y > size_t(LINES))
		throw std::logic_error("constructed window doesn't fit into the terminal");

	if (m_border)
	{
		++m_start_x;
		++m_start_y;
		m_width -= 2;
		m_height -= 2;
	}
	if (!m_title.empty())
	{
		m_start_y += 2;
		m_height -= 2;
	}
	
	m_window = newpad(m_height, m_width);
	wtimeout(m_window, 0);

	setBaseColor(color);
	setColor(m_base_color);
}

Window::Window(const Window &rhs)
: m_window(dupwin(rhs.m_window))
, m_start_x(rhs.m_start_x)
, m_start_y(rhs.m_start_y)
, m_width(rhs.m_width)
, m_height(rhs.m_height)
, m_window_timeout(rhs.m_window_timeout)
, m_color(rhs.m_color)
, m_base_color(rhs.m_base_color)
, m_border(rhs.m_border)
, m_prompt_hook(rhs.m_prompt_hook)
, m_title(rhs.m_title)
, m_color_stack(rhs.m_color_stack)
, m_input_queue(rhs.m_input_queue)
, m_fds(rhs.m_fds)
, m_escape_terminal_sequences(rhs.m_escape_terminal_sequences)
, m_bold_counter(rhs.m_bold_counter)
, m_underline_counter(rhs.m_underline_counter)
, m_reverse_counter(rhs.m_reverse_counter)
, m_alt_charset_counter(rhs.m_alt_charset_counter)
{
	setColor(m_color);
}

Window::Window(Window &&rhs)
: m_window(rhs.m_window)
, m_start_x(rhs.m_start_x)
, m_start_y(rhs.m_start_y)
, m_width(rhs.m_width)
, m_height(rhs.m_height)
, m_window_timeout(rhs.m_window_timeout)
, m_color(rhs.m_color)
, m_base_color(rhs.m_base_color)
, m_border(rhs.m_border)
, m_prompt_hook(rhs.m_prompt_hook)
, m_title(std::move(rhs.m_title))
, m_color_stack(std::move(rhs.m_color_stack))
, m_input_queue(std::move(rhs.m_input_queue))
, m_fds(std::move(rhs.m_fds))
, m_escape_terminal_sequences(rhs.m_escape_terminal_sequences)
, m_bold_counter(rhs.m_bold_counter)
, m_underline_counter(rhs.m_underline_counter)
, m_reverse_counter(rhs.m_reverse_counter)
, m_alt_charset_counter(rhs.m_alt_charset_counter)
{
	rhs.m_window = nullptr;
}

Window &Window::operator=(Window rhs)
{
	std::swap(m_window, rhs.m_window);
	std::swap(m_start_x, rhs.m_start_x);
	std::swap(m_start_y, rhs.m_start_y);
	std::swap(m_width, rhs.m_width);
	std::swap(m_height, rhs.m_height);
	std::swap(m_window_timeout, rhs.m_window_timeout);
	std::swap(m_color, rhs.m_color);
	std::swap(m_base_color, rhs.m_base_color);
	std::swap(m_border, rhs.m_border);
	std::swap(m_prompt_hook, rhs.m_prompt_hook);
	std::swap(m_title, rhs.m_title);
	std::swap(m_color_stack, rhs.m_color_stack);
	std::swap(m_input_queue, rhs.m_input_queue);
	std::swap(m_fds, rhs.m_fds);
	std::swap(m_escape_terminal_sequences, rhs.m_escape_terminal_sequences);
	std::swap(m_bold_counter, rhs.m_bold_counter);
	std::swap(m_underline_counter, rhs.m_underline_counter);
	std::swap(m_reverse_counter, rhs.m_reverse_counter);
	std::swap(m_alt_charset_counter, rhs.m_alt_charset_counter);
	return *this;
}

Window::~Window()
{
	delwin(m_window);
}

void Window::setColor(Color c)
{
	if (c.isDefault())
		c = m_base_color;
	if (c != Color::Default)
	{
		assert(!c.currentBackground());
		wcolor_set(m_window, c.pairNumber(), nullptr);
	}
	else
		wcolor_set(m_window, m_base_color.pairNumber(), nullptr);
	m_color = std::move(c);
}

void Window::setBaseColor(const Color &color)
{
	if (color.currentBackground())
		m_base_color = Color(color.foreground(), Color::transparent);
	else
		m_base_color = color;
}

void Window::setBorder(Border border)
{
	if (!border && m_border)
	{
		--m_start_x;
		--m_start_y;
		m_height += 2;
		m_width += 2;
		recreate(m_width, m_height);
	}
	else if (border && !m_border)
	{
		++m_start_x;
		++m_start_y;
		m_height -= 2;
		m_width -= 2;
		recreate(m_width, m_height);
	}
	m_border = border;
}

void Window::setTitle(const std::string &new_title)
{
	if (!new_title.empty() && m_title.empty())
	{
		m_start_y += 2;
		m_height -= 2;
		recreate(m_width, m_height);
	}
	else if (new_title.empty() && !m_title.empty())
	{
		m_start_y -= 2;
		m_height += 2;
		recreate(m_width, m_height);
	}
	m_title = new_title;
}

void Window::recreate(size_t width, size_t height)
{
	delwin(m_window);
	m_window = newpad(height, width);
	wtimeout(m_window, 0);
	setColor(m_color);
}

void Window::moveTo(size_t new_x, size_t new_y)
{
	m_start_x = new_x;
	m_start_y = new_y;
	if (m_border)
	{
		++m_start_x;
		++m_start_y;
	}
	if (!m_title.empty())
		m_start_y += 2;
}

void Window::adjustDimensions(size_t width, size_t height)
{
	if (m_border)
	{
		width -= 2;
		height -= 2;
	}
	if (!m_title.empty())
		height -= 2;
	m_height = height;
	m_width = width;
}

void Window::resize(size_t new_width, size_t new_height)
{
	adjustDimensions(new_width, new_height);
	recreate(m_width, m_height);
}

void Window::refreshBorder() const
{
	if (m_border)
	{
		size_t start_x = getStartX(), start_y = getStarty();
		size_t width = getWidth(), height = getHeight();
		color_set(m_border->pairNumber(), nullptr);
		attron(A_ALTCHARSET);
		// corners
		mvaddch(start_y, start_x, 'l');
		mvaddch(start_y, start_x+width-1, 'k');
		mvaddch(start_y+height-1, start_x, 'm');
		mvaddch(start_y+height-1, start_x+width-1, 'j');
		// lines
		mvhline(start_y, start_x+1, 'q', width-2);
		mvhline(start_y+height-1, start_x+1, 'q', width-2);
		mvvline(start_y+1, start_x, 'x', height-2);
		mvvline(start_y+1, start_x+width-1, 'x', height-2);
		if (!m_title.empty())
		{
			mvaddch(start_y+2, start_x, 't');
			mvaddch(start_y+2, start_x+width-1, 'u');
		}
		attroff(A_ALTCHARSET);
	}
	else
		color_set(m_base_color.pairNumber(), nullptr);
	if (!m_title.empty())
	{
		// clear title line
		mvhline(m_start_y-2, m_start_x, ' ', m_width);
		attron(A_BOLD);
		mvaddstr(m_start_y-2, m_start_x, m_title.c_str());
		attroff(A_BOLD);
		// add separator
		mvhline(m_start_y-1, m_start_x, 0, m_width);
	}
	standend();
	::refresh();
}

void Window::display()
{
	refreshBorder();
	refresh();
}

void Window::refresh()
{
	prefresh(m_window, 0, 0, m_start_y, m_start_x, m_start_y+m_height-1, m_start_x+m_width-1);
}

void Window::clear()
{
	werase(m_window);
	setColor(m_base_color);
}

void Window::bold(bool bold_state) const
{
	(bold_state ? wattron : wattroff)(m_window, A_BOLD);
}

void Window::underline(bool underline_state) const
{
	(underline_state ? wattron : wattroff)(m_window, A_UNDERLINE);
}

void Window::reverse(bool reverse_state) const
{
	(reverse_state ? wattron : wattroff)(m_window, A_REVERSE);
}

void Window::altCharset(bool altcharset_state) const
{
	(altcharset_state ? wattron : wattroff)(m_window, A_ALTCHARSET);
}

void Window::setTimeout(int timeout)
{
	m_window_timeout = timeout;
}

void Window::addFDCallback(int fd, void (*callback)())
{
	m_fds.push_back(std::make_pair(fd, callback));
}

void Window::clearFDCallbacksList()
{
	m_fds.clear();
}

bool Window::FDCallbacksListEmpty() const
{
	return m_fds.empty();
}

Key::Type Window::getInputChar(int key)
{
	if (!m_escape_terminal_sequences || key != Key::Escape)
		return key;
	auto define_mouse_event = [this](int type) {
		switch (type & ~28)
		{
		case 32:
			m_mouse_event.bstate = BUTTON1_PRESSED;
			break;
		case 33:
			m_mouse_event.bstate = BUTTON2_PRESSED;
			break;
		case 34:
			m_mouse_event.bstate = BUTTON3_PRESSED;
			break;
		case 96:
			m_mouse_event.bstate = BUTTON4_PRESSED;
			break;
		case 97:
			m_mouse_event.bstate = BUTTON5_PRESSED;
			break;
		default:
			return Key::None;
		}
		if (type & 4)
			m_mouse_event.bstate |= BUTTON_SHIFT;
		if (type & 8)
			m_mouse_event.bstate |= BUTTON_ALT;
		if (type & 16)
			m_mouse_event.bstate |= BUTTON_CTRL;
		if (m_mouse_event.x < 0 || m_mouse_event.x >= COLS)
			return Key::None;
		if (m_mouse_event.y < 0 || m_mouse_event.y >= LINES)
			return Key::None;
		return Key::Mouse;
	};
	auto get_xterm_modifier_key = [](int ch) {
		Key::Type modifier;
		switch (ch)
		{
		case '2':
			modifier = Key::Shift;
			break;
		case '3':
			modifier = Key::Alt;
			break;
		case '4':
			modifier = Key::Alt | Key::Shift;
			break;
		case '5':
			modifier = Key::Ctrl;
			break;
		case '6':
			modifier = Key::Ctrl | Key::Shift;
			break;
		case '7':
			modifier = Key::Alt | Key::Ctrl;
			break;
		case '8':
			modifier = Key::Alt | Key::Ctrl | Key::Shift;
			break;
		default:
			modifier = Key::None;
		}
		return modifier;
	};
	auto parse_number = [this](int &result) {
		int x;
		while (true)
		{
			x = wgetch(m_window);
			if (!isdigit(x))
				return x;
			result = result*10 + x - '0';
		}
	};
	key = wgetch(m_window);
	switch (key)
	{
	case '\t': // tty
		return Key::Shift | Key::Tab;
	case 'O':
		key = wgetch(m_window);
		switch (key)
		{
		// eterm
		case 'A':
			return Key::Up;
		case 'B':
			return Key::Down;
		case 'C':
			return Key::Right;
		case 'D':
			return Key::Left;
		// terminator
		case 'F':
			return Key::End;
		case 'H':
			return Key::Home;
		// rxvt
		case 'a':
			return Key::Ctrl | Key::Up;
		case 'b':
			return Key::Ctrl | Key::Down;
		case 'c':
			return Key::Ctrl | Key::Right;
		case 'd':
			return Key::Ctrl | Key::Left;
		// xterm
		case 'P':
			return Key::F1;
		case 'Q':
			return Key::F2;
		case 'R':
			return Key::F3;
		case 'S':
			return Key::F4;
		default:
			return Key::None;
		}
	case '[':
		key = wgetch(m_window);
		switch (key)
		{
		case 'a':
			return Key::Shift | Key::Up;
		case 'b':
			return Key::Shift | Key::Down;
		case 'c':
			return Key::Shift | Key::Right;
		case 'd':
			return Key::Shift | Key::Left;
		case 'A':
			return Key::Up;
		case 'B':
			return Key::Down;
		case 'C':
			return Key::Right;
		case 'D':
			return Key::Left;
		case 'F': // xterm
			return Key::End;
		case 'H': // xterm
			return Key::Home;
		case 'M': // standard mouse event
		{
			key = wgetch(m_window);
			int raw_x = wgetch(m_window);
			int raw_y = wgetch(m_window);
			// support coordinates up to 255
			m_mouse_event.x = (raw_x - 33) & 0xff;
			m_mouse_event.y = (raw_y - 33) & 0xff;
			return define_mouse_event(key);
		}
		case 'Z':
			return Key::Shift | Key::Tab;
		case '[': // F1 to F5 in tty
			key = wgetch(m_window);
			switch (key)
			{
			case 'A':
				return Key::F1;
			case 'B':
				return Key::F2;
			case 'C':
				return Key::F3;
			case 'D':
				return Key::F4;
			case 'E':
				return Key::F5;
			default:
				return Key::None;
			}
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
		{
			key -= '0';
			int delim = parse_number(key);
			if (key >= 2 && key <= 8)
			{
				Key::Type modifier;
				switch (delim)
				{
				case '~':
					modifier = Key::Null;
					break;
				case '^':
					modifier = Key::Ctrl;
					break;
				case '$':
					modifier = Key::Shift;
					break;
				case '@':
					modifier = Key::Ctrl | Key::Shift;
					break;
				case ';': // xterm insert/delete/page up/page down
				{
					int local_key = wgetch(m_window);
					modifier = get_xterm_modifier_key(local_key);
					local_key = wgetch(m_window);
					if (local_key != '~' || (key != 2 && key != 3 && key != 5 && key != 6))
						return Key::None;
					break;
				}
				default:
					return Key::None;
				}
				switch (key)
				{
				case 2:
					return modifier | Key::Insert;
				case 3:
					return modifier | Key::Delete;
				case 4:
					return modifier | Key::End;
				case 5:
					return modifier | Key::PageUp;
				case 6:
					return modifier | Key::PageDown;
				case 7:
					return modifier | Key::Home;
				case 8:
					return modifier | Key::End;
				default:
					std::cerr << "Unreachable code, aborting.\n";
					std::terminate();
				}
			}
			switch (delim)
			{
			case '~':
			{
				switch (key)
				{
				case 1: // tty
					return Key::Home;
				case 11:
					return Key::F1;
				case 12:
					return Key::F2;
				case 13:
					return Key::F3;
				case 14:
					return Key::F4;
				case 15:
					return Key::F5;
				case 17: // not a typo
					return Key::F6;
				case 18:
					return Key::F7;
				case 19:
					return Key::F8;
				case 20:
					return Key::F9;
				case 21:
					return Key::F10;
				case 23: // not a typo
					return Key::F11;
				case 24:
					return Key::F12;
				default:
					return Key::None;
				}
			}
			case ';':
				switch (key)
				{
				case 1: // xterm
				{
					key = wgetch(m_window);
					Key::Type modifier = get_xterm_modifier_key(key);
					if (modifier == Key::None)
						return Key::None;
					key = wgetch(m_window);
					switch (key)
					{
					case 'A':
						return modifier | Key::Up;
					case 'B':
						return modifier | Key::Down;
					case 'C':
						return modifier | Key::Right;
					case 'D':
						return modifier | Key::Left;
					case 'F':
						return modifier | Key::End;
					case 'H':
						return modifier | Key::Home;
					default:
						return Key::None;
					}
				}
				default: // urxvt mouse
					m_mouse_event.x = 0;
					delim = parse_number(m_mouse_event.x);
					if (delim != ';')
						return Key::None;
					m_mouse_event.y = 0;
					delim = parse_number(m_mouse_event.y);
					if (delim != 'M')
						return Key::None;
					--m_mouse_event.x;
					--m_mouse_event.y;
					return define_mouse_event(key);
				}
			default:
				return Key::None;
			}
		}
		default:
			return Key::None;
		}
	case ERR:
		return Key::Escape;
	default: // alt + something
	{
		auto key_prim = getInputChar(key);
		if (key_prim != Key::None)
			return Key::Alt | key_prim;
		return Key::None;
	}
	}
}

Key::Type Window::readKey()
{
	Key::Type result;
	// if there are characters in input queue,
	// get them and return immediately.
	if (!m_input_queue.empty())
	{
		result = m_input_queue.front();
		m_input_queue.pop();
		return result;
	}
	
	fd_set fds_read;
	FD_ZERO(&fds_read);
	FD_SET(STDIN_FILENO, &fds_read);
	timeval timeout = { m_window_timeout/1000, (m_window_timeout%1000)*1000 };
	
	int fd_max = STDIN_FILENO;
	for (const auto &fd : m_fds)
	{
		if (fd.first > fd_max)
			fd_max = fd.first;
		FD_SET(fd.first, &fds_read);
	}

	auto tv_addr = m_window_timeout < 0 ? nullptr : &timeout;
	int res = select(fd_max+1, &fds_read, nullptr, nullptr, tv_addr);
	if (res > 0)
	{
		if (FD_ISSET(STDIN_FILENO, &fds_read))
		{
			int key = wgetch(m_window);
			if (key == EOF)
				result = Key::EoF;
			else
				result = getInputChar(key);
		}
		else
			result = Key::None;

		for (const auto &fd : m_fds)
			if (FD_ISSET(fd.first, &fds_read))
				fd.second();
	}
	else
		result = Key::None;
	return result;
}

void Window::pushChar(const Key::Type ch)
{
	m_input_queue.push(ch);
}

std::string Window::prompt(const std::string &base, size_t width, bool encrypted)
{
	std::string result;

	rl::aborted = false;
	rl::w = this;
	getyx(m_window, rl::start_y, rl::start_x);
	rl::width = std::min(m_width-rl::start_x-1, width-1);
	rl::encrypted = encrypted;
	rl::base = base.c_str();

	curs_set(1);
	Mouse::disable();
	m_escape_terminal_sequences = false;
	char *input = readline(nullptr);
	m_escape_terminal_sequences = true;
	Mouse::enable();
	curs_set(0);
	if (input != nullptr)
	{
#ifdef HAVE_READLINE_HISTORY_H
		if (!encrypted && input[0] != 0)
			add_history(input);
#endif // HAVE_READLINE_HISTORY_H
		result = input;
		free(input);
	}

	if (rl::aborted)
		throw PromptAborted(std::move(result));

	return result;
}

void Window::goToXY(int x, int y)
{
	wmove(m_window, y, x);
}

int Window::getX()
{
	return getcurx(m_window);
}

int Window::getY()
{
	return getcury(m_window);
}

bool Window::hasCoords(int &x, int &y)
{
	return wmouse_trafo(m_window, &y, &x, 0);
}

bool Window::runPromptHook(const char *arg, bool *done) const
{
	if (m_prompt_hook)
	{
		bool continue_ = m_prompt_hook(arg);
		if (done != nullptr)
			*done = !continue_;
		return true;
	}
	else
		return false;
}

size_t Window::getWidth() const
{
	if (m_border)
		return m_width+2;
	else
		return m_width;
}

size_t Window::getHeight() const
{
	size_t height = m_height;
	if (m_border)
		height += 2;
	if (!m_title.empty())
		height += 2;
	return height;
}

size_t Window::getStartX() const
{
	if (m_border)
		return m_start_x-1;
	else
		return m_start_x;
}

size_t Window::getStarty() const
{
	size_t starty = m_start_y;
	if (m_border)
		--starty;
	if (!m_title.empty())
		starty -= 2;
	return starty;
}

const std::string &Window::getTitle() const
{
	return m_title;
}

const Color &Window::getColor() const
{
	return m_color;
}

const Border &Window::getBorder() const
{
	return m_border;
}

int Window::getTimeout() const
{
	return m_window_timeout;
}

const MEVENT &Window::getMouseEvent()
{
	return m_mouse_event;
}

void Window::scroll(Scroll where)
{
	idlok(m_window, 1);
	scrollok(m_window, 1);
	switch (where)
	{
		case Scroll::Up:
			wscrl(m_window, 1);
			break;
		case Scroll::Down:
			wscrl(m_window, -1);
			break;
		case Scroll::PageUp:
			wscrl(m_window, m_width);
			break;
		case Scroll::PageDown:
			wscrl(m_window, -m_width);
			break;
		default:
			break;
	}
	idlok(m_window, 0);
	scrollok(m_window, 0);
}


Window &Window::operator<<(const Color &c)
{
	if (c.isDefault())
	{
		while (!m_color_stack.empty())
			m_color_stack.pop();
		setColor(m_base_color);
	}
	else if (c.isEnd())
	{
		if (!m_color_stack.empty())
			m_color_stack.pop();
		if (!m_color_stack.empty())
			setColor(m_color_stack.top());
		else
			setColor(m_base_color);
	}
	else
	{
		if (c.currentBackground())
		{
			short background = m_color.isDefault()
				? Color::transparent
				: m_color.background();
			Color cc = Color(c.foreground(), background);
			setColor(cc);
			m_color_stack.push(cc);
		}
		else
		{
			setColor(c);
			m_color_stack.push(c);
		}
	}
	return *this;
}

Window &Window::operator<<(Format format)
{
	auto increase_flag = [](Window &w, int &flag, auto set) {
		++flag;
		(w.*set)(true);
	};
	auto decrease_flag = [](Window &w, int &flag, auto set) {
		if (flag > 0)
		{
			--flag;
			if (flag == 0)
				(w.*set)(false);
		}
	};
	switch (format)
	{
		case Format::Bold:
			increase_flag(*this, m_bold_counter, &Window::bold);
			break;
		case Format::NoBold:
			decrease_flag(*this, m_bold_counter, &Window::bold);
			break;
		case Format::Underline:
			increase_flag(*this, m_underline_counter, &Window::underline);
			break;
		case Format::NoUnderline:
			decrease_flag(*this, m_underline_counter, &Window::underline);
			break;
		case Format::Reverse:
			increase_flag(*this, m_reverse_counter, &Window::reverse);
			break;
		case Format::NoReverse:
			decrease_flag(*this, m_reverse_counter, &Window::reverse);
			break;
		case Format::AltCharset:
			increase_flag(*this, m_alt_charset_counter, &Window::altCharset);
			break;
		case Format::NoAltCharset:
			decrease_flag(*this, m_alt_charset_counter, &Window::altCharset);
			break;
	}
	return *this;
}

Window &Window::operator<<(TermManip tm)
{
	switch (tm)
	{
		case TermManip::ClearToEOL:
		{
			auto x = getX(), y = getY();
			mvwhline(m_window, y, x, ' ', m_width-x);
			goToXY(x, y);
		}
		break;
	}
	return *this;
}

Window &Window::operator<<(const XY &coords)
{
	goToXY(coords.x, coords.y);
	return *this;
}

Window &Window::operator<<(const char *s)
{
	waddstr(m_window, s);
	return *this;
}

Window &Window::operator<<(char c)
{
	// Might cause problem similar to
	// https://github.com/arybczak/ncmpcpp/issues/21, enable for testing as the
	// code in the ticket supposed to be culprit was rewritten.
	waddnstr(m_window, &c, 1);
	//wprintw(m_window, "%c", c);
	return *this;
}

Window &Window::operator<<(const wchar_t *ws)
{
	waddwstr(m_window, ws);
	return *this;
}

Window &Window::operator<<(wchar_t wc)
{
	waddnwstr(m_window, &wc, 1);
	return *this;
}

Window &Window::operator<<(int i)
{
	wprintw(m_window, "%d", i);
	return *this;
}

Window &Window::operator<<(double d)
{
	wprintw(m_window, "%f", d);
	return *this;
}

Window &Window::operator<<(const std::string &s)
{
	waddnstr(m_window, s.c_str(), s.length());
	return *this;
}

Window &Window::operator<<(const std::wstring &ws)
{
	waddnwstr(m_window, ws.c_str(), ws.length());
	return *this;
}

Window &Window::operator<<(size_t s)
{
	wprintw(m_window, "%zu", s);
	return *this;
}

}
