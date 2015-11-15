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

#ifndef WKS4_SPREADSHEET_H
#define WKS4_SPREADSHEET_H

#include <ostream>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"

#include "WPSDebug.h"
#include "WKSContentListener.h"

namespace WKS4SpreadsheetInternal
{
class Cell;
class SpreadSheet;
struct State;
}

class WKS4Parser;

/**
 * This class parses Microsoft Works spreadsheet file
 *
 */
class WKS4Spreadsheet
{
public:
	friend class WKS4Parser;

	//! constructor
	explicit WKS4Spreadsheet(WKS4Parser &parser);
	//! destructor
	~WKS4Spreadsheet();
	//! sets the listener
	void setListener(WKSContentListenerPtr &listen)
	{
		m_listener = listen;
	}

protected:
	//! return true if the pos is in the file, update the file size if need
	bool checkFilePosition(long pos);
	//! return the file version
	int version() const;
	//! returns the true if the file has LICS characters
	bool hasLICSCharacters() const;

	//! returns the number of spreadsheet
	int getNumSpreadsheets() const;
	//! send the sId'th spreadsheet
	void sendSpreadsheet(int sId);

	//! send the cell data
	void sendCellContent(WKS4SpreadsheetInternal::Cell const &cell);

	//////////////////////// open/close //////////////////////////////

	//! reads the report's header zone 17:54
	bool readReportOpen();
	//! reads the report's end zone 18:54
	bool readReportClose();

	//! reads the filter's header zone 10:54
	bool readFilterOpen();
	//! reads the filter's end zone 11:54
	bool readFilterClose();

	//
	// low level
	//
	//////////////////////// spread sheet //////////////////////////////

	//! reads a cell content data
	bool readCell();
	//! reads the result of a text formula
	bool readCellFormulaResult();
	//! reads sheet size
	bool readSheetSize();
	//! reads the column size ( in ??? )
	bool readColumnSize();
	//! reads the list of hidden columns zone ( unused )
	bool readHiddenColumns();

	//! reads a field property
	bool readMsWorksDOSFieldProperty();
	//! reads actualCell properties
	bool readMsWorksDOSCellProperty();
	//! reads the actual cell addendum properties ( contains at least the color)
	bool readMsWorksDOSCellExtraProperty();
	//! reads a page break (in a dos file)
	bool readMsWorksDOSPageBreak();

	//! reads the column size ( in ???)
	bool readMsWorksColumnSize();
	//! reads the row size ( in ???)
	bool readMsWorksRowSize();
	//! reads a page break
	bool readMsWorksPageBreak();
	//! reads a style
	bool readMsWorksStyle();

	/* reads a cell */
	bool readCell(Vec2i actPos, WKSContentListener::FormulaInstruction &instr);
	/* reads a formula */
	bool readFormula(long endPos, Vec2i const &pos,
	                 std::vector<WKSContentListener::FormulaInstruction> &formula, std::string &error);

private:
	WKS4Spreadsheet(WKS4Spreadsheet const &orig);
	WKS4Spreadsheet &operator=(WKS4Spreadsheet const &orig);
	//! returns the debug file
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}
	/** the input */
	RVNGInputStreamPtr m_input;
	shared_ptr<WKSContentListener> m_listener; /** the listener (if set)*/
	//! the main parser
	WKS4Parser &m_mainParser;
	//! the internal state
	shared_ptr<WKS4SpreadsheetInternal::State> m_state;
	//! the ascii file
	libwps::DebugFile &m_asciiFile;
};

#endif /* WPS4_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
