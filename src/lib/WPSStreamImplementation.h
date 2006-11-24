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

namespace libwps
{

class WPSFileStreamPrivate;

class WPSFileStream: public WPSInputStream
{
public:
	explicit WPSFileStream(const char* filename);
	~WPSFileStream();
	
	unsigned char getchar();
	long read(long n, char* buffer);
	long tell();
	void seek(long offset);
	bool atEnd();

	bool isOle();
	WPSInputStream *getWPSOleStream(char * name);

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

	unsigned char getchar();
	long read(long n, char* buffer);
	long tell();
	void seek(long offset);
	bool atEnd();

	bool isOle();
	WPSInputStream *getWPSOleStream(char * name);

private:
	WPSMemoryStreamPrivate* d;
	WPSMemoryStream(const WPSMemoryStream&); // copy is not allowed
	WPSMemoryStream& operator=(const WPSMemoryStream&); // assignment is not allowed
};

} // namespace libwps

#endif // __WPSSTREAMIMPLEMENTATION_H__
