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

#include "actions.h"

struct PushCharacters : public Action
{
	template <typename Iterator> PushCharacters(Window **w, Iterator first, Iterator last) : Action(aMacroUtility, "")
	{
		construct(w, first, last);
	}
	
	PushCharacters(Window **w, const std::initializer_list<int> &v) : Action(aMacroUtility, "")
	{
		construct(w, v.begin(), v.end());
	}
	
	virtual void Run()
	{
		for (auto it = itsQueue.begin(); it != itsQueue.end(); ++it)
			(*itsWindow)->PushChar(*it);
	}
	
	private:
		template <typename Iterator> void construct(Window **w, Iterator first, Iterator last)
		{
			itsWindow = w;
			for (; first != last; ++first)
				itsQueue.push_back(*first);
		}
		
		Window **itsWindow;
		std::vector<int> itsQueue;
};
