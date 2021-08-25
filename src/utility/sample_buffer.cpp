/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
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

#include <stdexcept>

#include "utility/sample_buffer.h"

void SampleBuffer::put(SampleBuffer::Iterator begin, SampleBuffer::Iterator end)
{
	size_t elems = end - begin;
	if (elems > m_buffer.size())
		throw std::out_of_range("Size of the buffer is smaller than the amount of elements");

	size_t free_elems = m_buffer.size() - m_offset;
	if (elems > free_elems)
	{
		size_t to_remove = elems - free_elems;
		std::copy(m_buffer.begin() + to_remove, m_buffer.end() - free_elems,
		          m_buffer.begin());
		m_offset -= to_remove;
	}
	std::copy(begin, end, m_buffer.begin() + m_offset);
	m_offset += elems;
}

size_t SampleBuffer::get(size_t elems, std::vector<int16_t> &dest)
{
	if (m_offset == 0)
		return 0;

	// If the amount of requested samples is bigger than available, return only
	// available.
	if (elems > m_offset)
		elems = m_offset;

	if (elems >= dest.size())
	{
		// If dest is smaller than the available amount of samples, discard the
		// ones that come first.
		size_t elems_lost = elems - dest.size();
		std::copy(m_buffer.begin() + elems_lost, m_buffer.begin() + elems, dest.begin());
	}
	else
	{
		// Copy samples to the destination buffer.
		std::copy(dest.begin() + elems, dest.end(), dest.begin());
		std::copy(m_buffer.begin(), m_buffer.begin() + elems, dest.end() - elems);
	}

	// Remove them from the internal buffer.
	std::copy(m_buffer.begin() + elems, m_buffer.begin() + m_offset, m_buffer.begin());
	m_offset -= elems;

	return elems;
}

void SampleBuffer::resize(size_t n)
{
	m_buffer.resize(n);
	clear();
}

void SampleBuffer::clear()
{
	m_offset = 0;
}

size_t SampleBuffer::size() const
{
	return m_offset;
}

const std::vector<int16_t> &SampleBuffer::buffer() const
{
	return m_buffer;
}
