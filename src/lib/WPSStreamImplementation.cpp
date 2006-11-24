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

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include "WPSStreamImplementation.h"
#include "WPSOLEStream.h"
#include "libwps.h"

#include <fstream>
#include <sstream>
#include <string>

namespace libwps
{

class WPSFileStreamPrivate
{
public:
	WPSFileStreamPrivate();
	std::fstream file;
	std::stringstream buffer;
};

class WPSMemoryStreamPrivate
{
public:
	WPSMemoryStreamPrivate(const std::string str);
	std::stringstream buffer;
};

} // namespace libwps

libwps::WPSFileStreamPrivate::WPSFileStreamPrivate() :
	file(),
	buffer(std::ios::binary | std::ios::in | std::ios::out)
{
}

libwps::WPSMemoryStreamPrivate::WPSMemoryStreamPrivate(const std::string str) :
	buffer(str, std::ios::binary | std::ios::in)
{
}


libwps::WPSFileStream::WPSFileStream(const char* filename)
{
	d = new libwps::WPSFileStreamPrivate;
	
	d->file.open( filename, std::ios::binary | std::ios::in );
}

libwps::WPSFileStream::~WPSFileStream()
{
	delete d;
}

unsigned char libwps::WPSFileStream::getchar()
{
	return d->file.get();
}

long libwps::WPSFileStream::read(long nbytes, char* buffer)
{
	long nread = 0;
	
	if(d->file.good())
	{
	long curpos = d->file.tellg();
	d->file.read(buffer, nbytes); 
	nread = (long)d->file.tellg() - curpos;
	}
	
	return nread;
}

long libwps::WPSFileStream::tell()
{
	return d->file.good() ? (long)d->file.tellg() : -1L;
}

void libwps::WPSFileStream::seek(long offset)
{
	if(d->file.good())
		d->file.seekg(offset);
}

bool libwps::WPSFileStream::atEnd()
{
	return d->file.eof();
}

bool libwps::WPSFileStream::isOle()
{
	if (d->buffer.str().empty())
		d->buffer << d->file.rdbuf();
	Storage tmpStorage( d->buffer );
	if (tmpStorage.isOle())
		return true;
	return false;
}

libwps::WPSInputStream* libwps::WPSFileStream::getWPSOleStream(char * name)
{
	if (d->buffer.str().empty())
		d->buffer << d->file.rdbuf();
	libwps::Storage *tmpStorage = new libwps::Storage( d->buffer );
	libwps::Stream tmpStream( tmpStorage, name );
	if (!tmpStorage || (tmpStorage->result() != Storage::Ok)  || !tmpStream.size())
	{
		if (tmpStorage)
			delete tmpStorage;
		return (libwps::WPSInputStream*)0;
	}
	
	unsigned char *tmpBuffer = new unsigned char[tmpStream.size()];
	unsigned long tmpLength;
	tmpLength = tmpStream.read(tmpBuffer, tmpStream.size());

	// sanity check
	if (tmpLength > tmpStream.size() || tmpLength < tmpStream.size())
	/* something went wrong here and we do not trust the
	   resulting buffer */
	{
		if (tmpStorage)
			delete tmpStorage;
		return (libwps::WPSInputStream*)0;
	}

	delete tmpStorage;
	return new libwps::WPSMemoryStream((const char *)tmpBuffer, tmpLength);
}


libwps::WPSMemoryStream::WPSMemoryStream(const char *data, const unsigned int dataSize)
{
	d = new libwps::WPSMemoryStreamPrivate(std::string(data, dataSize));
}

libwps::WPSMemoryStream::~WPSMemoryStream()
{
	delete d;
}

unsigned char libwps::WPSMemoryStream::getchar()
{
	return d->buffer.get();
}

long libwps::WPSMemoryStream::read(long nbytes, char* buffer)
{
	long nread = 0;
	
	if(d->buffer.good())
	{
	long curpos = d->buffer.tellg();
	d->buffer.read(buffer, nbytes); 
	nread = (long)d->buffer.tellg() - curpos;
	}
	
	return nread;
}

long libwps::WPSMemoryStream::tell()
{
	return d->buffer.good() ? (long)d->buffer.tellg() : -1L;
}

void libwps::WPSMemoryStream::seek(long offset)
{
	if(d->buffer.good())
		d->buffer.seekg(offset);
}

bool libwps::WPSMemoryStream::atEnd()
{
	return d->buffer.eof();
}

bool libwps::WPSMemoryStream::isOle()
{
	libwps::Storage tmpStorage( d->buffer );
	if (tmpStorage.isOle())
		return true;
	return false;
}

libwps::WPSInputStream* libwps::WPSMemoryStream::getWPSOleStream(char * name)
{
	libwps::Storage *tmpStorage = new libwps::Storage( d->buffer );
	libwps::Stream tmpStream( tmpStorage, name );
	if (!tmpStorage || (tmpStorage->result() != libwps::Storage::Ok)  || !tmpStream.size())
	{
		if (tmpStorage)
			delete tmpStorage;
		return (libwps::WPSInputStream*)0;
	}
	
	unsigned char *tmpBuffer = new unsigned char[tmpStream.size()];
	unsigned long tmpLength;
	tmpLength = tmpStream.read(tmpBuffer, tmpStream.size());

	// sanity check
	if (tmpLength > tmpStream.size() || tmpLength < tmpStream.size())
	/* something went wrong here and we do not trust the
	   resulting buffer */
	{
		if (tmpStorage)
			delete tmpStorage;
		return (libwps::WPSInputStream*)0;
	}

	delete tmpStorage;
	return new libwps::WPSMemoryStream((const char *)tmpBuffer, tmpLength);
}
