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

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSCell.h"
#include "WKSContentListener.h"
#include "WPSEntry.h"
#include "WPSFont.h"
#include "WPSHeader.h"
#include "WPSPageSpan.h"

#include "WKS4Spreadsheet.h"

#include "WKS4.h"

namespace WKS4ParserInternal
{
//! the font of a WKS4Parser
struct Font : public WPSFont
{
	//! constructor
	Font() : WPSFont(), m_type(libwps_tools_win::Font::DOS_850)
	{
	}
	//! font encoding type
	libwps_tools_win::Font::Type m_type;
};

//! the state of WKS4
struct State
{
	State() :  m_eof(-1), m_version(-1), m_fontsList(), m_pageSpan(), m_actPage(0), m_numPages(0)
	{
	}
	//! returns a color corresponding to an id
	bool getColor(int id, uint32_t &color) const;
	//! returns a default font (Courier12) with file's version to define the default encoding */
	WPSFont getDefaulFont() const
	{
		WPSFont res;
		if (m_version <= 2)
			res.m_name="Courier";
		else
			res.m_name="Times New Roman";
		res.m_size=12;
		return res;
	}

	//! the last file position
	long m_eof;
	//! the file version
	int m_version;
	//! the fonts list
	std::vector<Font> m_fontsList;
	//! the actual document size
	WPSPageSpan m_pageSpan;
	int m_actPage /** the actual page*/, m_numPages /* the number of pages */;
};

bool State::getColor(int id, uint32_t &color) const
{
	if (m_version<=2)
	{
		static const uint32_t colorDosMap[]=
		{
			0x0, 0xFF0000, 0x00FF00, 0x0000FF,
			0x00FFFF, 0xFF00FF, 0xFFFF00
		};
		if (id < 0 || id >= 7)
		{
			WPS_DEBUG_MSG(("WPS4ParserInternal::StategetColor(): unknown Dos color id: %d\n",id));
			return false;
		}
		color=colorDosMap[id];
		return true;
	}
	static const uint32_t colorMap[]=
	{
		// 0x00RRGGBB
		0, // auto
		0,
		0x0000FF, 0x00FFFF,
		0x00FF00, 0xFF00FF,	0xFF0000, 0xFFFF00,
		0x808080, 0xFFFFFF,	0x000080, 0x008080,
		0x008000, 0x800080,	0x808000, 0xC0C0C0
	};
	if (id < 0 || id >= 16)
	{
		WPS_DEBUG_MSG(("WPS4Parser::getColor(): unknown color id: %d\n",id));
		return false;
	}
	color = colorMap[id];
	return true;
}

}

// constructor, destructor
WKS4Parser::WKS4Parser(RVNGInputStreamPtr &input, WPSHeaderPtr &header) :
	WKSParser(input, header), m_listener(), m_state(), m_spreadsheetParser()

{
	m_state.reset(new WKS4ParserInternal::State);
	m_spreadsheetParser.reset(new WKS4Spreadsheet(*this));
}

WKS4Parser::~WKS4Parser ()
{
}

int WKS4Parser::version() const
{
	return m_state->m_version;
}

bool WKS4Parser::checkFilePosition(long pos)
{
	if (m_state->m_eof < 0)
	{
		RVNGInputStreamPtr input = getInput();
		long actPos = input->tell();
		input->seek(0, librevenge::RVNG_SEEK_END);
		m_state->m_eof=input->tell();
		input->seek(actPos, librevenge::RVNG_SEEK_SET);
	}
	return pos <= m_state->m_eof;
}

//////////////////////////////////////////////////////////////////////
// interface with WKS4Spreadsheet
//////////////////////////////////////////////////////////////////////
bool WKS4Parser::getColor(int id, uint32_t &color) const
{
	return m_state->getColor(id, color);
}

bool WKS4Parser::getFont(int id, WPSFont &font, libwps_tools_win::Font::Type &type) const
{
	if (id < 0 || id>=(int)m_state->m_fontsList.size())
	{
		WPS_DEBUG_MSG(("WKS4Parser::getFont: can not find font %d\n", id));
		return false;
	}
	WKS4ParserInternal::Font const &ft=m_state->m_fontsList[size_t(id)];
	font=ft;
	type=ft.m_type;
	return true;
}

// main function to parse the document
void WKS4Parser::parse(librevenge::RVNGSpreadsheetInterface *documentInterface)
{
	RVNGInputStreamPtr input=getInput();
	if (!input)
	{
		WPS_DEBUG_MSG(("WKS4Parser::parse: does not find main ole\n"));
		throw(libwps::ParseException());
	}

	if (!checkHeader(0L, true)) throw(libwps::ParseException());

	bool ok=false;
	try
	{
		ascii().setStream(input);
		ascii().open("MN0");

		if (checkHeader(0L) && readZones())
			m_listener=createListener(documentInterface);
		if (m_listener)
		{
			m_spreadsheetParser->setListener(m_listener);

			m_listener->startDocument();
			m_spreadsheetParser->sendSpreadsheet();
			m_listener->endDocument();
			m_listener.reset();
			ok = true;
		}
	}
	catch (...)
	{
		WPS_DEBUG_MSG(("WKS4Parser::parse: exception catched when parsing MN0\n"));
		throw(libwps::ParseException());
	}

	ascii().reset();
	if (!ok)
		throw(libwps::ParseException());
}

shared_ptr<WKSContentListener> WKS4Parser::createListener(librevenge::RVNGSpreadsheetInterface *interface)
{
	return shared_ptr<WKSContentListener>(new WKSContentListener(interface));
}

////////////////////////////////////////////////////////////
// low level
////////////////////////////////////////////////////////////
// read the header
////////////////////////////////////////////////////////////
bool WKS4Parser::checkHeader(WPSHeader *header, bool strict)
{
	*m_state = WKS4ParserInternal::State();
	libwps::DebugStream f;

	RVNGInputStreamPtr input = getInput();
	if (!checkFilePosition(12))
	{
		WPS_DEBUG_MSG(("WKS4Parser::checkHeader: file is too short\n"));
		return false;
	}

	input->seek(0,librevenge::RVNG_SEEK_SET);
	int firstOffset = libwps::read16(input);
	f << "FileHeader(";
	if (firstOffset == 0)
	{
		m_state->m_version=2;
		f << "DOS";
	}
	else if (firstOffset == 0xff)
	{
		f << "Windows";
		m_state->m_version=3;
	}

	if ((firstOffset != 0 && firstOffset != 0xFF) || libwps::read16(input) != 2
	        || libwps::read16(input) != 0x0404)
	{
		WPS_DEBUG_MSG(("WKS4Parser::checkHeader: header contain unknown data\n"));
		return false;
	}
	if (strict)
	{
		for (int i=0; i < 3; ++i)
		{
			if (!readZone()) return false;
		}
	}
	ascii().addPos(0);
	ascii().addNote(f.str().c_str());
	ascii().addPos(6);
	if (header)
	{
		header->setMajorVersion((uint8_t) m_state->m_version);
		header->setKind(WPS_SPREADSHEET);
	}
	return true;
}

bool WKS4Parser::readZones()
{
	RVNGInputStreamPtr input = getInput();
	input->seek(6, librevenge::RVNG_SEEK_SET);

	while(readZone()) ;

	//
	// look for ending
	//
	long pos = input->tell();
	if (input->seek(0x4, librevenge::RVNG_SEEK_CUR) != 0)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readZones: cell header is too short\n"));
		return false;
	}
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	int type = (int) libwps::readU16(input); // 1
	int length = (int) libwps::readU16(input);
	if (length)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readZones: parse breaks before ending\n"));
		return false;
	}

	ascii().addPos(pos);
	if (type != 1)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readZones: odd end cell type: %d\n", type));
		ascii().addNote("__End###");
	}
	else
		ascii().addNote("__End");

	return true;
}

bool WKS4Parser::readZone()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();
	long pos = input->tell();
	int id = (int) libwps::readU8(input);
	int type =  (int) libwps::read8(input);
	long sz =  (long) libwps::readU16(input);
	if (sz<0 || !checkFilePosition(pos+4+sz))
	{
		WPS_DEBUG_MSG(("WKS4Parser::readZone: size is bad\n"));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}

	bool ok = true, isParsed = false;
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	switch(type)
	{
	case 0:
		switch(id)
		{
		case 0x1:
			ok = false;
			break;
			// case 2: ff
			// case 3: 0 and one time 00000000000075656269726420626f786573207468617420776572652000
			// case 4: 0|ff
			// case 5: ff
		case 0x6:
			ok = m_spreadsheetParser->readSheetSize();
			isParsed = true;
			break;
			// case 7|9: 0a00010001000a000f00270000000000000000000000000004000400960000 or similar
		case 0x8:
			ok = m_spreadsheetParser->readColumnSize();
			isParsed = true;
			break;
			// case a: id + small id
		case 0xb:
			ok = readZoneB();
			isParsed=true;
			break;
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
		case 0x10:
			ok = m_spreadsheetParser->readCell();
			isParsed = true;
			break;
			// case 1a: 0000000009002100
			// case 24: 0
			// case 25|26: 0 [ repeated 0xF2 times]
		case 0x2d:
		case 0x2e:
			readChartDef();
			isParsed = true;
			break;
		case 0x2f: // first microsoft works file ?
			if (sz!=1) break;
			WPS_DEBUG_MSG(("WKS4Parser::readZone: arggh a WKS1 file?\n"));
			f << "Entries(PreVersion):vers=" << (int) libwps::readU8(input);
			if (m_state->m_version==2)
				m_state->m_version=1;
			ascii().addPos(pos);
			ascii().addNote(f.str().c_str());
			isParsed = true;
			break;
			// case 31: 1-2
		case 0x41:
			readChartName();
			isParsed = true;
			break;
		default:
			break;
		}
		break;
	case 0x54:
		switch(id)
		{
		case 0x2:
			ok = m_spreadsheetParser->readDOSCellProperty();
			isParsed = true;
			break;
		case 0x5:
			if (sz!=2) break;
			f << "Entries(Version):vers=" << std::hex << libwps::readU16(input) << std::dec;
			ascii().addPos(pos);
			ascii().addNote(f.str().c_str());
			isParsed = true;
			break;
		case 0x13:
			ok = m_spreadsheetParser->readPageBreak();
			isParsed = true;
			break;
		case 0x14:
			readChartUnknown();
			isParsed = true;
			break;
		case 0x15:
			readChartList();
			isParsed = true;
			break;
		case 0x1c:
			m_spreadsheetParser->readDOSCellExtraProperty();
			isParsed = true;
			break;
		case 0x23: // single page ?
		case 0x37: // multiple page ?
			ok = readPrnt();
			isParsed = true;
			break;
		case 0x40:
			readChartFont();
			isParsed = true;
			break;
		case 0x56:
			ok = readFont();
			isParsed = true;
			break;
		case 0x5a:
			ok = m_spreadsheetParser->readStyle();
			isParsed = true;
			break;
		case 0x5b:
			ok = m_spreadsheetParser->readCell();
			isParsed = true;
			break;
		case 0x65:
			ok = m_spreadsheetParser->readRowSize2();
			isParsed = true;
			break;
		case 0x67: // single page ?
		case 0x82: // multiple page ?
			ok = readPrn2();
			isParsed = true;
			break;
		case 0x6b:
			ok = m_spreadsheetParser->readColumnSize2();
			isParsed = true;
			break;
		case 0x80:
		case 0x81:
			readChartLimit();
			isParsed = true;
			break;
		case 0x84:
			readChart2Font();
			isParsed = true;
			break;
		default:
			break;
		}
		break;
	default:
		ok = false;
		break;
	}

	if (!ok)
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	if (isParsed)
	{
		input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
		return true;
	}

	/*

	   StructA25: size=0
	   StructA26: size=2 content 0
	   StructA50: size=12
	 */
	f << "Entries(Struct";
	if (type == 0x54) f << "A";
	f << std::hex << id << std::dec << "E):";
	if (sz) ascii().addDelimiter(pos+4,'|');
	input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
//   generic
////////////////////////////////////////////////////////////
bool WKS4Parser::readFont()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();
	long pos = input->tell();
	int type = (int)libwps::read16(input);

	if (type != 0x5456)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readFont: not a font zone\n"));
		return false;
	}
	long sz = (long)libwps::readU16(input);
	long endPos = pos+4+sz;
	if (sz < 32)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readFont: seems very short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(Font)###");
		return true;
	}

	WKS4ParserInternal::Font font;
	int flags = (int)libwps::readU8(input);
	uint32_t attributes = 0;
	if (flags & 1) attributes |= WPS_BOLD_BIT;
	if (flags & 2) attributes |= WPS_ITALICS_BIT;
	if (flags & 4) attributes |= WPS_BOLD_BIT;
	if (flags & 8) attributes |= WPS_STRIKEOUT_BIT;

	font.m_attributes=attributes;
	if (flags & 0xF0)
	{
		if (!m_state->getColor((flags >> 4), font.m_color))
		{
			WPS_DEBUG_MSG(("WKS4Parser::readFont: unknown color\n"));
			f << "##color=" << (flags >> 4) << ",";
		}
	}

	int val=(int)libwps::readU8(input);
	if (val) f << "f0=" << std::hex << val << std::dec << ",";
	std::string name("");
	while (int(input->tell()) < endPos-4)
	{
		char c = (char) libwps::readU8(input);
		if (c == '\0') break;
		name+= c;
	}

	font.m_type=libwps_tools_win::Font::getFontType(name);
	if (font.m_type==libwps_tools_win::Font::UNKNOWN)
		font.m_type=version()<=2 ? libwps_tools_win::Font::DOS_850 : libwps_tools_win::Font::WIN3_WEUROPE;
	font.m_name=name;

	input->seek(endPos-4, librevenge::RVNG_SEEK_SET);
	val = (int) libwps::readU16(input); // always 0x20
	if (val!=0x20)  f << "f1=" << std::hex << val << std::dec << ",";
	int fSize = libwps::read16(input)/2;
	if (fSize >= 1 && fSize <= 50)
		font.m_size=double(fSize);
	font.m_extra=f.str();

	f.str("");
	f << "Entries(Font):";
	f << "font" << m_state->m_fontsList.size() << "[" << font << "]";
	m_state->m_fontsList.push_back(font);

	if (fSize == 0 && fSize > 50)
		f << "###fSize=" << fSize;
	if (name.length() <= 0) f << "###";
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool WKS4Parser::readPrnt()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();
	long pos = input->tell();
	int type = (int) libwps::read16(input);
	if (type != 0x5423 && type != 0x5437)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readPrnt: not a prnt zone\n"));
		return false;
	}
	long sz = (long)libwps::readU16(input);
	long endPos = pos+4+sz;

	f << "Entries(PRNT):";
	if (sz >= 12)
	{
		float dim[6];
		for (int i = 0; i < 6; i++)
			dim[i] = float(libwps::read16(input))/1440.f;
		f << "dim=" << dim[5] << "x" << dim[4] << ",";
		f << "margin=[" << dim[0] << "x" << dim[2] << ","
		  << dim[3] << "x" << dim[1] << "],";
		// check me
		m_state->m_pageSpan.setFormWidth(dim[5]);
		m_state->m_pageSpan.setFormLength(dim[4]);
		m_state->m_pageSpan.setMarginLeft(dim[0]);
		m_state->m_pageSpan.setMarginTop(dim[2]);
		m_state->m_pageSpan.setMarginRight(dim[3]);
		m_state->m_pageSpan.setMarginBottom(dim[1]);
	}
	long numElt = (endPos-input->tell())/2;
	for (int i = 0; i < numElt; i++)
	{
		// f0=1 (numSheet ?), f3/4=0x2d0 (dim in inches ? )
		int val = libwps::read16(input);
		if (!val) continue;
		f << "f" << i << "=" << std::hex << val << std::dec << ",";
	}

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Parser::readPrn2()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();
	long pos = input->tell();
	long type = libwps::read16(input);
	if (type != 0x5482 && type != 0x5467)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readPrn2: not a prn2 zone\n"));
		return false;
	}
	long sz = libwps::readU16(input);
	long endPos = pos+4+sz;

	f << "Entries(PRN2):";
	if (sz >= 64)
	{
		float dim[8];
		for (int st = 0; st < 2; st++)
		{
			for (int i = 0; i < 8; i++)
				dim[i] = float(libwps::read32(input))/1440.f;
			f << "dim" << st << "=" << dim[5] << "x" << dim[4] << ",";
			f << "margin" << st << "=[" << dim[0] << "x" << dim[2] << ","
			  << dim[3] << "x" << dim[1] << "],";
			f << "head/foot" << st << "?=" << dim[7] << "x" << dim[6] << ",";
		}
	}
	long numElt = (endPos-input->tell())/4;
	/*
	  in general only f0=1,
	  but sometime f0=1,f2=1,f8=64,f42=174,f44=1,f46=175,f48=1
	 */
	for (int i = 0; i < numElt; i++)
	{
		int val = (int) libwps::read16(input);
		if (!val) continue;
		f << "f" << i << "=" << std::hex << val << std::dec << ",";
	}

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Parser::readZoneB()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::readU16(input);
	if (type != 0xb)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readZoneB: not a zoneB type\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	if (sz != 0x18)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readZoneB: size seems bad\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(ZoneB):###");
		return true;
	}
	f << "Entries(ZoneB):";
	std::string name("");
	for (int i=0; i < 24; ++i)
	{
		char c=(char) libwps::read8(input);
		if (c=='\0') break;
		name+= c;
	}
	f << name;

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	if (input->tell()!=pos+4+sz)
		ascii().addDelimiter(input->tell(),'|');
	return true;
}

////////////////////////////////////////////////////////////
//   chart
////////////////////////////////////////////////////////////

bool WKS4Parser::readChartDef()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::readU16(input);
	if (type != 0x2D && type != 0x2e)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartDef: not a chart definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	int normalSz = (type == 0x2D) ? 0x1b5 : 0x1c5;
	if (sz < normalSz)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartDef: chart definition too short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(ChartDef):###");
		return true;
	}

	f << "Entries(ChartDef):";
	if (type == 0x2e)
	{
		std::string name("");
		for (int i = 0; i < 16; i++)
		{
			char c = (char) libwps::readU8(input);
			if (c == '\0') break;
			name+=c;
		}
		input->seek(pos+4+16, librevenge::RVNG_SEEK_SET);
		f << "title=" << name << ",";
	}
	for (int i = 0; i < 26; i++)
	{
		// some cell zone ?? : f0,f1,f2,f3 -> main zone, f14:f15 ?
		int val1 = (int) libwps::read16(input);
		int val2 = (int) libwps::read16(input);
		if (val1 != -1 || (val2 && val2 != -1))
		{
			WKSContentListener::FormulaInstruction instr;
			input->seek(-4, librevenge::RVNG_SEEK_CUR);
			if (!m_spreadsheetParser->readCell(Vec2i(0,0), instr)) f << "#";
			f << "f" << i << "=" << instr << ",";
		}
	}
	for (int i = 0; i < 49; i++)
	{
		// g0..g3 some small number
		// always g4=g8=0 and g9=4,g10=4,g11=4,g12=4,g13=4,g14=4
		int val = (char) libwps::read8(input);
		if (val) f << "g" << i << "=" << val << ",";
	}


	for (int i = 0; i < 10; i++)
	{
		// ok to find string before i < 6
		// checkme after
		std::string name("");
		long actPos = input->tell();
		int dataSz = i < 4 ? 40 : 20;
		for (int j = 0; j < dataSz; j++)
		{
			char c = (char) libwps::readU8(input);
			if (c == '\0') break;
			name+=c;
		}
		input->seek(actPos+dataSz, librevenge::RVNG_SEEK_SET);
		if (name.length()) f << "s" << i << "=" << name << ",";
	}
	for (int i = 0; i < 4; i++)
	{
		// always h0=h1=71, h2=1, h3=0 ?
		int val = (int) libwps::read8(input);
		if (val) f << "h" << i << "=" << std::hex << val << std::dec << ",";
	}

	if (sz != normalSz) ascii().addDelimiter(input->tell(),'#');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool WKS4Parser::readChartName()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 0x41)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartName: not a chart name\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);

	if (sz < 0x10)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartName: chart name is too short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(ChartName):###");
		return true;
	}

	std::string name("");
	for (int i = 0; i < 16; i++)
	{
		char c = (char) libwps::readU8(input);
		if (c == '\0') break;
		name += c;
	}
	f << "Entries(ChartName):" << name;
	if (sz != 0x10) ascii().addDelimiter(pos+4+sz, '#');

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Parser::readChartFont()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 0x5440)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartFont: not a chart font\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	long endPos = pos+4+sz;
	if (sz < 0x22)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartFont: chart font is too short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(ChartFont):###");
		return true;
	}
	f << "Entries(ChartFont):";
	int nbElt = int(sz/0x22);
	for (int i = 0; i < nbElt; i++)
	{
		long actPos = input->tell();
		f << "ft" << i << "=[";
		int fl = (int) libwps::readU8(input);
		f << "flag=" << std::hex << fl << std::dec << ",";
		std::string name("");
		for (int j = 0; j < 32; j++)
		{
			char c = (char) libwps::readU8(input);
			if (c == '\0') break;
			name += c;
		}
		f << name;
		input->seek(actPos+33, librevenge::RVNG_SEEK_SET);
		int fl2 = (int) libwps::readU8(input);
		if (fl2) f << ",flag2=" << std::hex << fl << std::dec;
		f << "],";
	}
	if (long(input->tell()) != endPos)
		ascii().addDelimiter(input->tell(),'#');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Parser::readChart2Font()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 0x5484)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChart2Font: not a chart2 font\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	long endPos = pos+4+sz;
	if (sz < 0x23)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChart2Font: chart2 font is too short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(ChartFont):###");
		return true;
	}
	f << "Entries(Chart2Font):";
	int nbElt = int(sz/0x23);
	for (int i = 0; i < nbElt; i++)
	{
		long actPos = input->tell();
		f << "ft" << i << "=[";
		int fl = (int) libwps::readU8(input);
		f << "flag=" << std::hex << fl << std::dec << ",";
		std::string name("");
		for (int j = 0; j < 32; j++)
		{
			char c = (char) libwps::readU8(input);
			if (c == '\0') break;
			name += c;
		}
		f << name;
		input->seek(actPos+33, librevenge::RVNG_SEEK_SET);
		int fl2 = (int) libwps::readU8(input);
		if (fl2) f << ",#flag2=" << std::hex << fl2 << std::dec;
		int fl3 = (int) libwps::readU8(input); // check me ?
		if (fl3) f << ",sz="  << fl3/2;
		f << "],";
	}
	if (long(input->tell()) != endPos)
		ascii().addDelimiter(input->tell(),'#');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Parser::readChartLimit()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type == 0x5480) f << "Entries(ChartBegin)";
	else if (type == 0x5481) f << "Entries(ChartEnd)";
	else
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartLimit: not a chart limit\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	if (sz != 0) ascii().addDelimiter(pos+4,'#');

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool WKS4Parser::readChartUnknown()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 0x5414)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartUnknown: not a chart ???\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	long endPos = pos+4+sz;
	if (sz < 0x8d)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartUnknown: chart2 ??? is too short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(ChartUnknown):###");
		return true;
	}
	f << "Entries(ChartUnknown):";

	for (int i = 0; i < 34; i++)
	{
		/* I find
		  f0={00|20}{23|30|32|34|38|60|70}
		  f5=7|17 f9=(f5)00 f11=0|ffff f24,...f29 (can have value)
		 f30=5|7 f33=7|5007*/
		int val = (int) libwps::readU16(input);
		if (val) f << "f" << i << "=" << std::hex << val << std::dec << ",";
	}
	long actPos = input->tell();
	std::string name("");
	for (int i = 0; i < 33; i++)
	{
		char c = (char) libwps::readU8(input);
		if (c == '\0') break;
		name+=c;
	}
	if (name.length()) f << "name=" << name << ",";
	input->seek(actPos+33, librevenge::RVNG_SEEK_SET);
	for (int i = 0; i < 3; i++)
	{
		int val = (int) libwps::read16(input);
		if (val) f << "g" << i << "=" << val << ",";
	}
	for (int i = 0; i < 5; i++)
	{
		// some cell zone ?? :
		int val1 = (int) libwps::read16(input);
		int val2 = (int) libwps::read16(input);
		if (val1 != -1 || val2)   // absolute or relative to ?
		{
			WKSContentListener::FormulaInstruction instr;
			input->seek(-4, librevenge::RVNG_SEEK_CUR);
			if (!m_spreadsheetParser->readCell(Vec2i(0,0), instr)) f << "#";
			f << "cell" << i << "=" << instr << ",";
		}
	}
	int val = (int) libwps::read16(input);
	if (val) f << "h0=" << val << ","; // 0 or 2

	// look like TL?=1,1.25,BR?=9,6,pageLength?=11,8.5
	f << "dim?=[";
	for (int i = 0; i < 6; i++)
	{
		val = (int) libwps::read16(input);
		f << val/1440. << ",";
	}
	f << "]";

	if (long(input->tell()) != endPos) ascii().addDelimiter(input->tell(), '#');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool WKS4Parser::readChartList()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 0x5415)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartList: not a chart ???\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);

	if (sz < 0x1e)
	{
		WPS_DEBUG_MSG(("WKS4Parser::readChartList: chart definition too short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(ChartList?):###");
		return true;
	}

	f << "Entries(ChartList?):";
	for (int i = 0; i < 6; i++)
	{
		int typ = (int) libwps::read8(input);
		int val1 = (int) libwps::read16(input);
		int val2 = (int) libwps::read16(input);
		if (type) f << "f" << i << "=" << typ << ",";
		if (val1 != -1 || val2)
		{
			WKSContentListener::FormulaInstruction instr;
			input->seek(-4, librevenge::RVNG_SEEK_CUR);
			if (!m_spreadsheetParser->readCell(Vec2i(0,0), instr)) f << "#";
			f << "g" << i << "=" << instr << ",";
		}
	}

	if (sz != 0x1e) ascii().addDelimiter(input->tell(),'#');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
