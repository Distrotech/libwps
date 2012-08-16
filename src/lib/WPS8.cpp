/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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
 */

#include <stdlib.h>
#include <string.h>

#include <libwpd-stream/WPXStream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSContentListener.h"
#include "WPSEntry.h"
#include "WPSHeader.h"
#include "WPSOLEParser.h"
#include "WPSOLEStream.h"
#include "WPSPageSpan.h"
#include "WPSPosition.h"
#include "WPSSubDocument.h"

#include "WPS8Graph.h"
#include "WPS8Struct.h"
#include "WPS8Table.h"
#include "WPS8Text.h"

#include "WPS8.h"

namespace WPS8ParserInternal
{
//! Internal: the subdocument of a WPS8Parser
class SubDocument : public WPSSubDocument
{
public:
	//! type of an entry
	enum Type { Unknown, TEXT };
	//! constructor for a text entry
	SubDocument(WPXInputStreamPtr input, WPS8Parser &pars, WPSEntry const &entry) :
		WPSSubDocument (input, &pars), m_entry(entry) {}
	//! destructor
	~SubDocument() {}

	//! operator==
	virtual bool operator==(shared_ptr<WPSSubDocument> const &doc) const
	{
		if ( !doc || !WPSSubDocument::operator==(doc))
			return false;
		SubDocument const *sDoc = dynamic_cast<SubDocument const *>(doc.get());
		if (!sDoc) return false;
		return m_entry == sDoc->m_entry;
	}

	//! the parser function
	void parse(shared_ptr<WPSContentListener> &listener, libwps::SubDocumentType subDocumentType);
	//! the entry
	WPSEntry m_entry;
};

void SubDocument::parse(shared_ptr<WPSContentListener> &listener, libwps::SubDocumentType subDocumentType)
{
	if (!listener.get())
	{
		WPS_DEBUG_MSG(("WPS8ParserInternal::SubDocument::parse: no listener\n"));
		return;
	}
	if (!dynamic_cast<WPS8ContentListener *>(listener.get()))
	{
		WPS_DEBUG_MSG(("WPS8ParserInternal::SubDocument::parse: bad listener\n"));
		return;
	}

	WPS8ContentListenerPtr &listen =  reinterpret_cast<WPS8ContentListenerPtr &>(listener);
	if (!m_parser)
	{
		listen->insertCharacter(' ');
		WPS_DEBUG_MSG(("WPS8ParserInternal::SubDocument::parse: bad parser\n"));
		return;
	}

	if (m_entry.isParsed() && subDocumentType != libwps::DOC_HEADER_FOOTER)
	{
		listen->insertCharacter(' ');
		WPS_DEBUG_MSG(("WPS8ParserInternal::SubDocument::parse: this zone is already parsed\n"));
		return;
	}
	m_entry.setParsed(true);
	if (m_entry.type() != "Text")
	{
		listen->insertCharacter(' ');
		WPS_DEBUG_MSG(("WPS8ParserInternal::SubDocument::parse: send not Text entry is not implemented\n"));
		return;
	}

	if (!m_entry.valid())
	{
		WPS_DEBUG_MSG(("SubDocument::parse: empty document found...\n"));
		listen->insertCharacter(' ');
		return;
	}


	WPS8Parser *mnParser = reinterpret_cast<WPS8Parser *>(m_parser);
	mnParser->send(m_entry);
}

/** Internal: a frame, a zone which can contain text, picture, ... and have some borders */
struct Frame
{
	//! constructor
	Frame() : m_parsed(false), m_type(UNKNOWN), m_typeFlag(0), m_pos(),
		m_idStrs(-1), m_idObject(-1), m_idTable(-1), m_idOle(-1), m_columns(1), m_idBorder(),
		m_backgroundColor(0xFFFFFF), m_error("")
	{
		m_pos.setRelativePosition(WPSPosition::Page);
		m_pos.setPage(1);
	}

	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, Frame const &ft);

	/** The frame type
	 *
	 * - Header/Footer: the header and footer frame
	 * - Table : a table
	 * - Object : a picture, ...
	 * - Text : a text zone */
	enum { UNKNOWN = 0, Header, Footer, Table, Object, Text };

	//! a flag to know if the frame is already sent to the listener
	mutable bool m_parsed;
	//! the frame type
	int m_type;
	//! some unknown flags which are defined near the frame type
	int m_typeFlag;
	//! the frame position
	WPSPosition m_pos;
	int m_idStrs /** identifier corresponding to a text zone (STRS)*/,
	    m_idObject /** identifier corresponding to an object zone (EOBJ)*/,
	    m_idTable /** identifier corresponding to a table (MCLD)*/,
	    m_idOle /** identifier corresponding to an ole*/;
	//! the number of columns for a textbox, ...
	int m_columns;
	//! the border: an entry to some complex border (if sets)
	WPSEntry m_idBorder;
	//! the border's color
	uint32_t m_backgroundColor;
	//! a string used to store the parsing errors
	std::string m_error;
};

//! operator<< for a Frame
std::ostream &operator<<(std::ostream &o, Frame const &ft)
{
	switch(ft.m_type)
	{
	case Frame::Header:
		o<< "header";
		break;
	case Frame::Footer:
		o<< "footer";
		break;
	case Frame::Table:
		o<< "table";
		break;
	case Frame::Text:
		o<< "textbox";
		break;
	case Frame::Object:
		o<< "object";
		break;
	default:
		o << "###type=unknown";
		break;
	}
	o << "(";
	if (ft.m_idStrs >= 0) o << "STRS" << ft.m_idStrs << ",";
	if (ft.m_idObject >= 0) o << "EOBJ" << ft.m_idObject << ",";
	if (ft.m_idTable >= 0) o << "MCLD/Table" << ft.m_idTable << ",";
	if (ft.m_idOle >= 0) o << "oleId=" << ft.m_idOle << ",";
	if (ft.m_typeFlag) o << std::hex << "flags=" << ft.m_typeFlag << std::dec;
	o << "),";

	o << ft.m_pos << ",";
	switch(ft.m_pos.page())
	{
	case -1:
		o << "allpages,";
		break;
	case -2:
		break; // undef
	default:
		if (ft.m_pos.page() < 0) o << "###page=" << ft.m_pos.page() << ",";
	}

	if (ft.m_columns != 1) o << ft.m_columns << "columns,";
	if (ft.m_idBorder.valid())
		o << "border='" << ft.m_idBorder.name() << "':" << ft.m_idBorder.id() << ",";
	if (ft.m_backgroundColor != 0xFFFFFF)
		o << "backColor=" << std::hex << ft.m_backgroundColor << std::dec << ",";

	if (!ft.m_error.empty()) o << "errors=(" << ft.m_error << ")";
	return o;
}

//! the state of WPS8
struct State
{
	State() : m_eof(-1), m_pageSpan(), m_localeLanguage(""), m_background(),
		m_frameList(), m_object2FrameMap(), m_table2FrameMap(),
		m_docPropertyTypes(), m_frameTypes(),
		m_numColumns(1), m_actPage(0), m_numPages(0)
	{
		initTypeMaps();
	}
	//! initializes the type map
	void initTypeMaps();
	//! the end of file
	long m_eof;
	//! the actual document size
	WPSPageSpan m_pageSpan;
	//! the language
	std::string m_localeLanguage;
	//! an identificator to design a background picture
	WPSEntry m_background;

	//! the frame's list
	std::vector<Frame> m_frameList;
	//! a map m_idObject -> frame
	std::map<int, int> m_object2FrameMap;
	//! a map m_idTable -> frame
	std::map<int, int> m_table2FrameMap;

	//! the document property type
	std::map<int,int> m_docPropertyTypes;
	//! the frame type
	std::map<int,int> m_frameTypes;
	int m_numColumns /** the number of columns */;
	int m_actPage /** the actual page*/, m_numPages /* the number of pages */;
};

void State::initTypeMaps()
{
	static int const docTypes[] =
	{
		0, 0x22, 1, 0x22, 2, 0x22, 3, 0x22, 4, 0x22, 5, 0x22, 6, 0x22, 7, 0x22,
		8, 0x1a, 0xa, 0x2,
		0x13, 0x2a, 0x15, 0x2a,
		0x18, 0x12, 0x19, 0x2, 0x1b, 0x12, 0x1c, 0x22, 0x1e, 0x22,
		0x26, 0x22, 0x27, 0x22,
		0x28, 0x22, 0x29, 0x22, 0x2a, 0x22, 0x2b, 0x22, 0x2c, 0x82, 0x2d, 0x82, 0x2e, 0x82
	};
	for (int i = 0; i+1 < int(sizeof(docTypes)/sizeof(int)); i+=2)
		m_docPropertyTypes[docTypes[i]] = docTypes[i+1];
	static int const frameTypes[] =
	{
		0, 0x1a, 1, 0x12, 2, 0x12, 3, 0x2, 4, 0x22, 5, 0x22, 6, 0x22, 7, 0x22,
		8, 0x22, 9, 0x22, 0xa, 0x22,
		0x10, 0x2a, 0x11, 0x82, 0x13, 0x12, 0x14, 0x12, 0x17, 0x2,
		0x18, 0x22, 0x19, 0x22, 0x1a, 0x12, 0x1b, 0x22, 0x1d, 0x22, 0x1e, 0x22, 0x1f, 0x22,
		0x20, 0x22, 0x26, 0x22,
		0x2a, 0x22, 0x2c, 0x1a, 0x2d, 0x1a, 0x2e, 0x22, 0x2f, 0x2,
		0x30, 0x22
	};
	for (int i = 0; i+1 < int(sizeof(frameTypes)/sizeof(int)); i+=2)
		m_frameTypes[frameTypes[i]] = frameTypes[i+1];

}
}

// constructor, destructor
WPS8Parser::WPS8Parser(WPXInputStreamPtr &input, WPSHeaderPtr &header) :
	WPSParser(input, header),
	m_listener(), m_graphParser(), m_tableParser(), m_textParser(), m_state()
{
	if (version()<5) setVersion(5);
	m_state.reset(new WPS8ParserInternal::State);
	m_graphParser.reset(new WPS8Graph(*this));
	m_tableParser.reset(new WPS8Table(*this));
	m_textParser.reset(new WPS8Text(*this));
}

WPS8Parser::~WPS8Parser ()
{
}

// small funtion ( dimension, ...)
float WPS8Parser::pageHeight() const
{
	return float(m_state->m_pageSpan.getFormLength()-m_state->m_pageSpan.getMarginTop()-m_state->m_pageSpan.getMarginBottom());
}

float WPS8Parser::pageWidth() const
{
	return float(m_state->m_pageSpan.getFormWidth()-m_state->m_pageSpan.getMarginLeft()-m_state->m_pageSpan.getMarginRight());
}

int WPS8Parser::numColumns() const
{
	return m_state->m_numColumns;
}

bool WPS8Parser::checkInFile(long pos)
{
	if (pos <= m_state->m_eof)
		return true;
	WPXInputStreamPtr input = getInput();
	long actPos = input->tell();
	input->seek(pos, WPX_SEEK_SET);
	bool ok = input->tell() == pos;
	if (ok) m_state->m_eof = pos;
	input->seek(actPos, WPX_SEEK_SET);
	return ok;
}

// listener, new page
void WPS8Parser::setListener(shared_ptr<WPS8ContentListener> listener)
{
	m_listener = listener;
	m_graphParser->setListener(m_listener);
	m_tableParser->setListener(m_listener);
	m_textParser->setListener(m_listener);
}

shared_ptr<WPS8ContentListener> WPS8Parser::createListener(WPXDocumentInterface *interface)
{
	std::vector<WPSPageSpan> pageList;
	WPSPageSpan ps(m_state->m_pageSpan);

	int numPages = 1;
	int textPages = m_textParser->numPages();
	if (textPages > numPages) numPages = textPages;
	int tablePages = m_tableParser->numPages();
	if (tablePages > numPages) numPages = tablePages;
	int graphPages = m_graphParser->numPages();
	if (graphPages>=numPages) numPages = graphPages;

	WPSEntry entry = m_textParser->getHeaderEntry();
	if (entry.valid())
	{
		WPSSubDocumentPtr subdoc(new WPS8ParserInternal::SubDocument
		                         (getInput(), *this, entry));
		ps.setHeaderFooter(WPSPageSpan::HEADER, WPSPageSpan::ALL, subdoc);
	}

	entry = m_textParser->getFooterEntry();
	if (entry.valid())
	{
		WPSSubDocumentPtr subdoc(new WPS8ParserInternal::SubDocument
		                         (getInput(), *this, entry));
		ps.setHeaderFooter(WPSPageSpan::FOOTER, WPSPageSpan::ALL, subdoc);
	}
#ifdef DEBUG
	// create all the pages + an empty page, if we have some remaining data...
	numPages++;
#endif

	pageList.push_back(ps);
	for (int i = 1; i < numPages; i++) pageList.push_back(ps);
	m_state->m_numPages=numPages;
	return shared_ptr<WPS8ContentListener>
	       (new WPS8ContentListener(pageList, interface));
}

void WPS8Parser::newPage(int number)
{
	if (number <= m_state->m_actPage || number > m_state->m_numPages)
		return;

	while (m_state->m_actPage < number)
	{
		m_state->m_actPage++;

		if (m_listener.get() == 0L || m_state->m_actPage == 1)
			continue;
		m_listener->insertBreak(WPS_PAGE_BREAK);
		m_graphParser->sendObjects(m_state->m_actPage);
	}
}

////////////////////////////////////////////////////////////
// interface with the graph/text parser
void WPS8Parser::send(WPSEntry const &entry)
{
	WPXInputStreamPtr input = getInput();
	long actPos = input->tell();
	m_textParser->readText(entry);
	input->seek(actPos, WPX_SEEK_SET);
}

void WPS8Parser::sendTextInCell(int strsId, int cellId)
{
	WPXInputStreamPtr input = getInput();
	long actPos = input->tell();
	m_textParser->readTextInCell(strsId,cellId);
	input->seek(actPos, WPX_SEEK_SET);
}

bool WPS8Parser::sendTable(Vec2f const &size, int objectId)
{
	std::map<int, int>::iterator pos = m_state->m_object2FrameMap.find(objectId);
	if (pos == m_state->m_object2FrameMap.end())
	{
		WPS_DEBUG_MSG(("WPS8Parser::sendTable can not find the table %d \n", objectId));
		return false;
	}

	WPS8ParserInternal::Frame const &frame = m_state->m_frameList[(size_t)pos->second];
	if (frame.m_idStrs < 0)
	{
		WPS_DEBUG_MSG(("WPS8Parser:sendTable can not find the text zone \n"));
		return false;
	}
	if (frame.m_idTable < 0)
	{
		WPS_DEBUG_MSG(("WPS8Parser:sendTable can not find the table zone \n"));

#if 0
		// FIXME
		WPSPosition position(Vec2f(), size);
		position.m_anchorTo = WPSPosition::CharBaseLine; // CHECKME
		position.m_wrapping = WPSPosition::WDynamic;
		sendTextBox(position, frame.m_idStrs);
		return true;
#endif
		return false;
	}
	frame.m_parsed = true;
	return m_tableParser->sendTable(size, frame.m_idTable, frame.m_idStrs);
}

int WPS8Parser::getTableSTRSId(int tableId) const
{
	std::map<int, int>::iterator pos = m_state->m_table2FrameMap.find(tableId);
	// probably ok: checkme
	if (pos == m_state->m_table2FrameMap.end())
		return -1;
	WPS8ParserInternal::Frame const &frame = m_state->m_frameList[(size_t)pos->second];
	if (frame.m_idStrs < 0)
	{
		WPS_DEBUG_MSG(("WPS8Parser:sendTable can not find the text zone \n"));
	}
	return frame.m_idStrs;
}

////////////////////////////////////////////////////////////
// main function to parse the document
void WPS8Parser::parse(WPXDocumentInterface *documentInterface)
{
	WPXInputStreamPtr input=getInput();
	if (!input)
	{
		WPS_DEBUG_MSG(("WPS8Parser::parse: does not find main ole\n"));
		throw(libwps::ParseException());
	}

	// fixme: actually the pictures are not send to listener, so no need to parse OLEstructures
#ifdef DEBUG
	try
	{
		createOLEStructures();
	}
	catch (...)
	{
		WPS_DEBUG_MSG(("WPS8Parser::parse: exception catched when parsing secondary OLEs\n"));
	}
#endif

	ascii().setStream(input);
	ascii().open("CONTENTS");
	try
	{
		if (!createStructures()) throw(libwps::ParseException());
	}
	catch (...)
	{
		WPS_DEBUG_MSG(("WPS8Parser::parse: exception catched when parsing MN0\n"));
		throw(libwps::ParseException());
	}
	setListener(createListener(documentInterface));
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("WPS8Parser::parse: can not create the listener\n"));
		throw(libwps::ParseException());
	}
	m_listener->startDocument();
	WPSEntry ent = m_textParser->getTextEntry();
	if (ent.valid())
		m_textParser->readText(ent);
	else
	{
		WPS_DEBUG_MSG(("WPS8Parser::parse: can not find main text entry\n"));
		throw(libwps::ParseException());
	}

#ifdef DEBUG
	m_tableParser->flushExtra();
#endif
	m_textParser->flushExtra();
#ifdef DEBUG
	m_graphParser->sendObjects(-1);
#endif

	m_listener->endDocument();
	m_listener.reset();

	ascii().reset();
}

// find and create all the zones ( normal/ole )
bool WPS8Parser::createStructures()
{
	if (!getInput()) return false;

	WPXInputStreamPtr input = getInput();
	parseHeaderIndex();

	// initialize the text, table, ..
	if (!m_textParser->readStructures())
		return false;
	m_graphParser->readStructures(input);
	m_tableParser->readStructures(input);

	NameMultiMap::iterator pos;

	// read DOP zone (document properties)
	pos = getNameEntryMap().lower_bound("DOP ");
	while (getNameEntryMap().end() != pos)
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("DOP ")) break;
		if (!entry.hasType("DOP ")) continue;

		WPSPageSpan page;
		if (readDocProperties(entry, page)) m_state->m_pageSpan = page;
	}

	// printer data
	pos = getNameEntryMap().lower_bound("PRNT");
	while (getNameEntryMap().end() != pos)
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("PRNT")) break;
		if (!entry.hasType("WNPR")) continue;

		readWNPR(entry);
	}

	// SYID
	pos = getNameEntryMap().lower_bound("SYID");
	while (getNameEntryMap().end() != pos)
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("SYID")) break;
		if (!entry.hasType("SYID")) continue;

		std::vector<int> listId;
		readSYID(entry, listId);
	}

	// document title
	pos = getNameEntryMap().find("TITL");
	WPXString title;
	if (getNameEntryMap().end() != pos && pos->second.hasType("TITL"))
	{
		pos->second.setParsed(true);
		input->seek(pos->second.begin(), WPX_SEEK_SET);
		m_textParser->readString(input, pos->second.length(), title);
		ascii().addPos(pos->second.begin());
		ascii().addNote(title.cstr());
	}

	// ok, we can now read the frame
	pos = getNameEntryMap().lower_bound("FRAM");
	while (getNameEntryMap().end() != pos)
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("FRAM")) break;
		if (!entry.hasType("FRAM")) continue;

		readFRAM(entry);
	}
	// creates the correspondance between the eobj and the frame
	size_t numFrames = m_state->m_frameList.size();
	for (size_t i = 0; i < numFrames; i++)
	{
		WPS8ParserInternal::Frame const &frame = m_state->m_frameList[i];
		if (frame.m_idObject < 0) continue;
		m_state->m_object2FrameMap[frame.m_idObject] = (int) i;
		m_state->m_table2FrameMap[frame.m_idTable] = (int) i;
	}


	m_graphParser->computePositions();

	return true;
}

bool WPS8Parser::createOLEStructures()
{
	if (!getInput()) return false;

	shared_ptr<libwps::Storage> storage = getHeader()->getOLEStorage();
	if (!storage) return true;
	WPSOLEParser oleParser("CONTENTS");
	if (!oleParser.parse(storage)) return false;

	m_graphParser->storeObjects(oleParser.getObjects(),oleParser.getObjectsId(),
	                            oleParser.getObjectsPosition());
#ifdef DEBUG
	// there can remain some embedded Works subdocument ( WKS, ... )
	// with name MN0 and some unknown picture ole
	std::vector<std::string> unparsed = oleParser.getNotParse();

	size_t numUnparsed = unparsed.size();

	for (size_t i = 0; i < numUnparsed; i++)
	{
		std::string const &name = unparsed[i];
		if (name == "CONTENTS")
			continue;
		if (name== "SPELLING")
		{
			WPXInputStreamPtr ole = storage->getDocumentOLEStream(name.c_str());
			if (ole && readSPELLING(ole, name))
				continue;
		}
		WPS_DEBUG_MSG(("WPS8Parser::createOLEStructures:: Find unparsed ole: %s\n", name.c_str()));

#ifdef DEBUG_WITH_FILES
		WPXInputStreamPtr ole = storage->getDocumentOLEStream(name.c_str());
		if (!ole.get())
		{
			WPS_DEBUG_MSG(("WPS8Parser::createOLEStructures: error: can find OLE part: \"%s\"\n", name.c_str()));
			continue;
		}

		WPXBinaryData data;
		if (libwps::readDataToEnd(ole,data))
			libwps::Debug::dumpFile(data, name.c_str());
#endif
	}
#endif
	return true;
}

////////////////////////////////////////////////////////////
// read the index
bool WPS8Parser::parseHeaderIndexEntry()
{
	WPXInputStreamPtr input = getInput();
	long pos = input->tell();
	ascii().addPos(pos);

	libwps::DebugStream f;

	uint16_t cch = libwps::readU16(input);

	// check if the entry can be read
	input->seek(pos + cch, WPX_SEEK_SET);
	if (input->tell() != pos+cch)
	{
		WPS_DEBUG_MSG(("WPS8Parser::parseHeaderIndexEntry: error: incomplete entry\n"));
		ascii().addNote("###IndexEntry incomplete (ignored)");
		return false;
	}
	input->seek(pos + 2, WPX_SEEK_SET);

	if (0x18 != cch)
	{
		if (cch < 0x18)
		{
			input->seek(pos + cch, WPX_SEEK_SET);
			ascii().addNote("###IndexEntry too short(ignored)");
			if (cch < 10) throw libwps::ParseException();
			return true;
		}
	}

	std::string name;

	// sanity check
	int i;
	for (i =0; i < 4; i++)
	{
		uint8_t c = libwps::readU8(input);
		name.append(1, char(c));

		if (c != 0 && c != 0x20 && (41 > c || c > 90))
		{
			WPS_DEBUG_MSG(("WPS8Parser::parseHeaderIndexEntry: error: bad character=%u (0x%02x) in name in header index\n", c, c));
			ascii().addNote("###IndexEntry bad name(ignored)");

			input->seek(pos + cch, WPX_SEEK_SET);
			return true;
		}
	}

	f << "Entries("<<name<<")";
	if (cch != 24) f << ", #size=" << int(cch);
	int id = (int) libwps::readU16(input);
	f << ", id=" << id << ", (";
	for (i = 0; i < 2; i ++)
	{
		int val = (int) libwps::read16(input);
		f << val << ",";
	}

	std::string name2;
	for (i =0; i < 4; i++)
		name2.append(1, (int) libwps::readU8(input));
	f << "), " << name2;

	WPSEntry hie;
	hie.setName(name);
	hie.setType(name2);
	hie.setId(id);
	hie.setBegin((long) libwps::readU32(input));
	hie.setLength((long) libwps::readU32(input));

	f << ", offset=" << std::hex << hie.begin() << ", length=" << hie.length();

	std::string mess;
	if (cch != 0x18 && parseHeaderIndexEntryEnd(pos+cch, hie, mess))
		f << "," << mess;

	input->seek(hie.end(), WPX_SEEK_SET);
	if (input->tell() != hie.end())
	{
		f << ", ###ignored";
		ascii().addNote(f.str().c_str());
		input->seek(pos + cch, WPX_SEEK_SET);
		return true;
	}

	getNameEntryMap().insert(NameMultiMap::value_type(name, hie));

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	ascii().addPos(hie.begin());
	f.str("");
	f << name;
	if (name != name2) f << "/" << name2;
	f << ":" << std::dec << id;
	ascii().addNote(f.str().c_str());

	ascii().addPos(hie.end());
	ascii().addNote("_");

	input->seek(pos + cch, WPX_SEEK_SET);
	return true;
}

//
// read the end the entry index: normally a string name
//
bool WPS8Parser::parseHeaderIndexEntryEnd(long endPos, WPSEntry &hie, std::string &mess)
{
	WPXInputStreamPtr input = getInput();

	long pos = input->tell();
	long len = endPos-pos;
	libwps::DebugStream f;

	int size = (int) libwps::read16(input);
	WPXString str;
	if (2*(size+1) != len || !m_textParser->readString(input, 2*size, str))
	{
		WPS_DEBUG_MSG(("WPS8Parser::parseHeaderIndexEntryEnd: unknown data\n"));
		ascii().addPos(pos);
		ascii().addNote("Entry(end): ###ignored");
	}
	else
	{
		hie.setExtra(str.cstr());
		f << "'" << str.cstr() << "'";
		mess = f.str();
	}
	return true;
}

/**
 * In the header, parse the index to the different sections of
 * the CONTENTS stream.
 *
 */
bool WPS8Parser::parseHeaderIndex()
{
	WPXInputStreamPtr input = getInput();
	getNameEntryMap().clear();
	input->seek(0x08, WPX_SEEK_SET);

	long pos = input->tell();
	int i0 = (int) libwps::read16(input);
	int i1 = (int) libwps::read16(input);
	uint16_t n_entries = libwps::readU16(input);
	// fixme: sanity check n_entries

	libwps::DebugStream f;
	f << "Header: N=" << n_entries << ", " << i0 << ", " << i1 << "(";

	for (int i = 0; i < 4; i++)
		f << std::hex << libwps::read16(input) << std::dec << ",";
	f << "), ";
	f << "unk=" << std::hex <<libwps::read16(input) << std::dec << ",";

	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());

	input->seek(0x18, WPX_SEEK_SET);
	bool readSome = false;
	do
	{
		if (input->atEOS()) return readSome;

		pos = input->tell();
		f.str("");
		uint16_t unknown1 = libwps::readU16(input); // function of the size of the entries ?

		uint16_t n_entries_local = libwps::readU16(input);
		f << "Header("<<std::hex << unknown1<<"): N=" << std::dec << n_entries_local;

		if (n_entries_local > 0x20)
		{
			WPS_DEBUG_MSG(("WPS8Parser::parseHeaderIndex: error: n_entries_local=%i\n", n_entries_local));
			return readSome;
		}

		uint32_t next_index_table = libwps::readU32(input);
		f << std::hex << ", nextHeader=" << next_index_table;
		if (next_index_table != 0xFFFFFFFF && long(next_index_table) < pos)
		{
			WPS_DEBUG_MSG(("WPS8Parser::parseHeaderIndex: error: next_index_table=%x decreasing !!!\n", next_index_table));
			return readSome;
		}

		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());

		do
		{
			if (!parseHeaderIndexEntry()) return readSome;

			readSome=true;
			n_entries--;
			n_entries_local--;
		}
		while (n_entries > 0 && n_entries_local);

		if (0xFFFFFFFF == next_index_table && n_entries > 0)
		{
			WPS_DEBUG_MSG(("WPS8Parser::parseHeaderIndex: error: expected more header index entries\n"));
			return n_entries > 0;
		}

		if (0xFFFFFFFF == next_index_table)	break;

		if (input->seek(next_index_table, WPX_SEEK_SET) != 0) return readSome;
	}
	while (n_entries > 0);

	return true;
}

////////////////////////////////////////////////////////////
// low level
////////////////////////////////////////////////////////////
// read DOP zone: the document properties
bool WPS8Parser::readDocProperties(WPSEntry const &entry, WPSPageSpan &page)
{
	if (!entry.hasType(entry.name()))
	{
		WPS_DEBUG_MSG(("WPS8Parser::readDocProperties: warning: DOP name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}

	WPXInputStreamPtr input = getInput();
	page=WPSPageSpan();

	long page_offset = entry.begin();
	long length = entry.length();
	long endPage = entry.end();

	if (length < 2)
	{
		WPS_DEBUG_MSG(("WPS8Parser::readDocProperties: warning: DOP length=0x%lx\n", length));
		return false;
	}

	entry.setParsed();
	input->seek(page_offset, WPX_SEEK_SET);

	libwps::DebugStream f, f2;
	if (libwps::read16(input) != length)
	{
		WPS_DEBUG_MSG(("WPS8Parser::readDocProperties: invalid length=%ld\n", length));
		return false;
	}

	WPS8Struct::FileData mainData;
	std::string error;
	bool readOk = readBlockData(input, endPage,mainData, error);

	size_t numChild = mainData.m_recursData.size();
	double dim[8];
	bool setVal[8] = { false, false, false, false, false, false, false, false};
	for (size_t c = 0; c < numChild; c++)
	{
		WPS8Struct::FileData const &dt = mainData.m_recursData[c];
		if (dt.isBad()) continue;
		if (m_state->m_docPropertyTypes.find(dt.id())==m_state->m_docPropertyTypes.end())
		{
			f << "##" << dt << ",";
			continue;
		}
		if (m_state->m_docPropertyTypes.find(dt.id())->second != dt.type())
		{
			WPS_DEBUG_MSG(("WPS8Parser::readDocProperties: unexpected type for %d=%d\n", dt.id(), dt.type()));
			f << "###" << dt << ",";
			continue;
		}
		// is dt.id()==6 another dimension ?
		if (dt.id() >= 0 && (dt.id() < 6 || dt.id()==7))
		{
			dim[dt.id()] = float(dt.m_value)/914400.f;
			setVal[dt.id()] = true;
			continue;
		}
		bool ok = true;
		switch (dt.id())
		{
		case 0x8:
			if (!dt.m_value) break;
			if (dt.m_value >= 1 && dt.m_value <= 13)
				m_state->m_numColumns = int(1+dt.m_value);
			else
				f2 << "#";
			f2 << "numCols=" << dt.m_value+1 << ",";
			break;
		case 0x18: // 1/{_,66,96,112,186,228} 2/_
		case 0x1b:   // -1/200,3/66,3/_
			f2 << "f" << dt.id();
			if (dt.m_value)
			{
				f2 << "=" << std::hex << dt.m_value << std::dec << "(";
				int8_t low = int8_t(dt.m_value & 0xFF);
				if (low) f2 << (int) low;
				else f2 << "0";
				f2 << ":";
				if (dt.m_value & 0xFF00) f2 << (dt.m_value>>8);
				else f2 << "0";
				f2 << ")";
			}
			f2 << ",";
			break;
		case 0x13:   // an entries index, always an IBGF ?
			m_state->m_background.setName(dt.m_text);
			m_state->m_background.setId(int(dt.m_value));
			f2 << "background(entries)='" << dt.m_text << "':" << dt.m_value << ",";
			break;
		case 0x15:   // another entries index, always an 'BDR '?
			WPS_DEBUG_MSG(("WPS8Parser::readDocProperties: find a BDR entry, not implemented\n"));
			f2 << "pageBorder(entries)='" << dt.m_text << "':" << dt.m_value << ",";
			break;
		case 0xa:
			if (dt.isTrue()) f2 << "colSep(line),";
			break;
		case 0x19: // almost always set
			if (dt.isFalse())
				f2 <<  "f" << dt.id() << "=false,";
			else
				f2 <<  "f" << dt.id() << ",";
			break;
		case 0x1c:   // 0.1
			f2 <<  "colSep=" << float(dt.m_value)/914400.f << "(inch),";
			break;
		case 0x28:   // main language ?
			f2 << "lang?=" << libwps_tools_win::Language::name(dt.m_value) << ",";
			m_state->m_localeLanguage = libwps_tools_win::Language::localeName(dt.m_value);
			break;
		case 0x1e: // -3, 47 ou 85
		case 0x29: // 1 ou 2
			f2 <<  "f" << dt.id() << "=" << dt.m_value << ",";
			break;
			// 0x2a: an integer = 4|6|b
			// 0x2b: an integer = 0|8|c
		case 0x2c:
		{
			if (dt.isRead() || !dt.isArray())
			{
				ok = false;
				break;
			}
			int size = int(dt.end()-dt.begin());
			if (size < 2 || size%2 != 0)
			{
				ok = false;
				break;
			}
			long actPos = input->tell();
			int numElt = (size-2)/2;

			input->seek(dt.begin()+2, WPX_SEEK_SET);
			std::string str;
			int elt=0;
			while (elt < numElt)
			{
				long val = libwps::read16(input);
				if (val < 30 && val != '\t' && val != '\n')
				{
					input->seek(-2, WPX_SEEK_CUR);
					break;
				}
				elt++;
				str += (char) val;
			}
			if (!str.length())
			{
				ok = false;
				input->seek(actPos, WPX_SEEK_SET);
				break;
			}
			f2 << "filename?=["; // wkthmLET.fmt and wkthmSCH.fmt
			if (str.length()) f2 << "\"" << str << "\",";
			numElt -= elt;
			elt=0;
			while (elt++ < numElt)
			{
				long val = (long) libwps::readU16(input);
				if (val) f2 << "f" << elt << "=" << std::hex << val << std::dec << ",";
			}
			f2 << "],";
			input->seek(actPos, WPX_SEEK_SET);
			break;
		}
		case 0x2d:
		case 0x2e:
		{
			// In one file with unk2d=[f1=20,], unk2e=[f2=20,]
			if (dt.isRead() || !dt.isArray())
			{
				ok = false;
				break;
			}
			int size = int(dt.end()-dt.begin());
			if (size != 0x100)
			{
				ok = false;
				break;
			}
			long actPos = input->tell();
			input->seek(dt.begin(), WPX_SEEK_SET);
			f2 << "unk" << std::hex << dt.id() << "=" << "[";
			for (int i = 0; i < 128; i++)
			{
				long val = (long) libwps::readU16(input);
				if (val) f2 << "f" << i << "=" << std::hex << val << std::dec << ",";
			}
			f2 << "]" << std::dec << ",";
			input->seek(actPos, WPX_SEEK_SET);
			break;
		}

		default:
			ok = false;
			break;
		}
		if (ok) continue;
		f2 << dt << ",";
	}
	if (setVal[0])
	{
		if (dim[0] < 0.5 || dim[0] > 40) f << "###";
		else page.setFormWidth(dim[0]);
		f << "width=" << dim[0] << ",";
	}
	if (setVal[1])
	{
		if (dim[1] < 0.5 || dim[1] > 40) f << "###";
		else page.setFormLength(dim[1]);
		f << "height=" << dim[1] << ",";
	}
	f << "margin=[";
	for (int i = 2; i < 6; i++)
	{
		if (!setVal[i])
		{
			f << "_,";
			continue;
		}
		int w = i%2;
		double dd = w ? page.getFormWidth() : page.getFormLength();
		bool ok = (dim[i] >= 0) && (2*dim[i] < dd);
		switch (i)
		{
		case 2:
			if (ok) page.setMarginTop(dim[i]);
			break;
		case 3:
			if (ok) page.setMarginLeft(dim[i]);
			break;
		case 4:
			if (ok) page.setMarginBottom(dim[i]);
			break;
		case 5:
			if (ok) page.setMarginRight(dim[i]);
			break;
		default:
			break;
		}
		if (!ok) f << "###";
		f << dim[i] << ",";
	}
	f << "],";
	if (setVal[7]) f << "bordDim?=" << dim[7] << ",";
	f << f2.str();

	if (!readOk) f << "###or [" << mainData << "]";

	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());

	return true;
}

/** Frame zone: ie a zone which can contains text, picture, ...
    and have some border */
bool WPS8Parser::readFRAM(WPSEntry const &entry)
{
	typedef WPS8ParserInternal::Frame Frame;
	libwps::DebugStream f;
	if (!entry.hasType(entry.name()))
	{
		WPS_DEBUG_MSG(("WPS8Parser::readFRAM warning: name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}
	WPXInputStreamPtr input = getInput();

	long page_offset = entry.begin();
	long length = entry.length();
	long endPage = entry.end();

	if (length < 2)
	{
		WPS_DEBUG_MSG(("WPS8Parser::readFRAM warning: length=0x%lx\n", length));
		return false;
	}

	entry.setParsed();
	input->seek(page_offset, WPX_SEEK_SET);

	int numFram = libwps::read16(input);
	if (numFram < 0 || numFram*2 > length)
	{
		WPS_DEBUG_MSG(("WPS8Parser::readFRAM warning: length=0x%lx, num=%d\n", length, numFram));
		return false;
	}
	f << "N=" << numFram;

	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());


	bool parsedAll = true, color = false;
	long lastPos;
	for (int i = 0; i < numFram; i++)
	{
		lastPos = input->tell();
		if (lastPos+2 > endPage)
		{
			parsedAll = false;
			break;
		}
		int sz = libwps::read16(input);
		if (sz < 2 || lastPos + sz > endPage)
		{
			parsedAll = false;
			break;
		}

		WPS8Struct::FileData mainData;
		std::string error;
		bool readOk = readBlockData(input, lastPos + sz, mainData, error);

		size_t numChild = mainData.m_recursData.size();
		double dim[3];
		bool setVal[3] = { false, false, false};

		double bDim[4];
		bool bset = false, bsetVal[4] = { false, false, false, false};
		libwps::DebugStream f2;

		Frame frame;
		int lastPage = 0;
		Vec2f minP, sizeP;
		for (size_t c = 0; c < numChild; c++)
		{
			WPS8Struct::FileData const &dt = mainData.m_recursData[c];
			if (dt.isBad()) continue;
			if (m_state->m_frameTypes.find(dt.id())==m_state->m_frameTypes.end())
			{
				WPS_DEBUG_MSG(("WPS8Parser::readFRAM: unexpected id for %d\n", dt.id()));
				f2 << "##" << dt << ",";
				continue;
			}
			if (m_state->m_frameTypes.find(dt.id())->second != dt.type())
			{
				WPS_DEBUG_MSG(("WPS8Parser::readFRAM: unexpected type for %d=%d\n", dt.id(), dt.type()));
				f2 << "###" << dt << ",";
				continue;
			}
			if (dt.id() >= 4 && dt.id() < 11)
			{
				switch(dt.id())
				{
				case 4:
					minP.setX(float(dt.m_value)/914400.f);
					break;
				case 5:
					minP.setY(float(dt.m_value)/914400.f);
					break;
				case 6:
					sizeP.setX(float(dt.m_value)/914400.f);
					break;
				case 7:
					sizeP.setY(float(dt.m_value)/914400.f);
					break;
				default:
					dim[dt.id()-8] = float(dt.m_value)/914400.f;
					setVal[dt.id()-8] = true;
					break;
				}
				continue;
			}

			bool ok = true;
			switch (dt.id())
			{
			case 0:
			{
				int page = (int16_t) dt.m_value;
				// try to avoid creating to many page
				if (page > lastPage+100)
				{
					ok = false;
					break;
				}
				frame.m_pos.setPage(page > 0 ? page+1 : page);
				if (page > lastPage) lastPage = page;
				break;
			}
			case 1:
			{
				int low = int(dt.m_value & 0xFF);
				switch(low)
				{
					// associated with0x13=22:[0|96], 0x18,1b=0x41,0x2e, rarely with 0x1a
				case 6:
					frame.m_type = Frame::Header;
					break;
					// associated with 0x13=22:[0|96],0x18,1b=0x41,0x2e, rarely with 0x1a
				case 7:
					frame.m_type = Frame::Footer;
					break;

					// does the following correspond to page, column, char position ?

					// associated with 0x11=(m_idOle,0,1), 0x2e=?
				case 8:
					frame.m_type = Frame::Object;
					break;
					// associated with 0x18=?, 0x46=? and often with a BDR entry
					// CHECKME: seems also associated to a MCLD, how ?
				case 9:
					frame.m_type = Frame::Text;
					break;
					// associated with f3 and 0x11=(table_id,0,0) and 0x2a=?
				case 12:
					frame.m_type = Frame::Table;
					break;

				default:
					f2 << "###type=" << low;
					break;
				}

				frame.m_typeFlag = (uint8_t) (dt.m_value>>8);
				break;
			}
			case 0x2:
				if (dt.m_value&1) f2 << "noText[right],";
				if (dt.m_value&2) f2 << "noText[left],";
				if (dt.m_value&0xFC) f2 << "#f2=" << std::hex << (dt.m_value&0xFC) << ",";
				break;
			case 0x3: // seem to exist iff type=12
				if ((frame.m_type == Frame::Table) != dt.isTrue())
					f2 <<  "isTable?["
					   <<  (dt.isTrue() ? "true" : "false") << "],";
				break;
			case 0x10:   // an entry index always a BDR ?
				frame.m_idBorder.setName(dt.m_text);
				frame.m_idBorder.setId(int(dt.m_value));
				break;
			case 0x11:   // a list of 3 int32 ? Only the first is signifiant ?
			{
				if (dt.isRead() || !dt.isArray())
				{
					ok = false;
					break;
				}
				int size = int(dt.end()-dt.begin());
				if (size < 2 || (size+2)%4 != 0)
				{
					ok = false;
					break;
				}
				int numElt = (size-2)/4;
				long actPos = input->tell();

				input->seek(dt.begin()+2, WPX_SEEK_SET);
				std::vector<long> values;
				for (int elt = 0; elt < numElt; elt++)
				{
					long val = libwps::read32(input);
					values.push_back(val);
				}

				bool canHaveId = frame.m_type==Frame::Table || frame.m_type==Frame::Object;
				if (canHaveId && numElt == 3 && values[1] == 0 && values[2] ==0)
					frame.m_idObject = int(values[0]);
				else if (canHaveId && numElt == 3 && values[1] == 0 && values[2] == 1)
					frame.m_idOle = int(values[0]);
				else
				{
					WPS_DEBUG_MSG(("WPS8Parser::readFRAM unknown field 0x11\n"));
					f2 << "###f17=(";
					for (int j = 0; j < numElt; j++) f2 << values[size_t(j)] << ",";
					f2 << "),";
				}

				input->seek(actPos, WPX_SEEK_SET);
				break;
			}
			// associated with a tex box and field 0x19
			case 0x17:
				color = dt.isTrue();
				break;
			case 0x18:
			{
				if (dt.m_value < 0 || dt.m_value >= m_textParser->getNumTextZones())
				{
					ok = false;
					break;
				}
				int type = m_textParser->getTextZoneType((int) dt.m_value);
				if ((type == 6 && frame.m_type == Frame::Header) ||
				        (type == 7 && frame.m_type == Frame::Footer) ||
				        (type == 5))
					frame.m_idStrs = (int) dt.m_value;
				else
				{
					WPS_DEBUG_MSG(("WPS8Parser::readFRAM odd id for field 0x18\n"));
					ok = false;
				}
				break;
			}
			case 0x19: // CHECKME: can we also have a front color and a pattern ?
				if (!color)
					f2 << "#f23=false,";
				frame.m_backgroundColor = dt.getRGBColor();
				break;
			case 0x1b: // 41
				if (frame.m_type!=Frame::Footer || dt.m_value != 0x41)
					f2 << "##f" << dt.id() << "=" << (int16_t) dt.m_value << ",";
				break;
			case 0x26: // rotation in degree from center ( CCW)
				f2 << "rot=" << float(dt.m_value)/10. << "deg,";
				break;
				//
				// Table specific field ?
				//
			case 0x2a:   // link to the m_idTable(ids) number for a table
			{
				int id = (int16_t) dt.m_value;
				if (frame.m_type==Frame::Table)
					frame.m_idTable = id;
				else
					f2 << "f" << dt.id() << "=" << id << ",";
				break;
			}
			case 0x2c: // CHEKME
				if (frame.m_type==Frame::Table)
					frame.m_columns = int(dt.m_value);
				else ok = false;
				break;
			case 0x14: // in relation with 0x30 ?
				if (frame.m_type==Frame::Table)
					f2 << "grpId=" << (int16_t) dt.m_value << ",";
				else ok = false;
				break;
			case 0x30: // seems to link the table together
				if (frame.m_type==Frame::Table)
					f2 << "tableGrp=" << (int16_t) dt.m_value << ",";
				else ok = false;
				break;
			case 0x2d: // appear 2 time in a table with value 2
				f2 << "f" << dt.id() << "=" << (int16_t) dt.m_value << ",";
				break;

				// UNKNOWN
			case 0x1a: // appear 2 time (in a header and in a footer) with value 16
				f2 << "f" << dt.id() << "=" << (int16_t) dt.m_value << ",";
				break;

			case 0x2e: // in a table: objectId+1, in textbox: m_idTable ?, other = i ?
				f2 << "id" << dt.id() << "=" << (int16_t) (dt.m_value-1) << ",";
				break;
			case 0x13:
			{
				// table 0x6-??,
				// header/footer 0x16-??,
				// textbox 0x10(means automatic resize)
				// object : no set
				f2 << "f" << dt.id();
				if (dt.m_value)
				{
					f2 << "=" << std::hex << dt.m_value << std::dec << "(";
					int low = int(dt.m_value & 0xFF);
					if (low) f2 << low;
					else f2 << "0";
					f2 << ":";
					int high = int(dt.m_value>>8);
					if (dt.m_value & 0xFF00) f2 << high;
					else f2 << "0";
					f2 << ")";
				}
				f2 << ",";
				break;
			}
			case 0x2f:
				if (dt.isTrue())
					f2 <<  "f" << dt.id() << ",";
				else
					f2 <<  "f" << dt.id() << "=false,";
				break;
			case 0x1D:
			case 0x1E:
			case 0x1F:
			case 0x20: // in inches small value 0/0.05/0.1 -> a border size ?
				bset = true;
				bsetVal[dt.id()-0x1D] = true;
				bDim[dt.id()-0x1D] =  float(dt.m_value)/914400.f;
				break;

			default:
				ok = false;
			}

			if (ok) continue;
			f2 << "#" << dt << ",";
		}
		frame.m_pos.setOrigin(minP);
		frame.m_pos.setSize(sizeP);
		m_state->m_frameList.push_back(frame);

		f.str("");
		f << entry.name() << "(" << i << "):" << frame << ",";
		if (setVal[0] || setVal[1])
		{
			f << "f8/9=[";
			for (int j = 0; j < 2; j++)
			{
				if (!setVal[j])
				{
					f << "_,";
					continue;
				}
				double diff = 25/18.-dim[j];
				if (diff >= -1e-3 && diff <= 1e-3) f << "*,";
				else f << dim[j] << ",";
			}
			f << "],";
		}
		if (setVal[2]) f << "border[w]=" << dim[2] << ",";

		if (bset)
		{
			f << "borderMod[w]=["; // L, T, R, B
			for (int j = 0; j < 4; j++)
			{
				if (!bsetVal[j])
				{
					f << "_,";
					continue;
				}
				f << bDim[j] << ",";
			}
			f << "],";
		}
		f << f2.str();
		if (!readOk) f << ", ###or" << mainData;

		ascii().addPos(lastPos);
		ascii().addNote(f.str().c_str());
	}
	if (parsedAll) return true;

	ascii().addPos(lastPos);
	f.str("");
	f << "###" << entry.name();
	ascii().addNote(f.str().c_str());
	return false;
}

// reads SYID zone: an unknown zone which seems to have the
//    same number of entries than the text zone
// also related to the MCLD ?
bool WPS8Parser::readSYID(WPSEntry const &entry, std::vector<int> &listId)
{
	WPXInputStreamPtr input = getInput();
	listId.resize(0);
	libwps::DebugStream f;
	if (!entry.hasType(entry.name()))
	{
		WPS_DEBUG_MSG(("WPS8Parser::readSYID: warning: SYID name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}

	long page_offset = entry.begin();
	long length = entry.length();

	if (length < 4)
	{
		WPS_DEBUG_MSG(("WPS8Parser::readSYID: warning: SYID length=0x%lx\n", length));
		return false;
	}

	input->seek(page_offset, WPX_SEEK_SET);

	int unk = libwps::read32(input);
	int numID = libwps::read32(input);
	if (numID < 0 || 4*(numID+2) != length)
	{
		WPS_DEBUG_MSG(("WPS8Parser::readSYID: invalid length=%ld, num=%d\n", length, numID));
		return false;
	}

	f << "N=" << numID << ", unkn=" << unk << ", (";
	for (int i = 0; i < numID; i++)
	{
		int val = libwps::read32(input);
		listId.push_back(val);
		f << val << ",";
	}
	f << ")";

	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());

	entry.setParsed();
	return true;
}

// reads the WNPR zone : a zone which seems to contain the printer preferences
// Warning: read data are not used
bool WPS8Parser::readWNPR(WPSEntry const &entry)
{
	if (!entry.hasType("WNPR"))
	{
		WPS_DEBUG_MSG(("WPS8Parser::readWNPR: warning: WNPR name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}
	WPXInputStreamPtr input = getInput();
	long page_offset = entry.begin();
	long length = entry.length();
	long endPage = entry.end();

	if (length < 40)
	{
		WPS_DEBUG_MSG(("WPS8Parser::readWNPR: warning: WNPR length=0x%lx\n", length));
		return false;
	}

	entry.setParsed();
	input->seek(page_offset, WPX_SEEK_SET);

	libwps::DebugStream f;

	f << std::hex << libwps::readU32(input) << std::dec << ",";
	long dim[4];
	for (int i = 0; i < 4; i++)
	{
		dim[i] = libwps::read32(input);
		if (dim[i] <= 0) return false;
	}

	if (dim[2] && dim[3])
	{
		f << "width=" << float(dim[0])/float(dim[2]) << ",";
		f << "height=" << float(dim[1])/float(dim[3]) << ",";
	}
	else
	{
		f << "###width=" << dim[0] << ":" << dim[3] << ",";
		f << "###height=" << dim[1] << ":" << dim[3] << ",";
	}

	f << "printmargins?=(";
	for (int i = 0; i < 4; i++)
	{
		long val = libwps::readU32(input);
		long sz = dim[2+(i%2)];
		if (sz)
			f << float(val)/float(sz) << ",";
		else
			f << "###" << val << ",";
	}
	f << "),";

	for (int i = 0; i < 2; i++)
		f << std::hex << libwps::readU16(input) << std::dec <<",";


	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());

	long actPos = input->tell();
	if (actPos + 32*2+38+22 > endPage)
	{
		WPS_DEBUG_MSG(("WPS8Parser::readWNPR: length=0x%lx seems too short\n", length));
		return false;
	}

	// DEVMODEA
	std::string st;
	for (int i = 0; i < 32; i++)
	{
		unsigned char c = libwps::readU8(input);
		if (!c) continue; // normally end by c, check me
		st+= char(c);
	}
	f.str("");
	f << "PRNT(DevMode):";
	f << "devName='" << st << "'," << std::hex;
	f << "specVersion=" << libwps::readU16(input) << ",";
	f << "driverVersion=" << libwps::readU16(input) << ",";
	int dmSize = libwps::readU16(input);
	if (actPos + dmSize > endPage || dmSize < 124) return false;
	f << "dmSize=" << dmSize << ",";
	f << "driverExtras=" << libwps::readU16(input) << ",";
	f << "dmFields=" << libwps::readU32(input) << ",";
	f << "orientation=" << libwps::readU16(input) << ","; // 1: portrait, 2: landscape
	f << "paperSize=" << libwps::readU16(input) << ","; // 1: letter, 2:letter small, ..

	long dim2[3];
	for (int i = 0; i < 3; i++)
		dim2[i] = libwps::read16(input);
	f << std::dec;
	if (dim2[2] > 0)
	{
		f << "paperLength=" << std::dec << float(dim2[0])/float(dim2[2]) << ",";
		f << "paperWidth=" << float(dim2[1])/float(dim2[2]) << ",";
	}
	else
	{
		f << "paperLength=" << dim2[0] << ",";
		f << "paperWidth=" << dim2[1] << ",";
		f << "dmScale=" << dim2[2] << ",";
	}
	f << "dmCopies=" << libwps::readU16(input) << ",";
	f << "dmDefaultSource=" << libwps::readU16(input) << ",";
	f << "dmPrintQuality=" << libwps::read16(input) << ",";
	f << std::hex;
	f << "dmColor=" << libwps::readU16(input) << ",";
	f << "dmDuplex=" << libwps::readU16(input) << ",";
	f << "dmYResolution=" << libwps::readU16(input) << ",";
	f << "dmTTOption=" << libwps::readU16(input) << ",";
	f << "dmCollate=" << libwps::readU16(input) << ",";

	st="";
	for (int i = 0; i < 32; i++)
	{
		unsigned char c = libwps::readU8(input);
		if (!c) continue; // normally end by c, check me
		st+= char(c);
	}
	f << "formName='" << st << "',";
	f << "dmLogPixels=" << libwps::readU16(input) << ",";
	f << "dmBitsPerPel=" << libwps::readU32(input) << ",";
	f << "dmPelsWidth=" << libwps::readU32(input) << ",";
	f << "dmPelsHeight=" << libwps::readU32(input) << ",";
	f << "dmDisplayFlags=" << libwps::readU32(input) << ",";
	f << "dmDisplayFrequency=" << libwps::readU32(input) << ",";

	ascii().addPos(actPos);
	ascii().addNote(f.str().c_str());

	long devEndPos = actPos+dmSize;
	actPos = input->tell();
	if (actPos != devEndPos)
	{
		ascii().addPos(actPos);
		ascii().addNote("PRNT(DevMode-End)");
	}

	ascii().addPos(devEndPos);
	ascii().addNote("PRNT(Unknown)");

	return true;
}

////////////////////////////////////////////////////////////////
//
// ? Syllabes seperator to read the text
// it is really related to spell check or to speech
// Can surely be safety ignored in normal version.
//
////////////////////////////////////////////////////////////////
#ifndef DEBUG
bool WPS8Parser::readSPELLING(WPXInputStreamPtr, std::string const &)
{
	return true;
}
#else
bool WPS8Parser::readSPELLING(WPXInputStreamPtr input, std::string const &oleName)
{
	// SPELLING
	input->seek(0, WPX_SEEK_SET);
	int vers = (int) libwps::read32(input); // always 6 ?
	if (vers < 0 || input->atEOS()) return false;

	std::string fName = libwps::Debug::flattenFileName(oleName);
	libwps::DebugFile asciiFile(input);
	asciiFile.open(fName);


	int actId = 0;
	std::map<uint32_t, int> listIds;

	libwps::DebugStream f;
	int num = 0;
	while(!input->atEOS())
	{
		long pos = input->tell();
		int numVal = (int) libwps::read32(input);

		if (numVal < 0 || input->seek(8*numVal, WPX_SEEK_CUR) || input->tell() != pos+4+8*numVal)
		{
			input->seek(pos, WPX_SEEK_SET);
			break;
		}

		f.str("");
		f << "SPELLING" << num++ << ": "; // linked to STRSj by num = numSTRS-j?
		input->seek(pos+4, WPX_SEEK_SET);
		for (int j = 0; j < numVal; j++)
		{
			uint32_t ptr = libwps::readU32(input);
			int id;
			if (listIds.find(ptr) == listIds.end())
			{
				id = actId++;
				listIds[ptr] = id;
			}
			else
				id = listIds[ptr];
			long sPos = (long) libwps::read32(input);
			f << std::hex << 2*sPos << ":SP" <<  std::dec << id << ",";
		}
		asciiFile.addPos(pos);
		asciiFile.addNote(f.str().c_str());
	}
	f.str("");
	f << "SPELLING:";
	if (vers != 6) f << "version = " << vers << ", ";
	f << "list=[";
	for (std::map<uint32_t, int>::iterator it = listIds.begin(); it != listIds.end(); it++)
	{
		uint32_t val = it->first;
		f << "SP" << std::dec << it->second << "(" << (val&0xFF) << ":"
		  << std::hex << (val>>8) << std::dec << "),";

		// (val&0xFF) :121-123, 129, 133, 139, 145
		// (val>>8) : 0, e6, d9a3b, 3b75f4, 474555, 548bbb, 609daa,748cce, ba32b9, c068cd, d39bea
		//          seems constant in a SPELLING Ole
	}
	f << "]";
	asciiFile.addPos(0);
	asciiFile.addNote(f.str().c_str());
	if (!input->atEOS())
	{
		asciiFile.addPos(input->tell());
		asciiFile.addNote("###SPELLING");
	}
	return true;
}
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
