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

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>

#ifdef WIN32
# include <winsock.h>
#else
# include <sys/select.h>
# include <unistd.h>
#endif

#include "error.h"
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
			if (done)
			{
				rl_done = 1;
				return 0;
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
}

namespace NC {

Color Color::Default(0, -1, true, false);
Color Color::Black(COLOR_BLACK, -1);
Color Color::Red(COLOR_RED, -1);
Color Color::Green(COLOR_GREEN, -1);
Color Color::Yellow(COLOR_YELLOW, -1);
Color Color::Blue(COLOR_BLUE, -1);
Color Color::Magenta(COLOR_MAGENTA, -1);
Color Color::Cyan(COLOR_CYAN, -1);
Color Color::White(COLOR_WHITE, -1);
Color Color::End(0, 0, false, true);

std::ostream &operator<<(std::ostream &os, const Color &c)
{
	if (c.isDefault())
		os << "default";
	else if (c == Color::Black)
		os << "black";
	else if (c == Color::Red)
		os << "red";
	else if (c == Color::Green)
		os << "green";
	else if (c == Color::Yellow)
		os << "yellow";
	else if (c == Color::Blue)
		os << "blue";
	else if (c == Color::Magenta)
		os << "magenta";
	else if (c == Color::Cyan)
		os << "cyan";
	else if (c == Color::White)
		os << "white";
	else if (c.isEnd())
		os << "end";
	else
		os << c.foreground() << "_" << c.background();
	return os;
}

std::istream &operator>>(std::istream &is, Color &c)
{
	auto get_single_color = [](const std::string &s) {
		short result = -1;
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
		else if (std::all_of(s.begin(), s.end(), isdigit))
		{
			result = atoi(s.c_str());
			if (result < 1 || result > 256)
				result = -1;
			else
				--result;
		}
		return result;
	};
	std::string sc;
	is >> sc;
	if (sc == "default")
			c = Color::Default;
	else if (sc == "end")
		c = Color::End;
	else
	{
		short value = get_single_color(sc);
		if (value != -1)
			c = Color(value);
		else
		{
			size_t underscore = sc.find('_');
			if (underscore != std::string::npos)
			{
				short fg = get_single_color(sc.substr(0, underscore));
				short bg = get_single_color(sc.substr(underscore+1));
				if (fg != -1 && bg != -1)
					c = Color(fg, bg);
				else
					is.setstate(std::ios::failbit);
			}
			else
				is.setstate(std::ios::failbit);
		}
	}
	return is;
}

std::ostream &operator<<(std::ostream &os, Format f)
{
	switch (f)
	{
		case Format::None:
			os << "none";
			break;
		case Format::Bold:
			os << "bold";
			break;
		case Format::NoBold:
			os << "bold";
			break;
		case Format::Underline:
			os << "underline";
			break;
		case Format::NoUnderline:
			os << "no_underline";
			break;
		case Format::Reverse:
			os << "reverse";
			break;
		case Format::NoReverse:
			os << "no_reverse";
			break;
		case Format::AltCharset:
			os << "alt_charset";
			break;
		case Format::NoAltCharset:
			os << "no_alt_charset";
			break;
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, Border b)
{
	switch (b)
	{
		case Border::None:
			os << "none";
			break;
		case Border::Black:
			os << "black";
			break;
		case Border::Red:
			os << "red";
			break;
		case Border::Green:
			os << "green";
			break;
		case Border::Yellow:
			os << "yellow";
			break;
		case Border::Blue:
			os << "blue";
			break;
		case Border::Magenta:
			os << "magenta";
			break;
		case Border::Cyan:
			os << "cyan";
			break;
		case Border::White:
			os << "white";
			break;
	}
	return os;
}

std::istream &operator>>(std::istream &is, Border &b)
{
	std::string sb;
	is >> sb;
	if (sb == "none")
		b = Border::None;
	else if (sb == "black")
		b = Border::Black;
	else if (sb == "red")
		b = Border::Red;
	else if (sb == "green")
		b = Border::Green;
	else if (sb == "yellow")
		b = Border::Yellow;
	else if (sb == "blue")
		b = Border::Blue;
	else if (sb == "magenta")
		b = Border::Magenta;
	else if (sb == "cyan")
		b = Border::Cyan;
	else if (sb == "white")
		b = Border::White;
	else
		is.setstate(std::ios::failbit);
	return is;
}

std::ostream &operator<<(std::ostream &os, Scroll s)
{
	switch (s)
	{
		case Scroll::Up:
			os << "scroll_up";
			break;
		case Scroll::Down:
			os << "scroll_down";
			break;
		case Scroll::PageUp:
			os << "scroll_page_up";
			break;
		case Scroll::PageDown:
			os << "scroll_page_down";
			break;
		case Scroll::Home:
			os << "scroll_home";
			break;
		case Scroll::End:
			os << "scroll_end";
			break;
	}
	return os;
}

void initScreen(bool enable_colors)
{
	initscr();
	if (has_colors() && enable_colors)
	{
		start_color();
		use_default_colors();
		int npair = 1;
		for (int bg = -1; bg < COLORS; ++bg)
		{
			for (int fg = 0; npair < COLOR_PAIRS && fg < COLORS; ++fg, ++npair)
				init_pair(npair, fg, bg);
		}
	}
	raw();
	nonl();
	noecho();
	curs_set(0);

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
	rl_bind_key(KEY_CTRL_C, abort_prompt);
	rl_bind_key(KEY_CTRL_G, abort_prompt);
	// do not change the state of the terminal
	rl_prep_term_function = nullptr;
	rl_deprep_term_function = nullptr;
	// do not catch signals
	rl_catch_signals = 0;
	// overwrite readline callbacks
	rl_getc_function = rl::read_key;
	rl_redisplay_function = rl::display_string;
	rl_startup_hook = rl::add_base;
	// initialize readline (needed, otherwise
	// we get segmentation fault on SIGWINCH).
	rl_initialize();
}

void destroyScreen()
{
	curs_set(1);
	endwin();
}

int Color::pairNumber() const
{
	int result;
	if (isDefault())
		result = 0;
	else if (isEnd())
		throw std::logic_error("'end' doesn't have a corresponding pair number");
	else
	{
		result = background();
		result *= COLORS;
		result += foreground();
	}
	return result;
}

Window::Window(size_t startx,
		size_t starty,
		size_t width,
		size_t height,
		const std::string &title,
		Color color,
		Border border)
		: m_window(0),
		m_border_window(0),
		m_start_x(startx),
		m_start_y(starty),
		m_width(width),
		m_height(height),
		m_window_timeout(-1),
		m_color(color),
		m_base_color(color),
		m_border(border),
		m_prompt_hook(0),
		m_title(title),
		m_bold_counter(0),
		m_underline_counter(0),
		m_reverse_counter(0),
		m_alt_charset_counter(0)
{
	if (m_start_x > size_t(COLS)
	||  m_start_y > size_t(LINES)
	||  m_width+m_start_x > size_t(COLS)
	||  m_height+m_start_y > size_t(LINES))
		FatalError("Constructed window is bigger than terminal size");
	
	if (m_border != Border::None)
	{
		m_border_window = newpad(m_height, m_width);
		wattron(m_border_window, COLOR_PAIR(int(m_border)));
		box(m_border_window, 0, 0);
		m_start_x++;
		m_start_y++;
		m_width -= 2;
		m_height -= 2;
	}
	if (!m_title.empty())
	{
		m_start_y += 2;
		m_height -= 2;
	}
	
	m_window = newpad(m_height, m_width);
	
	setColor(m_color);
	keypad(m_window, 1);
}

Window::Window(const Window &rhs)
: m_window(dupwin(rhs.m_window))
, m_border_window(dupwin(rhs.m_border_window))
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
, m_bold_counter(rhs.m_bold_counter)
, m_underline_counter(rhs.m_underline_counter)
, m_reverse_counter(rhs.m_reverse_counter)
, m_alt_charset_counter(rhs.m_alt_charset_counter)
{
}

Window::Window(Window &&rhs)
: m_window(rhs.m_window)
, m_border_window(rhs.m_border_window)
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
, m_bold_counter(rhs.m_bold_counter)
, m_underline_counter(rhs.m_underline_counter)
, m_reverse_counter(rhs.m_reverse_counter)
, m_alt_charset_counter(rhs.m_alt_charset_counter)
{
	rhs.m_window = 0;
	rhs.m_border_window = 0;
}

Window &Window::operator=(Window rhs)
{
	std::swap(m_window, rhs.m_window);
	std::swap(m_border_window, rhs.m_border_window);
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
	std::swap(m_bold_counter, rhs.m_bold_counter);
	std::swap(m_underline_counter, rhs.m_underline_counter);
	std::swap(m_reverse_counter, rhs.m_reverse_counter);
	std::swap(m_alt_charset_counter, rhs.m_alt_charset_counter);
	return *this;
}

Window::~Window()
{
	delwin(m_window);
	delwin(m_border_window);
}

void Window::setColor(Color c)
{
	if (c.isDefault())
		c = m_base_color;
	if (c != Color::Default)
		wcolor_set(m_window, c.pairNumber(), nullptr);
	else
		wcolor_set(m_window, m_base_color.pairNumber(), nullptr);
	m_color = std::move(c);
}

void Window::setBaseColor(Color c)
{
	m_base_color = std::move(c);
}

void Window::setBorder(Border border)
{
	if (border == Border::None && m_border != Border::None)
	{
		delwin(m_border_window);
		m_start_x--;
		m_start_y--;
		m_height += 2;
		m_width += 2;
		recreate(m_width, m_height);
	}
	else if (border != Border::None && m_border == Border::None)
	{
		m_border_window = newpad(m_height, m_width);
		wattron(m_border_window, COLOR_PAIR(int(border)));
		box(m_border_window,0,0);
		m_start_x++;
		m_start_y++;
		m_height -= 2;
		m_width -= 2;
		recreate(m_width, m_height);
	}
	else
	{
		wattron(m_border_window,COLOR_PAIR(int(border)));
		box(m_border_window, 0, 0);
	}
	m_border = border;
}

void Window::setTitle(const std::string &new_title)
{
	if (m_title == new_title)
	{
		return;
	}
	else if (!new_title.empty() && m_title.empty())
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
	setTimeout(m_window_timeout);
	setColor(m_color);
	keypad(m_window, 1);
}

void Window::moveTo(size_t new_x, size_t new_y)
{
	m_start_x = new_x;
	m_start_y = new_y;
	if (m_border != Border::None)
	{
		m_start_x++;
		m_start_y++;
	}
	if (!m_title.empty())
		m_start_y += 2;
}

void Window::adjustDimensions(size_t width, size_t height)
{
	if (m_border != Border::None)
	{
		delwin(m_border_window);
		m_border_window = newpad(height, width);
		wattron(m_border_window, COLOR_PAIR(int(m_border)));
		box(m_border_window, 0, 0);
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
	if (m_border != Border::None)
		prefresh(m_border_window, 0, 0, getStarty(), getStartX(), m_start_y+m_height, m_start_x+m_width);
	if (!m_title.empty())
	{
		if (m_border != Border::None)
			attron(COLOR_PAIR(int(m_border)));
		else
			wcolor_set(m_border_window, m_base_color.pairNumber(), nullptr);
		mvhline(m_start_y-1, m_start_x, 0, m_width);
		attron(A_BOLD);
		mvhline(m_start_y-2, m_start_x, 32, m_width); // clear title line
		mvaddstr(m_start_y-2, m_start_x, m_title.c_str());
		attroff(COLOR_PAIR(int(m_border)) | A_BOLD);
	}
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
	if (timeout != m_window_timeout)
	{
		m_window_timeout = timeout;
		wtimeout(m_window, timeout);
	}
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

int Window::readKey()
{
	int result;
	// if there are characters in input queue, get them and
	// return immediately.
	if (!m_input_queue.empty())
	{
		result = m_input_queue.front();
		m_input_queue.pop();
		return result;
	}
	// in pdcurses polling stdin doesn't work, so we can't poll
	// both stdin and other file descriptors in one select. the
	// workaround is to set the timeout of select to 0, poll
	// other file descriptors and then wait for stdin input with
	// the given timeout. unfortunately, this results in delays
	// since ncmpcpp doesn't see that data arrived while waiting
	// for input from stdin, but it seems there is no better option.
	
	fd_set fdset;
	FD_ZERO(&fdset);
#	if !defined(USE_PDCURSES)
	FD_SET(STDIN_FILENO, &fdset);
	timeval timeout = { m_window_timeout/1000, (m_window_timeout%1000)*1000 };
#	else
	timeval timeout = { 0, 0 };
#	endif
	
	int fd_max = STDIN_FILENO;
	for (FDCallbacks::const_iterator it = m_fds.begin(); it != m_fds.end(); ++it)
	{
		if (it->first > fd_max)
			fd_max = it->first;
		FD_SET(it->first, &fdset);
	}
	
	if (select(fd_max+1, &fdset, 0, 0, m_window_timeout < 0 ? 0 : &timeout) > 0)
	{
#		if !defined(USE_PDCURSES)
		result = FD_ISSET(STDIN_FILENO, &fdset) ? wgetch(m_window) : ERR;
#		endif // !USE_PDCURSES
		for (FDCallbacks::const_iterator it = m_fds.begin(); it != m_fds.end(); ++it)
			if (FD_ISSET(it->first, &fdset))
				it->second();
	}
#	if !defined(USE_PDCURSES)
	else
		result = ERR;
#	else
	result = wgetch(m_window);
#	endif
	return result;
}

void Window::pushChar(int ch)
{
	m_input_queue.push(ch);
}

std::string Window::prompt(const std::string &base, size_t width, bool encrypted)
{
	rl::aborted = false;
	rl::w = this;
	getyx(m_window, rl::start_y, rl::start_x);
	rl::width = std::min(m_width-rl::start_x-1, width-1);
	rl::encrypted = encrypted;
	rl::base = base.c_str();

	mmask_t oldmask;
	std::string result;

	curs_set(1);
	keypad(m_window, 0);
	mousemask(0, &oldmask);
	char *input = readline(nullptr);
	mousemask(oldmask, nullptr);
	keypad(m_window, 1);
	curs_set(0);
	if (input != nullptr)
	{
		if (input[0] != 0)
			add_history(input);
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
#	ifndef USE_PDCURSES
	return wmouse_trafo(m_window, &y, &x, 0);
#	else
	// wmouse_trafo is broken in pdcurses, use our own implementation
	size_t u_x = x, u_y = y;
	if (u_x >= m_start_x && u_x < m_start_x+m_width
	&&  u_y >= m_start_y && u_y < m_start_y+m_height)
	{
		x -= m_start_x;
		y -= m_start_y;
		return true;
	}
	return false;
#	endif
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
	if (m_border != Border::None)
		return m_width+2;
	else
		return m_width;
}

size_t Window::getHeight() const
{
	size_t height = m_height;
	if (m_border != Border::None)
		height += 2;
	if (!m_title.empty())
		height += 2;
	return height;
}

size_t Window::getStartX() const
{
	if (m_border != Border::None)
		return m_start_x-1;
	else
		return m_start_x;
}

size_t Window::getStarty() const
{
	size_t starty = m_start_y;
	if (m_border != Border::None)
		starty--;
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

Border Window::getBorder() const
{
	return m_border;
}

int Window::getTimeout() const
{
	return m_window_timeout;
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
		setColor(c);
		m_color_stack.push(c);
	}
	return *this;
}

Window &Window::operator<<(Format format)
{
	switch (format)
	{
		case Format::None:
			bold((m_bold_counter = 0));
			reverse((m_reverse_counter = 0));
			altCharset((m_alt_charset_counter = 0));
			break;
		case Format::Bold:
			bold(++m_bold_counter);
			break;
		case Format::NoBold:
			if (--m_bold_counter <= 0)
				bold((m_bold_counter = 0));
			break;
		case Format::Underline:
			underline(++m_underline_counter);
			break;
		case Format::NoUnderline:
			if (--m_underline_counter <= 0)
				underline((m_underline_counter = 0));
			break;
		case Format::Reverse:
			reverse(++m_reverse_counter);
			break;
		case Format::NoReverse:
			if (--m_reverse_counter <= 0)
				reverse((m_reverse_counter = 0));
			break;
		case Format::AltCharset:
			altCharset(++m_alt_charset_counter);
			break;
		case Format::NoAltCharset:
			if (--m_alt_charset_counter <= 0)
				altCharset((m_alt_charset_counter = 0));
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
	waddch(m_window, c);
	return *this;
}

Window &Window::operator<<(const wchar_t *ws)
{
#	ifdef NCMPCPP_UNICODE
	waddwstr(m_window, ws);
#	else
	wprintw(m_window, "%ls", ws);
#	endif // NCMPCPP_UNICODE
	return *this;
}

Window &Window::operator<<(wchar_t wc)
{
#	ifdef NCMPCPP_UNICODE
	waddnwstr(m_window, &wc, 1);
#	else
	wprintw(m_window, "%lc", wc);
#	endif // NCMPCPP_UNICODE
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
#	ifdef NCMPCPP_UNICODE
	waddnwstr(m_window, ws.c_str(), ws.length());
#	else
	wprintw(m_window, "%lc", ws.c_str());
#	endif // NCMPCPP_UNICODE
	return *this;
}

Window &Window::operator<<(size_t s)
{
	wprintw(m_window, "%zu", s);
	return *this;
}

}
