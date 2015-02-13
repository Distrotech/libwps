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

#include "LotusSpreadsheet.h"

#include "Lotus.h"

using namespace libwps;

//! Internal: namespace to define internal class of LotusParser
namespace LotusParserInternal
{
//! the font of a LotusParser
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
	SubDocument(RVNGInputStreamPtr input, LotusParser &pars, bool header) :
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
		WPS_DEBUG_MSG(("LotusParserInternal::SubDocument::parse: no listener\n"));
		return;
	}
	if (!dynamic_cast<WKSContentListener *>(listener.get()))
	{
		WPS_DEBUG_MSG(("LotusParserInternal::SubDocument::parse: bad listener\n"));
		return;
	}

	LotusParser *pser = m_parser ? dynamic_cast<LotusParser *>(m_parser) : 0;
	if (!pser)
	{
		listener->insertCharacter(' ');
		WPS_DEBUG_MSG(("LotusParserInternal::SubDocument::parse: bad parser\n"));
		return;
	}
#if 0
	pser->sendHeaderFooter(m_header);
#endif
}

//! the state of LotusParser
struct State
{
	//! constructor
	State(libwps_tools_win::Font::Type fontType) :  m_eof(-1), m_fontType(fontType), m_version(-1), m_fontsList(), m_pageSpan(), m_maxSheet(0), m_actPage(0), m_numPages(0),
		m_headerString(""), m_footerString("")
	{
	}
	//! returns a color corresponding to an id
	bool getColor(int id, uint32_t &color) const;
	//! return the default font style
	libwps_tools_win::Font::Type getDefaultFontType() const
	{
		if (m_fontType != libwps_tools_win::Font::UNKNOWN)
			return m_fontType;
		return m_version<=2 ? libwps_tools_win::Font::DOS_850 : libwps_tools_win::Font::WIN3_WEUROPE;
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
	//! the fonts list
	std::vector<Font> m_fontsList;
	//! the actual document size
	WPSPageSpan m_pageSpan;
	//! the last sheet number
	int m_maxSheet;
	int m_actPage /** the actual page*/, m_numPages /* the number of pages */;
	//! the header string
	std::string m_headerString;
	//! the footer string
	std::string m_footerString;
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
			WPS_DEBUG_MSG(("LotusParserInternal::State::getColor(): unknown Dos color id: %d\n",id));
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
		WPS_DEBUG_MSG(("LotusParserInternal::State::getColor(): unknown color id: %d\n",id));
		return false;
	}
	color = colorMap[id];
	return true;
}

}

// constructor, destructor
LotusParser::LotusParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
                         libwps_tools_win::Font::Type encoding) :
	WKSParser(input, header), m_listener(), m_state(), m_spreadsheetParser()

{
	m_state.reset(new LotusParserInternal::State(encoding));
	m_spreadsheetParser.reset(new LotusSpreadsheet(*this));
}

LotusParser::~LotusParser()
{
}

int LotusParser::version() const
{
	return m_state->m_version;
}

bool LotusParser::checkFilePosition(long pos)
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

libwps_tools_win::Font::Type LotusParser::getDefaultFontType() const
{
	return m_state->getDefaultFontType();
}

//////////////////////////////////////////////////////////////////////
// interface with LotusSpreadsheet
//////////////////////////////////////////////////////////////////////
bool LotusParser::getColor(int id, uint32_t &color) const
{
	return m_state->getColor(id, color);
}

bool LotusParser::getFont(int id, WPSFont &font, libwps_tools_win::Font::Type &type) const
{
	if (id < 0 || id>=(int)m_state->m_fontsList.size())
	{
		WPS_DEBUG_MSG(("LotusParser::getFont: can not find font %d\n", id));
		return false;
	}
	LotusParserInternal::Font const &ft=m_state->m_fontsList[size_t(id)];
	font=ft;
	type=ft.m_type;
	return true;
}

// main function to parse the document
void LotusParser::parse(librevenge::RVNGSpreadsheetInterface *documentInterface)
{
	RVNGInputStreamPtr input=getInput();
	if (!input)
	{
		WPS_DEBUG_MSG(("LotusParser::parse: does not find main ole\n"));
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
#if 0
			m_spreadsheetParser->setListener(m_listener);

			m_listener->startDocument();
			m_spreadsheetParser->sendSpreadsheet();
			m_listener->endDocument();
			m_listener.reset();
			ok = true;
#endif
		}
	}
	catch (...)
	{
		WPS_DEBUG_MSG(("LotusParser::parse: exception catched when parsing MN0\n"));
		throw (libwps::ParseException());
	}

	ascii().reset();
	if (!ok)
		throw(libwps::ParseException());
}

shared_ptr<WKSContentListener> LotusParser::createListener(librevenge::RVNGSpreadsheetInterface *interface)
{
	std::vector<WPSPageSpan> pageList;
	WPSPageSpan ps(m_state->m_pageSpan);
	if (!m_state->m_headerString.empty())
	{
		WPSSubDocumentPtr subdoc(new LotusParserInternal::SubDocument
		                         (getInput(), *this, true));
		ps.setHeaderFooter(WPSPageSpan::HEADER, WPSPageSpan::ALL, subdoc);
	}
	if (!m_state->m_footerString.empty())
	{
		WPSSubDocumentPtr subdoc(new LotusParserInternal::SubDocument
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
bool LotusParser::checkHeader(WPSHeader *header, bool strict)
{
	*m_state = LotusParserInternal::State(m_state->m_fontType);
	libwps::DebugStream f;

	RVNGInputStreamPtr input = getInput();
	if (!checkFilePosition(12))
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: file is too short\n"));
		return false;
	}

	input->seek(0,librevenge::RVNG_SEEK_SET);
	int firstOffset = (int) libwps::readU8(input);
	int type = (int) libwps::read8(input);
	int val=(int) libwps::read16(input);
	bool needEncoding=true;
	f << "FileHeader:";
	if (firstOffset == 0 && type == 0 && val==0x1a)
	{
		m_state->m_version=1;
		f << "DOS,";
	}
	else
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: find unexpected first data\n"));
		return false;
	}
	val=(int) libwps::readU16(input);
	if (val>=0x1000 && val<=0x1005)
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: find lotus123, sorry parsing this format is not implemented\n"));
		m_state->m_version=(val-0x1000)+1;
		f << "lotus123[" << m_state->m_version << "],";
	}
	else if (val==0x8007)
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: find lotus file format, sorry parsing only this file is not implemented\n"));
		// REMOVE ME and return false;
		f << "lotus123[FMT],";
	}
	else
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: unknown lotus 123 header\n"));
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
		header->setMajorVersion(uint8_t(100+m_state->m_version));
		header->setCreator(libwps::WPS_LOTUS);
		header->setKind(libwps::WPS_SPREADSHEET);
		header->setNeedEncoding(needEncoding);
	}
	return true;
}

bool LotusParser::readZones()
{
	RVNGInputStreamPtr input = getInput();
	input->seek(0, librevenge::RVNG_SEEK_SET);

	// data, format and ?
	for (int wh=0; wh<2; ++wh)
	{
		while (readZone()) ;

		//
		// look for ending
		//
		long pos = input->tell();
		if (!checkFilePosition(pos+4))
			break;
		int type = (int) libwps::readU16(input); // 1
		int length = (int) libwps::readU16(input);
		if (type==1 && length==0)
		{
			ascii().addPos(pos);
			ascii().addNote("Entries(EOF)");
			// end of block, look for other blocks
			continue;
		}
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		break;
	}

	while (!input->isEnd())
	{
		long pos=input->tell();
		int id = (int) libwps::readU8(input);
		int type = (int) libwps::readU8(input);
		long sz = (long) libwps::readU16(input);
		if ((type>0x2a) || sz<0 || !checkFilePosition(pos+4+sz))
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			break;
		}
		libwps::DebugStream f;
		f << "Entries(UnknZon" << std::hex << id << "):";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
	}

	if (!input->isEnd())
	{
		ascii().addPos(input->tell());
		ascii().addNote("Entries(Unknown)");
	}

	return true;
}

bool LotusParser::readZone()
{
	libwps::DebugStream f;
	RVNGInputStreamPtr input = getInput();
	long pos = input->tell();
	int id = (int) libwps::readU8(input);
	int type = (int) libwps::readU8(input);
	long sz = (long) libwps::readU16(input);
	if ((type>0x2a) || sz<0 || !checkFilePosition(pos+4+sz))
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	f << "Entries(Lotus";
	if (type) f << std::hex << type << std::dec << "A";
	f << std::hex << id << std::dec << "E):";
	bool ok = true, isParsed = false, needWriteInAscii = false;
	int row, col, table;
	int val;
	switch (type)
	{
	case 0:
		switch (id)
		{
		case 0:
			if (sz!=26)
			{
				ok=false;
				break;
			}
			f.str("");
			f << "Entries(BOF):";
			val=(int) libwps::readU16(input);
			if (val==0x8007)
				f << "FMT,";
			else if (val>=0x1000 && val <= 0x1005)
				f << "version=" << (val-0x1000) << ",";
			else
				f << "#version=" << std::hex << val << std::dec << ",";
			for (int i=0; i<4; ++i)   // f0=4, f3 a small number
			{
				val=(int) libwps::read16(input);
				if (val)
					f << "f" << i << "=" << val << ",";
			}
			val=(int) libwps::readU8(input);
			if (val)
			{
				m_state->m_maxSheet=val;
				f << "max[sheet]=" << val << ",";
			}
			for (int i=0; i<7; ++i)   // g0/g1=0..fd, g2=0|4, g3=0|5|7|1e|20|30, g4=0|8c|3d, g5=1|10, g6=2|a
			{
				val=(int) libwps::readU8(input);
				if (val)
					f << "g" << i << "=" << std::hex << val << std::dec << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x1: // EOF
			ok = false;
			break;
		case 0x3:
			if (sz!=6)
			{
				ok=false;
				break;
			}
			for (int i=0; i<3; ++i)   // f0=1, f2=1|32
			{
				val=(int) libwps::read16(input);
				if (val)
					f << "f" << i << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x4:
			if (sz!=28)
			{
				ok=false;
				break;
			}
			for (int i=0; i<2; ++i)   // f0=1|3, f1=1
			{
				val=(int) libwps::read8(input);
				if (val!=1)
					f << "f" << i << "=" << val << ",";
			}
			for (int i=0; i<2; ++i)   // f2=1-3, f1=0|1
			{
				val=(int) libwps::read16(input);
				if (val)
					f << "f" << i+1 << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x5:
		{
			f.str("");
			f << "Entries(Select):";
			if (sz!=16)
			{
				ok=false;
				break;
			}
			val=(int) libwps::read16(input);
			if (val)
				f << "id?=" << val << ","; // maybe sheet

			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x6:
			if (sz!=5)
			{
				ok=false;
				break;
			}
			val=(int) libwps::read16(input);
			if (val)
				f << "id?=" << val << ","; // small number
			f << "f1=" << libwps::readU16(input) << ",";
			f << "f2=" << (int) libwps::read8(input) << ","; // 7-9
			isParsed=needWriteInAscii=true;
			break;
		case 0x7:
		{
			f.str("");
			f << "Entries(ColSize):";
			if (sz<4 || (sz%2))
			{
				ok=false;
				f << "###";
				break;
			}
			val=(int) libwps::readU8(input);
			if (val)
				f << "sheet[id]=" << val << ","; // small number
			val=(int) libwps::readU8(input); // always 0?
			if (val) f << "f0=" << val << ",";
			f << "f1=" << std::hex << libwps::readU16(input) << std::dec << ","; // big number
			int N=int((sz-4)/2);
			f << "widths=[";
			for (int i=0; i<N; ++i)
			{
				col=(int)libwps::readU8(input);
				int width=(int)libwps::readU8(input); // width in char, default 12...
				f << width << "C:col" << col << ",";
			}
			f << "],";
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x9:
		{
			if (sz<0x1a)   // find 1a and 1c
			{
				ok=false;
				break;
			}
			val=(int) libwps::read16(input);
			if (val)
				f << "id=" << val << ","; // small number
			std::string name("");
			for (int i=0; i<16; ++i)
			{
				char c=(char) libwps::readU8(input);
				if (!c) break;
				name += c;
			}
			f << name << ",";
			input->seek(pos+4+18, librevenge::RVNG_SEEK_SET);
			for (int i=0; i<2; ++i)
			{
				f << "unkn" << i << "=[";
				for (int j=0; j<4; ++j)
				{
					val=(int)libwps::readU8(input);
					if (val) f << val << ",";
					else f << "_,";
				}
				f << "],";
			}
			// if sz=1c, followed by an int 4 or 9
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0xa:
		{
			if (sz<19)
			{
				ok=false;
				break;
			}
			int wh=(int) libwps::read16(input);
			if (wh==1)
				f << "link,";
			else if (wh)
				f << "#wh=" << wh << ","; // small number
			std::string name("");
			for (int i=0; i<16; ++i)
			{
				char c=(char) libwps::readU8(input);
				if (!c) break;
				name += c;
			}
			f << name << ",";
			input->seek(pos+4+18, librevenge::RVNG_SEEK_SET);
			if (wh==1)
			{
				name="";
				for (int i=18; i<sz; ++i)
				{
					char c=(char) libwps::readU8(input);
					if (!c) break;
					name += c;
				}
				f << "link=" << name << ",";
			}
			else if (sz<26)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: zone a seems too short\n"));
				f << "###";
			}
			else
			{
				for (int i=0; i<2; ++i)
				{
					f << "unkn" << i << "=[";
					for (int j=0; j<4; ++j)
					{
						val=(int)libwps::readU8(input);
						if (val) f << val << ",";
						else f << "_,";
					}
					f << "],";
				}
			}
			// if sz=1c, followed by an int 4 or 9
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0xb: // 0,1,-1
		case 0x1e: // always with 0
		case 0x21:
			if (sz!=1)
			{
				ok=false;
				break;
			}
			val=(int) libwps::read8(input);
			if (val==1)
				f << "true,";
			else if (val)
				f << "val=" << val << ",";
			break;
		case 0xc: // find 0 or 4 int with value 0|1|ff
			for (int i=0; i<sz; ++i)
			{
				val=(int) libwps::read8(input);
				if (val==1) f << "f" << i << ",";
				else if (val) f << "f" << i << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0xe:
			if (sz<32)
			{
				ok=false;
				break;
			}
			for (int i=0; i<30; ++i)   // f7=0|f, f8=0|60, f9=0|54, f17=80, f18=0|ff, f19=3f|40, f26=0|f8, f27=80|ff, f28=b|c,f29=40
			{
				val=(int) libwps::read8(input);
				if (val) f << "f" << i << "=" << val << ",";
			}
			val=(int) libwps::read16(input); // always 1?
			if (val!=1) f << "f30=" << val << ",";
			isParsed=needWriteInAscii=true;
			break;
		case 0xf:
			if (sz<0x56)
			{
				ok=false;
				break;
			}
			val=(int) libwps::read8(input); // 1|2
			if (val!=1) f << "f0=" << val << ",";
			for (int i=0; i<3; ++i)
			{
				long actPos=input->tell();
				std::string name("");
				for (int j=0; j<16; ++j)
				{
					char c=(char) libwps::readU8(input);
					if (!c) break;
					name += c;
				}
				if (!name.empty())
					f << "str" << i << "=" << name << ",";
				input->seek(actPos+16, librevenge::RVNG_SEEK_SET);
			}
			for (int i=0; i<17; ++i)   // f2=f11=1,f15=0|1, f16=0|2, f17=0|1|2
			{
				val=(int) libwps::read8(input);
				if (val) f << "f" << i+1 << "=" << val << ",";
			}
			for (int i=0; i<10; ++i)   // g0=0|1,g1=Ø|1, g2=4|a, g3=4c|50|80, g4=g5=0|2, g6=42, g7=41|4c, g8=3c|42|59
			{
				val=(int) libwps::read16(input);
				if (val) f << "g" << i << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x10:
		case 0x12:
		{
			if (sz<3)
			{
				ok=false;
				break;
			}
			for (int i=0; i<2; ++i)
			{
				val=(int) libwps::readU8(input);
				if (val) f << "f" << i << "=" << val << ",";
			}
			std::string data("");
			for (int i=2; i<sz; ++i)
			{
				char c=(char) libwps::readU8(input);
				if (!c) break;
				data += c;
			}
			if (!data.empty())
				f << "data=" << data << ",";
			if (input->tell()!=pos+4+sz && input->tell()+1!=pos+4+sz)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: the string zone %d seems too short\n", id));
				f << "###";
			}
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x15:
		case 0x1d:
			if (sz!=4)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			val=(int) libwps::read16(input); // small number 6-c maybe a style
			if (val) f << "f0=" << val << ",";
			for (int i=0; i<2; ++i)   // zone15: f1=3, f2=2-5, zone 1d: always 0
			{
				val=(int) libwps::readU8(input);
				if (val) f << "f" << i+1 << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x16:
		case 0x1a:
		case 0x26:
		{
			f.str("");
			if (id==0x16)
				f << "Entries(TextCell):";
			else if (id==0x1a)
				f << "Entries(TextVal):";
			else
				f << "Entries(Note):";
			if (sz<5)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			row=(int) libwps::readU16(input);
			table=(int) libwps::readU8(input);
			col=(int) libwps::readU8(input);
			f << "C" << col << "-" << row;
			if (table) f << "[" << table << "]";
			f << ",";
			std::string data("");
			for (int i=4; i<sz; ++i)
			{
				char c=(char) libwps::readU8(input);
				if (!c) break;
				data += c;
			}
			if (!data.empty())
				f << "data=" << data << ",";
			if (input->tell()!=pos+4+sz && input->tell()+1!=pos+4+sz)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: the string zone %d seems too short\n", id));
				f << "###";
			}
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x17:
		case 0x19:   // value+formula
		{
			f.str("");
			if (id==0x17)
				f << "Entries(DoubleVal):";
			else
				f << "Entries(Formula):";
			if ((id==0x17 && sz!=14) || (id==0x19 && sz<=14))
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			row=(int) libwps::readU16(input);
			table=(int) libwps::readU8(input);
			col=(int) libwps::readU8(input);
			f << "C" << col << "-" << row;
			if (table) f << "[" << table << "]";
			f << ",";
			double res;
			bool isNaN;
			if (!libwps::readDouble10(input, res, isNaN))
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: can read a double for zone%d\n", id));
				f << "###";
			}
			else
				f << res << ",";
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x18:
		{
			f.str("");
			f << "Entries(SmallDoubVal):";
			if (sz!=6)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			row=(int) libwps::readU16(input);
			table=(int) libwps::readU8(input);
			col=(int) libwps::readU8(input);
			f << "C" << col << "-" << row;
			if (table) f << "[" << table << "]";
			f << ",";
			double res;
			bool isNaN;
			if (!libwps::readDouble2Inv(input, res, isNaN))
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: can read a double for zone%d\n", id));
				f << "###";
			}
			else
				f << res << ",";
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x1c: // always 00002d000000
			if (sz!=6)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			for (int i=0; i<6; ++i)   // some int
			{
				val=(int) libwps::readU8(input);
				if (val) f << "f" << i << "=" << std::hex << val << std::dec << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x1f:
			f.str("");
			f << "Entries(Columns):";
			if (sz!=8)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			val=(int) libwps::readU8(input);
			if (val) f << "sheet=" << val << ",";
			val=(int) libwps::readU8(input);
			f << "col=" << val << ",";
			val=(int) libwps::readU8(input); // always 1?
			if (val!=1) f << "f0=" << val << ",";
			val=(int) libwps::readU8(input); // between 0 and 94
			if (val) f << "f1=" << val << ",";
			val=(int) libwps::readU16(input);
			if (val)
				f << "row[first]=" << val << ",";
			val=(int) libwps::readU16(input);
			if (val)
				f << "row[last]=" << val << ",";
			isParsed=needWriteInAscii=true;
			break;
		case 0x23:
		{
			f.str("");
			f << "Entries(SheetName):";
			if (sz<5)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			val=(int) libwps::read16(input); // always 14000
			if (val!=14000)
				f << "f0=" << val << ",";
			int sheetId=(int) libwps::readU16(input);
			f << "id[sheet]=" << sheetId << ",";
			std::string data("");
			for (int i=4; i<sz; ++i)
			{
				char c=(char) libwps::readU8(input);
				if (!c) break;
				data += c;
			}
			if (!data.empty())
				f << "data=" << data << ",";
			if (input->tell()!=pos+4+sz && input->tell()+1!=pos+4+sz)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: the string zone %d seems too short\n", id));
				f << "###";
			}
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x25:
		{
			f.str("");
			f << "Entries(MidDoubVal):";
			if (sz!=8)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			row=(int) libwps::readU16(input);
			table=(int) libwps::readU8(input);
			col=(int) libwps::readU8(input);
			f << "C" << col << "-" << row;
			if (table) f << "[" << table << "]";
			f << ",";
			double res;
			bool isNaN;
			if (!libwps::readDouble4Inv(input, res, isNaN))
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: can read a double for zone%d\n", id));
				f << "###";
			}
			else
				f << res << ",";
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x27:
		{
			f.str("");
			f << "Entries(Double8Val):";
			if (sz!=12)
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			row=(int) libwps::readU16(input);
			table=(int) libwps::readU8(input);
			col=(int) libwps::readU8(input);
			f << "C" << col << "-" << row;
			if (table) f << "[" << table << "]";
			f << ",";
			double res;
			bool isNaN;
			if (!libwps::readDouble8(input, res, isNaN))
			{
				WPS_DEBUG_MSG(("LotusParser::checkHeader: can read a double for zone%d\n", id));
				f << "###";
			}
			else
				f << res << ",";
			isParsed=needWriteInAscii=true;
			break;
		}
		// case 11: 1byte+strings + ?
		// case 13: big structure
		// case 1b: a struct checkme
		default:
			break;
		}
		break;
	default:
		// checkme: maybe <5 is ok
		if (version()<=2)
		{
			ok=false;
			break;
		}
		break;
	}
	if (!ok)
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	if (sz && input->tell()!=pos+4+sz)
		ascii().addDelimiter(input->tell(),'|');
	input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
	if (!isParsed || needWriteInAscii)
	{
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}
	return true;
}

////////////////////////////////////////////////////////////
//   generic
////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------
// Header/Footer
// ----------------------------------------------------------------------

////////////////////////////////////////////////////////////
//   chart
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
//   Unknown
////////////////////////////////////////////////////////////


/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
