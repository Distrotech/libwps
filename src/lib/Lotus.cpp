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
#include <map>
#include <sstream>
#include <stack>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WKSContentListener.h"
#include "WKSSubDocument.h"

#include "WPSCell.h"
#include "WPSEntry.h"
#include "WPSFont.h"
#include "WPSHeader.h"
#include "WPSOLE1Parser.h"
#include "WPSPageSpan.h"
#include "WPSStream.h"

#include "LotusGraph.h"
#include "LotusSpreadsheet.h"
#include "LotusStyleManager.h"

#include "Lotus.h"

using namespace libwps;

//! Internal: namespace to define internal class of LotusParser
namespace LotusParserInternal
{
//! the font of a LotusParser
struct Font : public WPSFont
{
	//! constructor
	explicit Font(libwps_tools_win::Font::Type type) : WPSFont(), m_type(type)
	{
	}
	//! font encoding type
	libwps_tools_win::Font::Type m_type;
};

//! Internal: the subdocument of a LotusParser
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
	explicit State(libwps_tools_win::Font::Type fontType) : m_fontType(fontType), m_version(-1),
		m_inMainContentBlock(false), m_fontsMap(), m_pageSpan(), m_maxSheet(0),
		m_actualLevels(), m_actPage(0), m_numPages(0),
		m_headerString(""), m_footerString(""), m_metaData()
	{
	}
	//! return the default font style
	libwps_tools_win::Font::Type getDefaultFontType() const
	{
		if (m_fontType != libwps_tools_win::Font::UNKNOWN)
			return m_fontType;
		return libwps_tools_win::Font::WIN3_WEUROPE;
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

	//! returns a name corresponding to the actual level(for debugging)
	std::string getLevelsDebugName() const
	{
		std::stringstream s;
		for (size_t i=0; i<m_actualLevels.size(); ++i)
		{
			if (i==0 && m_actualLevels[0]==Vec2i(0,0)) continue;
			if (i<4)
			{
				char const *(wh[])= {"Z", "T", "C", "R"};
				s << wh[i];
			}
			else
				s << "[F" << i << "]";
			if (m_actualLevels[i][0]==m_actualLevels[i][1])
				s << "_";
			else if (m_actualLevels[i][0]+1==m_actualLevels[i][1])
				s << m_actualLevels[i][0];
			else
				s << m_actualLevels[i][0] << "x" << m_actualLevels[i][1]-1;
		}
		return s.str();
	}
	//! the user font type
	libwps_tools_win::Font::Type m_fontType;
	//! the file version
	int m_version;
	//! a flag used to know if we are in the main block or no
	bool m_inMainContentBlock;
	//! the fonts map
	std::map<int, Font> m_fontsMap;
	//! the actual document size
	WPSPageSpan m_pageSpan;
	//! the last sheet number
	int m_maxSheet;
	//! the actual zone
	std::vector<Vec2i> m_actualLevels;
	int m_actPage /** the actual page*/, m_numPages /* the number of pages */;
	//! the header string
	std::string m_headerString;
	//! the footer string
	std::string m_footerString;
	//! the metadata
	librevenge::RVNGPropertyList m_metaData;

private:
	State(State const &);
	State operator=(State const &);
};

}

// constructor, destructor
LotusParser::LotusParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
                         libwps_tools_win::Font::Type encoding) :
	WKSParser(input, header), m_listener(), m_state(), m_styleManager(), m_graphParser(), m_spreadsheetParser()

{
	m_state.reset(new LotusParserInternal::State(encoding));
	m_styleManager.reset(new LotusStyleManager(*this));
	m_graphParser.reset(new LotusGraph(*this));
	m_spreadsheetParser.reset(new LotusSpreadsheet(*this));
}

LotusParser::~LotusParser()
{
}

int LotusParser::version() const
{
	return m_state->m_version;
}

//////////////////////////////////////////////////////////////////////
// interface
//////////////////////////////////////////////////////////////////////
libwps_tools_win::Font::Type LotusParser::getDefaultFontType() const
{
	return m_state->getDefaultFontType();
}

bool LotusParser::getFont(int id, WPSFont &font, libwps_tools_win::Font::Type &type) const
{
	if (m_state->m_fontsMap.find(id)==m_state->m_fontsMap.end())
	{
		WPS_DEBUG_MSG(("LotusParser::getFont: can not find font %d\n", id));
		return false;
	}
	LotusParserInternal::Font const &ft=m_state->m_fontsMap.find(id)->second;
	font=ft;
	type=ft.m_type;
	return true;
}

bool LotusParser::hasGraphics(int sheetId) const
{
	return m_graphParser->hasGraphics(sheetId);
}

void LotusParser::sendGraphics(int sheetId)
{
	m_graphParser->sendGraphics(sheetId);
}

//////////////////////////////////////////////////////////////////////
// parsing
//////////////////////////////////////////////////////////////////////

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
		if (checkHeader(0L) && createZones())
			createListener(documentInterface);
		if (m_listener)
		{
			m_styleManager->updateState();
			m_spreadsheetParser->updateState();

			m_graphParser->setListener(m_listener);
			m_spreadsheetParser->setListener(m_listener);

			m_listener->startDocument();
			for (int i=0; i<=m_state->m_maxSheet; ++i)
				m_spreadsheetParser->sendSpreadsheet(i);
			m_listener->endDocument();
			m_listener.reset();
			ok = true;
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

bool LotusParser::createListener(librevenge::RVNGSpreadsheetInterface *interface)
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
	int numPages=m_state->m_maxSheet+1;
	if (numPages<=0) numPages=1;
	for (int i=0; i<numPages; ++i) pageList.push_back(ps);
	m_listener.reset(new WKSContentListener(pageList, interface));
	m_listener->setMetaData(m_state->m_metaData);
	return true;
}

////////////////////////////////////////////////////////////
// low level
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// read the header
////////////////////////////////////////////////////////////
bool LotusParser::checkHeader(WPSHeader *header, bool strict)
{
	m_state.reset(new LotusParserInternal::State(m_state->m_fontType));
	shared_ptr<WPSStream> mainStream(new WPSStream(getInput(), ascii()));
	if (!checkHeader(mainStream, true, strict))
		return false;
	if (header)
	{
		header->setMajorVersion(uint8_t(100+m_state->m_version));
		header->setCreator(libwps::WPS_LOTUS);
		header->setKind(libwps::WPS_SPREADSHEET);
		header->setNeedEncoding(true);
	}
	return true;
}

bool LotusParser::checkHeader(shared_ptr<WPSStream> stream, bool mainStream, bool strict)
{
	RVNGInputStreamPtr input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	if (!stream->checkFilePosition(12))
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: file is too short\n"));
		return false;
	}

	input->seek(0,librevenge::RVNG_SEEK_SET);
	int firstOffset = (int) libwps::readU8(input);
	int type = (int) libwps::read8(input);
	int val=(int) libwps::read16(input);
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
	if (!mainStream)
	{
		if (val!=0x8007)
		{
			WPS_DEBUG_MSG(("LotusParser::checkHeader: find unknown lotus file format\n"));
			return false;
		}
		f << "lotus123[FMT],";
	}
	else if (val>=0x1000 && val<=0x1002)
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: find lotus123 file\n"));
		m_state->m_version=(val-0x1000)+1;
		f << "lotus123[" << m_state->m_version << "],";
	}
#ifdef DEBUG
	else if (val>0x1002 && val<=0x1005)
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: find lotus123 file\n"));
		m_state->m_version=(val-0x1000)+1;
		f << "lotus123[" << m_state->m_version << "],";
	}
	else if (val==0x8007)
	{
		WPS_DEBUG_MSG(("LotusParser::checkHeader: find lotus file format, sorry parsing this file is only implemented for debugging, not output will be created\n"));
		f << "lotus123[FMT],";
	}
#endif
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
			if (!readZone(stream)) return false;
		}
	}
	ascFile.addPos(0);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusParser::createZones()
{
	RVNGInputStreamPtr input=getInput();
	if (!input)
	{
		WPS_DEBUG_MSG(("LotusParser::createZones: can not find the main input\n"));
		return false;
	}
	m_styleManager->cleanState();
	m_graphParser->cleanState();
	m_spreadsheetParser->cleanState();

	int const vers=version();

	shared_ptr<WPSStream> mainStream(new WPSStream(input, ascii()));
	if (vers>=3)
	{
		shared_ptr<WPSOLE1Parser> ole1Parser(new WPSOLE1Parser(mainStream));
		ole1Parser->createZones();
		shared_ptr<WPSStream> wkStream=ole1Parser->getStreamForName(vers==3 ? "WK3" : "123");
		if (wkStream)
		{
			if (!readZones(wkStream)) return false;
			ole1Parser->updateMetaData(m_state->m_metaData, getDefaultFontType());
			if (vers==3)
			{
				shared_ptr<WPSStream> fmStream=ole1Parser->getStreamForName("FM3");
				if (fmStream) readZones(fmStream);
			}
			return true;
		}
	}
	input->seek(0, librevenge::RVNG_SEEK_SET);
	if (!readZones(mainStream)) return false;
	if (vers<=2) parseFormatStream();
	return true;
}

bool LotusParser::parseFormatStream()
{
	RVNGInputStreamPtr file=getFileInput();
	if (!file || !file->isStructured()) return false;

	unsigned numSubStreams = file->subStreamCount();
	std::string wk3Name, fm3Name;
	RVNGInputStreamPtr formatInput;
	for (unsigned i = 0; i < numSubStreams; ++i)
	{
		char const *nm=file->subStreamName(i);
		std::string name(nm);
		size_t len=name.length();
		if (len<=4 || name.find_last_of('/')!=std::string::npos || name[0]=='.' || name[len-4]!='.')
			continue;
		std::string extension=name.substr(len-3);
		if (extension=="wk3" || extension=="WK3")
		{
			if (!wk3Name.empty())
			{
				wk3Name.clear();
				break;
			}
			wk3Name=name;
		}
		else if (extension=="fm3" || extension=="FM3")
		{
			if (!fm3Name.empty())
			{
				fm3Name.clear();
				break;
			}
			fm3Name=name;
			formatInput.reset(file->getSubStreamByName(nm));
			formatInput->seek(0, librevenge::RVNG_SEEK_SET);
		}
	}
	if (wk3Name.empty() || fm3Name.empty() ||  !formatInput ||
	        wk3Name.substr(0,wk3Name.length()-3) != fm3Name.substr(0,fm3Name.length()-3))
	{
		WPS_DEBUG_MSG(("LotusParser::parseFormatStream: can not find the format stream\n"));
		return false;
	}

	shared_ptr<WPSStream> formatStream(new WPSStream(formatInput));
	formatInput->seek(0, librevenge::RVNG_SEEK_SET);
	formatStream->m_ascii.open("FM3");
	formatStream->m_ascii.setStream(formatInput);
	if (!checkHeader(formatStream, false, false))
	{
		WPS_DEBUG_MSG(("LotusParser::parseFormatStream: can not read format stream\n"));
		return false;
	}
	return readZones(formatStream);
}

bool LotusParser::readZones(shared_ptr<WPSStream> stream)
{
	if (!stream)
	{
		WPS_DEBUG_MSG(("LotusParser::readZones: can not find the stream\n"));
		return false;
	}
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;

	bool mainDataRead=false;
	// data, format and ?
	for (int wh=0; wh<2; ++wh)
	{
		if (input->isEnd())
			break;

		while (readZone(stream)) ;

		//
		// look for ending
		//
		long pos = input->tell();
		if (!stream->checkFilePosition(pos+4))
			break;
		int type = (int) libwps::readU16(input); // 1
		int length = (int) libwps::readU16(input);
		if (type==1 && length==0)
		{
			ascFile.addPos(pos);
			ascFile.addNote("Entries(EOF)");
			if (!mainDataRead)
				mainDataRead=m_state->m_inMainContentBlock;
			// end of block, look for other blocks
			continue;
		}
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		break;
	}

	while (!input->isEnd())
	{
		long pos=input->tell();
		if (pos>=stream->m_eof) break;
		int id = (int) libwps::readU8(input);
		int type = (int) libwps::readU8(input);
		long sz = (long) libwps::readU16(input);
		if ((type>0x2a) || sz<0 || !stream->checkFilePosition(pos+4+sz))
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			break;
		}
		libwps::DebugStream f;
		f << "Entries(UnknZon" << std::hex << id << "):";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
	}

	if (!input->isEnd() && input->tell()<stream->m_eof)
	{
		ascFile.addPos(input->tell());
		ascFile.addNote("Entries(Unknown)");
	}

	return mainDataRead || m_spreadsheetParser->hasSomeSpreadsheetData();
}

bool LotusParser::readZone(shared_ptr<WPSStream> stream)
{
	if (!stream)
		return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	int id = (int) libwps::readU8(input);
	int type = (int) libwps::readU8(input);
	long sz = (long) libwps::readU16(input);
	long endPos=pos+4+sz;
	if ((type>0x2a) || sz<0 || !stream->checkFilePosition(endPos))
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	f << "Entries(Lotus";
	if (type) f << std::hex << type << std::dec << "A";
	f << std::hex << id << std::dec << "E):";
	bool ok = true, isParsed = false, needWriteInAscii = false;
	int val;
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	int const vers=version();
	switch (type)
	{
	case 0:
		switch (id)
		{
		case 0:
		{
			if (sz!=26)
			{
				ok=false;
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			f.str("");
			f << "Entries(BOF):";
			val=(int) libwps::readU16(input);
			m_state->m_inMainContentBlock=false;
			if (val==0x8007)
				f << "FMT,";
			else if (val>=0x1000 && val <= 0x1005)
			{
				m_state->m_inMainContentBlock=true;
				f << "version=" << (val-0x1000) << ",";
			}
			else
				f << "#version=" << std::hex << val << std::dec << ",";
			for (int i=0; i<4; ++i)   // f0=4, f3 a small number
			{
				val=(int) libwps::read16(input);
				if (val)
					f << "f" << i << "=" << val << ",";
			}
			val=(int) libwps::readU8(input);
			if (m_state->m_inMainContentBlock)
			{
				m_spreadsheetParser->setLastSpreadsheetId(val);
				m_state->m_maxSheet=val;
			}
			if (val && m_state->m_inMainContentBlock)
				f << "max[sheet]=" << val << ",";
			else if (val)
				f << "max[fmt]=" << val << ",";

			for (int i=0; i<7; ++i)   // g0/g1=0..fd, g2=0|4, g3=0|5|7|1e|20|30, g4=0|8c|3d, g5=1|10, g6=2|a
			{
				val=(int) libwps::readU8(input);
				if (val)
					f << "g" << i << "=" << std::hex << val << std::dec << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x1: // EOF
			ok = false;
			break;
		case 0x3:
			if (sz!=6)
			{
				ok=false;
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
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
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
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
			f << "Entries(SheetUnknA):";
			if (sz!=16)
			{
				ok=false;
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			val=(int) libwps::readU8(input);
			if (val)
				f << "sheet[id]=" << val << ",";
			val=(int) libwps::read8(input); // always 0?
			if (val)
				f << "f0=" << val << ",";

			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x6: // one by sheet
			f.str("");
			f << "Entries(SheetUnknB):";
			if (sz!=5)
			{
				ok=false;
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			val=(int) libwps::readU8(input);
			if (val)
				f << "sheet[id]=" << val << ",";
			for (int i=0; i<4; ++i)   // f0=0, f2=0|1, f3=7-9
			{
				val=(int) libwps::read8(input); // always 0?
				if (val)
					f << "f" << i << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x7:
			ok=isParsed=m_spreadsheetParser->readColumnSizes(stream);
			break;
		case 0x9:
			ok=isParsed=m_spreadsheetParser->readCellName(stream);
			break;
		case 0xa:
			ok=isParsed=readLinkZone(stream);
			break;
		case 0xb: // 0,1,-1
		case 0x1e: // always with 0
		case 0x21:
			if (sz!=1)
			{
				ok=false;
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			val=(int) libwps::read8(input);
			if (val==1)
				f << "true,";
			else if (val)
				f << "val=" << val << ",";
			break;
		case 0xc: // find 0 or 4 int with value 0|1|ff
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			for (int i=0; i<sz; ++i)
			{
				val=(int) libwps::read8(input);
				if (val==1) f << "f" << i << ",";
				else if (val) f << "f" << i << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0xe:
			if (sz<30)
			{
				ok=false;
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			for (int i=0; i<30; ++i)   // f7=0|f, f8=0|60, f9=0|54, f17=80, f18=0|ff, f19=3f|40, f26=0|f8, f27=80|ff, f28=b|c,f29=40
			{
				val=(int) libwps::read8(input);
				if (val) f << "f" << i << "=" << val << ",";
			}
			if (sz>=32)
			{
				val=(int) libwps::read16(input); // always 1?
				if (val!=1) f << "f30=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0xf:
			if (sz<0x56)
			{
				ok=false;
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
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
		case 0x10: // CHECKME
		{
			if (sz<3)
			{
				ok=false;
				break;
			}
			f.str("");
			f << "Entries(Macro):";
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
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
			if (input->tell()!=endPos && input->tell()+1!=endPos)
			{
				WPS_DEBUG_MSG(("LotusParser::readZone: the string zone %d seems too short\n", id));
				f << "###";
			}
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0x11:
			ok=isParsed=readChartDefinition(stream);
			break;
		case 0x12:
			ok=isParsed=readChartName(stream);
			break;
		case 0x13:
			isParsed=m_spreadsheetParser->readRowFormats(stream);
			break;
		case 0x15:
		case 0x1d:
			if (sz!=4)
			{
				WPS_DEBUG_MSG(("LotusParser::readZone: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			val=(int) libwps::read16(input); // small number 6-c maybe a style
			if (val) f << "f0=" << val << ",";
			for (int i=0; i<2; ++i)   // zone15: f1=3, f2=2-5, zone 1d: always 0
			{
				val=(int) libwps::readU8(input);
				if (val) f << "f" << i+1 << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x16: // the cell text
		case 0x17: // double10 cell
		case 0x18: // uint16 double cell
		case 0x19: // double10+formula
		case 0x1a: // text formula result cell
		case 0x25: // uint32 double cell
		case 0x26: // comment cell
		case 0x27: // double8 cell
		case 0x28: // double8+formula
			ok=isParsed=m_spreadsheetParser->readCell(stream);
			break;
		case 0x1b:
			isParsed=readDataZone(stream);
			break;
		case 0x1c: // always 00002d000000
			if (sz!=6)
			{
				WPS_DEBUG_MSG(("LotusParser::readZone: size of zone%d seems bad\n", id));
				f << "###";
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			for (int i=0; i<6; ++i)   // some int
			{
				val=(int) libwps::readU8(input);
				if (val) f << "f" << i << "=" << std::hex << val << std::dec << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0x1f:
			isParsed=ok=m_spreadsheetParser->readColumnDefinition(stream);
			break;
		case 0x23:
			isParsed=ok=m_spreadsheetParser->readSheetName(stream);
			break;
		// case 13: big structure

		//
		// format:
		//
		case 0x93: // 4
		case 0x96: // 0 or FF
		case 0x97: // 5F
		case 0x98: // 0|2|3
		case 0x99: // 0|4 or FF
		case 0x9c: // 0
		case 0xa3: // 0 or FF
		case 0xce: // 1
		case 0xcf: // 1
		case 0xd0: // 1
			if (m_state->m_inMainContentBlock) break;
			f.str("");
			f << "Entries(FMTByte" << std::hex << id << std::dec << "Z):";
			if (sz!=1)
			{
				f << "###";
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			val=int(libwps::readU8(input));
			if (val==0xFF) f << "true,";
			else if (val) f << "val=" << val << ",";
			isParsed=needWriteInAscii=true;
			break;
		case 0x87: // always with 0000
		case 0x88: // always with 0000
		case 0x8e: // with 57|64
		case 0x9a: // with 800
		case 0x9b: // with 720
		case 0xcd: // with 57|64
			if (m_state->m_inMainContentBlock) break;
			f.str("");
			f << "Entries(FMTInt" << std::hex << id << std::dec << "Z):";
			if (sz!=2)
			{
				f << "###";
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			val=int(libwps::readU16(input));
			if (val) f << "val=" << val << ",";
			isParsed=needWriteInAscii=true;
			break;
		case 0x86:
		case 0x89:
		case 0xba: // header?
		case 0xbb: // footer?
		{
			if (m_state->m_inMainContentBlock) break;
			f.str("");
			if (id==0x86)
				f << "Entries(FMTPrinter):";
			else if (id==0x89)
				f << "Entries(FMTPrinter):shortName,";
			else if (id==0xba)
				f << "Entries(FMTHeader):";
			else
				f << "Entries(FMTFooter):";

			if (sz<1)
			{
				f << "###";
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			std::string text;
			for (int i=0; i<sz; ++i) text+=char(libwps::readU8(input));
			f << text << ",";
			isParsed=needWriteInAscii=true;
			break;
		}
		case 0xae:
			if (m_state->m_inMainContentBlock) break;
			isParsed=m_styleManager->readFMTFontName(stream);
			break;
		case 0xaf:
		case 0xb1:
			if (m_state->m_inMainContentBlock) break;
			isParsed=m_styleManager->readFMTFontSize(stream);
			break;
		case 0xb0:
			if (m_state->m_inMainContentBlock) break;
			isParsed=m_styleManager->readFMTFontId(stream);
			break;
		case 0xb6:
			if (m_state->m_inMainContentBlock) break;
			isParsed=readFMTStyleName(stream);
			break;
		case 0xb8: // always 0101
			if (m_state->m_inMainContentBlock) break;
			f.str("");
			f << "Entries(FMTInts" << std::hex << id << std::dec << "Z):";
			if (sz!=2)
			{
				f << "###";
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			for (int i=0; i<2; ++i)
			{
				val=int(libwps::readU8(input));
				if (val!=1) f << "f" << i << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0xc3:
			if (m_state->m_inMainContentBlock) break;
			isParsed=ok=m_spreadsheetParser->readSheetHeader(stream);
			break;
		case 0xc4: // with 0-8, 5c-15c
		case 0xcb: // with 1,1
			if (m_state->m_inMainContentBlock) break;
			f.str("");
			f << "Entries(FMTInt2" << std::hex << id << std::dec << "Z):";
			if (sz!=4)
			{
				f << "###";
				break;
			}
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			for (int i=0; i<2; ++i)
			{
				val=int(libwps::readU16(input));
				if (val) f << "f" << i << "=" << val << ",";
			}
			isParsed=needWriteInAscii=true;
			break;
		case 0xc5:
			if (m_state->m_inMainContentBlock) break;
			isParsed=ok=m_spreadsheetParser->readExtraRowFormats(stream);
			break;
		case 0xc9:
			if (m_state->m_inMainContentBlock) break;
			isParsed=ok=m_graphParser->readZoneBeginC9(stream);
			break;
		case 0xca: // a graphic
			if (m_state->m_inMainContentBlock) break;
			isParsed=ok=m_graphParser->readGraphic(stream);
			break;
		case 0xcc: // frame of a graphic
			if (m_state->m_inMainContentBlock) break;
			isParsed=ok=m_graphParser->readFrame(stream);
			break;
		case 0xd1: // the textbox data
			if (m_state->m_inMainContentBlock) break;
			f.str("");
			f << "Entries(FMTTextBoxD):";
			break;
		case 0xb7: // main style ?
		case 0xbf: // serie style ?
		case 0xc0: // horizontal axis ?
		case 0xc2: // vertical axis ?
			if (m_state->m_inMainContentBlock) break;
			f.str("");
			f << "Entries(FMTChart" << std::hex << id << std::dec << "):";
			break;
		default:
			input->seek(pos+4, librevenge::RVNG_SEEK_SET);
			if (!m_state->m_inMainContentBlock && id>=0x80)
			{
				f.str("");
				f << "Entries(FMT" << std::hex << id << std::dec << "E):";
			}
			break;
		}
		break;
	default:
		// checkme: maybe <5 is ok
		if (vers<=2)
		{
			ok=false;
			break;
		}
		ok = isParsed=readZoneV3(stream);
		break;
	}
	if (!ok)
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	if (sz && input->tell()!=pos && input->tell()!=endPos)
		ascFile.addDelimiter(input->tell(),'|');
	input->seek(endPos, librevenge::RVNG_SEEK_SET);
	if (!isParsed || needWriteInAscii)
	{
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
	}
	return true;
}

bool LotusParser::readDataZone(shared_ptr<WPSStream> stream)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	int type = (int) libwps::readU16(input);
	long sz = (long) libwps::readU16(input);
	long endPos=pos+4+sz;
	if (type!=0x1b || sz<2)
	{
		WPS_DEBUG_MSG(("LotusParser::readDataZone: the zone seems odd\n"));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	type = (int) libwps::readU16(input);
	f << "Entries(Data" << std::hex << type << std::dec << "E):";
	bool isParsed=false, needWriteInAscii=false;
	sz-=2;
	int val;
	switch (type)
	{
	//
	// mac windows
	//
	case 0x7d2:
	{
		f.str("");
		f << "Entries(WindowsMacDef):";
		if (sz<26)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the windows definition seems bad\n"));
			f << "###";
			break;
		}
		val=(int) libwps::readU8(input);
		if (val) f << "id=" << val << ",";
		val=(int) libwps::read8(input); // find 0|2
		if (val) f << "f0=" << val << ",";
		int dim[4];
		for (int i=0; i<4; ++i)
		{
			dim[i]=(int) libwps::read16(input);
			val=(int) libwps::read16(input);
			if (!val) continue;
			if (i)
				f << "num[split]=" << val << ",";
			else
				f << "dim" << i << "[h]=" << val << ",";
		}
		f << "dim=" << Box2i(Vec2i(dim[0],dim[1]),Vec2i(dim[2],dim[3])) << ",";
		for (int i=0; i<8; ++i)   // small value or 100
		{
			val=(int) libwps::read8(input);
			if (val) f << "f" << i+1 << "=" << val << ",";
		}
		isParsed=needWriteInAscii=true;
		int remain=int(sz-26);
		if (remain<=1) break;
		std::string name("");
		for (int i=0; i<remain; ++i)
			name+=(char) libwps::readU8(input);
		f << name << ",";
		break;
	}
	case 0x7d3:
	{
		f.str("");
		f << "Entries(WindowsMacSplit):";
		if (sz<24)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the windows split seems bad\n"));
			f << "###";
			break;
		}
		val=(int) libwps::readU8(input);
		if (val) f << "id=" << val << ",";
		val=(int) libwps::readU8(input);
		if (val) f << "split[id]=" << val << ",";
		for (int i=0; i<3; ++i)   // 0 or 1
		{
			val=(int) libwps::read8(input);
			if (val) f << "f" << i+1 << "=" << val << ",";
		}
		int dim[4];
		for (int i=0; i<4; ++i)
		{
			val=(int) libwps::read16(input);
			dim[i]=(int) libwps::read16(input);
			if (val) f << "dim" << i <<"[h]=" << val << ",";
		}
		f << "dim=" << Box2i(Vec2i(dim[0],dim[1]),Vec2i(dim[2],dim[3])) << ",";
		for (int i=0; i<3; ++i)
		{
			static int const expected[]= {0,-1,25};
			val=(int) libwps::read8(input);
			if (val!=expected[i]) f << "g" << i << "=" << val << ",";
		}
		isParsed=needWriteInAscii=true;
		break;
	}
	case 0x7d4:
	{
		f.str("");
		f << "Entries(WindowsMacUnkn0)";
		if (sz<5)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the windows unkn0 seems bad\n"));
			f << "###";
			break;
		}
		for (int i=0; i<4; ++i)   // always 2,1,1,2 ?
		{
			val=(int) libwps::read8(input);
			if (val) f << "f" << i << "=" << val << ",";
		}
		isParsed=needWriteInAscii=true;
		int remain=int(sz-4);
		if (remain<=1) break;
		std::string name("");
		for (int i=0; i<remain; ++i) // always LMBCS 1.2?
			name+=(char) libwps::readU8(input);
		f << name << ",";
		break;
	}
	case 0x7d5: // frequently followed by Lotus13 block and SheetRow, ...
		f.str("");
		f << "Entries(SheetBegin):";
		if (sz!=11)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the sheet begin zone seems bad\n"));
			f << "###";
			break;
		}
		// time to update the style manager state
		m_styleManager->updateState();

		val=(int) libwps::readU8(input);
		if (val) f << "sheet[id]=" << val << ",";
		// then always 0a3fff00ffff508451ff ?
		isParsed=needWriteInAscii=true;
		break;
	case 0x7d7:
		isParsed=m_spreadsheetParser->readRowSizes(stream, endPos);
		break;
	case 0x7d8:
	case 0x7d9:
	{
		f.str("");
		int dataSz=type==0x7d8 ? 1 : 2;
		if (type==0x7d8)
			f << "Entries(ColMacBreak):";
		else
			f << "Entries(RowMacBreak):";

		if (sz<4 || (sz%dataSz))
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the page mac break seems bad\n"));
			f << "###";
			break;
		}
		val=(int) libwps::readU8(input);
		if (val) f << "sheet[id]=" << val << ",";
		val=(int) libwps::readU8(input); // always 0
		if (val) f << "f0=" << val << ",";
		f << "break=[";
		int N=int((sz-2)/dataSz);
		for (int i=0; i<N; ++i)
		{
			if (dataSz==1)
				f << (int) libwps::readU8(input) << ",";
			else
				f << libwps::readU16(input) << ",";
		}
		f << "],";
		isParsed=needWriteInAscii=true;
		break;
	}

	//
	// selection
	//
	case 0xbb8:
		f.str("");
		f << "Entries(MacSelect):";

		if (sz!=18)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the mac selection seems bad\n"));
			f << "###";
			break;
		}
		for (int i=0; i<3; ++i)   // f0=0, f1=f2=1
		{
			val=(int) libwps::read16(input);
			if (val) f << "f" << i << "=" << val << ",";
		}
		for (int i=0; i<3; ++i)
		{
			int row=(int) libwps::readU16(input);
			int sheet=(int) libwps::readU8(input);
			int col=(int) libwps::readU8(input);
			f << "C" << col << "-" << row;
			if (sheet) f << "[" << sheet << "],";
			else f << ",";
		}
		isParsed=needWriteInAscii=true;
		break;

	//
	// style
	//
	case 0xfa0: // wk3
		isParsed=m_styleManager->readFontStyleA0(stream, endPos);
		break;
	case 0xfa1: // wk6-wk9, with size 26
		f.str("");
		f << "Entries(FontStyle):";
		break;
	case 0xfaa: // 10Style
	case 0xfab:
		isParsed=m_styleManager->readLineStyle(stream, endPos, type==0xfaa ? 0 : 1);
		break;
	case 0xfb4: // 20 Style
		isParsed=m_styleManager->readColorStyle(stream, endPos);
		break;
	case 0xfbe: // 30Style
		isParsed=m_styleManager->readFormatStyle(stream, endPos);
		break;
	case 0xfc8: // 40Style
		isParsed=m_styleManager->readGraphicStyle(stream, endPos);
		break;
	case 0xfc9: // with size 33
		f.str("");
		f << "Entries(GraphicStyle):";
		break;
	case 0xfd2: // 50Style
		isParsed=m_styleManager->readCellStyleD2(stream, endPos);
		break;
	case 0xfdc:
		isParsed=readMacFontName(stream, endPos);
		break;
	case 0xfe6: // wk5
		if (version()!=3) break;
		isParsed=m_styleManager->readCellStyleE6(stream, endPos);
		break;
	case 0xff0: // wk5
		isParsed=m_styleManager->readFontStyleF0(stream, endPos);
		break;

	// 0xfe6: X X CeId : 60Style

	//
	// graphic
	//

	case 0x2328:
		isParsed=m_graphParser->readZoneBegin(stream, endPos);
		break;
	case 0x2332: // line
	case 0x2346: // rect, rectoval, rect
	case 0x2350: // arac
	case 0x2352: // rect shadow
	case 0x23f0: // frame
		isParsed=m_graphParser->readZoneData(stream, endPos, type);
		break;
	case 0x23fa: // textbox data
		isParsed=m_graphParser->readTextBoxData(stream, endPos);
		break;

	//
	// mac pict
	//
	case 0x240e:
		isParsed=m_graphParser->readPictureDefinition(stream, endPos);
		break;
	case 0x2410:
		isParsed=m_graphParser->readPictureData(stream, endPos);
		break;

	//
	// mac printer
	//
	case 0x2af8:
		isParsed=readDocumentInfoMac(stream, endPos);
		break;
	case 0x2afa:
		f.str("");
		f << "Entries(PrinterMacUnkn1):";
		if (sz!=3)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the printer unkn1 seems bad\n"));
			f << "###";
			break;
		}
		for (int i=0; i<3; ++i)
		{
			val=(int) libwps::readU8(input);
			static int const expected[]= {0x1f, 0xe0, 0/*or 1*/};
			if (val!=expected[i])
				f << "f" << i << "=" << val << ",";
		}
		isParsed=needWriteInAscii=true;
		break;
	case 0x2afb:
	{
		f.str("");
		f << "Entries(PrinterMacName):";
		if (sz<3)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the printername seems bad\n"));
			f << "###";
			break;
		}
		val=(int) libwps::read16(input);
		if (val!=20) f << "f0=" << val << ",";
		std::string name("");
		for (int i=4; i<sz; ++i)
		{
			char c=(char) libwps::readU8(input);
			if (!c) break;
			name+=c;
		}
		f << name << ",";
		isParsed=needWriteInAscii=true;
		break;
	}
	case 0x2afc:
		f.str("");
		f << "Entries(PrintMacInfo):";
		if (sz<120)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the printinfo seems bad\n"));
			f << "###";
			break;
		}
		isParsed=needWriteInAscii=true;
		break;

	case 0x32e7:
		isParsed=m_styleManager->readMenuStyleE7(stream, endPos);
		break;

	// 32e7: related to style ?
	case 0x36b0:
		isParsed=m_spreadsheetParser->readSheetName1B(stream, endPos);
		break;

	//
	// 4268, 4269
	//

	case 0x4a38:
		f.str("");
		f << "Entries(LinkUnkA):";
		break;
	case 0x4a39:
		f.str("");
		f << "Entries(LinkUnkB):";
		break;
	case 0x6590:
		isParsed=m_spreadsheetParser->readNote(stream, endPos);
		break;

	default:
		break;
	}
	if (!isParsed || needWriteInAscii)
	{
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
	}
	if (input->tell()!=endPos)
		ascFile.addDelimiter(input->tell(),'|');
	input->seek(endPos, librevenge::RVNG_SEEK_SET);
	return true;
}

bool LotusParser::readZoneV3(shared_ptr<WPSStream> stream)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	int type = (int) libwps::readU16(input);
	long sz = (long) libwps::readU16(input);
	long endPos=pos+4+sz;
	if (sz<0 || !stream->checkFilePosition(endPos))
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	f << "Entries(Data" << std::hex << type << std::dec << "N):";

	switch (type)
	{
	// level 1=table, 2=col, 3=row
	case 0x106:
		f.str("");
		f << "Entries(Level)[open]:";
		m_state->m_actualLevels.push_back(Vec2i(0,0));
		f << "[" << m_state->getLevelsDebugName() << "],";
		break;
	case 0x107:
		f.str("");
		f << "Entries(Level)[close]:";
		if (m_state->m_actualLevels.empty())
		{
			WPS_DEBUG_MSG(("LotusParser::readZoneV3: the level seems bad\n"));
			f << "###";
			break;
		}
		else
			m_state->m_actualLevels.pop_back();
		f << "[" << m_state->getLevelsDebugName() << "],";
		break;
	case 0x800:
	{
		f.str("");
		f << "Entries(Level)[select]:";
		if (sz<2)
		{
			WPS_DEBUG_MSG(("LotusParser::readDataZone: the level size seems bad\n"));
			f << "###";
			break;
		}
		if (m_state->m_actualLevels.empty())
		{
			WPS_DEBUG_MSG(("LotusParser::readZoneV3: the level seems bad\n"));
			f << "###";
			break;
		}
		long count=(int)(sz>=4 ? libwps::readU32(input) : libwps::readU16(input));
		Vec2i &zone=m_state->m_actualLevels.back();
		if ((int)(zone[1]+count)<0)
		{
			WPS_DEBUG_MSG(("LotusParser::readZoneV3: arg the delta bad\n"));
			f << "###delta=" << count << ",";
			count=0;
		}
		zone[0] = zone[1];
		zone[1] += int(count);
		f << "[" << m_state->getLevelsDebugName() << "],";
		break;
	}
	case 0x801:
		f.str("");
		f << "Entries(SetStyle)[" << m_state->getLevelsDebugName() << "]:";
		break;
	case 0x804:
		f.str("");
		f << "Entries(FormatUnkn0):";
		break;
	default:
		break;
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	if (input->tell()!=endPos && input->tell()!=pos)
		ascFile.addDelimiter(input->tell(),'|');
	input->seek(endPos, librevenge::RVNG_SEEK_SET);
	return true;
}

////////////////////////////////////////////////////////////
//   generic
////////////////////////////////////////////////////////////
bool LotusParser::readMacFontName(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	const int vers=version();
	long pos = input->tell();
	long sz=endPos-pos;
	f << "Entries(MacFontName):";
	if ((vers<=1 && sz<7) || (vers>1 && sz!=42))
	{
		WPS_DEBUG_MSG(("LotusParser::readMacFontName: the zone size seems bad\n"));
		f << "###";
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	if (vers<=1)
	{
		// seems only to exist in a lotus mac file, so revert the default encoding to MacRoman if undef
		if (m_state->m_fontType==libwps_tools_win::Font::UNKNOWN)
			m_state->m_fontType=libwps_tools_win::Font::MAC_ROMAN;

		int id=(int) libwps::readU16(input);
		f << "FN" << id << ",";
		int val=(int) libwps::readU16(input); // always 0?
		if (val)
			f << "f0=" << val << ",";
		val=(int) libwps::read16(input); // find -1, 30 (Geneva), 60 (Helvetica)
		if (val)
			f << "f1=" << val << ",";
		librevenge::RVNGString name("");
		bool nameOk=true;
		for (int i=0; i<sz-6; ++i)
		{
			char c=(char) libwps::readU8(input);
			if (!c) break;
			if (nameOk && !(c==' ' || (c>='0'&&c<='9') || (c>='a'&&c<='z') || (c>='A'&&c<='Z')))
			{
				nameOk=false;
				WPS_DEBUG_MSG(("LotusParser::readMacFontName: find odd character in name\n"));
				f << "#";
			}
			name.append(c);
		}
		f << name.cstr() << ",";
		if (m_state->m_fontsMap.find(id)!=m_state->m_fontsMap.end())
		{
			WPS_DEBUG_MSG(("LotusParser::readMacFontName: a font with id=%d already exists\n", id));
			f << "###id,";
		}
		else if (nameOk && !name.empty())
		{
			libwps_tools_win::Font::Type encoding=name!="Symbol" ?
			                                      libwps_tools_win::Font::MAC_ROMAN : libwps_tools_win::Font::MAC_SYMBOL;
			LotusParserInternal::Font font(encoding);
			font.m_name=name;
			m_state->m_fontsMap.insert(std::map<int, LotusParserInternal::Font>::value_type(id,font));
		}
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());
		return true;
	}

	for (int i=0; i<4; ++i)
	{
		int val=(int) libwps::read8(input); // 0|1
		if (val)
			f << "fl" << i << "=" << val << ",";
	}
	for (int i=0; i<2; ++i)   // f1=0|1288
	{
		int val=(int) libwps::read16(input);
		if (val)
			f << "f" << i << "=" << val << ",";
	}
	std::string name("");
	for (int i=0; i<8; ++i)
	{
		char c=(char) libwps::read8(input);
		if (!c) break;
		name+=c;
	}
	f << name << ",";
	input->seek(pos+16, librevenge::RVNG_SEEK_SET);
	if (input->tell()!=endPos)
	{
		ascFile.addDelimiter(input->tell(),'|');
		input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusParser::readFMTStyleName(shared_ptr<WPSStream> stream)
{
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	int type = (int) libwps::read16(input);
	if (type!=0xb6)
	{
		WPS_DEBUG_MSG(("LotusParser::readFMTStyleName: not a font name definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	long endPos=pos+4+sz;
	if (sz<8)
	{
		WPS_DEBUG_MSG(("LotusParser::readFMTStyleName: the zone size seems bad\n"));
		ascFile.addPos(pos);
		ascFile.addNote("Entries(FMTStyleName):###");
		return true;
	}
	f << "Entries(FMTStyleName):";
	f << "id=" << libwps::readU16(input) << ",";
	std::string name;
	for (int i=0; i<6; ++i)
	{
		char c=char(libwps::readU8(input));
		if (c==0) break;
		name+= c;
	}
	f << "title=" << name << ",";
	input->seek(pos+12, librevenge::RVNG_SEEK_SET);
	name.clear();
	for (int i=0; i<sz-8; ++i) name+= char(libwps::readU8(input));
	f << name << ",";
	if (input->tell()!=endPos)
	{
		WPS_DEBUG_MSG(("LotusParser::readFMTStyleName: find extra data\n"));
		f << "###extra";
		input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusParser::readLinkZone(shared_ptr<WPSStream> stream)
{
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	int type = (int) libwps::read16(input);
	if (type!=0xa)
	{
		WPS_DEBUG_MSG(("LotusParser::readLinkZone: not a link definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	f << "Entries(Link):";
	if (sz < 19)
	{
		WPS_DEBUG_MSG(("LotusParser::readLinkZone: the zone is too short\n"));
		f << "###";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	type=(int) libwps::read16(input);
	if (type==0) // fixme: find if this is a note, so that we can retrieve it
		f << "chart/note/...,";
	else if (type==1)
		f << "file,";
	else
	{
		WPS_DEBUG_MSG(("LotusParser::readLinkZone: find unknown type\n"));
		f << "##type=" << type << ",";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	f << "ID=" << int(libwps::readU8(input)) << ","; // 0,19,42,53,ff
	f << "id=" << (int) libwps::readU8(input) << ",";
	std::string name("");
	for (int i=0; i<14; ++i)
	{
		char c=(char) libwps::readU8(input);
		if (!c) break;
		name += c;
	}
	f << "\"" << name << "\",";
	input->seek(pos+4+18, librevenge::RVNG_SEEK_SET);
	switch (type)
	{
	case 0:
		if (sz<26)
		{
			WPS_DEBUG_MSG(("LotusParser::readLinkZone: the chart zone seems too short\n"));
			f << "###";
			break;
		}
		for (int i=0; i<2; ++i)
		{
			int row=(int) libwps::readU16(input);
			int table=(int) libwps::readU8(input);
			int col=(int) libwps::readU8(input);
			f << "C" << col << "-" << row;
			if (table) f << "[" << table << "]";
			if (i==0)
				f << "<->";
			else
				f << ",";
		}
		break;
	case 1:
		name="";
		for (int i=18; i<sz; ++i)
		{
			char c=(char) libwps::readU8(input);
			if (!c) break;
			name += c;
		}
		f << "link=" << name << ",";
		break;
	default:
		WPS_DEBUG_MSG(("LotusParser::readLinkZone: find unknown type\n"));
		f << "###";
		break;
	}
	if (input->tell()!=pos+4+sz && input->tell()+1!=pos+4+sz)
	{
		WPS_DEBUG_MSG(("LotusParser::readLinkZone: the zone seems too short\n"));
		f << "##";
		ascFile.addDelimiter(input->tell(), '|');
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

// ----------------------------------------------------------------------
// Header/Footer/PageDim
// ----------------------------------------------------------------------
bool LotusParser::readDocumentInfoMac(shared_ptr<WPSStream> stream, long endPos)
{
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	f << "Entries(DocMacInfo):";
	if (endPos-pos!=51)
	{
		WPS_DEBUG_MSG(("LotusParser::readDocumentInfoMac: the zone size seems bad\n"));
		f << "###";
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	int dim[7];
	for (int i=0; i<7; ++i)
	{
		int val=(int) libwps::read8(input);
		if (i==0)
			f << "dim[unkn]=";
		else if (i==1)
			f << "margins=[";
		else if (i==5)
			f << "pagesize=[";
		dim[i]=(int) libwps::read16(input);
		f << dim[i];
		if (val) f << "[" << val << "]";
		val=(int) libwps::read8(input); // always 0
		if (val) f << "[" << val << "]";
		f << ",";
		if (i==4 || i==6) f << "],";
	}
	// check order
	if (dim[5]>dim[1]+dim[3] && dim[6]>dim[2]+dim[4])
	{
		m_state->m_pageSpan.setFormWidth(dim[5]);
		m_state->m_pageSpan.setFormLength(dim[6]);
		m_state->m_pageSpan.setMarginLeft(dim[1]);
		m_state->m_pageSpan.setMarginTop(dim[2]);
		m_state->m_pageSpan.setMarginRight(dim[3]);
		m_state->m_pageSpan.setMarginBottom(dim[4]);
	}
	else
		f << "###";
	f << "unkn=[";
	for (int i=0; i<5; ++i)   // 1,1,1,100|inf,1
	{
		int val=(int) libwps::read16(input);
		if (val==9999)
			f << "inf,";
		else if (val)
			f << val << ",";
		else
			f << "_,";
	}
	f << "],";
	for (int i=0; i<13; ++i)   // always 0?
	{
		int val=(int) libwps::read8(input);
		if (val)
			f << "g" << i << "=" << val << ",";
	}
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
//   chart
////////////////////////////////////////////////////////////

bool LotusParser::readChartDefinition(shared_ptr<WPSStream> stream)
{
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 0x11)
	{
		WPS_DEBUG_MSG(("LotusParser::readChartDefinition: not a chart name\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	long endPos=pos+4+sz;
	f << "Entries(ChartDef):";
	if (sz < 0xB2) // find b2 or b3
	{
		WPS_DEBUG_MSG(("LotusParser::readChartDefinition: chart name is too short\n"));
		f << "###";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	f << "id=" << (int) libwps::readU8(input) << ",";
	std::string name("");
	for (int i=0; i<16; ++i)
	{
		char c=(char) libwps::readU8(input);
		if (!c) break;
		name += c;
	}
	if (!name.empty()) f << name << ",";
	input->seek(pos+4+17, librevenge::RVNG_SEEK_SET);
	for (int i=0; i<43; ++i)   // small number
	{
		int val=(int) libwps::read8(input);
		if (val) f << "f" << i << "=" << val << ",";
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());

	pos=input->tell();
	f.str("");
	f << "ChartDef-A:";
	for (int i=0; i<28; ++i)   // small number expect f24=0|4|64
	{
		int val=(int) libwps::read8(input);
		if (val) f << "f" << i << "=" << val << ",";
	}
	for (int i=0; i<9; ++i)   // small number expect g0=1|2, g1=g2=g3=1|14|20, g4=g5=0|1
	{
		int val=(int) libwps::read16(input);
		if (val) f << "g" << i << "=" << val << ",";
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());

	pos=input->tell();
	f.str("");
	f << "ChartDef-B:";
	if (input->tell()!=endPos)
		input->seek(endPos, librevenge::RVNG_SEEK_SET);
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusParser::readChartName(shared_ptr<WPSStream> stream)
{
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	long type = (long) libwps::read16(input);
	if (type != 0x12)
	{
		WPS_DEBUG_MSG(("LotusParser::readChartName: not a chart name\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	f << "Entries(ChartName):";
	if (sz < 3)
	{
		WPS_DEBUG_MSG(("LotusParser::readChartName: chart name is too short\n"));
		f << "###";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}

	int val=(int) libwps::readU8(input);
	f << "chart[id]=" << val << ",";
	int id=(int) libwps::readU8(input);
	f << "data[id]=" << id << ",";
	std::string name("");
	for (int i = 0; i < sz-2; i++)
	{
		char c = (char) libwps::readU8(input);
		if (c == '\0') break;
		name += c;
	}
	f << name << ",";
	if (input->tell()!=pos+4+sz && input->tell()+1!=pos+4+sz)
	{
		WPS_DEBUG_MSG(("LotusParser::readChartName: the zone seems too short\n"));
		f << "##";
		ascFile.addDelimiter(input->tell(), '|');
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
//   Unknown
////////////////////////////////////////////////////////////


/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
