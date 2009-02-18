/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#include "window.h"

using std::string;
using std::wstring;

void InitScreen(bool enable_colors)
{
	setlocale(LC_ALL, "");
	initscr();
	if (has_colors() && enable_colors)
	{
		start_color();
		use_default_colors();
		int num = 1;
		for (int i = -1; i < 8; i++)
			for (int j = 0; j < 8; j++)
				init_pair(num++, j, i);
	}
	noecho();
	cbreak();
	curs_set(0);
}

void DestroyScreen()
{
	curs_set(1);
	endwin();
}

Window::Window(size_t startx,
		size_t starty,
		size_t width,
		size_t height,
		const string &title,
		Color color,
		Border border)
		: itsWindow(0),
		itsWinBorder(0),
		itsGetStringHelper(0),
		itsStartX(startx),
		itsStartY(starty),
		itsWidth(width),
		itsHeight(height),
		itsWindowTimeout(-1),
		itsX(0),
		itsY(0),
		itsTitle(title),
		itsColor(color),
		itsBaseColor(color),
		itsBgColor(clDefault),
		itsBaseBgColor(clDefault),
		itsBorder(border)
{
	if (itsStartX > size_t(COLS)
	||  itsStartY > size_t(LINES)
	||  itsWidth+itsStartX > size_t(COLS)
	||  itsHeight+itsStartY > size_t(LINES))
		throw BadSize();
	
	if (itsBorder != brNone)
	{
		itsWinBorder = newpad(itsHeight, itsWidth);
		wattron(itsWinBorder, COLOR_PAIR(itsBorder));
		box(itsWinBorder, 0, 0);
		itsStartX++;
		itsStartY++;
		itsWidth -= 2;
		itsHeight -= 2;
	}
	if (!itsTitle.empty())
	{
		itsStartY += 2;
		itsHeight -= 2;
	}
	
	itsWindow = newpad(itsHeight, itsWidth);
	
	SetColor(itsColor);
	keypad(itsWindow, 1);
}

Window::Window(const Window &w)
{
	itsWindow = dupwin(w.itsWindow);
	itsWinBorder = dupwin(w.itsWinBorder);
	itsGetStringHelper = w.itsGetStringHelper;
	itsWindowTimeout = w.itsWindowTimeout;
	itsX = w.itsX;
	itsY = w.itsY;
	itsStartX = w.itsStartX;
	itsStartY = w.itsStartY;
	itsWidth = w.itsWidth;
	itsHeight = w.itsHeight;
	itsTitle = w.itsTitle;
	itsColors = w.itsColors;
	itsColor = w.itsColor;
	itsBaseColor = w.itsBaseColor;
	itsBgColor = w.itsBgColor;
	itsBaseBgColor = w.itsBaseBgColor;
	itsBorder = w.itsBorder;
	SetColor(itsColor, itsBgColor);
	keypad(itsWindow, 1);
}

Window::~Window()
{
	delwin(itsWindow);
	delwin(itsWinBorder);
}

void Window::SetColor(Color col, Color background)
{
	if (col == clDefault)
		col = itsBaseColor;
	
	if (col != clDefault)
		wattron(itsWindow, COLOR_PAIR(background*8+col));
	else
		wattroff(itsWindow, COLOR_PAIR(itsColor));
	itsColor = col;
	itsBgColor = background;
}

void Window::SetBaseColor(Color col, Color background)
{
	itsBaseColor = col;
	itsBaseBgColor = background;
}

void Window::SetBorder(Border border)
{
	if (border == brNone && itsBorder != brNone)
	{
		delwin(itsWinBorder);
		itsStartX--;
		itsStartY--;
		itsHeight += 2;
		itsWidth += 2;
		Recreate();
	}
	else if (border != brNone && itsBorder == brNone)
	{
		itsWinBorder = newpad(itsHeight, itsWidth);
		wattron(itsWinBorder, COLOR_PAIR(border));
		box(itsWinBorder,0,0);
		itsStartX++;
		itsStartY++;
		itsHeight -= 2;
		itsWidth -= 2;
		Recreate();
	}
	else
	{
		wattron(itsWinBorder,COLOR_PAIR(border));
		box(itsWinBorder, 0, 0);
	}
	itsBorder = border;
}

void Window::SetTitle(const string &newtitle)
{
	if (itsTitle == newtitle)
	{
		return;
	}
	else if (!newtitle.empty() && itsTitle.empty())
	{
		itsStartY += 2;
		itsHeight -= 2;
		Recreate();
	}
	else if (newtitle.empty() && !itsTitle.empty())
	{
		itsStartY -= 2;
		itsHeight += 2;
		Recreate();
	}
	itsTitle = newtitle;
}

void Window::Recreate()
{
	delwin(itsWindow);
	itsWindow = newpad(itsHeight, itsWidth);
	SetTimeout(itsWindowTimeout);
	SetColor(itsColor, itsBgColor);
	keypad(itsWindow, 1);
}

void Window::MoveTo(size_t newx, size_t newy)
{
	itsStartX = newx;
	itsStartY = newy;
	if (itsBorder != brNone)
	{
		itsStartX++;
		itsStartY++;
	}
	if (!itsTitle.empty())
		itsStartY += 2;
}

void Window::AdjustDimensions(size_t &width, size_t &height)
{
	if (itsBorder != brNone)
	{
		delwin(itsWinBorder);
		itsWinBorder = newpad(height, width);
		wattron(itsWinBorder, COLOR_PAIR(itsBorder));
		box(itsWinBorder, 0, 0);
		width -= 2;
		height -= 2;
	}
	if (!itsTitle.empty())
		height -= 2;
	itsHeight = height;
	itsWidth = width;
}

void Window::Resize(size_t width, size_t height)
{
	AdjustDimensions(width, height);
	Recreate();
}

void Window::ShowBorder() const
{
	if (itsBorder != brNone)
	{
		refresh();
		prefresh(itsWinBorder, 0, 0, GetStartY(), GetStartX(), itsStartY+itsHeight, itsStartX+itsWidth);
	}
	if (!itsTitle.empty())
	{
		if (itsBorder != brNone)
			attron(COLOR_PAIR(itsBorder));
		else
			attron(COLOR_PAIR(itsBaseColor));
		mvhline(itsStartY-1, itsStartX, 0, itsWidth);
		attron(A_BOLD);
		mvhline(itsStartY-2, itsStartX, 32, itsWidth); // clear title line
		mvaddstr(itsStartY-2, itsStartX, itsTitle.c_str());
		attroff(COLOR_PAIR(itsBorder) | A_BOLD);
	}
	refresh();
}

void Window::Display()
{
	ShowBorder();
	Refresh();
}

void Window::Refresh()
{
	prefresh(itsWindow, 0, 0, itsStartY, itsStartX, itsStartY+itsHeight-1, itsStartX+itsWidth-1);
}

void Window::Clear(bool)
{
	werase(itsWindow);
	Window::Refresh();
}

void Window::Hide(char x) const
{
	for (size_t i = 0; i < GetHeight(); i++)
		mvhline(i+GetStartY(), GetStartX(), x, GetWidth());
	refresh();
}

void Window::Bold(bool bold) const
{
	(bold ? wattron : wattroff)(itsWindow, A_BOLD);
}

void Window::Reverse(bool reverse) const
{
	(reverse ? wattron : wattroff)(itsWindow, A_REVERSE);
}

void Window::AltCharset(bool alt) const
{
	(alt ? wattron : wattroff)(itsWindow, A_ALTCHARSET);
}

void Window::SetTimeout(int timeout)
{
	itsWindowTimeout = timeout;
	wtimeout(itsWindow, timeout);
}

void Window::ReadKey(int &input) const
{
	input = wgetch(itsWindow);
}

void Window::ReadKey() const
{
	wgetch(itsWindow);
}

/*void Window::Write(bool cte, const char *format, ...) const
{
	va_list list;
	va_start(list, format);
	vw_printw(itsWindow, format, list);
	va_end(list);
	if (cte)
		wclrtoeol(itsWindow);
}

void Window::WriteXY(int x, int y, bool cte, const char *format, ...) const
{
	va_list list;
	va_start(list, format);
	wmove(itsWindow, y, x);
	vw_printw(itsWindow, format, list);
	va_end(list);
	if (cte)
		wclrtoeol(itsWindow);
}*/

string Window::GetString(const string &base, size_t length, size_t width, bool encrypted) const
{
	int input;
	size_t beginning, maxbeginning, minx, x, real_x, y, maxx, real_maxx;
	
	getyx(itsWindow, y, x);
	minx = real_maxx = maxx = real_x = x;
	
	width--;
	if (width == size_t(-1))
		width = itsWidth-x-1;
	
	curs_set(1);
	std::wstring tmp = ToWString(base);
	
	string tmp_in;
	wchar_t wc_in;
	bool gotoend = 1;
	bool block_scrolling = 0;
	
	mbstate_t state;
	memset(&state, 0, sizeof(state));
	
	// disable scrolling if wide chars are used
	for (wstring::const_iterator it = tmp.begin(); it != tmp.end(); it++)
		if (wcwidth(*it) > 1)
			block_scrolling = 1;
	
	beginning = -1;
	
	do
	{
		if (tmp.empty())
			block_scrolling = 0;
		
		maxbeginning = block_scrolling ? 0 : (tmp.length() < width ? 0 : tmp.length()-width);
		maxx = minx + (Length(tmp) < width ? Length(tmp) : width);
		
		real_maxx = minx + (tmp.length() < width ? tmp.length() : width);
		
		if (beginning > maxbeginning)
			beginning = maxbeginning;
		
		if (gotoend)
		{
			size_t real_real_maxx = minx;
			size_t biggest_x = minx+width;
				
			if (block_scrolling && maxx >= biggest_x)
			{
				size_t i = 0;
				for (wstring::const_iterator it = tmp.begin(); i < width; it++, real_real_maxx++)
					i += wcwidth(*it);
			}
			else
				real_real_maxx = real_maxx;
				
			real_x = real_real_maxx;
			x = block_scrolling ? (maxx > biggest_x ? biggest_x : maxx) : maxx;
			beginning = maxbeginning;
			gotoend = 0;
		}
		
		mvwhline(itsWindow, y, minx, 32, width+1);
		
		if (!encrypted)
			mvwprintw(itsWindow, y, minx, "%ls", tmp.substr(beginning, width+1).c_str());
		else
			mvwhline(itsWindow, y, minx, '*', maxx-minx);
		
		if (itsGetStringHelper)
			itsGetStringHelper(tmp);
		
		wmove(itsWindow, y, x);
		input = wgetch(itsWindow);
		
		switch (input)
		{
			case ERR:
			case KEY_UP:
			case KEY_DOWN:
				break;
			case KEY_RIGHT:
			{
				if (x < maxx)
				{
					real_x++;
					x += wcwidth(tmp[beginning+real_x-minx-1]);
				}
				else if (beginning < maxbeginning)
					beginning++;
				break;
			}
			case KEY_BACKSPACE: case 127:
			{
				if (x <= minx && !beginning)
					break;
			}
			case KEY_LEFT:
			{
				if (x > minx)
				{
					real_x--;
					x -= wcwidth(tmp[beginning+real_x-minx]);
				}
				else if (beginning > 0)
					beginning--;
				if (input != KEY_BACKSPACE && input != 127)
					break; // backspace = left & delete.
			}
			case KEY_DC:
			{
				if ((real_x-minx)+beginning == tmp.length())
					break;
				tmp.erase(tmp.begin()+(real_x-minx)+beginning);
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
			case 10:
				break;
			default:
			{
				if (tmp.length() >= length)
					break;
				
				tmp_in += input;
				if ((int)mbrtowc(&wc_in, tmp_in.c_str(), MB_CUR_MAX, &state) < 0)
					break;
				
				if (wcwidth(wc_in) > 1)
					block_scrolling = 1;
				
				if ((real_x-minx)+beginning >= tmp.length())
				{
					tmp.push_back(wc_in);
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
					tmp.insert(tmp.begin()+(real_x-minx)+beginning, wc_in);
					if (x < maxx)
					{
						real_x++;
						x += wcwidth(wc_in);
					}
					else if (beginning < maxbeginning)
						beginning++;
				}
				tmp_in.clear();
			}
		}
	}
	while (input != 10);
	curs_set(0);
	
	return ToString(tmp);
}

void Window::GetXY(int &x, int &y)
{
	getyx(itsWindow, y, x);
	itsX = x;
	itsY = y;
}

void Window::GotoXY(int x, int y)
{
	wmove(itsWindow, y, x);
	itsX = x;
	itsY = y;
}

const int &Window::X() const
{
	return itsX;
}

const int &Window::Y() const
{
	return itsY;
}

void Window::Scrollable(bool scrollable) const
{
	scrollok(itsWindow, scrollable);
	idlok(itsWindow, scrollable);
}

size_t Window::GetWidth() const
{
	if (itsBorder != brNone)
		return itsWidth+2;
	else
		return itsWidth;
}

size_t Window::GetHeight() const
{
	size_t height = itsHeight;
	if (itsBorder != brNone)
		height += 2;
	if (!itsTitle.empty())
		height += 2;
	return height;
}

size_t Window::GetStartX() const
{
	if (itsBorder != brNone)
		return itsStartX-1;
	else
		return itsStartX;
}

size_t Window::GetStartY() const
{
	size_t starty = itsStartY;
	if (itsBorder != brNone)
		starty--;
	if (!itsTitle.empty())
		starty -= 2;
	return starty;
}

const string &Window::GetTitle() const
{
	return itsTitle;
}

Color Window::GetColor() const
{
	return itsColor;
}

Border Window::GetBorder() const
{
	return itsBorder;
}

void Window::Scroll(Where where)
{
	idlok(itsWindow, 1);
	scrollok(itsWindow, 1);
	switch (where)
	{
		case wUp:
			wscrl(itsWindow, 1);
			break;
		case wDown:
			wscrl(itsWindow, -1);
			break;
		case wPageUp:
			wscrl(itsWindow, itsWidth);
			break;
		case wPageDown:
			wscrl(itsWindow, -itsWidth);
			break;
		default:
			break;
	}
	idlok(itsWindow, 0);
	scrollok(itsWindow, 0);
}


Window &Window::operator<<(const Colors &colors)
{
	if (colors.fg == clEnd || colors.bg == clEnd)
		return *this;
	itsColors.push(colors);
	SetColor(colors.fg, colors.bg);
	return *this;
}

Window &Window::operator<<(const Color &color)
{
	switch (color)
	{
		case clDefault:
			while (!itsColors.empty())
				itsColors.pop();
			SetColor(itsBaseColor, itsBaseBgColor);
			break;
		case clEnd:
			if (!itsColors.empty())
				itsColors.pop();
			if (!itsColors.empty())
				SetColor(itsColors.top().fg, itsColors.top().bg);
			else
				SetColor(itsBaseColor, itsBaseBgColor);
			break;
		default:
			itsColors.push(Colors(color, clDefault));
			SetColor(itsColors.top().fg, itsColors.top().bg);
	}
	return *this;
}

Window &Window::operator<<(const Format &format)
{
	switch (format)
	{
		case fmtNone:
			Bold(0);
			Reverse(0);
			AltCharset(0);
			break;
		case fmtBold:
			Bold(1);
			break;
		case fmtBoldEnd:
			Bold(0);
			break;
		case fmtReverse:
			Reverse(1);
			break;
		case fmtReverseEnd:
			Reverse(0);
			break;
		case fmtAltCharset:
			AltCharset(1);
			break;
		case fmtAltCharsetEnd:
			AltCharset(0);
			break;
	}
	return *this;
}

Window &Window::operator<<(int (*f)(WINDOW *))
{
	f(itsWindow);
	return *this;
}

Window &Window::operator<<(const XY &coords)
{
	GotoXY(coords.x, coords.y);
	return *this;
}

Window &Window::operator<<(const char *s)
{
	wprintw(itsWindow, "%s", s);
	return *this;
}

Window &Window::operator<<(const char &c)
{
	wprintw(itsWindow, "%c", c);
	return *this;
}

Window &Window::operator<<(const wchar_t *ws)
{
	wprintw(itsWindow, "%ls", ws);
	return *this;
}

Window &Window::operator<<(const wchar_t &wc)
{
	wprintw(itsWindow, "%lc", wc);
	return *this;
}

Window &Window::operator<<(const int &i)
{
	wprintw(itsWindow, "%d", i);
	return *this;
}

Window &Window::operator<<(const double &d)
{
	wprintw(itsWindow, "%f", d);
	return *this;
}

Window &Window::operator<<(const string &s)
{
	for (string::const_iterator it = s.begin(); it != s.end(); it++)
		wprintw(itsWindow, "%c", *it);
	return *this;
}

Window &Window::operator<<(const wstring &ws)
{
	for (wstring::const_iterator it = ws.begin(); it != ws.end(); it++)
		wprintw(itsWindow, "%lc", *it);
	return *this;
}

Window &Window::operator<<(const size_t &s)
{
	wprintw(itsWindow, "%u", s);
	return *this;
}

Window * Window::EmptyClone() const
{
	return new Window(GetStartX(), GetStartY(), GetWidth(), GetHeight(), itsTitle, itsBaseColor, itsBorder);
}

char *ToString(const wchar_t *ws)
{
	mbstate_t mbs;
	memset(&mbs, 0, sizeof(mbs));
	size_t len = wcsrtombs(NULL, &ws, 0, &mbs);
	
	if (len == size_t(-1))
		return 0;
	
	char *s = new char[len+1]();
	wcsrtombs(s, &ws, len, &mbs);
	s[len] = 0;
	return s;
}

wchar_t *ToWString(const char *s)
{
	mbstate_t mbs;
	memset(&mbs, 0, sizeof(mbs));
	size_t len = mbsrtowcs(NULL, &s, 0, &mbs);
	
	if (len == size_t(-1))
		return 0;
	
	wchar_t *ws = new wchar_t[len+1]();
	mbsrtowcs(ws, &s, len, &mbs);
	ws[len] = 0;
	return ws;
}

string ToString(const wstring &ws)
{
	string result;
	char *s = ToString(ws.c_str());
	if (s)
	{
		result = s;
		delete [] s;
	}
	return result;
}

wstring ToWString(const string &s)
{
	wstring result;
	wchar_t *ws = ToWString(s.c_str());
	if (ws)
	{
		result = ws;
		delete [] ws;
	}
	return result;
}

size_t Window::Length(const wstring &ws)
{
	size_t length = 0;
	for (wstring::const_iterator it = ws.begin(); it != ws.end(); it++)
		length += wcwidth(*it);
	return length;
}

