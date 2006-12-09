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

#include "WPSStreamImplementation.h"
#include "WPSOLEStream.h"
#include "libwps.h"

#include <fstream>
#include <sstream>
#include <string>

using namespace libwps;

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

WPSFileStreamPrivate::WPSFileStreamPrivate() :
	file(),
	buffer(std::ios::binary | std::ios::in | std::ios::out)
{
}

WPSMemoryStreamPrivate::WPSMemoryStreamPrivate(const std::string str) :
	buffer(str, std::ios::binary | std::ios::in)
{
}


WPSFileStream::WPSFileStream(const char* filename)
{
	d = new WPSFileStreamPrivate;
	
	d->file.open( filename, std::ios::binary | std::ios::in );
}

WPSFileStream::~WPSFileStream()
{
	delete d;
}

const uint8_t *WPSFileStream::read(size_t numBytes, size_t &numBytesRead)
{
	numBytesRead = 0;
	
	if (numBytes < 0)
	{
		return 0;
	}

	uint8_t *buffer = new uint8_t[numBytes];

	if(d->file.good())
	{
		long curpos = d->file.tellg();
		d->file.readsome((char *)buffer, numBytes); 
		numBytesRead = (long)d->file.tellg() - curpos;
	}
	
	return buffer;
}

long WPSFileStream::tell()
{
	return d->file.good() ? (long)d->file.tellg() : -1L;
}

int WPSFileStream::seek(long offset, WPX_SEEK_TYPE seekType)
{
	if (seekType == WPX_SEEK_SET && offset < 0)
		offset = 0;
	if (seekType == WPX_SEEK_CUR && (tell() + offset < 0))
		offset = -tell();
	if(d->file.good())
		d->file.seekg((seekType == WPX_SEEK_SET) ? offset : (offset + tell()));
}

bool WPSFileStream::atEOS()
{
	return d->file.eof();
}

bool WPSFileStream::isOLEStream()
{
	if (d->buffer.str().empty())
		d->buffer << d->file.rdbuf();
	Storage tmpStorage( d->buffer );
	if (tmpStorage.isOLEStream())
		return true;
	return false;
}

WPSInputStream* WPSFileStream::getDocumentOLEStream(const char * name)
{
	if (d->buffer.str().empty())
		d->buffer << d->file.rdbuf();
	Storage *tmpStorage = new Storage( d->buffer );
	Stream tmpStream( tmpStorage, name );
	if (!tmpStorage || (tmpStorage->result() != Storage::Ok)  || !tmpStream.size())
	{
		if (tmpStorage)
			delete tmpStorage;
		return (WPSInputStream*)0;
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
		return (WPSInputStream*)0;
	}

	delete tmpStorage;
	return new WPSMemoryStream((const char *)tmpBuffer, tmpLength);
}


WPSMemoryStream::WPSMemoryStream(const char *data, const unsigned int dataSize)
{
	d = new WPSMemoryStreamPrivate(std::string(data, dataSize));
}

WPSMemoryStream::~WPSMemoryStream()
{
	delete d;
}

const uint8_t *WPSMemoryStream::read(size_t numBytes, size_t &numBytesRead)
{
	numBytesRead = 0;
	
	if (numBytes < 0)
	{
		return 0;
	}

	uint8_t *buffer = new uint8_t[numBytes];

	if(d->buffer.good())
	{
		long curpos = d->buffer.tellg();
		d->buffer.readsome((char *)buffer, numBytes); 
		numBytesRead = (long)d->buffer.tellg() - curpos;
	}
	
	return buffer;
}

long WPSMemoryStream::tell()
{
	return d->buffer.good() ? (long)d->buffer.tellg() : -1L;
}

int WPSMemoryStream::seek(long offset, WPX_SEEK_TYPE seekType)
{
	if (seekType == WPX_SEEK_SET && offset < 0)
		offset = 0;
	if (seekType == WPX_SEEK_CUR && (tell() + offset < 0))
		offset = -tell();
	if(d->buffer.good())
		d->buffer.seekg((seekType == WPX_SEEK_SET) ? offset : (offset + tell()));
}

bool WPSMemoryStream::atEOS()
{
	return d->buffer.eof();
}

bool WPSMemoryStream::isOLEStream()
{
	Storage tmpStorage( d->buffer );
	if (tmpStorage.isOLEStream())
		return true;
	return false;
}

WPSInputStream* WPSMemoryStream::getDocumentOLEStream(const char * name)
{
	Storage *tmpStorage = new Storage( d->buffer );
	Stream tmpStream( tmpStorage, name );
	if (!tmpStorage || (tmpStorage->result() != Storage::Ok)  || !tmpStream.size())
	{
		if (tmpStorage)
			delete tmpStorage;
		return (WPSInputStream*)0;
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
		return (WPSInputStream*)0;
	}

	delete tmpStorage;
	return new WPSMemoryStream((const char *)tmpBuffer, tmpLength);
}
