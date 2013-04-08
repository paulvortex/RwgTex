/*
	DDS GIMP plugin

	Copyright (C) 2004 Shawn Kirst <skirst@gmail.com>,
   with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA.
*/

#ifndef __DDSPLUGIN_H
#define __DDSPLUGIN_H

#define DDS_PLUGIN_VERSION_MAJOR     2
#define DDS_PLUGIN_VERSION_MINOR     2
#define DDS_PLUGIN_VERSION_REVISION  1

#define DDS_PLUGIN_VERSION  \
   ((unsigned int)(DDS_PLUGIN_VERSION_MAJOR << 16) | \
    (unsigned int)(DDS_PLUGIN_VERSION_MINOR <<  8) | \
    (unsigned int)(DDS_PLUGIN_VERSION_REVISION))

#endif
