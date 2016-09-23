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

#ifndef WPS_STRING_STREAM_H
#define WPS_STRING_STREAM_H

#include <librevenge-stream/librevenge-stream.h>

class WPSStringStreamPrivate;

/** internal class used to create a RVNGInputStream from a unsigned char's pointer

    \note this class (highly inspired from librevenge) does not
    implement the isStructured's protocol, ie. it only returns false.
 */
class WPSStringStream: public librevenge::RVNGInputStream
{
public:
	//! constructor
	WPSStringStream(const unsigned char *data, const unsigned int dataSize);
	//! destructor
	~WPSStringStream();

	//! append some data at the end of the string
	void append(const unsigned char *data, const unsigned int dataSize);
	/**! reads numbytes data.

	 * \return a pointer to the read elements
	 */
	const unsigned char *read(unsigned long numBytes, unsigned long &numBytesRead);
	//! returns actual offset position
	long tell();
	/*! \brief seeks to a offset position, from actual, beginning or ending position
	 * \return 0 if ok
	 */
	int seek(long offset, librevenge::RVNG_SEEK_TYPE seekType);
	//! returns true if we are at the end of the section/file
	bool isEnd();

	/** returns true if the stream is ole

	 \sa returns always false*/
	bool isStructured();
	/** returns the number of sub streams.

	 \sa returns always 0*/
	unsigned subStreamCount();
	/** returns the ith sub streams name

	 \sa returns always 0*/
	const char *subStreamName(unsigned);
	/** returns true if a substream with name exists

	 \sa returns always false*/
	bool existsSubStream(const char *name);
	/** return a new stream for a ole zone

	 \sa returns always 0 */
	librevenge::RVNGInputStream *getSubStreamByName(const char *name);
	/** return a new stream for a ole zone

	 \sa returns always 0 */
	librevenge::RVNGInputStream *getSubStreamById(unsigned);

private:
	/// the string stream data
	WPSStringStreamPrivate *m_data;
	WPSStringStream(const WPSStringStream &); // copy is not allowed
	WPSStringStream &operator=(const WPSStringStream &); // assignment is not allowed
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
