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

#ifndef WKS4_FORMAT_H
#define WKS4_FORMAT_H

#include <ostream>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"

#include "WPSDebug.h"
#include "WKSContentListener.h"

namespace WKS4FormatInternal
{
class Cell;
class Format;
struct State;
}

class WKS4Parser;

/**
 * This class parses Lotus format file
 *
 */
class WKS4Format
{
public:
	friend class WKS4Parser;

	//! constructor
	explicit WKS4Format(WKS4Parser &parser, RVNGInputStreamPtr input);
	//! destructor
	~WKS4Format();
	//! try to parse an input
	bool parse();

protected:
	//! return true if the pos is in the file, update the file size if need
	bool checkFilePosition(long pos);

	//
	// low level
	//
	//////////////////////// format zone //////////////////////////////
	//! checks if the document header is correct (or not)
	bool checkHeader(bool strict=false);
	/** finds the different zones */
	bool readZones();
	//! reads a zone
	bool readZone();

	//! reads a format font name: zones 0xae
	bool readFontName();
	//! reads a format font sizes zones 0xaf and 0xb1
	bool readFontSize();
	//! reads a format font id zone: 0xb0
	bool readFontId();

private:
	WKS4Format(WKS4Format const &orig);
	WKS4Format &operator=(WKS4Format const &orig);
	//! returns the debug file
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}
	/** the input */
	RVNGInputStreamPtr m_input;
	//! the main parser
	WKS4Parser &m_mainParser;
	//! the internal state
	shared_ptr<WKS4FormatInternal::State> m_state;
	//! the ascii file
	libwps::DebugFile m_asciiFile;
};

#endif /* WPS4_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
