/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwps.sourceforge.net
 */

#include <stdlib.h>
#include <string.h>

#include "libwps_internal.h"

#include "WPSParser.h"

#include "WPSTextParser.h"

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////
WPSTextParser::WPSTextParser(WPSParser &parser, RVNGInputStreamPtr &input) :
	m_version(0), m_input(input),  m_mainParser(parser),
	m_textPositions(), m_FODList(), m_asciiFile(parser.ascii())
{
}

WPSTextParser::~WPSTextParser() {}


int WPSTextParser::version() const
{
	if (m_version <= 0)
		m_version=m_mainParser.version();
	return m_version;
}

WPSParser::NameMultiMap &WPSTextParser::getNameEntryMap()
{
	return m_mainParser.getNameEntryMap();
}

WPSParser::NameMultiMap const &WPSTextParser::getNameEntryMap() const
{
	return m_mainParser.getNameEntryMap();
}

////////////////////////////////////////////////////////////
// read data
////////////////////////////////////////////////////////////
bool WPSTextParser::readFDP(WPSEntry const &entry,
                            std::vector<DataFOD> &fods,
                            WPSTextParser::FDPParser parser)
{
	RVNGInputStreamPtr input = getInput();
	if (entry.length() <= 0 || entry.begin() <= 0)
	{
		WPS_DEBUG_MSG(("WPSTextParser::readFDP: warning: FDP entry unintialized\n"));
		return false;
	}

	entry.setParsed();
	long page_offset = entry.begin();
	long length = entry.length();
	long endPage = entry.end();

	bool smallFDP = version() < 5;
	int deplSize = smallFDP ? 1 : 2;
	int headerSize = smallFDP ? 4 : 8;

	if (length < headerSize)
	{
		WPS_DEBUG_MSG(("WPSTextParser::readFDP: warning: FDP offset=0x%lx, length=0x%lx\n",
		               (unsigned long) page_offset, (unsigned long) length));
		return false;
	}

	libwps::DebugStream f, f2;
	if (smallFDP)
	{
		endPage--;
		input->seek(endPage, librevenge::RVNG_SEEK_SET);
	}
	else
		input->seek(page_offset, librevenge::RVNG_SEEK_SET);
	uint16_t cfod = deplSize == 1 ? (uint16_t) libwps::readU8(m_input) : libwps::readU16(m_input);

	f << "FDP: N="<<(int) cfod;
	if (smallFDP) input->seek(page_offset, librevenge::RVNG_SEEK_SET);
	else f << ", unk=" << libwps::read16(m_input);

	if (headerSize+(4+deplSize)*static_cast<long>(cfod) > length)
	{
		WPS_DEBUG_MSG(("WPSTextParser::readFDP: error: cfod = %i (0x%X)\n", cfod, (unsigned) cfod));
		return false;
	}

	int firstFod = int(fods.size());
	long lastLimit = firstFod ? fods.back().m_pos : 0;

	long lastReadPos = 0L;

	DataFOD::Type type = DataFOD::ATTR_UNKN;
	if (entry.hasType("FDPC")) type = DataFOD::ATTR_TEXT;
	else if (entry.hasType("FDPP")) type = DataFOD::ATTR_PARAG;
	else
	{
		WPS_DEBUG_MSG(("WPSTextParser::readFDP: FDP error: unknown type = '%s'\n", entry.type().c_str()));
	}

	/* Read array of fcLim of FODs.  The fcLim refers to the offset of the
	   last character covered by the formatting. */
	for (int i = 0; i <= cfod; ++i)
	{
		DataFOD fod;
		fod.m_type = type;
		fod.m_pos = (long) libwps::readU32(m_input);
		if (fod.m_pos == 0) fod.m_pos=m_textPositions.begin();

		/* check that fcLim is not too large */
		if (fod.m_pos > m_textPositions.end())
		{
			WPS_DEBUG_MSG(("WPSTextParser::readFDP: error: length of 'text selection' %ld > "
			               "total text length %ld\n", fod.m_pos, m_textPositions.end()));
			return false;
		}

		/* check that pos is monotonic */
		if (lastLimit > fod.m_pos)
		{
			WPS_DEBUG_MSG(("WPSTextParser::readFDP: error: character position list must "
			               "be monotonic, but found %ld, %ld\n", lastLimit, fod.m_pos));
			return false;
		}

		lastLimit = fod.m_pos;

		if (i != cfod)
			fods.push_back(fod);
		else // ignore the last text position
			lastReadPos = fod.m_pos;
	}

	std::vector<DataFOD>::iterator fods_iter;
	/* Read array of bfprop of FODs.  The bfprop is the offset where
	   the FPROP is located. */
	f << ", Tpos:defP=(";
	for (fods_iter = fods.begin() + firstFod; fods_iter!= fods.end(); ++fods_iter)
	{
		unsigned depl = deplSize == 1 ? libwps::readU8(m_input) : libwps::readU16(m_input);
		/* check size of bfprop  */
		if ((depl < unsigned(headerSize+(4+deplSize)*cfod) && depl > 0) ||
		        page_offset+long(depl)  > endPage)
		{
			WPS_DEBUG_MSG(("WPSTextParser::readFDP: error: pos of bfprop is bad "
			               "%u (0x%X)\n", depl, depl));
			return false;
		}

		if (depl)
			(*fods_iter).m_defPos = long(depl) + page_offset;
	}
	ascii().addPos(input->tell());

	std::map<long,int> mapPtr;
	bool smallSzInProp = smallFDP;
	for (fods_iter = fods.begin() + firstFod; fods_iter!= fods.end(); ++fods_iter)
	{
		long pos = (*fods_iter).m_defPos;
		f << std::hex << (*fods_iter).m_pos << std::dec << ":";
		if (pos == 0)
		{
			f << "_, ";
			continue;
		}

		std::map<long,int>::iterator it= mapPtr.find(pos);
		if (it != mapPtr.end())
		{
			(*fods_iter).m_id = mapPtr[pos];
			f << entry.type() << (*fods_iter).m_id << ", ";
			continue;
		}

		input->seek(pos, librevenge::RVNG_SEEK_SET);
		int szProp = smallSzInProp ? libwps::readU8(m_input) : libwps::readU16(m_input);
		if (smallSzInProp) szProp++;
		if (szProp == 0)
		{
			WPS_DEBUG_MSG(("WPSTextParser::readFDP: error: 0 == szProp at file offset 0x%lx\n", (unsigned long)(input->tell()-1)));
			return false;
		}
		long endPos = pos+szProp;
		if (endPos > endPage)
		{
			WPS_DEBUG_MSG(("WPSTextParser::readFDP: error: cch = %d, too large\n", szProp));
			return false;
		}

		int id;
		std::string mess;
		if (parser &&(this->*parser)(endPos, id, mess))
		{
			(*fods_iter).m_id = mapPtr[pos] = id;

			f2.str("");
			f2 << entry.type()  << id <<":" << mess;
			ascii().addPos(pos);
			ascii().addNote(f2.str().c_str());
			pos = input->tell();
		}
		f << entry.type() << (*fods_iter).m_id << ", ";
		if (pos != endPos)
		{
			f2.str("");
			f2 << entry.type() << "###";
			ascii().addPos(pos);
			ascii().addNote(f2.str().c_str());
		}
	}
	f << "), lstPos=" << std::hex << lastReadPos << std::dec << ", ";

	ascii().addPos(page_offset);
	ascii().addNote(f.str().c_str());

	/* go to end of page */
	input->seek(endPage, librevenge::RVNG_SEEK_SET);

	return m_textPositions.end() > lastReadPos;
}

std::vector<WPSTextParser::DataFOD> WPSTextParser::mergeSortedFODLists
(std::vector<WPSTextParser::DataFOD> const &lst1,
 std::vector<WPSTextParser::DataFOD> const &lst2) const
{
	std::vector<WPSTextParser::DataFOD> res;
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

		if (val.m_pos < m_textPositions.begin() || val.m_pos > m_textPositions.end())
			continue;

		res.push_back(val);
	}
	return res;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
