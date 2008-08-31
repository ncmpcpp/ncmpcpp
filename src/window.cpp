/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "window.h"

Window::Window(int startx, int starty, int width, int height, string title, COLOR color, BORDER border) : itsWindow(0), itsWinBorder(0), itsStartX(startx), itsStartY(starty), itsWidth(width), itsHeight(height), BBEnabled(1), AutoRefreshEnabled(1), itsTitle(title), itsColor(color), itsBaseColor(color), itsBgColor(clDefault), itsBaseBgColor(clDefault), itsBorder(border)
{
	if (itsStartX < 0) itsStartX = 0;
	if (itsStartY < 0) itsStartY = 0;
	
	if (itsBorder != brNone)
	{
		itsWinBorder = newpad(itsHeight, itsWidth);
		wattron(itsWinBorder,COLOR_PAIR(itsBorder));
		box(itsWinBorder,0,0);
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
	
	if (itsWidth > 0 && itsHeight > 0)
		itsWindow = newwin(itsHeight, itsWidth, itsStartY, itsStartX);
	else
		itsWindow = newwin(0, 0, 0, 0);
	
	SetColor(itsColor);
}

Window::~Window()
{
	delwin(itsWindow);
	delwin(itsWinBorder);
}

void Window::SetColor(COLOR col, COLOR background)
{
	if (col != clDefault)
		wattron(itsWindow,COLOR_PAIR(background*8+col));
	else
		wattroff(itsWindow,COLOR_PAIR(itsColor));
	itsColor = col;
	itsBgColor = background;
}

void Window::SetBaseColor(COLOR col, COLOR background)
{
	itsBaseColor = col;
	itsBaseBgColor = background;
}

bool Window::have_to_recreate(BORDER border)
{
	if (border == brNone && itsBorder != brNone)
	{
		delwin(itsWinBorder);
		itsStartX--;
		itsStartY--;
		itsHeight += 2;
		itsWidth += 2;
		return true;
	}
	if (border != brNone && itsBorder == brNone)
	{
		itsWinBorder = newpad(itsHeight,itsWidth);
		wattron(itsWinBorder,COLOR_PAIR(border));
		box(itsWinBorder,0,0);
		itsStartX++;
		itsStartY++;
		itsHeight -= 2;
		itsWidth -= 2;
		return true;
	}
	return false;
}

bool Window::have_to_recreate(string newtitle)
{
	if (!newtitle.empty() && itsTitle.empty())
	{
		itsStartY += 2;
		itsHeight -= 2;
		return true;
	}
	if (newtitle.empty() && !itsTitle.empty())
	{
		itsStartY -= 2;
		itsHeight += 2;
		return true;
	}
	return false;
}

void Window::SetBorder(BORDER border)
{
	if (have_to_recreate(border))
		recreate_win();
	itsBorder = border;
}

void Window::SetTitle(string newtitle)
{
	if (itsTitle == newtitle)
		return;
	if (have_to_recreate(newtitle))
		recreate_win();
	itsTitle = newtitle;
}

void Window::recreate_win()
{
	delwin(itsWindow);
	itsWindow = newwin(itsHeight, itsWidth, itsStartY, itsStartX);
	SetColor(itsColor);
}

bool Window::reallocate_win(int newx, int newy)
{
	if (newx < 0 || newy < 0 || (newx == itsStartX && newy == itsStartY)) return false;
	itsStartX = newx;
	itsStartY = newy;
	if (itsBorder != brNone)
	{
		itsStartX++;
		itsStartY++;
	}
	if (!itsTitle.empty())
		itsStartY += 2;
	return true;
}

void Window::MoveTo(int newx, int newy)
{
	if (reallocate_win(newx, newy))
		mvwin(itsWindow, itsStartY, itsStartX);
}

void Window::Resize(int width, int height)
{
	if (itsBorder != brNone)
	{
		delwin(itsWinBorder);
		itsWinBorder = newpad(height, width);
		wattron(itsWinBorder,COLOR_PAIR(itsBorder));
		box(itsWinBorder,0,0);
		width -= 2;
		height -= 2;
	}
	if (!itsTitle.empty())
		height -= 2;
	
	if (height > 0 && width > 0 && wresize(itsWindow, height, width) == OK)
	{
		itsHeight = height;
		itsWidth = width;
	}
}

void Window::show_border() const
{
	if (itsBorder != brNone)
	{
		refresh();
		prefresh(itsWinBorder,0,0,GetStartY(),GetStartX(),itsStartY+itsHeight,itsStartX+itsWidth);
	}
	if (!itsTitle.empty())
	{
		if (itsBorder != brNone)
			attron(COLOR_PAIR(itsBorder));
		else
			attron(COLOR_PAIR(itsBaseColor));
		mvhline(itsStartY-1, itsStartX, 0, itsWidth);
		attron(A_BOLD);
		mvaddstr(itsStartY-2, itsStartX, itsTitle.c_str());
		attroff(COLOR_PAIR(itsBorder) | A_BOLD);
	}
	refresh();
}

void Window::Display(bool stub)
{
	show_border();
	wrefresh(itsWindow);
}

void Window::Refresh(bool stub)
{
	wrefresh(itsWindow);
}

void Window::Clear()
{
	for (int i = 0; i < GetHeight(); i++)
		mvwhline(itsWindow, i, 0, 32, GetWidth());
	wrefresh(itsWindow);
}

void Window::Hide(char x) const
{
	for (int i = 0; i < GetHeight(); i++)
		mvhline(i+GetStartY(), GetStartX(), x, GetWidth());
	refresh();
}

void Window::Bold(bool bold) const
{
	bold ? wattron(itsWindow, A_BOLD) : wattroff(itsWindow, A_BOLD);
}

void Window::Reverse(bool reverse) const
{
	reverse ? wattron(itsWindow, A_REVERSE) : wattroff(itsWindow, A_REVERSE);
}

void Window::AltCharset(bool alt) const
{
	alt ? wattron(itsWindow, A_ALTCHARSET) : wattroff(itsWindow, A_ALTCHARSET);
}

void Window::Delay(bool delay) const
{
	nodelay(itsWindow, !delay);
}

void Window::Timeout(int timeout) const
{
	wtimeout(itsWindow, timeout);
}

void Window::ReadKey(int &input) const
{
	 keypad(itsWindow, TRUE); input = wgetch(itsWindow); keypad(itsWindow, FALSE);
}

void Window::ReadKey() const
{
	wgetch(itsWindow);
}

void Window::Write(int limit, const string &str, CLEAR_TO_EOL clrtoeol)
{
	if (BBEnabled && !str.empty())
	{
		bool collect = false;
		string color, tmp;
		for (string::const_iterator it = str.begin(); it != str.end() && limit > 0; it++)
		{
			if (*it != '[' && !collect)
			{
				tmp += *it;
				limit--;
			}
			else
				collect = 1;
		
			if (collect)
			{
				if (*it != '[')
				{
					color += *it;
					if (color.length() > 10) collect = 0; // longest bbcode is 10 chars long
				}
				else
				{
					limit -= color.length();
					tmp += color;
					color = *it;
				}
			}
		
			if (*it == ']' || it+1 == str.end())
				collect = 0;
			
			if (!collect && !color.empty())
			{
				waddstr(itsWindow,tmp.c_str());
				tmp.clear();
				if (is_valid_color(color))
				{
					std::pair<COLOR, COLOR> colors = into_color(color);
					SetColor(colors.first, colors.second);
				}
				else
				{
					limit -= color.length();
					tmp += limit > 0 ? color : color.substr(0, color.length()+limit);
				}
				color.clear();
			}
		}
		if (!tmp.empty()) waddstr(itsWindow,tmp.c_str());
	}
	else
		waddstr(itsWindow,str.c_str());
	
	if (clrtoeol)
		wclrtoeol(itsWindow);
	if (AutoRefreshEnabled)
		wrefresh(itsWindow);
}

#ifdef UTF8_ENABLED
void Window::Write(int limit, const wstring &str, CLEAR_TO_EOL clrtoeol)
{
	if (BBEnabled)
	{
		bool collect = false;
		wstring color, tmp;
		for (wstring::const_iterator it = str.begin(); it != str.end() && limit > 0; it++)
		{
			if (*it != '[' && !collect)
			{
				tmp += *it;
				limit--;
			}
			else
				collect = 1;
		
			if (collect)
			{
				if (*it != '[')
				{
					color += *it;
					if (color.length() > 10) collect = 0; // longest bbcode is 10 chars long
				}
				else
				{
					limit -= color.length();
					tmp += color;
					color = *it;
				}
			}
			
			if (*it == ']' || it+1 == str.end())
				collect = 0;
			
			if (!collect && !color.empty())
			{
				waddwstr(itsWindow,tmp.c_str());
				tmp.clear();
				if (is_valid_color(ToString(color)))
				{
					std::pair<COLOR, COLOR> colors = into_color(ToString(color));
					SetColor(colors.first, colors.second);
				}
				else
				{
					limit -= color.length();
					tmp += limit > 0 ? color : color.substr(0, color.length()+limit);
				}
				color.clear();
			}
		}
		if (!tmp.empty()) waddwstr(itsWindow,tmp.c_str());
	}
	else
		waddwstr(itsWindow,str.c_str());
	
	if (clrtoeol)
		wclrtoeol(itsWindow);
	if (AutoRefreshEnabled)
		wrefresh(itsWindow);
}

void Window::WriteXY(int x, int y, int limit, const wstring &str, CLEAR_TO_EOL cleartoeol)
{
	wmove(itsWindow,y,x);
	Write(limit, str, cleartoeol);
}
#endif

void Window::WriteXY(int x, int y, int limit, const string &str, CLEAR_TO_EOL cleartoeol)
{
	wmove(itsWindow,y,x);
	Write(limit, str, cleartoeol);
}


string Window::GetString(const string &base, unsigned int length, void (*given_function)()) const
{
	curs_set(1);
	
	keypad(itsWindow,TRUE);
	
	int input, minx, x, y, maxx;
	wstring tmp;
	
	getyx(itsWindow,y,x);
	minx = maxx = x;
	
	tmp = ToWString(base);
	
	wmove(itsWindow,y,minx);
	wprintw(itsWindow, "%ls",tmp.c_str());
	
	maxx += tmp.length();
	
	wrefresh(itsWindow);
	
	string tmp_in;
	wchar_t wc_in;
	x = maxx;
	
	do
	{
		if (given_function)
			given_function();
		wmove(itsWindow,y,x);
		input = wgetch(itsWindow);
	
		switch (input)
		{
			case ERR:
				continue;
			case KEY_UP:
			case KEY_DOWN: break;
			case KEY_RIGHT:
			{	
				if (x < maxx)
					x++;
				break;
			}
			case KEY_BACKSPACE: case 127:
			{
				if (x <= minx) break;
			}
			case KEY_LEFT:
			{
				if (x > minx)
					x--;
				if (input != KEY_BACKSPACE && input != 127) break; // backspace = left & delete.
			}
			case KEY_DC:
			{
				if ((maxx-x) < 1) break;
				tmp.erase(tmp.end()-(maxx-x));
				wmove(itsWindow,y,x); // for backspace
				wdelch(itsWindow);
				maxx--;
				break;
			}
			case KEY_HOME:
			{
				x = minx;
				break;
			}
			case KEY_END:
			{
				x = maxx;
				break;
			}
			case 10: break;
			default:
			{
				if (maxx-minx >= length)
					break;
				
				tmp_in += input;
				if (mbtowc(&wc_in, tmp_in.c_str(), MB_CUR_MAX) < 0)
					break;
				
				if (maxx == x)
					tmp.push_back(wc_in);
				else
					tmp.insert(tmp.end()-(maxx-x),wc_in);
				
				winsstr(itsWindow, tmp_in.c_str());
				tmp_in.clear();
				
				x++;
				maxx++;
			}
		}
	}
	while (input != 10);
	
	keypad(itsWindow, FALSE);
	curs_set(0);
	return ToString(tmp);
}

void Window::Scrollable(bool scrollable) const
{
	scrollok(itsWindow, scrollable);
	idlok(itsWindow, scrollable);
}

void Window::GetXY(int &x, int &y) const
{
	getyx(itsWindow, y, x);
}

void Window::GotoXY(int x, int y) const
{
	wmove(itsWindow, y, x);
}

int Window::GetWidth() const
{
	if (itsBorder != brNone)
		return itsWidth+2;
	else
		return itsWidth;
}

int Window::GetHeight() const
{
	int height = itsHeight;
	if (itsBorder != brNone)
		height += 2;
	if (!itsTitle.empty())
		height += 2;
	return height;
}

int Window::GetStartX() const
{
	if (itsBorder != brNone)
		return itsStartX-1;
	else
		return itsStartX;
}

int Window::GetStartY() const
{
	int starty = itsStartY;
	if (itsBorder != brNone)
		starty--;
	if (!itsTitle.empty())
		starty -= 2;
	return starty;
}

string Window::GetTitle() const
{
	return itsTitle;
}

COLOR Window::GetColor() const
{
	return itsColor;
}

BORDER Window::GetBorder() const
{
	return itsBorder;
}

void EnableColors()
{
	if (has_colors())
	{
		start_color();
		int num = 1;
		if (use_default_colors() != ERR)
			for (int i = -1; i < 8; i++)
				for (int j = 0; j < 8; j++)
					init_pair(num++, j, i);
		else
			for (int i = 1; i <= 8; i++)
				init_pair(i, i, COLOR_BLACK);
	}
}

Window * Window::EmptyClone()
{
	return new Window(GetStartX(),GetStartY(),GetWidth(),GetHeight(),itsTitle,itsBaseColor,itsBorder);
}

char * ToString(const wchar_t *ws)
{
	string s;
	for (int i = 0; i < wcslen(ws); i++)
	{
		char *c = (char *)calloc(MB_CUR_MAX, sizeof(char));
		wctomb(c, ws[i]);
		s += c;
		delete [] c;
	}
	return (char *)s.c_str();
}

string OmitBBCodes(const string &str)
{
	if (str.empty())
		return "";
	bool collect = false;
	string tmp, color, result;
	for (string::const_iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it != '[' && !collect)
			tmp += *it;
		else
			collect = 1;
		
		if (collect)
		{
			if (*it != '[')
				color += *it;
			else
			{
				tmp += color;
				color = *it;
			}
		}
		
		if (*it == ']' || it+1 == str.end())
			collect = 0;
		
		if (!collect)
		{
			result += tmp;
			tmp.clear();
			if (!is_valid_color(color))
				tmp += color;
			color.clear();
		}
	}
	if (!tmp.empty()) result += tmp;
	return result;
}

int CountBBCodes(const string &str)
{
	if (str.empty())
		return 0;
	bool collect = false;
	int length = 0;
	string color;
	for (string::const_iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it != '[' && !collect);
		else
			collect = 1;
		
		if (collect)
		{
			if (*it != '[')
			{
				color += *it;
				length++;
			}
			else
			{
				length -= color.length();
				length++;
				color = *it;
			}
		}
		
		if (*it == ']' || it+1 == str.end())
			collect = 0;
		
		if (!collect)
		{
			if (!is_valid_color(color))
				length -= color.length();
			color.clear();
		}
	}
	return length;
}

int CountBBCodes(const wstring &str)
{
	if (str.empty())
		return 0;
	bool collect = false;
	int length = 0;
	wstring color;
	for (wstring::const_iterator it = str.begin(); it != str.end(); it++)
	{
		if (*it != '[' && !collect);
		else
			collect = 1;
		
		if (collect)
		{
			if (*it != '[')
			{
				color += *it;
				length++;
			}
			else
			{
				length -= color.length();
				length++;
				color = *it;
			}
		}
		
		if (*it == ']' || it+1 == str.end())
			collect = 0;
		
		if (!collect)
		{
			if (!is_valid_color(ToString(color)))
				length -= color.length();
			color.clear();
		}
	}
	return length;
}

wchar_t * ToWString(const char *s)
{
	wchar_t *ws = (wchar_t *)calloc(strlen(s)+1, sizeof(wchar_t));
	mbstowcs(ws, s, strlen(s));
	return ws;
}

string ToString(const wstring &ws)
{
	string s;
	for (wstring::const_iterator it = ws.begin(); it != ws.end(); it++)
	{
		char *c = (char *)calloc(MB_CUR_MAX, sizeof(char));
		wctomb(c, *it);
		s += c;
		delete [] c;
	}
	return s;
}

wstring ToWString(const string &s)
{
	wchar_t *ws = (wchar_t *)calloc(s.length()+1, sizeof(wchar_t));
	mbstowcs(ws, s.c_str(), s.length());
	wstring result = ws;
	delete [] ws;
	return result;
}
