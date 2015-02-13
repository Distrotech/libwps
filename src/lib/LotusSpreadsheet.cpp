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
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 */

#include <stdlib.h>
#include <string.h>

#include <cmath>
#include <sstream>
#include <limits>
#include <stack>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSCell.h"
#include "WKSContentListener.h"
#include "WPSEntry.h"
#include "WPSFont.h"

#include "Lotus.h"

#include "LotusSpreadsheet.h"

namespace LotusSpreadsheetInternal
{

///////////////////////////////////////////////////////////////////
//! a class used to store a style of a cell in LotusSpreadsheet
struct Style : public WPSCellFormat
{
	//! construtor
	Style(libwps_tools_win::Font::Type type) : WPSCellFormat(), m_font(), m_fontType(type), m_extra("")
	{
		for (int i = 0; i < 10; i++) m_unknFlags[i] = 0;
	}

	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, Style const &style);
	//! operator==
	bool operator==(Style const &st) const;
	//! operator!=
	bool operator!=(Style const &st) const
	{
		return !(*this==st);
	}
	/** the font */
	WPSFont m_font;
	//! font encoding type
	libwps_tools_win::Font::Type m_fontType;
	/** some flag */
	int m_unknFlags[10];
	/** extra data */
	std::string m_extra;
};

//! operator<<
std::ostream &operator<<(std::ostream &o, Style const &style)
{
	o << "font=[" << style.m_font << "],";
	o << static_cast<WPSCellFormat const &>(style) << ",";

	bool hasUnkn = false;
	for (int i = 0; i < 10; i++)
	{
		if (!style.m_unknFlags[i]) continue;
		hasUnkn=true;
		break;
	}
	if (hasUnkn)
	{
		o << "unkn=[" << std::hex;
		for (int i = 0; i < 10; i++)
		{
			if (style.m_unknFlags[i]) o << "fS" << i << "=" << std::hex << style.m_unknFlags[i] << std::dec << ",";
		}
		o << std::dec << "]";
	}
	if (style.m_extra.length())
		o << ", extra=[" << style.m_extra << "]";

	return o;
}

bool Style::operator==(Style const &st) const
{
	if (m_font!=st.m_font) return false;
	if (m_format!=st.m_format || m_subFormat!=st.m_subFormat || m_digits!=st.m_digits || m_protected !=st.m_protected)
		return false;
	int diff = WPSCellFormat::compare(st);
	if (diff) return false;
	for (int i = 0; i < 10; i++)
	{
		if (m_unknFlags[i]!=st.m_unknFlags[i])
			return false;
	}
	return m_extra==st.m_extra;
}

///////////////////////////////////////////////////////////////////
//! the style manager
class StyleManager
{
public:
	StyleManager() : m_stylesList() {}
	//! add a new style and returns its id
	int add(Style const &st, bool dosFile)
	{
		if (dosFile)
		{
			for (size_t i=0; i < m_stylesList.size(); ++i)
			{
				if (m_stylesList[i]==st) return int(i);
			}
		}
		m_stylesList.push_back(st);
		return int(m_stylesList.size())-1;
	}
	//! returns the style with id
	bool get(int id, Style &style) const
	{
		if (id<0|| id >= (int) m_stylesList.size())
		{
			WPS_DEBUG_MSG(("LotusParserInternal::StyleManager::get can not find style %d\n", id));
			return false;
		}
		style=m_stylesList[size_t(id)];
		return true;
	}
	//! returns the number of style
	int size() const
	{
		return (int) m_stylesList.size();
	}
	//! print a style
	void print(int id, std::ostream &o) const
	{
		if (id < 0) return;
		if (id < int(m_stylesList.size()))
			o << ", style=" << m_stylesList[size_t(id)];
		else
		{
			WPS_DEBUG_MSG(("LotusParserInternal::StyleManager::print: can not find a style\n"));
			o << ", ###style=" << id;
		}
	}

protected:
	//! the styles
	std::vector<Style> m_stylesList;
};

//! a cellule of a Lotus spreadsheet
class Cell : public WPSCell
{
public:
	/// constructor
	Cell() : m_styleId(-1), m_hAlign(WPSCellFormat::HALIGN_DEFAULT), m_content() { }

	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, Cell const &cell);

	//! call when a cell must be send
	bool send(WPSListenerPtr &/*listener*/)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheetInternal::Cell::send: must not be called\n"));
		return false;
	}

	//! call when the content of a cell must be send
	bool sendContent(WPSListenerPtr &/*listener*/)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheetInternal::Cell::sendContent: must not be called\n"));
		return false;
	}

	//! the style
	int m_styleId;
	//! the horizontal align (in dos file)
	WPSCellFormat::HorizontalAlignment m_hAlign;
	//! the content
	WKSContentListener::CellContent m_content;
};

//! operator<<
std::ostream &operator<<(std::ostream &o, Cell const &cell)
{
	o << reinterpret_cast<WPSCell const &>(cell)
	  << cell.m_content << ",style=" << cell.m_styleId << ",";
	switch (cell.m_hAlign)
	{
	case WPSCellFormat::HALIGN_LEFT:
		o << "left,";
		break;
	case WPSCellFormat::HALIGN_CENTER:
		o << "centered,";
		break;
	case WPSCellFormat::HALIGN_RIGHT:
		o << "right,";
		break;
	case WPSCellFormat::HALIGN_FULL:
		o << "full,";
		break;
	case WPSCellFormat::HALIGN_DEFAULT:
	default:
		break; // default
	}
	return o;
}

///////////////////////////////////////////////////////////////////
//! the spreadsheet of a WPS4Spreadsheet
class Spreadsheet
{
public:
	//! a constructor
	Spreadsheet() : m_numCols(0), m_numRows(0), m_widthCols(), m_heightRows(), m_cellsList(),
		m_rowPageBreaksList() {}
	//! returns the last cell
	Cell *getLastCell()
	{
		if (m_cellsList.size()) return &m_cellsList[size_t(m_cellsList.size()-1)];
		return 0;
	}
	//! set the columns size
	void setColumnWidth(int col, int w=-1)
	{
		if (col < 0) return;
		if (col >= int(m_widthCols.size())) m_widthCols.resize(size_t(col)+1, -1);
		m_widthCols[size_t(col)] = w;
		if (col >= m_numCols) m_numCols=col+1;
	}
	//! set the rows size
	void setRowHeight(int row, int h=-1)
	{
		if (row < 0) return;
		if (row >= int(m_heightRows.size())) m_heightRows.resize(size_t(row)+1, -1);
		m_heightRows[size_t(row)] = h;
		if (row >= m_numRows) m_numRows=row+1;
	}

	//! convert the m_widthCols, m_heightRows in a vector of of point size
	static std::vector<float> convertInPoint(std::vector<int> const &list,
	                                         float defSize)
	{
		size_t numElt = list.size();
		std::vector<float> res;
		res.resize(numElt);
		for (size_t i = 0; i < numElt; i++)
		{
			if (list[i] < 0) res[i] = defSize;
			else res[i] = float(list[i])/20.f;
		}
		return res;
	}
	//! returns true if the spreedsheet is empty
	bool empty() const
	{
		return m_cellsList.size() == 0;
	}
	/** the number of columns */
	int m_numCols;
	/** the number of rows */
	int m_numRows;

	/** the column size in TWIP (?) */
	std::vector<int> m_widthCols;
	/** the row size in TWIP (?) */
	std::vector<int> m_heightRows;
	/** the list of not empty cells */
	std::vector<Cell> m_cellsList;
	/** the list of row page break */
	std::vector<int> m_rowPageBreaksList;

	/** returns the last Right Bottom cell position */
	Vec2i getRightBottomPosition() const
	{
		int maxX = 0, maxY = 0;
		size_t numCell = m_cellsList.size();
		for (size_t i = 0; i < numCell; i++)
		{
			Vec2i const &p = m_cellsList[i].position();
			if (p[0] > maxX) maxX = p[0];
			if (p[1] > maxY) maxY = p[1];
		}
		return Vec2i(maxX, maxY);
	}
};

//! the state of LotusSpreadsheet
struct State
{
	//! constructor
	State() :  m_eof(-1), m_version(-1), m_styleManager(), m_spreadsheetList(), m_spreadsheetStack()
	{
		pushNewSheet();
	}
	//! returns the number of spreadsheet
	int getNumSheet() const
	{
		return int(m_spreadsheetList.size());
	}
	//! returns the ith spreadsheet
	Spreadsheet &getSheet(int id)
	{
		if (id<0||id>=int(m_spreadsheetList.size()))
		{
			WPS_DEBUG_MSG(("LotusSpreadsheetInternal::State::getSheet: can find spreadsheet %d\n", id));
			static Spreadsheet empty;
			return empty;
		}
		return m_spreadsheetList[size_t(id)];
	}
	//! returns the actual sheet
	Spreadsheet &getActualSheet()
	{
		return m_spreadsheetList[m_spreadsheetStack.top()];
	}
	//! create a new sheet and stack id
	void pushNewSheet()
	{
		size_t id=m_spreadsheetList.size();
		m_spreadsheetStack.push(id);
		m_spreadsheetList.resize(id+1);
	}
	//! try to pop the actual sheet
	bool popSheet()
	{
		if (m_spreadsheetStack.size()<=1)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheetInternal::State::popSheet: can pop the main sheet\n"));
			return false;
		}
		m_spreadsheetStack.pop();
		return true;
	}
	//! the last file position
	long m_eof;
	//! the file version
	int m_version;
	//! the style manager
	StyleManager m_styleManager;

protected:
	//! the list of spreadsheet ( first: main spreadsheet, other report spreadsheet )
	std::vector<Spreadsheet> m_spreadsheetList;
	//! the stack of spreadsheet id
	std::stack<size_t> m_spreadsheetStack;
};

}

// constructor, destructor
LotusSpreadsheet::LotusSpreadsheet(LotusParser &parser) :
	m_input(parser.getInput()), m_listener(), m_mainParser(parser), m_state(new LotusSpreadsheetInternal::State),
	m_asciiFile(parser.ascii())
{
	m_state.reset(new LotusSpreadsheetInternal::State);
}

LotusSpreadsheet::~LotusSpreadsheet()
{
}

int LotusSpreadsheet::version() const
{
	if (m_state->m_version<0)
		m_state->m_version=m_mainParser.version();
	return m_state->m_version;
}

bool LotusSpreadsheet::checkFilePosition(long pos)
{
	if (m_state->m_eof < 0)
	{
		long actPos = m_input->tell();
		m_input->seek(0, librevenge::RVNG_SEEK_END);
		m_state->m_eof=m_input->tell();
		m_input->seek(actPos, librevenge::RVNG_SEEK_SET);
	}
	return pos <= m_state->m_eof;
}


////////////////////////////////////////////////////////////
// low level

////////////////////////////////////////////////////////////
//   parse sheet data
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Data
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// filter
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// report
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// send data
////////////////////////////////////////////////////////////
void LotusSpreadsheet::sendSpreadsheet()
{
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::sendSpreadsheet: I can not find the listener\n"));
		return;
	}
	LotusSpreadsheetInternal::Spreadsheet &sheet = m_state->getSheet(0);
	size_t numCell = sheet.m_cellsList.size();

	int prevRow = -1;
	m_listener->openSheet(sheet.convertInPoint(sheet.m_widthCols,76), librevenge::RVNG_POINT);
	std::vector<float> rowHeight = sheet.convertInPoint(sheet.m_heightRows,16);
	for (size_t i = 0; i < numCell; i++)
	{
		LotusSpreadsheetInternal::Cell const &cell= sheet.m_cellsList[i];
		// FIXME: openSheetRow/openSheetCell must do that
		if (cell.position()[1] != prevRow)
		{
			while (cell.position()[1] > prevRow)
			{
				if (prevRow != -1)
					m_listener->closeSheetRow();
				prevRow++;
				if (prevRow < int(rowHeight.size()))
					m_listener->openSheetRow(rowHeight[size_t(prevRow)], librevenge::RVNG_POINT);
				else
					m_listener->openSheetRow(16, librevenge::RVNG_POINT);
			}
		}
		sendCellContent(cell);
	}
	if (prevRow!=-1) m_listener->closeSheetRow();
	m_listener->closeSheet();
}

void LotusSpreadsheet::sendCellContent(LotusSpreadsheetInternal::Cell const &cell)
{
	if (m_listener.get() == 0L)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::sendCellContent: I can not find the listener\n"));
		return;
	}

	LotusSpreadsheetInternal::Style cellStyle(m_mainParser.getDefaultFontType());
	if (cell.m_styleId<0 || !m_state->m_styleManager.get(cell.m_styleId,cellStyle))
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::sendCellContent: I can not find the cell style\n"));
	}
	if (version()<=2 && cell.m_hAlign!=WPSCellFormat::HALIGN_DEFAULT)
		cellStyle.setHAlignement(cell.m_hAlign);

	librevenge::RVNGPropertyList propList;
	libwps_tools_win::Font::Type fontType = cellStyle.m_fontType;
	m_listener->setFont(cellStyle.m_font);

	LotusSpreadsheetInternal::Cell finalCell(cell);
	finalCell.WPSCellFormat::operator=(cellStyle);
	finalCell.setFont(cellStyle.m_font);
	WKSContentListener::CellContent content(cell.m_content);
	for (size_t f=0; f < content.m_formula.size(); ++f)
	{
		if (content.m_formula[f].m_type!=WKSContentListener::FormulaInstruction::F_Text)
			continue;
		std::string &text=content.m_formula[f].m_content;
		librevenge::RVNGString finalString("");
		for (size_t c=0; c < text.length(); ++c)
		{
			WPSListener::appendUnicode
			((uint32_t)libwps_tools_win::Font::unicode((unsigned char)text[c],fontType), finalString);
		}
		text=finalString.cstr();
	}
	m_listener->openSheetCell(finalCell, content, propList);

	if (cell.m_content.m_textEntry.valid())
	{
		m_input->seek(cell.m_content.m_textEntry.begin(), librevenge::RVNG_SEEK_SET);
		bool prevEOL=false;
		while (!m_input->isEnd() && m_input->tell()<cell.m_content.m_textEntry.end())
		{
			unsigned char c=(unsigned char) libwps::readU8(m_input);
			if (c==0xd)
			{
				m_listener->insertEOL();
				prevEOL=true;
			}
			else if (c==0xa)
			{
				if (!prevEOL)
				{
					WPS_DEBUG_MSG(("LotusSpreadsheet::sendCellContent: find 0xa without 0xd\n"));
				}
				prevEOL=false;
			}
			else
			{
				m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::unicode(c,fontType));
				prevEOL=false;
			}
		}
	}
	m_listener->closeSheetCell();
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
