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

#ifndef __WPSSTREAMIMPLEMENTATION_H__
#define __WPSSTREAMIMPLEMENTATION_H__

#include "WPSStream.h"

class WPSFileStreamPrivate;

class WPSFileStream: public WPSInputStream
{
public:
	explicit WPSFileStream(const char* filename);
	~WPSFileStream();
	
	const uint8_t *read(size_t numBytes, size_t &numBytesRead);
	long tell();
	int seek(long offset, WPX_SEEK_TYPE seekType);
	bool atEOS();

	bool isOLEStream();
	WPXInputStream *getDocumentOLEStream(const char * name);
	WPXInputStream *getDocumentOLEStream();

private:
	WPSFileStreamPrivate* d;
	WPSFileStream(const WPSFileStream&); // copy is not allowed
	WPSFileStream& operator=(const WPSFileStream&); // assignment is not allowed
};

class WPSMemoryStreamPrivate;

class WPSMemoryStream: public WPSInputStream
{
public:
	WPSMemoryStream(const char *data, const unsigned int dataSize);
	~WPSMemoryStream();

	const uint8_t *read(size_t numBytes, size_t &numBytesRead);
	long tell();
	int seek(long offset, WPX_SEEK_TYPE seekType);
	bool atEOS();

	bool isOLEStream();
	WPXInputStream *getDocumentOLEStream(const char * name);
	WPXInputStream *getDocumentOLEStream();

private:
	WPSMemoryStreamPrivate* d;
	WPSMemoryStream(const WPSMemoryStream&); // copy is not allowed
	WPSMemoryStream& operator=(const WPSMemoryStream&); // assignment is not allowed
};

#endif // __WPSSTREAMIMPLEMENTATION_H__
