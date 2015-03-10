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

#include "WKSContentListener.h"
#include "WKSSubDocument.h"

#include "WPSCell.h"
#include "WPSEntry.h"
#include "WPSFont.h"
#include "WPSHeader.h"
#include "WPSPageSpan.h"

#include "QuattroSpreadsheet.h"

#include "Quattro.h"

using namespace libwps;

//! Internal: namespace to define internal class of QuattroParser
namespace QuattroParserInternal
{
//! the font of a QuattroParser
struct Font : public WPSFont
{
	//! constructor
	Font(libwps_tools_win::Font::Type type) : WPSFont(), m_type(type)
	{
	}
	//! font encoding type
	libwps_tools_win::Font::Type m_type;
};

//! Internal: the subdocument of a WPS4Parser
class SubDocument : public WKSSubDocument
{
public:
	//! constructor for a text entry
	SubDocument(RVNGInputStreamPtr input, QuattroParser &pars, bool header) :
		WKSSubDocument(input, &pars), m_header(header) {}
	//! destructor
	~SubDocument() {}

	//! operator==
	virtual bool operator==(shared_ptr<WKSSubDocument> const &doc) const
	{
		if (!doc || !WKSSubDocument::operator==(doc))
			return false;
		SubDocument const *sDoc = dynamic_cast<SubDocument const *>(doc.get());
		if (!sDoc) return false;
		return m_header == sDoc->m_header;
	}

	//! the parser function
	void parse(shared_ptr<WKSContentListener> &listener, libwps::SubDocumentType subDocumentType);
	//! a flag to known if we need to send the header or the footer
	bool m_header;
};

void SubDocument::parse(shared_ptr<WKSContentListener> &listener, libwps::SubDocumentType)
{
	if (!listener.get())
	{
		WPS_DEBUG_MSG(("QuattroParserInternal::SubDocument::parse: no listener\n"));
		return;
	}
	if (!dynamic_cast<WKSContentListener *>(listener.get()))
	{
		WPS_DEBUG_MSG(("QuattroParserInternal::SubDocument::parse: bad listener\n"));
		return;
	}

	QuattroParser *pser = m_parser ? dynamic_cast<QuattroParser *>(m_parser) : 0;
	if (!pser)
	{
		listener->insertCharacter(' ');
		WPS_DEBUG_MSG(("QuattroParserInternal::SubDocument::parse: bad parser\n"));
		return;
	}
	pser->sendHeaderFooter(m_header);
}

//! the state of QuattroParser
struct State
{
	//! constructor
	State(libwps_tools_win::Font::Type fontType) :  m_eof(-1), m_fontType(fontType), m_version(-1), m_hasLICSCharacters(false),
		m_fontsList(), m_pageSpan(), m_actPage(0), m_numPages(0), m_headerString(""), m_footerString("")
	{
	}
	//! returns a color corresponding to an id
	bool getColor(int id, WPSColor &color) const;
	//! return the default font style
	libwps_tools_win::Font::Type getDefaultFontType() const
	{
		if (m_fontType != libwps_tools_win::Font::UNKNOWN)
			return m_fontType;
		if (m_version>2)
			return libwps_tools_win::Font::WIN3_WEUROPE;
		return libwps_tools_win::Font::CP_437;
	}

	//! returns a default font (Courier12) with file's version to define the default encoding */
	WPSFont getDefaultFont() const
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
	//! the user font type
	libwps_tools_win::Font::Type m_fontType;
	//! the file version
	int m_version;
	//! flag to know if the character
	bool m_hasLICSCharacters;
	//! the fonts list
	std::vector<Font> m_fontsList;
	//! the actual document size
	WPSPageSpan m_pageSpan;
	int m_actPage /** the actual page*/, m_numPages /* the number of pages */;
	//! the header string
	std::string m_headerString;
	//! the footer string
	std::string m_footerString;
};

bool State::getColor(int id, WPSColor &color) const
{
	static const uint32_t quattroColorMap[]=
	{
		0x000000, 0x0000FF, 0x00FF00, 0x00FFFF,
		0xFF0000, 0xFF00FF, 0x996633/*brown*/, 0xFFFFFF,
		0x808080 /* gray */, 0x0000C0, 0x00C000, 0x00C0C0,
		0xC00000, 0xC000C0, 0xFFFF00, 0xC0C0C0 /* bright white */
	};
	if (id < 0 || id >= 16)
	{
		WPS_DEBUG_MSG(("QuattroParserInternal::State::getColor(): unknown Quattro Pro color id: %d\n",id));
		return false;
	}
	color = WPSColor(quattroColorMap[id]);
	return true;
}

}

// constructor, destructor
QuattroParser::QuattroParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
                             libwps_tools_win::Font::Type encoding) :
	WKSParser(input, header), m_listener(), m_state(), m_spreadsheetParser()

{
	m_state.reset(new QuattroParserInternal::State(encoding));
	m_spreadsheetParser.reset(new QuattroSpreadsheet(*this));
}

QuattroParser::~QuattroParser()
{
}

int QuattroParser::version() const
{
	return m_state->m_version;
}

bool QuattroParser::checkFilePosition(long pos)
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

libwps_tools_win::Font::Type QuattroParser::getDefaultFontType() const
{
	return m_state->getDefaultFontType();
}

//////////////////////////////////////////////////////////////////////
// interface with QuattroSpreadsheet
//////////////////////////////////////////////////////////////////////
bool QuattroParser::getColor(int id, WPSColor &color) const
{
	return m_state->getColor(id, color);
}

bool QuattroParser::getFont(int id, WPSFont &font, libwps_tools_win::Font::Type &type) const
{
	if (id < 0 || id>=(int)m_state->m_fontsList.size())
	{
		WPS_DEBUG_MSG(("QuattroParser::getFont: can not find font %d\n", id));
		return false;
	}
	QuattroParserInternal::Font const &ft=m_state->m_fontsList[size_t(id)];
	font=ft;
	type=ft.m_type;
	return true;
}

bool QuattroParser::hasLICSCharacters() const
{
	if (version()<=2)
		return m_state->m_hasLICSCharacters;
	return false;
}

// main function to parse the document
void QuattroParser::parse(librevenge::RVNGSpreadsheetInterface *documentInterface)
{
	RVNGInputStreamPtr input=getInput();
	if (!input)
	{
		WPS_DEBUG_MSG(("QuattroParser::parse: does not find main ole\n"));
		throw (libwps::ParseException());
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
			int numSheet=m_spreadsheetParser->getNumSpreadsheets();
			if (numSheet==0) ++numSheet;
			for (int i=0; i<numSheet; ++i)
				m_spreadsheetParser->sendSpreadsheet(i);
			m_listener->endDocument();
			m_listener.reset();
			ok = true;
		}
	}
	catch (...)
	{
		WPS_DEBUG_MSG(("QuattroParser::parse: exception catched when parsing MN0\n"));
		throw (libwps::ParseException());
	}

	ascii().reset();
	if (!ok)
		throw(libwps::ParseException());
}

shared_ptr<WKSContentListener> QuattroParser::createListener(librevenge::RVNGSpreadsheetInterface *interface)
{
	std::vector<WPSPageSpan> pageList;
	WPSPageSpan ps(m_state->m_pageSpan);
	if (!m_state->m_headerString.empty())
	{
		WPSSubDocumentPtr subdoc(new QuattroParserInternal::SubDocument
		                         (getInput(), *this, true));
		ps.setHeaderFooter(WPSPageSpan::HEADER, WPSPageSpan::ALL, subdoc);
	}
	if (!m_state->m_footerString.empty())
	{
		WPSSubDocumentPtr subdoc(new QuattroParserInternal::SubDocument
		                         (getInput(), *this, false));
		ps.setHeaderFooter(WPSPageSpan::FOOTER, WPSPageSpan::ALL, subdoc);
	}
	pageList.push_back(ps);
	return shared_ptr<WKSContentListener>(new WKSContentListener(pageList, interface));
}

////////////////////////////////////////////////////////////
// low level
////////////////////////////////////////////////////////////
// read the header
////////////////////////////////////////////////////////////
bool QuattroParser::checkHeader(WPSHeader *header, bool strict)
{
	*m_state = QuattroParserInternal::State(m_state->m_fontType);
	libwps::DebugStream f;

	RVNGInputStreamPtr input = getInput();
	if (!checkFilePosition(12))
	{
		WPS_DEBUG_MSG(("QuattroParser::checkHeader: file is too short\n"));
		return false;
	}

	input->seek(0,librevenge::RVNG_SEEK_SET);
	int firstOffset = (int) libwps::readU8(input);
	int type = (int) libwps::read8(input);
	bool needEncoding=true;
	f << "FileHeader:";
	if (firstOffset == 0 && type == 0)
		m_state->m_version=1;
	else
	{
		WPS_DEBUG_MSG(("QuattroParser::checkHeader: find unexpected first data\n"));
		return false;
	}
	if (type != 0)
	{
		WPS_DEBUG_MSG(("QuattroParser::checkHeader: find unexpected type file\n"));
		return false;
	}
	int val=(int) libwps::read16(input);
	if (val==2)
	{
		// version
		val=(int) libwps::readU16(input);
		if (val==0x5120)
		{
			m_state->m_version=1;
			f << "quattropro[wb1],";
		}
		else if (val==0x5121)
		{
			m_state->m_version=2;
			f << "quattropro[wb2],";
		}
		else
		{
			WPS_DEBUG_MSG(("QuattroParser::checkHeader: find unknown file version\n"));
			return false;
		}
	}
	else
	{
		WPS_DEBUG_MSG(("QuattroParser::checkHeader: header contain unexpected size field data\n"));
		return false;
	}

	input->seek(0, librevenge::RVNG_SEEK_SET);
	if (strict)
	{
		for (int i=0; i < 4; ++i)
		{
			if (!readZone()) return false;
		}
	}
	ascii().addPos(0);
	ascii().addNote(f.str().c_str());

	if (header)
	{
		header->setMajorVersion((uint8_t) m_state->m_version);
		header->setCreator(libwps::WPS_QUATTRO_PRO);
		header->setKind(libwps::WPS_SPREADSHEET);
		header->setNeedEncoding(needEncoding);
	}
	return true;
}

bool QuattroParser::readZones()
{
	RVNGInputStreamPtr input = getInput();
	input->seek(0, librevenge::RVNG_SEEK_SET);
	while (readZone()) ;

	//
	// look for ending
	//
	long pos = input->tell();
	if (!checkFilePosition(pos+4))
	{
		WPS_DEBUG_MSG(("QuattroParser::readZones: cell header is too short\n"));
		return m_spreadsheetParser->getNumSpreadsheets()>0;
	}
	int type = (int) libwps::readU16(input); // 1
	int length = (int) libwps::readU16(input);
	if (length)
	{
		WPS_DEBUG_MSG(("QuattroParser::readZones: parse breaks before ending\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(BAD):###");
		return m_spreadsheetParser->getNumSpreadsheets()>0;
	}

	ascii().addPos(pos);
	if (type != 1)
	{
		WPS_DEBUG_MSG(("QuattroParser::readZones: odd end cell type: %d\n", type));
		ascii().addNote("Entries(BAD):###");
	}
	else
		ascii().addNote("__End");

	return true;
}

bool QuattroParser::readZone()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();
	long pos = input->tell();
	int id = (int) libwps::readU8(input);
	int type = (int) libwps::read8(input);
	long sz = (long) libwps::readU16(input);
	if (sz<0 || !checkFilePosition(pos+4+sz))
	{
		WPS_DEBUG_MSG(("QuattroParser::readZone: size is bad\n"));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}

	if (type!=0)
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}

	f << "Entries(Struct" << std::hex << id << std::dec << "E):";
	bool ok = true, isParsed = false, needWriteInAscii = false;
	int val;
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	switch (id)
	{
	/* also
	   32: symphony windows settings(144)
	   37: password checksum(4)
	   3c: query(127)
	   3d: query name(16)
	   3e: symphony print record (679)
	   3f: printer name(16)
	   40: symphony graph record (499)
	   42: zoom(9)
	   43: number of split windows(2)
	   44: number of screen row(2)
	   45: number of screen column(2)
	   46: name ruler range(25)
	   47: name sheet range(25)
	   48: autoload comm(65)
	   49: autoexecutute macro adress(8)
	   4a: query parse information
	 */
	case 0:
		if (sz!=2) break;
		f.str("");
		f << "version=" << std::hex << libwps::readU16(input) << std::dec << ",";
		isParsed=needWriteInAscii=true;
		break;
	case 0x1: // EOF
		ok = false;
		break;
	// boolean
	case 0x2: // Calculation mode 0 or FF
	case 0x3: // Calculation order
	case 0x4: // Split window type
	case 0x5: // Split window syn
	case 0x29: // label format 22|27|5e (spreadsheet)
	case 0x30: // formatted/unformatted print 0|ff (spreadsheet)
	case 0x31: // cursor/location 1|2
	case 0x38: // lock
		f.str("");
		f << "Entries(Byte" << std::hex << id << std::dec << "Z):";
		if (sz!=1) break;
		input->seek(pos+4, librevenge::RVNG_SEEK_SET);
		val=(int) libwps::readU8(input);
		if (id==0x29)
			f << "val=" << std::hex << val << std::dec << ",";
		else if (id==0x31)
		{
			if (val!=1) f << val << ",";
		}
		else
		{
			if (val==0xFF) f << "true,";
			else if (val) f << "#val=" << val << ",";
		}
		isParsed=needWriteInAscii=true;
		break;
	case 0x6: // active worksheet range
		ok = m_spreadsheetParser->readSheetSize();
		isParsed = true;
		break;
	case 0x7: // window 1 record
	case 0x9: // window 2 record
		ok = readWindowRecord();
		isParsed=true;
		break;
	case 0x8: // col width
		ok = m_spreadsheetParser->readColumnSize();
		isParsed = true;
		break;
	case 0xa: // col width (window 2)
		if (sz!=3) break;
		input->seek(pos+4, librevenge::RVNG_SEEK_SET);
		// varies in this file from 0 to 5
		f << "id=" << libwps::read16(input) << ",";
		// small number 9-13: a dim?
		f << "dim?=" << libwps::read8(input) << ",";
		isParsed=needWriteInAscii=true;
		break;
	case 0xb: // named range
		ok = readFieldName();
		isParsed=true;
		break;
	case 0xc: // blank cell
	case 0xd: // integer cell
	case 0xe: // floating cell
	case 0xf: // label cell
	case 0x10: // formula cell
		ok = m_spreadsheetParser->readCell();
		isParsed = true;
		break;
	case 0x33:  // value of string formula
		ok = m_spreadsheetParser->readCellFormulaResult();
		isParsed = true;
		break;
	// some spreadsheet zone ( mainly flags )
	case 0x18: // data table range
	case 0x19: // query range
	case 0x20: // distribution range
	case 0x27: // print setup
	case 0x2a: // print borders
		ok = readUnknown1();
		isParsed=true;
		break;
	case 0x1a: // print range
	case 0x1b: // sort range
	case 0x1c: // fill range
	case 0x1d: // primary sort key range
	case 0x23:   // secondary sort key range
	{
		int expectedSz=8;
		f.str("");
		switch (id)
		{
		case 0x1a: // only in spreadsheet?
			f << "Entries(PrintRange):";
			break;
		case 0x1b: // a dimension or also some big selection? 31999=infinity?, related to report?
			f << "Entries(SortRange):";
			break;
		case 0x1c: // a dimension or also some big selection? only in spreadsheet
			f << "Entries(FillRange):";
			break;
		case 0x1d:
			f << "Entries(PrimSort):";
			expectedSz=9;
			break;
		case 0x23:
			f << "Entries(SecSort):";
			expectedSz=9;
			break;
		default:
			break;
		}
		if (sz!=expectedSz) break;
		input->seek(pos+4, librevenge::RVNG_SEEK_SET);
		int dim[4];
		for (int i=0; i<4; ++i) dim[i]=(int) libwps::read16(input);
		// in a spreadsheet, the cell or the cells corresponding to the field
		// in a database, col,0,col,0
		if (dim[0]==-1 && dim[1]==dim[0] && dim[2]==dim[0] && dim[3]==dim[0])
		{
		}
		else
		{
			f << "cell=" << dim[0] << "x" << dim[1];
			if (dim[0]!=dim[2] || dim[1]!=dim[3])
				f << "<->" << dim[2] << "x" << dim[3];
			f << ",";
		}
		if (expectedSz==9)
		{
			val=(int) libwps::readU8(input); // 0|1|ff
			if (val==0xFF) f << "true,";
			else if (val) f << "val=" << val << ",";
		}
		isParsed=needWriteInAscii=true;
		break;
	}
	case 0x24: // protection (global)
		f.str("");
		f << "Entries(Protection):global,";
		if (sz!=1) break;
		input->seek(pos+4, librevenge::RVNG_SEEK_SET);
		val=(int) libwps::readU8(input);
		if (val==0)
		{
			f.str("");
			f << "_";
		}
		else if (val==0xFF) f << "protected,";
		else if (val) f << "#protected=" << val << ",";
		isParsed=needWriteInAscii=true;
		break;
	case 0x25: // footer
	case 0x26: // header
		readHeaderFooter(id==0x26);
		isParsed = true;
		break;
	case 0x28: // print margin
		if (sz!=10) break;
		input->seek(pos+4, librevenge::RVNG_SEEK_SET);
		for (int i=0; i<5; ++i)   // f1=4c|96|ac|f0
		{
			static const int expected[]= {4, 0x4c, 0x42, 2, 2};
			val=(int) libwps::read16(input);
			if (val!=expected[i]) f << "f" << i << "=" << val << ",";
		}
		isParsed=needWriteInAscii=true;
		break;
	case 0x2d: // graph setting
	case 0x2e: // named graph setting
		readChartDef();
		isParsed = true;
		break;
	case 0x2f: // iteration count: only in dos file Wk1, Wks(dos), Wq[12] ?
		if (sz!=1) break;
		input->seek(pos+4, librevenge::RVNG_SEEK_SET);
		f.str("");
		val=(int) libwps::readU8(input);
		f << "Entries(ItCount):dos";
		if (val!=1) f << "=" << val << ",";
		isParsed = needWriteInAscii = true;
		break;
	case 0x36: // find one time in a spreadsheet (not standard)
	{
		if (sz!=0x1e) break;
		input->seek(pos+4, librevenge::RVNG_SEEK_SET);
		for (int i=0; i<3; ++i)   // always 0?
		{
			val=(int) libwps::read16(input);
			if (val) f << "f" << i << "=" << val << ",";
		}
		// after find some junk, so..
		isParsed = needWriteInAscii = true;
		break;
	}
	case 0x41: // graph record name
		readChartName();
		isParsed = true;
		break;
	case 0x64: // hidden column
		isParsed = m_spreadsheetParser->readHiddenColumns();
		break;

	//
	// specific to Quattro Pro file
	//
	case 0x9b: // only in  wq1 file
		readUserFonts();
		isParsed=true;
		break;
	case 0x9c:
	{
		f.str("");
		f << "Entries(CellProperty)[position]:";
		if ((sz%6)!=0) break;
		input->seek(pos+4, librevenge::RVNG_SEEK_SET);
		int N=int(sz/6);
		f << "num=[";
		for (int i=0; i<N; ++i)
		{
			int row=(int) libwps::readU16(input);
			int col=(int) libwps::readU16(input);
			f << "C" << col << "x" << row << ":";
			int num=(int) libwps::readU16(input);
			f << num << ",";
		}
		f << "],";
		isParsed=needWriteInAscii=true;
		break;
	}
	case 0x9d:
		m_spreadsheetParser->readCellProperty();
		isParsed = true;
		break;
	case 0xc9:
		m_spreadsheetParser->readUserStyle();
		isParsed = true;
		break;
	case 0xd8:
		m_spreadsheetParser->readCellStyle();
		isParsed = true;
		break;
	case 0xdc:
		m_spreadsheetParser->readSpreadsheetOpen();
		isParsed = true;
		break;
	case 0xdd:
		m_spreadsheetParser->readSpreadsheetClose();
		isParsed = true;
		break;
	default:
		break;
	}

	if (!ok)
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	if (isParsed)
	{
		if (needWriteInAscii)
		{
			ascii().addPos(pos);
			ascii().addNote(f.str().c_str());
		}
		input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
		return true;
	}

	if (sz && input->tell()!=pos && input->tell()!=pos+4+sz)
		ascii().addDelimiter(input->tell(),'|');
	input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
//   generic
////////////////////////////////////////////////////////////
bool QuattroParser::readUserFonts()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();
	long pos = input->tell();
	int type = (int)libwps::read16(input);

	if (type != 0x9b)
	{
		WPS_DEBUG_MSG(("QuattroParser::readUserFonts: not a font zone\n"));
		return false;
	}
	long sz = (long)libwps::readU16(input);
	f << "Entries(UserFont)[qpro]:";
	if ((sz%8)!=0)
	{
		WPS_DEBUG_MSG(("QuattroParser::readUserFonts: seems very short\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	int N=int(sz/8);
	for (int i=0; i<N; ++i)
	{
		pos=input->tell();
		f.str("");

		QuattroParserInternal::Font font(getDefaultFontType());
		int flags = (int)libwps::readU16(input);
		uint32_t attributes = 0;
		if (flags & 1) attributes |= WPS_BOLD_BIT;
		if (flags & 2) attributes |= WPS_ITALICS_BIT;
		if (flags & 8) attributes |= WPS_UNDERLINE_BIT;

		font.m_attributes=attributes;
		if (flags & 0xFFF4)
			f << "fl=" << std::hex << (flags & 0xFFF4) << std::dec << ",";
		int fId=(int) libwps::readU16(input);
		f << "fId=" << fId << ",";
		int fSize = (int) libwps::readU16(input);
		if (fSize >= 1 && fSize <= 50)
			font.m_size=double(fSize);
		else
			f << "###fSize=" << fSize << ",";
		int color = (int) libwps::readU16(input);
		if (color && !m_state->getColor(color, font.m_color))
		{
			WPS_DEBUG_MSG(("QuattroParser::readUserFonts: unknown color\n"));
			f << "##color=" << color << ",";
		}

		font.m_extra=f.str();
		m_state->m_fontsList.push_back(font);

		f.str("");
		f << "UserFont:Fo" << i << "," << font;

		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		input->seek(pos+8, librevenge::RVNG_SEEK_SET);
	}
	return true;
}

// ----------------------------------------------------------------------
// Header/Footer
// ----------------------------------------------------------------------
void QuattroParser::sendHeaderFooter(bool header)
{
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("QuattroParser::sendHeaderFooter: can not find the listener\n"));
		return;
	}

	m_listener->setFont(m_state->getDefaultFont());
	libwps_tools_win::Font::Type fontType=m_state->getDefaultFontType();

	std::string const &text=header ? m_state->m_headerString : m_state->m_footerString;
	bool hasLICS=hasLICSCharacters();
	for (size_t i=0; i < text.size(); ++i)
	{
		unsigned char c=(unsigned char) text[i];
		if (c==0xd)
			m_listener->insertEOL();
		else if (c==0xa)
			continue;
		else if (!hasLICS)
			m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::unicode(c,fontType));
		else
			m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::LICSunicode(c,fontType));
	}

}

bool QuattroParser::readHeaderFooter(bool header)
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();
	long pos = input->tell();
	int type = (int) libwps::read16(input);
	if (type != 0x0026 && type != 0x0025)
	{
		WPS_DEBUG_MSG(("QuattroParser::readHeaderFooter: not a header/footer\n"));
		return false;
	}
	long sz = (long)libwps::readU16(input);
	long endPos = pos+4+sz;

	f << "Entries(" << (header ? "HeaderText" : "FooterText") << "):";
	if (sz==1)
	{
		// followed with 0
		int val=(int) libwps::read8(input);
		if (val) f << "##f0=" << val << ",";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	if (sz < 0xF2)
	{
		WPS_DEBUG_MSG(("QuattroParser::readHeaderFooter: the header/footer size seeem odds\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return false;
	}
	std::string text("");
	for (long i=0; i < sz; i++)
	{
		char c=(char) libwps::read8(input);
		if (c=='\0') break;
		text+=c;
	}
	if (header)
		m_state->m_headerString=text;
	else
		m_state->m_footerString=text;
	f << text;
	if (input->tell()!=endPos)
		ascii().addDelimiter(input->tell(), '|');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

bool QuattroParser::readFieldName()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::readU16(input);
	if (type != 0xb)
	{
		WPS_DEBUG_MSG(("QuattroParser::readFieldName: not a zoneB type\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	f << "Entries(FldNames):";
	if (sz != 0x18 && sz != 0x1e)
	{
		// find also 0x85 a zone with 4 fldnames ?
		WPS_DEBUG_MSG(("QuattroParser::readFieldName: size seems bad\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	std::string name("");
	int sSz=(int) libwps::readU8(input);
	if (sSz<=0 || sSz>15)
	{
		f << "##sSz=" << sSz << ",";
		WPS_DEBUG_MSG(("QuattroParser::readFieldName: name's size seems bad\n"));
	}
	else
	{
		for (int i=0; i < sSz; ++i)
			name+= (char) libwps::read8(input);
	}
	f << name << ',';

	input->seek(pos+20, librevenge::RVNG_SEEK_SET);
	// the position
	int dim[4];
	if (sz==0x18)
	{
		for (int i=0; i<4; ++i) dim[i]=(int) libwps::read16(input);
	}
	else
	{
		for (int i=0; i<7; ++i)
		{
			int val=(int) libwps::read16(input);
			if (i<2) dim[i]=val;
			else if (i>=3 && i<5) dim[i-1]=val;
			else if (val) f << "f" << i << "=" << val << ",";
		}
	}
	f << "cell=" << dim[0] << "x" << dim[1];
	if (dim[0]!=dim[2] || dim[1]!=dim[3])
		f << "<->" << dim[2] << "x" << dim[3];
	f << ",";
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
//   chart
////////////////////////////////////////////////////////////

bool QuattroParser::readChartDef()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::readU16(input);
	if (type != 0x2D && type != 0x2e)
	{
		WPS_DEBUG_MSG(("QuattroParser::readChartDef: not a chart definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	int normalSz = (type == 0x2D) ? 0x1b5 : 0x1c5;
	if (sz < normalSz)
	{
		WPS_DEBUG_MSG(("QuattroParser::readChartDef: chart definition too short\n"));
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

bool QuattroParser::readChartName()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 0x41)
	{
		WPS_DEBUG_MSG(("QuattroParser::readChartName: not a chart name\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);

	if (sz < 0x10)
	{
		WPS_DEBUG_MSG(("QuattroParser::readChartName: chart name is too short\n"));
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

////////////////////////////////////////////////////////////
//   Unknown
////////////////////////////////////////////////////////////


/* the zone 0:7 and 0:9 */
bool QuattroParser::readWindowRecord()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 7 && type != 9)
	{
		WPS_DEBUG_MSG(("QuattroParser::readWindowRecord: unknown type\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);

	// normally size=0x1f but one time 0x1e
	if (sz < 0x1e)
	{
		WPS_DEBUG_MSG(("QuattroParser::readWindowRecord: zone seems too short\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(WindowRecord):###");
		return true;
	}

	f << "Entries(WindowRecord)[" << type << "]:";
	// f0=0-a|1f|21, f1=1-3c|1da, f2=0-6|71|7f|f1, f3=4|9|a|c(size?),
	// f4=0|4|6-11, f5=5-2c, f6=0|3|6|10|1f|20, f7=0-3c|1ca (related to f1?)
	// f8=0|1, f9=0|2, f10=f11=0
	for (int i=0; i<12; ++i)
	{
		int val=(int) libwps::read16(input);
		if (val) f << "f" << i << "=" << val << ",";
	}
	for (int i=0; i<2; ++i)   // g0=4, g1=4|a|b|10
	{
		int val=(int) libwps::read16(input);
		if (val!=4) f << "g" << i << "=" << val << ",";
	}
	int val=(int) libwps::read16(input); // number between -5 and ad
	f << "g2=" << val << ",";

	if (sz!=0x1e)
		ascii().addDelimiter(input->tell(),'|');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

/* some spreadsheet zones 0:18, 0:19, 0:20, 0:27, 0:2a */
bool QuattroParser::readUnknown1()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();

	long pos = input->tell();
	long type = (long) libwps::read16(input);
	int expectedSize=0, extraSize=0;
	switch (type)
	{
	case 0x18:
	case 0x19:
		expectedSize=(version()>=2) ? 0x25 : 0x19;
		break;
	case 0x20:
	case 0x2a:
		expectedSize=(version()>=2) ? 0x18 : 0x10;
		break;
	case 0x27:
		expectedSize=0x19;
		extraSize=15;
		break;
	default:
		WPS_DEBUG_MSG(("QuattroParser::readUnknown1: unexpected type ???\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);

	f << "Entries(Flags" << std::hex << type << std::dec << ")]:";
	if (sz != expectedSize+extraSize)
	{
		// find also 270001000[01]
		if (type==0x27 && sz==1)
		{
			f << "f0=" << (int) libwps::read8(input) << ",";
			ascii().addPos(pos);
			ascii().addNote(f.str().c_str());
			return true;
		}
		WPS_DEBUG_MSG(("QuattroParser::readUnknown1: the zone size seems too bad\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}

	// find always 0xff, excepted for zone 18(f0=0), zone 19(f24=0|3), zone 27(f0=..=f23=0|ff, f24=0|3)
	for (int i=0; i<expectedSize; ++i)
	{
		int val=(int) libwps::read8(input);
		if (val!=-1) f << "f" << i << "=" << val << ",";
	}

	if (type==0x27)
	{
		int val=(int) libwps::read8(input); // always 0
		if (val) f << "g0=" << val << ",";
		for (int i=0; i<7; ++i)   // g1=0|4, g2=0|72, g4=0|20, g5=0|1|4, g6=0|1, g7=0|-1|205|80d8|e9f
		{
			val=(int) libwps::read16(input);
			if (val) f << "g" << i+1 << "=" << val << ",";
		}
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	return true;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
