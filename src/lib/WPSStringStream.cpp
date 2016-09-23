/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwps.sourceforge.net
 */

#include <cstring>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "WPSStringStream.h"

//! internal data of a WPSStringStream
class WPSStringStreamPrivate
{
public:
	//! constructor
	WPSStringStreamPrivate(const unsigned char *data, unsigned dataSize);
	//! destructor
	~WPSStringStreamPrivate();
	//! append some data at the end of the actual stream
	void append(const unsigned char *data, unsigned dataSize);
	//! the stream buffer
	std::vector<unsigned char> m_buffer;
	//! the stream offset
	long m_offset;
private:
	WPSStringStreamPrivate(const WPSStringStreamPrivate &);
	WPSStringStreamPrivate &operator=(const WPSStringStreamPrivate &);
};

WPSStringStreamPrivate::WPSStringStreamPrivate(const unsigned char *data, unsigned dataSize) :
	m_buffer(0),
	m_offset(0)
{
	if (data && dataSize)
	{
		m_buffer.resize(dataSize);
		std::memcpy(&m_buffer[0], data, dataSize);
	}
}

WPSStringStreamPrivate::~WPSStringStreamPrivate()
{
}

void WPSStringStreamPrivate::append(const unsigned char *data, unsigned dataSize)
{
	if (!dataSize) return;
	size_t actualSize=m_buffer.size();
	m_buffer.resize(actualSize+size_t(dataSize));
	std::memcpy(&m_buffer[actualSize], data, dataSize);
}

WPSStringStream::WPSStringStream(const unsigned char *data, const unsigned int dataSize) :
	librevenge::RVNGInputStream(),
	m_data(new WPSStringStreamPrivate(data, dataSize))
{
}

WPSStringStream::~WPSStringStream()
{
	if (m_data) delete m_data;
}

void WPSStringStream::append(const unsigned char *data, const unsigned int dataSize)
{
	if (m_data) m_data->append(data, dataSize);
}

const unsigned char *WPSStringStream::read(unsigned long numBytes, unsigned long &numBytesRead)
{
	numBytesRead = 0;

	if (numBytes == 0 || !m_data)
		return 0;

	long numBytesToRead;

	if (static_cast<unsigned long>(m_data->m_offset)+numBytes < m_data->m_buffer.size())
		numBytesToRead = long(numBytes);
	else
		numBytesToRead = long(m_data->m_buffer.size()) - m_data->m_offset;

	numBytesRead = static_cast<unsigned long>(numBytesToRead); // about as paranoid as we can be..

	if (numBytesToRead == 0)
		return 0;

	long oldOffset = m_data->m_offset;
	m_data->m_offset += numBytesToRead;

	return &m_data->m_buffer[size_t(oldOffset)];

}

long WPSStringStream::tell()
{
	return m_data ? m_data->m_offset : 0;
}

int WPSStringStream::seek(long offset, librevenge::RVNG_SEEK_TYPE seekType)
{
	if (!m_data) return -1;
	if (seekType == librevenge::RVNG_SEEK_CUR)
		m_data->m_offset += offset;
	else if (seekType == librevenge::RVNG_SEEK_SET)
		m_data->m_offset = offset;
	else if (seekType == librevenge::RVNG_SEEK_END)
		m_data->m_offset = offset+long(m_data->m_buffer.size());

	if (m_data->m_offset < 0)
	{
		m_data->m_offset = 0;
		return -1;
	}
	if (long(m_data->m_offset) > long(m_data->m_buffer.size()))
	{
		m_data->m_offset = long(m_data->m_buffer.size());
		return -1;
	}

	return 0;
}

bool WPSStringStream::isEnd()
{
	if (!m_data || long(m_data->m_offset) >= long(m_data->m_buffer.size()))
		return true;

	return false;
}

bool WPSStringStream::isStructured()
{
	return false;
}

unsigned WPSStringStream::subStreamCount()
{
	return 0;
}

const char *WPSStringStream::subStreamName(unsigned)
{
	return 0;
}

bool WPSStringStream::existsSubStream(const char *)
{
	return false;
}

librevenge::RVNGInputStream *WPSStringStream::getSubStreamById(unsigned)
{
	return 0;
}

librevenge::RVNGInputStream *WPSStringStream::getSubStreamByName(const char *)
{
	return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
