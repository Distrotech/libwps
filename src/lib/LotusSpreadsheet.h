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

#ifndef LOTUS_SPREADSHEET_H
#define LOTUS_SPREADSHEET_H

#include <ostream>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"

#include "WPSDebug.h"
#include "WKSContentListener.h"

namespace LotusParserInternal
{
struct LotusStream;
}

namespace LotusSpreadsheetInternal
{
class Cell;
class Spreadsheet;
struct Style;
struct State;
}

class LotusParser;
class LotusStyleManager;

/**
 * This class parses Microsoft Works spreadsheet file
 *
 */
class LotusSpreadsheet
{
public:
	friend class LotusParser;

	//! constructor
	explicit LotusSpreadsheet(LotusParser &parser);
	//! destructor
	~LotusSpreadsheet();
	//! clean internal state
	void cleanState();
	//! update internal state (must be called one time before sending data)
	void updateState();
	//! sets the listener
	void setListener(WKSContentListenerPtr &listen)
	{
		m_listener = listen;
	}
	//! set the last spreadsheet number ( default 0)
	void setLastSpreadsheetId(int id);
protected:
	//! return the file version
	int version() const;
	//! returns true if some spreadsheet are defined
	bool hasSomeSpreadsheetData() const;

	//! send the data
	void sendSpreadsheet(int sheetId);

	/** send the cell data in a row

	 \note this function does not call openSheetRow, closeSheetRow*/
	void sendRowContent(LotusSpreadsheetInternal::Spreadsheet const &sheet, int row);
	//! send the cell data
	void sendCellContent(LotusSpreadsheetInternal::Cell const &cell,
	                     LotusSpreadsheetInternal::Style const &style, int numRepeated=1);

	//////////////////////// report //////////////////////////////

	//
	// low level
	//
	//////////////////////// spread sheet //////////////////////////////

	//! reads a sheet name
	bool readSheetName(LotusParserInternal::LotusStream &stream);

	//! reads the columns definitions
	bool readColumnDefinition(LotusParserInternal::LotusStream &stream);
	//! reads the column sizes ( in char )
	bool readColumnSizes(LotusParserInternal::LotusStream &stream);
	//! reads the row formats
	bool readRowFormats(LotusParserInternal::LotusStream &stream);
	//! reads a cell's row format
	bool readRowFormat(LotusParserInternal::LotusStream &stream, LotusSpreadsheetInternal::Style &style, int &numCell, long endPos);
	//! reads the row size ( in pt*32 )
	bool readRowSizes(LotusParserInternal::LotusStream &stream, long endPos);

	//! reads a cell
	bool readCell(LotusParserInternal::LotusStream &stream);
	//! reads a cell or list of cell name
	bool readCellName(LotusParserInternal::LotusStream &stream);

	// in fmt

	//! try to read a sheet header: 0xc3
	bool readSheetHeader(LotusParserInternal::LotusStream &stream);
	//! try to read an extra row format: 0xc5
	bool readExtraRowFormats(LotusParserInternal::LotusStream &stream);

	// in zone 0x1b

	//! try to read a note: subZone id 9065
	bool readNote(LotusParserInternal::LotusStream &stream, long endPos);

	// data in formula

	/* reads a cell */
	bool readCell(LotusParserInternal::LotusStream &stream, int sId, bool isList, WKSContentListener::FormulaInstruction &instr);
	/* reads a formula */
	bool readFormula(LotusParserInternal::LotusStream &stream, long endPos, int sId, bool newFormula,
	                 std::vector<WKSContentListener::FormulaInstruction> &formula, std::string &error);
private:
	LotusSpreadsheet(LotusSpreadsheet const &orig);
	LotusSpreadsheet &operator=(LotusSpreadsheet const &orig);
	shared_ptr<WKSContentListener> m_listener; /** the listener (if set)*/
	//! the main parser
	LotusParser &m_mainParser;
	//! the style manager
	shared_ptr<LotusStyleManager> m_styleManager;
	//! the internal state
	shared_ptr<LotusSpreadsheetInternal::State> m_state;
};

#endif /* LOTUS_SPREAD_SHEET_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
