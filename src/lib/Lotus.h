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

#include "WKSParser.h"

namespace LotusParserInternal
{
class SubDocument;
struct State;
}

class LotusGraph;
class LotusSpreadsheet;
class LotusStyleManager;

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
	//! return true if the pos is in the file, update the file size if need
	bool checkFilePosition(long pos);
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

	//! send the graphics corresponding to a sheetId
	void sendGraphics(int sheetId);

	/** creates the main listener */
	shared_ptr<WKSContentListener> createListener(librevenge::RVNGSpreadsheetInterface *interface);

	//
	// low level
	//

	/** finds the different zones (spreadsheet, chart, print, ...) */
	bool readZones();
	/** parse the different zones 1B */
	bool readDataZone();
	//! reads a zone
	bool readZone();

	//////////////////////// generic ////////////////////////////////////

	//! reads a font name
	bool readFontName(long endPos);
	//! reads a link
	bool readLinkZone();
	//! reads a mac document info zone: zone 1b, then 2af8
	bool readDocumentInfoMac(long endPos);

	//////////////////////// chart zone //////////////////////////////

	//! reads a chart definitions
	bool readChartDefinition();
	//! reads the chart name or title
	bool readChartName();

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
