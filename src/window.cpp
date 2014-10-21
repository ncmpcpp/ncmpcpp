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
		if (w->runGetStringHelper(rl_line_buffer, &done))
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

	char pt = rl_line_buffer[rl_point];
	rl_line_buffer[rl_point] = 0;
	wchar_t pre_pos[rl_point+1];
	pre_pos[mbstowcs(pre_pos, rl_line_buffer, rl_point)] = 0;
	rl_line_buffer[rl_point] = pt;

	int pos = wcswidth(pre_pos, rl_point);
	if (pos < 0)
		pos = rl_point;

	mvwhline(w->raw(), start_y, start_x, ' ', width+1);
	w->goToXY(start_x, start_y);
	if (size_t(pos) <= width)
	{
		print_string(pre_pos, pos);

		wchar_t post_pos[rl_end-rl_point+1];
		post_pos[mbstowcs(post_pos, rl_line_buffer+rl_point, rl_end-rl_point)] = 0;

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

namespace NC {//

std::ostream &operator<<(std::ostream &os, Color c)
{
	switch (c)
	{
		case Color::Default:
			os << "default";
			break;
		case Color::Black:
			os << "black";
			break;
		case Color::Red:
			os << "red";
			break;
		case Color::Green:
			os << "green";
			break;
		case Color::Yellow:
			os << "yellow";
			break;
		case Color::Blue:
			os << "blue";
			break;
		case Color::Magenta:
			os << "magenta";
			break;
		case Color::Cyan:
			os << "cyan";
			break;
		case Color::White:
			os << "white";
			break;
		case Color::End:
			os << "color_end";
			break;
	}
	return os;
}

std::istream &operator>>(std::istream &is, Color &c)
{
	std::string sc;
	is >> sc;
	if (sc == "default")
		c = Color::Default;
	else if (sc == "black")
		c = Color::Black;
	else if (sc == "red")
		c = Color::Red;
	else if (sc == "green")
		c = Color::Green;
	else if (sc == "yellow")
		c = Color::Yellow;
	else if (sc == "blue")
		c = Color::Blue;
	else if (sc == "magenta")
		c = Color::Magenta;
	else if (sc == "cyan")
		c = Color::Cyan;
	else if (sc == "white")
		c = Color::White;
	else if (sc == "color_end")
		c = Color::End;
	else
		is.setstate(std::ios::failbit);
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

void initScreen(GNUC_UNUSED const char *window_title, bool enable_colors)
{
	const int ColorsTable[] =
	{
		COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
		COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
	};
#	ifdef XCURSES
	Xinitscr(1, const_cast<char **>(&window_title));
#	else
	initscr();
#	endif // XCURSES
	if (has_colors() && enable_colors)
	{
		start_color();
		use_default_colors();
		int num = 1;
#		ifdef USE_PDCURSES
		int i = 0;
#		else
		int i = -1;
#		endif // USE_PDCURSES
		for (; i < 8; ++i)
			for (int j = 0; j < 8; ++j)
				init_pair(num++, ColorsTable[j], i < 0 ? i : ColorsTable[i]);
	}
	raw();
	nonl();
	noecho();
	curs_set(0);

	rl_initialize();
	// disable autocompletion
	rl_attempted_completion_function = [](const char *, int, int) -> char ** {
		rl_attempted_completion_over = 1;
		return nullptr;
	};
	// do not catch signals
	rl_catch_signals = 0;
	// overwrite readline callbacks
	rl_getc_function = rl::read_key;
	rl_redisplay_function = rl::display_string;
	rl_startup_hook = rl::add_base;
}

void destroyScreen()
{
	curs_set(1);
	endwin();
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
		m_bg_color(Color::Default),
		m_base_color(color),
		m_base_bg_color(Color::Default),
		m_border(border),
		m_get_string_helper(0),
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
, m_bg_color(rhs.m_bg_color)
, m_base_color(rhs.m_base_color)
, m_base_bg_color(rhs.m_base_bg_color)
, m_border(rhs.m_border)
, m_get_string_helper(rhs.m_get_string_helper)
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
, m_bg_color(rhs.m_bg_color)
, m_base_color(rhs.m_base_color)
, m_base_bg_color(rhs.m_base_bg_color)
, m_border(rhs.m_border)
, m_get_string_helper(rhs.m_get_string_helper)
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
	std::swap(m_bg_color, rhs.m_bg_color);
	std::swap(m_base_color, rhs.m_base_color);
	std::swap(m_base_bg_color, rhs.m_base_bg_color);
	std::swap(m_border, rhs.m_border);
	std::swap(m_get_string_helper, rhs.m_get_string_helper);
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

void Window::setColor(Color fg, Color bg)
{
	if (fg == Color::Default)
		fg = m_base_color;
	
	if (fg != Color::Default)
		wattron(m_window, COLOR_PAIR(int(bg)*8+int(fg)));
	else
		wattroff(m_window, COLOR_PAIR(int(m_color)));
	m_color = fg;
	m_bg_color = bg;
}

void Window::setBaseColor(Color fg, Color bg)
{
	m_base_color = fg;
	m_base_bg_color = bg;
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
	setColor(m_color, m_bg_color);
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

void Window::showBorder() const
{
	if (m_border != Border::None)
	{
		::refresh();
		prefresh(m_border_window, 0, 0, getStarty(), getStartX(), m_start_y+m_height, m_start_x+m_width);
	}
	if (!m_title.empty())
	{
		if (m_border != Border::None)
			attron(COLOR_PAIR(int(m_border)));
		else
			attron(COLOR_PAIR(int(m_base_color)));
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
	showBorder();
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

std::string Window::getString(const std::string &base, size_t width, bool encrypted)
{
	rl::w = this;
	getyx(m_window, rl::start_y, rl::start_x);
	rl::width = width;
	rl::encrypted = encrypted;
	rl::base = base.c_str();

	width--;
	if (width == size_t(-1))
		rl::width = m_width-rl::start_x-1;
	else
		rl::width = width;

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

bool Window::runGetStringHelper(const char *arg, bool *done) const
{
	if (m_get_string_helper)
	{
		bool continue_ = m_get_string_helper(arg);
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

Color Window::getColor() const
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


Window &Window::operator<<(Colors colors)
{
	if (colors.fg == Color::End || colors.bg == Color::End)
	{
		*this << Color::End;
		return *this;
	}
	m_color_stack.push(colors);
	setColor(colors.fg, colors.bg);
	return *this;
}

Window &Window::operator<<(Color color)
{
	switch (color)
	{
		case Color::Default:
			while (!m_color_stack.empty())
				m_color_stack.pop();
			setColor(m_base_color, m_base_bg_color);
			break;
		case Color::End:
			if (!m_color_stack.empty())
				m_color_stack.pop();
			if (!m_color_stack.empty())
				setColor(m_color_stack.top().fg, m_color_stack.top().bg);
			else
				setColor(m_base_color, m_base_bg_color);
			break;
		default:
			Color bg;
			if (m_color_stack.empty())
				bg = m_bg_color;
			else
				bg = m_color_stack.top().bg;
			m_color_stack.push(Colors(color, bg));
			setColor(m_color_stack.top().fg, m_color_stack.top().bg);
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

Window &Window::operator<<(int (*f)(WINDOW *))
{
	f(m_window);
	return *this;
}

Window &Window::operator<<(XY coords)
{
	goToXY(coords.x, coords.y);
	return *this;
}

Window &Window::operator<<(const char *s)
{
	for (const char *c = s; *c != '\0'; ++c)
		wprintw(m_window, "%c", *c);
	return *this;
}

Window &Window::operator<<(char c)
{
	wprintw(m_window, "%c", c);
	return *this;
}

Window &Window::operator<<(const wchar_t *ws)
{
	for (const wchar_t *wc = ws; *wc != L'\0'; ++wc)
		wprintw(m_window, "%lc", *wc);
	return *this;
}

Window &Window::operator<<(wchar_t wc)
{
	wprintw(m_window, "%lc", wc);
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
	// for some reason passing whole string at once with "%s" doesn't work
	// (string is cut in the middle, probably due to limitation of ncurses'
	// internal buffer?), so we need to pass characters in a loop.
	for (auto it = s.begin(); it != s.end(); ++it)
		wprintw(m_window, "%c", *it);
	return *this;
}

Window &Window::operator<<(const std::wstring &ws)
{
	for (auto it = ws.begin(); it != ws.end(); ++it)
		wprintw(m_window, "%lc", *it);
	return *this;
}

Window &Window::operator<<(size_t s)
{
	wprintw(m_window, "%zu", s);
	return *this;
}

}
