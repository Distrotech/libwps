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

#include <map>

#include "libwps_internal.h"

#include "WPSContentListener.h"
#include "WPSEntry.h"
#include "WPSList.h"
#include "WPSPageSpan.h"
#include "WPSParagraph.h"
#include "WPSSubDocument.h"

#include "WPS8.h"
#include "WPS8Struct.h"

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
	WPXString m_text;
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
	WPXString m_text;
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
	State() : m_textZones(), m_fontNames(),
		m_fontList(), m_paragraphList(),
		m_bookmarkMap(), m_notesList(), m_notesMap(),
		m_object(), m_objectMap(), m_tokenMap(),
		m_characterTypes(), m_paragraphTypes(), m_objectTypes(),
		m_plcList(), m_knownPLC()
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

	//! the list of different text zones
	std::vector<WPSEntry> m_textZones;

	//! the font names
	std::vector<std::string> m_fontNames;

	//! a list of all font properties
	std::vector<WPS8Struct::FileData> m_fontList;
	//! a list of paragraph properties
	std::vector<WPS8Struct::FileData> m_paragraphList;

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

	//! the character type
	std::map<int,int> m_characterTypes;
	//! the paragraph type
	std::map<int,int> m_paragraphTypes;
	//! the object type
	std::map<int,int> m_objectTypes;
	//! a list of all plcs
	std::vector<DataPLC> m_plcList;
	//! the known plc
	WPS8PLCInternal::KnownPLC m_knownPLC;
};

void State::initTypeMaps()
{
	static int const characterTypes[] =
	{
		0, 0x12, 2, 0x2, 3, 0x2, 4, 0x2, 5, 0x2,
		0xc, 0x22, 0xf, 0x12,
		0x10, 0x2, 0x12, 0x22, 0x13, 0x2, 0x14, 0x2, 0x15, 0x2, 0x16, 0x2, 0x17, 0x2,
		0x18, 0x22, 0x1a, 0x22, 0x1b, 0x22, 0x1e, 0x12,
		0x22, 0x22, 0x23, 0x22, 0x24, 0x8A,
		0x2d, 0x2, 0x2e, 0x22,
	};
	for (int i = 0; i+1 < int(sizeof(characterTypes)/sizeof(int)); i+=2)
		m_characterTypes[characterTypes[i]] = characterTypes[i+1];
	static int const paragraphTypes[] =
	{
		2, 0x22, 3, 0x1A, 4, 0x12, 6, 0x22,
		0xc, 0x22, 0xd, 0x22, 0xe, 0x22,
		0x12, 0x22, 0x13, 0x22, 0x14, 0x22, 0x15, 0x22, 0x17, 0x2,
		0x18, 0x2, 0x19, 0x1A, 0x1b, 0x2, 0x1c, 0x2, 0x1d, 0x2, 0x1e, 0x12, 0x1f, 0x22,
		0x20, 0x12, 0x21, 0x22, 0x22, 0x22, 0x23, 0x22, 0x24, 0x22, 0x25, 0x12,
		0x2a, 0x12,
		0x31, 0x12, 0x32, 0x82, 0x33, 0x12, 0x34, 0x22
	};
	for (int i = 0; i+1 < int(sizeof(paragraphTypes)/sizeof(int)); i+=2)
		m_paragraphTypes[paragraphTypes[i]] = paragraphTypes[i+1];
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
	//! constructor for a text entry
	SubDocument(WPXInputStreamPtr input, WPS8Text &pars, WPSEntry const &entry) :
		WPSSubDocument(input, 0), m_textParser(&pars), m_entry(entry) {}
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
		return true;
	}

	//! the parser function
	void parse(WPSContentListenerPtr &listener, libwps::SubDocumentType type);

	WPS8Text *m_textParser;
	WPSEntry m_entry;
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
	if (!dynamic_cast<WPS8ContentListener *>(listener.get()))
	{
		WPS_DEBUG_MSG(("SubDocument::parse: bad listener\n"));
		return;
	}
	WPS8ContentListenerPtr &listen =  reinterpret_cast<WPS8ContentListenerPtr &>(listener);

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
	m_input->seek(actPos, WPX_SEEK_SET);
}

}

/*
WPS8Text public
*/

WPS8Text::WPS8Text(WPS8Parser &parser) : WPSTextParser(parser, parser.getInput()),
	m_listener(), m_state()
{
	m_state.reset(new WPS8TextInternal::State);
}

WPS8Text::~WPS8Text ()
{
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
	WPXInputStreamPtr input = const_cast<WPS8Text *>(this)->getInput();
	long actPos = input->tell();
	input->seek(res.begin(), WPX_SEEK_SET);
	bool empty = libwps::read16(input) == 0xd;
	input->seek(actPos, WPX_SEEK_SET);
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
	WPXInputStreamPtr input = const_cast<WPS8Text *>(this)->getInput();
	long actPos = input->tell();
	input->seek(res.begin(), WPX_SEEK_SET);
	bool empty = libwps::read16(input) == 0xd;
	input->seek(actPos, WPX_SEEK_SET);
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


void WPS8Text::parse(WPXDocumentInterface *documentInterface)
{

	WPS_DEBUG_MSG(("WPS8Text::parse()\n"));
	if (!m_input)
	{
		WPS_DEBUG_MSG(("WPS8Text::parse: does not find main ole\n"));
		throw(libwps::ParseException());
	}

	try
	{
		/* parse pages */
		std::vector<WPSPageSpan> pageList;
		parsePages(pageList, m_input);

		/* parse document */
		m_listener.reset(new WPS8ContentListener(pageList, documentInterface));
		parse();
		m_listener.reset();
	}
	catch (...)
	{
		WPS_DEBUG_MSG(("WPS8Text::parse: exception catched when parsing CONTENTS\n"));
		throw(libwps::ParseException());
	}
}



/**
 * Read the range of the document text using previously-read
 * formatting information, up to but excluding entry.end().
 *
 */
void WPS8Text::readText(WPSEntry const &entry)
{
	WPXInputStreamPtr input = getInput();
	entry.setParsed(true);
	int lastCId=-1, lastPId=-1;
	std::vector<DataFOD>::iterator plcIt =	m_FODList.begin();
	while (plcIt != m_FODList.end() && plcIt->m_pos < entry.begin())
	{
		DataFOD const &plc = *plcIt;
		if (plc.m_type==DataFOD::ATTR_TEXT)
			lastCId = plc.m_id;
		else if (plc.m_type==DataFOD::ATTR_PARAG)
			lastPId = plc.m_id;
		plcIt++;
	}

	uint16_t specialCode=0;
	int fieldType = 0;
	input->seek(entry.begin(), WPX_SEEK_SET);
	while (!input->atEOS())
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
				lastCId = -1;
				if (plc.m_id < 0)
				{
					f << "[F_]";
					break;
				}
				f << "[F" << plc.m_id << "]";
				if (plc.m_id >= int(m_state->m_fontList.size()))
				{
					f << "#";
					WPS_DEBUG_MSG(("WPS8Text::readText: can not find font %d\n",plc.m_id));
					break;
				}
				propertyChange(m_state->m_fontList[size_t(plc.m_id)], specialCode, fieldType);
				break;
			}
			case DataFOD::ATTR_PARAG:
			{
				lastPId = -1;
				if (plc.m_id < 0)
				{
					f << "[P_]";
					break;
				}
				f << "[P" << plc.m_id << "]";
				if (plc.m_id >= int(m_state->m_paragraphList.size()))
				{
					f << "#";
					WPS_DEBUG_MSG(("WPS8Text::readText: can not find paragraph %d\n",plc.m_id));
					break;
				}
				propertyChangePara(m_state->m_paragraphList[size_t(plc.m_id)]);
				break;
			}
			case DataFOD::ATTR_PLC:
				if (plc.m_id < 0) break;
				if (plc.m_id >= int(m_state->m_plcList.size()))
				{
					f << "#[PLC"<< plc.m_id << "]";
					WPS_DEBUG_MSG(("WPS8Text::readText: can not find plc %d\n",plc.m_id));
					break;
				}
				f << "[" << m_state->m_plcList[size_t(plc.m_id)] << "]";
				break;
			case DataFOD::ATTR_UNKN:
			default:
				break;
			}
			plcIt++;
		}
		if (lastCId >= 0 && lastCId < int(m_state->m_fontList.size()))
		{
			propertyChange(m_state->m_fontList[size_t(lastCId)], specialCode, fieldType);
			lastCId = -1;
		}
		if (lastPId >= 0 && lastPId < int(m_state->m_paragraphList.size()))
		{
			propertyChangePara(m_state->m_paragraphList[size_t(lastPId)]);
			lastPId = -1;
		}
		f << ":";
		if ((finalPos-pos)%2)
		{
			WPS_DEBUG_MSG(("WPS8Text::readText: ### len is odd\n"));
			throw libwps::ParseException();
		}
		while (!input->atEOS())
		{
			if (input->tell()+1 >= finalPos) break;

			uint16_t readVal = libwps::readU16(input);
			if (0x00 == readVal)
				continue;
			f << (char) readVal;

			switch (readVal)
			{
			case 0x9:
				//Fixme: use m_listener->insertTab();
				m_listener->insertCharacter('\t');
				break;

			case 0x0A:
				break;

			case 0x0C:
				//fixme: add a page to list of pages
				//m_listener->insertBreak(WPS_PAGE_BREAK);
				break;

			case 0x0D:
				m_listener->insertEOL();
				break;

			case 0x0E:
				m_listener->insertBreak(WPS_COLUMN_BREAK);
				break;

			case 0x1E:
				//fixme: non-breaking hyphen
				break;

			case 0x1F:
				//fixme: optional breaking hyphen
				break;

			case 0x23:
				if (specialCode)
				{
					//	TODO: fields, pictures, etc.
					switch (specialCode)
					{
					case 3:
					case 4:
					{
						if (m_state->m_notesMap.find(pos) == m_state->m_notesMap.end())
						{
							WPS_DEBUG_MSG(("WPS8Text::readText can not find notes for position : %lx\n", pos));
							break;
						}
						WPS8TextInternal::Notes const &note = *m_state->m_notesMap[pos];
						if (!note.m_corr || note.m_note) break;
						WPSEntry nEntry = note.getCorrespondanceEntry(pos);
						shared_ptr<WPSSubDocument> doc(new WPS8TextInternal::SubDocument(input, *this, nEntry));
						m_listener->insertNote(specialCode==3 ? WPSContentListener::FOOTNOTE : WPSContentListener::ENDNOTE, doc);
						break;
					}
					case 5:
						switch (fieldType)
						{
						case -1:
							m_listener->insertField(WPSContentListener::PageNumber);
							break;
						case -4:
							m_listener->insertField(WPSContentListener::Date);
							break;
						case -5:
							m_listener->insertField(WPSContentListener::Time);
							break;
						default:
							break;
						}
						break;
					default:
						m_listener->insertUnicode(0x263B);
					}
					break;
				}
				// ! fallback to default

			case 0xfffc:
				// ! fallback to default

			default:
				if (readVal < 28 && readVal != 9)
				{
					// do not add unprintable control which can create invalid odt file
					WPS_DEBUG_MSG(("WPS8Text::readText(find unprintable character: ignored)\n"));
					break;
				}
				m_listener->insertUnicode(readUTF16LE(input, finalPos, readVal));
				break;
			}
			specialCode = 0;
		}
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}
}

/**
 * create the main structures
 */
bool WPS8Text::readStructures(WPXInputStreamPtr)
{
	WPS8Parser::NameMultiMap &nameTable = mainParser().getNameEntryMap();
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

	/* read fonts table */
	pos = nameTable.find("FONT");
	if (nameTable.end() == pos)
	{
		WPS_DEBUG_MSG(("WPS8Text::parse: error: no FONT in header index table\n"));
		return false;
	}
	readFontNames(pos->second);

	// find the FDDP and FDPC positions
	for (int st = 0; st < 2; st++)
	{
		std::vector<WPSEntry> zones;
		if (!findFDPStructures(st, zones))
			findFDPStructuresByHand(st, zones);

		size_t numZones = zones.size();
		std::vector<DataFOD> fdps;
		FDPParser parser = st==0  ? (FDPParser) &WPS8Text::readParagraph
		                   : (FDPParser) &WPS8Text::readFont;
		for (size_t i = 0; i < numZones; i++)
			readFDP(zones[i], fdps, parser);
		m_FODList = mergeSortedFODLists(m_FODList, fdps);
	}

	// BMKT : text position of bookmark ?
	pos = nameTable.lower_bound("BMKT");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos->second;
		pos++;
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
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("FTN ")) break;
		if (!entry.hasType("FTN ")) continue;

		readNotes(entry);
	}
	pos = nameTable.lower_bound("EDN ");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("EDN ")) break;
		if (!entry.hasType("EDN ")) continue;

		readNotes(entry);
	}
	createNotesCorrespondance();

	// read EOBJ zone : object position
	pos = nameTable.lower_bound("EOBJ");
	while (pos != nameTable.end())
	{
		WPSEntry const &entry = pos->second;
		pos++;
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
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("TOKN")) break;
		if (!entry.hasType("PLC ")) continue;

		std::vector<long> textPtrs;
		std::vector<long> listValues;

		if (!readPLC(entry, textPtrs, listValues,
		             &WPS8Text::defDataParser, &WPS8Text::tokenEndDataParser)) continue;
	}

#ifdef DEBUG
	// read style sheet zone ? can be safety skipped in normal mode
	pos = nameTable.lower_bound("STSH");
	while (nameTable.end() != pos)
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName("STSH")) break;
		if (!entry.hasType("STSH")) continue;

		if (!readSTSH(entry))
		{
			ascii().addPos(entry.begin());
			ascii().addNote("STSH##");
		}
	}
#endif
	return true;
}

/**
 * Read the page format from the file.  It seems that WPS8Text files
 * can only have one page format throughout the whole document.
 *
 */
void WPS8Text::parsePages(std::vector<WPSPageSpan> &pageList, WPXInputStreamPtr & /* input */)
{
	//fixme: this method doesn't do much

	/* record page format */
	WPSPageSpan ps;
	pageList.push_back(ps);
}

void WPS8Text::parse()
{
	WPS_DEBUG_MSG(("WPS8Text::parse()\n"));

	m_listener->startDocument();
	for (size_t i = 0; i < m_state->m_textZones.size(); i++)
	{
		WPSEntry const &zone = m_state->m_textZones[i];
		if (!zone.valid() || zone.id()==2 || zone.id()==3)
			continue;
		readText(zone);
	}
	m_listener->endDocument();
}

/**
 * Process a character property change.  The Works format supplies
 * all the character formatting each time there is any change (as
 * opposed to HTML, for example).  In Works 8, the position in
 * rgchProp is not significant because there are some kind of
 * codes.
 *
 */
void WPS8Text::propertyChange(WPS8Struct::FileData const &mainData, uint16_t &specialCode, int &fieldType)
{
	/* set default properties */
	uint32_t textAttributeBits = 0;
	m_listener->setTextColor(0);
	m_listener->setFontAttributes(0);
	m_listener->setFontSize(10);

	libwps::DebugStream f;
	if (mainData.m_value) f << "unk=" << mainData.m_value << ",";

	for (size_t c = 0; c < mainData.m_recursData.size(); c++)
	{
		WPS8Struct::FileData const &data = mainData.m_recursData[c];
		if (data.isBad()) continue;
		if (m_state->m_characterTypes.find(data.id())==m_state->m_characterTypes.end())
		{
			WPS_DEBUG_MSG(("WPS8Text::propertyChange: unexpected id %d\n", data.id()));
			f << "##" << data << ",";
			continue;
		}
		// find also id=2 with type=2 : does this means no bold ?
		if (m_state->m_characterTypes.find(data.id())->second != data.type())
		{
			WPS_DEBUG_MSG(("WPS8Text::propertyChange: unexpected type for %d\n", data.id()));
			f << "###" << data << ",";
			continue;
		}
		switch(data.id())
		{
		case 0x0:
			specialCode = (uint16_t) data.m_value;
			break;
		case 0x02:
			if (data.isTrue())
				textAttributeBits |= WPS_BOLD_BIT;
			else
				f << "#bold=false,";
			break;
		case 0x03:
			if (data.isTrue())
				textAttributeBits |= WPS_ITALICS_BIT;
			else
				f << "#it=false,";
			break;
		case 0x04:
			if (data.isTrue())
				textAttributeBits |= WPS_OUTLINE_BIT;
			else
				f << "#outline=false,";
			break;
		case 0x05:
			if (data.isTrue())
				textAttributeBits |= WPS_SHADOW_BIT;
			else
				f << "#shadow=false,";
			break;
		case 0x0c:
			m_listener->setFontSize(uint16_t(data.m_value/12700));
			break;
		case 0x0F:
			if ((data.m_value&0xFF) == 1) textAttributeBits |= WPS_SUPERSCRIPT_BIT;
			else if ((data.m_value&0xFF) == 2) textAttributeBits |= WPS_SUBSCRIPT_BIT;
			else f << "###ff=" << std::hex << data.m_value << std::dec << ",";
			break;
		case 0x10:
			if (data.isTrue())
				textAttributeBits |= WPS_STRIKEOUT_BIT;
			else
				f << "#strikeout=false,";
			break;
		case 0x12:
			m_listener->setTextLanguage(int(data.m_value));
			break;
		case 0x13:
			if (data.isTrue())
				textAttributeBits |= WPS_SMALL_CAPS_BIT;
			else
				f << "#smallbit=false,";
			break;
		case 0x14:
			if (data.isTrue())
				textAttributeBits |= WPS_ALL_CAPS_BIT;
			else
				f << "#allcaps=false,";
			break;
		case 0x16:
			if (data.isTrue())
				textAttributeBits |= WPS_EMBOSS_BIT;
			else
				f << "#emboss=false,";
			break;
		case 0x17:
			if (data.isTrue())
				textAttributeBits |= WPS_ENGRAVE_BIT;
			else
				f << "#engrave=false,";
			break;
			// fixme: there are various styles of underline
		case 0x1e:
			textAttributeBits |= WPS_UNDERLINE_BIT;
			break;
		case 0x22:
			fieldType = int(data.m_value);
			break;
		case 0x24:
		{
			if ((!data.isRead() && !data.readArrayBlock() && data.m_recursData.size() == 0) ||
			        !data.isArray())
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChange: can not read font array\n"));
				f << "###fontPb";
				break;
			}

			size_t nChild = data.m_recursData.size();
			if (!nChild || data.m_recursData[0].isBad() || data.m_recursData[0].type() != 0x18)
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChange: can not read font id\n"));
				f << "###fontPb";
				break;
			}
			uint8_t fId = (uint8_t)data.m_recursData[0].m_value;
			if (fId < m_state->m_fontNames.size())
				m_listener->setTextFont(m_state->m_fontNames[fId].c_str());
			else
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChange: can not read find font %d\n", int(fId)));
			}
			std::vector<int> formats;
			for (size_t i = 0; i < nChild; i++)
			{
				WPS8Struct::FileData const &subD = data.m_recursData[i];
				if (subD.isBad()) continue;
				int formId = subD.id() >> 3;
				int sId = subD.id() & 0x7;
				if (sId == 0)
				{
					formats.resize(size_t(formId)+1,-1);
					formats[size_t(formId)] = int(subD.m_value);
				}
				else
					f << "###formats"<<formId<<"." << sId << "=" << data.m_recursData[i] << ",";
			}
			f << "formats=[" << std::hex;
			for (size_t i = 0; i < formats.size(); i++)
			{
				if (formats[i] != -1)
					f << "f" << i << "=" << formats[i] << ",";
			}
			f << "],";
			break;
		}
		case 0x2e:
		{
			uint32_t col = (uint32_t) (data.m_value&0xFFFFFF);
			m_listener->setTextColor((col>>16)|(col&0xFF00)|((col&0xFF)<<16));;
			if (data.m_value &0xFF000000) f << "#f2e=" << std::hex << data.m_value << std::dec;
			break;
		}
		default:
			f << "#" << data << ",";
			break;
		}
	}

	m_listener->setFontAttributes(textAttributeBits);
}

void WPS8Text::propertyChangePara(WPS8Struct::FileData const &mainData)
{
	static const libwps::Justification _align[]=
	{
		libwps::JustificationLeft, libwps::JustificationRight,
		libwps::JustificationCenter, libwps::JustificationFull
	};
	libwps::Justification align = libwps::JustificationLeft;

	std::vector<WPSTabStop> tabList;
	m_listener->setTabs(tabList);

	libwps::DebugStream f;
	if (mainData.m_value) f << "unk=" << mainData.m_value << ",";

	float leftIndent=0.0, textIndent=0.0;
	int listLevel = 0;
	WPSList::Level level;
	for (size_t c = 0; c < mainData.m_recursData.size(); c++)
	{
		WPS8Struct::FileData const &data = mainData.m_recursData[c];
		if (data.isBad()) continue;
		if (m_state->m_paragraphTypes.find(data.id())==m_state->m_paragraphTypes.end())
		{
			WPS_DEBUG_MSG(("WPS8Text::propertyChangePara: unexpected id %d\n", data.id()));
			f << "###" << data << ",";
			continue;
		}
		if (m_state->m_paragraphTypes.find(data.id())->second != data.type())
		{
			WPS_DEBUG_MSG(("WPS8Text::propertyChangePara: unexpected type for %d\n", data.id()));
			f << "###" << data << ",";
			continue;
		}

		switch(data.id())
		{
			/* paragraph has a bullet specified in num format*/
		case 0x3:
			level.m_type = libwps::BULLET;
			level.m_bullet = "*";
			listLevel = 1;
			break;
		case 0x04:
			if (data.m_value >= 0 && data.m_value < 4) align = _align[data.m_value];
			else f << "###align=" << data.m_value << ",";
			break;
		case 0x0C:
			textIndent=float(data.m_value)/914400.0f;
			break;
		case 0x0D:
			leftIndent=float(data.m_value)/914400.0f;
			break;
			/* 0x0E: right/635, 0x12: before/635, 0x13: after/635 */
		case 0x14:
		{
			/* numbering style */
			int oldListLevel = listLevel;
			listLevel = 1;
			switch(data.m_value & 0xFFFF)
			{
			case 0: // checkme
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara: Find list flag=0\n"));
				level.m_type = libwps::NONE;
				listLevel = 0;
				break;
			case 2:
				level.m_type = libwps::ARABIC;
				break;
			case 3:
				level.m_type = libwps::LOWERCASE;
				break;
			case 4:
				level.m_type = libwps::UPPERCASE;
				break;
			case 5:
				level.m_type = libwps::LOWERCASE_ROMAN;
				break;
			case 6:
				level.m_type = libwps::UPPERCASE_ROMAN;
				break;
			default:
				listLevel=-1;
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara style %04lx\n",(data.m_value & 0xFFFF)));
				break;
			}
			if (listLevel == -1)
				listLevel = oldListLevel;
			else
				level.m_suffix=((data.m_value>>16) == 2) ? "." : ")";
		}
		break;
		case 0x32:
		{
			if (!data.isRead() && !data.readArrayBlock() && data.m_recursData.size() == 0)
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara can not find tabs array\n"));
				f << "###tabs,";
				break;
			}
			size_t nChild = data.m_recursData.size();
			if (nChild < 1 ||
			        data.m_recursData[0].isBad() || data.m_recursData[0].id() != 0x27)
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara can not find first child\n"));
				f << "###tabs,";
				break;
			}
			if (nChild == 1) break;

			int numTabs = int(data.m_recursData[0].m_value);
			if (numTabs == 0 || nChild < 2 ||
			        data.m_recursData[1].isBad() || data.m_recursData[1].id() != 0x28)
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara can not find second child\n"));
				f << "###tabs,";
				break;
			}

			WPS8Struct::FileData const &mData = data.m_recursData[1];
			size_t lastParsed = 0;
			if (mData.id() == 0x28 && mData.isArray() &&
			        (mData.isRead() || mData.readArrayBlock() || mData.m_recursData.size() != 0))
			{
				lastParsed = 1;
				size_t nTabsChilds = mData.m_recursData.size();
				int actTab = 0;
				tabList.resize(size_t(numTabs));

				for (size_t i = 0; i < nTabsChilds; i++)
				{
					if (mData.m_recursData[i].isBad()) continue;
					int value = mData.m_recursData[i].id();
					int wTab = value/8;
					int what = value%8;

					// the first tab can be skipped
					// so this may happens only one time
					if (wTab > actTab && actTab < numTabs)
					{
						tabList[size_t(actTab)].m_alignment = WPSTabStop::LEFT;
						tabList[size_t(actTab)].m_position =  0.;

						actTab++;
					}

					if (mData.m_recursData[i].isNumber() && wTab==actTab && what == 0
					        && actTab < numTabs)
					{
						tabList[size_t(actTab)].m_alignment = WPSTabStop::LEFT;
						tabList[size_t(actTab)].m_position =  float(mData.m_recursData[i].m_value)/914400.f;

						actTab++;
						continue;
					}
					if (mData.m_recursData[i].isNumber() && wTab == actTab-1 && what == 1)
					{
						int actVal = int(mData.m_recursData[i].m_value);
						switch((actVal & 0x3))
						{
						case 0:
							tabList[size_t(actTab-1)].m_alignment = WPSTabStop::LEFT;
							break;
						case 1:
							tabList[size_t(actTab-1)].m_alignment = WPSTabStop::RIGHT;
							break;
						case 2:
							tabList[size_t(actTab-1)].m_alignment = WPSTabStop::CENTER;
							break;
						case 3:
							tabList[size_t(actTab-1)].m_alignment = WPSTabStop::DECIMAL;
							break;
						default:
							break;
						}
						if (actVal&0xC)
							f << "###tabFl" << actTab<<":low=" << (actVal&0xC) << ",";
						actVal = (actVal>>8);
						/* not frequent:
						   but fl1:high=db[C], fl2:high=b7[R] appear relatively often*/
						if (actVal)
							f << "###tabFl" << actTab<<":high=" << actVal << ",";
						continue;
					}
					if (mData.m_recursData[i].isNumber() && wTab == actTab-1 && what == 2)
					{
						// tabList[actTab-1].m_leaderCharacter = mData.m_recursData[i].m_value;
						continue;
					}
					f << "###tabData:fl" << actTab << "=" << mData.m_recursData[i] << ",";
				}
				if (actTab != numTabs)
				{
					f << "NTabs[###founds]="<<actTab << ",";
					tabList.resize(size_t(actTab));
				}
			}
			for (size_t ch =lastParsed+1; ch < nChild; ch++)
			{
				if (data.m_recursData[ch].isBad()) continue;
				f << "extra[tabs]=[" << data.m_recursData[ch] << "],";
			}
			m_listener->setTabs(tabList);
		}
		break;
		default:
			f << "#" << data << ",";
			break;
		}
	}

	if (listLevel != -1)
	{
		if (listLevel)
		{
			if (!m_listener->getCurrentList())
				m_listener->setCurrentList(shared_ptr<WPSList>(new WPSList));
			m_listener->getCurrentList()->set(1, level);
		}
		m_listener->setCurrentListLevel(listLevel);
	}
	m_listener->setParagraphJustification(align);
	m_listener->setParagraphTextIndent(textIndent);
	m_listener->setParagraphMargin(leftIndent, WPS_LEFT);
}

////////////////////////////////////////////////////////////
// basic strings functions:
////////////////////////////////////////////////////////////
/* Read an UTF16 character in LE byte ordering, convert it. Courtesy of glib2 */
long WPS8Text::readUTF16LE(WPXInputStreamPtr input, long endPos, uint16_t firstC)
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
	if (firstC>=28 ) return firstC;
	WPS_DEBUG_MSG(("WPS8Text::readUTF16LE: error: character = %i (0x%X)\n", firstC, firstC));
	return 0xfffd;
}

bool WPS8Text::readString(WPXInputStreamPtr input, long page_size,
                          WPXString &res)
{
	res = "";
	long page_offset = input->tell();
	long endString = page_offset + page_size;

	while (input->tell() < endString-1 && !input->atEOS())
	{
		uint16_t val = libwps::readU16(input);
		if (!val) break;

		long unicode = readUTF16LE(input, endString, val);
		if (unicode != 0xfffd) WPSContentListener::appendUnicode(val, res);
	}
	return true;
}

////////////////////////////////////////////////////////////
// the fontname:
////////////////////////////////////////////////////////////
bool WPS8Text::readFontNames(WPSEntry const &entry)
{
	WPXInputStreamPtr input = getInput();
	if (!entry.hasType(entry.name()))
	{
		WPS_DEBUG_MSG(("WPS8Text::readFontNames: name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}

	if (entry.length() < 20)
	{
		WPS_DEBUG_MSG(("WPS8Text::readFontNames: length=0x%ld\n", entry.length()));
		return false;
	}

	long debPos = entry.begin();
	input->seek(debPos, WPX_SEEK_SET);

	long len = libwps::readU32(input); // len + 0x14 = size
	size_t n_fonts = (size_t) libwps::readU32(input);

	if (long(4*n_fonts) > len)
	{
		WPS_DEBUG_MSG(("WPS8Text::readFontNames: number=%d\n", int(n_fonts)));
		return false;
	}
	libwps::DebugStream f;

	entry.setParsed();
	f << "N=" << n_fonts;
	if (len+20 != entry.length()) f << ", ###L=" << std::hex << len+0x14;

	f << ", unkn=(" << std::hex;
	for (int i = 0; i < 3; i++) f << libwps::readU32(input) << ", ";
	f << "), dec=[";
	for (int i = 0; i < int(n_fonts); i++) f << ", " << libwps::read32(input);
	f << "]" << std::dec;

	ascii().addPos(debPos);
	ascii().addNote(f.str().c_str());

	long pageEnd = entry.end();

	/* read each font in the table */
	while (input->tell() > 0 && m_state->m_fontNames.size() < n_fonts)
	{
		debPos = input->tell();
		if (debPos+6 > long(pageEnd)) break;

		int string_size = (int) libwps::readU16(input);
		if (debPos+2*string_size+6 > long(pageEnd)) break;

		std::string s;
		for (; string_size>0; string_size--)
			s.append(1, (char) libwps::readU16(input));

		f.str("");
		f << "FONT("<<m_state->m_fontNames.size()<<"): " << s;
		f << ", unkn=(";
		for (int i = 0; i < 4; i++) f << (int) libwps::read8(input) << ", ";
		f << ")";
		ascii().addPos(debPos);
		ascii().addNote(f.str().c_str());

		m_state->m_fontNames.push_back(s);
	}

	if (m_state->m_fontNames.size() != n_fonts)
	{
		WPS_DEBUG_MSG(("WPS8Text::readFontNames: expected %i fonts but only found %i\n",
		               int(n_fonts), int(m_state->m_fontNames.size())));
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


	WPXInputStreamPtr input = getInput();
	long debPos = entry.begin();
	long length = entry.length();
	long endPos = entry.end();
	input->seek(debPos, WPX_SEEK_SET);

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
	long val;
	for (int i = 0; i < nNotes; i++)
	{
		pos = input->tell();
		f.str("");
		f << entry.name() << i << ":";
		val = (long) libwps::read16(input);
		if (val != 1) // always 1 ?
			f << val << ",";
		f << "id=" << libwps::read16(input) << ",";
		val = (long) libwps::read32(input);
		if (val != -1) // always -1 ?
			f << val << ",";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
	}

	// no sure what we can do of the end of the data ?
	pos = input->tell();
	if (pos+4*nNotes+10 != endPos) // ok with my files, it is general ?
	{
		WPS_DEBUG_MSG(("WPS8Text::readNotes: unexpected end size\n"));
		ascii().addPos(pos);
		f.str("");
		f << entry.name() << "[###End]";
		return true;
	}

	ascii().addPos(pos);
	f.str("");
	f << entry.name() << "[A]";
	ascii().addNote(f.str().c_str());
	ascii().addPos(pos+10);
	f.str("");
	f << entry.name() << "[B]";
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
		if (nIt->second != 0)
		{
			nIt++;
			continue;
		}
		nIt++;

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
// plc: font, paragraph
////////////////////////////////////////////////////////////
bool WPS8Text::readFont(long endPos, int &id, std::string &mess)
{
	WPXInputStreamPtr input=getInput();
	long actPos = input->tell();
	long size = endPos - actPos;

	/* other than blank, the shortest should be 2 bytes */
	if (size && (size%2) == 1)
	{
		WPS_DEBUG_MSG(("WPS8Text::readFont: error: charProperty size is odd\n"));
		return false;
	}

	WPS8Struct::FileData mainData;
	std::string error;

	bool readOk= size ? readBlockData(input, endPos, mainData, error) : true;
	libwps::DebugStream f;
	if (size && !readOk)
	{
		WPS_DEBUG_MSG(("WPS8Text::readFont: can not read data\n"));
		f << "###";
	}
	else
	{
		f << mainData;
		if (error.length())
			f << ",#err=(" << error << ")";
	}

	id = int(m_state->m_fontList.size());
	m_state->m_fontList.push_back(mainData);
	mess = f.str();
	return readOk;
}

bool WPS8Text::readParagraph(long endPos, int &id, std::string &mess)
{
	WPXInputStreamPtr input=getInput();
	long actPos = input->tell();
	long size = endPos - actPos;

	/* other than blank, the shortest should be 2 bytes */
	if (size && (size%2) == 1)
	{
		WPS_DEBUG_MSG(("WPS8Text::readParagraph: paraProperty size is odd\n"));
		return false;
	}

	WPS8Struct::FileData mainData;
	std::string error;

	bool readOk= size ? readBlockData(input, endPos, mainData, error) : true;
	libwps::DebugStream f;
	if (size && !readOk)
	{
		WPS_DEBUG_MSG(("WPS8Text::readParagraph: can not read data\n"));
		f << "###";
	}
	else
	{
		f << mainData;
		if (error.length())
			f << ",#err=(" << error << ")";
	}

	id = int(m_state->m_paragraphList.size());
	m_state->m_paragraphList.push_back(mainData);
	mess = f.str();
	return readOk;
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
	WPXInputStreamPtr input=getInput();
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
			input->seek(pos, WPX_SEEK_SET);
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
		input->seek(pos, WPX_SEEK_SET);
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
	while(input->tell() < endPage-sz1 && !input->atEOS())
		f << libwps::read16(input) << ",";
	f << std::dec;
	ascii().addNote(f.str().c_str());
	// File1= 0b000000 (N?) - 4c891700:00000000:001fcd00 when numBmkt=11
	// File2= 01000000 (N?) - 988f1200:00000000:0045dd01 when numBmkt=1
	// probably int, dim(*914400), int, short, short
	// ------ end of problematic part -------------

	input->seek(endPage-sz1, WPX_SEEK_SET);

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
			input->seek(pos, WPX_SEEK_SET);
			WPXString val;
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
	input->seek(endPage, WPX_SEEK_SET);

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

	WPXInputStreamPtr input = getInput();
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
			input->seek(pos, WPX_SEEK_SET);
			return false;
		}

		WPS8Struct::FileData data;
		std::string error;
		if (!readBlockData(input, pos+sz, data, error) && data.m_recursData.size() == 0)
		{
			input->seek(pos, WPX_SEEK_SET);
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
			input->seek(pos, WPX_SEEK_SET);
			return false;
		}

		std::string error;
		if (!readBlockData(input, pos+sz, data, error)  && data.m_recursData.size() == 0)
		{
			input->seek(pos, WPX_SEEK_SET);
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
			input->seek(pos, WPX_SEEK_SET);
			return false;
		}

		for (int i = 0; i < N; i++)
		{
			long val = (long) libwps::read32(input);
			if (val < 4*N)
			{
				input->seek(pos, WPX_SEEK_SET);
				return false;
			}
			val += pos+20;
			if (val > endPage)
			{
				input->seek(pos, WPX_SEEK_SET);
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
		input->seek(pos, WPX_SEEK_SET);
		int sz = (int) libwps::read16(input);
		if (sz < 0 || pos+2*sz+2 > endPage) return false;
		WPXString str;
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

////////////////////////////////////////////////////////////
// StyleSheet: STSH Zone (Checkme)
////////////////////////////////////////////////////////////
bool WPS8Text::readSTSH(WPSEntry const &entry)
{
	if (!entry.hasType(entry.name()))
	{
		WPS_DEBUG_MSG(("WPS8Text::readSTSH: warning: STSH name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}
	WPXInputStreamPtr input = getInput();
	long page_offset = entry.begin();
	long length = entry.length();
	long endPage = entry.end();

	if (length < 20)
	{
		WPS_DEBUG_MSG(("WPS8Text::readSTSH: warning: STSH length=0x%lx\n", length));
		return false;
	}

	entry.setParsed();
	input->seek(page_offset, WPX_SEEK_SET);

	libwps::DebugStream f;

	if (libwps::read32(input) != length-20) return false;
	int N = libwps::read32(input);

	if (N < 0) return false;
	f << "N=" << N; // 1 or 2

	f << std::hex << ", unk1=" << libwps::read32(input) << std::dec;
	int type = libwps::read32(input);
	f << ", type=" << type; // 4 : string ? 1 : unknown
	f << std::hex << ", unk2=" << libwps::read32(input); // "HST"
	f << ", pos=(";

	long debZone = input->tell();
	std::vector<long> ptrs;
	if (debZone + 4*N > endPage) return false;

	bool ok = true;
	for (int i = 0; i < N; i++)
	{
		long val = libwps::read32(input) + debZone;
		if (val >= endPage)
		{
			ok = false;
			break;
		}
		f << val << ",";
		ptrs.push_back(val);
	}
	f << ")";

	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());

	if (!ok) return false;

	for (size_t i = 0; i < size_t(N); i++)
	{
		long pos = ptrs[i];
		long endZPos = (i+1 == size_t(N)) ? endPage : ptrs[i+1];
		length = endZPos-pos;
		if (length < 2)
		{
			ok = false;
			continue;
		}

		f.str("");
		f << std::dec << "STSH(";
		if (type == 4) f << i;
		else
		{
			if (i%2) f << "P" << i/2;
			else f << "C" << i/2;
		}
		f << "):";

		input->seek(pos, WPX_SEEK_SET);
		int size = (int) libwps::readU16(input);
		bool correct = true;
		if (2*size + 2 + type != length) correct = false;
		else
		{
			switch(type)
			{
			case 4:
			{
				WPXString str;
				if (size) readString(input, 2*size, str);
				f << "'" << str.cstr() << "'";
				input->seek(pos+2+2*size, WPX_SEEK_SET);
				f << ", unkn=" << libwps::read32(input);
				break;
			}
			case 0:
			{
				WPS8Struct::FileData mainData;
				std::string error;
				int dataSz = (int) libwps::readU16(input);
				if (dataSz+2 != 2*size)
				{
					correct = false;
					break;
				}

				int id;
				std::string mess;
				if (i%2 == 0)
				{
					readFont(pos+2*size, id, mess);
					f << "Font" << id << "=" << mess;
				}
				else
				{
					readParagraph(pos+2*size, id, mess);
					f << "Paragraph" << id << "=" << mess;
				}
				break;
			}
			default:
				correct = false;
			}
		}
		if (!correct)
		{
			f << "###";
			ok = false;
		}

		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());

	}

	return ok;
}

////////////////////////////////////////////////////////////
// code to find the fdpc and fdpp entries; normal then by hand
////////////////////////////////////////////////////////////
bool WPS8Text::findFDPStructures(int which, std::vector<WPSEntry> &zones)
{
	WPXInputStreamPtr input = getInput();
	zones.resize(0);

	char const *indexName = which ? "BTEC" : "BTEP";
	char const *sIndexName = which ? "FDPC" : "FDPP";

	WPS8Parser::NameMultiMap::iterator pos =
	    mainParser().getNameEntryMap().lower_bound(indexName);

	std::vector<WPSEntry const *> listIndexed;
	while (pos != mainParser().getNameEntryMap().end())
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName(indexName)) break;
		if (!entry.hasType("PLC ")) continue;
		listIndexed.push_back(&entry);
	}

	size_t nFind = listIndexed.size();
	if (nFind==0) return false;

	// can nFind be > 1 ?
	bool ok = true;
	for (size_t i = 0; i+1 < nFind; i++)
	{
		ok = true;
		for (size_t j = 0; j+i+1 < nFind; j++)
		{
			if (listIndexed[j]->id() <= listIndexed[j+1]->id())
				continue;
			WPSEntry const *tmp = listIndexed[j];
			listIndexed[j] = listIndexed[j+1];
			listIndexed[j+1] = tmp;
			ok = false;
		}
		if (ok) break;
	}

	for (size_t i = 0; i+1 < nFind; i++)
		if (listIndexed[i]->id() == listIndexed[i+1]->id()) return false;

	// create a map offset -> entry
	std::map<long, WPSEntry const *> offsetMap;
	std::map<long, WPSEntry const *>::iterator offsIt;
	pos = mainParser().getNameEntryMap().lower_bound(sIndexName);
	while (pos != mainParser().getNameEntryMap().end())
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName(sIndexName)) break;
		offsetMap.insert(std::map<long, WPSEntry const *>::value_type
		                 (entry.begin(), &entry));
	}

	for (size_t i = 0; i < nFind; i++)
	{
		WPSEntry const &entry = *(listIndexed[i]);

		std::vector<long> textPtrs;
		std::vector<long> listValues;

		if (!readPLC(entry, textPtrs, listValues)) return false;

		size_t numV = listValues.size();
		if (textPtrs.size() != numV+1) return false;

		for (size_t j = 0; j < numV; j++)
		{
			long position = listValues[j];
			if (position <= 0) return false;

			offsIt = offsetMap.find(position);
			if (offsIt == offsetMap.end() || !offsIt->second->hasName(sIndexName))
				return false;

			zones.push_back(*offsIt->second);
		}
	}

	return true;
}

bool WPS8Text::findFDPStructuresByHand(int which, std::vector<WPSEntry> &zones)
{
	char const *indexName = which ? "FDPC" : "FDPP";
	WPS_DEBUG_MSG(("WPS8Text::findFDPStructuresByHand: error: need to create %s list by hand \n", indexName));
	zones.resize(0);

	WPS8Parser::NameMultiMap::iterator pos =
	    mainParser().getNameEntryMap().lower_bound(indexName);
	while (pos != mainParser().getNameEntryMap().end())
	{
		WPSEntry const &entry = pos->second;
		pos++;
		if (!entry.hasName(indexName)) break;
		if (!entry.hasType(indexName)) continue;

		zones.push_back(entry);
	}
	return zones.size() != 0;
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
	WPXInputStreamPtr input = getInput();
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

	input->seek(page_offset, WPX_SEEK_SET);
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

		fods[i].m_id = (int) m_state->m_plcList.size();
		if (ok) fods[i].m_defPos = pos;
		m_state->m_plcList.push_back(plc);

		if (printPLC)
		{
			f2.str("");
			f2 << plc.m_name << i << "(PLC"<<fods[i].m_id<<"):" << plc;
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
		input->seek(pos, WPX_SEEK_SET);
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
