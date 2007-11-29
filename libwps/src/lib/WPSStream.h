/* libwps
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02111-1301 USA
 *
 * For further information visit http://libwps.sourceforge.net
 */

#ifndef __WPSSTREAM_H__
#define __WPSSTREAM_H__

#include <libwpd/WPXStream.h>

class WPSInputStream: public WPXInputStream
{
public:
	WPSInputStream() : WPXInputStream(true) {}
	virtual ~WPSInputStream() {}
	virtual const uint8_t *read(size_t numBytes, size_t &numBytesRead) = 0;
	virtual long tell() = 0;
	virtual int seek(long offset, WPX_SEEK_TYPE seekType) = 0;
	virtual bool atEOS() = 0;

	virtual bool isOLEStream() = 0;
	virtual WPXInputStream *getDocumentOLEStream(const char * name) = 0;
	virtual WPXInputStream *getDocumentOLEStream() = 0;
};
#endif // __WPSSTREAM_H__
