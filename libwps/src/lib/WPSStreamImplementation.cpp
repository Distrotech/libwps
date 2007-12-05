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
#include <limits>

using namespace libwps;

class WPSFileStreamPrivate
{
public:
	WPSFileStreamPrivate();
	~WPSFileStreamPrivate();
	std::fstream file;
	std::stringstream buffer;
	unsigned long streamSize;
	uint8_t *buf;
	uint8_t *readBuffer;
	unsigned long readBufferLength;
	unsigned long readBufferPos;
private:
	WPSFileStreamPrivate(const WPSFileStreamPrivate&);
	WPSFileStreamPrivate& operator=(const WPSFileStreamPrivate&);
};

class WPSMemoryStreamPrivate
{
public:
	WPSMemoryStreamPrivate(const std::string str);
	~WPSMemoryStreamPrivate();
	std::stringstream buffer;
	unsigned long streamSize;
	uint8_t *buf;
};

WPSFileStreamPrivate::WPSFileStreamPrivate() :
	file(),
	buffer(std::ios::binary | std::ios::in | std::ios::out),
	streamSize(0),
	buf(0),
	readBuffer(0),
	readBufferLength(0),
	readBufferPos(0)
{	
}

WPSFileStreamPrivate::~WPSFileStreamPrivate()
{
	if (buf)
		delete [] buf;
	if (readBuffer)
		delete [] readBuffer;
}

WPSMemoryStreamPrivate::WPSMemoryStreamPrivate(const std::string str) :
	buffer(str, std::ios::binary | std::ios::in),
	streamSize(0),
	buf(0)
{
}

WPSMemoryStreamPrivate::~WPSMemoryStreamPrivate()
{
	if (buf)
		delete [] buf;
}

WPSFileStream::WPSFileStream(const char* filename) :
	d(new WPSFileStreamPrivate)
{
	d->file.open( filename, std::ios::binary | std::ios::in );
	d->file.seekg( 0, std::ios::end );
	d->streamSize = (d->file.good() ? (unsigned long)d->file.tellg() : (unsigned long)-1L);
	if (d->streamSize == (unsigned long)-1) // tellg() returned ERROR
		d->streamSize = 0;
	 // preventing possible unsigned/signed issues later by truncating the file
	if (d->streamSize > (std::numeric_limits<unsigned long>::max)() / 2)
		d->streamSize = (std::numeric_limits<unsigned long>::max)() / 2;
	d->file.seekg( 0, std::ios::beg );
}

WPSFileStream::~WPSFileStream()
{
	delete d;
}

#define BUFFER_MAX 65536

const uint8_t *WPSFileStream::read(size_t numBytes, size_t &numBytesRead)
{
	numBytesRead = 0;
	
	if (numBytes == 0 || /* atEOS() || */ numBytes > (std::numeric_limits<unsigned long>::max)()/2
		|| !d->file.good())
		return 0;

	// can we read from the buffer?
	if (d->readBuffer && (d->readBufferPos + numBytes > d->readBufferPos)
		&& (d->readBufferPos + numBytes <= d->readBufferLength))
	{
		const uint8_t *pTmp = d->readBuffer + d->readBufferPos;
		d->readBufferPos += numBytes;
		numBytesRead = numBytes;
		return pTmp;
	}

	// hmm, we cannot: go back by the bytes we read ahead && invalidate the buffer
	if (d->readBuffer)
	{
		d->file.seekg((unsigned long)d->file.tellg() - d->readBufferLength, std::ios::beg);
		d->file.seekg(d->readBufferPos, std::ios::cur);
		delete [] d->readBuffer;
		d->readBuffer = 0; d->readBufferPos = 0; d->readBufferLength = 0;
	}
	
	unsigned long curpos = tell();
	if (curpos == (unsigned long)-1)  // tellg() returned ERROR
		return 0;

	if ( (curpos + numBytes < curpos) /*overflow*/ ||
		(curpos + numBytes >= d->streamSize) ) /*reading more than available*/
	{
		numBytes = d->streamSize - curpos;
	}

	if (numBytes < BUFFER_MAX)
	{
		if (BUFFER_MAX < d->streamSize - curpos)
			d->readBufferLength = BUFFER_MAX;
		else /* BUFFER_MAX >= d->streamSize - curpos */
			d->readBufferLength = d->streamSize - curpos;
	}
	else
		d->readBufferLength = numBytes;
	
	d->file.seekg(d->readBufferLength, std::ios::cur);
	d->file.seekg(curpos, std::ios::beg);
	
	d->readBuffer = new uint8_t[d->readBufferLength];
	d->file.read((char *)(d->readBuffer), d->readBufferLength);
	
	if (!d->file.good())
		d->file.clear();
	d->readBufferPos = 0;
	if (d->readBufferLength == 0)
		return 0;

	numBytesRead = numBytes;
		
	d->readBufferPos += numBytesRead;
	return const_cast<const uint8_t*>( d->readBuffer );
}

long WPSFileStream::tell()
{
	return d->file.good() ? (long)((long)d->file.tellg() - d->readBufferLength + d->readBufferPos) : -1L;
}

int WPSFileStream::seek(long offset, WPX_SEEK_TYPE seekType)
{
	if (seekType == WPX_SEEK_CUR)
		offset += tell();

	if (offset < 0)
		offset = 0;
	if (offset > (long)d->streamSize)
		offset = (long)d->streamSize;
	
	if (d->file.good() && offset < d->file.tellg() && (unsigned long)offset >= (unsigned long)d->file.tellg() - d->readBufferLength)
	{
		d->readBufferPos = offset + d->readBufferLength - d->file.tellg();
		return 0;
	}
	
	if (d->readBuffer) // seeking outside of the buffer, so invalidate the buffer
	{
		d->file.seekg((unsigned long)d->file.tellg() - d->readBufferLength, std::ios::beg);
		d->file.seekg(d->readBufferPos, std::ios::cur);
		delete [] d->readBuffer;
		d->readBuffer = 0; d->readBufferPos = 0; d->readBufferLength = 0;
	}
	
	if(d->file.good())
	{
		d->file.seekg(offset, std::ios::beg);
		return (int) ((long)d->file.tellg() == -1) ;
	}
	else
		return -1;
}

bool WPSFileStream::atEOS()
{
	return (tell() >= (long)d->streamSize);
}

bool WPSFileStream::isOLEStream()
{
	if (d->readBuffer)
	{
		d->file.seekg((unsigned long)d->file.tellg() - d->readBufferLength, std::ios::beg);
		d->file.seekg(d->readBufferPos, std::ios::cur);
		delete [] d->readBuffer;
		d->readBuffer = 0; d->readBufferPos = 0; d->readBufferLength = 0;
	}
	
	if (d->buffer.str().empty())
		d->buffer << d->file.rdbuf();
	Storage tmpStorage( d->buffer );
	seek(0, WPX_SEEK_SET);
	if (tmpStorage.isOLEStream())
		return true;
	return false;
}

WPXInputStream* WPSFileStream::getDocumentOLEStream(const char * name)
{
	if (d->readBuffer)
	{
		d->file.seekg((unsigned long)d->file.tellg() - d->readBufferLength, std::ios::beg);
		d->file.seekg(d->readBufferPos, std::ios::cur);
		delete [] d->readBuffer;
		d->readBuffer = 0; d->readBufferPos = 0; d->readBufferLength = 0;
	}
	
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
	
	if (d->buf)
		delete [] d->buf;
	d->buf = new uint8_t[tmpStream.size()];
	unsigned long tmpLength;
	tmpLength = tmpStream.read((unsigned char *)(d->buf), tmpStream.size());

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
	return new WPSMemoryStream((const char *)(d->buf), tmpLength);
}

WPXInputStream* WPSFileStream::getDocumentOLEStream()
{
	return getDocumentOLEStream("PerfectOffice_MAIN");
}

WPSMemoryStream::WPSMemoryStream(const char *data, const unsigned int dataSize)
{
	d = new WPSMemoryStreamPrivate(std::string(data, dataSize));
	d->buffer.seekg( 0, std::ios::end );
	d->streamSize = (d->buffer.good() ? (unsigned long)d->buffer.tellg() : (unsigned long)-1L);
	if (d->streamSize == (unsigned long)-1L)
		d->streamSize = 0;
	d->buffer.seekg( 0, std::ios::beg );
}

WPSMemoryStream::~WPSMemoryStream()
{
	delete d;
}

const uint8_t *WPSMemoryStream::read(size_t numBytes, size_t &numBytesRead)
{
	numBytesRead = 0;
	
	if (0 == numBytes)
		return 0;
	
	if (numBytes < 0 || atEOS() || numBytes > (std::numeric_limits<unsigned long>::max)()/2)
		return 0;

	unsigned long curpos = d->buffer.tellg();
	if (curpos == (unsigned long)-1) //tellg() returned ERROR
		return 0;

	if ( (curpos + numBytes < curpos) /*overflow*/ ||
		(curpos + numBytes > d->streamSize) ) /*reading more than available*/
	{
		numBytes = d->streamSize - curpos;
	}

	if (d->buf)
		delete [] d->buf;
	d->buf = new uint8_t[numBytes];

	if(d->buffer.good())
	{
		d->buffer.read((char *)(d->buf), numBytes); 
		numBytesRead = (long)d->buffer.tellg() - curpos;
	}
	
	return d->buf;
}

long WPSMemoryStream::tell()
{
	return d->buffer.good() ? (long)d->buffer.tellg() : -1L;
}

int WPSMemoryStream::seek(long offset, WPX_SEEK_TYPE seekType)
{
	if (seekType == WPX_SEEK_SET)
	{
		if (offset < 0)
			offset = 0;
		if (offset > d->streamSize)
			offset = d->streamSize;
	}

	if (seekType == WPX_SEEK_CUR)
	{
		if (tell() + offset < 0)
			offset = -tell();
		if (tell() + offset > d->streamSize)
			offset = d->streamSize - tell();
	}

	if(d->buffer.good())
	{
		d->buffer.seekg(offset, ((seekType == WPX_SEEK_SET) ? std::ios::beg : std::ios::cur));
		return (int) ( (long)d->buffer.tellg() == -1);
	}
	else
		return -1;
}

bool WPSMemoryStream::atEOS()
{
	return (d->buffer.tellg() >= d->streamSize);
}

bool WPSMemoryStream::isOLEStream()
{
	Storage tmpStorage( d->buffer );
	if (tmpStorage.isOLEStream())
	{
		seek(0, WPX_SEEK_SET);
		return true;
	}
	seek(0, WPX_SEEK_SET);
	return false;
}

WPXInputStream* WPSMemoryStream::getDocumentOLEStream(const char * name)
{
	Storage *tmpStorage = new Storage( d->buffer );
	Stream tmpStream( tmpStorage, name );
	if (!tmpStorage || (tmpStorage->result() != Storage::Ok)  || !tmpStream.size())
	{
		if (tmpStorage)
			delete tmpStorage;
		return (WPSInputStream*)0;
	}

	if (d->buf)
		delete [] d->buf;
	d->buf = new uint8_t[tmpStream.size()];
	unsigned long tmpLength;
	tmpLength = tmpStream.read((unsigned char *)(d->buf), tmpStream.size());

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
	return new WPSMemoryStream((const char *)(d->buf), tmpLength);
}

WPXInputStream* WPSMemoryStream::getDocumentOLEStream()
{
	return getDocumentOLEStream("PerfectOffice_MAIN");
}
