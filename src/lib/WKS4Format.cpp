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

#include "WKS4Format.h"

#include "WKS4.h"

using namespace libwps;

//! Internal: namespace to define internal class of WKS4Format
namespace WKS4FormatInternal
{
//! small struct used to defined a font name
struct FontName
{
	//! constructor
	FontName() : m_name(), m_id(-2)
	{
		for (int i=0; i<2; ++i) m_size[i]=0;
	}
	//! the font name
	std::string m_name;
	//! the font id
	int m_id;
	//! the font height, font size
	int m_size[2];
};

//! the state of WKS4Format
struct State
{
	//! constructor
	explicit State() : m_eof(-1), m_idFontNameMap()
	{
	}

	//! the last file position
	long m_eof;
	//! a map id to font name style
	std::map<int, FontName> m_idFontNameMap;
};

}

// constructor, destructor
WKS4Format::WKS4Format(WKS4Parser &parser, RVNGInputStreamPtr input) :
	m_input(input), m_mainParser(parser), m_state(new WKS4FormatInternal::State()), m_asciiFile()
{
}

WKS4Format::~WKS4Format()
{
}

bool WKS4Format::checkFilePosition(long pos)
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


// main function to parse the document
bool WKS4Format::parse()
{
	if (!m_input)
	{
		WPS_DEBUG_MSG(("WKS4Format::parse: does not find main file\n"));
		return false;
	}

	if (!checkHeader()) return false;

	bool ok=false;
	try
	{
		ascii().setStream(m_input);
		ascii().open("FMT");

		ok = checkHeader() && readZones();
	}
	catch (...)
	{
		WPS_DEBUG_MSG(("WKS4Format::parse: exception catched when parsing FMT\n"));
		return false;
	}

	ascii().reset();
	return ok;
}

////////////////////////////////////////////////////////////
// low level
////////////////////////////////////////////////////////////
// read the header
////////////////////////////////////////////////////////////
bool WKS4Format::checkHeader(bool strict)
{
	m_state.reset(new WKS4FormatInternal::State);
	libwps::DebugStream f;

	if (!checkFilePosition(12))
	{
		WPS_DEBUG_MSG(("WKS4Format::checkHeader: file is too short\n"));
		return false;
	}

	m_input->seek(0,librevenge::RVNG_SEEK_SET);
	int firstOffset = (int) libwps::readU8(m_input);
	int type = (int) libwps::read8(m_input);
	f << "FileHeader:FMT,";
	if (firstOffset != 0 || type != 0)
	{
		WPS_DEBUG_MSG(("WKS4Format::checkHeader: find unexpected first data\n"));
		return false;
	}
	int val=(int) libwps::read16(m_input);
	if (val==2)
	{
		// version
		val=(int) libwps::readU16(m_input);
		if (val!=0x8006)
		{
			WPS_DEBUG_MSG(("WKS4Format::checkHeader: find unknown file version\n"));
			return false;
		}
	}
	else
	{
		WPS_DEBUG_MSG(("WKS4Format::checkHeader: header contain unexpected size field data\n"));
		return false;
	}

	m_input->seek(0, librevenge::RVNG_SEEK_SET);
	if (strict)
	{
		for (int i=0; i < 4; ++i)
		{
			if (!readZone()) return false;
		}
	}
	ascii().addPos(0);
	ascii().addNote(f.str().c_str());

	return true;
}

bool WKS4Format::readZones()
{
	m_input->seek(0, librevenge::RVNG_SEEK_SET);
	while (readZone()) ;

	//
	// look for ending
	//
	long pos = m_input->tell();
	if (!checkFilePosition(pos+4))
	{
		WPS_DEBUG_MSG(("WKS4Format::readZones: cell header is too short\n"));
		return false;
	}
	int type = (int) libwps::readU16(m_input); // 1
	int length = (int) libwps::readU16(m_input);
	if (length)
	{
		WPS_DEBUG_MSG(("WKS4Format::readZones: parse breaks before ending\n"));
		ascii().addPos(pos);
		ascii().addNote("Entries(BAD):###");
		return false;
	}

	ascii().addPos(pos);
	if (type != 1)
	{
		WPS_DEBUG_MSG(("WKS4Format::readZones: odd end cell type: %d\n", type));
		ascii().addNote("Entries(BAD):###");
	}
	else
		ascii().addNote("__End");

	return true;
}

bool WKS4Format::readZone()
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	int id = (int) libwps::readU8(m_input);
	int type = (int) libwps::read8(m_input);
	long sz = (long) libwps::readU16(m_input);
	if (sz<0 || !checkFilePosition(pos+4+sz))
	{
		WPS_DEBUG_MSG(("WKS4Format::readZone: size is bad\n"));
		m_input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}

	f << "Entries(FMT" << std::hex << id << std::dec << "E):";
	bool ok = true, isParsed = false, needWriteInAscii = false;
	int val;
	m_input->seek(pos, librevenge::RVNG_SEEK_SET);
	switch (type)
	{
	case 0:
		switch (id)
		{
		case 0:
			if (sz!=2) break;
			f.str("");
			f << "version=" << std::hex << libwps::readU16(m_input) << std::dec << ",";
			isParsed=needWriteInAscii=true;
			break;
		case 0x1: // EOF
			ok = false;
			break;
		// boolean
		case 0x2: // Calculation mode 0 or FF
			f.str("");
			f << "Entries(Byte" << std::hex << id << std::dec << "Z):";
			if (sz!=1) break;
			val=int(libwps::readU8(m_input));
			if (val==0xFF) f << "true,";
			else if (val) f << "#val=" << val << ",";
			isParsed=needWriteInAscii=true;
			break;
		case 0xae:
			isParsed=readFontName();
			break;
		case 0xaf:
		case 0xb1:
			isParsed=readFontSize();
			break;
		case 0xb0:
			isParsed=readFontId();
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
		m_input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	if (isParsed)
	{
		if (needWriteInAscii)
		{
			ascii().addPos(pos);
			ascii().addNote(f.str().c_str());
		}
		m_input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
		return true;
	}

	if (sz && m_input->tell()!=pos && m_input->tell()!=pos+4+sz)
		ascii().addDelimiter(m_input->tell(),'|');
	m_input->seek(pos+4+sz, librevenge::RVNG_SEEK_SET);
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
//   Font
////////////////////////////////////////////////////////////
bool WKS4Format::readFontName()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	int type = (int) libwps::read16(m_input);
	if (type!=0xae)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontName: not a font name definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	long endPos=pos+4+sz;
	f << "Entries(FMTFont)[name]:";
	if (sz < 2)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontName: the zone is too short\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	int id=(int) libwps::readU8(m_input);
	f << "id=" << id << ",";
	bool nameOk=true;
	std::string name("");
	for (long i=1; i<sz; ++i)
	{
		char c=(char) libwps::readU8(m_input);
		if (!c) break;
		if (nameOk && !(c==' ' || (c>='0'&&c<='9') || (c>='a'&&c<='z') || (c>='A'&&c<='Z')))
		{
			nameOk=false;
			WPS_DEBUG_MSG(("WKS4Format::readFontName: find odd character in name\n"));
			f << "#";
		}
		name += c;
	}
	f << name << ",";
	if (m_state->m_idFontNameMap.find(id)!=m_state->m_idFontNameMap.end())
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontName: can not update font map for id=%d\n", id));
	}
	else
	{
		WKS4FormatInternal::FontName font;
		font.m_name=name;
		m_state->m_idFontNameMap[id]=font;
	}
	if (m_input->tell()!=endPos)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontName: find extra data\n"));
		f << "###extra";
		m_input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Format::readFontId()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	int type = (int) libwps::read16(m_input);
	if (type!=0xb0)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontId: not a font id definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	long endPos=pos+4+sz;
	f << "Entries(FMTFont)[ids]:";
	if ((sz%2)!=0)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontId: the zone size is odd\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	f << "ids=[";
	bool isFirstError=true;
	for (int i=0; i<int(sz)/2; ++i)
	{
		int id =int(libwps::readU16(m_input));
		f << id << ",";
		if (m_state->m_idFontNameMap.find(i)!=m_state->m_idFontNameMap.end())
			m_state->m_idFontNameMap.find(i)->second.m_id=id;
		else if (isFirstError)
		{
			isFirstError=false;
			WPS_DEBUG_MSG(("WKS4Format::readFontId: can not update some font map for id=%d\n", id));
		}
	}
	f << "],";
	if (m_input->tell()!=endPos)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontId: find extra data\n"));
		f << "###extra";
		m_input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}

bool WKS4Format::readFontSize()
{
	libwps::DebugStream f;

	long pos = m_input->tell();
	int type = (int) libwps::read16(m_input);
	if (type!=0xaf && type!=0xb1)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontSize: not a font size definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(m_input);
	long endPos=pos+4+sz;
	int const wh=type==0xaf ? 0 : 1;
	f << "Entries(FMTFont)[size" << wh << "]:";
	if ((sz%2)!=0)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontSize: the zone size is odd\n"));
		f << "###";
		ascii().addPos(pos);
		ascii().addNote(f.str().c_str());
		return true;
	}
	f << "size=[";
	bool isFirstError=true;
	for (int i=0; i<int(sz)/2; ++i)
	{
		int size =int(libwps::readU16(m_input));
		f << size << ",";
		if (m_state->m_idFontNameMap.find(i)!=m_state->m_idFontNameMap.end())
			m_state->m_idFontNameMap.find(i)->second.m_size[wh]=size;
		else if (isFirstError)
		{
			isFirstError=false;
			WPS_DEBUG_MSG(("WKS4Format::readFontSize: can not update some font map for size=%d\n", size));
		}
	}
	f << "],";
	if (m_input->tell()!=endPos)
	{
		WPS_DEBUG_MSG(("WKS4Format::readFontSize: find extra data\n"));
		f << "###extra";
		m_input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
	return true;
}
////////////////////////////////////////////////////////////
//   Unknown
////////////////////////////////////////////////////////////


/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
