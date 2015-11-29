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
#include <limits>
#include <map>
#include <set>
#include <stack>
#include <sstream>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSCell.h"
#include "WKSContentListener.h"
#include "WPSEntry.h"
#include "WPSFont.h"

#include "Lotus.h"
#include "LotusStyleManager.h"

#include "LotusSpreadsheet.h"

namespace LotusSpreadsheetInternal
{

///////////////////////////////////////////////////////////////////
//! a class used to store a style of a cell in LotusSpreadsheet
struct Style : public WPSCellFormat
{
	//! construtor
	explicit Style(libwps_tools_win::Font::Type type) : WPSCellFormat(), m_fontType(type), m_extra("")
	{
		m_font.m_size=10;
	}

	//! returns true if the style has a background color and a border or a format type
	bool isVisible() const
	{
		return !backgroundColor().isWhite() || hasBorders() || m_format!=F_UNKNOWN;
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, Style const &style)
	{
		o << static_cast<WPSCellFormat const &>(style) << ",";
		if (!style.m_extra.empty()) o << style.m_extra;
		return o;
	}
	//! operator==
	bool operator==(Style const &st) const
	{
		if (m_fontType!=st.m_fontType || WPSCellFormat::compare(st)!=0) return false;
		return true;
	}
	//! operator!=
	bool operator!=(Style const &st) const
	{
		return !(*this==st);
	}
	//! font encoding type
	libwps_tools_win::Font::Type m_fontType;
	/** extra data */
	std::string m_extra;
};

//! a class used to store the styles of a row in LotusSpreadsheet
struct RowStyles
{
	//! constructor
	RowStyles() : m_colsToStyleMap()
	{
	}
	//! returns true if we have no style
	bool isEmpty(bool checkOnlyVisible=false) const
	{
		if (m_colsToStyleMap.empty())
			return true;
		if (!checkOnlyVisible)
			return false;
		for (std::map<Vec2i, Style>::const_iterator it=m_colsToStyleMap.begin(); it!=m_colsToStyleMap.end(); ++it)
		{
			if (it->second.isVisible())
				return false;
		}
		return true;
	}
	//! a map Vec2i(minCol,maxCol) to style
	std::map<Vec2i, Style> m_colsToStyleMap;
};

//! a list of position of a Lotus spreadsheet
struct CellsList
{
	//! constructor
	CellsList() : m_id(0), m_positions()
	{
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, CellsList const &pos)
	{
		o << pos.m_positions;
		if (pos.m_id) o << "[id=" << pos.m_id << "]";
		o << ",";
		return o;
	}
	//! the sheet id
	int m_id;
	//! the first and last position
	Box2i m_positions;
};

//! a cellule of a Lotus spreadsheet
class Cell : public WPSCell
{
public:
	/// constructor
	Cell() : m_styleId(-1), m_hAlign(WPSCellFormat::HALIGN_DEFAULT), m_content(), m_comment() { }

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
	//! the comment entry
	WPSEntry m_comment;
};

//! operator<<
std::ostream &operator<<(std::ostream &o, Cell const &cell)
{
	o << reinterpret_cast<WPSCell const &>(cell) << cell.m_content << ",";
	if (cell.m_styleId>=0) o << "style=" << cell.m_styleId << ",";
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

//! small struct use to keep trace of cell with style or content used by LotusSpreadsheet::sendSpreadsheet
struct CellPLC
{
	//! constructor
	CellPLC() : m_pos(0,0), m_cell(0), m_style(0)
	{
	}
	//! copy constructor
	CellPLC(CellPLC const &orig) : m_pos(orig.m_pos), m_cell(orig.m_cell), m_style(orig.m_style)
	{
	}
	//! operator=
	CellPLC &operator=(CellPLC const &orig)
	{
		if (this==&orig)
			return *this;
		m_pos=orig.m_pos;
		m_cell=orig.m_cell;
		m_style=orig.m_style;
		return *this;
	}
	//! the cell position
	Vec2i m_pos;
	//! a pointer to the cell content
	Cell const *m_cell;
	//! a pointer to the cell style
	Style const *m_style;
};

///////////////////////////////////////////////////////////////////
//! the spreadsheet of a LotusSpreadsheet
class Spreadsheet
{
public:
	//! a constructor
	Spreadsheet() : m_name(""), m_numCols(0), m_numRows(0), m_boundsColsMap(),
		m_widthColsInChar(), m_rowHeightMap(), m_heightDefault(16),
		m_rowPageBreaksList(), m_positionToCellMap() {}
	//! return a cell corresponding to a spreadsheet, create one if needed
	Cell &getCell(Vec2i const &pos)
	{
		if (m_positionToCellMap.find(pos)==m_positionToCellMap.end())
		{
			Cell cell;
			cell.setPosition(pos);
			m_positionToCellMap[pos]=cell;
		}
		return m_positionToCellMap.find(pos)->second;
	}
	//! return the row bounds corresponding to a column
	bool getRowBounds(int col, Vec2i &bound) const
	{
		if (m_boundsColsMap.empty()) return false;
		if (m_boundsColsMap.find(col)==m_boundsColsMap.end())
			bound=Vec2i(-1,-1);
		else
			bound=m_boundsColsMap.find(col)->second;
		return true;
	}
	//! set the columns size
	void setColumnWidthInChar(int col, int w=-1)
	{
		if (col < 0) return;
		if (col >= int(m_widthColsInChar.size()))
		{
			// sanity check
			if (!m_boundsColsMap.empty() && col >= int(m_widthColsInChar.size())+10 &&
			        m_boundsColsMap.find(col)==m_boundsColsMap.end())
			{
				WPS_DEBUG_MSG(("LotusSpreadsheetInternal::Spreadsheet::setColumnWidth: the column %d seems bad\n", col));
				return;
			}
			m_widthColsInChar.resize(size_t(col)+1, -1);
		}
		m_widthColsInChar[size_t(col)] = w;
		if (col >= m_numCols) m_numCols=col+1;
	}
	//! returns the row size in point
	float getRowHeight(int row) const
	{
		std::map<Vec2i,int>::const_iterator rIt=m_rowHeightMap.lower_bound(Vec2i(-1,row));
		if (rIt!=m_rowHeightMap.end() && rIt->first[0]<=row && rIt->first[1]>=row)
			return (float) rIt->second;
		return (float) m_heightDefault;
	}
	//! returns the height of a row in point and updated repeated row
	float getRowHeight(int row, int &numRepeated) const
	{
		std::map<Vec2i,int>::const_iterator rIt=m_rowHeightMap.lower_bound(Vec2i(-1,row));
		if (rIt!=m_rowHeightMap.end() && rIt->first[0]<=row && rIt->first[1]>=row)
		{
			numRepeated=rIt->first[1]-row+1;
			return (float) rIt->second;
		}
		numRepeated=10000;
		return (float) m_heightDefault;
	}
	//! set the rows size
	void setRowHeight(int row, int h)
	{
		if (h>=0)
			m_rowHeightMap[Vec2i(row,row)]=h;
	}
	//! try to compress the list of row height
	void compressRowHeights()
	{
		std::map<Vec2i,int> oldMap=m_rowHeightMap;
		m_rowHeightMap.clear();
		std::map<Vec2i,int>::const_iterator rIt=oldMap.begin();
		int actHeight=-1;
		Vec2i actPos(0,-1);
		while (rIt!=oldMap.end())
		{
			// first check for not filled row
			if (rIt->first[0]!=actPos[1]+1)
			{
				if (actHeight==m_heightDefault)
					actPos[1]=rIt->first[0]-1;
				else
				{
					if (actPos[1]>=actPos[0])
						m_rowHeightMap[actPos]=actHeight;
					actHeight=m_heightDefault;
					actPos=Vec2i(actPos[1]+1, rIt->first[0]-1);
				}
			}
			if (rIt->second!=actHeight)
			{
				if (actPos[1]>=actPos[0])
					m_rowHeightMap[actPos]=actHeight;
				actPos[0]=rIt->first[0];
				actHeight=rIt->second;
			}
			actPos[1]=rIt->first[1];
			++rIt;
		}
		if (actPos[1]>=actPos[0])
			m_rowHeightMap[actPos]=actHeight;
	}
	//! convert the m_widthColsInChar, m_rowHeightMap in a vector of of point size
	static std::vector<float> convertInPoint(std::vector<int> const &list,
	                                         float defSize, float factor=1)
	{
		size_t numElt = list.size();
		std::vector<float> res;
		res.resize(numElt);
		for (size_t i = 0; i < numElt; i++)
		{
			if (list[i] < 0) res[i] = defSize;
			else res[i] = float(list[i])*factor;
		}
		return res;
	}
	//! returns true if the spreedsheet is empty
	bool empty() const
	{
		return m_positionToCellMap.empty() && m_name.empty();
	}
	/** the sheet name */
	librevenge::RVNGString m_name;
	/** the number of columns */
	int m_numCols;
	/** the number of rows */
	int m_numRows;
	/** a map used to stored the min/max row of each columns */
	std::map<int, Vec2i> m_boundsColsMap;
	/** the column size in char */
	std::vector<int> m_widthColsInChar;
	/** the map Vec2i(min row, max row) to size in points */
	std::map<Vec2i,int> m_rowHeightMap;
	/** the default row size in point */
	int m_heightDefault;
	/** the list of row page break */
	std::vector<int> m_rowPageBreaksList;
	/** a map cell to not empty cells */
	std::map<Vec2i, Cell> m_positionToCellMap;
};

//! the state of LotusSpreadsheet
struct State
{
	//! constructor
	State() :  m_eof(-1), m_version(-1), m_spreadsheetList(), m_nameToCellsMap(),
		m_rowStylesList(), m_rowSheetIdToStyleIdMap(), m_rowSheetIdToChildRowIdMap()
	{
		m_spreadsheetList.resize(1);
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
	//! returns the ith spreadsheet
	librevenge::RVNGString getSheetName(int id) const
	{
		if (id>=0 && id<(int) m_spreadsheetList.size() && !m_spreadsheetList[size_t(id)].m_name.empty())
			return m_spreadsheetList[size_t(id)].m_name;
		librevenge::RVNGString name;
		name.sprintf("Sheet%d", id+1);
		return name;
	}
	//! returns true if a sheet has some style
	bool hasStyles(int sheetId) const
	{
		std::map<Vec2i,size_t>::const_iterator it=m_rowSheetIdToStyleIdMap.find(Vec2i(-1, sheetId));
		if (it!=m_rowSheetIdToStyleIdMap.end() && it->first[1]==sheetId)
			return true;
		return false;
	}
	//! the last file position
	long m_eof;
	//! the file version
	int m_version;
	//! the list of spreadsheet ( first: main spreadsheet, other report spreadsheet )
	std::vector<Spreadsheet> m_spreadsheetList;
	//! map name to position
	std::map<std::string, CellsList> m_nameToCellsMap;
	//! the list of row styles
	std::vector<RowStyles> m_rowStylesList;
	//! map Vec2i(row, sheetId) to row style id
	std::map<Vec2i,size_t> m_rowSheetIdToStyleIdMap;
	//! map Vec2i(row, sheetId) to child style
	std::multimap<Vec2i,Vec2i> m_rowSheetIdToChildRowIdMap;
};

}

// constructor, destructor
LotusSpreadsheet::LotusSpreadsheet(LotusParser &parser) :
	m_input(parser.getInput()), m_listener(), m_mainParser(parser), m_styleManager(parser.m_styleManager),
	m_state(new LotusSpreadsheetInternal::State), m_asciiFile(parser.ascii())
{
	m_state.reset(new LotusSpreadsheetInternal::State);
}

LotusSpreadsheet::~LotusSpreadsheet()
{
}

void LotusSpreadsheet::cleanState()
{
	m_state.reset(new LotusSpreadsheetInternal::State);
}

void LotusSpreadsheet::updateState()
{
	// update the state correspondance between row and row's styles
	if (!m_state->m_rowSheetIdToChildRowIdMap.empty())
	{
		std::set<Vec2i> seens;
		std::stack<Vec2i> toDo;
		for (std::map<Vec2i,size_t>::const_iterator it=m_state->m_rowSheetIdToStyleIdMap.begin();
		        it!=m_state->m_rowSheetIdToStyleIdMap.end(); ++it)
			toDo.push(it->first);
		while (!toDo.empty())
		{
			Vec2i pos=toDo.top();
			toDo.pop();
			if (seens.find(pos)!=seens.end())
			{
				WPS_DEBUG_MSG(("LotusSpreadsheet::updateState: dupplicated position, something is bad\n"));
				continue;
			}
			seens.insert(pos);
			std::multimap<Vec2i,Vec2i>::const_iterator cIt=m_state->m_rowSheetIdToChildRowIdMap.lower_bound(pos);
			if (cIt==m_state->m_rowSheetIdToChildRowIdMap.end() || cIt->first!=pos)
				continue;
			if (m_state->m_rowSheetIdToStyleIdMap.find(pos)==m_state->m_rowSheetIdToStyleIdMap.end())
			{
				WPS_DEBUG_MSG(("LotusSpreadsheet::updateState: something is bad\n"));
				continue;
			}
			size_t finalPos=m_state->m_rowSheetIdToStyleIdMap.find(pos)->second;
			while (cIt!=m_state->m_rowSheetIdToChildRowIdMap.end() && cIt->first==pos)
			{
				Vec2i const &cPos=cIt++->second;
				m_state->m_rowSheetIdToStyleIdMap[cPos]=finalPos;
				toDo.push(cPos);
			}
		}
	}
}

void LotusSpreadsheet::setLastSpreadsheetId(int id)
{
	if (id<0)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::setLastSpreadsheetId: the id:%d seems bad\n", id));
		return;
	}
	m_state->m_spreadsheetList.resize(size_t(id+1));
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

bool LotusSpreadsheet::hasSomeSpreadsheetData() const
{
	for (size_t i=0; i<m_state->m_spreadsheetList.size(); ++i)
	{
		if (!m_state->m_spreadsheetList[i].empty() || !m_state->hasStyles(int(i)))
			return true;
	}
	return false;
}

////////////////////////////////////////////////////////////
// low level

////////////////////////////////////////////////////////////
//   parse sheet data
////////////////////////////////////////////////////////////
bool LotusSpreadsheet::readColumnDefinition()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::read16(m_input);
	if (type != 0x1f)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readColumnDefinition: not a column definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	f << "Entries(ColDef):";
	if (sz<8 || (sz%4))
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readColumnDefinition: the zone is too short\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	int sheetId=(int) libwps::readU8(m_input);
	f << "sheet[id]=" << sheetId << ",";
	int col=(int) libwps::readU8(m_input);
	f << "col=" << col << ",";
	int N=(int) libwps::readU8(m_input);
	if (N!=1) f << "N=" << N << ",";
	int val=(int) libwps::readU8(m_input); // between 0 and 94
	if (val) f << "f0=" << val << ",";
	if (sz!=4+4*N)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readColumnDefinition: the number of columns seems bad\n"));
		f << "###N,";
		if (sz==8)
			N=1;
		else
		{
			ascii().addPos(pos);
			ascii().addNote(f.str().c_str());
			return true;
		}
	}
	Vec2i bound;
	for (int n=0; n<N; ++n)
	{
		int rowPos[2];
		for (int i=0; i<2; ++i) rowPos[i]=(int) libwps::readU16(m_input);
		if (n==0)
			bound=Vec2i(rowPos[0], rowPos[1]);
		else
		{
			if (rowPos[0]<bound[0])
				bound[0]=rowPos[0];
			if (rowPos[1]>bound[1])
				bound[1]=rowPos[1];
		}
		f << "row" << n << "[bound]=" << Vec2i(rowPos[0], rowPos[1]) << ",";
	}
	if (sheetId<0||sheetId>=m_state->getNumSheet())
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readColumnDefinition the zone id seems bad\n"));
		f << "##id";
	}
	else
	{
		LotusSpreadsheetInternal::Spreadsheet &sheet=m_state->getSheet(sheetId);
		if (sheet.m_boundsColsMap.find(col)!=sheet.m_boundsColsMap.end())
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readColumnDefinition the zone col seems bad\n"));
			f << "##col";
		}
		else
			sheet.m_boundsColsMap[col]=bound;
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool LotusSpreadsheet::readColumnSizes()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::read16(m_input);
	if (type != 0x7)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readColumnSizes: not a column size name\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	f << "Entries(ColSize):";
	if (sz < 4 || (sz%2))
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readColumnSizes: the zone is too odd\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	int sheetId=(int) libwps::readU8(m_input);
	f << "id[sheet]=" << sheetId << ",";
	LotusSpreadsheetInternal::Spreadsheet empty, *sheet=0;
	if (sheetId<0||sheetId>=int(m_state->m_spreadsheetList.size()))
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readColumnSizes: can find spreadsheet %d\n", sheetId));
		sheet=&empty;
		f << "###";
	}
	else
		sheet=&m_state->m_spreadsheetList[size_t(sheetId)];
	int val=(int) libwps::readU8(m_input); // always 0?
	if (val) f << "f0=" << val << ",";
	f << "f1=" << std::hex << libwps::readU16(m_input) << std::dec << ","; // big number
	int N=int((sz-4)/2);
	f << "widths=[";
	for (int i=0; i<N; ++i)
	{
		int col=(int)libwps::readU8(m_input);
		int width=(int)libwps::readU8(m_input); // width in char, default 12...
		sheet->setColumnWidthInChar(col, width);
		f << width << "C:col" << col << ",";
	}
	f << "],";
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool LotusSpreadsheet::readRowFormats()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::read16(m_input);
	if (type != 0x13)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: not a row definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	long endPos = pos+4+sz;
	f << "Entries(RowFormat):";
	if (sz<8)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: the zone is too short\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	int sheetId=(int) libwps::readU8(m_input);
	int rowType=(int) libwps::readU8(m_input);
	int row=int(libwps::readU16(m_input));
	int val;
	f << "sheet[id]=" << sheetId << ",";
	if (row) f << "row=" << row << ",";
	switch (rowType)
	{
	case 0:
	{
		f << "def,";
		size_t rowStyleId=m_state->m_rowStylesList.size();
		m_state->m_rowStylesList.resize(rowStyleId+1);

		int actCell=0;
		LotusSpreadsheetInternal::RowStyles &stylesList=m_state->m_rowStylesList.back();
		f << "[";
		while (m_input->tell()<endPos)
		{
			int numCell;
			LotusSpreadsheetInternal::Style style(m_mainParser.getDefaultFontType());
			if (!readRowFormat(style, numCell, endPos))
			{
				f << "###";
				WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: find extra data\n"));
				break;
			}
			if (numCell>=1)
				stylesList.m_colsToStyleMap.insert
				(std::map<Vec2i,LotusSpreadsheetInternal::Style>::value_type
				 (Vec2i(actCell,actCell+numCell-1),style));
			f << "[" << style << "]";
			if (numCell>1)
				f << "x" << numCell;
			f << ",";
			actCell+=numCell;
		}
		f << "],";
		m_state->m_rowSheetIdToStyleIdMap[Vec2i(row,sheetId)]=rowStyleId;
		if (actCell>256)
		{
			f << "###";
			WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: find too much cells\n"));
		}
		break;
	}
	case 1: // the last row definition, maybe the actual row style ?
		f << "last,";
		if (sz<12)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: the size seems bad\n"));
			f << "###sz,";
			break;
		}
		for (int i=0; i<8; ++i)  // f0=0|32|41|71|7e|fe,f1=0|1|40|50|c0, f2=0|4|5|b|41, f3=0|40|54|5c, f4=27, other 0
		{
			val=(int) libwps::readU8(m_input);
			if (val)
				f << "f" << i << "=" << std::hex << val << std::dec << ",";
		}
		break;
	case 2:
	{
		f << "dup,";
		if (sz!=8)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: the size seems bad\n"));
			f << "###sz,";
			break;
		}
		int sheetId2=(int) libwps::readU8(m_input);
		if (sheetId2!=sheetId)
			f << "#sheetId2=" << sheetId2 << ",";
		val=(int) libwps::readU8(m_input); // always 0?
		if (val)
			f << "f0=" << val << ",";
		val=int (libwps::readU16(m_input));
		if (val>=row)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: the original row seems bad\n"));
			f << "#";
		}
		m_state->m_rowSheetIdToChildRowIdMap.insert
		(std::multimap<Vec2i,Vec2i>::value_type(Vec2i(val,sheetId2),Vec2i(row,sheetId)));
		f << "orig[row]=" << val << ",";
		break;
	}
	default:
		WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: find unknown row type\n"));
		f << "###type=" << rowType << ",";
		break;
	}
	if (m_input->tell()!=endPos)
	{
		ascii().addDelimiter(m_input->tell(),'|');
		m_input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool LotusSpreadsheet::readRowFormat(LotusSpreadsheetInternal::Style &style, int &numCell, long endPos)
{
	numCell=1;

	libwps::DebugStream f;
	long actPos=m_input->tell();
	if (endPos-actPos<4)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: the zone seems too short\n"));
		return false;
	}

	int value[3];
	for (int i=0; i<3; ++i)
		value[i]=(i==1) ? (int) libwps::readU16(m_input) : (int) libwps::readU8(m_input);
	WPSFont font;
	if (value[2]&0x80)
	{
		if (actPos+5>endPos)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormats: the zone seems too short\n"));
			m_input->seek(actPos, librevenge::RVNG_SEEK_SET);
			return false;
		}
		value[2]&=0x7F;
		numCell=1+(int) libwps::readU8(m_input);
	}
	if ((value[0]&0x80)==0)
		f << "protected?,";
	switch ((value[0]>>4)&7)
	{
	case 0: // fixed
		f << "fixed,";
		style.setFormat(WPSCellFormat::F_NUMBER, 1);
		style.setDigits(value[0]&0xF);
		break;
	case 1: // scientific
		style.setFormat(WPSCellFormat::F_NUMBER, 2);
		style.setDigits(value[0]&0xF);
		break;
	case 2: // currency
		style.setFormat(WPSCellFormat::F_NUMBER, 4);
		style.setDigits(value[0]&0xF);
		break;
	case 3: // percent
		style.setFormat(WPSCellFormat::F_NUMBER, 3);
		style.setDigits(value[0]&0xF);
		break;
	case 4: // decimal
		style.setFormat(WPSCellFormat::F_NUMBER, 1);
		style.setDigits(value[0]&0xF);
		break;
	case 7:
		switch (value[0]&0xF)
		{
		case 0: // +/- : kind of bool
			style.setFormat(WPSCellFormat::F_BOOLEAN);
			f << "+/-,";
			break;
		case 1:
			style.setFormat(WPSCellFormat::F_NUMBER, 0);
			break;
		case 2:
			style.setDTFormat(WPSCellFormat::F_DATE, "%d %B %y");
			break;
		case 3:
			style.setDTFormat(WPSCellFormat::F_DATE, "%d %B");
			break;
		case 4:
			style.setDTFormat(WPSCellFormat::F_DATE, "%B %y");
			break;
		case 5:
			style.setFormat(WPSCellFormat::F_TEXT);
			break;
		case 6:
			style.setFormat(WPSCellFormat::F_TEXT);
			font.m_attributes |= WPS_HIDDEN_BIT;
			break;
		case 7:
			style.setDTFormat(WPSCellFormat::F_TIME, "%I:%M:%S%p");
			break;
		case 8:
			style.setDTFormat(WPSCellFormat::F_TIME, "%I:%M%p");
			break;
		case 9:
			style.setDTFormat(WPSCellFormat::F_DATE, "%m/%d/%y");
			break;
		case 0xa:
			style.setDTFormat(WPSCellFormat::F_DATE, "%m/%d");
			break;
		case 0xb:
			style.setDTFormat(WPSCellFormat::F_TIME, "%H:%M:%S");
			break;
		case 0xc:
			style.setDTFormat(WPSCellFormat::F_TIME, "%H:%M");
			break;
		case 0xd:
			style.setFormat(WPSCellFormat::F_TEXT);
			f << "label,";
			break;
		case 0xf: // automatic
			break;
		default:
			WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormat: find unknown 7e format\n"));
			f << "Fo=##7e,";
			break;
		}
		break;
	default:
		WPS_DEBUG_MSG(("LotusSpreadsheet::readRowFormat: find unknown %x format\n", (unsigned int)(value[0]&0x7F)));
		f << "##Fo=" << std::hex << (value[0]&0x7F) << std::dec << ",";
		break;
	}

	switch (value[2]&3)
	{
	case 1:
		style.setHAlignement(WPSCellFormat::HALIGN_LEFT);
		break;
	case 2:
		style.setHAlignement(WPSCellFormat::HALIGN_RIGHT);
		break;
	case 3:
		style.setHAlignement(WPSCellFormat::HALIGN_CENTER);
		break;
	default: // general
		break;
	}

	if (value[1]&1)
		f << "red[neg],";
	if (value[1]&2)
		f << "add[parenthesis],";
	/*
	  Now can either find some font definitions or a type id. I am not
	  sure how to distinguish these two cases ; this code does not
	  seem robust and may fail on some files...
	 */
	int wh=(value[2]>>2);
	if (value[2]&0x40)
	{
		int fId=(value[1]>>6)&0x3F;
		if (fId==0)
			;
		else if ((wh&7)==5)
		{
			if (!m_styleManager->updateCellStyle(fId, style, font, style.m_fontType))
				f << "#";
			f << "Ce" << fId << ",";
		}
		else if ((wh&7)==0)
		{
			if (!m_styleManager->updateFontStyle(fId, font, style.m_fontType))
				f << "#";
			f << "FS" << fId << ",";
		}
		else
			f << "#fId=" << fId << "[" << wh << "],";
		value[1] &= 0xF03C;
	}
	else
	{
		if (value[1]&0x40)
			font.m_attributes |= WPS_BOLD_BIT;
		if (value[1]&0x80)
			font.m_attributes |= WPS_ITALICS_BIT;
		if (value[1]>>11)
			font.m_size=(value[1]>>11);
		// values[1]&0x20 is often set in this case...
		value[1] &= 0x033c;
	}
	if (value[1])
		f << "f1=" << std::hex << value[1] << std::dec << ",";
	if (wh)
		f << "wh=" << wh << ",";
	if (font.m_size<=0)
		font.m_size=10; // if the size is not defined, set it to default
	style.setFont(font);
	style.m_extra=f.str();
	return true;
}

bool LotusSpreadsheet::readRowSizes(long endPos)
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long sz=endPos-pos;
	f << "Entries(RowSize):";
	if (sz<10 || (sz%8)!=2)
	{
		WPS_DEBUG_MSG(("LotusParser::readRowSizes: the zone size seems bad\n"));
		f << "###";
		ascii().addPos(pos-6);
		ascii().addNote(f.str().c_str());
		return true;
	}

	int sheetId=(int) libwps::readU8(m_input);
	f << "id[sheet]=" << sheetId << ",";
	LotusSpreadsheetInternal::Spreadsheet empty, *sheet=0;
	if (sheetId<0||sheetId>=int(m_state->m_spreadsheetList.size()))
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readRowSizes: can find spreadsheet %d\n", sheetId));
		sheet=&empty;
		f << "###";
	}
	else
		sheet=&m_state->m_spreadsheetList[size_t(sheetId)];
	int val=(int) libwps::readU8(m_input);  // always 0
	if (val) f << "f0=" << val << ",";
	ascii().addPos(pos-6);
	ascii().addNote(f.str().c_str());
	int N=int(sz/8);
	for (int i=0; i<N; ++i)
	{
		pos=m_input->tell();
		f.str("");
		f << "RowSize-" << i << ":";
		int row=(int) libwps::readU16(m_input);
		f << "row=" << row << ",";
		val=(int) libwps::readU16(m_input);
		f << "dim=" << float(val+31)/32.f << ",";
		sheet->setRowHeight(row, int((val+31)/32));
		for (int j=0; j<2; ++j)
		{
			val=(int) libwps::read16(m_input);
			if (val!=j-1)
				f << "f" << j << "=" << val <<",";
		}
		m_input->seek(pos+8, librevenge::RVNG_SEEK_SET);
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}

	return true;
}

bool LotusSpreadsheet::readSheetName()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::read16(m_input);
	if (type != 0x23)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readSheetName: not a sheet name\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	f << "Entries(SheetName):";
	if (sz < 5)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readSheetName: sheet name is too short\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	int val=(int) libwps::read16(m_input); // always 14000
	if (val!=14000)
		f << "f0=" << std::hex << val << std::dec << ",";
	int sheetId=(int) libwps::readU8(m_input);
	f << "id[sheet]=" << sheetId << ",";
	val=(int) libwps::readU8(m_input); // always 0
	if (val)
		f << "f1=" << val << ",";
	librevenge::RVNGString name("");
	libwps_tools_win::Font::Type fontType=m_mainParser.getDefaultFontType();
	for (int i = 0; i < sz-4; i++)
	{
		unsigned char c = libwps::readU8(m_input);
		if (c == '\0') break;
		libwps::appendUnicode((uint32_t)libwps_tools_win::Font::unicode(c,fontType), name);
	}
	f << name.cstr() << ",";
	if (m_input->tell()!=pos+4+sz && m_input->tell()+1!=pos+4+sz)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readSheetName: the zone seems too short\n"));
		f << "##";
		ascii().addDelimiter(m_input->tell(), '|');
	}
	if (sheetId<0||sheetId>=m_state->getNumSheet())
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readSheetName: the zone id seems bad\n"));
		f << "##id";
	}
	else
		m_state->getSheet(sheetId).m_name=name;
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
// Cell
////////////////////////////////////////////////////////////
bool LotusSpreadsheet::readCellName()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::read16(m_input);
	if (type!=9)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readCellName: not a cell name cell\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	long endPos=pos+4+sz;
	f << "Entries(CellName):";
	if (sz < 0x1a)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readCellName: the zone is too short\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	int val=(int) libwps::read16(m_input); // 0 or 1
	if (val)
		f << "f0=" << val << ",";
	std::string name("");
	for (int i=0; i<16; ++i)
	{
		char c=(char) libwps::readU8(m_input);
		if (!c) break;
		name += c;
	}
	f << name << ",";
	m_input->seek(pos+4+18, librevenge::RVNG_SEEK_SET);
	LotusSpreadsheetInternal::CellsList cells;
	for (int i=0; i<2; ++i)
	{
		int row=(int) libwps::readU16(m_input);
		int sheetId=(int) libwps::readU8(m_input);
		int col=(int) libwps::readU8(m_input);
		if (i==0)
			cells.m_positions.setMin(Vec2i(col,row));
		else
			cells.m_positions.setMax(Vec2i(col,row));
		if (i==0)
			cells.m_id=sheetId;
		else if (cells.m_id!=sheetId)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCellName: the second sheet id seems bad\n"));
			f << "##id2=" << sheetId << ",";
		}
	}
	f << cells << ",";
	if (m_state->m_nameToCellsMap.find(name)!=m_state->m_nameToCellsMap.end())
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readCellName: cell with name %s already exists\n", name.c_str()));
		f << "##name=" << name << ",";
	}
	else
		m_state->m_nameToCellsMap[name]=cells;
	std::string note("");
	int remain=int(endPos-m_input->tell());
	for (int i=0; i<remain; ++i)
	{
		char c=(char) libwps::readU8(m_input);
		if (!c) break;
		note+=c;
	}
	if (!note.empty())
		f << "note=" << note << ",";
	if (m_input->tell()!=endPos)
		ascii().addDelimiter(m_input->tell(),'|');

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool LotusSpreadsheet::readCell()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::read16(m_input);
	std::string what("");
	if (type == 0x16)
		what="TextCell";
	else if (type == 0x17)
		what="Doub10Cell";
	else if (type == 0x18)
		what="DoubU16Cell";
	else if (type == 0x19)
		what="Doub10FormCell";
	else if (type == 0x1a) // checkme
		what="TextFormCell";
	else if (type == 0x25)
		what="DoubU32Cell";
	else if (type == 0x26)
		what="CommentCell";
	else if (type == 0x27)
		what="Doub8Cell";
	else if (type == 0x28)
		what="Doub8FormCell";
	else
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: not a cell's cell\n"));
		return false;
	}

	long sz = (long) libwps::readU16(m_input);
	if (sz < 5)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: the zone is too short\n"));
		f << "Entries(" << what << "):###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	long endPos=pos+4+sz;
	int row=(int) libwps::readU16(m_input);
	int sheetId=(int) libwps::readU8(m_input);
	int col=(int) libwps::readU8(m_input);
	if (sheetId) f << "sheet[id]=" << sheetId << ",";

	LotusSpreadsheetInternal::Spreadsheet empty, *sheet=0;
	if (sheetId<0||sheetId>=int(m_state->m_spreadsheetList.size()))
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: can find spreadsheet %d\n", sheetId));
		sheet=&empty;
		f << "###";
	}
	else
		sheet=&m_state->m_spreadsheetList[size_t(sheetId)];

	LotusSpreadsheetInternal::Cell &cell=sheet->getCell(Vec2i(col, row));
	switch (type)
	{
	case 0x16:
	case 0x1a:
	case 0x26:   // comment
	{
		std::string text("");
		long begText=m_input->tell();
		for (int i=4; i<sz; ++i)
		{
			char c=(char) libwps::readU8(m_input);
			if (!c) break;
			text += c;
		}
		f << "\"" << text << "\",";
		if (!text.empty())
		{
			if (text[0]=='\'') cell.m_hAlign=WPSCellFormat::HALIGN_DEFAULT;
			else if (text[0]=='\\') cell.m_hAlign=WPSCellFormat::HALIGN_LEFT;
			else if (text[0]=='^') cell.m_hAlign=WPSCellFormat::HALIGN_CENTER;
			else if (text[0]=='\"') cell.m_hAlign=WPSCellFormat::HALIGN_RIGHT;
			else
				--begText;
			++begText;
		}

		WPSEntry entry;
		entry.setBegin(begText);
		entry.setEnd(endPos);
		if (type==0x16)
		{
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_TEXT;
			cell.m_content.m_textEntry=entry;
		}
		else if (type==0x1a)
		{
			if (cell.m_content.m_contentType!=WKSContentListener::CellContent::C_FORMULA)
				cell.m_content.m_contentType=WKSContentListener::CellContent::C_TEXT;
			cell.m_content.m_textEntry=entry;
		}
		else if (type==0x26)
			cell.m_comment=entry;
		else
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: find unexpected type %x\n", (unsigned int) type));
			f << "###type";
		}

		if (m_input->tell()!=endPos && m_input->tell()+1!=endPos)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: the string zone seems too short\n"));
			f << "###";
		}
		break;
	}
	case 0x17:
	{
		if (sz!=14)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: the double10 zone seems too short\n"));
			f << "###";
		}
		double res;
		bool isNaN;
		if (!libwps::readDouble10(m_input, res, isNaN))
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: can read a double10 zone\n"));
			f << "###";
			break;
		}
		if (cell.m_content.m_contentType!=WKSContentListener::CellContent::C_FORMULA)
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
		cell.m_content.setValue(res);
		break;
	}
	case 0x18:
	{
		if (sz!=6)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: the uint16 zone seems too short\n"));
			f << "###";
		}
		double res;
		bool isNaN;
		if (!libwps::readDouble2Inv(m_input, res, isNaN))
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: can read a uint16 zone\n"));
			f << "###";
			break;
		}
		if (cell.m_content.m_contentType!=WKSContentListener::CellContent::C_FORMULA)
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
		cell.m_content.setValue(res);
		break;
	}
	case 0x19:
	{
		if (sz<=14)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: the double10+formula zone seems too short\n"));
			f << "###";
		}
		double res;
		bool isNaN;
		if (!libwps::readDouble10(m_input, res, isNaN))
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: can read double10+formula for zone\n"));
			f << "###";
			break;
		}
		cell.m_content.m_contentType=WKSContentListener::CellContent::C_FORMULA;
		cell.m_content.setValue(res);
		ascii().addDelimiter(m_input->tell(),'|');
		std::string error;
		if (!readFormula(endPos, sheetId, false, cell.m_content.m_formula, error))
		{
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
			ascii().addDelimiter(m_input->tell()-1, '#');
			if (error.length()) f << error;
			break;
		}
		if (error.length()) f << error;
		if (m_input->tell()+1>=endPos)
			break;
		static bool first=true;
		if (first)
		{
			first=false;
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: find err message for double10+formula\n"));
		}
		// find in one file "Formula failed to convert"
		error="";
		int remain=int(endPos-m_input->tell());
		for (int i=0; i<remain; ++i) error += (char) libwps::readU8(m_input);
		f << "#err[msg]=" << error << ",";
		break;
	}
	case 0x25:
	{
		if (sz!=8)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: the uint32 zone seems too short\n"));
			f << "###";
		}
		double res;
		bool isNaN;
		if (!libwps::readDouble4Inv(m_input, res, isNaN))
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: can read a uint32 zone\n"));
			f << "###";
			break;
		}
		if (cell.m_content.m_contentType!=WKSContentListener::CellContent::C_FORMULA)
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
		cell.m_content.setValue(res);
		break;
	}
	case 0x27:
	{
		if (sz!=12)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: the double8 zone seems too short\n"));
			f << "###";
		}
		double res;
		bool isNaN;
		if (!libwps::readDouble8(m_input, res, isNaN))
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: can read a double8 zone\n"));
			f << "###";
			break;
		}
		if (cell.m_content.m_contentType!=WKSContentListener::CellContent::C_FORMULA)
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
		cell.m_content.setValue(res);
		break;
	}
	case 0x28:
	{
		if (sz<=12)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: the double8 formula zone seems too short\n"));
			f << "###";
		}
		double res;
		bool isNaN;
		if (!libwps::readDouble8(m_input, res, isNaN))
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: can read a double8 formula zone\n"));
			f << "###";
			break;
		}
		cell.m_content.m_contentType=WKSContentListener::CellContent::C_FORMULA;
		cell.m_content.setValue(res);
		ascii().addDelimiter(m_input->tell(),'|');
		std::string error;
		if (!readFormula(endPos, sheetId, true, cell.m_content.m_formula, error))
		{
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
			ascii().addDelimiter(m_input->tell()-1, '#');
		}
		else if (m_input->tell()+1<endPos)
		{
			// often end with another bytes 03, probably for alignement
			WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: find extra data for double8 formula zone\n"));
			f << "###extra";
		}
		if (error.length()) f << error;
		break;
	}
	default:
		WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: oops find unimplemented type\n"));
		break;
	}
	std::string extra=f.str();
	f.str("");
	f << "Entries(" << what << "):" << cell << "," << extra;
	if (m_input->tell()!=endPos)
		ascii().addDelimiter(m_input->tell(),'|');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

////////////////////////////////////////////////////////////
// filter
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// report
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// send data
////////////////////////////////////////////////////////////
void LotusSpreadsheet::sendSpreadsheet(int sheetId)
{
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::sendSpreadsheet: I can not find the listener\n"));
		return;
	}
	if (sheetId<0||sheetId>=m_state->getNumSheet())
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::sendSpreadsheet: the sheet %d seems bad\n", sheetId));
		return;
	}
	LotusSpreadsheetInternal::Spreadsheet &sheet = m_state->getSheet(sheetId);

	m_listener->openSheet(sheet.convertInPoint(sheet.m_widthColsInChar,72,8), librevenge::RVNG_POINT,
	                      std::vector<int>(), m_state->getSheetName(sheetId));
	m_mainParser.sendGraphics(sheetId);
	sheet.compressRowHeights();
	typedef std::map<int, LotusSpreadsheetInternal::CellPLC> SparseRow_t;
	typedef std::map<int, SparseRow_t> SparseTable_t;
	SparseTable_t table;
	std::map<Vec2i, LotusSpreadsheetInternal::Cell>::const_iterator cIt;
	for (cIt=sheet.m_positionToCellMap.begin(); cIt!=sheet.m_positionToCellMap.end(); ++cIt)
	{
		Vec2i pos=cIt->first;
		if (table.find(pos[1])==table.end())
			table[pos[1]]=SparseRow_t();
		SparseRow_t &rowMap=table.find(pos[1])->second;
		LotusSpreadsheetInternal::CellPLC plc;
		plc.m_pos=pos;
		plc.m_cell=&cIt->second;
		rowMap[pos[0]]=plc;
	}
	size_t numRowStyle=m_state->m_rowStylesList.size();
	std::map<Vec2i,size_t>::const_iterator rIt=m_state->m_rowSheetIdToStyleIdMap.lower_bound(Vec2i(-1,sheetId));
	while (rIt!=m_state->m_rowSheetIdToStyleIdMap.end() && rIt->first[1]==sheetId)
	{
		int row=rIt->first[0];
		size_t listId=rIt->second;
		++rIt;
		if (listId>=numRowStyle)
		{
			WPS_DEBUG_MSG(("LotusSpreadsheet::sendSpreadsheet: can not find list %d\n", int(listId)));
			continue;
		}
		LotusSpreadsheetInternal::RowStyles const &listStyle=m_state->m_rowStylesList[listId];
		if (listStyle.isEmpty()) continue;
		if (table.find(row)==table.end())
		{
			if (listStyle.isEmpty(true)) continue;
			table[row]=SparseRow_t();
		}
		SparseRow_t &rowMap=table.find(row)->second;
		LotusSpreadsheetInternal::CellPLC plc;
		std::map<Vec2i, LotusSpreadsheetInternal::Style>::const_iterator sIt;
		for (sIt=listStyle.m_colsToStyleMap.begin(); sIt!=listStyle.m_colsToStyleMap.end(); ++sIt)
		{
			bool isVisible=sIt->second.isVisible();
			for (int col=sIt->first[0]; col<=sIt->first[1]; ++col)
			{
				if (rowMap.find(col)==rowMap.end())
				{
					if (!isVisible)
						continue;
					plc.m_pos=Vec2i(col, row);
					rowMap[col]=plc;
				}
				rowMap.find(col)->second.m_style=&sIt->second;
			}
		}
	}
	LotusSpreadsheetInternal::Style defaultStyle(m_mainParser.getDefaultFontType());
	LotusSpreadsheetInternal::Cell emptyCell;
	int prevRow = -1;
	for (SparseTable_t::const_iterator tIt = table.begin(); tIt != table.end(); ++tIt)
	{
		int row=tIt->first;
		if (row>prevRow+1)
		{
			while (row > prevRow+1)
			{
				if (prevRow != -1) m_listener->closeSheetRow();
				int numRepeat;
				float h=sheet.getRowHeight(prevRow+1, numRepeat);
				if (row<prevRow+1+numRepeat)
					numRepeat=row-1-prevRow;
				m_listener->openSheetRow(h, librevenge::RVNG_POINT, false, numRepeat);
				prevRow+=numRepeat;
			}
		}
		if (prevRow != -1) m_listener->closeSheetRow();
		m_listener->openSheetRow(sheet.getRowHeight(++prevRow), librevenge::RVNG_POINT);
		for (SparseRow_t::const_iterator colIt = tIt->second.begin(); colIt != tIt->second.end(); ++colIt)
		{
			LotusSpreadsheetInternal::CellPLC const &plc=colIt->second;
			if (plc.m_cell==0)
				emptyCell.setPosition(plc.m_pos);
			sendCellContent(plc.m_cell ? *plc.m_cell : emptyCell, plc.m_style ? *plc.m_style : defaultStyle);
		}
	}
	if (prevRow!=-1) m_listener->closeSheetRow();

	m_listener->closeSheet();
}

void LotusSpreadsheet::sendCellContent(LotusSpreadsheetInternal::Cell const &cell,
                                       LotusSpreadsheetInternal::Style const &style)
{
	if (m_listener.get() == 0L)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::sendCellContent: I can not find the listener\n"));
		return;
	}

	LotusSpreadsheetInternal::Style cellStyle(style);
	if (cell.m_hAlign!=WPSCellFormat::HALIGN_DEFAULT)
		cellStyle.setHAlignement(cell.m_hAlign);

	libwps_tools_win::Font::Type fontType = cellStyle.m_fontType;

	m_listener->setFont(cellStyle.getFont());

	LotusSpreadsheetInternal::Cell finalCell(cell);
	finalCell.WPSCellFormat::operator=(cellStyle);
	WKSContentListener::CellContent content(cell.m_content);
	for (size_t f=0; f < content.m_formula.size(); ++f)
	{
		if (content.m_formula[f].m_type!=WKSContentListener::FormulaInstruction::F_Text)
			continue;
		std::string &text=content.m_formula[f].m_content;
		librevenge::RVNGString finalString("");
		for (size_t c=0; c < text.length(); ++c)
		{
			libwps::appendUnicode
			((uint32_t)libwps_tools_win::Font::unicode((unsigned char)text[c],fontType), finalString);
		}
		text=finalString.cstr();
	}
	m_listener->openSheetCell(finalCell, content);

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
			else if (c)
			{
				m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::unicode(c,fontType));
				prevEOL=false;
			}
		}
	}
	if (cell.m_comment.valid())
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::sendCellContent: oops sending comment is not implemented\n"));
	}
	m_listener->closeSheetCell();
}

////////////////////////////////////////////////////////////
// formula
////////////////////////////////////////////////////////////

bool LotusSpreadsheet::readCell(int sId, bool isList, WKSContentListener::FormulaInstruction &instr)
{
	instr=WKSContentListener::FormulaInstruction();
	instr.m_type=isList ? WKSContentListener::FormulaInstruction::F_CellList :
	             WKSContentListener::FormulaInstruction::F_Cell;
	int flags=(int) libwps::readU8(m_input);
	int lastSheetId=-1;
	for (int i=0; i<2; ++i)
	{
		int row=(int) libwps::readU16(m_input);
		int sheetId=(int) libwps::readU8(m_input);
		int col=(int) libwps::readU8(m_input);
		instr.m_position[i]=Vec2i(col, row);
		int wh=(i==0) ? (flags&0xF) : (flags>>4);
		instr.m_positionRelative[i]=Vec2b(wh&1, wh&2);
		if (i==0)
		{
			if (sheetId!=sId)
				instr.m_sheetName=m_state->getSheetName(sheetId);
			if (!isList) break;
			lastSheetId=sheetId;
		}
		else if (lastSheetId!=sheetId)
		{
			ascii().addDelimiter(m_input->tell()-2,'#');
			static bool isFirst=true;
			if (isFirst)
			{
				WPS_DEBUG_MSG(("LotusSpreadsheet::readCell: find some dubious sheet id\n"));
				isFirst=false;
			}
		}
	}
	return true;
}

namespace LotusSpreadsheetInternal
{
struct Functions
{
	char const *m_name;
	int m_arity;
};

static Functions const s_listFunctions[] =
{
	{ "", 0} /*SPEC: number*/, {"", 0}/*SPEC: cell*/, {"", 0}/*SPEC: cells*/, {"=", 1} /*SPEC: end of formula*/,
	{ "(", 1} /* SPEC: () */, {"", 0}/*SPEC: number*/, { "", 0} /*SPEC: text*/, {"", 0}/*name reference*/,
	{ "", 0}/* SPEC: abs name ref*/, {"", 0}/* SPEC: err range ref*/, { "", 0}/* SPEC: err cell ref*/, {"", 0}/* SPEC: err constant*/,
	{ "", -2} /* unused*/, { "", -2} /*unused*/, {"-", 1}, {"+", 2},

	{ "-", 2},{ "*", 2},{ "/", 2},{ "^", 2},
	{ "=", 2},{ "<>", 2},{ "<=", 2},{ ">=", 2},
	{ "<", 2},{ ">", 2},{ "And", 2},{ "Or", 2},
	{ "Not", 1}, { "+", 1}, { "&", 2}, { "NA", 0} /* not applicable*/,

	{ "NA", 0} /* Error*/,{ "Abs", 1},{ "Int", 1},{ "Sqrt", 1},
	{ "Log10", 1},{ "Ln", 1},{ "Pi", 0},{ "Sin", 1},
	{ "Cos", 1},{ "Tan", 1},{ "Atan2", 2},{ "Atan", 1},
	{ "Asin", 1},{ "Acos", 1},{ "Exp", 1},{ "Mod", 2},

	{ "Choose", -1},{ "IsNa", 1},{ "IsError", 1},{ "False", 0},
	{ "True", 0},{ "Rand", 0},{ "Date", 3},{ "Now", 0},
	{ "PMT", 3} /*BAD*/,{ "PV", 3} /*BAD*/,{ "FV", 3} /*BAD*/,{ "IF", 3},
	{ "Day", 1},{ "Month", 1},{ "Year", 1},{ "Round", 2},

	{ "Time", 3},{ "Hour", 1},{ "Minute", 1},{ "Second", 1},
	{ "IsNumber", 1},{ "IsText", 1},{ "Len", 1},{ "Value", 1},
	{ "Text", 2}/* or fixed*/, { "Mid", 3}, { "Char", 1},{ "Ascii", 1},
	{ "Find", 3},{ "DateValue", 1} /*checkme*/,{ "TimeValue", 1} /*checkme*/,{ "CellPointer", 1} /*checkme*/,

	{ "Sum", -1},{ "Average", -1},{ "COUNT", -1},{ "Min", -1},
	{ "Max", -1},{ "VLookUp", 3},{ "NPV", 2}, { "Var", -1},
	{ "StDev", -1},{ "IRR", 2} /*BAD*/, { "HLookup", 3},{ "DSum", 3},
	{ "DAvg", 3},{ "DCnt", 3},{ "DMin", 3},{ "DMax", 3},

	{ "DVar", 3},{ "DStd", 3},{ "Index", 3}, { "Columns", 1},
	{ "Rows", 1},{ "Rept", 2},{ "Upper", 1},{ "Lower", 1},
	{ "Left", 2},{ "Right", 2},{ "Replace", 4}, { "Proper", 1},
	{ "Cell", 1} /*checkme*/,{ "Trim", 1},{ "Clean", 1} /*UNKN*/,{ "T", 1},

	{ "IsNonText", 1},{ "Exact", 2},{ "", -2} /*App not implemented*/,{ "", 3} /*UNKN*/,
	{ "Rate", 3} /*BAD*/,{ "TERM", 3}, { "CTERM", 3}, { "SLN", 3},
	{ "SYD", 4},{ "DDB", 4},{ "SplFunc", -1} /*SplFunc*/,{ "Sheets", 1},
	{ "Info", 1},{ "SumProduct", -1},{ "IsRange", 1},{ "DGet", -1},

	{ "DQuery", -1},{ "Coord", 4}, { "", -2} /*reserved*/, { "Today", 0},
	{ "Vdb", -1},{ "Dvars", -1},{ "Dstds", -1},{ "Vars", -1},
	{ "Stds", -1},{ "D360", 2},{ "", -2} /*reserved*/,{ "IsApp", 0},
	{ "IsAaf", -1},{ "Weekday", 1},{ "DateDiff", 3},{ "Rank", -1},

	{ "NumberString", 2},{ "DateString", 1}, { "Decimal", 1}, { "Hex", 1},
	{ "Db", 4},{ "PMTI", 4},{ "SPI", 4},{ "Fullp", 1},
	{ "Halfp", 1},{ "PureAVG", -1},{ "PureCount", -1},{ "PureMax", -1},
	{ "PureMin", -1},{ "PureSTD", -1},{ "PureVar", -1},{ "PureSTDS", -1},

	{ "PureVars", -1},{ "PMT2", 3}, { "PV2", 3}, { "FV2", 3},
	{ "TERM2", 3},{ "", -2} /*UNKN*/,{ "D360", 2},{ "", -2} /*UNKN*/,
	{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/, { "", -2} /*UNKN*/,
	{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,
};
}

bool LotusSpreadsheet::readFormula(long endPos, int sheetId, bool newFormula,
                                   std::vector<WKSContentListener::FormulaInstruction> &formula, std::string &error)
{
	formula.resize(0);
	error = "";
	long pos = m_input->tell();
	if (endPos - pos < 1) return false;

	std::stringstream f;
	std::vector<std::vector<WKSContentListener::FormulaInstruction> > stack;
	bool ok = true;
	while (long(m_input->tell()) != endPos)
	{
		double val = 0.0;
		bool isNaN;
		pos = m_input->tell();
		if (pos > endPos) return false;
		int wh = (int) libwps::readU8(m_input);
		int arity = 0;
		WKSContentListener::FormulaInstruction instr;
		switch (wh)
		{
		case 0x0:
			if ((!newFormula && (endPos-pos<11 || !libwps::readDouble10(m_input, val, isNaN))) ||
			        (newFormula && (endPos-pos<9 || !libwps::readDouble8(m_input, val, isNaN))))
			{
				f.str("");
				f << "###number";
				error=f.str();
				ok = false;
				break;
			}
			instr.m_type=WKSContentListener::FormulaInstruction::F_Double;
			instr.m_doubleValue=val;
			break;
		case 0x1:
		{
			if (endPos-pos<6 || !readCell(sheetId, false, instr))
			{
				f.str("");
				f << "###cell short";
				error=f.str();
				ok = false;
				break;
			}
			break;
		}
		case 0x2:
		{
			if (endPos-pos<10 || !readCell(sheetId, true, instr))
			{
				f.str("");
				f << "###list cell short";
				error=f.str();
				ok = false;
				break;
			}
			break;
		}
		case 0x5:
			instr.m_type=WKSContentListener::FormulaInstruction::F_Double;
			if ((!newFormula && (endPos-pos<3 || !libwps::readDouble2Inv(m_input, val, isNaN))) ||
			        (newFormula && (endPos-pos<5 || !libwps::readDouble4Inv(m_input, val, isNaN))))
			{
				WPS_DEBUG_MSG(("LotusSpreadsheet::readFormula: can read a uint16/32 zone\n"));
				f << "###uint16/32";
				error=f.str();
				break;
			}
			instr.m_doubleValue=val;
			break;
		case 0x6:
			instr.m_type=WKSContentListener::FormulaInstruction::F_Text;
			while (!m_input->isEnd())
			{
				if (m_input->tell() >= endPos)
				{
					ok=false;
					break;
				}
				char c = (char) libwps::readU8(m_input);
				if (c==0) break;
				instr.m_content += c;
			}
			break;
		case 0x7:
		case 0x8:
		{
			std::string variable("");
			while (!m_input->isEnd())
			{
				if (m_input->tell() >= endPos)
				{
					ok=false;
					break;
				}
				char c = (char) libwps::readU8(m_input);
				if (c==0) break;
				variable += c;
			}
			if (!ok)
				break;
			if (m_state->m_nameToCellsMap.find(variable)==m_state->m_nameToCellsMap.end())
			{
				WPS_DEBUG_MSG(("LotusSpreadsheet::readFormula: can not find variable %s\n", variable.c_str()));
				f << "##variable=" << variable << ",";
				error=f.str();
				instr.m_type=WKSContentListener::FormulaInstruction::F_Text;
				instr.m_content = variable;
				break;
			}
			LotusSpreadsheetInternal::CellsList cells=m_state->m_nameToCellsMap.find(variable)->second;
			instr.m_position[0]=cells.m_positions[0];
			instr.m_position[1]=cells.m_positions[1];
			instr.m_positionRelative[0]=instr.m_positionRelative[1]=Vec2b(wh==7,wh==7);
			if (cells.m_id != sheetId)
				instr.m_sheetName=m_state->getSheetName(cells.m_id);
			instr.m_type=cells.m_positions[0]==cells.m_positions[1] ?
			             WKSContentListener::FormulaInstruction::F_Cell :
			             WKSContentListener::FormulaInstruction::F_CellList;
			break;
		}
		default:
			if (wh >= 0xb0 || LotusSpreadsheetInternal::s_listFunctions[wh].m_arity == -2)
			{
				f.str("");
				f << "##Funct" << std::hex << wh;
				error=f.str();
				ok = false;
				break;
			}
			instr.m_type=WKSContentListener::FormulaInstruction::F_Function;
			instr.m_content=LotusSpreadsheetInternal::s_listFunctions[wh].m_name;
			ok=!instr.m_content.empty();
			arity = LotusSpreadsheetInternal::s_listFunctions[wh].m_arity;
			if (arity == -1)
				arity = (int) libwps::read8(m_input);
			if (wh==0x7a)   // special Spell function
			{
				int sSz=(int) libwps::readU16(m_input);
				if (m_input->tell()+sSz>endPos)
				{
					WPS_DEBUG_MSG(("LotusSpreadsheet::readFormula: can not find spell function length\n"));
					f << "###spell[length]=" << sSz << ",";
					error = f.str();
					ok=false;
				}
				WKSContentListener::FormulaInstruction lastArg;
				lastArg.m_type=WKSContentListener::FormulaInstruction::F_Text;
				for (int i=0; i<sSz; ++i)
				{
					char c = (char) libwps::readU8(m_input);
					if (c==0) break;
					lastArg.m_content += c;
				}
				std::vector<WKSContentListener::FormulaInstruction> child;
				child.push_back(lastArg);
				stack.push_back(child);
				++arity;
				break;
			}
			break;
		}

		if (!ok) break;
		std::vector<WKSContentListener::FormulaInstruction> child;
		if (instr.m_type!=WKSContentListener::FormulaInstruction::F_Function)
		{
			child.push_back(instr);
			stack.push_back(child);
			continue;
		}
		size_t numElt = stack.size();
		if ((int) numElt < arity)
		{
			f.str("");
			f << instr.m_content << "[##" << arity << "]";
			error=f.str();
			ok = false;
			break;
		}
		//
		// first treat the special cases
		//
		if (arity==3 && instr.m_type==WKSContentListener::FormulaInstruction::F_Function && instr.m_content=="TERM")
		{
			// @TERM(pmt,pint,fv) -> NPER(pint,-pmt,pv=0,fv)
			std::vector<WKSContentListener::FormulaInstruction> pmt=
			    stack[size_t((int)numElt-3)];
			std::vector<WKSContentListener::FormulaInstruction> pint=
			    stack[size_t((int)numElt-2)];
			std::vector<WKSContentListener::FormulaInstruction> fv=
			    stack[size_t((int)numElt-1)];

			stack.resize(size_t(++numElt));
			// pint
			stack[size_t((int)numElt-4)]=pint;
			//-pmt
			std::vector<WKSContentListener::FormulaInstruction> &node=stack[size_t((int)numElt-3)];
			instr.m_type=WKSContentListener::FormulaInstruction::F_Operator;
			instr.m_content="-";
			node.resize(0);
			node.push_back(instr);
			instr.m_content="(";
			node.push_back(instr);
			node.insert(node.end(), pmt.begin(), pmt.end());
			instr.m_content=")";
			node.push_back(instr);
			//pv=zero
			instr.m_type=WKSContentListener::FormulaInstruction::F_Long;
			instr.m_longValue=0;
			stack[size_t((int)numElt-2)].resize(0);
			stack[size_t((int)numElt-2)].push_back(instr);
			//fv
			stack[size_t((int)numElt-1)]=fv;
			arity=4;
			instr.m_type=WKSContentListener::FormulaInstruction::F_Function;
			instr.m_content="NPER";
		}
		else if (arity==3 && instr.m_type==WKSContentListener::FormulaInstruction::F_Function && instr.m_content=="CTERM")
		{
			// @CTERM(pint,fv,pv) -> NPER(pint,pmt=0,-pv,fv)
			std::vector<WKSContentListener::FormulaInstruction> pint=
			    stack[size_t((int)numElt-3)];
			std::vector<WKSContentListener::FormulaInstruction> fv=
			    stack[size_t((int)numElt-2)];
			std::vector<WKSContentListener::FormulaInstruction> pv=
			    stack[size_t((int)numElt-1)];
			stack.resize(size_t(++numElt));
			// pint
			stack[size_t((int)numElt-4)]=pint;
			// pmt=0
			instr.m_type=WKSContentListener::FormulaInstruction::F_Long;
			instr.m_longValue=0;
			stack[size_t((int)numElt-3)].resize(0);
			stack[size_t((int)numElt-3)].push_back(instr);
			// -pv
			std::vector<WKSContentListener::FormulaInstruction> &node=stack[size_t((int)numElt-2)];
			instr.m_type=WKSContentListener::FormulaInstruction::F_Operator;
			instr.m_content="-";
			node.resize(0);
			node.push_back(instr);
			instr.m_content="(";
			node.push_back(instr);
			node.insert(node.end(), pv.begin(), pv.end());
			instr.m_content=")";
			node.push_back(instr);

			//fv
			stack[size_t((int)numElt-1)]=fv;
			arity=4;
			instr.m_type=WKSContentListener::FormulaInstruction::F_Function;
			instr.m_content="NPER";
		}

		if ((instr.m_content[0] >= 'A' && instr.m_content[0] <= 'Z') || instr.m_content[0] == '(')
		{
			if (instr.m_content[0] != '(')
				child.push_back(instr);

			instr.m_type=WKSContentListener::FormulaInstruction::F_Operator;
			instr.m_content="(";
			child.push_back(instr);
			for (int i = 0; i < arity; i++)
			{
				if (i)
				{
					instr.m_content=";";
					child.push_back(instr);
				}
				std::vector<WKSContentListener::FormulaInstruction> const &node=
				    stack[size_t((int)numElt-arity+i)];
				child.insert(child.end(), node.begin(), node.end());
			}
			instr.m_content=")";
			child.push_back(instr);

			stack.resize(size_t((int) numElt-arity+1));
			stack[size_t((int)numElt-arity)] = child;
			continue;
		}
		if (arity==1)
		{
			instr.m_type=WKSContentListener::FormulaInstruction::F_Operator;
			stack[numElt-1].insert(stack[numElt-1].begin(), instr);
			if (wh==3) break;
			continue;
		}
		if (arity==2)
		{
			instr.m_type=WKSContentListener::FormulaInstruction::F_Operator;
			stack[numElt-2].push_back(instr);
			stack[numElt-2].insert(stack[numElt-2].end(), stack[numElt-1].begin(), stack[numElt-1].end());
			stack.resize(numElt-1);
			continue;
		}
		ok=false;
		error = "### unexpected arity";
		break;
	}

	if (!ok) ;
	else if (stack.size()==1 && stack[0].size()>1 && stack[0][0].m_content=="=")
	{
		formula.insert(formula.begin(),stack[0].begin()+1,stack[0].end());
		return true;
	}
	else
		error = "###stack problem";

	static bool first = true;
	if (first)
	{
		WPS_DEBUG_MSG(("LotusSpreadsheet::readFormula: I can not read some formula\n"));
		first = false;
	}

	f.str("");
	for (size_t i = 0; i < stack.size(); ++i)
	{
		for (size_t j=0; j < stack[i].size(); ++j)
			f << stack[i][j] << ",";
	}
	f << error;
	error = f.str();
	return false;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
