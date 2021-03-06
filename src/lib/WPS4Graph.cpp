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

#include <iomanip>
#include <iostream>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"
#include "WPSContentListener.h"
#include "WPSEntry.h"
#include "WPSFont.h"
#include "WPSOLEParser.h"
#include "WPSParagraph.h"
#include "WPSPosition.h"

#include "WPS4.h"

#include "WPS4Graph.h"

/** Internal: the structures of a WPS4Graph */
namespace WPS4GraphInternal
{
//! Internal: the state of a WPS4Graph
struct State
{
	State() : m_version(-1), m_numPages(0), m_objects(), m_objectsId(), m_parsed() {}
	//! the version
	int m_version;
	//! the number page
	int m_numPages;

	//! the list of objects
	std::vector<WPSOLEParserObject> m_objects;
	//! the list of object's ids
	std::vector<int> m_objectsId;
	//! list of flags to know if the data are been sent to the listener
	std::vector<bool> m_parsed;
};
}

// constructor/destructor
WPS4Graph::WPS4Graph(WPS4Parser &parser):
	m_listener(), m_mainParser(parser), m_state(new WPS4GraphInternal::State),
	m_asciiFile(parser.ascii())
{
}

WPS4Graph::~WPS4Graph()
{
}

// small functions: version/numpages/update position
int WPS4Graph::version() const
{
	if (m_state->m_version <= 0)
		m_state->m_version = m_mainParser.version();
	return m_state->m_version;
}
int WPS4Graph::numPages() const
{
	return m_state->m_numPages;
}

void WPS4Graph::computePositions() const
{
	size_t numObject = m_state->m_objects.size();
	m_state->m_numPages = numObject ? 1 : 0;
	m_state->m_parsed.resize(numObject, false);
}

// update the positions and send data to the listener
void WPS4Graph::storeObjects(std::vector<WPSOLEParserObject> const &objects,
                             std::vector<int> const &ids)
{
	size_t numObject = objects.size();
	if (numObject != ids.size())
	{
		WPS_DEBUG_MSG(("WPS4Graph::storeObjects: unconsistent arguments\n"));
		return;
	}
	for (size_t i = 0; i < numObject; i++)
	{
		m_state->m_objects.push_back(objects[i]);
		m_state->m_objectsId.push_back(ids[i]);
	}
}

// send object
void WPS4Graph::sendObject(Vec2f const &sz, int id)
{
	if (m_listener.get() == 0L)
	{
		WPS_DEBUG_MSG(("WPS4Graph::sendObject: listener is not set\n"));
		return;
	}

	size_t numObject = m_state->m_objects.size();
	int pos = -1;
	for (size_t g = 0; g < numObject; g++)
	{
		if (m_state->m_objectsId[g] != id) continue;
		pos = int(g);
	}

	if (pos < 0)
	{
		WPS_DEBUG_MSG(("WPS4Graph::sendObject: can not find %d object\n", id));
		return;
	}

	m_state->m_parsed[size_t(pos)] = true;
	WPSPosition posi(Vec2f(),sz);
	posi.setRelativePosition(WPSPosition::CharBaseLine);
	posi.m_wrapping = WPSPosition::WDynamic;
	float scale = float(1.0/m_state->m_objects[size_t(pos)].m_position.getInvUnitScale(librevenge::RVNG_INCH));
	posi.setNaturalSize(scale*m_state->m_objects[size_t(pos)].m_position.naturalSize());
	m_listener->insertPicture(posi, m_state->m_objects[size_t(pos)].m_data, m_state->m_objects[size_t(pos)].m_mime);
}

void WPS4Graph::sendObjects(int page)
{
	if (page != -1) return;
	if (m_listener.get() == 0L)
	{
		WPS_DEBUG_MSG(("WPS4Graph::sendObjects: listener is not set\n"));
		return;
	}

	size_t numObject = m_state->m_objects.size();
#ifdef DEBUG
	bool firstSend = false;
#endif
	for (size_t g = 0; g < numObject; g++)
	{
		if (m_state->m_parsed[g]) continue;
#ifdef DEBUG
		if (!firstSend)
		{
			firstSend = true;
			WPS_DEBUG_MSG(("WPS4Graph::sendObjects: find some extra pictures\n"));
			m_listener->setFont(WPSFont::getDefault());
			m_listener->setParagraph(WPSParagraph());
			m_listener->insertEOL();
			librevenge::RVNGString message = "--------- The original document has some extra pictures: -------- ";
			m_listener->insertUnicodeString(message);
			m_listener->insertEOL();
		}
#endif
		m_state->m_parsed[g] = true;
		// as we do not have the size of the data, we insert small picture
		WPSPosition pos(Vec2f(),Vec2f(1.,1.));
		pos.setRelativePosition(WPSPosition::CharBaseLine);
		pos.m_wrapping = WPSPosition::WDynamic;
		m_listener->insertPicture(pos, m_state->m_objects[g].m_data, m_state->m_objects[g].m_mime);
	}
}

////////////////////////////////////////////////////////////
//  low level
////////////////////////////////////////////////////////////
int WPS4Graph::readObject(RVNGInputStreamPtr input, WPSEntry const &entry)
{
	int resId = -1;
	if (!entry.valid() || entry.length() <= 4)
	{
		WPS_DEBUG_MSG(("WPS4Graph::readObject: invalid object\n"));
		return false;
	}
	long endPos = entry.end();
	input->seek(entry.begin(), librevenge::RVNG_SEEK_SET);

	libwps::DebugStream f;
	int numFind = 0;

	librevenge::RVNGBinaryData pict;
	WPSPosition pictPos;
	int actConfidence = -100;
	int oleId = -1;
	bool replace = false;

	long lastPos;
	while (1)
	{
		WPSPosition actPictPos;
		lastPos = input->tell();
		if (lastPos >= endPos) break;

		f.str("");
		f << "ZZEOBJ" << entry.id() << "-" << numFind++ << "(Contents):";
		int type = libwps::readU16(input);
		if (type == 0x4f4d)   // OM
		{
			if (lastPos+8 > endPos) break;

			oleId = libwps::read16(input);
			f << "Ole" << oleId << ",";
			int unkn = libwps::read16(input);
			if (unkn) f << "#unkn=" << unkn << ",";
			int val = libwps::read16(input);
			f << val << ",";
			if (lastPos+10 <= endPos) f << std::hex << libwps::read16(input);

			for (size_t i = 0; i < m_state->m_objectsId.size(); i++)
			{
				if (m_state->m_objectsId[i] != oleId) continue;
				if (0 > actConfidence)
				{
					actConfidence = 0;
					pict = m_state->m_objects[i].m_data;
					replace = false;
				}
			}
			ascii().addPos(lastPos);
			ascii().addNote(f.str().c_str());
			continue;
		}
		bool readData = false;
		long endDataPos = -1;
		int confidence = -1;
		if (type == 0x8)
		{
			if (lastPos+6 > endPos) break;
			// a simple metafile object ?
			float dim[2];
			for (int i = 0; i < 2; i++) dim[i] = float(libwps::read16(input)/1440.);
			// look like the final size : so we can use it as the picture size :-~
			f << "sz=(" << dim[0] << "," << dim[1] << ")," << libwps::read16(input);
			readData = true;
			endDataPos = endPos-1;
			confidence = 0;
		}
		else if (type == 0x501)
		{
			// find some metafile picture and MSDraw picture in v2 file
			if (lastPos+24 > endPos) break;
			// list of ole object, metafile, ...
			long val = libwps::readU16(input);
			if (val) f << "#unkn=" << val << ",";
			int type_ = libwps::read32(input);
			f << "type=" << type_ << ",";
			long nSize = libwps::read32(input);
			if (nSize <= 0 || lastPos+22+nSize > endPos) break;

			std::string name;
			for (int i = 0; i < nSize; i++)
			{
				char c = char(libwps::readU8(input));
				if (c==0) break;
				name+= c;
			}
			f << "name='" << name << "',";
			for (int i = 0; i < 2; i++)
			{
				val = (long) libwps::readU32(input);
				if (val) f << "f" << i << "=" << std::hex << val << ",";
			}
			long dSize = (long) libwps::readU32(input);
			long actPos = input->tell();

			bool  ok = dSize > 0 && dSize+actPos <= endPos;
			if (ok)
			{
				if (name == "METAFILEPICT" && dSize > 8)
				{
					f << "type=" << libwps::read16(input) << ",sz=(";
					for (int i = 0; i < 2; i++)
						f << libwps::read16(input)/1440. << ",";
					f << ")," << libwps::read16(input);
					actPos+=8;
					dSize-=8;
					confidence = 3;
				}
				readData = true;
				endDataPos = dSize+actPos-1;
			}

			if (!ok) f << "###";
			f << "dSize=" << std::hex << dSize << std::dec;
		}
		else
			break;

		librevenge::RVNGBinaryData data;
		long actPos = input->tell();
		bool ok = readData && libwps::readData(input,(unsigned long)(endDataPos+1-actPos), data);
		if (confidence > actConfidence && data.size())
		{
			actConfidence = confidence;
			pict = data;
			replace = true;
		}
		if (actPictPos.naturalSize().x() > 0 && actPictPos.naturalSize().y() > 0)
		{
			if (replace || pictPos.naturalSize().x() <= 0 || pictPos.naturalSize().y() <=0)
				pictPos = actPictPos;
		}
		ascii().addPos(lastPos);
		ascii().addNote(f.str().c_str());

		if (!ok)
		{
			if (endDataPos > 0 && endDataPos+1 <= endPos)
				input->seek(endDataPos+1, librevenge::RVNG_SEEK_SET);
			else
				break;
		}

		ascii().skipZone(actPos, endDataPos);
#ifdef DEBUG_WITH_FILES
		std::stringstream f2;
		f2 << "Eobj" << entry.id() << "-" << numFind-1;
		libwps::Debug::dumpFile(data, f2.str().c_str());
#endif

		input->seek(endDataPos+1, librevenge::RVNG_SEEK_SET);
	}

	if (lastPos != endPos)
	{
		ascii().addPos(lastPos);
		ascii().addNote("ZZEOBJ(Contents)###");
		ascii().addPos(endPos);
		ascii().addNote("_");
	}

	if (!pict.size())
		WPS_DEBUG_MSG(("WPS4Graph::readObject: Can not find picture for object: %d\n", oleId));
	else if (replace)
	{
		bool found = false;
		int maxId = -1;
		for (size_t i = 0; i < m_state->m_objectsId.size(); i++)
		{
			if (m_state->m_objectsId[i] != oleId)
			{
				if (m_state->m_objectsId[i] > maxId) maxId = m_state->m_objectsId[i];
				continue;
			}
			m_state->m_objects[i].m_data = pict;
			m_state->m_objects[i].m_mime = "image/pict";
			if (pictPos.naturalSize().x() > 0 && pictPos.naturalSize().y() > 0)
			{
				float scale = float(1.0/pictPos.getInvUnitScale(m_state->m_objects[i].m_position.unit()));
				m_state->m_objects[i].m_position.setNaturalSize(scale*pictPos.naturalSize());
			}
			found = true;
		}
		if (!found)
		{
			if (oleId < 0) oleId = maxId+1;
			WPSOLEParserObject object;
			object.m_data=pict;
			object.m_position=pictPos;
			m_state->m_objects.push_back(object);
			m_state->m_objectsId.push_back(oleId);
		}
		resId = oleId;
	}
	else
		resId = oleId;

	return resId;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
