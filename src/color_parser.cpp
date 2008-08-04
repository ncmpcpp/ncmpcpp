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

const COLOR * Window::into_color(const string &str) const
{
	COLOR *colors = new COLOR[2];
	
	if (str == "[/]")
	{
		Bold(0);
		Reverse(0);
		AltCharset(0);
		colors[0] = itsBaseColor;
		colors[1] = itsBaseBgColor;
		return colors;
	}
	
	if (str[1] == '/')
	{
		if (str == "[/a]")
			AltCharset(0);
		
		if (str == "[/b]")
			Bold(0);
		
		if (str == "[/r]")
			Reverse(0);
		
		if (str.length() > 4) // /green etc.
		{
			colors[0] = itsBaseColor; 
			colors[1] = itsBaseBgColor;
			return colors;
		}
		colors[0] = itsColor;
		colors[1] = itsBgColor;
		return colors;
	}
	else
	{
		if (str == "[a]") AltCharset(1);
		if (str == "[b]") Bold(1);
		if (str == "[r]") Reverse(1);
		
		if (str.length() <= 3)
		{
			colors[0] = itsColor;
			colors[1] = itsBgColor;
		}
	
		if (str == "[red]")
		{
			colors[0] = clRed;
			colors[1] = itsBaseBgColor;
		}
		if (str == "[black]")
		{
			colors[0] = clBlack;
			colors[1] = itsBaseBgColor;
		}
		if (str == "[blue]")
		{
			colors[0] = clBlue;
			colors[1] = itsBaseBgColor;
		}
		if (str == "[green]")
		{
			colors[0] = clGreen;
			colors[1] = itsBaseBgColor;
		}
		if (str == "[yellow]")
		{
			colors[0] = clYellow;
			colors[1] = itsBaseBgColor;
		}
		if (str == "[magenta]")
		{
			colors[0] = clMagenta;
			colors[1] = itsBaseBgColor;
		}
		if (str == "[cyan]")
		{
			colors[0] = clCyan;
			colors[1] = itsBaseBgColor;
		}
		if (str == "[white]")
		{
			colors[0] = clWhite;
			colors[1] = itsBaseBgColor;
		}
		
		/*if (str == "[black_red]")
		{
			colors[0] = clBlack;
			colors[1] = clRed;
		}
		if (str == "[black_blue]")
		{
			colors[0] = clBlack;
			colors[1] = clBlue;
		}
		if (str == "[black_green]")
		{
			colors[0] = clBlack;
			colors[1] = clGreen;
		}
		if (str == "[black_yellow]")
		{
			colors[0] = clBlack;
			colors[1] = clYellow;
		}
		if (str == "[black_magenta]")
		{
			colors[0] = clBlack;
			colors[1] = clMagenta;
		}
		if (str == "[black_cyan]")
		{
			colors[0] = clBlack;
			colors[1] = clCyan;
		}
		if (str == "[black_white]")
		{
			colors[0] = clBlack;
			colors[1] = clWhite;
		}
		if (str == "[red_black]")
		{
			colors[0] = clRed;
			colors[1] = clBlack;
		}
		if (str == "[red_blue]")
		{
			colors[0] = clRed;
			colors[1] = clBlue;
		}
		if (str == "[red_green]")
		{
			colors[0] = clRed;
			colors[1] = clGreen;
		}
		if (str == "[red_yellow]")
		{
			colors[0] = clRed;
			colors[1] = clYellow;
		}
		if (str == "[red_magenta]")
		{
			colors[0] = clRed;
			colors[1] = clMagenta;
		}
		if (str == "[red_cyan]")
		{
			colors[0] = clRed;
			colors[1] = clCyan;
		}
		if (str == "[red_white]")
		{
			colors[0] = clRed;
			colors[1] = clWhite;
		}
		if (str == "[red_white]")
		{
			colors[0] = clRed;
			colors[1] = clWhite;
		}
		if (str == "[blue_black]")
		{
			colors[0] = clBlue;
			colors[1] = clBlack;
		}
		if (str == "[blue_red]")
		{
			colors[0] = clBlue;
			colors[1] = clRed;
		}
		if (str == "[blue_green]")
		{
			colors[0] = clBlue;
			colors[1] = clGreen;
		}
		if (str == "[blue_yellow]")
		{
			colors[0] = clBlue;
			colors[1] = clYellow;
		}
		if (str == "[blue_magenta]")
		{
			colors[0] = clBlue;
			colors[1] = clMagenta;
		}
		if (str == "[blue_cyan]")
		{
			colors[0] = clBlue;
			colors[1] = clCyan;
		}
		if (str == "[blue_white]")
		{
			colors[0] = clBlue;
			colors[1] = clWhite;
		}
		if (str == "[green_black]")
		{
			colors[0] = clGreen;
			colors[1] = clBlack;
		}
		if (str == "[green_red]")
		{
			colors[0] = clGreen;
			colors[1] = clRed;
		}
		if (str == "[green_blue]")
		{
			colors[0] = clGreen;
			colors[1] = clBlue;
		}
		if (str == "[green_yellow]")
		{
			colors[0] = clGreen;
			colors[1] = clYellow;
		}
		if (str == "[green_magenta]")
		{
			colors[0] = clGreen;
			colors[1] = clMagenta;
		}
		if (str == "[green_cyan]")
		{
			colors[0] = clGreen;
			colors[1] = clCyan;
		}
		if (str == "[green_white]")
		{
			colors[0] = clGreen;
			colors[1] = clWhite;
		}
		if (str == "[yellow_black]")
		{
			colors[0] = clYellow;
			colors[1] = clBlack;
		}
		if (str == "[yellow_red]")
		{
			colors[0] = clYellow;
			colors[1] = clRed;
		}
		if (str == "[yellow_blue]")
		{
			colors[0] = clYellow;
			colors[1] = clBlue;
		}
		if (str == "[yellow_green]")
		{
			colors[0] = clYellow;
			colors[1] = clGreen;
		}
		if (str == "[yellow_magenta]")
		{
			colors[0] = clYellow;
			colors[1] = clMagenta;
		}
		if (str == "[yellow_cyan]")
		{
			colors[0] = clYellow;
			colors[1] = clCyan;
		}
		if (str == "[yellow_white]")
		{
			colors[0] = clYellow;
			colors[1] = clWhite;
		}
		if (str == "[magenta_black]")
		{
			colors[0] = clMagenta;
			colors[1] = clBlack;
		}
		if (str == "[magenta_red]")
		{
			colors[0] = clMagenta;
			colors[1] = clRed;
		}
		if (str == "[magenta_blue]")
		{
			colors[0] = clMagenta;
			colors[1] = clBlue;
		}
		if (str == "[magenta_green]")
		{
			colors[0] = clMagenta;
			colors[1] = clGreen;
		}
		if (str == "[magenta_yellow]")
		{
			colors[0] = clMagenta;
			colors[1] = clYellow;
		}
		if (str == "[magenta_cyan]")
		{
			colors[0] = clMagenta;
			colors[1] = clCyan;
		}
		if (str == "[magenta_white]")
		{
			colors[0] = clMagenta;
			colors[1] = clWhite;
		}
		if (str == "[cyan_black]")
		{
			colors[0] = clCyan;
			colors[1] = clBlack;
		}
		if (str == "[cyan_red]")
		{
			colors[0] = clCyan;
			colors[1] = clRed;
		}
		if (str == "[cyan_blue]")
		{
			colors[0] = clCyan;
			colors[1] = clBlue;
		}
		if (str == "[cyan_green]")
		{
			colors[0] = clCyan;
			colors[1] = clGreen;
		}
		if (str == "[cyan_yellow]")
		{
			colors[0] = clCyan;
			colors[1] = clYellow;
		}
		if (str == "[cyan_magenta]")
		{
			colors[0] = clCyan;
			colors[1] = clMagenta;
		}
		if (str == "[cyan_white]")
		{
			colors[0] = clCyan;
			colors[1] = clWhite;
		}
		if (str == "[white_black]")
		{
			colors[0] = clWhite;
			colors[1] = clBlack;
		}
		if (str == "[white_red]")
		{
			colors[0] = clWhite;
			colors[1] = clRed;
		}
		if (str == "[white_blue]")
		{
			colors[0] = clWhite;
			colors[1] = clBlue;
		}
		if (str == "[white_green]")
		{
			colors[0] = clWhite;
			colors[1] = clGreen;
		}
		if (str == "[white_yellow]")
		{
			colors[0] = clWhite;
			colors[1] = clYellow;
		}
		if (str == "[white_magenta]")
		{
			colors[0] = clWhite;
			colors[1] = clMagenta;
		}
		if (str == "[white_cyan]")
		{
			colors[0] = clWhite;
			colors[1] = clCyan;
		}*/
	}
	return colors;
}

bool is_valid_color(const string &str)
{
	return  str == "[/]" ||
		str == "[b]" ||
		str == "[/b]" ||
		str == "[r]" ||
		str == "[/r]" ||
		str == "[a]" ||
		str == "[/a]" ||
		
		str == "[black]" ||
		str == "[/black]" ||
		str == "[red]" ||
		str == "[/red]" ||
		str == "[blue]" ||
		str == "[/blue]" ||
		str == "[green]" ||
		str == "[/green]" ||
		str == "[yellow]" ||
		str == "[/yellow]" ||
		str == "[magenta]" ||
		str == "[/magenta]" ||
		str == "[cyan]" ||
		str == "[/cyan]" ||
		str == "[white]" ||
		str == "[/white]";
			
		/*str == "[black_red]"||
		str == "[/black_red]" ||
		str == "[black_blue]"||
		str == "[/black_blue]" ||
		str == "[black_green]"||
		str == "[/black_green]" ||
		str == "[black_yellow]"||
		str == "[/black_yellow]" ||
		str == "[black_magenta]"||
		str == "[/black_magenta]" ||
		str == "[black_cyan]"||
		str == "[/black_cyan]" ||
		str == "[black_white]"||
		str == "[/black_white]" ||
			
		str == "[red_black]"||
		str == "[/red_black]" ||
		str == "[red_blue]"||
		str == "[/red_blue]" ||
		str == "[red_green]"||
		str == "[/red_green]" ||
		str == "[red_yellow]"||
		str == "[/red_yellow]" ||
		str == "[red_magenta]"||
		str == "[/red_magenta]" ||
		str == "[red_cyan]"||
		str == "[/red_cyan]" ||
		str == "[red_white]"||
		str == "[/red_white]" ||

		str == "[blue_black]"||
		str == "[/blue_black]" ||
		str == "[blue_red]"||
		str == "[/blue_red]" ||
		str == "[blue_green]"||
		str == "[/blue_green]" ||
		str == "[blue_yellow]"||
		str == "[/blue_yellow]" ||
		str == "[blue_magenta]"||
		str == "[/blue_magenta]" ||
		str == "[blue_cyan]"||
		str == "[/blue_cyan]" ||
		str == "[blue_white]"||
		str == "[/blue_white]" ||

		str == "[green_black]"||
		str == "[/green_black]" ||
		str == "[green_red]"||
		str == "[/green_red]" ||
		str == "[green_blue]"||
		str == "[/green_blue]" ||
		str == "[green_yellow]"||
		str == "[/green_yellow]" ||
		str == "[green_magenta]"||
		str == "[/green_magenta]" ||
		str == "[green_cyan]"||
		str == "[/green_cyan]" ||
		str == "[green_white]"||
		str == "[/green_white]" ||

		str == "[yellow_black]"||
		str == "[/yellow_black]" ||
		str == "[yellow_red]"||
		str == "[/yellow_red]" ||
		str == "[yellow_blue]"||
		str == "[/yellow_blue]" ||
		str == "[yellow_green]"||
		str == "[/yellow_green]" ||
		str == "[yellow_magenta]"||
		str == "[/yellow_magenta]" ||
		str == "[yellow_cyan]"||
		str == "[/yellow_cyan]" ||
		str == "[yellow_white]"||
		str == "[/yellow_white]" ||

		str == "[magenta_black]"||
		str == "[/magenta_black]" ||
		str == "[magenta_red]"||
		str == "[/magenta_red]" ||
		str == "[magenta_blue]"||
		str == "[/magenta_blue]" ||
		str == "[magenta_green]"||
		str == "[/magenta_green]" ||
		str == "[magenta_yellow]"||
		str == "[/magenta_yellow]" ||
		str == "[magenta_cyan]"||
		str == "[/magenta_cyan]" ||
		str == "[magenta_white]"||
		str == "[/magenta_white]" ||

		str == "[cyan_black]"||
		str == "[/cyan_black]" ||
		str == "[cyan_red]"||
		str == "[/cyan_red]" ||
		str == "[cyan_blue]"||
		str == "[/cyan_blue]" ||
		str == "[cyan_green]"||
		str == "[/cyan_green]" ||
		str == "[cyan_yellow]"||
		str == "[/cyan_yellow]" ||
		str == "[cyan_magenta]"||
		str == "[/cyan_magenta]" ||
		str == "[cyan_white]"||
		str == "[/cyan_white]" ||

		str == "[white_black]"||
		str == "[/white_black]" ||
		str == "[white_red]"||
		str == "[/white_red]" ||
		str == "[white_blue]"||
		str == "[/white_blue]" ||
		str == "[white_green]"||
		str == "[/white_green]" ||
		str == "[white_yellow]"||
		str == "[/white_yellow]" ||
		str == "[white_magenta]"||
		str == "[/white_magenta]" ||
		str == "[white_cyan]"||
		str == "[/white_cyan]";*/
}

