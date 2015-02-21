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

#ifndef WKS4_H
#define WKS4_H

#include <vector>

#include <librevenge-stream/librevenge-stream.h>
#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WKSParser.h"

namespace WKS4ParserInternal
{
class SubDocument;
struct State;
}

class WKS4Spreadsheet;

/**
 * This class parses Microsoft Works spreadsheet or a database file
 *
 */
class WKS4Parser : public WKSParser
{
	friend class WKS4ParserInternal::SubDocument;
	friend class WKS4Spreadsheet;
public:
	//! constructor
	WKS4Parser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
	           libwps_tools_win::Font::Type encoding=libwps_tools_win::Font::UNKNOWN);
	//! destructor
	~WKS4Parser();
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
	// interface with WKS4Spreadsheet
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
	//! reads a Quattro Pro zone
	bool readZoneQuattro();

	//////////////////////// generic ////////////////////////////////////

	//! reads the fonts
	bool readFont();

	//! reads a printer data ?
	bool readPrnt();

	//! reads another printer data. Seem simillar to ZZPrnt
	bool readPrn2();

	//! reads the header/footer
	bool readHeaderFooter(bool header);

	//! read a list of field name + ...
	bool readFieldName();

	//////////////////////// chart zone //////////////////////////////

	//! reads the chart name or title
	bool readChartName();

	//! reads a structure which seems to define a chart
	bool readChartDef();

	//! reads a structure which seems to define two chart font (only present in windows file)
	bool readChartFont();

	//! reads a structure which seems to define four chart font (only present in windows file)
	bool readChart2Font();

	//! reads end/begin of chart (only present in windows file)
	bool readChartLimit();

	//! reads a list of int/cellule which seems relative to a chart : CHECKME
	bool readChartList();

	//! reads an unknown structure which seems relative to a chart : CHECKME
	bool readChartUnknown();

	//////////////////////// unknown zone //////////////////////////////

	//! reads windows record 0:7|0:9
	bool readWindowRecord();
	/** reads some unknown spreadsheet zones 0:18|0:19|0:20|0:27|0:2a

	 \note this zones seems to consist of a list of flags potentially
	 followed by other data*/
	bool readUnknown1();

	shared_ptr<WKSContentListener> m_listener; /** the listener (if set)*/
	//! the internal state
	shared_ptr<WKS4ParserInternal::State> m_state;
	//! the spreadsheet manager
	shared_ptr<WKS4Spreadsheet> m_spreadsheetParser;
};

#endif /* WPS4_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
