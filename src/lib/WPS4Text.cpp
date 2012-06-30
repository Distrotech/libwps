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

#ifdef DEBUG_WITH_FILES
// set to 1 to debug the font property
#  define DEBUG_FP 1
// set to 1 to debug the paragraph property
#  define DEBUG_PP 1
// set to 1 to print the plc position
#  define DEBUG_PLC_POS 1
#endif

#include <iomanip>
#include <iostream>

#include <map>
#include <vector>

#include <libwpd/WPXString.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSContentListener.h"
#include "WPSPosition.h"
#include "WPSParagraph.h"

#include "WPS4.h"

#include "WPS4Text.h"

/** Internal and low level: the structures of a WPS4Text used to parse PLC*/
namespace WPS4PLCInternal
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

//////////////////////////////////////////////////////////////////////////////
// general enum
//////////////////////////////////////////////////////////////////////////////
namespace WPS4TextInternal
{
/** a enum used to type a zone */
enum ZoneType { Z_String=-1, Z_Header=0, Z_Footer=1, Z_Main=2, Z_Note, Z_Bookmark, Z_DLink, Z_Unknown};
/** Internal: class to store a font name: name with encoding type */
struct FontName
{
	//! constructor with file's version to define the default encoding */
	FontName(int version=4) : m_name(""), m_type(libwps_tools_win::Font::WIN3_WEUROPE)
	{
		if (version <= 2) m_type = libwps_tools_win::Font::DOS_850;
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, FontName const &ft);
	//! checks if the font name is set
	bool isSet() const
	{
		return !m_name.empty();
	}
	/** returns the default dos name corresponding to \a id th font */
	static std::string getDosName(int id);

	//! font name
	std::string m_name;
	//! font encoding type
	libwps_tools_win::Font::Type m_type;
};
//! operator<< for a font name
std::ostream &operator<<(std::ostream &o, FontName const &ft)
{
	if (!ft.m_name.empty()) o << "name='" << ft.m_name << "'";
	else o << "name='Unknown'";
	switch(ft.m_type)
	{
	case libwps_tools_win::Font::WIN3_CYRILLIC:
		o << ",type=Cyr";
		break;
	case libwps_tools_win::Font::WIN3_CEUROPE:
		o << ",type=Ce";
		break;
	case libwps_tools_win::Font::WIN3_GREEK:
		o << ",type=Greek";
		break;
	case libwps_tools_win::Font::WIN3_TURKISH:
		o << ",type=Tur";
		break;
	case libwps_tools_win::Font::WIN3_BALTIC:
		o << ",type=Baltic";
		break;
	case libwps_tools_win::Font::WIN3_ARABIC:
		o << ",type=Arabic";
		break;
	case libwps_tools_win::Font::WIN3_HEBREW:
		o << ",type=Hebrew";
		break;
	case libwps_tools_win::Font::WIN3_VIETNAMESE:
		o << ",type=Vietnamese";
		break;
	case libwps_tools_win::Font::WIN3_WEUROPE:
	case libwps_tools_win::Font::DOS_850:
	default:
		break;
	}
	return o;
}

std::string FontName::getDosName(int id)
{
	switch (id)
	{
	case 0:
		return "Courier";
	case 1:
		return "Courier PC";
	case 3:
		return "Univers_Scale";
	case 4:
		return "Universe";
	case 6:
		return "LinePrinterPC";
	case 7:
		return "LinePrinter";
	case 16:
		return "CGTimes_Scale";
	case 24:
		return "CGTimes";
	default:
		break;
	}

	WPS_DEBUG_MSG(("WPS4TextInternal::FontName::getDosName: encountered unknown font %i\n", id));
	return "Courier";
}
/** Internal: class to store font properties */
struct Font : public WPSFont
{
	//! constructor with file's version to define the default encoding */
	Font(int version=4) : WPSFont(), m_type(libwps_tools_win::Font::WIN3_WEUROPE),
		m_backColor(0xFFFFFF), m_special(false), m_dlinkId(-1)
	{
		if (version <= 2) m_type = libwps_tools_win::Font::DOS_850;
	}
	//! returns a default font (Courier12) with file's version to define the default encoding */
	static Font getDefault(int version)
	{
		Font res(version);
		if (version <= 2)
			res.m_name="Courier";
		else
			res.m_name="Times New Roman";
		res.m_size=12;
		return res;
	}

	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, Font const &ft);

	//! returns true if the font (or background) color is set
	bool hasColor() const
	{
		return (m_color != 0 || m_backColor != 0xFFFFFF);
	}
	/** resets to undef the font and the background color */
	void resetColor()
	{
		m_color = 0;
		m_backColor = 0xFFFFFF;
	}
	//! the font encoding type
	libwps_tools_win::Font::Type m_type;
	//! background  color index
	uint32_t m_backColor;
	//! a flag to know if we have a special field (a note), ...
	bool m_special;
	//! a id to retrieve a file name ( dos )
	int m_dlinkId;
};

//! operator<< for font properties
std::ostream &operator<<(std::ostream &o, Font const &ft)
{
	o << reinterpret_cast<WPSFont const &>(ft) << ",";

	if (ft.m_special)
	{
		if (ft.m_dlinkId >= 0)
			o << "spec[" << ft.m_dlinkId << "],";
		else
			o << "spec,";
	}

	if (ft.m_backColor != 0xFFFFFF)
		o << "bgCol=" << ft.m_backColor << ",";
	return o;
}
/** Internal: class to store paragraph properties */
struct Paragraph : public WPSParagraph
{
	//! constructor
	Paragraph() : WPSParagraph()  { }
};

/** Internal: class to store an note type */
struct Note : public WPSEntry
{
	//! constructor
	Note() : WPSEntry(), m_label(""), m_error("") {}
	bool isNumeric() const
	{
		return m_label.len()==0;
	}
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, Note const &note)
	{
		if (note.m_label.len())
			o << "lab=" << note.m_label.cstr() << ",";
		else
			o << "numeric,";
		if (!note.m_error.empty()) o << note.m_error << ",";
		return o;
	}
	//! the label if not numeric
	WPXString m_label;
	//! a string used to store the parsing errors
	std::string m_error;
};


/** Internal: class to store an object definition */
struct Object
{
	//! constructor
	Object() : m_id(-1), m_size(), m_pos(), m_unknown(0), m_extra("") {}
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, Object const &obj);

	//! the object identificator
	int m_id;
	//! the object size in the document
	Vec2f m_size;
	//! an entry which indicates where the object is defined in the file
	WPSEntry m_pos;
	//! unknown data
	long m_unknown;
	//! a string used to store the parsing errors
	std::string m_extra;
};
//! operator<< for an object
std::ostream &operator<<(std::ostream &o, Object const &obj)
{
	if (obj.m_id > -1) o << "ole" << obj.m_id;
	o <<": size(" << obj.m_size << ")";
	if (obj.m_pos.valid()) o << std::hex << ", def=(0x" << obj.m_pos.begin() << "->" << obj.m_pos.end() << ")";
	if (obj.m_unknown) o << std::hex << ", unkn=" << obj.m_unknown << std::dec;
	if (!obj.m_extra.empty()) o << ", err=" << obj.m_extra;
	return o;
}

/** Internal: class to store an object definition */
struct DosLink
{
	//! constructor
	DosLink() : m_type(-1), m_width(-1), m_size(), m_name(""), m_pos(), m_extra("") {}
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, DosLink const &dlink);

	//! the type
	int m_type;
	//! the width
	float m_width;
	//! the object size in the document
	Vec2f m_size;
	//! the file name
	std::string m_name;
	//! an entry which indicates where the object is defined in the file
	WPSEntry m_pos;
	//! a string used to store the parsing errors
	std::string m_extra;
};
//! operator<< for an object
std::ostream &operator<<(std::ostream &o, DosLink const &dlink)
{
	switch(dlink.m_type)
	{
	case -1:
		break;
	case 1:
		o << "chart,";
		break;
	case 0x81:
		o << "pict,";
		break;
	case 0x40:
		o << "spreadsheet,";
		break;
	default:
		o << "#type=" << dlink.m_type << ",";
		break;
	}
	if (dlink.m_width >= 0) o << "width?=" << dlink.m_width << ",";
	if (dlink.m_size.x() >= 0 && (dlink.m_size.y()<0 || dlink.m_size.y()>0))
		o <<"size=" << dlink.m_size << ",";
	if (dlink.m_name.length()) o << "name='" << dlink.m_name << "',";
	if (!dlink.m_extra.empty()) o << ", err=" << dlink.m_extra;
	return o;
}

/** Internal: class to store a date/time format */
struct DateTime
{
	//! constructor
	DateTime() : m_type(-1), m_extra("") {}
	//! returns a format to used with strftime
	std::string format() const;
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, DateTime const &dtime);

	//! the type
	int m_type;
	//! a string used to store the parsing errors
	std::string m_extra;
};

std::string DateTime::format() const
{
	switch(m_type)
	{
	case 0:
		return "%m/%d/%Y";
	case 1:
		return "%m/%Y";
	case 2:
		return "%d %B %Y";
	case 3:
		return "%A %d %B %Y";
	case 4:
		return "%B %Y";
	case 5:
		return "%m/%d/%Y %I:%M";
	case 6:
		return "%m/%d/%Y %I:%M:%S";
	case 7:
		return "%I:%M:%S";
	case 8:
		return "%I:%M";
	case 9:
		return "%H:%M:%S";
	case 10:
		return "%H:%M";
	default:
		break;
	}
	return "";
}

//! operator<< for an object
std::ostream &operator<<(std::ostream &o, DateTime const &dtime)
{
	switch(dtime.m_type)
	{
	case -1:
		break;
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		o << "date[F"<<dtime.m_type<<"],";
		break;
	case 5:
	case 6:
		o << "date&time[F"<<dtime.m_type-5<<"],";
		break;
	case 7:
	case 8:
	case 9:
	case 10:
		o << "time[F"<<dtime.m_type-7<<"],";
		break;
	default:
		o << "#type=" << dtime.m_type << ",";
		break;
	}
	if (!dtime.m_extra.empty()) o << ", err=" << dtime.m_extra;
	return o;
}

/** different types
 *
 * - BTE: font/paragraph properties
 * - OBJECT: object properties
 * - FTNp, FTNd: footnote position in text and footnote content
 * - BKMK: comment field
 * - DTTM: field type: date/time/..
 */
enum PLCType { BTE=0, OBJECT, FTNp, FTNd, BKMK, DTTM, Unknown};

/** Internal: class to store the PLC: Pointer List Content ? */
struct DataPLC
{
	//! constructor
	DataPLC(): m_name(""), m_type(Unknown), m_value(-1), m_extra() {}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, DataPLC const &plc);
	//! the entry field name
	std::string m_name;
	//! the plc type
	PLCType m_type;
	//! a potential value
	long m_value;
	//! a string used to store the parsing errors
	std::string m_extra;
};
//! operator<< for a dataPLC
std::ostream &operator<<(std::ostream &o, DataPLC const &plc)
{
	o << "type=" << plc.m_name << ",";
	if (plc.m_value != -1) o << "val=" << std::hex << plc.m_value << std::dec << ", ";
	if (!plc.m_extra.empty()) o << "errors=(" << plc.m_extra << ")";
	return o;
}

//! structure which retrieves data information which correspond to a text position
struct DataFOD
{
	/** different type which can be associated to a text position
	 *
	 * ATTR_TEXT: all text attributes (font, size, ...)
	 * ATTR_PARAG: all paragraph attributes (margin, tabs, ...)
	 * ATTR_PLC: other attribute (note, fields ... )
	 */
	enum Type { ATTR_TEXT, ATTR_PARAG, ATTR_PLC, ATTR_UNKN };

	//! the constructor
	DataFOD() : m_type(ATTR_UNKN), m_pos(-1), m_defPos(0), m_id(-1) {}

	//! the type of the attribute
	Type m_type;
	//! the offset position of the text modified by this attribute
	long m_pos;
	//! the offset position of the definition of the attribute in the file
	long m_defPos;
	//! an identificator (which must be unique by category)
	int m_id;
};

/** Internal: the state of a WPS4Text */
struct State
{
	//! constructor
	State() : m_version(-1), m_fontNames(), m_fontList(),  m_paragraphList(),
		m_FDPCs(), m_FDPPs(), m_footnoteList(), m_footnoteMap(), m_bookmarkMap(), m_dosLinkList(),
		m_FODList(), m_text(), m_main(), m_header(), m_footer(), m_otherZones(),
		m_objectMap(), m_dateTimeMap(), m_plcList(), m_knownPLC()
	{}

	/** function which takes two sorted list of attribute (by text position).
		\return a list of attribute */
	std::vector<DataFOD> mergeSortedFODList
	(std::vector<DataFOD> const &lst1, std::vector<DataFOD> const &lst2) const;

	//! the version
	int m_version;
	//! the list of fonts names
	std::map<int,FontName> m_fontNames;
	//! the list of all font properties
	std::vector<Font> m_fontList;
	//! the list of all paragraph properties
	std::vector<Paragraph> m_paragraphList;

	//! the list of FDPC entries (ie list to find the font properties lists )
	std::vector<WPSEntry> m_FDPCs;
	//! the list of FDPP entries (ie list to find the paragraph properties lists )
	std::vector<WPSEntry> m_FDPPs;

	//! the footnote entries
	std::vector<Note> m_footnoteList;
	//! map: footnote in text -> footnote entry
	std::map<long,Note const *> m_footnoteMap;
	//! map: bookmark in text -> bookmark
	std::map<long, WPSEntry> m_bookmarkMap;
	//! the dos file links
	std::vector<DosLink> m_dosLinkList;
	//! the list of a FOD
	std::vector<DataFOD> m_FODList;

	WPSEntry m_text/** all the text zone entry*/, m_main /** the main text zone entry*/,
	         m_header /** the header text entry*/, m_footer /** the footer text entry*/;

	//! the entries which are not in main/header/footer text and in the footnotes
	std::vector<WPSEntry> m_otherZones;
	//! map: object in text -> object
	std::map<long, Object> m_objectMap;
	//! map: date field in text -> date time format
	std::map<long, DateTime> m_dateTimeMap;
	//! a list of all PLCs
	std::vector<DataPLC> m_plcList;
	//! the known plc
	WPS4PLCInternal::KnownPLC m_knownPLC;
};

std::vector<DataFOD> State::mergeSortedFODList(std::vector<DataFOD> const &lst1, std::vector<DataFOD> const &lst2) const
{
	std::vector<DataFOD> res;
	// we regroup these two lists in one list
	size_t num1 = lst1.size(), i1 = 0;
	size_t num2 = lst2.size(), i2 = 0;

	while (i1 < num1 || i2 < num2)
	{
		DataFOD val;
		if (i2 == num2) val = lst1[i1++];
		else if (i1 == num1 || lst2[i2].m_pos < lst1[i1].m_pos)
			val = lst2[i2++];
		else val = lst1[i1++];

		if (val.m_pos < m_text.begin() || val.m_pos > m_text.end())
			continue;

		res.push_back(val);
	}
	return res;
}
}

//////////////////////////////////////////////////////////////////////////////
//
//   MAIN CODE
//
//////////////////////////////////////////////////////////////////////////////

// constructor/destructor
WPS4Text::WPS4Text(WPS4Parser &parser, WPXInputStreamPtr &input) :
	m_input(input), m_mainParser(parser), m_listener(), m_state(), m_asciiFile(parser.m_asciiFile)
{
	m_state.reset(new WPS4TextInternal::State);
}

WPS4Text::~WPS4Text()
{
}

// version, number of page
int WPS4Text::version() const
{
	if (m_state->m_version <= 0)
		m_state->m_version = m_mainParser.version();
	return m_state->m_version;
}

int WPS4Text::numPages() const
{
	int numPage = 1;
	m_input->seek(m_state->m_text.begin(), WPX_SEEK_SET);
	while (!m_input->atEOS() && m_input->tell() != m_state->m_text.end())
	{
		if (libwps::readU8(m_input.get()) == 0x0C) numPage++;
	}
	return numPage;
}

// return main/header/footer/all entry
WPSEntry WPS4Text::getHeaderEntry() const
{
	if (m_state->m_header.valid()) return m_state->m_header;
	WPS4Parser::NameMultiMap const &nameMultiMap = m_mainParser.m_nameMultiMap;
	WPS4Parser::NameMultiMap::const_iterator pos;
	pos = nameMultiMap.find("SHdr");
	if (pos == nameMultiMap.end()) return WPSEntry();
	WPSEntry res = pos->second;
	res.setType("TEXT");
	res.setId(WPS4TextInternal::Z_String);
	return res;
}

WPSEntry WPS4Text::getFooterEntry() const
{
	if (m_state->m_footer.valid()) return m_state->m_footer;
	WPS4Parser::NameMultiMap const &nameMultiMap = m_mainParser.m_nameMultiMap;
	WPS4Parser::NameMultiMap::const_iterator pos;
	pos = nameMultiMap.find("SFtr");
	if (pos == nameMultiMap.end()) return WPSEntry();
	WPSEntry res = pos->second;
	res.setType("TEXT");
	res.setId(WPS4TextInternal::Z_String);
	return res;
}

WPSEntry WPS4Text::getMainTextEntry() const
{
	return m_state->m_main;
}

WPSEntry WPS4Text::getAllTextEntry() const
{
	return m_state->m_text;
}

////////////////////////////////////////////////////////////
// send the data
////////////////////////////////////////////////////////////
void WPS4Text::flushExtra()
{
	size_t numExtra = m_state->m_otherZones.size();
	if (numExtra == 0 || m_listener.get() == 0L) return;

	setProperty(WPS4TextInternal::Font::getDefault(version()));
	setProperty(WPS4TextInternal::Paragraph());
	m_listener->insertEOL();
#ifdef DEBUG
	WPXString message = "--------- extra text zone -------- ";
	m_listener->insertUnicodeString(message);
#endif
	for (size_t i = 0; i < numExtra; i++)
		readText(m_state->m_otherZones[i]);
}

bool WPS4Text::readText(WPSEntry const &zone)
{
	bool bookmark = zone.id() == WPS4TextInternal::Z_Bookmark;
	bool dlink = zone.id() == WPS4TextInternal::Z_DLink;
	bool simpleString = zone.id() == WPS4TextInternal::Z_String || bookmark || dlink;
	bool mainZone = zone.id() == WPS4TextInternal::Z_Main;

	if (m_listener.get() == 0L)
	{
		WPS_DEBUG_MSG(("WPS4Text::readText can not find the listener\n"));
		return false;
	}
	if (!zone.valid())
	{
		WPS_DEBUG_MSG(("WPS4Text::readText invalid zone, must not happen\n"));
		m_listener->insertCharacter(' ');
		return false;
	}
	int numCols = 1;
	if (mainZone)
	{
		numCols = m_mainParser.numColumns();
		if (numCols > 1)
		{
			if (m_listener->isSectionOpened())
			{
				WPS_DEBUG_MSG(("WPS4Text::readText the section is already open\n"));
			}
			else
			{
				int w = int(72.0*m_mainParser.pageWidth())/numCols;
				std::vector<int> width;
				width.resize(size_t(numCols), w);
				m_listener->openSection(width,WPX_POINT);
			}
		}
		else
			numCols = 1;
	}
	std::vector<WPS4TextInternal::DataFOD>::iterator FODs_iter = m_state->m_FODList.begin();

	// update the property to correspond to the text
	int prevFId = -1, prevPId = -1;
	if (simpleString) FODs_iter = m_state->m_FODList.end();
	else if (FODs_iter == m_state->m_FODList.end() && mainZone)
	{
		WPS_DEBUG_MSG(("WPS4Text::readText: CAN NOT FIND any FODs for main zone, REVERT to basic string!!!!!!!!!\n"));
		simpleString = true;
	}

	for ( ; FODs_iter!= m_state->m_FODList.end(); FODs_iter++)
	{
		WPS4TextInternal::DataFOD const &fod = *(FODs_iter);
		if (fod.m_pos >= zone.begin()) break;

		int id = (*FODs_iter).m_id;
		if (fod.m_type == WPS4TextInternal::DataFOD::ATTR_TEXT) prevFId = id;
		else if (fod.m_type == WPS4TextInternal::DataFOD::ATTR_PARAG) prevPId = id;
	}

	WPS4TextInternal::Font actFont;
	if (prevFId != -1)
		actFont = m_state->m_fontList[size_t(prevFId)];
	else
		actFont = WPS4TextInternal::Font::getDefault(version());
	setProperty(actFont);

	if (prevPId != -1)
		setProperty(m_state->m_paragraphList[size_t(prevPId)]);
	else
		setProperty(WPS4TextInternal::Paragraph());

	if (dlink)
	{
		m_listener->insertUnicodeString("include ");
		m_listener->insertUnicode(0x226a);
	}
	bool first = true;
	int actPage = 1;
	for ( ; simpleString || FODs_iter!= m_state->m_FODList.end(); FODs_iter++)
	{
		long actPos;
		long lastPos;

		WPS4TextInternal::DataFOD fod;
		WPS4TextInternal::DataFOD::Type type = WPS4TextInternal::DataFOD::ATTR_UNKN;
		int fId = -1;
		if (simpleString)
		{
			actPos = zone.begin();
			lastPos = zone.end();
		}
		else
		{
			fod = *(FODs_iter);
			actPos = first ? zone.begin() : fod.m_pos;
			if (long(actPos) >= zone.end()) break;
			first = false;

			if (++FODs_iter!= m_state->m_FODList.end())
			{
				lastPos = (*FODs_iter).m_pos;
				if (long(lastPos) >= zone.end()) lastPos = zone.end();
			}
			else
				lastPos = zone.end();
			FODs_iter--;
			fId = fod.m_id;
			type = fod.m_type;
			if (type == WPS4TextInternal::DataFOD::ATTR_TEXT)
			{
				if (fId >= 0)
					actFont = m_state->m_fontList[size_t(fId)];
				else
					actFont = WPS4TextInternal::Font::getDefault(version());
				setProperty(actFont);
			}
			else if (type == WPS4TextInternal::DataFOD::ATTR_PARAG)
			{
				if (fId >= 0)
					setProperty(m_state->m_paragraphList[size_t(fId)]);
				else setProperty(WPS4TextInternal::Paragraph());
			}
			else if (type == WPS4TextInternal::DataFOD::ATTR_PLC)
			{
				if (fId >= 0 && m_state->m_plcList[size_t(fId)].m_type == WPS4TextInternal::BKMK)
				{
					WPSEntry bkmk;
					if (m_state->m_bookmarkMap.find(actPos) == m_state->m_bookmarkMap.end())
					{
						WPS_DEBUG_MSG(("WPS4Text::readText: can not find the bookmark entry\n"));
					}
					else
						bkmk = m_state->m_bookmarkMap.find(actPos)->second;
					bkmk.setType("TEXT");
					bkmk.setId(WPS4TextInternal::Z_Bookmark);
					m_mainParser.createDocument(bkmk, libwps::DOC_COMMENT_ANNOTATION);
				}
			}
		}
		m_input->seek(actPos, WPX_SEEK_SET);
		std::string chaine("");
		long len = lastPos-actPos;
		for (long i = len; i>0; i--)
		{
			long pos = m_input->tell();
			uint8_t readVal = libwps::readU8(m_input);
			if (0x00 == readVal)
			{
				if (i != 1)
				{
					WPS_DEBUG_MSG(("WPS4Text::readText: find some unexpected 0 character\n"));
					// probably an error, but we can ignore id
					chaine += '#';
				}
				continue;
			}

			chaine += char(readVal);
			switch (readVal)
			{
			case 0x01: // chart ?
			case 0x08: // spreadsheet range
			case 0x0e: // picture
			{
				if (!actFont.m_special || actFont.m_dlinkId >= int(m_state->m_dosLinkList.size()))
				{
					WPS_DEBUG_MSG(("WPS4Text::readText: send DLINK can not find id\n"));
					break;
				}
				int id = actFont.m_dlinkId >= 0 ? actFont.m_dlinkId : 0;
				WPSEntry ent = m_state->m_dosLinkList[size_t(id)].m_pos;
				ent.setType("TEXT");
				ent.setId(WPS4TextInternal::Z_DLink);
				WPSPosition pos_(Vec2f(),Vec2f(float(3.0),float(0.2)));
				pos_.setRelativePosition(WPSPosition::Paragraph, WPSPosition::XCenter);
				pos_.m_wrapping = WPSPosition::WNone;
				WPXPropertyList extras;
				m_mainParser.createTextBox(ent, pos_, extras);
				m_listener->insertEOL();
				break;
			}
			case 0x02:
				m_listener->insertField(WPSContentListener::PageNumber);
				break;
			case 0x03:
				m_listener->insertField(WPSContentListener::Date);
				break;
			case 0x04:
				m_listener->insertField(WPSContentListener::Time);
				break;
			case 0x05:
				m_listener->insertField(WPSContentListener::Title);
				break;
			case 0x07:
			{
				if (m_state->m_objectMap.find(actPos) == m_state->m_objectMap.end())
				{
					WPS_DEBUG_MSG(("WPS4Text::readText: can not find object for position : %lX\n", actPos));
				}
				else
				{
					WPS4TextInternal::Object const &obj = m_state->m_objectMap[actPos];
					if (obj.m_id < 0) break;

					m_mainParser.sendObject(obj.m_size, obj.m_id);
				}
				break;
			}
			case 0x0f:
			{
				if (m_state->m_dateTimeMap.find(actPos) == m_state->m_dateTimeMap.end())
				{
					WPS_DEBUG_MSG(("WPS4Text::readText: can not find date/time for position : %lX\n", actPos));
				}
				else
				{
					WPS4TextInternal::DateTime const &form = m_state->m_dateTimeMap[actPos];
					std::string format = form.format();
					if (format.length())
						m_listener->insertDateTimeField(format.c_str());
					else
					{
						WPS_DEBUG_MSG(("WPS4Text::readText: unknown date/time format for position : %lX\n", actPos));
					}
				}
				break;
			}
			case 0x06:   // footnote
			{
				// ok if this is the first character of the footnote definition
				if (zone.id() == WPS4TextInternal::Z_Note) break;
				if (m_state->m_footnoteMap.find(pos) == m_state->m_footnoteMap.end() ||
				        m_state->m_footnoteMap[pos] == 0L)
				{
					WPS_DEBUG_MSG(("WPS4Text::readText:do not find the footnote zone\n"));
					break;
				}
				WPS4TextInternal::Note const &note = *m_state->m_footnoteMap[pos];
				m_mainParser.createNote(note, note.m_label);
				break;
			}
			case 0x09:
				m_listener->insertTab();
				break;
			case 0x0C:
				if (mainZone) m_mainParser.newPage(++actPage);
				break;
			case 0x0d:
				break; // 0d0a = end of line
			case 0x0a:
				m_listener->insertEOL();
				break;
			case 0x0b: // check me
				m_listener->insertEOL(true);
				break;
			case 0x11: // insecable hyphen
				m_listener->insertUnicode(0x2011);
				break;
			case 0x12: // insecable space
				m_listener->insertUnicode(0xA0);
				break;
			case 0x1F: // optional hyphen
				break;
			case '&':
				if (simpleString && pos+2 <= lastPos)
				{
					int nextVal = libwps::readU8(m_input);
					bool done = true;
					switch (nextVal)   // check me
					{
					case 'p':
					case 'P':
						m_listener->insertField(WPSContentListener::PageNumber);
						break;
					case 'd':
					case 'D':
						m_listener->insertField(WPSContentListener::Date);
						break;
					case 't':
					case 'T':
						m_listener->insertField(WPSContentListener::Time);
						break;
					case 'f':
					case 'F':
						m_listener->insertField(WPSContentListener::Title);
						break;
						// case '&': check me does '&&'->'&' ?
					default:
						done = false;
						break;
					}
					if (done)
					{
						i--;
						break;
					}
					m_input->seek(-1, WPX_SEEK_CUR);
				}
			default:
				if (version()<=2)
				{
					// special caracter
					if (readVal==0xca) // not breaking space
					{
						m_listener->insertCharacter(0xA0);
						break;
					}
				}
				if (readVal < 0x20)
				{
					WPS_DEBUG_MSG(("WPS4Text:readText odd character %x\n", readVal));
					chaine += '#';
					break;
				}
				m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::unicode(readVal, actFont.m_type));
				break;
			}
		}

		if (simpleString) break;

		libwps::DebugStream f;
		f << "Text";
		switch (type)
		{
		case WPS4TextInternal::DataFOD::ATTR_TEXT:
#if DEBUG_FP
			f << "[";
			if (fId >= 0) f << "C" << fId << ":" << m_state->m_fontList[size_t(fId)] << "]";
			else f << "_]";
#endif
			break;
		case WPS4TextInternal::DataFOD::ATTR_PARAG:
#if DEBUG_PP
			f << "[";
			if (fId >= 0) f << "P" << fId << ":" << m_state->m_paragraphList[size_t(fId)] << "]";
			else f << "_]";
#endif
			break;
		case WPS4TextInternal::DataFOD::ATTR_PLC:
#if DEBUG_PLC_POS
			f << "[PLC";
			if (fId>= 0) f << m_state->m_plcList[size_t(fId)] << "]";
			else f << "_]";
#endif
			break;
		case WPS4TextInternal::DataFOD::ATTR_UNKN:
		default:
#if DEBUG_PLC_POS
			f << "[WPS4TextInternal::DataFOD(###Unknown)]";
#endif
			break;
		}
		f << "='"<<chaine<<"'";
		ascii().addPos(actPos);
		ascii().addNote(f.str().c_str());
	}

	if (dlink)
		m_listener->insertUnicode(0x226b);

	return true;
}

////////////////////////////////////////////////////////////
// find all the text entries
////////////////////////////////////////////////////////////
bool WPS4Text::readEntries()
{
	WPS4Parser::NameMultiMap &nameMultiMap = m_mainParser.m_nameMultiMap;
	WPS4Parser::NameMultiMap::iterator pos;

	libwps::DebugStream f;
	long actPos = m_input->tell();
	f << "ZZHeader-Text:Limit(";

	int textLimits[4];
	// look like begin of text : end of header/end of footer/end text
	// but sometimes the zones overlaps !!!
	for (int i = 0; i < 4; i++) textLimits[i] = libwps::read32(m_input);

	bool first = true, ok = true;
	long lastPos = textLimits[0] < 0x100 ? 0x100 : textLimits[0];
	for (int i = 0; i < 3; i++)
	{
		long newPos = textLimits[i+1];
		WPSEntry zone;
		zone.setBegin(lastPos);
		zone.setEnd(newPos);
		zone.setType("TEXT");
		zone.setId(i);

		if (newPos >= lastPos)
			lastPos = newPos;
		if (!zone.valid() || zone.begin() < 0x100)
		{
			if (newPos != 0x100 && newPos != -1)
			{
				WPS_DEBUG_MSG(("WPS4Text::readEntries: find odd text limit\n"));
				ok = false;
			}
			f << "_, ";
			continue;
		}

		if (first)
		{
			m_state->m_text.setBegin(zone.begin());
			first = false;
		}

		m_state->m_text.setEnd(zone.end());
		nameMultiMap.insert(WPS4Parser::NameMultiMap::value_type(zone.type(), zone));

		switch (i)
		{
		case 0:
			m_state->m_header = zone;
			break;
		case 1:
			m_state->m_footer = zone;
			break;
		case 2:
			m_state->m_main = zone;
			break;
		default:
			break;
		}

		f << "Text"<<i << "=" << std::hex << zone.begin() << "x" << zone.end() << ",";
		ascii().addPos(zone.begin());
		std::string name = "ZZ";
		name+= zone.type();
		name+=char('0'+i);
		ascii().addNote(name.c_str());
		ascii().addPos(zone.end());
		ascii().addNote("_");
	}
	f << ")";
	if (!ok)
	{
		m_state->m_header = m_state->m_footer = WPSEntry();
		m_state->m_main = m_state->m_text;
	}
	if (!m_state->m_text.valid())
	{
		WPS_DEBUG_MSG(("WPS4Text::readEntries: textPosition is not valid"));
		return false;
	}

	/* stream offset to end of file */
	long eof = libwps::readU32(m_input);

	if (m_state->m_text.end() > eof)
	{
		WPS_DEBUG_MSG(("WPS4Text:readEntries: can not find text positions\n"));
		return false;
	}

	// check if fPositions.offset_eos
	long newPos = m_input->tell();
	if (m_input->seek(eof-1, WPX_SEEK_SET) != 0 || m_input->tell() != eof-1)
	{
		eof = m_input->tell();
		WPS_DEBUG_MSG(("WPS4Text:readEntries: incomplete file\n"));
		if (eof < m_state->m_text.end()) return false;
	}
	m_mainParser.setSizeFile(eof);

	f << ", endFile=" << eof;
	ascii().addPos(actPos);
	ascii().addNote(f.str().c_str());

	m_input->seek(newPos, WPX_SEEK_SET);

	static char const * (zName[]) =
	{ "BTEC", "BTEP", "SHdr", "SFtr", "DLINK", "FTNp", "FTNd", "BKMK", "FONT" };

	for (int i = 0; i < 9; i++)
		m_mainParser.parseEntry(zName[i]);

	return true;
}

////////////////////////////////////////////////////////////
// find all the text structures
////////////////////////////////////////////////////////////
bool WPS4Text::readStructures()
{
	WPS4Parser::NameMultiMap &nameMultiMap = m_mainParser.m_nameMultiMap;
	WPS4Parser::NameMultiMap::iterator pos;

	// first find the font name
	pos = nameMultiMap.find("FONT");
	if (pos != nameMultiMap.end()) readFontNames(pos->second);

	// now find the character and paragraph properties
	for (int i = 0; i < 2; i++)
	{
		// we begin by i = 1 to create firsts the fdpc structure
		if (findFDPStructures(1-i)) continue;
		findFDPStructuresByHand(1-i);
	}

	/* read character FODs (FOrmatting Descriptors) */
	size_t numFDP = m_state->m_FDPCs.size();
	std::vector<WPS4TextInternal::DataFOD> fdps;
	for (size_t i = 0; i < numFDP; i++)
		readFDP(m_state->m_FDPCs[i], fdps, &WPS4Text::readFont);
	m_state->m_FODList = m_state->mergeSortedFODList(fdps, m_state->m_FODList);


	/* read paragraphs FODs (FOrmatting Descriptors) */
	fdps.resize(0);
	numFDP = m_state->m_FDPPs.size();
	for (size_t i = 0; i < numFDP; i++)
		readFDP(m_state->m_FDPPs[i], fdps, &WPS4Text::readParagraph);
	m_state->m_FODList = m_state->mergeSortedFODList(fdps, m_state->m_FODList);

	/* read the object structures */
	pos = nameMultiMap.find("EOBJ");
	if (pos != nameMultiMap.end())
	{
		std::vector<long> textPtrs, listValues;
		readPLC(pos->second, textPtrs, listValues, &WPS4Text::objectDataParser);
	}

	// update the footnote
	WPSEntry ftnD, ftnP;
	pos = nameMultiMap.find("FTNd");
	if (pos !=  m_mainParser.m_nameMultiMap.end()) ftnD = pos->second;
	pos = nameMultiMap.find("FTNp");
	if (pos !=  m_mainParser.m_nameMultiMap.end()) ftnP = pos->second;
	readFootNotes(ftnD, ftnP);

	// bookmark
	pos = nameMultiMap.find("BKMK");
	if (pos != nameMultiMap.end())
	{
		std::vector<long> textPtrs, listValues;
		readPLC(pos->second, textPtrs, listValues, &WPS4Text::bkmkDataParser);
	}

	// the list of file
	pos = nameMultiMap.find("DLINK");
	if (pos != nameMultiMap.end())
		readDosLink(pos->second);

	// date/time format
	pos = nameMultiMap.find("DTTM");
	if (pos != nameMultiMap.end())
	{
		WPSEntry const &zone = pos->second;
		std::vector<long> textPtrs, listValues;
		readPLC(zone, textPtrs, listValues, &WPS4Text::dttmDataParser);
	}

	// finally, we must remove the footnote of textposition...
	long bot = m_state->m_main.begin();
	long endPos = m_state->m_main.end();
	size_t numFootNotes = m_state->m_footnoteList.size(), actNote = 0;
	bool textPosUpdated = false;
	while (bot < endPos)
	{
		if (actNote < numFootNotes &&
		        m_state->m_footnoteList[actNote].begin()==bot)
		{
			bot = m_state->m_footnoteList[actNote].end();
			actNote++;
			continue;
		}
		long lastPos = actNote < numFootNotes ?
		               m_state->m_footnoteList[actNote].begin() : endPos;
		if (lastPos > endPos) lastPos = endPos;
		WPSEntry mZone;
		mZone.setBegin(bot);
		mZone.setEnd(lastPos);
		mZone.setType("TEXT");
		if (!textPosUpdated)
		{
			mZone.setId(WPS4TextInternal::Z_Main);
			m_state->m_main = mZone;
			textPosUpdated = true;
		}
		else
		{
			if (m_state->m_otherZones.size() == 0)
			{
				WPS_DEBUG_MSG(("WPS4Text::readStructures: find unknown text zone\n"));
			}
			mZone.setId(WPS4TextInternal::Z_Unknown);
			m_state->m_otherZones.push_back(mZone);
		}
		bot = lastPos;
	}


	return true;
}

////////////////////////////////////////////////////////////
//  find FDP zones ( normal method followed by another method
//   which may works for some bad files )
////////////////////////////////////////////////////////////
bool WPS4Text::findFDPStructures(int which)
{
	std::vector<WPSEntry> &zones = which ? m_state->m_FDPCs : m_state->m_FDPPs;
	zones.resize(0);

	char const *indexName = which ? "BTEC" : "BTEP";
	char const *sIndexName = which ? "FDPC" : "FDPP";

	WPS4Parser::NameMultiMap &nameMultiMap = m_mainParser.m_nameMultiMap;
	WPS4Parser::NameMultiMap::iterator pos = nameMultiMap.find(indexName);
	if (pos == nameMultiMap.end()) return false;

	std::vector<long> textPtrs;
	std::vector<long> listValues;

	if (!readPLC(pos->second, textPtrs, listValues)) return false;

	size_t numV = listValues.size();
	if (textPtrs.size() != numV+1) return false;

	WPSEntry zone;
	zone.setType(sIndexName);

	for (size_t i = 0; i < numV; i++)
	{
		long bPos = listValues[i];
		if (bPos <= 0) return false;
		zone.setBegin(bPos);
		zone.setLength(0x80);

		zones.push_back(zone);
	}

	return true;
}

bool WPS4Text::findFDPStructuresByHand(int which)
{
	char const *indexName = which ? "FDPC" : "FDPP";
	WPS_DEBUG_MSG(("WPS4Text::findFDPStructuresByHand: need to create %s list by hand \n", indexName));

	std::vector<WPSEntry> &zones = which ? m_state->m_FDPCs : m_state->m_FDPPs;
	zones.resize(0);

	long debPos;
	if (which == 1)
	{
		// hack: each fdp block is aligned with 0x80,
		//       and appears consecutively just after the text
		uint32_t pnChar = uint32_t((m_state->m_text.end()+127)>>7);
		/* sanity check */
		if (0 == pnChar)
		{
			WPS_DEBUG_MSG(("WPS4Text::findFDPStructuresByHand: pnChar is 0, so file may be corrupt\n"));
			throw libwps::ParseException();
		}
		debPos = 0x80 * pnChar;
	}
	else
	{
		size_t nFDPC = m_state->m_FDPCs.size();
		if (!nFDPC)
		{
			WPS_DEBUG_MSG(("WPS4Text::findFDPStructuresByHand: can not find last fdpc pos\n"));
			return false;
		}
		debPos = m_state->m_FDPCs[nFDPC-1].end();
	}

	WPSEntry fdp;
	fdp.setType(indexName);

	long lastPos = m_state->m_text.begin();
	while (1)
	{
		m_input->seek(debPos+0x7f, WPX_SEEK_SET);
		if (m_input->tell() != debPos+0x7f)
		{
			WPS_DEBUG_MSG(("WPS4Text: find EOF while parsing the %s\n", indexName));
			return false;
		}
		int nbElt = libwps::readU8(m_input);
		if (5*nbElt+4 > 0x80)
		{
			WPS_DEBUG_MSG(("WPS4Text: find too big number of data while parsing the %s\n", indexName));
			return false;
		}
		m_input->seek(debPos, WPX_SEEK_SET);
		if (long(libwps::readU32(m_input)) != lastPos)
		{
			WPS_DEBUG_MSG(("WPS4Text: find incorrect linking while parsing the %s\n", indexName));
			return false;
		}
		if (nbElt != 1)
			m_input->seek(4*nbElt-4, WPX_SEEK_CUR);

		long newPos = libwps::readU32(m_input);
		if (newPos < lastPos || newPos > m_state->m_text.end())
		{
			WPS_DEBUG_MSG(("WPS4Text: find incorrect linking while parsing the %s\n", indexName));
			return false;
		}
		fdp.setBegin(debPos);
		fdp.setLength(0x80);
		zones.push_back(fdp);

		if (newPos == m_state->m_text.end()) break;

		lastPos = newPos;
		debPos = fdp.end();
	}

	return true;
}

////////////////////////////////////////////////////////////
// read a fdp ( note this function may also be used in WPS8 )
////////////////////////////////////////////////////////////
bool WPS4Text::readFDP(WPSEntry const &entry,
                       std::vector<WPS4TextInternal::DataFOD> &fods,
                       WPS4Text::FDPParser parser)
{
	if (entry.length() <= 0 || entry.begin() <= 0)
	{
		WPS_DEBUG_MSG(("WPS4Text::readFDP: warning: FDP entry unintialized"));
		return false;
	}

	entry.setParsed();
	uint32_t page_offset = uint32_t(entry.begin());
	long length = entry.length();
	long endPage = entry.end();

	bool smallFDP = version() < 5;
	int deplSize = smallFDP ? 1 : 2;
	int headerSize = smallFDP ? 4 : 8;

	if (length < headerSize)
	{
		WPS_DEBUG_MSG(("WPS4Text::readFDP: warning: FDP offset=0x%X, length=0x%lx\n",
		               page_offset, length));
		return false;
	}

	libwps::DebugStream f;
	if (smallFDP)
	{
		endPage--;
		m_input->seek(endPage, WPX_SEEK_SET);
	}
	else
		m_input->seek(page_offset, WPX_SEEK_SET);
	uint16_t cfod = deplSize == 1 ? (uint16_t) libwps::readU8(m_input) : libwps::readU16(m_input);

	f << "Entries(" << entry.type() << "): N="<<(int) cfod;
	if (smallFDP) m_input->seek(page_offset, WPX_SEEK_SET);
	else f << ", unk=" << libwps::read16(m_input);

	if (headerSize+(4+deplSize)*cfod > length)
	{
		WPS_DEBUG_MSG(("WPS4Text::readFDP: cfod = %i (0x%X)\n", cfod, cfod));
		return false;
	}

	int firstFod = int(fods.size());
	long lastLimit = firstFod ? fods.back().m_pos : 0;

	long lastReadPos = 0L;

	WPS4TextInternal::DataFOD::Type type = WPS4TextInternal::DataFOD::ATTR_UNKN;
	if (entry.hasType("FDPC")) type = WPS4TextInternal::DataFOD::ATTR_TEXT;
	else if (entry.hasType("FDPP")) type = WPS4TextInternal::DataFOD::ATTR_PARAG;
	else
	{
		WPS_DEBUG_MSG(("WPS4Text::readFDP: unknown type = '%s'\n", entry.type().c_str()));
	}

	/* Read array of fcLim of FODs.  The fcLim refers to the offset of the
	   last character covered by the formatting. */
	for (int i = 0; i <= cfod; i++)
	{
		WPS4TextInternal::DataFOD fod;
		fod.m_type = type;
		fod.m_pos = libwps::readU32(m_input);
		if (fod.m_pos == 0) fod.m_pos=m_state->m_text.begin();

		/* check that fcLim is not too large */
		if (fod.m_pos > m_state->m_text.end())
		{
			WPS_DEBUG_MSG(("WPS4Text::readFDP: length of 'text selection' %ld > "
			               "total text length %ld\n", fod.m_pos, m_state->m_text.end()));
			return false;
		}

		/* check that pos is monotonic */
		if (lastLimit > fod.m_pos)
		{
			WPS_DEBUG_MSG(("WPS4Text::readFDP: character position list must "
			               "be monotonic, but found %ld, %ld\n", lastLimit, fod.m_pos));
			return false;
		}

		lastLimit = fod.m_pos;

		if (i != cfod)
			fods.push_back(fod);
		else // ignore the last text position
			lastReadPos = fod.m_pos;
	}

	std::vector<WPS4TextInternal::DataFOD>::iterator fods_iter;
	/* Read array of bfprop of FODs.  The bfprop is the offset where
	   the FPROP is located. */
	f << ", Tpos:defP=(" << std::hex;
	for (fods_iter = fods.begin() + firstFod; fods_iter!= fods.end(); fods_iter++)
	{
		unsigned depl = deplSize == 1 ? libwps::readU8(m_input) : libwps::readU16(m_input);
		/* check size of bfprop  */
		if ((depl < unsigned(headerSize+(4+deplSize)*cfod) && depl > 0) ||
		        long(page_offset+depl)  > endPage)
		{
			WPS_DEBUG_MSG(("WPS4Text::readFDP: pos of bfprop is bad "
			               "%i (0x%X)\n", depl, depl));
			return false;
		}

		if (depl)
			(*fods_iter).m_defPos = depl + page_offset;

		f << (*fods_iter).m_pos << ":";
		if (depl) f << (*fods_iter).m_defPos << ", ";
		else f << "_, ";
	}
	f << "), lstPos=" << lastReadPos << ", ";

	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());
	ascii().addPos(m_input->tell());

	std::map<long,int> mapPtr;
	bool smallSzInProp = smallFDP ? true : false;
	for (fods_iter = fods.begin() + firstFod; fods_iter!= fods.end(); fods_iter++)
	{
		long pos = (*fods_iter).m_defPos;
		if (pos == 0) continue;

		std::map<long,int>::iterator it= mapPtr.find(pos);
		if (it != mapPtr.end())
		{
			(*fods_iter).m_id = mapPtr[pos];
			continue;
		}

		m_input->seek(pos, WPX_SEEK_SET);
		int szProp = smallSzInProp ? libwps::readU8(m_input) : libwps::readU16(m_input);
		if (smallSzInProp) szProp++;
		if (szProp == 0)
		{
			WPS_DEBUG_MSG(("WPS4Text::readFDP: 0 == szProp at file offset 0x%lx\n", (m_input->tell())-1));
			return false;
		}
		long endPos = pos+szProp;
		if (endPos > endPage)
		{
			WPS_DEBUG_MSG(("WPS4Text::readFDP: cch = %d, too large\n", szProp));
			return false;
		}

		ascii().addPos(endPos);
		ascii().addPos(pos);
		int id;
		std::string mess;
		if (parser &&(this->*parser) (endPos, id, mess) )
		{
			(*fods_iter).m_id = mapPtr[pos] = id;

			f.str("");
			f << entry.type()  << std::dec << id <<":" << mess;
			ascii().addNote(f.str().c_str());
			pos = m_input->tell();
		}

		if (pos != endPos)
		{
			ascii().addPos(pos);
			f.str("");
			f << entry.type() << "###";
		}
	}

	/* go to end of page */
	m_input->seek(endPage, WPX_SEEK_SET);

	return m_state->m_text.end() > lastReadPos;
}

// PLC Data: default parser
bool WPS4Text::defDataParser(long , long , int , long endPos, std::string &mess)
{
	mess = "";
	libwps::DebugStream f;

	long actPos = m_input->tell();
	long length = endPos+1-actPos;
	int sz = (length%4)==0 ? 4 : (length%2)==0 ? 2 : 1;
	f << "unk["<< sz << "]=";
	while (m_input->tell() <= endPos+1-sz)
	{
		long val = 0;
		switch(sz)
		{
		case 1:
			val = libwps::readU8(m_input);
			break;
		case 2:
			val = libwps::readU16(m_input);
			break;
		case 4:
			val = libwps::readU32(m_input);
			break;
		default:
			break;
		}
		f << std::hex << val << std::dec << ",";
	}
	mess = f.str();
	return true;
}

////////////////////////////////////////////////////////////
// the fonts name zone (zone8)
////////////////////////////////////////////////////////////
bool WPS4Text::readFontNames(WPSEntry const &entry)
{
	if (!entry.valid()) return false;

	m_input->seek(entry.begin(), WPX_SEEK_SET);

	long endPos = entry.end();
	int nFonts = 0;
	while (m_input->tell() < endPos)
	{
		long actPos;
		actPos = m_input->tell();
		libwps::DebugStream f;

		/* Sometimes the font numbers start at 0 and increment nicely.
		   However, other times the font numbers jump around. */
		uint8_t font_number = libwps::readU8(m_input);
		if (m_state->m_fontNames.find(font_number) != m_state->m_fontNames.end())
		{
			WPS_DEBUG_MSG(("WPS4Text::readFontNames: at position 0x%lx: font number %i duplicated\n",
			               (m_input->tell())-2, font_number));
			throw libwps::ParseException();
		}

		f << "Font" << nFonts++ << ": id=" << (int)font_number << ", ";
		//fixme: what is this byte? maybe a font class
		uint8_t unknown_byte = libwps::readU8(m_input);
		f << "unk=" << (int)unknown_byte << ", ";

		std::string s;
		uint8_t nChar = libwps::readU8(m_input);
		for (uint8_t i = nChar; i>0; i--)
		{
			if (m_input->atEOS())
			{
				WPS_DEBUG_MSG(("WPS4Text::readFontNames: can not read the font number %i (end of file)\n",
				               font_number));
				throw libwps::ParseException();
			}
			unsigned char val = libwps::readU8(m_input);
			// sanity check (because sometimes contains char > 0x80 .. )
			if (val >= ' ' && val <= 'z') s.append(1,char(val));
			else
			{
				static bool first = true;
				if (first)
				{
					first = false;
					WPS_DEBUG_MSG(("WPS4Text:readFontNames find odd caracters in font name : %d\n", (int) val));
				}
				f << "##oddC=" << (unsigned int) val << ", ";
			}
		}
		libwps_tools_win::Font::Type fType;
		if (version() <= 2) fType = libwps_tools_win::Font::DOS_850; //checkme ?
		else fType = libwps_tools_win::Font::getWin3Type(s);
		;
		WPS4TextInternal::FontName font;
		font.m_name = s;
		font.m_type = fType;
		f << font;

		m_state->m_fontNames[font_number] = font;

		ascii().addPos(actPos);
		ascii().addNote(f.str().c_str());
		ascii().addPos(m_input->tell());
	}

	return true;
}

////////////////////////////////////////////////////////////
// the font:
////////////////////////////////////////////////////////////
void WPS4Text::setProperty(WPS4TextInternal::Font const &ft)
{
	if (m_listener.get() == 0L) return;
	m_listener->setFont(ft);
}

bool WPS4Text::readFont(long endPos, int &id, std::string &mess)
{
	WPS4TextInternal::Font font(version());
	font.m_size = 12;

	libwps::DebugStream f;

	int fl[4] = { 0, 0, 0, 0};
	if (m_input->tell() < endPos) fl[0] = libwps::readU8(m_input);

	/* set difference from default properties */
	uint32_t attributes = 0;
	if (fl[0] & 0x01) attributes |= WPS_BOLD_BIT;
	if (fl[0] & 0x02) attributes |= WPS_ITALICS_BIT;
	if (fl[0] & 0x04) attributes |= WPS_STRIKEOUT_BIT;
	fl[0] &= 0xf8;

	// what & 0x01 -> ???
	// what & 0x02 -> note
	// what & 0x04 -> ???
	// what & 0x08 -> fName
	// what & 0x10 -> size
	// what & 0x20 -> underline (fl[2])
	// what & 0x40 -> decalage
	// what & 0x80 -> color
	int what = 0;
	if (m_input->tell() < endPos) what = libwps::readU8(m_input);

	font.m_special = ((what & 2) != 0);
	what &= 0xfd;

	if (m_input->tell() < endPos)
	{
		// the fonts
		// FIXME: find some properties with size=3,
		//        for which this character seems
		//        related to size, not font
		uint8_t font_n = libwps::readU8(m_input);

		if (m_state->m_fontNames.find(font_n) != m_state->m_fontNames.end())
		{
			font.m_name=m_state->m_fontNames[font_n].m_name;
			font.m_type=m_state->m_fontNames[font_n].m_type;
		}
		else if (version() <= 2)
		{
			font.m_name=WPS4TextInternal::FontName::getDosName(font_n);
			font.m_type=libwps_tools_win::Font::DOS_850;
		}
		else
		{
			WPS_DEBUG_MSG(("WPS4Text: error: encountered font %i which is not indexed\n",
			               font_n ));
		}

		if (font.m_name.empty()) f << "###nameId=" << int(font_n) << ",";
	}

	if (m_input->tell() < endPos)
	{
		// underline, ...
		int underlinePos = libwps::readU8(m_input);
		if (underlinePos)
		{
			if (!(what & 0x20)) f << "undFl,";
			else what &= 0xdf;
			attributes |= WPS_UNDERLINE_BIT;
		}
	}

	if (m_input->tell() < endPos)   // font size * 2
	{
		int fSize = libwps::readU8(m_input);
		if (fSize)
		{
			if (!(what & 0x10)) f << "szFl,";
			else what &= 0xef;
			font.m_size = (fSize/2);
		}
	}

	if (m_input->tell() < endPos)   // height decalage -> sub/superscript
	{
		int fDec = libwps::read8(m_input);
		if (fDec)
		{
			if (!(what & 0x40)) f << "sub/supFl(val=" << fDec<<"),";
			else what &= 0xbf;

			if (fDec > 0) attributes |= WPS_SUPERSCRIPT_BIT;
			else attributes |= WPS_SUBSCRIPT_BIT;
		}
	}
	if (m_input->tell()+2 <= endPos)   // color field
	{
		int bkColor = libwps::readU8(m_input);
		int ftColor = libwps::readU8(m_input);
		bool setColor = (what & 0x80);
		what &= 0x7F;

		if ((bkColor || ftColor) && !setColor)
		{
			setColor = true;
			f << "colorFl,";
		}
		if (setColor)
		{
			uint32_t color;
			if (m_mainParser.getColor(bkColor, color))
				font.m_backColor = color;
			if (m_mainParser.getColor(ftColor, color))
				font.m_color = color;
		}
	}
	if (m_input->tell() < endPos)
		font.m_dlinkId = libwps::readU8(m_input);
	if (fl[0])  f << "unkn0=" << std::hex << fl[0] << ",";

	if (m_input->tell() != endPos)
	{
		f << "#unknEnd=(";
		while (m_input->tell() < endPos) f << std::hex << libwps::readU8(m_input) <<",";
		f << ")";
	}

	font.m_attributes = attributes;
	font.m_extra = f.str();

	id = int(m_state->m_fontList.size());
	m_state->m_fontList.push_back(font);
	f.str("");
	f << font;
	mess = f.str();

	return true;
}

////////////////////////////////////////////////////////////
// the file list: only in dos3  ?
////////////////////////////////////////////////////////////
bool WPS4Text::readDosLink(WPSEntry const &entry)
{
	if (!entry.valid()) return false;

	long length = entry.length();
	if (length%44)
	{
		WPS_DEBUG_MSG(("WPS4Text::readDosLink: length::=%ld seem odd\n", length));
		return false;
	}

	m_input->seek(entry.begin(), WPX_SEEK_SET);
	libwps::DebugStream f;
	long numElt = length/44;
	long val;
	for (long n = 0; n < numElt; n++)
	{
		WPS4TextInternal::DosLink link;
		long pos = m_input->tell();
		long endPos = pos+44;
		f.str("");
		for (int i = 0; i < 2; i++) // always 0, 0
		{
			val = libwps::readU16(m_input);
			if (val) f << "unkn" << i << "=" << std::hex << val << std::dec << ",";
		}
		link.m_width = float(libwps::readU16(m_input)/1440.);
		for (int i = 2; i < 4; i++) // always f0, f0
		{
			val = libwps::readU16(m_input);
			if (val != 0xf0) f << "unkn" << i << "=" << std::hex << val << std::dec << ",";
		}
		link.m_type = libwps::readU8(m_input);
		val = libwps::readU8(m_input);
		if (val) // find 0x18 for a spreadsheet
			f << "unk4=" << std::hex << val << std::dec << ",";
		switch(link.m_type)
		{
		case 0x81: // picture ?
		{
			long dim[2];
			for (int i = 0; i < 2; i++) dim[i] = libwps::readU16(m_input);
			link.m_size = Vec2f(float(dim[0])/1440.f, float(dim[1])/1440.f);
			val = libwps::readU16(m_input); // always 0
			if (val) f << "g0=" << val << ",";
			val = libwps::readU16(m_input); // always 4
			if (val != 4) f << "g1=" << val << ",";
		}
		case 0x40: // spreadsheet range
		case 0x01: // char ?
		{
			std::string name("");
			link.m_pos.setBegin(m_input->tell());
			while (!m_input->atEOS() && long(m_input->tell()) < endPos)
			{
				char c = char(libwps::readU8(m_input));
				if (!c)
				{
					m_input->seek(-1, WPX_SEEK_CUR);
					break;
				}
				name += c;
			}
			link.m_pos.setEnd(m_input->tell());
			link.m_pos.setId(WPS4TextInternal::Z_DLink);
			link.m_name = name;
			break;
		}
		default:
			break;
		}
		link.m_extra = f.str();
		m_state->m_dosLinkList.push_back(link);
		f.str("");
		f << "ZZDLINK-" << n << ":" << link;
		if (long(m_input->tell()) != endPos)
			ascii().addDelimiter(m_input->tell(),'|');
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		m_input->seek(endPos, WPX_SEEK_SET);
	}
	return true;
}

////////////////////////////////////////////////////////////
// the paragraph properties:
////////////////////////////////////////////////////////////
void WPS4Text::setProperty(WPS4TextInternal::Paragraph const &pp)
{
	if (m_listener.get() == 0L) return;
	pp.send(m_listener);
}

bool WPS4Text::readParagraph(long endPos, int &id, std::string &mess)
{
	long actPos = m_input->tell();
	long size = endPos - actPos;

	WPS4TextInternal::Paragraph pp;
	if (size && size < 3)
	{
		WPS_DEBUG_MSG(("WPS4Text:readParagraph:(sz=%ld)\n", size));
		return false;
	}

	libwps::DebugStream f;
	for (int i = 0; i < 3; i++)
	{
		int v = libwps::readU8(m_input);
		if (v != 0) f << "unkn"<<i<< "=" << v;
	}

	while (m_input->tell() < endPos)
	{
		int v = libwps::readU8(m_input);
		long pos = m_input->tell();
		bool ok = true, done = true;
		int arg = -1;
		switch(v)
		{
		case 0x2:
		{
			if (pos+1 > endPos)
			{
				ok = false;
				break;
			}
			arg = libwps::readU8(m_input);
			f << "f2=" << arg << ",";
			break;
		}
		case 0x5:
		{
			if (pos+1 > endPos)
			{
				ok = false;
				break;
			}
			arg = libwps::readU8(m_input);
			switch(arg)
			{
			case 0:
				pp.m_justify = libwps::JustificationLeft;
				break;
			case 1:
				pp.m_justify = libwps::JustificationCenter;
				break;
			case 2:
				pp.m_justify = libwps::JustificationRight;
				break;
			case 3:
				pp.m_justify = libwps::JustificationFull;
				break;
			default:
				f << "#just=" << arg << ",";
				pp.m_justify = libwps::JustificationLeft;
			}
			break;
		}
		case 0x7:   // 1: marked don't break paragraph
		case 0x8:   // 1: marked keep paragraph with next
		{
			if (pos+1 > endPos)
			{
				ok = false;
				break;
			}
			arg = libwps::readU8(m_input);
			if (arg == 0) break;
			if (arg == 1) pp.m_breakStatus |= ((v == 7) ? libwps::NoBreakBit : libwps::NoBreakWithNextBit);
			else f << "#status=" << arg << ",";
			break;
		}

		// BORDER
		case 0x9:
		{
			if (pos+1 > endPos)
			{
				ok = false;
				break;
			}
			arg = libwps::readU8(m_input);
			pp.m_borderStyle = libwps::BorderSingle;
			pp.m_borderWidth = 1;
			int style = (arg&0xf);
			switch(style)
			{
			case 0:
				break;
			case 1:
				pp.m_borderWidth = 2;
				break;
			case 2:
				pp.m_borderStyle = libwps::BorderDouble;
				break;
			case 3:
				pp.m_borderStyle = libwps::BorderDot;
				break;
			case 4:
				pp.m_borderStyle = libwps::BorderLargeDot;
				break;
			case 5:
				pp.m_borderStyle = libwps::BorderDash;
				break;
			case 6:
			case 7:
			case 8:
				pp.m_borderWidth = style-3;
				break;
			case 9:
			case 10:
				pp.m_borderWidth = style-7;
				pp.m_borderStyle = libwps::BorderDouble;
				break;
			default:
				f << "#borderStyle=" << style << ",";
				WPS_DEBUG_MSG(("WPS4Text:readParagraph: unknown border style\n"));
				break;
			}
			int high = (arg>>4);
			if (version() < 3)
			{
				uint32_t color;
				if (high && m_mainParser.getColor(high, color))
					pp.m_borderColor = color;
				else if (high)
					f << "#borderColor=" << high << ",";
			}
			else
			{
				switch(high)
				{
				case 0:
					break;
				case 4:
					pp.m_border = 0xf;
					break;
				case 8:
					pp.m_border = 0xf;
					f << "borderShaded,";
					break;
				default:
					f << "#borderStyle[high]=" << high << ",";
					break;
				}
			}
			break;
		}
		case 0xa: // 1: top border
		case 0xb: //  : bottom border
		case 0xc: //  : left border
		case 0xd: //  : right border
		{
			if (pos+1 > endPos)
			{
				ok = false;
				break;
			}
			arg = libwps::readU8(m_input);
			if (arg == 0) break;
			if (arg == 1)
			{
				switch(v)
				{
				case 0xa:
					pp.m_border |= libwps::TopBorderBit;
					break;
				case 0xb:
					pp.m_border |= libwps::BottomBorderBit;
					break;
				case 0xc:
					pp.m_border |= libwps::LeftBorderBit;
					break;
				case 0xd:
					pp.m_border |= libwps::RightBorderBit;
					break;
				default:
					break;
				}
			}
			else f << "#border=" << arg << ",";
			break;
		}
		case 0x18:   // border color
		{
			if (long(pos)==endPos)
			{
				ok = false;
				break;
			}
			int colorId = libwps::readU8(m_input);
			uint32_t color;
			if (m_mainParser.getColor(colorId, color))
				pp.m_borderColor = color;
			else
				f << "#colorId=" << colorId << ",";
			break;
		}
		case 0xe:   // 1: bullet
		{
			if (pos+1 > endPos)
			{
				ok = false;
				break;
			}
			arg = libwps::readU8(m_input);
			if (arg == 0) break;

			pp.m_listLevelIndex = 1;
			pp.m_listLevel.m_type = libwps::BULLET;
			static const uint32_t bulletList[]=
			{
				0x2022, 0x3e, 0x25c6, 0x21d2, 0x25c7, 0x2605, /* 1-6 */
				0, 0, 0, 0, 0, 0, /* 7-12 unknown */
				0, 0, 0, 0, 0, 0x2750, /* 13-17 unknown and document... */
				0x2713, 0x261e, 0x2704, 0x2611, 0x2612, 0x270e /* 18-24 */
			};
			if (arg <= 24 && bulletList[arg-1])
				WPSContentListener::appendUnicode(bulletList[arg-1], pp.m_listLevel.m_bullet);
			else
				WPSContentListener::appendUnicode(0x2022, pp.m_listLevel.m_bullet);
			break;
		}
		case 0x1b:
		case 0x1a:
		case 0x10:   // the bullet char : 0x18
		{
			if (pos+1 > endPos)
			{
				ok = false;
				break;
			}
			arg = libwps::readU8(m_input);
			done = true;
			switch (v)
			{
			case 0x1a:
				if (arg) f << "backPattern=" << arg << ",";
				break;
			case 0x1b:
			{
				if (arg==0) break;
				uint32_t color;
				if (m_mainParser.getColor(arg>>4, color))
					f << "backPatternBackColor=" << std::hex << color << std::dec << ";";
				else
					f << "#backPatternBackColor=" << (arg>>4) << ",";
				if (m_mainParser.getColor(arg&0xf, color))
					f << "backPatternFrontColor=" << std::hex << color << std::dec << ";";
				else
					f << "#backPatternFrontColor=" << (arg&0xf) << ",";
				break;
			}
			case 0x10:
				if (arg!=0x18) f << "bullet?=" << arg << ",";
				break;
			default:
				done = false;
				break;
			}
			break;
		}
		case 0xf:   // tabs:
		{
			if (pos+1 > endPos)
			{
				ok = false;
				break;
			}
			int nVal = libwps::read8(m_input);
			if (nVal < 2 || pos + 1 + nVal > endPos)
			{
				ok = false;
				break;
			}
			int flag = libwps::readU8(m_input);
			if (flag) f << "#tabsFl=" << flag << ",";
			size_t nItem = libwps::readU8(m_input);
			if ((unsigned long)nVal != 2 + 3*nItem)
			{
				ok = false;
				break;
			}
			pp.m_tabs.resize(nItem);
			for (size_t i = 0; i < nItem; i++)
				pp.m_tabs[i].m_position = libwps::read16(m_input)/1440.;
			for (size_t i = 0; i < nItem; i++)
			{
				enum WPSTabStop::Alignment align = WPSTabStop::LEFT;
				int val = libwps::readU8(m_input);
				switch((val & 0x3))
				{
				case 0:
					align = WPSTabStop::LEFT;
					break;
				case 1:
					align = WPSTabStop::CENTER;
					break;
				case 2:
					align = WPSTabStop::RIGHT;
					break;
				case 3:
					align = WPSTabStop::DECIMAL;
					break;
				default:
					break;
				}
				pp.m_tabs[i].m_alignment = align;

				if (val&4) f << "#Tabbits3";
				val = (val>>3);

				switch(val)
				{
				case 0:
					break;
				case 1:
					pp.m_tabs[i].m_leaderCharacter = '.';
					break;
				case 2:
					pp.m_tabs[i].m_leaderCharacter = '-';
					break;
				case 3:
					pp.m_tabs[i].m_leaderCharacter = '_';
					break;
				case 4:
					pp.m_tabs[i].m_leaderCharacter = '=';
					break;
				default:
					f << "#TabSep=" << val;
				}
			}

			break;
		}
		case 0x11: // right margin : 1440*inches
		case 0x12: // left margin
		case 0x13: // another margin ( check me )
		case 0x14: // left text indent (relative to left margin)
		case 0x15: // line spacing (inter line) 240
		case 0x16: // line spacing before 240 = 1 line spacing
		case 0x17:   // line spacing after
		{
			if (pos+2 > endPos)
			{
				ok = false;
				break;
			}

			arg = libwps::read16(m_input);
			switch(v)
			{
			case 0x11:
				pp.m_margins[2] = arg/1440.;
				break;
			case 0x13: // seems another way to define the left margin
				f << "#left,";
			case 0x12:
				pp.m_margins[1] = arg/1440.;
				break;
			case 0x14:
				pp.m_margins[0] = arg/1440.;
				break;
			case 0x15:
			{
				pp.m_spacings[0] = arg ? arg/240. : 1.0;
				if (pp.m_spacings[0] < 1.0 || pp.m_spacings[0] > 2.0)
				{
					f << "##interLineSpacing=" << pp.m_spacings[0] << ",";
					pp.m_spacings[0] = (pp.m_spacings[0] < 1.0) ? 1.0 : 2.0;
				}
				break;
			}
			case 0x16:
				pp.m_spacings[1] = arg/240.;
				break;
			case 0x17:
				pp.m_spacings[2] = arg/240.;
				break;
			default:
				done = false;
			}
			break;
		}
		default:
			ok = false;
		}
		if (!ok)
		{
			m_input->seek(pos, WPX_SEEK_SET);
			f << "###v" << v<<"=" <<std::hex;
			while (m_input->tell() < endPos)
				f << (int) libwps::readU8(m_input) << ",";
			break;
		}

		if (done) continue;

		f << "f" << v << "=" << std::hex << arg << std::dec << ",";
	}
	if (pp.m_listLevelIndex >= 1)
		pp.m_margins[0] +=  pp.m_margins[1];
	else if (pp.m_margins[0] + pp.m_margins[1] < 0.0)
	{
		// sanity check
		if (pp.m_margins[1] < 0.0) pp.m_margins[1] = 0.0;
		pp.m_margins[0] = -pp.m_margins[1];
	}
	pp.m_extra = f.str();

	id = int(m_state->m_paragraphList.size());
	m_state->m_paragraphList.push_back(pp);

	f.str("");
	f << pp;
	mess = f.str();
	return true;
}

////////////////////////////////////////////////////////////
// the foot note properties:
////////////////////////////////////////////////////////////
bool WPS4Text::readFootNotes(WPSEntry const &ftnD, WPSEntry const &ftnP)
{
	if (!ftnD.valid() && !ftnP.valid()) return true;
	if (!ftnD.valid() || !ftnP.valid())
	{
		WPS_DEBUG_MSG(("WPS4Text::readFootNotes: one of the two entry is not valid, footnote will be ignored\n"));
		return false;
	}

	std::vector<long> footNotePos,footNoteDef, listValues;
	if (!readPLC(ftnP, footNotePos, listValues, &WPS4Text::footNotesDataParser))
	{
		WPS_DEBUG_MSG(("WPS4Text::readFootNotes: can not read positions\n"));
		return false;
	}

	if (!readPLC(ftnD, footNoteDef, listValues))
	{
		WPS_DEBUG_MSG(("WPS4Text::readFootNotes: can not read definitions\n"));
		return false;
	}

	int numFootNotes = int(footNotePos.size())-1;
	if (numFootNotes <= 0 || int(footNoteDef.size())-1 != numFootNotes)
	{
		WPS_DEBUG_MSG(("WPS4Text::readFootNotes: no footnotes\n"));
		return false;
	}

	// save the actual type and create a list of footnote entries
	std::vector<WPS4TextInternal::Note> noteTypes=m_state->m_footnoteList;
	m_state->m_footnoteList.resize(0);

	std::vector<int> corresp;
	for (size_t i = 0; i < size_t(numFootNotes); i++)
	{
		WPS4TextInternal::Note fZone;
		fZone.setBegin(footNoteDef[i]);
		fZone.setEnd(footNoteDef[i+1]);
		fZone.setType("TEXT");
		fZone.setId(WPS4TextInternal::Z_Note);
		m_state->m_footnoteList.push_back(fZone);
		corresp.push_back(int(i));

		// sort the footnote
		for (size_t j = i; j > 0; j--)
		{
			if (m_state->m_footnoteList[j].begin() >=
			        m_state->m_footnoteList[j-1].end()) break;

			if (m_state->m_footnoteList[j].end() >
			        m_state->m_footnoteList[j-1].begin())
			{
				WPS_DEBUG_MSG
				(("WPS4Text: error: can not create footnotes zone, found %lx and %lx\n",
				  m_state->m_footnoteList[j].end(),m_state->m_footnoteList[j-1].begin()));

				m_state->m_footnoteList.resize(0);
				return false;
			}

			WPS4TextInternal::Note tmpZ = m_state->m_footnoteList[j];
			m_state->m_footnoteList[j] = m_state->m_footnoteList[j-1];
			m_state->m_footnoteList[j-1] = tmpZ;

			int pos = corresp[j];
			corresp[j] = corresp[j-1];
			corresp[j-1] = pos;
		}
	}
	// ok, we can create the map, ...
	for (size_t i = 0; i < size_t(numFootNotes); i++)
	{
		size_t id = size_t(corresp[i]);
		WPS4TextInternal::Note &z = m_state->m_footnoteList[id];
		if (id < noteTypes.size())
		{
			z.m_label = noteTypes[id].m_label;
			z.m_error = noteTypes[id].m_error;
		}
		m_state->m_footnoteMap[footNotePos[id]] = &z;
	}
	return true;
}

bool WPS4Text::footNotesDataParser (long /*bot*/, long /*eot*/, int id,
                                    long endPos, std::string &mess)
{
	mess = "";

	long actPos = m_input->tell();
	long length = endPos+1-actPos;
	if (length != 12)
	{
		WPS_DEBUG_MSG(("WPS4Text::footNotesDataParser: unknown size %ld for footdata data\n", length));
		return false;
	}
	libwps::DebugStream f;
	WPS4TextInternal::Note note;
	int type = libwps::readU16(m_input);
	if (type & 1)
	{
		if (type != 1)
			f << "###numeric=" << std::hex << type << std::dec << ",";
	}
	else if (type == 0 || type > 20)
		f << "###char,";
	else
	{
		int numC = type/2;
		WPXString label("");
		libwps_tools_win::Font::Type actType =
		    (version() < 3) ? libwps_tools_win::Font::DOS_850 :
		    libwps_tools_win::Font::WIN3_WEUROPE;

		for (int i=0; i < numC; i++)
		{
			unsigned char c = libwps::readU8(m_input);
			if (c >= 0x20)
				WPSContentListener::appendUnicode(uint32_t(libwps_tools_win::Font::unicode(c, actType)),label);
			else
				f << "#(" << std::hex << int(c) << std::dec << ")";
		}
		note.m_label = label;
	}
	note.m_error=f.str();
	if (id >= int(m_state->m_footnoteList.size()))
		m_state->m_footnoteList.resize(size_t(id+1));
	m_state->m_footnoteList[size_t(id)]=note;
	f.str("");
	f << note;
	mess = f.str();
	m_input->seek(endPos+1, WPX_SEEK_SET);
	return true;
}

////////////////////////////////////////////////////////////
// the bookmark properties:
////////////////////////////////////////////////////////////
bool WPS4Text::bkmkDataParser(long bot, long /*eot*/, int /*id*/,
                              long endPos, std::string &mess)
{
	mess = "";
	if (m_state->m_bookmarkMap.find(bot) != m_state->m_bookmarkMap.end())
	{
		WPS_DEBUG_MSG(("WPS4Text:bkmkDataParser: bookmark already exists in this position\n"));
		return true;
	}

	long actPos = m_input->tell();
	long length = endPos+1-actPos;
	if (length != 16)
	{
		WPS_DEBUG_MSG(("WPS4Text::bkmkDataParser: unknown size %ld for bkmkdata data\n", length));
		return false;
	}

	for (int i = 0; i < 16; i++)
	{
		char c = char(libwps::readU8(m_input));
		if (c == '\0') break;
		mess += c;
	}
	WPSEntry ent;
	ent.setBegin(actPos);
	ent.setEnd(m_input->tell());
	ent.setId(WPS4TextInternal::Z_String);
	m_state->m_bookmarkMap[bot] = ent;
	m_input->seek(endPos+1, WPX_SEEK_SET);
	return true;
}

////////////////////////////////////////////////////////////
// the object properties:
////////////////////////////////////////////////////////////
bool WPS4Text::objectDataParser (long bot, long /*eot*/, int id,
                                 long endPos, std::string &mess)
{
	mess = "";
	if (m_state->m_objectMap.find(bot) != m_state->m_objectMap.end())
	{
		WPS_DEBUG_MSG(("WPS4Text:objectDataParser: object already exists in this position\n"));
		return true;
	}

	libwps::DebugStream f;

	long actPos = m_input->tell();
	long length = endPos+1-actPos;
	if (length != 36)
	{
		WPS_DEBUG_MSG(("WPS4Text:objectDataParser unknown size %ld for object data\n", length));
		return false;
	}

	f << "type(?)=" <<libwps::read16(m_input) << ","; // 3->08 4->4f4d or 68->list?
	for (int i = 0; i < 2; i++)
	{
		int v =libwps::read16(m_input);
		if (v) f << "unkn1:" << i << "=" << v << ",";
	}
	float dim[4];
	for (int i = 0; i < 4; i++)
		dim[i] =float(libwps::read16(m_input)/1440.);

	// CHECKME: the next two sizes are often simillar,
	//         maybe the first one is the original size and the second
	//         size in the document...
	f << "origSz?=[" << dim[0] << "," << dim[1] << "],";

	WPS4TextInternal::Object obj;
	obj.m_size = Vec2f(dim[2], dim[3]); // CHECKME: unit

	long size = libwps::readU32(m_input);
	long pos = libwps::readU32(m_input);

	actPos = m_input->tell();
	if (pos >= 0 && size > 0 && pos+size <= long(m_mainParser.getSizeFile()))
	{
		obj.m_pos.setBegin(pos);
		obj.m_pos.setLength(size);
		obj.m_pos.setId(id);

		int objectId = m_mainParser.readObject(m_input, obj.m_pos);
		if (objectId == -1)
		{
			WPS_DEBUG_MSG(("WPS4Text::objectDataParser: can not find the object %d\n", id));
		}
		obj.m_id = objectId;
		m_state->m_objectMap[bot] = obj;
	}
	else
	{
		WPS_DEBUG_MSG(("WPS4Text::objectDataParser: bad object position\n"));
	}

	m_input->seek(actPos, WPX_SEEK_SET);

	for (int i = 0; i < 7; i++)
	{
		long val =libwps::read16(m_input);
		if (val) f << "unkn2:" << i << "=" << val << ",";
	}

	obj.m_extra = f.str();
	f.str("");
	f << obj;

	mess = f.str();
	return true;
}

////////////////////////////////////////////////////////////
// the dttm properties:
////////////////////////////////////////////////////////////
bool WPS4Text::dttmDataParser (long bot, long /*eot*/, int /*id*/,
                               long endPos, std::string &mess)
{
	mess = "";
	if (m_state->m_dateTimeMap.find(bot) != m_state->m_dateTimeMap.end())
	{
		WPS_DEBUG_MSG(("WPS4Text:dttmDataParser: dttm already exists in this position\n"));
		return true;
	}

	libwps::DebugStream f;

	long actPos = m_input->tell();
	long length = endPos+1-actPos;
	if (length != 42)
	{
		WPS_DEBUG_MSG(("WPS4Text:dttmDataParser unknown size %ld for dttm data\n", length));
		return false;
	}

	WPS4TextInternal::DateTime form;
	int val;
	for (int i = 0; i < 3; i++) // always 0, 0, 0 ?
	{
		val =libwps::read16(m_input);
		if (val) f << "f" << i << "=" << val << ",";
	}
	form.m_type=libwps::read16(m_input);
	val =libwps::read16(m_input); // alway 0 ?
	if (val) f << "f3=" << val << ",";
	// end unknown
	for (int i = 0; i < 16; i++)
	{
		val =libwps::readU16(m_input);
		if (val) f << "g" << i << "=" << std::hex << val << std::dec << ",";
	}
	form.m_extra = f.str();
	m_state->m_dateTimeMap[bot] = form;
	f.str("");
	f << form;
	mess = f.str();
	return true;
}

////////////////////////////////////////
// VERY LOW LEVEL ( plc )
////////////////////////////////////////
/** Internal and low level: the structures of a WPS4Text used to parse PLC*/
namespace WPS4PLCInternal
{
/** Internal and low level: the PLC different types and their structures */
struct PLC
{
	/** the PLC types */
	typedef enum WPS4TextInternal::PLCType PLCType;
	/** the way to define the text positions
	 *
	 * - P_ABS: absolute position,
	 * - P_REL: position are relative to the beginning text offset */
	typedef enum { P_ABS=0, P_REL, P_UNKNOWN} Position;
	/** the type of the content
	 *
	 * - T_CST: size is constant
	 * - T_STRUCT: a structured type ( which unknown size) */
	typedef enum { T_CST=0, T_COMPLEX, T_UNKNOWN} Type;

	//! constructor
	PLC(PLCType w= WPS4TextInternal::Unknown, Position p=P_UNKNOWN, Type t=T_UNKNOWN, unsigned char tChar='\0', int f=1) :
		m_type(w), m_pos(p), m_contentType(t), m_textChar(tChar), m_cstFactor(f) {}

	//! PLC type
	PLCType m_type;
	//! the way to define the text positions
	Position m_pos;
	//! the type of the content
	Type m_contentType;
	/** the character which appears in the text when this PLC is found
	 *
	 * '\\0' means that there is not default character */
	unsigned char m_textChar;
	//! some data are stored divided by some unit
	int m_cstFactor;
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
	m_knowns["BTEP"] =
	    PLC(WPS4TextInternal::BTE, PLC::P_ABS, PLC::T_CST, '\0', 0x80);
	m_knowns["BTEC"] =
	    PLC(WPS4TextInternal::BTE,PLC::P_ABS, PLC::T_CST, '\0', 0x80);
	m_knowns["EOBJ"] =
	    PLC(WPS4TextInternal::OBJECT,PLC::P_UNKNOWN, PLC::T_COMPLEX, 0x7);
	m_knowns["FTNp"] =
	    PLC(WPS4TextInternal::FTNp,PLC::P_REL, PLC::T_CST, 0x6);
	m_knowns["FTNd"] =
	    PLC(WPS4TextInternal::FTNd,PLC::P_REL, PLC::T_COMPLEX, 0x6);
	m_knowns["BKMK"] =
	    PLC(WPS4TextInternal::BKMK,PLC::P_REL, PLC::T_COMPLEX);
	m_knowns["DTTM"] =
	    PLC(WPS4TextInternal::DTTM,PLC::P_REL, PLC::T_COMPLEX, 0xf);
}
}

bool WPS4Text::readPLC
(WPSEntry const &zone,
 std::vector<long> &textPtrs, std::vector<long> &listValues, WPS4Text::DataParser parser)
{
	textPtrs.resize(0);
	listValues.resize(0);
	long size = zone.length();
	if (zone.begin() <= 0 || size < 8) return false;
	WPS4PLCInternal::PLC plcType = m_state->m_knownPLC.get(zone.type());

	libwps::DebugStream f;
	ascii().addPos(zone.begin());
	m_input->seek(zone.begin(), WPX_SEEK_SET);

	long lastPos = 0;
	std::vector<WPS4TextInternal::DataFOD> fods;
	unsigned numElt = 0;
	f << "pos=(";
	while (numElt*4+4 <= unsigned(size))
	{
		long newPos = libwps::readU32(m_input);
		if (plcType.m_pos == WPS4PLCInternal::PLC::P_UNKNOWN)
		{
			if (newPos < m_state->m_text.begin())
				plcType.m_pos = WPS4PLCInternal::PLC::P_REL;
			else if (newPos+m_state->m_text.begin() > m_state->m_text.end())
				plcType.m_pos = WPS4PLCInternal::PLC::P_ABS;
			else if (plcType.m_textChar=='\0')
			{
				WPS_DEBUG_MSG(("WPS4Text:readPLC Can not decide position for PLC: %s\n", zone.type().c_str()));
				plcType.m_pos = WPS4PLCInternal::PLC::P_REL;
			}
			else
			{
				long actPos = m_input->tell();
				m_input->seek(newPos, WPX_SEEK_SET);
				if (libwps::readU8(m_input) == plcType.m_textChar)
					plcType.m_pos = WPS4PLCInternal::PLC::P_ABS;
				else plcType.m_pos = WPS4PLCInternal::PLC::P_REL;
				m_input->seek(actPos, WPX_SEEK_SET);
			}
		}

		if (plcType.m_pos == WPS4PLCInternal::PLC::P_REL)
			newPos += m_state->m_text.begin();

		if (newPos < lastPos ||
		        newPos > m_state->m_text.end())
		{
			// sometimes the convertissor do not their jobs correctly
			// for the last element
			if (plcType.m_pos == WPS4PLCInternal::PLC::P_REL &&
			        newPos == m_state->m_text.end()+m_state->m_text.begin())
				newPos = m_state->m_text.end();
			else
				return false;
		}

		textPtrs.push_back(newPos);

		WPS4TextInternal::DataFOD fod;
		fod.m_type = WPS4TextInternal::DataFOD::ATTR_PLC;
		fod.m_pos = newPos;

		f << std::hex << newPos << ", ";
		if (newPos == m_state->m_text.end()) break;

		numElt++;
		lastPos = newPos;
		fods.push_back(fod);
	}
	f << ")";

	if (numElt < 1) return false;

	long dataSize = (size-4*numElt-4)/numElt;
	if (dataSize > 100) return false;
	if (unsigned(size)!= numElt*(4+dataSize)+4) return false;

	ascii().addNote(f.str().c_str());

	if (!dataSize)
	{
		for (size_t i = 0; i < numElt; i++)
		{
			listValues.push_back(-1);
			fods[i].m_id = int(m_state->m_plcList.size());
		}
		WPS4TextInternal::DataPLC plc;
		plc.m_name = zone.type();
		plc.m_type = plcType.m_type;
		m_state->m_plcList.push_back(plc);
		m_state->m_FODList = m_state->mergeSortedFODList(fods, m_state->m_FODList);
		return true;
	}

	// ok we have some data
	bool ok = true;
	long pos = m_input->tell();
	WPS4Text::DataParser pars = parser;
	if ((dataSize == 3 || dataSize > 4) && !pars)
		pars = &WPS4Text::defDataParser;

	for (size_t i = 0; i < numElt; i++)
	{
		WPS4TextInternal::DataPLC plc;

		if (!pars && dataSize <= 4)
		{
			switch(dataSize)
			{
			case 1:
				plc.m_value = libwps::readU8(m_input);
				break;
			case 2:
				plc.m_value = libwps::readU16(m_input);
				break;
			case 4:
				plc.m_value = libwps::readU32(m_input);
				break;
			default:
				WPS_DEBUG_MSG(("WPS4Text:readPLC: unexpected PLC size\n"));
			case 0:
				plc.m_value = 0;
			}
			plc.m_value *=plcType.m_cstFactor;
		}
		else
		{
			std::string mess;
			if (!(this->*pars)(textPtrs[i], textPtrs[i+1], int(i), pos+dataSize-1, mess))
			{
				ok = false;
				break;
			}
			plc.m_extra = mess;
			m_input->seek(pos+dataSize, WPX_SEEK_SET);
		}

		listValues.push_back(plc.m_value);

		fods[i].m_id = int(m_state->m_plcList.size());
		fods[i].m_defPos = pos;

		plc.m_name = zone.type();
		plc.m_type = plcType.m_type;
		m_state->m_plcList.push_back(plc);

		f.str("");
		f << "ZZ" << zone.type() << i << ":" << plc;
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());

		pos += dataSize;
	}

	if (ok) m_state->m_FODList = m_state->mergeSortedFODList(fods, m_state->m_FODList);
	return true;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
