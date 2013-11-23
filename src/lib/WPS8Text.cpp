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

#include <map>

#include "libwps_internal.h"

#include "WPSContentListener.h"
#include "WPSEntry.h"
#include "WPSList.h"
#include "WPSParagraph.h"
#include "WPSSubDocument.h"

#include "WPS8.h"
#include "WPS8Struct.h"
#include "WPS8TextStyle.h"

#include "WPS8Text.h"

/** Internal and low level: the structures of a WPS8Text used to parse PLC*/
namespace WPS8PLCInternal
{
/** Internal and low level: the PLC different types and their structures */
struct PLC;

//! a map of known plc
struct KnownPLC
{
public:
	//! constructor
	KnownPLC();
	//! destructor
	~KnownPLC();
	//! returns the PLC corresponding to a name
	PLC get(std::string const &name);
protected:
	//! creates the map of known PLC
	void createMapping();
	//! map name -> known PLC
	std::map<std::string, PLC> m_knowns;
};
}

namespace WPS8TextInternal
{
/** different types
 *
 * - BTE: font/paragraph properties
 * - OBJECT: object properties: image, table, ..
 * - STRS: the text zones
 * - TCD: the text subdivision ( cells subdivision in a table, ...)
 * - TOKEN: field type: date/time/..
 * - BMKT: bookmark, or a field in a datafile ?
 */
enum PLCType { BTE=0, TCD, STRS, OBJECT, TOKEN, BMKT, Unknown};

/** Internal: class to store a field definition (BKMT) */
struct Bookmark
{
	//! constructor
	Bookmark() : m_id(-1), m_text(""), m_error("") {}
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, Bookmark const &tok);

	//! an index
	int m_id;
	//! the field value
	librevenge::RVNGString m_text;
	//! a string used to store the parsing errors
	std::string m_error;
};
//! operator<< for a Bookmark
std::ostream &operator<<(std::ostream &o, Bookmark const &bmk)
{
	o << std::dec << "Bookm" << bmk.m_id << "='" << bmk.m_text.cstr() << "'";
	if (!bmk.m_error.empty()) o << ", err=[" << bmk.m_error << "]";
	return o;
}
/** Internal: class to store the note position */
struct Notes
{
	/** constructor */
	Notes() : m_zoneNote(-1), m_zoneCorr(-1), m_type(WPSContentListener::FOOTNOTE), m_note(false), m_corr(0L), m_positions() {}
	/** copy constructor */
	Notes(Notes const &orig) :
		m_zoneNote(-1), m_zoneCorr(-1), m_type(WPSContentListener::FOOTNOTE), m_note(false), m_corr(0L), m_positions()
	{
		*this = orig;
	}
	/** copy operator */
	Notes &operator=(Notes const &orig)
	{
		if (this != &orig)
		{
			m_zoneNote = orig.m_zoneNote;
			m_zoneCorr = orig.m_zoneCorr;
			m_type = orig.m_type;
			m_note = orig.m_note;
			m_corr = orig.m_corr;
			m_positions = orig.m_positions;
		}
		return *this;
	}
	WPSEntry getCorrespondanceEntry(long offset) const
	{
		WPSEntry res;
		if (!m_corr) return res;
		size_t nPos = m_positions.size();
		for (size_t i = 0; i+1 < nPos; i++)
		{
			if (m_positions[i] != offset) continue;
			res.setBegin(m_corr->m_positions[i]);
			res.setEnd(m_corr->m_positions[i+1]);
			res.setId(m_type); // set the type
			res.setType("Text");
			break;
		}
		return res;
	}

	/** the id of the zone which corresponds to the data */
	int m_zoneNote;
	/** the id of the zone which called/used this data */
	int m_zoneCorr;
	/** the note type : footnote or endnote */
	WPSContentListener::NoteType m_type;
	/** a flag to know if this is the note content */
	bool m_note;
	/** a pointer to the corresponding notes */
	Notes const *m_corr;
	/** the positions of the data in the file */
	std::vector<long> m_positions;

	/*! \struct Compare
	 * \brief internal struct used to create sorted map
	 */
	struct Compare
	{
		//! comparaison function
		bool operator()(Notes const *n1, Notes const *n2) const
		{
			int diff = n1->m_zoneNote - n2->m_zoneNote;
			if (diff) return diff < 0;
			diff = n1->m_zoneCorr - n2->m_zoneCorr;
			if (diff) return diff < 0;
			diff = int(n1->m_type)-int(n2->m_type);
			return diff < 0;
		}
	};
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, Notes const &note);
};

//! operator<< for an object
std::ostream &operator<<(std::ostream &o, Notes const &note)
{
	o << std::dec;
	switch (note.m_type)
	{
	case WPSContentListener::ENDNOTE:
		o << "endnote";
		break;
	case WPSContentListener::FOOTNOTE:
		o << "footnote";
		break;
	default:
		o << "###Unknown" << (int) note.m_type;
		break;
	}
	o << " in zone=" << note.m_zoneNote << "(corr. zone=" << note.m_zoneCorr << ")";
	size_t numPos = note.m_positions.size();
	o << ": N=" << numPos-1;
	o << ",ptrs=(" << std::hex;
	for (size_t i = 0; i < numPos; i++) o << "0x" << note.m_positions[i] << ",";
	o << ")" << std::dec;
	return o;
}
/** Internal: class to store an object definition */
struct Object
{
	//! the object type
	enum Type { Unknown = 0, Table, Image };

	//! constructor
	Object() : m_type(Unknown), m_id(-1), m_size(), m_unknown(0), m_error("") {}
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, Object const &obj);

	//! the type (normally a Type, ...)
	int m_type;
	//! an identificator
	int m_id;
	//! the size of the object in a page
	Vec2f m_size;
	//! unknown data
	long m_unknown;
	//! a string used to store the parsing errors
	std::string m_error;
};

//! operator<< for an object
std::ostream &operator<<(std::ostream &o, Object const &obj)
{
	o << std::dec;
	switch (obj.m_type)
	{
	case Object::Table:
		o << "Table";
		break;
	// an object store in another Ole: id-> gives a pointer
	case Object::Image:
		o << "Object";
		break;
	default:
		o << "Unknown" << -1-obj.m_type;
		break;
	}
	if (obj.m_id > -1) o << ",eobj(id)=" << obj.m_id;
	o <<": size(" << obj.m_size << ")";

	// Object : unkn=0,1
	// Table : unkn = 609a1b52, 64cf1858, 64e5da2f, 3311ef0
	if (obj.m_unknown) o << std::hex << ", unkn=" << obj.m_unknown << std::dec;
	if (!obj.m_error.empty()) o << ", err=" << obj.m_error;
	return o;
}

/** Internal: class to store a field definition (TOKN) */
struct Token
{
	//! constructor
	Token() : m_type(WPSContentListener::None), m_textLength(-1), m_unknown(-1), m_text(""), m_error("") {}
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, Token const &tok);

	//! the field type
	WPSContentListener::FieldType m_type;
	//! the length of the text corresponding to the token
	int m_textLength;
	//! an unknown value
	int m_unknown;
	//! the field value
	librevenge::RVNGString m_text;
	//! a string used to store the parsing errors
	std::string m_error;
};
//! operator<< for a Token
std::ostream &operator<<(std::ostream &o, Token const &tok)
{
	o << std::dec;
	switch (tok.m_type)
	{
	case WPSContentListener::PageNumber:
		o << "field[page],";
		break;
	case WPSContentListener::Date:
		o << "field[date],";
		break;
	case WPSContentListener::Time:
		o << "field[time],";
		break;
	case WPSContentListener::Title:
		o << "field[title],";
		break;
	case WPSContentListener::Link:
		o << "field[link],";
		break;
	case WPSContentListener::Database:
	case WPSContentListener::None:
	default:
		o << "##field[unknown]" << ",";
		break;
	}
	if (tok.m_text.len()) o << "value='" << tok.m_text.cstr() << "',";
	if (tok.m_textLength != -1) o << "textLen=" << tok.m_textLength << ",";
	if (tok.m_unknown != -1) o << "unkn=" << tok.m_unknown << ",";
	if (!tok.m_error.empty()) o << "err=[" << tok.m_error << "]";
	return o;
}
/** Internal: class to store the PLC: Pointer List Content ? */
struct DataPLC
{
	//! constructor
	DataPLC(): m_name(""), m_type(Unknown), m_value(-1), m_error("") {}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, DataPLC const &plc);
	//! the entry name
	std::string m_name;
	//! the plc type
	PLCType m_type;
	//! a potential value
	long m_value;
	//! a string used to store the parsing errors
	std::string m_error;
};
//! operator<< for a DataPLC
std::ostream &operator<<(std::ostream &o, DataPLC const &plc)
{
	o << "type=" << plc.m_name << ",";
	if (plc.m_value != -1) o << "val=" << std::hex << plc.m_value << std::dec << ", ";
	if (!plc.m_error.empty()) o << "errors=(" << plc.m_error << ")";
	return o;
}

/** Internal: the state of a WPS4Text */
struct State
{
	//! constructor
	State() : m_textZones(), m_bookmarkMap(), m_notesList(), m_notesMap(),
		m_object(), m_objectMap(), m_tokenMap(), m_tcdMap(),
		m_objectTypes(), m_plcList(), m_knownPLC()
	{
		initTypeMaps();
	}

	//! initializes the type map
	void initTypeMaps();
	//! returns the entry corresponding to a type id
	WPSEntry getTextZones(int id) const
	{
		size_t numZones = m_textZones.size();
		for (size_t i = 0; i < numZones; i++)
		{
			WPSEntry const &z = m_textZones[i];
			if (!z.valid()) continue;
			if (z.id() == id) return z;
		}
		return WPSEntry();
	}
	//! tests if a text zone is simillar to \a entry, if yes, updates is parsed field
	void setParsed(WPSEntry const &entry, bool fl)
	{
		size_t numZones = m_textZones.size();
		for (size_t i = 0; i < numZones; i++)
		{
			if (m_textZones[i] != entry) continue;
			m_textZones[i].setParsed(fl);
			return;
		}
	}
	//! try to return a entry for a cell in table zones
	WPSEntry getTCDZone(int strsId, int cellId) const
	{
		if (strsId < 0 || strsId >= int(m_textZones.size()) || cellId < 0)
			return WPSEntry();
		if (m_tcdMap.find(strsId) == m_tcdMap.end())
		{
			if (cellId != 0)
				return WPSEntry();
			// a table with 1 cell has no tcd,
			m_textZones[size_t(strsId)].setParsed(true);
			return m_textZones[size_t(strsId)];
		}

		std::vector<long> const &endPos = m_tcdMap.find(strsId)->second;
		if (cellId >= int(endPos.size()))
			return WPSEntry();

		m_textZones[size_t(strsId)].setParsed(true);
		WPSEntry res = m_textZones[size_t(strsId)];
		if (cellId)
			res.setBegin(endPos[size_t(cellId-1)]+2);
		res.setEnd(endPos[size_t(cellId)]);
		return res;
	}

	//! the list of different text zones
	std::vector<WPSEntry> m_textZones;

	//! a map text offset->bookmark
	std::map<long, Bookmark> m_bookmarkMap;

	//! the list of notes
	std::vector<Notes> m_notesList;

	//! a map text offset->notes
	std::map<long, Notes *> m_notesMap;

	//! actual object
	Object m_object;
	//! a map text offset->object
	std::map<long, Object> m_objectMap;

	//! a map text offset->token
	std::map<long, Token> m_tokenMap;

	//! a map strsId -> last positions of cells
	std::map<int, std::vector<long> > m_tcdMap;

	//! the object type
	std::map<int,int> m_objectTypes;
	//! a list of all plcs
	std::vector<DataPLC> m_plcList;
	//! the known plc
	WPS8PLCInternal::KnownPLC m_knownPLC;
};

void State::initTypeMaps()
{
	static int const objectTypes[] =
	{
		0, 0x1A, 1, 0x22, 2, 0x22, 3, 0x22, 4, 0x22
	};
	for (int i = 0; i+1 < int(sizeof(objectTypes)/sizeof(int)); i+=2)
		m_objectTypes[objectTypes[i]] = objectTypes[i+1];
}

//! Internal: the subdocument of a WPS8Text
class SubDocument : public WPSSubDocument
{
public:
	//! constructor for a note/endnote entry
	SubDocument(RVNGInputStreamPtr input, WPS8Text &pars, WPSEntry const &entry) :
		WPSSubDocument(input, 0), m_textParser(&pars), m_entry(entry), m_text("") {}
	//! constructor for a comment entry
	SubDocument(RVNGInputStreamPtr input, librevenge::RVNGString const &text) :
		WPSSubDocument(input, 0), m_textParser(0), m_entry(), m_text(text) {}

	//! destructor
	~SubDocument() {}

	//! operator==
	virtual bool operator==(WPSSubDocumentPtr const &doc) const
	{
		if (!WPSSubDocument::operator==(doc))
			return false;
		SubDocument const *sDoc = dynamic_cast<SubDocument const *>(doc.get());
		if (!sDoc) return false;
		if (m_entry != sDoc->m_entry) return false;
		if (m_textParser != sDoc->m_textParser) return false;
		if (m_text != sDoc->m_text) return false;
		return true;
	}

	//! the parser function
	void parse(WPSContentListenerPtr &listener, libwps::SubDocumentType type);

	WPS8Text *m_textParser;
	WPSEntry m_entry;
	librevenge::RVNGString m_text;
private:
	SubDocument(SubDocument const &orig);
	SubDocument &operator=(SubDocument const &orig);
};

void SubDocument::parse(WPSContentListenerPtr &listener, libwps::SubDocumentType type)
{
	if (!listener.get())
	{
		WPS_DEBUG_MSG(("SubDocument::parse: no listener\n"));
		return;
	}
	if (!dynamic_cast<WPSContentListener *>(listener.get()))
	{
		WPS_DEBUG_MSG(("SubDocument::parse: bad listener\n"));
		return;
	}
	WPSContentListenerPtr &listen =  reinterpret_cast<WPSContentListenerPtr &>(listener);

	if (type==libwps::DOC_COMMENT_ANNOTATION)
	{
		listen->insertUnicodeString(m_text);
		return;
	}
	if (!m_textParser)
	{
		listen->insertCharacter(' ');
		WPS_DEBUG_MSG(("SubDocument::parse: bad parser\n"));
		return;
	}
	if (!m_entry.valid())
	{
		listen->insertCharacter(' ');
		return;
	}
	long actPos = m_input->tell();
	if (type == libwps::DOC_NOTE)
		m_textParser->readText(m_entry);
	else
	{
		WPS_DEBUG_MSG(("SubDocument::parse: find unknown type of document...\n"));
	}
	m_input->seek(actPos, librevenge::RVNG_SEEK_SET);
}

}

//////////////////////////////////////////////////////////////////////////////
// MAIN CODE
//////////////////////////////////////////////////////////////////////////////

// constructor/destructor
WPS8Text::WPS8Text(WPS8Parser &parser) : WPSTextParser(parser, parser.getInput()),
	m_listener(), m_styleParser(), m_state()
{
	m_state.reset(new WPS8TextInternal::State);
	m_styleParser.reset(new WPS8TextStyle(*this));
}

WPS8Text::~WPS8Text ()
{
}

//! sets the listener
void WPS8Text::setListener(WPSContentListenerPtr &listen)
{
	m_listener = listen;
	m_styleParser->setListener(listen);
}


// number of page
int WPS8Text::numPages() const
{
	int numPage = 1;
	m_input->seek(m_textPositions.begin(), librevenge::RVNG_SEEK_SET);
	while (!m_input->isEnd() && m_input->tell() < m_textPositions.end())
	{
		if (libwps::readU16(m_input.get()) == 0x0C) numPage++;
	}
	return numPage;
}


////////////////////////////////////////////////////////////
// interface with WPS8TextStyle
////////////////////////////////////////////////////////////
bool WPS8Text::readFont(long endPos, int &id, std::string &mess)
{
	return m_styleParser->readFont(endPos, id, mess);
}

bool WPS8Text::readParagraph(long endPos, int &id, std::string &mess)
{
	return m_styleParser->readParagraph(endPos, id, mess);
}

////////////////////////////////////////////////////////////
// retrieves zone
////////////////////////////////////////////////////////////
WPSEntry WPS8Text::getEntry(int strsId) const
{
	if (strsId >= int(m_state->m_textZones.size())) return WPSEntry();
	return m_state->m_textZones[size_t(strsId)];
}

WPSEntry WPS8Text::getHeaderEntry() const
{
	WPSEntry res=m_state->getTextZones(6);
	if (!res.valid() || res.length() != 2) return res;
	// check if the zone is empty ie. only a eol
	RVNGInputStreamPtr input = const_cast<WPS8Text *>(this)->getInput();
	long actPos = input->tell();
	input->seek(res.begin(), librevenge::RVNG_SEEK_SET);
	bool empty = libwps::read16(input) == 0xd;
	input->seek(actPos, librevenge::RVNG_SEEK_SET);
	if (empty)
	{
		m_state->setParsed(res, true);
		return WPSEntry();
	}
	return res;
}

WPSEntry WPS8Text::getFooterEntry() const
{
	WPSEntry res=m_state->getTextZones(7);
	if (!res.valid() || res.length() != 2) return res;
	// check if the zone is empty ie. only a eol
	RVNGInputStreamPtr input = const_cast<WPS8Text *>(this)->getInput();
	long actPos = input->tell();
	input->seek(res.begin(), librevenge::RVNG_SEEK_SET);
	bool empty = libwps::read16(input) == 0xd;
	input->seek(actPos, librevenge::RVNG_SEEK_SET);
	if (empty)
	{
		m_state->setParsed(res, true);
		return WPSEntry();
	}
	return res;
}

WPSEntry WPS8Text::getTextEntry() const
{
	return m_state->getTextZones(1);
}

int WPS8Text::getNumTextZones() const
{
	return (int) m_state->m_textZones.size();
}
int WPS8Text::getTextZoneType(int strsId) const
{
	assert(strsId >= 0 && strsId < int(m_state->m_textZones.size()));
	return m_state->m_textZones[size_t(strsId)].id();
}

void WPS8Text::readTextInCell(int strsId, int cellId)
{
	if (!m_listener) return;
	WPSEntry entry = m_state->getTCDZone(strsId, cellId);
	if (entry.length()==0)
	{
		m_listener->insertCharacter(' ');
		return;
	}
	readText(entry);
}

/**
 * Read the range of the document text using previously-read
 * formatting information, up to but excluding entry.end().
 *
 */
void WPS8Text::readText(WPSEntry const &entry)
{
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("WPS8Text::readText: called without listener!!!\n"));
		return;
	}
	RVNGInputStreamPtr input = getInput();
	m_state->setParsed(entry,true);
	bool mainZone = entry.id()==1;
	if (mainZone && mainParser().numColumns() > 1)
	{
		int numColumns = mainParser().numColumns();
		if (m_listener->isSectionOpened())
			m_listener->closeSection();
		int w = int(72.0*mainParser().pageWidth()/numColumns);
		std::vector<int> colSize(size_t(numColumns), w);
		m_listener->openSection(colSize, librevenge::RVNG_POINT);
	}
	int lastCId=-1, lastPId=-1; /* -2: nothing, -1: send default, >= 0: readId */
	std::vector<DataFOD>::iterator plcIt =	m_FODList.begin();
	while (plcIt != m_FODList.end() && plcIt->m_pos < entry.begin())
	{
		DataFOD const &plc = *(plcIt++);
		if (plc.m_type==DataFOD::ATTR_TEXT)
			lastCId = plc.m_id;
		else if (plc.m_type==DataFOD::ATTR_PARAG)
			lastPId = plc.m_id;
	}
	int actualPage = 1;
	WPS8TextStyle::FontData special;
	input->seek(entry.begin(), librevenge::RVNG_SEEK_SET);
	while (!input->isEnd())
	{
		long pos = input->tell();
		libwps::DebugStream f;
		f << "TEXT:";
		if (pos+1 >= entry.end())
			break;
		long finalPos = entry.end();
		while (plcIt != m_FODList.end())
		{
			DataFOD const &plc = *plcIt;
			if (plc.m_pos < pos)
			{
				WPS_DEBUG_MSG(("WPS8Text::readText: ### problem with pos\n"));
				continue;
			}
			if (plc.m_pos > pos)
			{
				if (plc.m_pos < finalPos)
					finalPos = plc.m_pos;
				break;
			}
			switch(plc.m_type)
			{
			case DataFOD::ATTR_TEXT:
			{
				lastCId = -2;
				if (plc.m_id < 0)
					f << "[C_]";
				else
					f << "[C" << plc.m_id << "]";
				m_styleParser->sendFont(plc.m_id, special);
				break;
			}
			case DataFOD::ATTR_PARAG:
			{
				lastPId = -2;
				if (plc.m_id < 0)
					f << "[P_]";
				else
					f << "[P" << plc.m_id << "]";
				m_styleParser->sendParagraph(plc.m_id);
				break;
			}
			case DataFOD::ATTR_PLC:
			{
				if (plc.m_id < 0) break;
				if (plc.m_id >= int(m_state->m_plcList.size()))
				{
					f << "#[PLC"<< plc.m_id << "]";
					WPS_DEBUG_MSG(("WPS8Text::readText: can not find plc %d\n",plc.m_id));
					break;
				}
				WPS8TextInternal::DataPLC const &thePLC = m_state->m_plcList[size_t(plc.m_id)];
				f << "[" << thePLC << "]";
#ifdef DEBUG
				// no sure, if we want to output this data
				if (thePLC.m_type == WPS8TextInternal::BMKT)
				{
					if (m_state->m_bookmarkMap.find(pos) == m_state->m_bookmarkMap.end())
					{
						WPS_DEBUG_MSG(("WPS8Text::readText: can not find bookmark for pos %lX\n",pos));
					}
					else
					{
						shared_ptr<WPSSubDocument> doc(new WPS8TextInternal::SubDocument(input, m_state->m_bookmarkMap.find(pos)->second.m_text));
						m_listener->insertComment(doc);
					}
				}
#endif
				break;
			}
			case DataFOD::ATTR_UNKN:
			default:
				break;
			}
			++plcIt;
		}
		if (lastCId >= -1)
		{
			m_styleParser->sendFont(lastCId, special);
			lastCId = -2;
		}
		if (lastPId >= -1)
		{
			m_styleParser->sendParagraph(lastPId);
			lastPId = -2;
		}
		f << ":";
		if ((finalPos-pos)%2)
		{
			WPS_DEBUG_MSG(("WPS8Text::readText: ### len is odd\n"));
			throw libwps::ParseException();
		}
		while (!input->isEnd())
		{
			if (input->tell()+1 >= finalPos) break;

			uint16_t readVal = libwps::readU16(input);
			if (0x00 == readVal)
				continue;
			f << (char) readVal;

			switch (readVal)
			{
			case 0x9:
				m_listener->insertTab();
				break;

			case 0x0A:
				m_listener->insertEOL(true);
				break;

			case 0x0C:
				if (mainZone)
					mainParser().newPage(++actualPage);
				else
				{
					WPS_DEBUG_MSG(("WPS8Text::readText: find page break in auxilliary zone\n"));
					m_listener->insertEOL();
				}
				break;
			case 0x0D:
				m_listener->insertEOL();
				break;

			case 0x0E:
				m_listener->insertBreak(WPS_COLUMN_BREAK);
				break;

			case 0x1E: // checkme: non-breaking hyphen
				m_listener->insertUnicode(0x2011);
				break;

			case 0x1F: // non-breaking space ? ( old: optional breaking hyphen)
				m_listener->insertUnicode(0xA0);
				break;

			case 0x23:
				//	TODO: fields, pictures, etc.
				switch (special.m_type)
				{
				case WPS8TextStyle::FontData::T_None:
					m_listener->insertCharacter('#');
					break;
				case WPS8TextStyle::FontData::T_Footnote:
				case WPS8TextStyle::FontData::T_Endnote:
				{
					long fPos = input->tell()-2; // the note can be linked, so must retrieve the pos...
					if (m_state->m_notesMap.find(fPos) == m_state->m_notesMap.end())
					{
						WPS_DEBUG_MSG(("WPS8Text::readText can not find notes for position : %lx\n", fPos));
						break;
					}
					WPS8TextInternal::Notes const &note = *m_state->m_notesMap[fPos];
					if (!note.m_corr || note.m_note) break;
					WPSEntry nEntry = note.getCorrespondanceEntry(fPos);
					shared_ptr<WPSSubDocument> doc(new WPS8TextInternal::SubDocument(input, *this, nEntry));
					m_listener->insertNote(special.m_type==WPS8TextStyle::FontData::T_Footnote ? WPSContentListener::FOOTNOTE : WPSContentListener::ENDNOTE, doc);
					break;
				}
				case WPS8TextStyle::FontData::T_Field:
					switch (special.m_fieldType)
					{
					case WPS8TextStyle::FontData::F_PageNumber:
						m_listener->insertField(WPSContentListener::PageNumber);
						break;
					case WPS8TextStyle::FontData::F_Date:
					case WPS8TextStyle::FontData::F_Time:
					{
						std::string format = special.format();
						if (format.length())
							m_listener->insertDateTimeField(format.c_str());
						else
						{
							WPS_DEBUG_MSG(("WPS8Text::readText: unknown date/time format for position : %lX\n", pos));
						}
						break;
					}
					case WPS8TextStyle::FontData::F_None:
					default:
						m_listener->insertUnicode(0x263B);
						break;
					}
					special = WPS8TextStyle::FontData();
					break;
				default:
					m_listener->insertUnicode(0x263B);
				}
				break;

			case 0xfffc:
			{
				if (special.m_type != WPS8TextStyle::FontData::T_Object)
					break;
				long objPos = input->tell()-2;
				if (m_state->m_objectMap.find(objPos) == m_state->m_objectMap.end())
				{
					WPS_DEBUG_MSG(("WPSText::readText can not find EOB for position : %lX\n", objPos));
					break;
				}
				WPS8TextInternal::Object const &obj = m_state->m_objectMap.find(objPos)->second;
				if (obj.m_type == WPS8TextInternal::Object::Image)
					mainParser().sendObject(obj.m_size, obj.m_id, true);
				else if (obj.m_type == WPS8TextInternal::Object::Table)
					mainParser().sendTable(obj.m_size, obj.m_id);
				else
				{
					WPS_DEBUG_MSG(("WPSText::readText do not know how to send object in position : %lX\n", objPos));
				}
				break;
			}
			default:
				if (readVal < 28)
				{
					// do not add unprintable control which can create invalid odt file
					WPS_DEBUG_MSG(("WPS8Text::readText(find unprintable character: ignored)\n"));
					break;
				}
				m_listener->insertUnicode((uint32_t)readUTF16LE(input, finalPos, readVal));
				break;
			}
		}
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}
}

////////////////////////////////////////////////////////////
// send unsent zone
////////////////////////////////////////////////////////////
void WPS8Text::flushExtra()
{
	if (!m_listener) return;
	for (size_t i = 0; i < m_state->m_textZones.size(); i++)
	{
		WPSEntry const &zone = m_state->m_textZones[i];
		if (!zone.valid() || zone.id() == 2 || zone.id() ==3
		        || zone.isParsed())
			continue;
		readText(zone);
	}
}

/**
 * create the main structures
 */
bool WPS8Text::readStructures()
{
	WPS8Parser::NameMultiMap &nameTable = getNameEntryMap();
	WPS8Parser::NameMultiMap::iterator pos;
	/* What is the total length of the text? */
	pos = nameTable.lower_bound("TEXT");
	if (nameTable.end() == pos)
	{
		WPS_DEBUG_MSG(("Works: error: no TEXT in header index table\n"));
		return false;
	}
	else
		m_textPositions = pos->second;

	// determine the text subpart
	pos = nameTable.find("STRS");
	bool ok = nameTable.end() != pos && pos->second.hasType("PLC ");
	if (ok)
	{
		std::vector<long> textPtrs;
		std::vector<long> listValues;
		m_state->m_textZones.resize(0);
		ok = readPLC(pos->second, textPtrs, listValues, &WPS8Text::textZonesDataParser);
	}
	if (!ok)
	{
		WPS_DEBUG_MSG(("WPS8Text::readEntries: error: can not find the TEXT subdivision\n"));
		m_state->m_textZones.resize(0);
		// we create a false zone
		WPSEntry zone = m_textPositions;
		zone.setId(1);
		m_state->m_textZones.push_back(zone);
	}

	if (!m_styleParser->readStructures())
		return false;

	// BMKT : text position of bookmark ?
	pos = nameTable.lower_bound("BMKT");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos++->second;
		if (!entry.hasName("BMKT")) break;
		if (!entry.hasType("PLC ")) continue;

		std::vector<long> textPtrs;
		std::vector<long> listValues;

		if (!readPLC(entry, textPtrs, listValues,
		             &WPS8Text::defDataParser, &WPS8Text::bmktEndDataParser)) continue;
	}

	//
	// look for footnote end note
	//
	m_state->m_notesList.resize(0);
	pos = nameTable.lower_bound("FTN ");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos++->second;
		if (!entry.hasName("FTN ")) break;
		if (!entry.hasType("FTN ")) continue;

		readNotes(entry);
	}
	pos = nameTable.lower_bound("EDN ");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos++->second;
		if (!entry.hasName("EDN ")) break;
		if (!entry.hasType("EDN ")) continue;

		readNotes(entry);
	}
	createNotesCorrespondance();

	// read EOBJ zone : object position
	pos = nameTable.lower_bound("EOBJ");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos++->second;
		if (!entry.hasName("EOBJ")) break;
		if (!entry.hasType("PLC ")) continue;

		std::vector<long> textPtrs;
		std::vector<long> listValues;
		m_state->m_object = WPS8TextInternal::Object();
		if (!readPLC(entry, textPtrs, listValues, &WPS8Text::objectDataParser))
			continue;
	}

	// TOKN : text position of token
	pos = nameTable.lower_bound("TOKN");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos++->second;
		if (!entry.hasName("TOKN")) break;
		if (!entry.hasType("PLC ")) continue;

		std::vector<long> textPtrs;
		std::vector<long> listValues;

		if (!readPLC(entry, textPtrs, listValues,
		             &WPS8Text::defDataParser, &WPS8Text::tokenEndDataParser)) continue;
	}

	// TCD : table separator
	pos = nameTable.lower_bound("TCD ");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos++->second;
		if (!entry.hasName("TCD ")) break;
		if (!entry.hasType("PLC ")) continue;

		std::vector<long> textPtrs;
		std::vector<long> listValues;
		if (!readPLC(entry, textPtrs, listValues)) continue;
		m_state->m_tcdMap[entry.id()] = textPtrs;
	}

#ifdef DEBUG
	// read style sheet zone ? can be safety skipped in normal mode
	pos = nameTable.lower_bound("STSH");
	while (nameTable.end() != pos)
	{
		WPSEntry const &entry = pos++->second;
		if (!entry.hasName("STSH")) break;
		if (!entry.hasType("STSH")) continue;

		if (!m_styleParser->readSTSH(entry))
		{
			ascii().addPos(entry.begin());
			ascii().addNote("STSH##");
		}
	}
#endif
	return true;
}

////////////////////////////////////////////////////////////
// basic strings functions:
////////////////////////////////////////////////////////////
/* Read an UTF16 character in LE byte ordering, convert it. Courtesy of glib2 */
long WPS8Text::readUTF16LE(RVNGInputStreamPtr input, long endPos, uint16_t firstC)
{
	if (firstC >= 0xdc00 && firstC < 0xe000)
	{
		WPS_DEBUG_MSG(("WPS8Text::readUTF16LE: error: character = %i (0x%X)\n", firstC, firstC));
		return 0xfffd;
	}
	if (firstC >= 0xd800 && firstC < 0xdc00)
	{
		firstC = uint16_t(firstC - 0xd800);
		if (input->tell() == endPos)
		{
			WPS_DEBUG_MSG(("WPS8Text::readUTF16LE: error: find high surrogate without low\n"));
			return 0xfffd;
		}
		uint16_t lowVal = libwps::readU16(input);
		if (lowVal >= 0xdc00 && lowVal < 0xe000)
		{
			lowVal = uint16_t(lowVal - 0xdc00);
			return firstC* 0x400 + lowVal + 0x10000;
		}
		else
		{
			WPS_DEBUG_MSG(("WPS8Text::readUTF16LE: error: surrogate character = %i (0x%X)\n", firstC, firstC));
			return 0xfffd;
		}
	}
	/** CHECKME: for Symbol font, we must probably convert 0xF0xx and 0x00xx in a 0x03yy symbol :-~ */
	if (firstC>=28 ) return firstC;
	WPS_DEBUG_MSG(("WPS8Text::readUTF16LE: error: character = %i (0x%X)\n", firstC, firstC));
	return 0xfffd;
}

bool WPS8Text::readString(RVNGInputStreamPtr input, long page_size,
                          librevenge::RVNGString &res)
{
	res = "";
	long page_offset = input->tell();
	long endString = page_offset + page_size;

	while (input->tell() < endString-1 && !input->isEnd())
	{
		uint16_t val = libwps::readU16(input);
		if (!val) break;

		long unicode = readUTF16LE(input, endString, val);
		if (unicode != 0xfffd) WPSContentListener::appendUnicode(val, res);
	}
	return true;
}

////////////////////////////////////////////////////////////
// end note or footnote / FTN code
////////////////////////////////////////////////////////////
bool WPS8Text::readNotes(WPSEntry const &entry)
{
	if (!entry.hasType(entry.name()))
	{
		WPS_DEBUG_MSG(("WPS8Text::readNotes: warning: readNotes name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}

	if (entry.length() < 20)
	{
		WPS_DEBUG_MSG(("WPS8Text::readNotes: warning: readNotes length=0x%ld\n", entry.length()));
		return false;
	}


	RVNGInputStreamPtr input = getInput();
	long debPos = entry.begin();
	long length = entry.length();
	long endPos = entry.end();
	input->seek(debPos, librevenge::RVNG_SEEK_SET);

	long zone = (long) libwps::readU32(input);
	int numTZones = int(m_state->m_textZones.size());
	if (zone < 0 || zone >= numTZones)
	{
		WPS_DEBUG_MSG(("WPS8Text::readNotes: warning: readNotes unknown zone: %d, numZones = %d\n", int(zone), numTZones));
		return false;
	}

	// CHECK ME, it is the number of notes or only an unknow field
	int nNotes = (int) libwps::read32(input);
	if (nNotes < 0 || 16+4*(3*nNotes+1) > length)
	{
		WPS_DEBUG_MSG(("WPS8Text::readNotes: warning: readNotes number=%d\n", nNotes));
		return false;
	}
	libwps::DebugStream f;

	entry.setParsed();
	WPS8TextInternal::Notes notes;
	if (strncmp(entry.name().c_str(),"FTN ",3)==0)
		notes.m_type = WPSContentListener::FOOTNOTE;
	else
		notes.m_type = WPSContentListener::ENDNOTE;
	notes.m_zoneNote = entry.id();
	notes.m_zoneCorr = (int) zone;

	f << "unkn=" << libwps::readU32(input) << ", fl=(";

	for (int i = 0; i < 4; i++) f << (int) libwps::read8(input) << ",";
	f << "),";

	long begTPos = m_textPositions.begin();
	long endTPos = m_textPositions.end();

	if (entry.id() >= 0 && entry.id() < int(m_state->m_textZones.size()))
	{
		begTPos = m_state->m_textZones[size_t(entry.id())].begin();
		endTPos = m_state->m_textZones[size_t(entry.id())].end();
	}
	else
	{
		WPS_DEBUG_MSG(("WPS8Text::readNotes: unknown entry id = %d\n", int(entry.id())));
	}
	std::vector<long> &notesPos = notes.m_positions;
	for (int i = 0; i <= nNotes; i++)
	{
		long ptr = begTPos + 2*(long) libwps::read32(input);
		if (ptr >= begTPos && ptr <= endTPos) notesPos.push_back(ptr);
		else f << "###pbptr" << i << "=" << std::hex << ptr << std::dec << ",";
	}
	m_state->m_notesList.push_back(notes);

	ascii().addPos(debPos);
	libwps::DebugStream f2;
	f2 << notes << "," << f.str();
	ascii().addNote(f2.str().c_str());

	long pos;
	for (int i = 0; i < nNotes; i++)
	{
		pos = input->tell();
		f.str("");
		f << entry.name() << i << ":";
		int type = (int) libwps::read16(input);
		switch(type)
		{
		case 0:
			f << "alpha,";
			break;
		case 1:
			f << "numeric,";
			break;
		default:
			f << "#type=" << type << ",";
			break;
		}
		f << "id=" << libwps::read16(input) << ",";
		long val = (long) libwps::read32(input);
		if (val != -1) f << "label" << val << ",";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}

	// no sure what we can do of the end of the data ?
	pos = input->tell();
	if (pos+12 > endPos)
	{
		WPS_DEBUG_MSG(("WPS8Text::readNotes: unexpected end size\n"));
		f.str("");
		f << entry.name() << "[###End]";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	f.str("");
	f << entry.name() << "[A]:";
	for (int i = 0; i < 2; i++)   // 2 size ?
	{
		f << libwps::read32(input) << ",";
	}
	// fixme: how to read the end of data and find the labels ???
	ascii().addDelimiter(input->tell(),'|');
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	ascii().addPos(endPos);
	ascii().addNote("_");

	return true;
}

void WPS8Text::createNotesCorrespondance()
{
	using WPS8TextInternal::Notes;
	std::vector<Notes> &notesList = m_state->m_notesList;
	size_t numNotes = notesList.size();
	if (!numNotes) return;

	// a map to store notes and their correspondance
	std::map<Notes const *, int, Notes::Compare> nMap;
	std::map<Notes const *, int, Notes::Compare>::iterator nIt, nIt2;

	std::map<long, Notes *> &notesMap = m_state->m_notesMap;

	for (size_t i = 0; i < numNotes; i++)
	{
		int nSameNote = 0;
		if ((nIt = nMap.find(&notesList[i])) != nMap.end())
		{
			WPS_DEBUG_MSG(("WPS8Text::createNotesCorrespondance: find some dupplicated notes\n"));
			nSameNote = nIt->second+1;
		}
		nMap[&(notesList[i])] = nSameNote;
	}

	nIt = nMap.begin();

	while (nIt != nMap.end())
	{
		Notes &notes = const_cast<Notes &>(*nIt->first);
		if (nIt++->second != 0)
			continue;

		WPSEntry entry = getEntry(notes.m_zoneCorr);
		if (entry.id() != 2 && entry.id() != 3)
			// not the reference zone
			continue;

		// notes in notes are not actually accepted
		entry = getEntry(notes.m_zoneNote);
		if (entry.id() == 2 || entry.id() == 3)
		{
			WPS_DEBUG_MSG(("WPS8Text::createNotesCorrespondance: find a note contained in a note, ignored\n"));
			continue;
		}

		Notes correspondingNotes;
		correspondingNotes.m_type = notes.m_type;
		correspondingNotes.m_zoneNote = notes.m_zoneCorr;
		correspondingNotes.m_zoneCorr = notes.m_zoneNote;
		nIt2 = nMap.find(&correspondingNotes);
		if (nIt2 == nMap.end())
		{
			WPS_DEBUG_MSG(("WPS8Text::createNotesCorrespondance: can not find the corresponding note\n"));
			continue;
		}
		int nNotes = int(notes.m_positions.size())-1;
		if (int(nIt2->first->m_positions.size())-1 != nNotes || nNotes <= 0)
		{
			WPS_DEBUG_MSG(("WPS8Text::createNotesCorrespondance: the notes' lists have not the same number of elements \n"));
			continue;
		}

		if (nIt2->second != 0) continue; // a duplicated notes
		Notes &notesCor = const_cast<Notes &>(*nIt2->first);
		notes.m_corr = &notesCor;
		notes.m_note = false;
		notesCor.m_corr = &notes;
		notesCor.m_note = true;

		/** we can now creates the link position -> notes */
		for (size_t i = 0; i < size_t(nNotes); i++)
		{
			notesMap[notes.m_positions[i]] = &notes;
			notesMap[notesCor.m_positions[i]] = &notesCor;
		}

		m_state->m_textZones[size_t(notes.m_zoneCorr)].setParsed(true);
	}
}

////////////////////////////////////////////////////////////
// plc: default, strs
////////////////////////////////////////////////////////////
bool WPS8Text::defDataParser
(long , long , int , WPS8Struct::FileData const &data, std::string &mess)
{
	mess = "";
	libwps::DebugStream f;
	if (!data.isRead() && !data.readArrayBlock() && data.m_recursData.size() == 0)
	{
		// we read nothing -> we stop
		f << ", " << data;
		mess = f.str();
		return true;
	}

	size_t numChild = data.m_recursData.size();
	if (numChild == 0) return true;

	f << "{";
	for (size_t c = 0; c < numChild; c++) f << data.m_recursData[c] << ",";
	f << "}";

	mess = f.str();
	return true;
}

////////////////////////////////////////////////////////////
// Text zones/ STRS zone
////////////////////////////////////////////////////////////
bool WPS8Text::textZonesDataParser
(long bot, long eot, int /*nId*/, WPS8Struct::FileData const &data, std::string &mess)
{
	mess = "";
	if (bot < m_textPositions.begin() || eot > m_textPositions.end()) return false;

	libwps::DebugStream f;
	if (!data.isRead() && !data.readArrayBlock() && data.m_recursData.size() == 0)
	{
		f << ", " << data;
		mess = f.str();
		WPS_DEBUG_MSG(("WPS8Text::textZonesDataParser: unknown structure\n"));
		return false;
	}

	size_t numChild = data.m_recursData.size();

	bool idSet = false;
	int id = -1;

	for (size_t c = 0; c < numChild; c++)
	{
		WPS8Struct::FileData const &dt = data.m_recursData[c];
		if (dt.isBad()) continue;
		if (dt.id()!=0 || dt.type() != 0x22)
		{
			WPS_DEBUG_MSG(("WPS8Text::textZonesDataParser: unexpected id for %d[%d]\n", data.id(), data.type()));
			f << "###" << data << ",";
			continue;
		}
		/* 1=Main, 2=Footnote, 3=Endnote, 5=Section/Table/Column/..,
		   6=Header, 7=Footer */
		f << "id=" << dt.m_value << ",";
		id = (int) dt.m_value;
		idSet = true;
	}

	if (!idSet)
	{
		size_t numZ = m_state->m_textZones.size();
		if (numZ == 0) WPS_DEBUG_MSG(("WPS8Text::textZonesDataParser: error: readSTRS with no type\n"));
		else
		{
			id = m_state->m_textZones[numZ-1].id();
			f << "rId=" << id;
		}
	}

	mess = f.str();

	WPSEntry zone;
	zone.setBegin(bot);
	zone.setEnd(eot);
	zone.setType("Text");
	zone.setId(id);
	m_state->m_textZones.push_back(zone);

	return true;
}

////////////////////////////////////////////////////////////
// Bookmark/ BMKT zone
//
// Note: I only find in very few file -> to be checked
////////////////////////////////////////////////////////////
bool WPS8Text::bmktEndDataParser(long endPage, std::vector<long> const &textPtrs)
{
	RVNGInputStreamPtr input=getInput();
	typedef WPS8TextInternal::Bookmark Bookmark;

	int numBmkt = int(textPtrs.size())-1;
	if (numBmkt <= 0) return false;

	libwps::DebugStream f;

	long pos = input->tell();
	ascii().addPos(pos);

	f << "BMKT(id)=(";
	std::vector<Bookmark> listBmk;

	for (int i = 0; i < numBmkt; i++)
	{
		Bookmark bmk;

		pos = input->tell();
		if (input->tell() + 4 > endPage) return false;

		int id = (int) libwps::read32(input);
		f << id << ",";
		if (id < 0 || (id > numBmkt && id > 100))
		{
			WPS_DEBUG_MSG(("WPS8Text::bmktEndDataParser: odd index =%d\n", id));
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			return false;
		}
		bmk.m_id = id;
		listBmk.push_back(bmk);
	}
	f << ")";

	int sz1 = (int) libwps::readU32(input);
	pos = input->tell();
	if (sz1 < 4*numBmkt || pos+1+sz1 > endPage)
	{
		WPS_DEBUG_MSG(("WPS8Text::bmktEndDataParser: pb with sz1 =%d\n", sz1));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	f << ", Size(StrZone) = " << sz1;
	ascii().addNote(f.str().c_str());

	// ------ Problematic part -------------
	ascii().addPos(input->tell());
	f.str("");
	f << "BMKT(##unkn):";
	f << libwps::read32(input) <<",";
	f << "dim?=" << float(libwps::read32(input))/914400.f <<"," << std::hex;
	while(input->tell() < endPage-sz1 && !input->isEnd())
		f << libwps::read16(input) << ",";
	f << std::dec;
	ascii().addNote(f.str().c_str());
	// File1= 0b000000 (N?) - 4c891700:00000000:001fcd00 when numBmkt=11
	// File2= 01000000 (N?) - 988f1200:00000000:0045dd01 when numBmkt=1
	// probably int, dim(*914400), int, short, short
	// ------ end of problematic part -------------

	input->seek(endPage-sz1, librevenge::RVNG_SEEK_SET);

	pos = input->tell();
	ascii().addPos(pos);
	f.str("");
	f << "BMKT(strPos)=(";

	std::vector<long> strPos;
	for (int i = 0; i < numBmkt; i++)
	{
		long sPos = pos+(long) libwps::read32(input);

		if (sPos > endPage)
		{
			WPS_DEBUG_MSG(("WPS8Text::bmktEndDataParser: pb with %dth pos=%ld\n", i, sPos));
			return false;
		}
		strPos.push_back(sPos);
		f << std::hex << sPos << ",";
	}
	f << ")";

	ascii().addNote(f.str().c_str());
	ascii().addPos(input->tell());

	for (size_t i = 0; i < size_t(numBmkt); i++)
	{
		pos = strPos[i];
		f.str("");
		f << "BMKT(" << std::dec << i << ")=";
		if (pos == endPage) f << "_"; // can this happens ?
		else
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			librevenge::RVNGString val;
			int sz = (int) libwps::readU16(input);
			if (pos + 2+ 2*sz > endPage || !readString(input, 2*sz, val))
			{
				WPS_DEBUG_MSG(("WPS8Text::bmktEndDataParser: pb with %dth string size=%d\n", int(i), sz));
				return false;
			}
			listBmk[i].m_text=val;
			f << "\"" << val.cstr() << "\"";
		}
		ascii().addPos(strPos[i]);
		ascii().addNote(f.str().c_str());
	}
	input->seek(endPage, librevenge::RVNG_SEEK_SET);

	// ok, we store the bookmark and we create the plc and the data fod ...
	std::vector<DataFOD> fods;
	for (size_t t = 0; t < size_t(numBmkt); t++)
	{
		if (m_state->m_bookmarkMap.find(textPtrs[t]) != m_state->m_bookmarkMap.end())
		{
			WPS_DEBUG_MSG(("WPS8Text::bmktEndDataParser: warning: already a bkmt in position %lx\n", textPtrs[t]));
			continue;
		}
		else m_state->m_bookmarkMap[textPtrs[t]] = listBmk[t];
		int newId = int(m_state->m_plcList.size());

		WPS8TextInternal::DataPLC plc;
		plc.m_type = WPS8TextInternal::BMKT;
		plc.m_name = "BOOKMARK";
		f.str("");
		f << listBmk[t];
		plc.m_error = f.str();
		m_state->m_plcList.push_back(plc);

		DataFOD fod;
		fod.m_type = DataFOD::ATTR_PLC;
		fod.m_pos = textPtrs[t];
		fod.m_id = newId;

		fods.push_back(fod);
	}

	if (fods.size())
		m_FODList = mergeSortedFODLists(m_FODList, fods);

	return true;

}

////////////////////////////////////////////////////////////
// Object/ Eobj code
////////////////////////////////////////////////////////////
bool WPS8Text::objectDataParser(long bot, long /*eot*/, int /*id*/,
                                WPS8Struct::FileData const &data, std::string &mess)
{
	typedef WPS8TextInternal::Object Object;
	if (m_state->m_objectMap.find(bot) != m_state->m_objectMap.end())
	{
		WPS_DEBUG_MSG(("WPS8Text::objectDataParser: error: eobj already exists in this position\n"));
		return true;
	}

	Object obj = m_state->m_object;
	obj.m_error="";

	mess = "";
	size_t numChild = data.m_recursData.size();

	long val[5] = { 0, 0, 0, 0, 0 };
	bool setVal[5] = { false, false, false, false, false };
	libwps::DebugStream f;
	for (size_t c = 0; c < numChild; c++)
	{
		WPS8Struct::FileData const &dt = data.m_recursData[c];
		if (dt.isBad()) continue;
		if (m_state->m_objectTypes.find(dt.id())==m_state->m_objectTypes.end())
		{
			WPS_DEBUG_MSG(("WPS8Text::objectDataParser: unexpected id %d\n", dt.id()));
			f << "###" << dt << ",";
			continue;
		}
		if (m_state->m_objectTypes.find(dt.id())->second != dt.type())
		{
			WPS_DEBUG_MSG(("WPS8Text::objectDataParser: unexpected type for %d\n", dt.id()));
			f << "###" << dt << ",";
			continue;
		}
		val[dt.id()] = dt.m_value;
		setVal[dt.id()] = true;
	}
	obj.m_error = f.str();

	if (setVal[0])
	{
		if (val[0] == 2) obj.m_type = Object::Image;
		else if (val[0] == 3) obj.m_type = Object::Table;
		else
		{
			static bool first = true;
			if (first)
			{
				first = false;
				WPS_DEBUG_MSG(("WPS8Text::objectDataParser: unknown type: %ld\n", val[0]));
			}
			obj.m_type = int(-1 - val[0]);
		}
	}
	if (setVal[3]) obj.m_id = int(val[3]);

	if (setVal[1]) obj.m_size.setX(float(val[1])/914400.f);
	if (setVal[2]) obj.m_size.setY(float(val[2])/914400.f);

	if (setVal[4]) obj.m_unknown = val[4];

	m_state->m_objectMap[bot] = m_state->m_object = obj;

	f.str("");
	f << obj;
	mess = f.str();

	return true;
}

////////////////////////////////////////////////////////////
// Token/ Tokn zone
////////////////////////////////////////////////////////////
bool WPS8Text::tokenEndDataParser(long endPage, std::vector<long> const &textPtrs)
{
	int numTokn = int(textPtrs.size())-1;
	if (numTokn <= 0) return false;

	RVNGInputStreamPtr input = getInput();
	libwps::DebugStream f;
	WPS8TextInternal::Token tokn;
	std::vector<WPS8TextInternal::Token> listToken;

	long pos = input->tell();
	for (size_t i = 0; i < size_t(numTokn); i++)
	{
		pos = input->tell();
		if (input->tell() + 2 > endPage) return false;

		int sz = (int) libwps::read16(input);
		if (sz < 2 || pos+sz > endPage)
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			return false;
		}

		WPS8Struct::FileData data;
		std::string error;
		if (!readBlockData(input, pos+sz, data, error) && data.m_recursData.size() == 0)
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			return false;
		}

		size_t numChild = data.m_recursData.size();
		tokn.m_error ="";
		f.str("");
		for (size_t c = 0; c < numChild; c++)
		{
			WPS8Struct::FileData const &dt = data.m_recursData[c];
			if (dt.isBad()) continue;
			if (dt.id()<0 || dt.id()>2 || dt.type()!=0x22)
			{
				WPS_DEBUG_MSG(("WPS8Text::tokenEndDataParser[A]: unexpected id %d[%d]\n", data.id(), data.type()));
				f << "###" << dt << ",";
				continue;
			}
			switch(dt.id())
			{
			case 0: // some unknown flags ? link=0x140, pageNumber=0, unknown(6)=0x400
				tokn.m_unknown=int(dt.m_value);
				break;
			case 1: // looks like the text size
				tokn.m_textLength=int(dt.m_value);
				break;
			case 2: // -2=link, -5=page number, -6=?
				switch(dt.m_value)
				{
				case -2:
					tokn.m_type = WPSContentListener::Link;
					break;
				case -5:
					tokn.m_type = WPSContentListener::PageNumber;
					break;
				// CHECKME: case -6-> strings = SPC, text character = 0xb7
				// an insecable character or space ?
				default:
					WPS_DEBUG_MSG(("WPS8Text::tokenEndDataParser: unknown type=%d\n", int(dt.m_value)));
					f << "###type=" << dt.m_value << ",";
				}
				break;
			default:
				f << "###id=" << dt.id() << ",";
				break;
			}
		}

		if (!error.empty()) f << error;
		if (!f.str().empty()) tokn.m_error = f.str();

		listToken.push_back(tokn);

		f.str("");
		f << std::dec << "TOKN/PLC" << i << ": " << tokn;
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}

	std::vector<int> idString;
	idString.resize(size_t(numTokn), -1);

	int numFollow = 0;
	for (int i = 0; i < numTokn; i++)
	{
		pos = input->tell();
		if (input->tell() + 2 > endPage) return false;

		WPS8Struct::FileData data;
		int sz = (int) libwps::read16(input);
		if (sz < 2 || pos+sz > endPage)
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			return false;
		}

		std::string error;
		if (!readBlockData(input, pos+sz, data, error)  && data.m_recursData.size() == 0)
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			return false;
		}

		f.str("");
		f << "TOKN(PLC" << i << "-a): ";

		bool idSet = false;
		int id = -1;
		size_t numChild = data.m_recursData.size();;
		for (size_t c = 0; c < numChild; c++)
		{
			WPS8Struct::FileData const &dt = data.m_recursData[c];
			if (dt.isBad()) continue;
			if (dt.id() != 0 || dt.type()!=0x22)
			{
				WPS_DEBUG_MSG(("WPS8Text::tokenEndDataParser[B]: unexpected id %d[%d]\n", data.id(), data.type()));
				f << "###" << dt << ",";
				continue;
			}

			id = int(dt.m_value);
			if (id >= 0)
			{
				idString[size_t(i)] = id;
				f << "strings" << id << ","; // id of the following strs
			}
			else if (id == -1) f << "_,";   // no string
			else f << "###id=" << id << ","; // exits ???
			idSet = true;
		}

		if (idSet && id >= 0) numFollow++;
		if (!error.empty()) f << error;

		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}

	std::vector<long> sPtrs;
	if (numFollow != 0)
	{
		// read the strings header
		pos = input->tell();
		if (pos+20 > endPage) return false;
		f.str("");
		f << "TOKN(strings):";
		long unkn = (long) libwps::read32(input);
		if (unkn+20+pos != endPage)
			f << "unkn=" << std::hex << unkn+20+pos << std::dec << ",";
		int N = (int) libwps::read32(input);
		if (N != numFollow) f << "###";
		f << "N=" << N << ",";
		// 8760a4 | b37464 | 6092ea37 | 6105db0e | ffffffff
		f << std::hex << libwps::readU32(input) << std::dec << ",";
		for (int i = 0; i <2; i++)
		{
			int val = (int) libwps::read32(input);
			if (val) f << "###f" << i << "=" << val << ",";
		}
		f << "ptr=(";

		if (pos+20+4*N > endPage)
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			return false;
		}

		for (int i = 0; i < N; i++)
		{
			long val = (long) libwps::read32(input);
			if (val < 4*N)
			{
				input->seek(pos, librevenge::RVNG_SEEK_SET);
				return false;
			}
			val += pos+20;
			if (val > endPage)
			{
				input->seek(pos, librevenge::RVNG_SEEK_SET);
				return false;
			}
			sPtrs.push_back(val);
			f << std::hex << val << ",";
		}
		f << ")" << std::dec;

		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}

	size_t numStrings = sPtrs.size();
	for (size_t i = 0; i < numStrings; i++)
	{
		pos = sPtrs[i];
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		int sz = (int) libwps::read16(input);
		if (sz < 0 || pos+2*sz+2 > endPage) return false;
		librevenge::RVNGString str;
		readString(input, 2*sz, str);

		bool find = false;
		for (size_t t = 0; t < size_t(numTokn); t++)
		{
			if (idString[t] != int(i)) continue;
			find = true;
			listToken[t].m_text = str;
		}
		f.str("");
		f << "TOKN/string" << i << ":" << str.cstr();
		if (!find) f << ", ###not find";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}

	// ok, we store the token and we create the plc and the data fod ...
	std::vector<DataFOD> fods;
	for (size_t t = 0; t < size_t(numTokn); t++)
	{
		if (m_state->m_tokenMap.find(textPtrs[t]) != m_state->m_tokenMap.end())
		{
			WPS_DEBUG_MSG(("WPS8Text::tokenEndDataParser: warning: already a tokn in position %lx\n", textPtrs[t]));
			continue;
		}
		else m_state->m_tokenMap[textPtrs[t]] = listToken[t];
		int newId = int(m_state->m_plcList.size());

		WPS8TextInternal::DataPLC plc;
		plc.m_type = WPS8TextInternal::TOKEN;
		plc.m_name = "TOKEN";
		f.str("");
		f << listToken[t];
		plc.m_error = f.str();
		m_state->m_plcList.push_back(plc);

		DataFOD fod;
		fod.m_type = DataFOD::ATTR_PLC;
		fod.m_pos = textPtrs[t];
		fod.m_id = newId;

		fods.push_back(fod);
	}

	if (fods.size())
		m_FODList = mergeSortedFODLists(m_FODList, fods);

	return true;
}

/////////////////////////////////////////////////////////////////////
//                    VERY LOW LEVEL (PLC)                         //
/////////////////////////////////////////////////////////////////////

/** Internal and low level: the structures of a WPS4Text used to parse PLC*/
namespace WPS8PLCInternal
{
/** Internal and low level: the PLC different types and their structures */
struct PLC
{
	/** the PLC types */
	typedef enum WPS8TextInternal::PLCType PLCType;
	/** the way to define the text positions
	 *
	 * - P_ABS: absolute position,
	 * - P_MREL: position are relative to the beginning of the main text offset,
	 * - P_MINCR: position are the length of text consecutive zones in the main text offset,
	 * - P_ZREL: position are relative to the beginning of a specific text offset,
	 * - P_ZINCR: position are the length of text consecutive zones in a specific text offset
	 */
	typedef enum { P_ABS=0, P_MREL, P_ZREL, P_MINCR, P_ZINCR, P_UNKNOWN} Position;
	/** the type of the content
	 *
	 * - T_CST: size is constant
	 * - T_STRUCT: a structured type ( which unknown size)
	 * - T_COMPLEX: a complex way to define content in many parts
	 */
	typedef enum { T_CST=0, T_STRUCT,  T_COMPLEX, T_UNKNOWN} Type;

	//! constructor
	PLC(PLCType w= WPS8TextInternal::Unknown, Position p=P_UNKNOWN, Type t=T_UNKNOWN) :
		m_type(w), m_pos(p), m_contentType(t) {}

	//! PLC type
	PLCType m_type;
	//! the way to define the text positions
	Position m_pos;
	//! the type of the content
	Type m_contentType;
};

KnownPLC::KnownPLC() : m_knowns()
{
	createMapping();
}

KnownPLC::~KnownPLC()
{
}

PLC KnownPLC::get(std::string const &name)
{
	std::map<std::string, PLC>::iterator pos = m_knowns.find(name);
	if (pos == m_knowns.end()) return PLC();
	return pos->second;
}

void KnownPLC::createMapping()
{
	m_knowns["BTEP"]=PLC(WPS8TextInternal::BTE,PLC::P_ABS, PLC::T_CST);
	m_knowns["BTEC"]=PLC(WPS8TextInternal::BTE,PLC::P_ABS, PLC::T_CST);
	m_knowns["TCD "]=PLC(WPS8TextInternal::TCD,PLC::P_ZREL, PLC::T_CST);
	m_knowns["EOBJ"]=PLC(WPS8TextInternal::OBJECT,PLC::P_ZREL, PLC::T_STRUCT);  // or maybe  P_MREL
	m_knowns["STRS"]=PLC(WPS8TextInternal::STRS, PLC::P_MINCR, PLC::T_STRUCT);
	m_knowns["TOKN"]=PLC(WPS8TextInternal::TOKEN, PLC::P_ZREL, PLC::T_COMPLEX);
	m_knowns["BMKT"]=PLC(WPS8TextInternal::BMKT, PLC::P_ZREL, PLC::T_COMPLEX); // or maybe P_MREL
}
}

bool WPS8Text::readPLC
(WPSEntry const &entry,
 std::vector<long> &textPtrs, std::vector<long> &listValues,
 WPS8Text::DataParser parser, WPS8Text::EndDataParser endParser)
{
	RVNGInputStreamPtr input = getInput();
	if (!entry.hasType("PLC "))
	{
		WPS_DEBUG_MSG(("WPS8Text::readPLC: warning: PLC name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}

	long page_offset = entry.begin();
	long length = entry.length();
	long endPage = entry.end();
	if (length < 16)
	{
		WPS_DEBUG_MSG(("WPS8Text::readPLC: warning: PLC length=0x%lx\n", length));
		return false;
	}

	input->seek(page_offset, librevenge::RVNG_SEEK_SET);
	int nPLC = (int) libwps::readU32(input);
	int dataSz = (int) libwps::read32(input);
	libwps::DebugStream f;
	f << "PLC: N=" << nPLC << ", dSize=" << dataSz;
	WPS8PLCInternal::PLC plcType = m_state->m_knownPLC.get(entry.name());

	if (plcType.m_contentType == WPS8PLCInternal::PLC::T_UNKNOWN && 4*nPLC+16 + dataSz *nPLC == length)
		plcType.m_contentType = WPS8PLCInternal::PLC::T_CST;
	else if (plcType.m_contentType == WPS8PLCInternal::PLC::T_CST)
	{
		if (4*nPLC+16 + dataSz *nPLC != length)
		{
			WPS_DEBUG_MSG(("WPS8Text::readPLC: warning: unknown data size\n"));
			dataSz = 0;
			f << ", ###data=Unkn";
		}
	}
	else dataSz = 0;

	if (4*nPLC+16 + dataSz*nPLC > length)
	{
		WPS_DEBUG_MSG(("WPS8Text::readPLC: warning: PLC length=0x%lx, N=%d\n", length, nPLC));
		return false;
	}
	entry.setParsed();

	int fl[4];
	for (int i = 0; i < 4; i++)
	{
		fl[i] = (int)libwps::read8(input);
		if (fl[i] == 0) continue;
		f << ", f"<<i<<"=" << fl[i];
	}

	// find the zone
	WPSEntry textZone = m_textPositions;
	if (plcType.m_pos == WPS8PLCInternal::PLC::P_ZREL
	        || plcType.m_pos == WPS8PLCInternal::PLC::P_ZINCR)
	{
		if (entry.id() >= 0 && entry.id() < int(m_state->m_textZones.size()))
			textZone = m_state->m_textZones[size_t(entry.id())];
		else
		{
			WPS_DEBUG_MSG(("WPS8Text::readPLC: warning: can not find the zone\n"));
			f << ", ###zone=" << entry.id();
		}
	}

	// read text pointer
	std::vector<WPS8Text::DataFOD> fods;
	textPtrs.resize(0);
	long lastPtr = textZone.begin();
	f << ",pos = (";
	for (int i = 0; i <= nPLC; i++)
	{
		long pos = (long) libwps::readU32(input);
		switch (plcType.m_pos)
		{
		case WPS8PLCInternal::PLC::P_ABS:
			if (pos == 0) pos = textZone.begin();
			break;
		case WPS8PLCInternal::PLC::P_ZREL:
		case WPS8PLCInternal::PLC::P_MREL:
			pos = 2*pos+textZone.begin();
			break;
		case WPS8PLCInternal::PLC::P_MINCR:
		case WPS8PLCInternal::PLC::P_ZINCR:
		{
			long newPos = lastPtr + 2*pos;
			pos = lastPtr;
			lastPtr = newPos;
			break;
		}
		case WPS8PLCInternal::PLC::P_UNKNOWN:
		default:
			break;
		}
		bool ok = pos >= textZone.begin() && pos <= textZone.end();
		if (!ok)
		{
			WPS_DEBUG_MSG(("WPS8Text::readPLC: warning: can not find pos\n"));
			f << "###";
		}
		f << std::hex << pos << ",";

		DataFOD fod;
		fod.m_type = DataFOD::ATTR_PLC;
		fod.m_pos = ok ? pos : 0;

		textPtrs.push_back(fod.m_pos);
		if (i != nPLC) fods.push_back(fod);
	}
	f << ")";


	// read data
	libwps::DebugStream f2;
	bool ok = true;

	listValues.resize(0);
	long pos = input->tell();
	for (size_t i = 0; i < size_t(nPLC); i++)
	{
		if (plcType.m_contentType == WPS8PLCInternal::PLC::T_COMPLEX) break;
		WPS8TextInternal::DataPLC plc;
		plc.m_type = plcType.m_type;
		plc.m_name = entry.name();
		bool printPLC = true;
		switch(plcType.m_contentType)
		{
		case WPS8PLCInternal::PLC::T_CST :
		{
			if (dataSz == 0)
			{
				printPLC = false;
				break;
			}
			if (dataSz > 4 || dataSz==3)
			{
				f2.str("");
				for (int j = 0; j < dataSz; j++)
					f2 << std::hex << int(libwps::readU8(input)) << ",";
				plc.m_error = f2.str();
			}
			else
			{
				plc.m_value = dataSz==1 ? (long) libwps::readU8(input) :
				              dataSz==2 ? (long) libwps::readU16(input) :
				              (long) libwps::readU32(input);
				listValues.push_back(plc.m_value);
			}
			break;
		}
		case WPS8PLCInternal::PLC::T_STRUCT :
		{
			long sz = (long) libwps::read16(input);
			if (sz < 2 || sz+pos > endPage)
			{
				ok = false;
				break;
			}

			WPS8Struct::FileData mainData;
			std::string error;
			readBlockData(input, sz+pos,mainData, error);

			if (parser)
			{
				std::string mess;
				if (!(this->*parser)(textPtrs[i], textPtrs[i+1], int(i), mainData, mess))
				{
					ok = false;
					break;
				}
				plc.m_error = mess;
			}
			else
			{
				f2.str("");
				f2 << mainData;
				if (!error.empty()) f2 << ", ###" << error;
				plc.m_error = f2.str();
			}
			break;
		}
		case WPS8PLCInternal::PLC::T_COMPLEX:
		case WPS8PLCInternal::PLC::T_UNKNOWN:
		default:
			ok = false;
			printPLC= false;
			break;
		}

		const int plcID = (int) m_state->m_plcList.size();
		if (i < fods.size())
		{
			fods[i].m_id = plcID;
			if (ok) fods[i].m_defPos = pos;
		}
		m_state->m_plcList.push_back(plc);

		if (printPLC)
		{
			f2.str("");
			f2 << plc.m_name << i << "(PLC"<<plcID<<"):" << plc;
			ascii().addPos(pos);
			ascii().addNote(f2.str().c_str());
		}

		if (ok) pos = input->tell();
		else plcType.m_contentType = WPS8PLCInternal::PLC::T_UNKNOWN;
	}
	if (fods.size())
		m_FODList = mergeSortedFODLists(m_FODList, fods);

	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());

	if (pos == endPage) return ok;

	if (ok && endParser)
	{
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		ok = (this->*endParser) (endPage, textPtrs);
		pos = input->tell();
		if (pos == endPage) return ok;
	}

	ascii().addPos(pos);

	f.str("");
	f << "###" << entry.name() << "/PLC";
	ascii().addNote(f.str().c_str());
	return ok;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
