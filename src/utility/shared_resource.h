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

#ifndef NCMPCPP_UTILITY_SHARED_RESOURCE_H
#define NCMPCPP_UTILITY_SHARED_RESOURCE_H

#include <mutex>

template <typename ResourceT>
struct Shared
{
	struct Resource
	{
		Resource(std::mutex &mutex, ResourceT &resource)
			: m_lock(std::unique_lock<std::mutex>(mutex)), m_resource(resource)
		{ }

		ResourceT &operator*() { return m_resource; }
		const ResourceT &operator*() const { return m_resource; }

		ResourceT *operator->() { return &m_resource; }
		const ResourceT *operator->() const { return &m_resource; }

	private:
		std::unique_lock<std::mutex> m_lock;
		ResourceT &m_resource;
	};

	Shared(){ }

	template <typename ValueT>
	Shared(ValueT &&value)
		: m_resource(std::forward<ValueT>(value))
	{ }

	Resource acquire() { return Resource(m_mutex, m_resource); }

private:
	std::mutex m_mutex;
	ResourceT m_resource;
};

#endif // NCMPCPP_UTILITY_SHARED_RESOURCE_H
