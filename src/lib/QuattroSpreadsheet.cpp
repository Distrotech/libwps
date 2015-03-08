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

#include "Quattro.h"

#include "QuattroSpreadsheet.h"

namespace QuattroSpreadsheetInternal
{

///////////////////////////////////////////////////////////////////
//! a class used to store a style of a cell in QuattroSpreadsheet
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
			WPS_DEBUG_MSG(("QuattroParserInternal::StyleManager::get can not find style %d\n", id));
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
			WPS_DEBUG_MSG(("QuattroParserInternal::StyleManager::print: can not find a style\n"));
			o << ", ###style=" << id;
		}
	}

protected:
	//! the styles
	std::vector<Style> m_stylesList;
};

//! a cellule of a Quattro spreadsheet
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
		WPS_DEBUG_MSG(("QuattroSpreadsheetInternal::Cell::send: must not be called\n"));
		return false;
	}

	//! call when the content of a cell must be send
	bool sendContent(WPSListenerPtr &/*listener*/)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheetInternal::Cell::sendContent: must not be called\n"));
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
protected:
	//! a comparaison structure used to sort cell by rows and and columns
	struct ComparePosition
	{
		//! constructor
		ComparePosition() {}
		//! comparaison function
		bool operator()(Vec2i const &c1, Vec2i const &c2) const
		{
			if (c1[1]!=c2[1])
				return c1[1]<c2[1];
			return c1[0]<c2[0];
		}
	};

public:
	//! the spreadsheet type
	enum Type { T_Spreadsheet, T_Filter, T_Report };

	//! a constructor
	Spreadsheet(Type type=T_Spreadsheet, int id=0) : m_type(type), m_id(id), m_numCols(0), m_numRows(0), m_LBPosition(-1,-1),
		m_widthCols(), m_heightRows(), m_positionToCellMap(), m_lastCellPos(),
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
	/** compute the last Right Bottom cell position */
	void computeRightBottomPosition()
	{
		int maxX = -1, maxY = -1;
		for (std::map<Vec2i, Cell>::const_iterator it=m_positionToCellMap.begin(); it!=m_positionToCellMap.end(); ++it)
		{
			Vec2i const &p = it->second.position();
			if (p[0] > maxX) maxX = p[0];
			if (p[1] > maxY) maxY = p[1];
		}
		m_LBPosition=Vec2i(maxX, maxY);
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
	/** the final Right Bottom position, computed by updateState */
	Vec2i m_LBPosition;

	/** the column size in TWIP (?) */
	std::vector<int> m_widthCols;
	/** the row size in TWIP (?) */
	std::vector<int> m_heightRows;
	/** a map cell to not empty cells */
	std::map<Vec2i, Cell, ComparePosition> m_positionToCellMap;
	/** the last cell position */
	Vec2i m_lastCellPos;
	/** the list of row page break */
	std::vector<int> m_rowPageBreaksList;

};

//! the state of QuattroSpreadsheet
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
			WPS_DEBUG_MSG(("QuattroSpreadsheetInternal::State::pushSheet: can not find the sheet\n"));
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
			WPS_DEBUG_MSG(("QuattroSpreadsheetInternal::State::popSheet: can pop the main sheet\n"));
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
QuattroSpreadsheet::QuattroSpreadsheet(QuattroParser &parser) :
	m_input(parser.getInput()), m_listener(), m_mainParser(parser), m_state(new QuattroSpreadsheetInternal::State),
	m_asciiFile(parser.ascii())
{
	m_state.reset(new QuattroSpreadsheetInternal::State);
}

QuattroSpreadsheet::~QuattroSpreadsheet()
{
}

int QuattroSpreadsheet::version() const
{
	if (m_state->m_version<0)
		m_state->m_version=m_mainParser.version();
	return m_state->m_version;
}

bool QuattroSpreadsheet::hasLICSCharacters() const
{
	if (m_state->m_hasLICSCharacters<0)
		m_state->m_hasLICSCharacters=m_mainParser.hasLICSCharacters() ? 1 : 0;
	return m_state->m_hasLICSCharacters==1;
}

bool QuattroSpreadsheet::checkFilePosition(long pos)
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

int QuattroSpreadsheet::getNumSpreadsheets() const
{
	return m_state->getMaximalSheet(QuattroSpreadsheetInternal::Spreadsheet::T_Spreadsheet)+1;
}

////////////////////////////////////////////////////////////
// low level

////////////////////////////////////////////////////////////
//   parse sheet data
////////////////////////////////////////////////////////////
bool QuattroSpreadsheet::readSheetSize()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x6)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSheetSize: not a sheet zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz < 8)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSheetSize: block is too short\n"));
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

	m_state->getActualSheet().setRowHeight(nRow-1);
	m_state->getActualSheet().setColumnWidth(nCol-1);
	return true;

}

bool QuattroSpreadsheet::readColumnSize()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x8)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readColumnSize: not a column size zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz < 3)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readColumnSize: block is too short\n"));
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
				WPS_DEBUG_MSG(("QuattroSpreadsheet::readColumnSize: I must increase the number of columns\n"));
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

bool QuattroSpreadsheet::readHiddenColumns()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x64)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readHiddenColumns: not a column hidden zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz != 32)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readHiddenColumns: block size seems bad\n"));
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
			WPS_DEBUG_MSG(("QuattroSpreadsheet::readHiddenColumns: find some hidden col, ignored\n"));
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

bool QuattroSpreadsheet::readCellStyle()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	int const vers=version();
	if (type != 0xd8)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCellStyle: not a style zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	f << "Entries(CellStyle):";
	if ((vers==1&&(sz%12)!=0) || (vers>1 && sz!=0x16))
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCellStyle: size seems bad\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	if (vers>1)   // TODO
	{
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	int N=int(sz/12);
	for (int i=0; i<N; ++i)
	{
		pos=m_input->tell();
		f.str("");
		f << "CellStyle-" << i << ":";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		m_input->seek(pos+12, librevenge::RVNG_SEEK_SET);
	}
	return true;
}

bool QuattroSpreadsheet::readCellProperty()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);

	if (type != 0x9d)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCellProperty: not a cell property zone zone\n"));
		return false;
	}
	f << "Entries(CellProperty):";
	long sz = libwps::readU16(m_input);
	if (sz!=7)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCellProperty: the size seems bad\n"));
		f << "###sz";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	int val=(int) libwps::readU8(m_input);
	if (val!=0xFF) f << "f0=" << std::hex << val << std::dec << ",";
	int col=(int) libwps::read16(m_input);
	int row=(int) libwps::read16(m_input);
	if (col<0 || row<0)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCellProperty: the position seems bad\n"));
		f << "###";
	}
	f << "C" << Vec2i(col,row) << ",";
	val=(int) libwps::readU8(m_input);
	switch (val>>6)
	{
	case 1:
		f << "left,";
		break;
	case 2:
		f << "right,";
		break;
	case 3:
		f << "center,";
		break;
	default:
	case 0:
		break;
	}
	val &= 0x3F;
	if (val) f << "f0=" << std::hex << val << std::dec << ",";
	ascii().addDelimiter(m_input->tell(), '|');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool QuattroSpreadsheet::readUserStyle()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	int const vers=version();
	if (type != 0xc9)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readUserStyle: not a style zone\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	f << "Entries(UserStyle):";
	if ((vers==1&&sz!=0x2a) || (vers>1 && sz!=0x24))
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readUserStyle: size seems bad\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	if (vers>1)   // TODO
	{
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}

	int val=(int) libwps::readU16(m_input); // between 6e and 77 ?
	f << "id=" << val << ",";
	int flags = (int)libwps::readU16(m_input);

	WPSFont font;
	uint32_t attributes = 0;
	if (flags & 1) attributes |= WPS_BOLD_BIT;
	if (flags & 2) attributes |= WPS_ITALICS_BIT;
	if (flags & 8) attributes |= WPS_UNDERLINE_BIT;

	font.m_attributes=attributes;
	if (flags & 0xFFF4)
		f << "font[fl]=" << std::hex << (flags & 0xFFF4) << std::dec << ",";
	int fId=(int) libwps::readU16(m_input);
	f << "fId=" << fId << ",";
	int fSize = (int) libwps::readU16(m_input);
	if (fSize >= 1 && fSize <= 50)
		font.m_size=double(fSize);
	else
		f << "###fSize=" << fSize << ",";
	int color = (int) libwps::readU16(m_input);
	if (color && !m_mainParser.getColor(color, font.m_color))
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readUserStyle: unknown color\n"));
		f << "##font[color]=" << color << ",";
	}
	f << "font=[" << font << "],";
	val=(int) libwps::read16(m_input);
	if (val!=-1) f << "f1=" << val << ",";
	int sSz=(int) libwps::readU8(m_input);
	if (sSz>15)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readUserStyle: the name size seems bad\n"));
		f << "###sz[name]=" << sSz << ",";
	}
	else
	{
		std::string name("");
		for (int i=0; i<sSz; ++i) name += (char) libwps::readU8(m_input);
		f << name << ",";
	}
	m_input->seek(pos+32, librevenge::RVNG_SEEK_SET);
	val=(int) libwps::readU16(m_input); // probably list of modified flags
	if (val)
		f << "wh=" << std::hex << val << std::dec << ",";
	for (int i=0; i<4; ++i)
	{
		val=(int) libwps::readU8(m_input);
		if (!val) continue;
		static char const *(wh[])= {"T", "L", "B", "R"};
		f << "bord" << wh[i] << "=";
		switch (val)
		{
		case 1:
			f << "norm,";
			break;
		case 2:
			f << "double,";
			break;
		case 3:
			f << "thick,";
			break;
		default:
			f << "#" << val << ",";
			break;
		}
	}
	val=(int) libwps::readU8(m_input);
	switch (val)
	{
	case 0:
		break;
	case 1:
		f << "back[color]=grey,";
		break;
	case 2:
		f << "back[color]=black,";
		break;
	default:
		f << "#back[color]=" << val << ",";
		break;
	}
	val=(int) libwps::readU8(m_input);
	switch (val)
	{
	case 0:
		break;
	case 1:
		f << "left,";
		break;
	case 2:
		f << "right,";
		break;
	case 3:
		f << "center,";
		break;
	default:
		f << "#align=" << val << ",";
		break;
	}
	val=(int) libwps::readU8(m_input);
	switch (val)
	{
	case 0:
		break;
	case 1:
		f << "input=labels[only],";
		break;
	case 2:
		f << "input=dates[only],";
		break;
	default:
		f << "#input=" << val << ",";
		break;
	}
	val=(int) libwps::readU8(m_input);
	if (val) f << "format=" << std::hex << val << std::dec << ",";
	int dim[2];
	for (int i=0; i<2; ++i) dim[i]=(int) libwps::read16(m_input);
	if (dim[0]||dim[1]) f << "dim=" << Vec2i(dim[0],dim[1]) << ",";
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
// general
////////////////////////////////////////////////////////////

bool QuattroSpreadsheet::readCell()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if ((type != 0x545b) && (type < 0xc || type > 0x10))
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: not a cell property\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	long endPos = pos+4+sz;

	if (sz < 5)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: cell def is too short\n"));
		return false;
	}
	int const vers=version();
	bool dosFile = vers<=1;
	int unkn1 = 0;
	if (dosFile)
		unkn1 = (int) libwps::readU8(m_input);
	int cellPos[2];
	cellPos[0]=(int) libwps::readU8(m_input);
	int sheetId=(int) libwps::readU8(m_input);
	cellPos[1]=(int) libwps::read16(m_input);
	if (cellPos[1] < 0)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: cell pos is bad\n"));
		return false;
	}
	if (sheetId)
	{
		if (vers==1)
		{
			WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: find unexpected sheet id\n"));
			f << "###";
		}
		f << "sheet[id]=" << sheetId << ",";
	}

	QuattroSpreadsheetInternal::Cell &cell=m_state->getActualSheet().getCell(Vec2i(cellPos[0],cellPos[1]));
	if (!dosFile)
	{
		for (int i=0; i<2; ++i)   // related to style and format?
		{
			int val=(int) libwps::readU8(m_input);
			if (val) f << "f" << i << "=" << val << ",";
		}
	}

	if (type & 0xFF00)
	{
		if (dosFile)
		{
			static bool first = true;
			if (first)
			{
				first = false;
				WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: find type=%ld in dos version\n", type));
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
		// pascal string
		char align=(char) libwps::readU8(m_input);
		if (align=='\'') cell.m_hAlign=WPSCellFormat::HALIGN_DEFAULT;
		else if (align=='\\') cell.m_hAlign=WPSCellFormat::HALIGN_LEFT;
		else if (align=='^') cell.m_hAlign=WPSCellFormat::HALIGN_CENTER;
		else if (align=='\"') cell.m_hAlign=WPSCellFormat::HALIGN_RIGHT;
		else f << "#align=" << (int) align << ",";

		int sSz=(int) libwps::readU8(m_input);
		if (2+sSz>dataSz)
		{
			WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: the string's size seems bad\n"));
			f << "###sz=" << sSz << ",";
			break;
		}
		else if (2+sSz!=dataSz)
		{
			f << "#sz=" << sSz << ",";
			endText=begText+2+sSz;
		}
		begText=m_input->tell();
		for (int i = 0; i < sSz; i++)
		{
			char c = (char) libwps::read8(m_input);
			if (c=='\0')
			{
				WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: find 0 in data\n"));
				f << "###[0]";
				break;
			}
			s += c;
		}
		f << s << ",";
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
			if (!readFormula(endPos, cell.position(), sheetId, cell.m_content.m_formula, error))
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
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: unknown type=%ld\n", type));
		ok = false;
		break;
	}
	if (!ok) ascii().addDelimiter(dataPos, '#');

	if (dosFile)
	{
		QuattroSpreadsheetInternal::Style style(m_mainParser.getDefaultFontType());
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

		int styleID = (unkn1>>4) & 0x7;
		switch (styleID)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		{
			if (styleID == 2) styleID = 3;
			else if (styleID == 3) styleID = 2;
			style.setFormat(WPSCell::F_NUMBER,1+styleID);
			break;
		}
		case 0x7:
			break;
		default:
			f << "type=" << styleID << ",";
		}
		if ((unkn1&0x80)==0) f << "style80=0,";
		style.setDigits(unkn1 & 0xF);
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

bool QuattroSpreadsheet::readCellFormulaResult()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = libwps::read16(m_input);
	if (type != 0x33)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCellFormulaResult: not a cell property\n"));
		return false;
	}
	long sz = libwps::readU16(m_input);
	if (sz<6)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readCellFormulaResult: the zone seems to short\n"));
		return false;
	}
	long endPos = pos+4+sz;

	bool dosFile = version() <= 1;
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
bool QuattroSpreadsheet::readSpreadsheetOpen()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::readU16(m_input);
	if (type != 0xdc)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSpreadsheetOpen: not a spreadsheet header\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	f << "Entries(Spreadsheet)[beg]:";
	if (sz!=2)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSpreadsheetOpen: the field size seems odd\n"));
		f << "###";
	}
	int id=(int) libwps::readU16(m_input);
	if (id<0||id>255)
	{
		f << "###";
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSpreadsheetOpen: the id seems odd\n"));
	}
	else if (id==0 && m_state->m_spreadsheetStack.size()!=1)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSpreadsheetOpen: find 0 but the stack is not empty\n"));
		f << "###stack";
		m_state->m_spreadsheetStack.push(m_state->m_spreadsheetList[0]);
	}
	else if (id)
		m_state->pushNewSheet(shared_ptr<QuattroSpreadsheetInternal::Spreadsheet>
		                      (new QuattroSpreadsheetInternal::Spreadsheet
		                       (QuattroSpreadsheetInternal::Spreadsheet::T_Spreadsheet, id)));
	f << id << ",";
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool QuattroSpreadsheet::readSpreadsheetClose()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	long type = (long) libwps::readU16(m_input);
	if (type != 0xdd)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSpreadsheetClose: not a spreadsheet header\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	f << "Entries(Spreadsheet)[end]:";
	QuattroSpreadsheetInternal::Spreadsheet const &sheet=m_state->getActualSheet();
	if (sheet.m_type!=QuattroSpreadsheetInternal::Spreadsheet::T_Spreadsheet)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSpreadsheetClose: can not find spreadsheet spreadsheet\n"));
		f << "###[noOpen],";
	}
	else if (m_state->m_spreadsheetStack.size() > 1)
		m_state->popSheet();
	if (sz!=0)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readSpreadsheetClose: the field size seems odd\n"));
		f << "###";
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool QuattroSpreadsheet::readCell
(Vec2i actPos, WKSContentListener::FormulaInstruction &instr, bool hasSheetId, int sheetId)
{
	instr=WKSContentListener::FormulaInstruction();
	instr.m_type=WKSContentListener::FormulaInstruction::F_Cell;
	bool ok = true;
	int pos[3];
	bool absolute[3] = { true, true, true};
	for (int dim = 0; dim < (hasSheetId ? 3 : 2); dim++)
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
			if (dim==2)
				val += sheetId;
			else
				val += actPos[dim];
			absolute[dim] = false;
		}
		else if (val==0xFFFF)
		{
			static bool first=true;
			if (first)   // in general associated with a nan value, so maybe be normal
			{
				WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: find some ffff cell\n"));
				first=false;
			}
			ok = false;
		}
		else
		{
			WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: can not read cell %x\n", (unsigned int)val));
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
			WPS_DEBUG_MSG(("QuattroSpreadsheet::readCell: can not read cell position\n"));
		}
		return false;
	}
	instr.m_position[0]=Vec2i(pos[0],pos[1]);
	instr.m_positionRelative[0]=Vec2b(!absolute[0],!absolute[1]);
	if (hasSheetId && pos[2]!=sheetId)
		instr.m_sheetName=m_state->getSheetName(pos[2]);
	return ok;
}

namespace QuattroSpreadsheetInternal
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

bool QuattroSpreadsheet::readFormula(long endPos, Vec2i const &position, int sheetId,
                                     std::vector<WKSContentListener::FormulaInstruction> &formula, std::string &error)
{
	int const vers=version();
	formula.resize(0);
	error = "";
	long pos = m_input->tell();
	if (endPos - pos < (vers==1 ? 6 : 4)) return false;
	int sz = (int) libwps::readU16(m_input);
	if (endPos-pos-2 != sz) return false;

	std::vector<WKSContentListener::FormulaInstruction> listCellsPos;
	size_t actCellId=0;
	int fieldPos[2]= {0,sz};
	for (int i=0; i<2; ++i)
	{
		fieldPos[i]= (int) libwps::readU16(m_input);
		if (vers>1) break;
	}
	if (fieldPos[0]<0||fieldPos[0]>fieldPos[1]||fieldPos[1]>sz)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readFormula: can not find the field header\n"));
		error="###fieldPos";
		return false;
	}
	if (fieldPos[0]!=sz)
	{
		m_input->seek(pos+2+fieldPos[0], librevenge::RVNG_SEEK_SET);
		ascii().addDelimiter(pos+2+fieldPos[0],'|');
		if (vers==1)
		{
			int N=(int)(sz-fieldPos[0])/4;
			for (int i=0; i<N; ++i)
			{
				WKSContentListener::FormulaInstruction cell;
				if (!readCell(position, cell))
				{
					ascii().addDelimiter(pos+2+fieldPos[0]+i*4,'@');
					WPS_DEBUG_MSG(("QuattroSpreadsheet::readFormula: can not read some cell\n"));
					error="###cell,";
					break;
				}
				listCellsPos.push_back(cell);
			}
		}
		else
		{
			while (!m_input->isEnd())
			{
				long actPos=m_input->tell();
				if (actPos+8>endPos) break;
				int type=(int) libwps::readU16(m_input);
				WKSContentListener::FormulaInstruction cell, cell2;
				if (type==0)
				{
					if (!readCell(position, cell, true, sheetId))
					{
						m_input->seek(actPos, librevenge::RVNG_SEEK_SET);
						break;
					}
					listCellsPos.push_back(cell);
					continue;
				}
				if (type!=0x1000 || actPos+14>endPos || !readCell(position, cell, true, sheetId) ||
				        !readCell(position, cell2, true, sheetId))
				{
					m_input->seek(actPos, librevenge::RVNG_SEEK_SET);
					break;
				}
				cell.m_type=WKSContentListener::FormulaInstruction::F_CellList;
				cell.m_position[1]=cell2.m_position[0];
				cell.m_positionRelative[1]=cell2.m_positionRelative[0];
				listCellsPos.push_back(cell);
			}
			if (m_input->tell() !=endPos)
			{
				ascii().addDelimiter(m_input->tell(),'@');
				WPS_DEBUG_MSG(("QuattroSpreadsheet::readFormula: can not read some cell\n"));
				error="###cell,";
			}
		}
		m_input->seek(pos+4+(vers==1 ? 2 : 0), librevenge::RVNG_SEEK_SET);
		endPos=pos+2+fieldPos[0];
	}
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
			if (actCellId>=listCellsPos.size())
			{
				f.str("");
				f << "###unknCell" << actCellId;
				error=f.str();
				ok = false;
				break;
			}
			instr=listCellsPos[actCellId++];
			break;
		case 0x2:
			if (vers>=2)
			{
				if (actCellId>=listCellsPos.size())
				{
					f.str("");
					f << "###unknListCell" << actCellId;
					error=f.str();
					ok = false;
					break;
				}
				instr=listCellsPos[actCellId++];
				break;
			}
			if (actCellId+1>=listCellsPos.size())
			{
				f.str("");
				f << "###unknListCell" << actCellId;
				error=f.str();
				ok = false;
				break;
			}
			instr=listCellsPos[actCellId++];
			instr.m_type=WKSContentListener::FormulaInstruction::F_CellList;
			instr.m_position[1]=listCellsPos[actCellId].m_position[0];
			instr.m_positionRelative[1]=listCellsPos[actCellId].m_positionRelative[0];
			++actCellId;
			break;
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
			if (wh >= 0x90 || QuattroSpreadsheetInternal::s_listFunctions[wh].m_arity == -2)
			{
				f.str("");
				f << "##Funct" << std::hex << wh;
				error=f.str();
				ok = false;
				break;
			}
			instr.m_type=WKSContentListener::FormulaInstruction::F_Function;
			instr.m_content=QuattroSpreadsheetInternal::s_listFunctions[wh].m_name;
			ok=!instr.m_content.empty();
			arity = QuattroSpreadsheetInternal::s_listFunctions[wh].m_arity;
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
			WPS_DEBUG_MSG(("QuattroSpreadsheet::readFormula: find some extra data\n"));
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
		WPS_DEBUG_MSG(("QuattroSpreadsheet::readFormula: I can not read some formula\n"));
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
// send data
////////////////////////////////////////////////////////////
void QuattroSpreadsheet::sendSpreadsheet(int sId)
{
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::sendSpreadsheet: I can not find the listener\n"));
		return;
	}
	shared_ptr<QuattroSpreadsheetInternal::Spreadsheet> sheet =
	    m_state->getSheet(QuattroSpreadsheetInternal::Spreadsheet::T_Spreadsheet, sId);
	if (!sheet)
	{
		if (sId==0)
		{
			WPS_DEBUG_MSG(("QuattroSpreadsheet::sendSpreadsheet: oops can not find the actual sheet\n"));
		}
		sheet.reset(new QuattroSpreadsheetInternal::Spreadsheet);
	}
	sheet->computeRightBottomPosition();

	m_listener->openSheet(sheet->convertInPoint(sheet->m_widthCols,76), librevenge::RVNG_POINT,
	                      m_state->getSheetName(sId));
	std::vector<float> rowHeight = sheet->convertInPoint(sheet->m_heightRows,16);
	for (int r=0; r<=sheet->m_LBPosition[1]; ++r)
	{
		m_listener->openSheetRow(r < int(rowHeight.size()) ? rowHeight[size_t(r)] : 16, librevenge::RVNG_POINT);
		for (int c=0; c<=sheet->m_LBPosition[0]; ++c)
		{
			if (sheet->m_positionToCellMap.find(Vec2i(c,r))==sheet->m_positionToCellMap.end())
				continue;
			QuattroSpreadsheetInternal::Cell const &cell= sheet->m_positionToCellMap.find(Vec2i(c,r))->second;
			sendCellContent(cell);
		}
		m_listener->closeSheetRow();
	}
	m_listener->closeSheet();
}

void QuattroSpreadsheet::sendCellContent(QuattroSpreadsheetInternal::Cell const &cell)
{
	if (m_listener.get() == 0L)
	{
		WPS_DEBUG_MSG(("QuattroSpreadsheet::sendCellContent: I can not find the listener\n"));
		return;
	}

	QuattroSpreadsheetInternal::Style cellStyle(m_mainParser.getDefaultFontType());
	if (cell.m_styleId<0 || !m_state->m_styleManager.get(cell.m_styleId,cellStyle))
	{
		static bool first=true;
		if (first)   // do not kow how to retrieve the style in Wq2 file, so normal:-~
		{
			first=false;
			WPS_DEBUG_MSG(("QuattroSpreadsheet::sendCellContent: I can not find some cell styles\n"));
		}
	}
	if (version()<=2 && cell.m_hAlign!=WPSCellFormat::HALIGN_DEFAULT)
		cellStyle.setHAlignement(cell.m_hAlign);

	librevenge::RVNGPropertyList propList;
	libwps_tools_win::Font::Type fontType = cellStyle.m_fontType;
	m_listener->setFont(cellStyle.m_font);

	QuattroSpreadsheetInternal::Cell finalCell(cell);
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
				WPSListener::appendUnicode
				((uint32_t)libwps_tools_win::Font::unicode((unsigned char)text[c],fontType), finalString);
			else
				WPSListener::appendUnicode
				((uint32_t)libwps_tools_win::Font::LICSunicode((unsigned char)text[c],fontType), finalString);
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
					WPS_DEBUG_MSG(("QuattroSpreadsheet::sendCellContent: find 0xa without 0xd\n"));
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
