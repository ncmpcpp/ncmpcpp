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

#include <cstring>
#include <cstdlib>

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

namespace NC {//

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
	noecho();
	cbreak();
	curs_set(0);
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
		m_bg_color(clDefault),
		m_base_color(color),
		m_base_bg_color(clDefault),
		m_border(border),
		m_get_string_helper(0),
		m_title(title),
		m_history(0),
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
	
	if (m_border != brNone)
	{
		m_border_window = newpad(m_height, m_width);
		wattron(m_border_window, COLOR_PAIR(m_border));
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

Window::Window(const Window &w) : m_window(dupwin(w.m_window)),
				m_border_window(dupwin(w.m_border_window)),
				m_start_x(w.m_start_x),
				m_start_y(w.m_start_y),
				m_width(w.m_width),
				m_height(w.m_height),
				m_window_timeout(w.m_window_timeout),
				m_color(w.m_color),
				m_bg_color(w.m_bg_color),
				m_base_color(w.m_base_color),
				m_base_bg_color(w.m_base_bg_color),
				m_border(w.m_border),
				m_get_string_helper(w.m_get_string_helper),
				m_title(w.m_title),
				m_color_stack(w.m_color_stack),
				m_history(w.m_history),
				m_bold_counter(w.m_bold_counter),
				m_underline_counter(w.m_underline_counter),
				m_reverse_counter(w.m_reverse_counter),
				m_alt_charset_counter(w.m_alt_charset_counter)
{
}

Window::~Window()
{
	delwin(m_window);
	delwin(m_border_window);
}

void Window::setColor(Color fg, Color bg)
{
	if (fg == clDefault)
		fg = m_base_color;
	
	if (fg != clDefault)
		wattron(m_window, COLOR_PAIR(bg*8+fg));
	else
		wattroff(m_window, COLOR_PAIR(m_color));
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
	if (border == brNone && m_border != brNone)
	{
		delwin(m_border_window);
		m_start_x--;
		m_start_y--;
		m_height += 2;
		m_width += 2;
		recreate(m_width, m_height);
	}
	else if (border != brNone && m_border == brNone)
	{
		m_border_window = newpad(m_height, m_width);
		wattron(m_border_window, COLOR_PAIR(border));
		box(m_border_window,0,0);
		m_start_x++;
		m_start_y++;
		m_height -= 2;
		m_width -= 2;
		recreate(m_width, m_height);
	}
	else
	{
		wattron(m_border_window,COLOR_PAIR(border));
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

void Window::createHistory()
{
	if (!m_history)
		m_history = new std::list<std::wstring>;
}

void Window::deleteHistory()
{
	delete m_history;
	m_history = 0;
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
	if (m_border != brNone)
	{
		m_start_x++;
		m_start_y++;
	}
	if (!m_title.empty())
		m_start_y += 2;
}

void Window::adjustDimensions(size_t width, size_t height)
{
	if (m_border != brNone)
	{
		delwin(m_border_window);
		m_border_window = newpad(height, width);
		wattron(m_border_window, COLOR_PAIR(m_border));
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
	if (m_border != brNone)
	{
		::refresh();
		prefresh(m_border_window, 0, 0, getStarty(), getStartX(), m_start_y+m_height, m_start_x+m_width);
	}
	if (!m_title.empty())
	{
		if (m_border != brNone)
			attron(COLOR_PAIR(m_border));
		else
			attron(COLOR_PAIR(m_base_color));
		mvhline(m_start_y-1, m_start_x, 0, m_width);
		attron(A_BOLD);
		mvhline(m_start_y-2, m_start_x, 32, m_width); // clear title line
		mvaddstr(m_start_y-2, m_start_x, m_title.c_str());
		attroff(COLOR_PAIR(m_border) | A_BOLD);
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
	m_window_timeout = timeout;
	wtimeout(m_window, timeout);
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

std::string Window::getString(const std::string &base, size_t length_, size_t width, bool encrypted)
{
	int input;
	size_t beginning, maxbeginning, minx, x, real_x, y, maxx, real_maxx;
	
	getyx(m_window, y, x);
	minx = real_maxx = maxx = real_x = x;
	
	width--;
	if (width == size_t(-1))
		width = m_width-x-1;
	
	curs_set(1);
	
	std::wstring wbase = ToWString(base);
	std::wstring *tmp = &wbase;
	std::list<std::wstring>::iterator history_it = m_history->end();
	
	std::string tmp_in;
	wchar_t wc_in;
	bool gotoend = 1;
	bool block_scrolling = 0;
	
	// disable scrolling if wide chars are used
	for (std::wstring::const_iterator it = tmp->begin(); it != tmp->end(); ++it)
		if (wcwidth(*it) > 1)
			block_scrolling = 1;
	
	beginning = -1;
	
	do
	{
		if (tmp->empty())
			block_scrolling = 0;
		
		maxbeginning = block_scrolling ? 0 : (tmp->length() < width ? 0 : tmp->length()-width);
		maxx = minx + (wideLength(*tmp) < width ? wideLength(*tmp) : width);
		
		real_maxx = minx + (tmp->length() < width ? tmp->length() : width);
		
		if (beginning > maxbeginning)
			beginning = maxbeginning;
		
		if (gotoend)
		{
			size_t real_real_maxx = minx;
			size_t biggest_x = minx+width;
				
			if (block_scrolling && maxx >= biggest_x)
			{
				size_t i = 0;
				for (std::wstring::const_iterator it = tmp->begin(); i < width; ++it, ++real_real_maxx)
					i += wcwidth(*it);
			}
			else
				real_real_maxx = real_maxx;
				
			real_x = real_real_maxx;
			x = block_scrolling ? (maxx > biggest_x ? biggest_x : maxx) : maxx;
			beginning = maxbeginning;
			gotoend = 0;
		}
		
		mvwhline(m_window, y, minx, ' ', width+1);
		
		if (!encrypted)
			mvwprintw(m_window, y, minx, "%ls", tmp->substr(beginning, width+1).c_str());
		else
			mvwhline(m_window, y, minx, '*', maxx-minx);
		
		if (m_get_string_helper)
			m_get_string_helper(*tmp);
		
		wmove(m_window, y, x);
		prefresh(m_window, 0, 0, m_start_y, m_start_x, m_start_y+m_height-1, m_start_x+m_width-1);
		input = readKey();
		
		switch (input)
		{
			case ERR:
			case KEY_MOUSE:
				break;
			case KEY_UP:
				if (m_history && !encrypted && history_it != m_history->begin())
				{
					while (--history_it != m_history->begin())
						if (!history_it->empty())
							break;
					tmp = &*history_it;
					gotoend = 1;
				}
				break;
			case KEY_DOWN:
				if (m_history && !encrypted && history_it != m_history->end())
				{
					while (++history_it != m_history->end())
						if (!history_it->empty())
							break;
					tmp = &(history_it == m_history->end() ? wbase : *history_it);
					gotoend = 1;
				}
				break;
			case KEY_RIGHT:
			{
				if (x < maxx)
				{
					real_x++;
					x += std::max(wcwidth((*tmp)[beginning+real_x-minx-1]), 1);
				}
				else if (beginning < maxbeginning)
					beginning++;
				break;
			}
			case KEY_CTRL_H:
			case KEY_BACKSPACE:
			case KEY_BACKSPACE_2:
			{
				if (x <= minx && !beginning)
					break;
			}
			case KEY_LEFT:
			{
				if (x > minx)
				{
					real_x--;
					x -= std::max(wcwidth((*tmp)[beginning+real_x-minx]), 1);
				}
				else if (beginning > 0)
					beginning--;
				if (input != KEY_CTRL_H && input != KEY_BACKSPACE && input != KEY_BACKSPACE_2)
					break; // backspace = left & delete.
			}
			case KEY_DC:
			{
				if ((real_x-minx)+beginning == tmp->length())
					break;
				tmp->erase(tmp->begin()+(real_x-minx)+beginning);
				if (beginning && beginning == maxbeginning && real_x < maxx)
				{
					real_x++;
					x++;
				}
				break;
			}
			case KEY_HOME:
			{
				real_x = x = minx;
				beginning = 0;
				break;
			}
			case KEY_END:
			{
				gotoend = 1;
				break;
			}
			case KEY_ENTER:
				break;
			case KEY_CTRL_U:
				tmp->clear();
				real_maxx = maxx = real_x = x = minx;
				maxbeginning = beginning = 0;
				break;
			default:
			{
				if (tmp->length() >= length_)
					break;
				
				tmp_in += input;
				if (int(mbrtowc(&wc_in, tmp_in.c_str(), MB_CUR_MAX, 0)) < 0)
					break;
				
				int wcwidth_res = wcwidth(wc_in);
				if (wcwidth_res > 1)
					block_scrolling = 1;
				
				if (wcwidth_res > 0) // is char printable? we want to ignore things like Ctrl-?, Fx etc.
				{
					if ((real_x-minx)+beginning >= tmp->length())
					{
						tmp->push_back(wc_in);
						if (!beginning)
						{
							real_x++;
							x += wcwidth(wc_in);
						}
						beginning++;
						gotoend = 1;
					}
					else
					{
						tmp->insert(tmp->begin()+(real_x-minx)+beginning, wc_in);
						if (x < maxx)
						{
							real_x++;
							x += wcwidth(wc_in);
						}
						else if (beginning < maxbeginning)
							beginning++;
					}
				}
				tmp_in.clear();
			}
		}
	}
	while (input != KEY_ENTER);
	curs_set(0);
	
	if (m_history && !encrypted)
	{
		if (history_it != m_history->end())
		{
			m_history->push_back(*history_it);
			tmp = &m_history->back();
			m_history->erase(history_it);
		}
		else
			m_history->push_back(*tmp);
	}
	
	return ToString(*tmp);
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

size_t Window::getWidth() const
{
	if (m_border != brNone)
		return m_width+2;
	else
		return m_width;
}

size_t Window::getHeight() const
{
	size_t height = m_height;
	if (m_border != brNone)
		height += 2;
	if (!m_title.empty())
		height += 2;
	return height;
}

size_t Window::getStartX() const
{
	if (m_border != brNone)
		return m_start_x-1;
	else
		return m_start_x;
}

size_t Window::getStarty() const
{
	size_t starty = m_start_y;
	if (m_border != brNone)
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

void Window::scroll(Where where)
{
	idlok(m_window, 1);
	scrollok(m_window, 1);
	switch (where)
	{
		case wUp:
			wscrl(m_window, 1);
			break;
		case wDown:
			wscrl(m_window, -1);
			break;
		case wPageUp:
			wscrl(m_window, m_width);
			break;
		case wPageDown:
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
	if (colors.fg == clEnd || colors.bg == clEnd)
	{
		*this << clEnd;
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
		case clDefault:
			while (!m_color_stack.empty())
				m_color_stack.pop();
			setColor(m_base_color, m_base_bg_color);
			break;
		case clEnd:
			if (!m_color_stack.empty())
				m_color_stack.pop();
			if (!m_color_stack.empty())
				setColor(m_color_stack.top().fg, m_color_stack.top().bg);
			else
				setColor(m_base_color, m_base_bg_color);
			break;
		default:
			m_color_stack.push(Colors(color, clDefault));
			setColor(m_color_stack.top().fg, m_color_stack.top().bg);
	}
	return *this;
}

Window &Window::operator<<(Format format)
{
	switch (format)
	{
		case fmtNone:
			bold((m_bold_counter = 0));
			reverse((m_reverse_counter = 0));
			altCharset((m_alt_charset_counter = 0));
			break;
		case fmtBold:
			bold(++m_bold_counter);
			break;
		case fmtBoldEnd:
			if (--m_bold_counter <= 0)
				bold((m_bold_counter = 0));
			break;
		case fmtUnderline:
			underline(++m_underline_counter);
			break;
		case fmtUnderlineEnd:
			if (--m_underline_counter <= 0)
				underline((m_underline_counter = 0));
			break;
		case fmtReverse:
			reverse(++m_reverse_counter);
			break;
		case fmtReverseEnd:
			if (--m_reverse_counter <= 0)
				reverse((m_reverse_counter = 0));
			break;
		case fmtAltCharset:
			altCharset(++m_alt_charset_counter);
			break;
		case fmtAltCharsetEnd:
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
