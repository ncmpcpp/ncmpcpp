/***************************************************************************
 *   Copyright (C) 2026-Now  rbrgmn <roman.bergman@tutanota.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef NCMPCPP_MPRIS_DBUS_H
#define NCMPCPP_MPRIS_DBUS_H

namespace NC {
struct Window;
}

namespace MPRIS {

void initialize();
void shutdown();
void registerFDCallback(NC::Window &w);
void process();

} // namespace MPRIS

#endif // NCMPCPP_MPRIS_DBUS_H
