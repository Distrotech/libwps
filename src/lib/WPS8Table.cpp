/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * For further information visit http://libwps.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include <iomanip>
#include <iostream>

#include <libwpd/WPXBinaryData.h>
#include <libwpd/WPXString.h>

#include "WPSContentListener.h"
#include "WPSEntry.h"
#include "WPSPosition.h"

#include "WPS8.h"
#include "WPS8Struct.h"

#include "WPS8Table.h"


/** Internal: the structures of a WPS8Table */
namespace WPS8TableInternal
{
/** Internal: class to store a border: unknown structure which contains 3 int */
struct Border
{
	//! constructor
	Border() : m_unknown1(-9999997-4) /* -infinity-4 */, m_unknown2(-1), m_unknown3(-1) { }
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, Border const &border);
	int m_unknown1 /** first unknown value*/, m_unknown2 /** second unknown value*/, m_unknown3 /** third unknown value*/;
};
//! operator<< for a Border
std::ostream &operator<<(std::ostream &o, Border const &bord)
{
	switch (bord.m_unknown1)
	{
	case -9999997-4:
		o << "_:";
		break;
	case -9999997:
		o << "-infty:";
		break;
	default:
		o << bord.m_unknown1 << ":";
	}
	if (bord.m_unknown2 == -1) o << "_:";
	else
	{
		o << (int) (bord.m_unknown2&0xff);
		int high = ((bord.m_unknown2>>8)&0xff);
		if (high) o<< std::hex << "(" << high << std::dec << ")";
		o << ":";
	}
	if (bord.m_unknown3 == -1) o << "_";
	else o << bord.m_unknown3;
	return o;
}

/** Internal: class to store a basic cell with borders */
struct Cell
{
	//! constructor
	Cell() : m_bdbox(), m_size()
	{
		m_colors[0] = 0;
		m_colors[1] = 0xFFFFFF;
		for (int i = 0; i < 4; i++) m_bordersWidth[i] = 0.0;
	}
	//! enum to indicate a border position
	enum RelativePosition { RIGHT, LEFT, TOP, BOTTOM, NODEF };

	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, Cell const &cell);

	//! checks if two cells are on the same row
	bool sameRow(Cell const &c) const
	{
		return (m_bdbox.min().y() == c.m_bdbox.min().y() &&
		        m_bdbox.max().y() == c.m_bdbox.max().y());
	}
	//! checks if two cells are on the same columns
	bool sameColumn(Cell const &c) const
	{
		return (m_bdbox.min().x() == c.m_bdbox.min().x() &&
		        m_bdbox.max().x() == c.m_bdbox.max().x());
	}
	//! checks if two cells rows overlaps
	bool rowOverlap(Cell const &c) const
	{
		if (m_bdbox.min().y() >= c.m_bdbox.min().y() && m_bdbox.min().y() < c.m_bdbox.max().y()) return true;
		if (m_bdbox.max().y() > c.m_bdbox.min().y() && m_bdbox.max().y() <= c.m_bdbox.max().y()) return true;
		if (c.m_bdbox.min().y() >= m_bdbox.min().y() && c.m_bdbox.max().y() <= m_bdbox.max().y()) return true;
		return false;
	}
	//! tries to find if a cell is continuous to the previous one
	RelativePosition position(Cell const &c, int distAllow=2) const
	{
		if (sameRow(c))
		{
			long diff = c.m_bdbox.min().x()-m_bdbox.max().x();
			if (0 <= diff && diff <= distAllow) return RIGHT;
			diff = m_bdbox.min().x()-c.m_bdbox.max().x();
			if (0 <= diff && diff <= distAllow) return LEFT;
			return NODEF;
		}
		if (sameColumn(c))
		{
			long diff = c.m_bdbox.min().y()-m_bdbox.max().y();
			if (0 <= diff && diff <= distAllow) return BOTTOM;
			diff = m_bdbox.min().y()-c.m_bdbox.max().y();
			if (0 <= diff && diff <= distAllow) return TOP;
			return NODEF;
		}
		return NODEF;
	}
	//! local position in point
	Box2i m_bdbox;
	//! frame size in inches
	Vec2f m_size;
	//! border size \warning checkme unit=Inches?
	float m_bordersWidth[4];
	//! border definition
	Border m_borders[4];
	//! the colors: border color followed by background color
	uint32_t m_colors[2];
};

//! operator<< for a Cell
std::ostream &operator<<(std::ostream &o, Cell const &cell)
{
	o << "dimOrig=[";
	if (cell.m_bdbox.size().x() > 0 || cell.m_bdbox.size().y() > 0) o << cell.m_bdbox;
	else o << "_";
	o << "],";
	if (cell.m_size.x() > 0 || cell.m_size.y() > 0) o << "size=" << cell.m_size << ",";
	bool hasBSize = false;
	for (int i = 0; i < 4; i++)
	{
		if (cell.m_bordersWidth[i] <= 0)
			continue;
		hasBSize = true;
		break;
	}
	if (hasBSize)
	{
		o << "borderSize=[";
		for (int i = 0; i < 4; i++)
			if (cell.m_bordersWidth[i] > 0 ) o << cell.m_bordersWidth[i] << ",";
			else o << "_,";
		o << "],";
	}

	if (cell.m_colors[0] != 0)
		o << "bordCol=" << std::hex << cell.m_colors[0] << std::hex << ",";
	if (cell.m_colors[1] != 0)
		o << "backCol=" << std::hex << cell.m_colors[1] << std::hex << ",";
	o << "bord=(";
	for (int i = 0; i < 4; i++) o << cell.m_borders[i] << ",";
	o << ")";
	return o;
}

/** Internal: class to store a table: list of cells */
struct Table
{
	//! constructor
	Table() : m_id(-1), m_cells(), m_rowsListCells(), m_parsed(false) {}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, Table const &table);
	//! tries to group the cells by row. \warning: not finished
	bool findRow();
	//! the table id
	int m_id;
	//! the list of cells
	std::vector<Cell> m_cells;
	//! number of cells by row
	std::vector<std::vector<int> > m_rowsListCells;

	//! a bool to know if the table has been parsed
	mutable bool m_parsed;
};
//! operator<< for a Table
std::ostream &operator<<(std::ostream &o, Table const &table)
{
	o << "id=" << table.m_id << "\n";
	size_t numRows = table.m_rowsListCells.size();
	if (numRows)
	{
		for (size_t i = 0; i < numRows; i++)
		{
			o << "\trow" << int(i) <<":\n";
			std::vector<int> const &cellLists = table.m_rowsListCells[i];
			for (size_t c = 0; c < cellLists.size(); c++)
				o << "\t\tcell" << c << ":" << table.m_cells[size_t(cellLists[c])] << "\n";
		}
		return o;
	}

	for (size_t c = 0; c < table.m_cells.size(); c++)
		o << "\tcell" << c << ":" << table.m_cells[c] << "\n";
	return o;
}

bool Table::findRow()
{
	size_t numCells = m_cells.size();
	m_rowsListCells.resize(0);
	if (numCells <= 1)
	{
		std::vector<int> r;
		r.push_back(0);
		m_rowsListCells.push_back(r);
		return true;
	}

	for (size_t i  = 0; i+1 < numCells; i++)
		if (m_cells[i].m_size.x() <= 0 || m_cells[i].m_size.y() <= 0)
		{
			static bool first = true;
			if (first)
			{
				first = false;
				WPS_DEBUG_MSG(("WPS8TableInternal::Table::findRow: some cell size are not defined\n"));
			}
			return false;
		}

	int lastRow = -1;
	for (size_t i = 0; i < numCells; i++)
	{
		if (i && m_cells[i].sameRow(m_cells[i-1]))
		{
			m_rowsListCells[size_t(lastRow)].push_back(int(i));
			continue;
		}

		size_t numRows = m_rowsListCells.size();
		size_t r;
		for (r = 0; r < numRows; r++)
		{
			if (m_cells[size_t(m_rowsListCells[r][0])].sameRow(m_cells[i])) break;
			if (m_cells[size_t(m_rowsListCells[r][0])].rowOverlap(m_cells[i]))
			{
				m_rowsListCells.resize(0);
				WPS_DEBUG_MSG(("WPS8TableInternal::Table::findRow: some rows overlap\n"));
				return false;
			}
		}
		if (r == numRows)
			m_rowsListCells.push_back(std::vector<int>());
		m_rowsListCells[r].push_back(int(i));
		lastRow = int(r);
	}
	// FIXME: we must now sorted each cells which are in the same rows
	return true;
}

/** Internal: the state of a WPS8Table */
struct State
{
	//! constructor
	State() : m_version(-1), m_numPages(0), m_tableMap(), m_MCLDTypes()
	{
		initTypeMaps();
	}

	//! initialize the type map
	void initTypeMaps();
	//! the version
	int m_version;
	//! the number page
	int m_numPages;
	//! a map id -> table
	std::map<int, Table> m_tableMap;
	//! the MCLD type
	std::map<int,int> m_MCLDTypes;
};

void State::initTypeMaps()
{
	static int const MCLDTypes[] =
	{
		0, 0x22, 1, 0x22, 2, 0x22, 3, 0x22, 4, 0x22, 5, 0x22, 6, 0x22, 7, 0x22,
		8, 0x22, 9, 0x22, 0xa, 0x22, 0xb, 0x1a, 0xc, 0x2, 0xd, 0x22, 0xe, 0x22,
		0x11, 0x22, 0x12, 0x22, 0x13, 0x12, 0x14, 0x2, 0x15, 0x22, 0x16, 0x22, 0x17, 0x22,
		0x18, 0x22, 0x19, 0x2, 0x1a, 0x2, 0x1d, 0x22, 0x1e, 0x22, 0x1f, 0x12,
		0x20, 0x22, 0x21, 0x12, 0x22, 0x22, 0x23, 0x22, 0x24, 0x12, 0x25, 0x22, 0x26, 0x22, 0x27, 0x12,
		0x28, 0x22, 0x29, 0x22, 0x2a, 0x12, 0x2b, 0x22, 0x2c, 0x12,
		0x31, 0x18
	};
	for (int i = 0; i+1 < int(sizeof(MCLDTypes)/sizeof(int)); i+=2)
		m_MCLDTypes[MCLDTypes[i]] = MCLDTypes[i+1];

}
}

////////////////////////////////////////////////////////////
// constructor/destructor
////////////////////////////////////////////////////////////
WPS8Table::WPS8Table(WPS8Parser &parser) :
	m_listener(), m_mainParser(parser), m_state(), m_asciiFile(parser.ascii())
{
	m_state.reset(new WPS8TableInternal::State);
}

WPS8Table::~WPS8Table()
{
}

////////////////////////////////////////////////////////////
// update the positions and send data to the listener
////////////////////////////////////////////////////////////
int WPS8Table::version() const
{
	if (m_state->m_version <= 0)
		m_state->m_version = m_mainParser.version();
	return m_state->m_version;
}

int WPS8Table::numPages() const
{
	return m_state->m_numPages;
}

////////////////////////////////////////////////////////////
// update the positions and send data to the listener
////////////////////////////////////////////////////////////
void WPS8Table::computePositions() const
{
	m_state->m_numPages = 0;
}

void WPS8Table::flushExtra(WPXInputStreamPtr )
{
	if (m_listener.get() == 0L) return;
	typedef WPS8TableInternal::Table Table;

	std::map<int, Table>::iterator pos = m_state->m_tableMap.begin();
	bool first = true;
	while (pos != m_state->m_tableMap.end())
	{
		Table &tab = pos->second;
		pos++;
		if (tab.m_parsed) continue;
		if (!first) continue;
		first = false;
		WPS_DEBUG_MSG(("WPS8Table::flushExtra: some table are not parsed\n"));
	}
}

////////////////////////////////////////////////////////////
// send a table id
////////////////////////////////////////////////////////////
bool WPS8Table::sendTable(Vec2f const &siz, int tableId, int strsid)
{
	typedef WPS8TableInternal::Table Table;

	std::map<int, Table>::iterator pos = m_state->m_tableMap.find(tableId);
	if (pos == m_state->m_tableMap.end())
		WPS_DEBUG_MSG(("WPS8Table::flushExtra: can not find table with id=%d\n", tableId));
	else
	{
		Table const &table = pos->second;
		if (table.m_parsed)
			WPS_DEBUG_MSG(("WPS8Table::flushExtra: table with id=%d is already parsed\n", tableId));
		else
			table.m_parsed = true;
	}

	WPSPosition tablePos(Vec2f(), siz);
	tablePos.m_anchorTo = WPSPosition::CharBaseLine; // CHECKME
	tablePos.m_wrapping = WPSPosition::WDynamic;

	m_mainParser.sendTextBox(tablePos, strsid);
	return true;
}

////////////////////////////////////////////////////////////
// find all structures which correspond to the table
////////////////////////////////////////////////////////////
bool WPS8Table::readStructures(WPXInputStreamPtr input)
{
	m_state->m_tableMap.clear();

	WPS8Parser::NameMultiMap &nameTable = m_mainParser.getNameEntryMap();
	WPS8Parser::NameMultiMap::iterator pos;
	pos = nameTable.lower_bound("MCLD");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("MCLD")) break;
		if (!entry.hasType("MCLD")) continue;

		readMCLD(input, entry);
	}

	return true;
}

////////////////////////////////////////////////////////////
// low level
////////////////////////////////////////////////////////////
bool WPS8Table::readMCLD(WPXInputStreamPtr input, WPSEntry const &entry)
{
	typedef WPS8TableInternal::Border Border;
	typedef WPS8TableInternal::Cell Cell;
	typedef WPS8TableInternal::Table Table;

	if (!entry.hasType(entry.name()))
	{
		WPS_DEBUG_MSG(("WPS8Table::readMCLD: warning: MCLD name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}

	long page_offset = entry.begin();
	long length = entry.length();
	long endPage = entry.end();

	if (length < 24)
	{
		WPS_DEBUG_MSG(("WPS8Table::readMCLD: warning: MCLD length=0x%lx\n", length));

		return false;
	}

	entry.setParsed();
	input->seek(page_offset, WPX_SEEK_SET);

	libwps::DebugStream f;
	int mZone = (int) libwps::read32(input);
	int nTables = (int) libwps::read32(input);

	f << "maxUnknown=" << mZone << ", nTables= " << nTables;
	if ((6+nTables) * 4 > length) return false;

	f << ", ids=(";
	std::vector<int> listIds;
	for (int i = 0; i < nTables; i++)
	{
		int val = (int) libwps::read32(input);
		listIds.push_back(val);
		f << val << ",";
	}
	f << ")";

	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());

	int mCounter = -1;
	bool ok = true;

	long lastPosOk = input->tell();
	while (input->tell() != endPage)
	{
		if (input->tell()+16 > endPage)
		{
			ok = false;
			break;
		}

		mCounter++;
		lastPosOk = input->tell();

		WPS8Struct::FileData tableData;
		int sz = (int) libwps::read16(input);
		if (sz < 2 || lastPosOk+sz > endPage)
		{
			ok = false;
			break;
		}

		std::string error;
		if (!readBlockData(input, lastPosOk+sz,tableData, error))
		{
			f.str("");
			f << tableData;
			error = f.str();
		}

		f.str("");
		int tableId = (mCounter < int(listIds.size())) ? listIds[size_t(mCounter)] : -1;
		if (tableId < 0) f << "MCLD/Table[###unknownId]:";
		else f << "MCLD/Table" << listIds[size_t(mCounter)]<<":";

		int N = (int) libwps::readU32(input);
		f << " nCells=" << N;
		if (N < 0 || N > 100)
		{
			ok = false;
			break;
		}

		Table table;
		table.m_id = mCounter;

		size_t numChild = tableData.m_recursData.size();
		if (numChild)
		{
			f << ",(";
			for (size_t c = 0; c < numChild; c++)
			{
				WPS8Struct::FileData const &dt = tableData.m_recursData[c];
				if (dt.isBad()) continue;
				int const expectedTypes[] = { 2, 0x22 };
				if (dt.id() < 0 || dt.id() > 1)
				{
					f << "##" << dt << ",";
					continue;
				}
				if (expectedTypes[dt.id()]!=dt.type())
				{
					WPS_DEBUG_MSG(("WPS8Table::readMCLD: unexpected type for %d=%d\n", dt.id(), dt.type()));
					f << "###" << dt << ",";
					continue;
				}
				bool done = true;
				switch (dt.id())
				{
				case 0: // always set
					f << "f" << dt.id() << ", ";
					break;
				case 1: // always 0
					f << "f" << dt.id() << "=" << dt.m_value << ", ";
					break;
				default:
					done = false;
					break;
				}

				if (!done) f << "###" << dt << ",";
			}
			f << ")";
		}
		if (!error.empty()) f << ", ###err=" << error;

		ascii().addPos(lastPosOk);
		ascii().addNote(f.str().c_str());

		table.m_cells.resize(size_t(N));
		for (size_t actualCell = 0; actualCell < size_t(N); actualCell++)
		{
			lastPosOk = input->tell();
			sz = (int) libwps::read16(input);
			if (sz < 2 || lastPosOk+sz > endPage)
			{
				ok = false;
				break;
			}

			WPS8Struct::FileData mainData;
			error = "";

			if (!readBlockData(input, lastPosOk+sz,mainData, error))
			{
				f.str("");
				f << mainData;
				error = f.str();
			}

			size_t nChild = mainData.m_recursData.size();

			// original position in point
			int dim[4] = {  0, 0, 0, 0 };

			f.str("");
			f << "MCLD/Table";
			if (tableId >= 0) f << tableId;
			f << "(Cell" << actualCell <<"):";

			Cell cell;
			libwps::DebugStream f2;

			for (size_t c = 0; c < nChild; c++)
			{
				WPS8Struct::FileData const &dt = mainData.m_recursData[c];
				if (dt.isBad()) continue;
				if (m_state->m_MCLDTypes.find(dt.id())==m_state->m_MCLDTypes.end())
				{
					f << "##" << dt << ",";
					continue;
				}
				if (m_state->m_MCLDTypes.find(dt.id())->second != dt.type())
				{
					WPS_DEBUG_MSG(("WPS8Table::readMCLD: unexpected type for %d=%d\n", dt.id(), dt.type()));
					f << "###" << dt << ",";
					continue;
				}

				bool done = true;
				switch (dt.id())
				{
				case 0:
				case 1:
				case 2:
				case 3: // min/max pos in point (96 dpi ?)
					dim[dt.id()+1-2*(dt.id()%2)] = int(dt.m_value);
					break;
				case 4:
				case 5: // look coherent with EOBJ info
					if (dt.id() == 4) cell.m_size.setX(float(dt.m_value)/914400.f);
					else cell.m_size.setY(float(dt.m_value)/914400.f);
					break;
					// border size : unknown dim
					// [ 1, 3, 1, 3, ]
					// [ 0.666667, 1.33333, 0.666667, 1.33333, ]/30
				case 6:
				case 7:
				case 8:
				case 9:
					cell.m_bordersWidth[dt.id()-6] = float(dt.m_value)/914400.f;
					break;
				case 0xe: // always 0x20000000
					if (dt.m_value != 0x20000000)
						f2 << "f" << dt.id() << "=" << std::hex << dt.m_value << std::dec << ",";
					break;
				case 0x12: //  0x0 or 0x997aa0 or 0x3fffe238 ?
					f2 << "f" << dt.id() << "=" << std::hex << dt.m_value << std::dec << ",";
					break;
				case 0x1f: // 0|1|6|7|8 - 0||ff
				case 0x13:   //0xff, 0x7aff or 0xe2ff
				{
					f2 << "f" << dt.id() << "=" <<  (int) (int8_t) (dt.m_value & 0xFF);
					int high =  (uint8_t) ((dt.m_value>>8) & 0xFF);
					if (high) f2 << std::hex << "(" << high << ")" << std::dec;
					if (dt.m_value & 0xFFFF0000)
						f2 << "(###extras=" << (int) (dt.m_value>>16) << ")";
					f2 << ",";
					break;
				}
				case 0x1d: // first color
				case 0x1e: // second color
					cell.m_colors[dt.id()-0x1d] = uint32_t(dt.m_value);
					if (dt.m_value & 0xFF000000)
						f2 << "###id" << dt.id() << "=" << (int) (uint8_t) ((dt.m_value>>25) & 0xFF) << ",";
					break;
					// 0x20->2b quatre paquet de 3 valeurs ?
				case 0x20:
				case 0x23:
				case 0x26:
				case 0x29: // often -9999997 or 0
					cell.m_borders[(dt.id()-0x20)/3].m_unknown1 = int(dt.m_value);
					break;
				case 0x21:
				case 0x24:
				case 0x27:
				case 0x2a: // often 1
					cell.m_borders[(dt.id()-0x21)/3].m_unknown2 = int(dt.m_value);
					break;
				case 0x22:
				case 0x25:
				case 0x28:
				case 0x2b: // often 20
					cell.m_borders[(dt.id()-0x22)/3].m_unknown3 = int(dt.m_value);
					break;
				case 0x2c: // 1, 0, -1
					if (dt.m_value < 0 || dt.m_value >= 256)
					{
						done = false;
						break;
					}
					f2 << "f" << dt.id() << "=" << (int) (int8_t)dt.m_value << ",";
					break;
					// always 0,excepted 0x15 and 0x18 which can be 0 or 1 ?
				case 0xa:
				case 0xb:
				case 0xd:
				case 0x11:
				case 0x15:
				case 0x16:
				case 0x17:
				case 0x18:
					if (dt.m_value == 0);
					else if (dt.m_value > -10 && dt.m_value < 10)
						f2 << "f" << dt.id() << "=" << dt.m_value << ",";
					else done = false;
					break;
				case 0xc:
				case 0x14:
				case 0x19:
				case 0x1a: // flag ?
					if (dt.isTrue())
						f2 << "f" << dt.id() << ", ";
					else
						f2 << "f" << dt.id() << "=false, ";
					break;
				default:
					done = false;
					break;
				}

				if (!done)
					f2 << dt << ",";
			}
			cell.m_bdbox.set(Vec2i(dim[0],dim[1]),Vec2i(dim[2],dim[3]));
			table.m_cells[actualCell] = cell;

			f << cell;
			if (!f2.str().empty())
				f << ", unk=(" << f2.str() << ")";

			if (!error.empty()) f << ",###err=" << error;
			input->seek(lastPosOk+sz, WPX_SEEK_SET);

			ascii().addPos(lastPosOk);
			ascii().addNote(f.str().c_str());
		}
		if (!ok) break;
#ifdef DEBUG
		table.findRow();
#endif
		if (tableId >=0) m_state->m_tableMap[tableId] = table;
		else
		{
			WPS_DEBUG_MSG(("WPS8Table::readMCLD: find a table with negative id\n"));
		}
	}

	if (!ok)
	{
		WPS_DEBUG_MSG(("WPS8Table::readMCLD: stopped prematively"));
		ascii().addPos(lastPosOk);
		ascii().addNote("###MCLD");
	}

	return ok;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
