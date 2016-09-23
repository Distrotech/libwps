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

#ifndef LOTUS_H
#define LOTUS_H

#include <vector>

#include <librevenge-stream/librevenge-stream.h>
#include "libwps_internal.h"
#include "libwps_tools_win.h"
#include "WPSDebug.h"

#include "WKSParser.h"

namespace LotusParserInternal
{
class SubDocument;
struct State;
}

class LotusGraph;
class LotusSpreadsheet;
class LotusStyleManager;

/* .wk3: a spreadsheet is composed in two files
       + a wk3 file which contains the spreadsheet data
       + a fm3 file which contains the different formatings

   .wk4: the file contains three parts:
       + the wk3 previous file
	   + the fm3 file
	   + an unknown part, which may code the file structure,

	   Normally the wk3 and the fm3 are a sequence of small zones,
	   but picture seems to be appeared at random position inside the
	   fm3 part (and even inside some structure fm3 structures...)

	   search for .ole and OLE1

 */

/**
 * This class parses a WK2..WK4 Lotus spreadsheet
 *
 */
class LotusParser : public WKSParser
{
	friend class LotusParserInternal::SubDocument;
	friend class LotusGraph;
	friend class LotusSpreadsheet;
	friend class LotusStyleManager;
public:
	//! constructor
	LotusParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
	            libwps_tools_win::Font::Type encoding=libwps_tools_win::Font::UNKNOWN);
	//! destructor
	~LotusParser();
	//! called by WPSDocument to parse the file
	void parse(librevenge::RVNGSpreadsheetInterface *documentInterface);
	//! checks if the document header is correct (or not)
	bool checkHeader(WPSHeader *header, bool strict=false);

protected:
	//! return the file version
	int version() const;

	//
	// interface
	//

	//! returns the font corresponding to an id
	bool getFont(int id, WPSFont &font, libwps_tools_win::Font::Type &type) const;
	/** returns the default font type, ie. the encoding given by the constructor if given
		or the encoding deduiced from the version.
	 */
	libwps_tools_win::Font::Type getDefaultFontType() const;

	//
	// interface with LotusGraph
	//

	//! return true if the sheet sheetId has some graphic
	bool hasGraphics(int sheetId) const;
	//! send the graphics corresponding to a sheetId
	void sendGraphics(int sheetId);

	/** try to parse the different zones */
	bool createZones(shared_ptr<WPSStream> mainStream);
	/** creates the main listener */
	shared_ptr<WKSContentListener> createListener(librevenge::RVNGSpreadsheetInterface *interface);

	//
	// low level
	//

	/// check for the existence of a format stream, if it exists, parse it
	bool parseFormatStream();

	//! checks if the document header is correct (or not)
	bool checkHeader(shared_ptr<WPSStream> stream, bool mainStream, bool strict);
	/** finds the different zones (spreadsheet, chart, print, ...) */
	bool readZones(shared_ptr<WPSStream> stream);
	/** parse the different zones 1B */
	bool readDataZone(shared_ptr<WPSStream> stream);
	//! reads a zone
	bool readZone(shared_ptr<WPSStream> stream);
	//! parse a wk123 zone
	bool readZoneV3(shared_ptr<WPSStream> stream);
	//////////////////////// generic ////////////////////////////////////

	//! reads a mac font name
	bool readMacFontName(shared_ptr<WPSStream> stream, long endPos);
	//! reads a format style name: b6
	bool readFMTStyleName(shared_ptr<WPSStream> stream);
	//! reads a link
	bool readLinkZone(shared_ptr<WPSStream> stream);
	//! reads a mac document info zone: zone 1b, then 2af8
	bool readDocumentInfoMac(shared_ptr<WPSStream> stream, long endPos);

	//////////////////////// chart zone //////////////////////////////

	//! reads a chart definitions
	bool readChartDefinition(shared_ptr<WPSStream> stream);
	//! reads the chart name or title
	bool readChartName(shared_ptr<WPSStream> stream);

	//////////////////////// unknown zone //////////////////////////////


	shared_ptr<WKSContentListener> m_listener; /** the listener (if set)*/
	//! the internal state
	shared_ptr<LotusParserInternal::State> m_state;
	//! the style manager
	shared_ptr<LotusStyleManager> m_styleManager;
	//! the graph manager
	shared_ptr<LotusGraph> m_graphParser;
	//! the spreadsheet manager
	shared_ptr<LotusSpreadsheet> m_spreadsheetParser;
};

#endif /* LOTUS_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
