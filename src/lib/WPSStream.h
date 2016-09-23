/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 */

#ifndef WPS_STREAM_H
#define WPS_STREAM_H

#include "libwps_internal.h"
#include "WPSDebug.h"

//! small structure use to store a stream and it debug file
struct WPSStream
{
	//! constructor with an ascii file
	WPSStream(RVNGInputStreamPtr input, libwps::DebugFile &ascii);
	//! constructor without an ascii file
	explicit WPSStream(RVNGInputStreamPtr input);
	//! destructor
	~WPSStream();
	//! return true if the position is in the file
	bool checkFilePosition(long pos) const
	{
		return pos<=m_eof;
	}
	//! the stream
	RVNGInputStreamPtr m_input;
	//! the ascii file
	libwps::DebugFile &m_ascii;
	//! the last position
	long m_eof;
protected:
	//! the local file(if not, is given)
	libwps::DebugFile m_asciiFile;
};

#endif /* WPS_STREAM_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
