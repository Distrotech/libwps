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

#ifndef QUATTRO_SPREADSHEET_H
#define QUATTRO_SPREADSHEET_H

#include <ostream>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"

#include "WPSDebug.h"
#include "WKSContentListener.h"

namespace QuattroSpreadsheetInternal
{
class Cell;
class SpreadSheet;
struct State;
}

class QuattroParser;

/**
 * This class parses Quattro Pro DOS spreadsheet file
 *
 */
class QuattroSpreadsheet
{
public:
	friend class QuattroParser;

	//! constructor
	explicit QuattroSpreadsheet(QuattroParser &parser);
	//! destructor
	~QuattroSpreadsheet();
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
	void sendCellContent(QuattroSpreadsheetInternal::Cell const &cell);

	//////////////////////// open/close //////////////////////////////

	//! reads a sheet header zone 0:dc (Quattro Pro wq2)
	bool readSpreadsheetOpen();
	//! reads a sheet header zone 0:dd (Quattro Pro wq2)
	bool readSpreadsheetClose();

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
	//! reads a sheet name (zone 0xde), unused...
	bool readSpreadsheetName();
	//! reads the column size ( in ??? )
	bool readColumnSize();
	//! reads the column size ( in points )
	bool readRowSize();
	//! reads the list of hidden columns zone ( unused )
	bool readHiddenColumns();

	//! reads a Quattro Pro property (zone 0x9d)
	bool readCellProperty();
	//! reads a Quattro Pro cell styles (zone 0xd8)
	bool readCellStyle();
	//! reads a Quattro Pro style ( zone 0xc9)
	bool readUserStyle();

	/* reads a cell */
	bool readCell(Vec2i actPos, WKSContentListener::FormulaInstruction &instr, bool hasSheetId=false, int sheetId=0);
	/* reads a formula */
	bool readFormula(long endPos, Vec2i const &pos,	int sheetId,
	                 std::vector<WKSContentListener::FormulaInstruction> &formula, std::string &error);

private:
	QuattroSpreadsheet(QuattroSpreadsheet const &orig);
	QuattroSpreadsheet &operator=(QuattroSpreadsheet const &orig);
	//! returns the debug file
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}
	/** the input */
	RVNGInputStreamPtr m_input;
	shared_ptr<WKSContentListener> m_listener; /** the listener (if set)*/
	//! the main parser
	QuattroParser &m_mainParser;
	//! the internal state
	shared_ptr<QuattroSpreadsheetInternal::State> m_state;
	//! the ascii file
	libwps::DebugFile &m_asciiFile;
};

#endif /* WPS4_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
