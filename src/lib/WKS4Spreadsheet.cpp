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

#include "WKS4.h"

#include "WKS4Spreadsheet.h"

namespace WKS4SpreadsheetInternal
{

///////////////////////////////////////////////////////////////////
//! a class used to store a style of a cell in WKS4Spreadsheet
struct Style : public WPSCellFormat
{
	//! construtor
	explicit Style(libwps_tools_win::Font::Type type) : WPSCellFormat(), m_font(), m_fontType(type), m_extra("")
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
			WPS_DEBUG_MSG(("WKS4ParserInternal::StyleManager::get can not find style %d\n", id));
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
			WPS_DEBUG_MSG(("WKS4ParserInternal::StyleManager::print: can not find a style\n"));
			o << ", ###style=" << id;
		}
	}

protected:
	//! the styles
	std::vector<Style> m_stylesList;
};

//! a cellule of a WKS4 spreadsheet
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
		WPS_DEBUG_MSG(("WKS4SpreadsheetInternal::Cell::send: must not be called\n"));
		return false;
	}

	//! call when the content of a cell must be send
	bool sendContent(WPSListenerPtr &/*listener*/)
	{
		WPS_DEBUG_MSG(("WKS4SpreadsheetInternal::Cell::sendContent: must not be called\n"));
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
	//! the spreadsheet type
	enum Type { T_Spreadsheet, T_Filter, T_Report };

	//! a constructor
	Spreadsheet(Type type=T_Spreadsheet, int id=0) : m_type(type), m_id(id), m_numCols(0), m_numRows(0),
		m_widthCols(), m_rowHeightMap(), m_heightDefault(16), m_positionToCellMap(), m_lastCellPos(),
		m_rowPageBreaksList() {}
	//! return a cell corresponding to a spreadsheet, create one if needed
	Cell &getCell(Vec2i const &pos)
	{
		if (m_positionToCellMap.find(pos)==m_positionToCellMap.end())
		{
			Cell cell;
			cell.setPosition(pos);
			m_positionToCellMap[pos]=cell;
		}
		m_lastCellPos=pos;
		return m_positionToCellMap.find(pos)->second;
	}
	//! returns the last cell
	Cell *getLastCell()
	{
		if (m_positionToCellMap.find(m_lastCellPos)==m_positionToCellMap.end())
			return 0;
		return &m_positionToCellMap.find(m_lastCellPos)->second;
	}
	//! set the columns size
	void setColumnWidth(int col, int w=-1)
	{
		if (col < 0) return;
		if (col >= int(m_widthCols.size())) m_widthCols.resize(size_t(col)+1, -1);
		m_widthCols[size_t(col)] = w;
		if (col >= m_numCols) m_numCols=col+1;
	}

	//! returns the row size in point
	float getRowHeight(int row) const
	{
		std::map<Vec2i,int>::const_iterator rIt=m_rowHeightMap.lower_bound(Vec2i(-1,row));
		if (rIt!=m_rowHeightMap.end() && rIt->first[0]<=row && rIt->first[1]>=row)
			return (float) rIt->second/20.f;
		return (float) m_heightDefault;
	}
	/** returns the height of a row in point and updated repeated row

		\note: you must first call compressRowHeigths
	 */
	float getRowHeight(int row, int &numRepeated) const
	{
		std::map<Vec2i,int>::const_iterator rIt=m_rowHeightMap.lower_bound(Vec2i(-1,row));
		if (rIt!=m_rowHeightMap.end() && rIt->first[0]<=row && rIt->first[1]>=row)
		{
			numRepeated=rIt->first[1]-row+1;
			return (float) rIt->second/20.f;
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
		int const defHeight=m_heightDefault*20; // conversion from point to TWIP
		while (rIt!=oldMap.end())
		{
			// first check for not filled row
			if (rIt->first[0]!=actPos[1]+1)
			{
				if (actHeight==defHeight)
					actPos[1]=rIt->first[0]-1;
				else
				{
					if (actPos[1]>=actPos[0])
						m_rowHeightMap[actPos]=actHeight;
					actHeight=defHeight;
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
	//! convert the m_widthCols in a vector of of point size
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
		return m_positionToCellMap.empty();
	}
	//! the spreadsheet type
	Type m_type;
	//! the spreadsheet id
	int m_id;
	/** the number of columns */
	int m_numCols;
	/** the number of rows */
	int m_numRows;

	/** the column size in TWIP (?) */
	std::vector<int> m_widthCols;
	/** the map Vec2i(min row, max row) to size in TWIP (?) */
	std::map<Vec2i,int> m_rowHeightMap;
	/** the default row size in point */
	int m_heightDefault;
	/** a map cell to not empty cells */
	std::map<Vec2i, Cell> m_positionToCellMap;
	/** the last cell position */
	Vec2i m_lastCellPos;
	/** the list of row page break */
	std::vector<int> m_rowPageBreaksList;

};

//! the state of WKS4Spreadsheet
struct State
{
	//! constructor
	State() :  m_eof(-1), m_version(-1), m_hasLICSCharacters(-1), m_styleManager(), m_spreadsheetList(), m_spreadsheetStack()
	{
		pushNewSheet(shared_ptr<Spreadsheet>(new Spreadsheet(Spreadsheet::T_Spreadsheet, 0)));
	}
	//! returns the maximal spreadsheet
	int getMaximalSheet(Spreadsheet::Type type=Spreadsheet::T_Spreadsheet) const
	{
		int max=-1;
		for (size_t i=0; i<m_spreadsheetList.size(); ++i)
		{
			shared_ptr<Spreadsheet> sheet=m_spreadsheetList[i];
			if (!sheet || sheet->m_type != type || sheet->m_id<=max || sheet->empty()) continue;
			max=sheet->m_id;
		}
		return max;
	}
	//! returns the ith real spreadsheet
	shared_ptr<Spreadsheet> getSheet(Spreadsheet::Type type, int id)
	{
		for (size_t i=0; i<m_spreadsheetList.size(); ++i)
		{
			shared_ptr<Spreadsheet> sheet=m_spreadsheetList[i];
			if (!sheet || sheet->m_type!=type || sheet->m_id!=id)
				continue;
			return sheet;
		}
		return shared_ptr<Spreadsheet>();
	}
	//! returns the ith spreadsheet
	librevenge::RVNGString getSheetName(int id) const
	{
		librevenge::RVNGString name;
		name.sprintf("Sheet%d", id+1);
		return name;
	}
	//! returns the actual sheet
	Spreadsheet &getActualSheet()
	{
		return *m_spreadsheetStack.top();
	}
	//! create a new sheet and stack id
	void pushNewSheet(shared_ptr<Spreadsheet> sheet)
	{
		if (!sheet)
		{
			WPS_DEBUG_MSG(("WKS4SpreadsheetInternal::State::pushSheet: can not find the sheet\n"));
			return;
		}
		m_spreadsheetStack.push(sheet);
		m_spreadsheetList.push_back(sheet);
	}
	//! try to pop the actual sheet
	bool popSheet()
	{
		if (m_spreadsheetStack.size()<=1)
		{
			WPS_DEBUG_MSG(("WKS4SpreadsheetInternal::State::popSheet: can pop the main sheet\n"));
			return false;
		}
		m_spreadsheetStack.pop();
		return true;
	}
	//! the last file position
	long m_eof;
	//! the file version
	int m_version;
	//! int to code if the file has LICS characters:-1 means unknown, 0 means no, 1 means yes
	int m_hasLICSCharacters;
	//! the style manager
	StyleManager m_styleManager;

	//! the list of spreadsheet ( first: main spreadsheet, other report spreadsheet )
	std::vector<shared_ptr<Spreadsheet> > m_spreadsheetList;
	//! the stack of spreadsheet id
	std::stack<shared_ptr<Spreadsheet> > m_spreadsheetStack;
};

}

// constructor, destructor
WKS4Spreadsheet::WKS4Spreadsheet(WKS4Parser &parser) :
	m_input(parser.getInput()), m_listener(), m_mainParser(parser), m_state(new WKS4SpreadsheetInternal::State),
	m_asciiFile(parser.ascii())
{
	m_state.reset(new WKS4SpreadsheetInternal::State);
}

WKS4Spreadsheet::~WKS4Spreadsheet()
{
}

int WKS4Spreadsheet::version() const
{
	if (m_state->m_version<0)
		m_state->m_version=m_mainParser.version();
	return m_state->m_version;
}

bool WKS4Spreadsheet::hasLICSCharacters() const
{
	if (m_state->m_hasLICSCharacters<0)
		m_state->m_hasLICSCharacters=m_mainParser.hasLICSCharacters() ? 1 : 0;
	return m_state->m_hasLICSCharacters==1;
}

bool WKS4Spreadsheet::checkFilePosition(long pos)
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

int WKS4Spreadsheet::getNumSpreadsheets() const
{
	return m_state->getMaximalSheet(WKS4SpreadsheetInternal::Spreadsheet::T_Spreadsheet)+1;
}

////////////////////////////////////////////////////////////
// low level

////////////////////////////////////////////////////////////
//   parse sheet data
////////////////////////////////////////////////////////////
bool WKS4Spreadsheet::readSheetSize()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x6)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readSheetSize: not a sheet zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz < 8)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readSheetSize: block is too short\n"));
		return false;
	}
	f << "Entries(SheetSize):";
	for (int i = 0; i < 2; i++)   // always 2 zero ?
	{
		int val = libwps::read16(m_input);
		if (!val) continue;
		f << "f" << i << "=" << std::hex << val << std::dec << ",";
	}
	int nCol = libwps::read16(m_input)+1;
	f << "nCols=" << nCol << ",";
	int nRow =  libwps::read16(m_input);
	f << "nRow=" << nRow << ",";
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	// empty spreadsheet
	if (nRow==-1 && nCol==0) return true;
	if (nRow < 0 || nCol <= 0) return false;

	m_state->getActualSheet().setColumnWidth(nCol-1);
	return true;

}

bool WKS4Spreadsheet::readColumnSize()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x8)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readColumnSize: not a column size zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz < 3)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readColumnSize: block is too short\n"));
		return false;
	}

	int col = libwps::read16(m_input);
	int width = libwps::readU8(m_input);

	bool ok = col >= 0 && col < m_state->getActualSheet().m_numCols+10;
	f << "Entries(Column):Col" << col << "";
	if (!ok) f << "###";
	f << ":width=" << width << ",";

	if (ok)
	{
		if (col >= m_state->getActualSheet().m_numCols)
		{
			static bool first = true;
			if (first)
			{
				first = false;
				WPS_DEBUG_MSG(("WKS4Spreadsheet::readColumnSize: I must increase the number of columns\n"));
			}
			f << "#col[inc],";
		}
		// checkme: unit in character(?) -> TWIP
		m_state->getActualSheet().setColumnWidth(col, width*105);
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return ok;
}

bool WKS4Spreadsheet::readHiddenColumns()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x64)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readHiddenColumns: not a column hidden zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz != 32)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readHiddenColumns: block size seems bad\n"));
		return false;
	}

	f << "Entries(HiddenCol):";
	for (int i=0; i<32; ++i)
	{
		int val=(int) libwps::readU8(m_input);
		if (!val) continue;
		static bool first=true;
		if (first)
		{
			WPS_DEBUG_MSG(("WKS4Spreadsheet::readHiddenColumns: find some hidden col, ignored\n"));
			first=false;
		}
		// checkme
		for (int j=0, depl=1; j<8; ++j, depl<<=1)
		{
			if (val&depl)
				f << "col" << j+8*i << "[hidden],";
		}
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

////////////////////////////////////////////////////////////
// MSWorks
////////////////////////////////////////////////////////////
bool WKS4Spreadsheet::readMsWorksColumnSize()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x546b)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksColumnSize: not a column size zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz != 4)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksColumnSize: block size is odd\n"));
		return false;
	}

	int col = libwps::read16(m_input);
	int width = libwps::readU16(m_input); // unit? 1point ~ 115

	bool ok = col >= 0 && col < m_state->getActualSheet().m_numCols+10;
	if (ok)
		m_state->getActualSheet().setColumnWidth(col, width & 0x7FFF);

	f << "Entries(Colum2):Col" << col << ":";
	if (!ok) f << "###";
	if (width & 0x8000)
	{
		f << "flag?,";
		width &= 0x7FFF;
	}
	f << "width(unit?)=" << width;

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool WKS4Spreadsheet::readMsWorksRowSize()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x5465)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksRowSize: not a row size zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz != 4)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksRowSize: block size is odd\n"));
		return false;
	}

	int row = libwps::read16(m_input);
	int width = libwps::readU16(m_input); // unit? 1point ~ 105

	bool ok = row >= 0;
	if (ok) m_state->getActualSheet().setRowHeight(row, width & 0x7FFF);
	f << "Entries(Row2):Row" << row << ":";
	if (!ok) f << "###";
	if (width & 0x8000)
	{
		f << "flag?,";
		width &= 0x7FFF;
	}
	f << "width(unit?)=" << width;

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool WKS4Spreadsheet::readMsWorksPageBreak()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);

	if (type != 0x5413)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksPageBreak: not a pgbreak zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz < 2)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksPageBreak: seems very short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(PBRK)###");
		return true;
	}
	int row = libwps::read16(m_input)+1;
	m_state->getActualSheet().m_rowPageBreaksList.push_back(row);

	f << "Entries(PBRK): row="<< row;
	if (sz != 2) ascii().addDelimiter(m_input->tell(),'#');

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Spreadsheet::readMsWorksStyle()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x545a)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksStyle: not a style property\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	long endPos=pos+4+sz;
	if (sz < 8)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksStyle: style property is too short\n"));
		return false;
	}

	// win3 has 8 bytes while last version is at least 10 bytes
	int fl[6];
	for (int i=0; i<2; ++i) fl[i] = libwps::readU8(m_input);
	for (int i=0; i<3; ++i) fl[2+i] = libwps::readU16(m_input);
	fl[5]= sz>=10 ? libwps::readU16(m_input) : 0;

	WKS4SpreadsheetInternal::Style style(m_mainParser.getDefaultFontType());
	if (!m_mainParser.getFont(fl[3], style.m_font, style.m_fontType))
	{
		f << ",#fontId = " << fl[3];

		static bool first = true;
		if (first)
		{
			WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksStyle: can not find a font\n"));
			first = false;
		}
	}
	fl[3]=0;


	if (fl[1] & 0x1C)
	{
		switch ((fl[1] & 0x1C) >> 2)
		{
		case 0:
			break;
		case 1:
		case 4: // left rellenar
			style.setHAlignement(WPSCell::HALIGN_LEFT);
			break;
		case 2:
		case 5: // center in selection
			style.setHAlignement(WPSCell::HALIGN_CENTER);
			break;
		case 3:
			style.setHAlignement(WPSCell::HALIGN_RIGHT);
			break;
		default:
			f << ",#align=" << ((fl[1] & 0x1C) >> 2);
			break;
		}
		fl[1] &= 0xE3;
	}
	switch ((fl[0]&0xf))
	{
	case 0:
		style.setFormat(WPSCell::F_NUMBER,1);
		break;
	case 1:
		style.setFormat(WPSCell::F_NUMBER,2);
		break;
	case 2:
		style.setFormat(WPSCell::F_NUMBER,4);
		break;
	case 3:
		style.setFormat(WPSCell::F_NUMBER,3);
		break;
	case 4:
		style.setFormat(WPSCell::F_NUMBER,5);
		break;
	case 5:
	{
		int wh=(fl[0]>>5);
		switch (wh)
		{
		case 0:
			style.setFormat(WPSCell::F_TEXT);
			break;
		case 1:
			style.setFormat(WPSCell::F_BOOLEAN);
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		{
			char const *(format[]) = { "%H:%M%p", "%I:%M:%S%p", "%H:%M", "%H:%M:%S" };
			style.setDTFormat(WPSCell::F_TIME, format[wh-2]);
			break;
		}
		default:
			f << "#type=" << std::hex << fl[0] << std::dec << ",";
			break;
		}
		break;
	}
	case 6:
	{
		char const *(format[]) = { "%m/%d/%Y", "%d %B %Y", "%m/%Y", "%B %Y", "%m/%d", "%d %B", "%m/%d/%y:6"/*TODO*/, "%B"};
		style.setDTFormat(WPSCell::F_DATE, format[int(fl[0]>>5)]);
		fl[0] &= 0x18;
		break;
	}
	case 0xa:
		style.setFormat(WPSCell::F_NUMBER, 6);
		break;
	case 0xc:
		style.setFormat(WPSCell::F_NUMBER, 7);
		break;
	case 0xd:
		style.setFormat(WPSCell::F_NUMBER, 4);
		f << "negative[inRed],";
		break;
	default:
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksStyle: find unknown format %d\n", (fl[0]&0xF)));
		f << ", ##type=" << std::hex << (fl[0]&0xf) << std::dec << ",";
		break;
	}
	fl[0] &= 0xF0;

	if (style.getFormat() == WPSCell::F_NUMBER)
	{
		int digits = 0;
		if (fl[1] &= 0x1)
		{
			digits = 8;
			fl[1] &= 2;
		}
		digits += (fl[0]>>5);
		fl[0] &= 0x18;
		style.setDigits(digits);
	}
	if (fl[1]&0x20) f << "ajust[text],";
	switch (fl[1]>>6)
	{
	case 0:
		style.setVAlignement(WPSCellFormat::VALIGN_BOTTOM);
		break;
	case 1:
		style.setVAlignement(WPSCellFormat::VALIGN_CENTER);
		break;
	case 2:
		style.setVAlignement(WPSCellFormat::VALIGN_TOP);
		break;
	default:
		f << "##vAlign=3,";
		break;
	}
	fl[1] &= 0x10; // 10
	//
	// the color
	//

	WPSColor frontColor=WPSColor::black();
	int colorId=(fl[4]>>5)&0xF;
	if (colorId && m_mainParser.getColor(colorId,frontColor))   // primary color
		f << "primColor=" << std::hex << frontColor << std::dec << ",";

	WPSColor backColor=WPSColor::white();
	colorId=(fl[4]>>9)&0xF;
	if (colorId && m_mainParser.getColor(colorId,backColor))   // secondary color
		f << "secColor=" << std::hex << backColor << std::dec << ",";

	int pat= (fl[4]&0xf);
	if (pat==15) f << "###pat15,";
	else if (pat)
	{
		static float const patternPercent[]= {0, 1.f, 0.5f, 0.25f, 0.2f, 0.2f, 0.2f, 0.5f, 0.5f, 0.5f, 0.5f, 0.8f, 0.2f, 0.8f, 0.2f};
		float percent=patternPercent[pat];
		f << "patPercent=" << percent << ",";
		style.setBackgroundColor(WPSColor::barycenter(percent,frontColor, 1.0f-percent, backColor));
	}
	fl[4]&=0xE010;

	//
	// Border
	//
	for (int b=0, decal=0; b<4; b++, decal+=4)
	{
		int const wh[]= {WPSBorder::LeftBit, WPSBorder::TopBit, WPSBorder::RightBit, WPSBorder::BottomBit};
		int bType=(fl[2]>>decal)&0xf;
		if (bType==0) continue;
		WPSBorder border;
		switch (bType&0x7)
		{
		case 1:
			break;
		case 2:
			border.m_width=2;
			break;
		case 3:
			border.m_style = WPSBorder::LargeDot;
			break;
		case 4:
			border.m_style = WPSBorder::Dash;
			break;
		case 5:
			border.m_type = WPSBorder::Double;
			break;
		case 6:
			border.m_style = WPSBorder::Dot;
			break;
		case 7:
			border.m_width=3;
			break;
		default:
			break;
		}
		if ((fl[5]>>decal)&0xf)
			m_mainParser.getColor(((fl[5]>>decal)&0xf), border.m_color);
		style.setBorders(wh[b], border);
		if (bType&0x8) f << "bType[" << b << "]&8,";
	}
	fl[2] = fl[5] = 0;

	for (int i = 0; i < 6; i++) style.m_unknFlags[i] = fl[i];
	style.m_extra = f.str();

	/* end of parsing */
	f.str("");
	f << "Entries(Style):Style" << m_state->m_styleManager.size() << "," << style;
	m_state->m_styleManager.add(style, false);
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	if (m_input->tell()!=endPos) ascii().addDelimiter(m_input->tell(),'#');
	return true;
}

////////////////////////////////////////////////////////////
// MsWorks (only in dos)
////////////////////////////////////////////////////////////

bool WKS4Spreadsheet::readMsWorksDOSCellProperty()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x5402)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSCellProperty: not a cell property\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz < 2)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSCellProperty: cell property is too short\n"));
		return false;
	}

	f << "Entries(CellDosProperty):";
	WKS4SpreadsheetInternal::Cell *cell = m_state->getActualSheet().getLastCell();
	if (!cell)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSCellProperty: can not find the cell\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	WKS4SpreadsheetInternal::Style style(m_mainParser.getDefaultFontType());
	if (cell->m_styleId>=0)
	{
		if (!m_state->m_styleManager.get(cell->m_styleId, style))
			f << ",###style";
	}

	int fl[2];
	for (int i = 0; i < 2; i++)
		fl[i] = libwps::readU8(m_input);

	switch ((fl[0] & 0x7))
	{
	case 0x5:
		if (cell->m_content.m_contentType == WKSContentListener::CellContent::C_TEXT) fl[0] &= 0xF8;
		break;
	case 0x6:
		if (cell->m_content.m_contentType == WKSContentListener::CellContent::C_NUMBER) fl[0] &= 0xF8;
		break;
	case 0x7:
		if (cell->m_content.m_contentType == WKSContentListener::CellContent::C_FORMULA) fl[0] &= 0xF8;
		break;
	default:
		break;
	}
	WPSCell::FormatType newForm = WPSCell::F_UNKNOWN;
	int subForm = 0;
	switch (fl[0]>>5)
	{
	case 0:
		newForm = WPSCell::F_NUMBER;
		subForm=1;
		break;
	case 1:
		newForm = WPSCell::F_NUMBER;
		subForm=2;
		break;
	case 2:
		newForm = WPSCell::F_NUMBER;
		subForm=4;
		break;
	case 3:
		newForm = WPSCell::F_NUMBER;
		subForm=3;
		break;
	case 4:
		newForm = WPSCell::F_NUMBER;
		subForm=5;
		break;
	case 5:   // normal (or time)
	{
		int wh=(fl[1]>>2)&0x7;
		if (wh>=2 && wh <=5)
		{
			newForm = WPSCell::F_TIME;
			subForm=wh-2;
			fl[1]&=0xE3;
		}
		break;
	}
	case 6:
		newForm = WPSCell::F_DATE;
		subForm=(fl[1]>>2)&0x7;
		fl[1]&=0xE3;
		break;
	default:
		f << "fl0[high]=" << (fl[0]>>5) << ",";
		break;
	}
	fl[0] &= 0x1F;
	if (newForm != WPSCell::F_UNKNOWN &&
	        (newForm != style.getFormat() || subForm != style.getSubFormat()))
	{
		if (style.getFormat() == WPSCell::F_UNKNOWN)
			;
		else if (newForm != style.getFormat())
			f << "#prevForm = " << int(style.getFormat());
		else
			f << "#prevSubForm = " << style.getSubFormat();
		if (newForm==WPSCell::F_DATE && subForm>=0 && subForm<=7)
		{
			char const *(wh[])= { "%m/%d/%y", "%B %d, %Y", "%m/%y", "%B %Y", "%m/%d", "%B %d", "%m", "%B"};
			style.setDTFormat(newForm, wh[subForm]);
		}
		else if (newForm==WPSCell::F_TIME && subForm>=0 && subForm<=3)
		{
			char const *(wh[])= { "%I:%M%p", "%I:%M:%S%p", "%H:%M", "%H:%M:%S"};
			style.setDTFormat(newForm, wh[subForm]);
		}
		else
			style.setFormat(newForm, subForm);
	}

	uint32_t fflags = 0; // checkme
	if (fl[0] & 0x10)
	{
		fflags |= WPS_ITALICS_BIT;
		fl[0] &= 0xEF;
	}
	if (fl[1] & 0x20)
	{
		fflags |= WPS_BOLD_BIT;
		fl[1] &= 0xDF;
	}
	if (fl[1] & 0x40)
	{
		fflags |= WPS_UNDERLINE_BIT;
		fl[1] &= 0xBF;
	}
	style.m_font.m_attributes=fflags;
	switch (fl[1]&3)
	{
	case 1:
		style.setHAlignement(WPSCell::HALIGN_LEFT);
		break;
	case 2:
		style.setHAlignement(WPSCell::HALIGN_CENTER);
		break;
	case 3:
		style.setHAlignement(WPSCell::HALIGN_RIGHT);
		break;
	case 0:
	default:
		break;
	}
	fl[1] &= 0xFC;
	style.m_unknFlags[0] = fl[0];
	style.m_unknFlags[1] = fl[1];
	int id = m_state->m_styleManager.add(style, true);
	cell->m_styleId=id;

#ifdef DEBUG_WITH_FILES
	f << *cell;
	m_state->m_styleManager.print(id, f);
#endif
	if (sz > 2) ascii().addDelimiter(pos+6, '#');

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Spreadsheet::readMsWorksDOSFieldProperty()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x5406)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSFieldProperty: not a cell property\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz < 4)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSFieldProperty: cell property is too short\n"));
		return false;
	}

	f << "Entries(FieldDosProperty):";
	f << "id=" << libwps::readU16(m_input) << ",";
	WKS4SpreadsheetInternal::Style style(m_mainParser.getDefaultFontType());
	int fl[2];
	for (int i = 0; i < 2; i++)
		fl[i] = libwps::readU8(m_input);

	switch ((fl[0] & 0x7))
	{
	case 0x5:
		f << "text,";
		fl[0] &= 0xF8;
		break;
	case 0x6:
		f << "number,";
		fl[0] &= 0xF8;
		break;
	case 0x7:
		f << "formula,";
		fl[0] &= 0xF8;
		break;
	default:
		break;
	}
	if (fl[0]>>5) f << "subForm=" << (fl[0]>>5) << ",";
	fl[0] &= 0x1F;
	uint32_t fflags = 0; // checkme
	if (fl[0] & 0x10)
	{
		fflags |= WPS_ITALICS_BIT;
		fl[0] &= 0xEF;
	}
	if (fl[1] & 0x20)
	{
		fflags |= WPS_BOLD_BIT;
		fl[1] &= 0xDF;
	}
	if (fl[1] & 0x40)
	{
		fflags |= WPS_UNDERLINE_BIT;
		fl[1] &= 0xBF;
	}
	style.m_font.m_attributes=fflags;
	switch (fl[1]&3)
	{
	case 1:
		style.setHAlignement(WPSCell::HALIGN_LEFT);
		break;
	case 2:
		style.setHAlignement(WPSCell::HALIGN_CENTER);
		break;
	case 3:
		style.setHAlignement(WPSCell::HALIGN_RIGHT);
		break;
	case 0:
	default:
		break;
	}
	fl[1] &= 0xFC;
	style.m_unknFlags[0] = fl[0];
	style.m_unknFlags[1] = fl[1];
	f << style;
	if (sz > 4) ascii().addDelimiter(pos+8, '|');

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;

}

bool WKS4Spreadsheet::readMsWorksDOSCellExtraProperty()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x541c)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSCellExtraProperty: not a cell property\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz < 8)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSCellExtraProperty: cell property is too short\n"));
		return false;
	}

	f << "Entries(CellDosExtra):";
	WKS4SpreadsheetInternal::Cell *cell = m_state->getActualSheet().getLastCell();
	if (!cell)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSCellExtraProperty: can not find the cell\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	WKS4SpreadsheetInternal::Style style(m_mainParser.getDefaultFontType());
	if (cell->m_styleId>=0)
	{
		if (!m_state->m_styleManager.get(cell->m_styleId, style))
			f << ",###style";
	}
	int values[8];  // f0: [02468ac][0157], f1=0..9, f2=0..a, f3=[0-6a][0156], f4=0|9|d|40|80|c0, f5=0|ed
	for (int i=0; i < 8; ++i)
		values[i]=(int) libwps::readU8(m_input);
	if (style.getFormat()==WPSCell::F_NUMBER)   // not sure
	{
		if (values[2]==5)
		{
			style.setFormat(WPSCell::F_NUMBER,7);
			values[2]=0;
		}
		else if (values[2]==0xa)
		{
			style.setFormat(WPSCell::F_NUMBER,6);
			style.setDigits(((values[3]>>3)&0x7)+1);
			values[2]=0;
			values[3]&=0xC7;
		}
	}
	WPSColor color;
	if ((values[6]&0xE0) && m_mainParser.getColor(values[6]>>5,color))
	{
		style.m_font.m_color=color;
		f << "fontColor=" << std::hex << color << std::dec << ",";
		values[6] &= 0x1F;
	}
	for (int i=0; i < 8; ++i)
	{
		if (values[i])
			f << "f" << i << "=" << std::hex << values[i] << std::dec << ",";
	}

	int id = m_state->m_styleManager.add(style, true);
	cell->m_styleId=id;

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	if (m_input->tell()!=pos+4+sz) ascii().addDelimiter(m_input->tell(),'#');

	return false;
}

bool WKS4Spreadsheet::readMsWorksDOSPageBreak()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);

	if (type != 0x5427)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readMsWorksDOSPageBreak: not a pgbreak zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (!sz)
	{
		ascii().addPos(pos);
		ascii().addNote("_");
		return true;
	}
	int row = (int) libwps::read8(m_input)+1;
	m_state->getActualSheet().m_rowPageBreaksList.push_back(row);

	f << "Entries(PBRK): row="<< row;
	if (sz != 1) ascii().addDelimiter(m_input->tell(),'#');

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
// general
////////////////////////////////////////////////////////////

bool WKS4Spreadsheet::readCell()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if ((type != 0x545b) && (type < 0xc || type > 0x10))
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: not a cell property\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	long endPos = pos+4+sz;

	if (sz < 5)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: cell def is too short\n"));
		return false;
	}
	int const vers=version();
	bool dosFile = vers < 3;
	int format = 0xFF;
	if (dosFile)
		format = (int) libwps::readU8(m_input);
	int cellPos[2];
	cellPos[0]=(int) libwps::readU8(m_input);
	int sheetId=(int) libwps::readU8(m_input);
	cellPos[1]=(int) libwps::read16(m_input);
	if (cellPos[1] < 0)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: cell pos is bad\n"));
		return false;
	}
	if (sheetId)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: find unexpected sheet id\n"));
		f << "###sheet[id]=" << sheetId << ",";
	}

	WKS4SpreadsheetInternal::Cell &cell=m_state->getActualSheet().getCell(Vec2i(cellPos[0],cellPos[1]));
	if (!dosFile)
		cell.m_styleId = (int) libwps::read16(m_input);

	if (type & 0xFF00)
	{
		if (dosFile)
		{
			static bool first = true;
			if (first)
			{
				first = false;
				WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: find type=%ld in dos version\n", type));
			}
			f << "###";
		}
		f << "win[version],";
	}

	long dataPos = m_input->tell();
	int dataSz = int(endPos-dataPos);

	bool ok = true;
	switch (type)
	{
	case 12:
	{
		if (dataSz == 0)
		{
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NONE;
			break;
		}
		ok = false;
		break;
	}
	case 13:
	{
		if (dataSz == 2)
		{
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
			cell.m_content.setValue(libwps::read16(m_input));
			break;
		}
		ok = false;
		break;
	}
	case 14:
	{
		double val;
		bool isNaN;
		if (dataSz == 8 && libwps::readDouble8(m_input, val, isNaN))
		{
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
			cell.m_content.setValue(val);
			break;
		}
		ok = false;
		break;
	}
	case 15:
	{
		cell.m_content.m_contentType=WKSContentListener::CellContent::C_TEXT;
		long begText=m_input->tell(), endText=begText+dataSz;
		std::string s("");
		for (int i = 0; i < dataSz; i++)
		{
			char c = (char) libwps::read8(m_input);
			if (c=='\0')
			{
				endText=m_input->tell()-1;
				if (i == dataSz-1) break;
				WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: cell content seems bad\n"));
				f << "###";
				break;
			}
			s += c;
		}
		f << s << ",";
		if (dosFile && s.length())
		{
			if (s[0]=='\'') cell.m_hAlign=WPSCellFormat::HALIGN_DEFAULT;
			else if (s[0]=='\\') cell.m_hAlign=WPSCellFormat::HALIGN_LEFT;
			else if (s[0]=='^') cell.m_hAlign=WPSCellFormat::HALIGN_CENTER;
			else if (s[0]=='\"') cell.m_hAlign=WPSCellFormat::HALIGN_RIGHT;
			else
				--begText;
			++begText;
		}
		cell.m_content.m_textEntry.setBegin(begText);
		cell.m_content.m_textEntry.setEnd(endText);
		break;
	}
	case 16:
	{
		double val;
		bool isNaN;
		if (dataSz >= 8 && libwps::readDouble8(m_input, val, isNaN))
		{
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_FORMULA;
			cell.m_content.setValue(val);
			std::string error;
			if (!readFormula(endPos, cell.position(), cell.m_content.m_formula, error))
			{
				cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
				ascii().addDelimiter(m_input->tell()-1, '#');
			}
			if (error.length()) f << error;
			break;
		}
		ok = false;
		break;
	}
	case 0x545b:   // CHECKME: sometime we do not read the good number
	{
		double val;
		bool isNaN;
		if (dataSz == 4 && libwps::readDouble4(m_input, val, isNaN))
		{
			cell.m_content.m_contentType=WKSContentListener::CellContent::C_NUMBER;
			cell.m_content.setValue(val);
			break;
		}
		ok = false;
		break;
	}
	default:
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: unknown type=%ld\n", type));
		ok = false;
		break;
	}
	if (!ok) ascii().addDelimiter(dataPos, '#');

	if (dosFile)
	{
		WKS4SpreadsheetInternal::Style style(m_mainParser.getDefaultFontType());
		switch (cell.m_content.m_contentType)
		{
		case WKSContentListener::CellContent::C_NONE:
			break;
		case WKSContentListener::CellContent::C_TEXT:
			style.setFormat(WPSCell::F_TEXT);
			break;
		case WKSContentListener::CellContent::C_NUMBER:
		case WKSContentListener::CellContent::C_FORMULA:
		case WKSContentListener::CellContent::C_UNKNOWN:
		default:
			style.setFormat(WPSCell::F_NUMBER);
			break;
		}

		int styleID = (format>>4) & 0x7;
		switch (styleID)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
				break;
			if (styleID == 2) styleID = 3;
			else if (styleID == 3) styleID = 2;
			style.setFormat(WPSCell::F_NUMBER,1+styleID);
			break;
		// unsure find in some Quattro Pro File
		case 5:
			if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
				break;
			switch (format&0xF)
			{
			case 4: // a date but no sure which format is good
				style.setDTFormat(WPSCell::F_DATE, "%m/%d/%y");
				break;
			case 5:
				style.setDTFormat(WPSCell::F_DATE, "%m/%d");
				break;
			default:
				WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell::updateFormat: unknown format %x\n", (unsigned) format));
				break;
			}
			break;
		// LotusSymphony format
		case 0x7:
			switch (format&0xF)
			{
			case 0: // +/- : kind of bool
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setFormat(WPSCell::F_BOOLEAN);
				break;
			case 1:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setFormat(WPSCell::F_NUMBER, 0);
				break;
			case 2:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_DATE, "%d %B %y");
				break;
			case 3:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_DATE, "%d %B");
				break;
			case 4:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_DATE, "%B %y");
				break;
			case 5:
				if (cell.m_content.m_contentType!=WKSContentListener::CellContent::C_TEXT)
					break;
				style.setFormat(WPSCell::F_TEXT);
				break;
			case 6:
				if (cell.m_content.m_contentType!=WKSContentListener::CellContent::C_TEXT)
					break;
				style.setFormat(WPSCell::F_TEXT);
				style.m_font.m_attributes |= WPS_HIDDEN_BIT;
				break;
			case 7:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_TIME, "%I:%M:%S%p");
				break;
			case 8:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_TIME, "%I:%M%p");
				break;
			case 9:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_DATE, "%m/%d/%y");
				break;
			case 0xa:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_DATE, "%m/%d");
				break;
			case 0xb:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_TIME, "%H:%M:%S");
				break;
			case 0xc:
				if (cell.m_content.m_contentType==WKSContentListener::CellContent::C_TEXT)
					break;
				style.setDTFormat(WPSCell::F_TIME, "%H:%M");
				break;
			case 0xd:
				if (cell.m_content.m_contentType!=WKSContentListener::CellContent::C_TEXT)
					break;
				style.setFormat(WPSCell::F_TEXT);
				break;
			case 0xf: // automatic
				break;
			default:
				break;
			}
			break;
		default:
			f << "type=" << styleID << ",";
		}
		if ((format&0x80)==0) f << "protected=0,";
		style.setDigits(format & 0xF);
		cell.m_styleId=m_state->m_styleManager.add(style, true);
	}
	m_input->seek(pos+sz, librevenge::RVNG_SEEK_SET);

	std::string extra=f.str();
	f.str("");
	f << "Entries(CellContent):" << cell << "," << extra;
#ifdef DEBUG_WITH_FILES
	m_state->m_styleManager.print(cell.m_styleId, f);
#endif

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool WKS4Spreadsheet::readCellFormulaResult()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x33)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readCellFormulaResult: not a cell property\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz<6)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readCellFormulaResult: the zone seems to short\n"));
		return false;
	}
	long endPos = pos+4+sz;

	bool dosFile = version() < 3;
	// skip format for dosFile
	m_input->seek(dosFile ? pos+5 : pos+4, librevenge::RVNG_SEEK_SET);
	f << "CellContent[res]:";
	int dim[2];
	for (int i=0; i<2; ++i) dim[i]=(int) libwps::readU16(m_input);
	f << "C" << dim[0] << "x" << dim[1] << ",";
	// skip format for windows file
	if (!dosFile) m_input->seek(2, librevenge::RVNG_SEEK_CUR);
	int sSz=int(endPos-m_input->tell());
	std::string text("");
	for (int i=0; i<sSz; ++i)
	{
		char c=(char) libwps::readU8(m_input);
		if (c==0) break;
		text+=c;
	}
	f << text << ",";
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
// Data
////////////////////////////////////////////////////////////
bool WKS4Spreadsheet::readCell
(Vec2i actPos, WKSContentListener::FormulaInstruction &instr)
{
	instr=WKSContentListener::FormulaInstruction();
	instr.m_type=WKSContentListener::FormulaInstruction::F_Cell;
	bool ok = true;
	int pos[2];
	bool absolute[2] = { true, true};
	for (int dim = 0; dim < 2; dim++)
	{
		int val = (int) libwps::readU16(m_input);
		if ((val & 0xF000) == 0); // absolue value ?
		else if ((val & 0xc000) == 0x8000)   // relative ?
		{
			if (version()==1)
			{
				val &= 0xFF;
				if ((val & 0x80) && val+actPos[dim] >= 0x100)
					// sometimes this value is odd, so do not generate errors here
					val = val - 0x100;
			}
			else
			{
				val &= 0x3FFF;
				if (val & 0x2000) val = val - 0x4000;
			}
			val += actPos[dim];
			absolute[dim] = false;
		}
		else if (val==0xFFFF)
		{
			static bool first=true;
			if (first)   // in general associated with a nan value, so maybe be normal
			{
				WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: find some ffff cell\n"));
				first=false;
			}
			ok = false;
		}
		else
		{
			WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: can not read cell %x\n", (unsigned int)val));
			ok = false;
		}
		pos[dim] = val;
	}

	if (pos[0] < 0 || pos[1] < 0)
	{
		std::stringstream f;
		f << "###[" << pos[1] << "," << pos[0] << "]";
		if (ok)
		{
			WPS_DEBUG_MSG(("WKS4Spreadsheet::readCell: can not read cell position\n"));
		}
		return false;
	}
	instr.m_position[0]=Vec2i(pos[0],pos[1]);
	instr.m_positionRelative[0]=Vec2b(!absolute[0],!absolute[1]);
	return ok;
}

namespace WKS4SpreadsheetInternal
{
struct Functions
{
	char const *m_name;
	int m_arity;
};

static Functions const s_listFunctions[] =
{
	{ "", 0} /*SPEC: number*/, {"", 0}/*SPEC: cell*/, {"", 0}/*SPEC: cells*/, {"=", 1} /*SPEC: end of formula*/,
	{ "(", 1} /* SPEC: () */, {"", 0}/*SPEC: number*/, { "", -2} /*SPEC: text*/, {"", -2}/*unused*/,
	{ "-", 1}, {"+", 2}, {"-", 2}, {"*", 2},
	{ "/", 2}, { "^", 2}, {"=", 2}, {"<>", 2},

	{ "<=", 2},{ ">=", 2},{ "<", 2},{ ">", 2},
	{ "And", 2},{ "Or", 2}, { "Not", 1}, { "+", 1},
	{ "&", 2}, { "", -2} /*unused*/,{ "", -2} /*unused*/,{ "", -2} /*unused*/,
	{ "", -2} /*unused*/,{ "", -2} /*unused*/,{ "", -2} /*unused*/,{ "NA", 0} /*checkme*/,

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
	{ "Cell", 1} /*checkme*/,{ "Trim", 1},{ "", -2} /*UNKN*/,{ "T", 1},

	{ "IsNonText", 1},{ "Exact", 2},{ "", -2} /*UNKN*/,{ "", 3} /*UNKN*/,
	{ "Rate", 3} /*BAD*/,{ "TERM", 3}, { "CTERM", 3}, { "SLN", 3},
	{ "SYD", 4},{ "DDB", 4} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,
	{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,

	{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/, { "", -2} /*UNKN*/,
	{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,
	{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,{ "", -2} /*UNKN*/,
	{ "", -2} /*UNKN*/,{ "And", -1},{ "Or", -1},{ "Not", 1},

};
}

bool WKS4Spreadsheet::readFormula(long endPos, Vec2i const &position,
                                  std::vector<WKSContentListener::FormulaInstruction> &formula, std::string &error)
{
	int const vers=version();
	formula.resize(0);
	error = "";
	long pos = m_input->tell();
	if (endPos - pos < 2) return false;
	int sz = (int) libwps::readU16(m_input);
	if (endPos-pos-2 != sz) return false;

	std::stringstream f;
	std::vector<std::vector<WKSContentListener::FormulaInstruction> > stack;
	bool ok = true;
	while (long(m_input->tell()) != endPos)
	{
		double val;
		bool isNaN;
		pos = m_input->tell();
		if (pos > endPos) return false;
		int wh = (int) libwps::readU8(m_input);
		int arity = 0;
		WKSContentListener::FormulaInstruction instr;
		switch (wh)
		{
		case 0x0:
			if (endPos-pos<9 || !libwps::readDouble8(m_input, val, isNaN))
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
			if (endPos-pos<5)
			{
				f.str("");
				f << "###cell short";
				error=f.str();
				ok = false;
				break;
			}
			ok = readCell(position, instr);
			break;
		case 0x2:
		{
			if (endPos-pos< (vers<=1 ? 7 : 9) || !readCell(position, instr))
			{
				f.str("");
				f << "###list cell short";
				error=f.str();
				ok = false;
				break;
			}
			WKSContentListener::FormulaInstruction instr2;
			if (!readCell(position, instr2))
			{
				ok = false;
				f.str("");
				f << "###list cell short(2)";
				error=f.str();
				break;
			}
			instr.m_type=WKSContentListener::FormulaInstruction::F_CellList;
			instr.m_position[1]=instr2.m_position[0];
			instr.m_positionRelative[1]=instr2.m_positionRelative[0];
			break;
		}
		case 0x5:
			instr.m_type=WKSContentListener::FormulaInstruction::F_Long;
			instr.m_longValue=(long) libwps::read16(m_input);
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
		default:
			if (wh >= 0x90 || WKS4SpreadsheetInternal::s_listFunctions[wh].m_arity == -2)
			{
				f.str("");
				f << "##Funct" << std::hex << wh;
				error=f.str();
				ok = false;
				break;
			}
			instr.m_type=WKSContentListener::FormulaInstruction::F_Function;
			instr.m_content=WKS4SpreadsheetInternal::s_listFunctions[wh].m_name;
			ok=!instr.m_content.empty();
			arity = WKS4SpreadsheetInternal::s_listFunctions[wh].m_arity;
			if (arity == -1) arity = (int) libwps::read8(m_input);
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
			if (wh==3)
				break;
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
		if (m_input->tell()!=endPos)
		{
			WPS_DEBUG_MSG(("WKS4Spreadsheet::readFormula: find some extra data\n"));
			error="##extra data";
			ascii().addDelimiter(m_input->tell(),'#');
		}
		return true;
	}
	else
		error = "###stack problem";

	static bool first = true;
	if (first)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readFormula: I can not read some formula\n"));
		first = false;
	}

	f.str("");
	for (size_t i = 0; i < stack.size(); ++i)
	{
		for (size_t j=0; j < stack[i].size(); ++j)
			f << stack[i][j] << ",";
	}
	f << error << "###";
	error = f.str();
	return false;
}

////////////////////////////////////////////////////////////
// filter
////////////////////////////////////////////////////////////
bool WKS4Spreadsheet::readFilterOpen()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::readU16(m_input);
	if (type != 0x5410)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readFilterOpen: not a filter header\n"));
		return false;
	}
	m_state->pushNewSheet(shared_ptr<WKS4SpreadsheetInternal::Spreadsheet>
	                      (new WKS4SpreadsheetInternal::Spreadsheet
	                       (WKS4SpreadsheetInternal::Spreadsheet::T_Filter, 0)));
	long sz = (long) libwps::readU16(m_input);
	f << "Entries(Filter)[beg]:";
	if (sz!=0)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readFilterOpen: the field size seems odd\n"));
		f << "###";
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Spreadsheet::readFilterClose()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::readU16(m_input);
	if (type != 0x5411)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readFilterClose: not a filter header\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	f << "Entries(Filter)[end]:";
	WKS4SpreadsheetInternal::Spreadsheet const &sheet=m_state->getActualSheet();
	if (sheet.m_type!=WKS4SpreadsheetInternal::Spreadsheet::T_Filter)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readFilterClose: can not find filter spreadsheet\n"));
		f << "###[noOpen],";
	}
	else
		m_state->popSheet();
	if (sz!=0)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readFilterClose: the field size seems odd\n"));
		f << "###";
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
// report
////////////////////////////////////////////////////////////
bool WKS4Spreadsheet::readReportOpen()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::readU16(m_input);
	if (type != 0x5417)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readReportOpen: not a report header\n"));
		return false;
	}
	m_state->pushNewSheet(shared_ptr<WKS4SpreadsheetInternal::Spreadsheet>
	                      (new WKS4SpreadsheetInternal::Spreadsheet
	                       (WKS4SpreadsheetInternal::Spreadsheet::T_Report, 0)));
	long sz = (long) libwps::readU16(m_input);
	long endPos = pos+4+sz;
	f << "Entries(Report)[header]:";
	if (sz < 16+10+7 || !checkFilePosition(endPos))
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readReportOpen: report header too short\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}

	std::string name("");
	for (int i=0; i<16; ++i)
	{
		char c=(char) libwps::readU8(m_input);
		if (c=='\0') break;
		name+=c;
	}
	f << name << ",";
	m_input->seek(pos+4+16, librevenge::RVNG_SEEK_SET);
	int val=(int) libwps::readU8(m_input); // always 0?
	if (val) f << "f0=" << val << ",";
	for (int i=0; i<3; ++i)   // normally -1:0, but I also find 4:1
	{
		val=(int) libwps::read16(m_input);
		int unkn=(int) libwps::readU8(m_input);
		if (val==-1 && unkn==0) continue;
		f << "unk" << i << "=" << val << ":" << unkn << ",";
	}
	int N=(int) libwps::readU16(m_input);
	if (m_input->tell()+N+7>endPos)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readReportOpen: number of fields seems bad\n"));
		f << "###N[fields]=" << N << ",";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	// we must update the number of columns
	m_state->getActualSheet().m_numCols=N;
	f << "fields=[";
	for (int i=0; i<N; ++i) f << (int) libwps::readU8(m_input) << ",";
	f << "],";
	for (int i=0; i<8; i++)
	{
		if (m_input->tell()>endPos) break;
		static int const expected[]= {0,0x1a,4,0xff, 0,0xa, 0, 1};
		val=(int) libwps::readU8(m_input);
		if (val!=expected[i]) f << "g" << i << "=" << val << ",";
	}
	ascii().addDelimiter(m_input->tell(), '|');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Spreadsheet::readReportClose()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::readU16(m_input);
	if (type != 0x5418)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readReportClose: not a report header\n"));
		return false;
	}
	f << "Report[end]:";
	WKS4SpreadsheetInternal::Spreadsheet const &sheet=m_state->getActualSheet();
	if (sheet.m_type!=WKS4SpreadsheetInternal::Spreadsheet::T_Report)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readReportClose: can not find report spreadsheet\n"));
		f << "###[noOpen],";
	}
	else
		m_state->popSheet();
	long sz = (long) libwps::readU16(m_input);
	if (sz)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::readReportClose: find some data\n"));
		f << "###[sz],";
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
// send data
////////////////////////////////////////////////////////////
void WKS4Spreadsheet::sendSpreadsheet(int sId)
{
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::sendSpreadsheet: I can not find the listener\n"));
		return;
	}
	shared_ptr<WKS4SpreadsheetInternal::Spreadsheet> sheet =
	    m_state->getSheet(WKS4SpreadsheetInternal::Spreadsheet::T_Spreadsheet, sId);
	if (!sheet)
	{
		if (sId==0)
		{
			WPS_DEBUG_MSG(("WKS4Spreadsheet::sendSpreadsheet: oops can not find the actual sheet\n"));
		}
		sheet.reset(new WKS4SpreadsheetInternal::Spreadsheet);
	}

	m_listener->openSheet(sheet->convertInPoint(sheet->m_widthCols,76), librevenge::RVNG_POINT,
	                      std::vector<int>(), m_state->getSheetName(sId));
	sheet->compressRowHeights();
	std::map<Vec2i, WKS4SpreadsheetInternal::Cell>::const_iterator it = sheet->m_positionToCellMap.begin();
	int prevRow = -1;
	while (it!=sheet->m_positionToCellMap.end())
	{
		int row=it->first[1];
		WKS4SpreadsheetInternal::Cell const &cell=(it++)->second;
		if (row>prevRow+1)
		{
			while (row > prevRow+1)
			{
				if (prevRow != -1) m_listener->closeSheetRow();
				int numRepeat;
				float h=sheet->getRowHeight(prevRow+1, numRepeat);
				if (row<prevRow+1+numRepeat)
					numRepeat=row-1-prevRow;
				m_listener->openSheetRow(h, librevenge::RVNG_POINT, false, numRepeat);
				prevRow+=numRepeat;
			}
		}
		if (row!=prevRow)
		{
			if (prevRow != -1) m_listener->closeSheetRow();
			m_listener->openSheetRow(sheet->getRowHeight(++prevRow), librevenge::RVNG_POINT);
		}
		sendCellContent(cell);
	}
	if (prevRow!=-1) m_listener->closeSheetRow();
	m_listener->closeSheet();
}

void WKS4Spreadsheet::sendCellContent(WKS4SpreadsheetInternal::Cell const &cell)
{
	if (m_listener.get() == 0L)
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::sendCellContent: I can not find the listener\n"));
		return;
	}

	WKS4SpreadsheetInternal::Style cellStyle(m_mainParser.getDefaultFontType());
	if (cell.m_styleId<0 || !m_state->m_styleManager.get(cell.m_styleId,cellStyle))
	{
		WPS_DEBUG_MSG(("WKS4Spreadsheet::sendCellContent: I can not find the cell style\n"));
	}
	if (version()<=2 && cell.m_hAlign!=WPSCellFormat::HALIGN_DEFAULT)
		cellStyle.setHAlignement(cell.m_hAlign);

	libwps_tools_win::Font::Type fontType = cellStyle.m_fontType;
	m_listener->setFont(cellStyle.m_font);

	WKS4SpreadsheetInternal::Cell finalCell(cell);
	finalCell.WPSCellFormat::operator=(cellStyle);
	finalCell.setFont(cellStyle.m_font);
	WKSContentListener::CellContent content(cell.m_content);
	bool hasLICS=hasLICSCharacters();
	for (size_t f=0; f < content.m_formula.size(); ++f)
	{
		if (content.m_formula[f].m_type!=WKSContentListener::FormulaInstruction::F_Text)
			continue;
		std::string &text=content.m_formula[f].m_content;
		librevenge::RVNGString finalString("");
		for (size_t c=0; c < text.length(); ++c)
		{
			if (!hasLICS)
				libwps::appendUnicode
				((uint32_t)libwps_tools_win::Font::unicode((unsigned char)text[c],fontType), finalString);
			else
				libwps::appendUnicode
				((uint32_t)libwps_tools_win::Font::LICSunicode((unsigned char)text[c],fontType), finalString);
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
					WPS_DEBUG_MSG(("WKS4Spreadsheet::sendCellContent: find 0xa without 0xd\n"));
				}
				prevEOL=false;
			}
			else
			{
				if (!hasLICS)
					m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::unicode(c,fontType));
				else
					m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::LICSunicode(c,fontType));
				prevEOL=false;
			}
		}
	}
	m_listener->closeSheetCell();
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
