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

#ifndef QUATTRO_H
#define QUATTRO_H

#include <vector>

#include <librevenge-stream/librevenge-stream.h>
#include "libwps/libwps.h"

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WKSParser.h"

namespace QuattroParserInternal
{
class SubDocument;
struct State;
}

class QuattroSpreadsheet;

/**
 * This class parses Quattro Pro spreadsheet: .wq1 and wq2
 *
 */
class QuattroParser : public WKSParser
{
	friend class QuattroParserInternal::SubDocument;
	friend class QuattroSpreadsheet;
public:
	//! constructor
	QuattroParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
	              libwps_tools_win::Font::Type encoding=libwps_tools_win::Font::UNKNOWN);
	//! destructor
	~QuattroParser();
	//! called by WPSDocument to parse the file
	void parse(librevenge::RVNGSpreadsheetInterface *documentInterface);
	//! checks if the document header is correct (or not)
	bool checkHeader(WPSHeader *header, bool strict=false);

protected:
	//! return true if the pos is in the file, update the file size if need
	bool checkFilePosition(long pos);
	//! return the file version
	int version() const;
	/** returns the default font type, ie. the encoding given by the constructor if given
		or the encoding deduiced from the version.
	 */
	libwps_tools_win::Font::Type getDefaultFontType() const;
	//! returns the true if the file has LICS characters
	bool hasLICSCharacters() const;

	//
	// interface with QuattroSpreadsheet
	//

	//! returns the color corresponding to an id
	bool getColor(int id, WPSColor &color) const;
	//! returns the font corresponding to an id
	bool getFont(int id, WPSFont &font, libwps_tools_win::Font::Type &type) const;

	/** creates the main listener */
	shared_ptr<WKSContentListener> createListener(librevenge::RVNGSpreadsheetInterface *interface);
	//! send the header/footer
	void sendHeaderFooter(bool header);

	//
	// low level
	//

	/** finds the different zones (spreadsheet, chart, print, ...) */
	bool readZones();
	//! reads a zone
	bool readZone();

	//////////////////////// generic ////////////////////////////////////

	//! reads the user fonts
	bool readUserFonts();

	//! reads the header/footer
	bool readHeaderFooter(bool header);

	//! read a list of field name + ...
	bool readFieldName();

	//////////////////////// chart zone //////////////////////////////

	//! reads the chart name or title
	bool readChartName();

	//! reads a structure which seems to define a chart
	bool readChartDef();

	//////////////////////// unknown zone //////////////////////////////

	//! reads windows record 0:7|0:9
	bool readWindowRecord();
	/** reads some unknown spreadsheet zones 0:18|0:19|0:20|0:27|0:2a

	 \note this zones seems to consist of a list of flags potentially
	 followed by other data*/
	bool readUnknown1();

	shared_ptr<WKSContentListener> m_listener; /** the listener (if set)*/
	//! the internal state
	shared_ptr<QuattroParserInternal::State> m_state;
	//! the spreadsheet manager
	shared_ptr<QuattroSpreadsheet> m_spreadsheetParser;
};

#endif /* WPS4_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
