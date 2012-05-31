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

#include <iomanip>
#include <iostream>

#include <libwpd/WPXBinaryData.h>
#include <libwpd/WPXString.h>

#include "libwps_internal.h"
#include "WPSContentListener.h"
#include "WPSEntry.h"
#include "WPSPosition.h"

#include "WPS4.h"

#include "WPS4Graph.h"

/** Internal: the structures of a WPS4Graph */
namespace WPS4GraphInternal
{
//! Internal: the state of a WPS4Graph
struct State
{
	State() : m_version(-1), m_numPages(0), m_objects(), m_objectsPosition(), m_objectsId(), m_parsed() {}
	//! the version
	int m_version;
	//! the number page
	int m_numPages;

	//! the list of objects
	std::vector<WPXBinaryData> m_objects;
	//! the list of positions
	std::vector<WPSPosition> m_objectsPosition;
	//! the list of object's ids
	std::vector<int> m_objectsId;
	//! list of flags to know if the data are been sent to the listener
	std::vector<bool> m_parsed;
};
}

// constructor/destructor
WPS4Graph::WPS4Graph(WPS4Parser &parser):
	m_listener(), m_mainParser(parser), m_state(new WPS4GraphInternal::State),
	m_asciiFile(parser.m_asciiFile)
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
	int numObject = m_state->m_objects.size();
	m_state->m_numPages = numObject ? 1 : 0;
	m_state->m_parsed.resize(numObject, false);
}

// update the positions and send data to the listener
void WPS4Graph::storeObjects(std::vector<WPXBinaryData> const &objects,
                             std::vector<int> const &ids,
                             std::vector<WPSPosition> const &positions)
{
	int numObject = objects.size();
	if (numObject != int(ids.size()))
	{
		WPS_DEBUG_MSG(("WPS4Graph::storeObjects: unconsistent arguments\n"));
		return;
	}
	for (int i = 0; i < numObject; i++)
	{
		m_state->m_objects.push_back(objects[i]);
		m_state->m_objectsPosition.push_back(positions[i]);
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

	int numObject = m_state->m_objects.size();
	int pos = -1;
	for (int g = 0; g < numObject; g++)
	{
		if (m_state->m_objectsId[g] != id) continue;
		pos = g;
	}

	if (pos < 0)
	{
		WPS_DEBUG_MSG(("WPS4Graph::sendObject: can not find %d object\n", id));
		return;
	}

	m_state->m_parsed[pos] = true;
	WPSPosition posi(Vec2f(),sz);
	posi.setRelativePosition(WPSPosition::CharBaseLine);
	posi.m_wrapping = WPSPosition::WDynamic;
	float scale = 1.0/m_state->m_objectsPosition[pos].getInvUnitScale(WPX_INCH);
	posi.setNaturalSize(scale*m_state->m_objectsPosition[pos].naturalSize());
	m_listener->insertPicture(posi, m_state->m_objects[pos]);
}

void WPS4Graph::sendObjects(int page)
{
	if (page != -1) return;
	if (m_listener.get() == 0L)
	{
		WPS_DEBUG_MSG(("WPS4Graph::sendObjects: listener is not set\n"));
		return;
	}

	int numObject = m_state->m_objects.size();
#ifdef DEBUG
	bool firstSend = false;
#endif
	for (int g = 0; g < numObject; g++)
	{
		if (m_state->m_parsed[g]) continue;
#ifdef DEBUG
		if (!firstSend)
		{
			firstSend = true;
			WPS_DEBUG_MSG(("WPS4Graph::sendObjects: find some extra pictures\n"));
			m_listener->setFont(WPSFont::getDefault());
			m_listener->setParagraphJustification(libwps::JustificationLeft);
			m_listener->insertEOL();
			WPXString message = "--------- The original document has some extra pictures: -------- ";
			m_listener->insertUnicodeString(message);
			m_listener->insertEOL();
		}
#endif
		m_state->m_parsed[g] = true;
		// as we do not have the size of the data, we insert small picture
		WPSPosition pos(Vec2f(),Vec2f(1.,1.));
		pos.setRelativePosition(WPSPosition::CharBaseLine);
		pos.m_wrapping = WPSPosition::WDynamic;
		m_listener->insertPicture(pos, m_state->m_objects[g]);
	}
}

////////////////////////////////////////////////////////////
//  low level
////////////////////////////////////////////////////////////
int WPS4Graph::readObject(WPXInputStreamPtr input, WPSEntry const &entry)
{
	int resId = -1;
	if (!entry.valid() || entry.length() <= 4)
	{
		WPS_DEBUG_MSG(("WPS4Graph::readObject: invalid object\n"));
		return false;
	}
	long endPos = entry.end();
	input->seek(entry.begin(), WPX_SEEK_SET);

	long lastPos = entry.begin();

	libwps::DebugStream f;
	int numFind = 0;

	WPXBinaryData pict;
	WPSPosition pictPos;
	int actConfidence = -100;
	int oleId = -1;
	bool replace = false;

	while(1)
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

			for (int i = 0; i < int(m_state->m_objectsId.size()); i++)
			{
				if (m_state->m_objectsId[i] != oleId) continue;
				if (0 > actConfidence)
				{
					actConfidence = 0;
					pict = m_state->m_objects[i];
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
			for (int i = 0; i < 2; i++) dim[i] = libwps::read16(input)/1440.;
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
			int val = libwps::readU16(input);
			if (val) f << "#unkn=" << val << ",";
			int type_ = libwps::read32(input);
			f << "type=" << type_ << ",";
			long nSize = libwps::read32(input);
			if (nSize <= 0 || lastPos+22+nSize > endPos) break;

			std::string name;
			for (int i = 0; i < nSize; i++)
			{
				unsigned char c = libwps::readU8(input);
				if (c==0) break;
				name+= c;
			}
			f << "name='" << name << "',";
			for (int i = 0; i < 2; i++)
			{
				val = libwps::readU32(input);
				if (val) f << "f" << i << "=" << std::hex << val << ",";
			}
			long dSize = libwps::readU32(input);
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

		WPXBinaryData data;
		long actPos = input->tell();
		bool ok = readData && libwps::readData(input,endDataPos+1-actPos, data);
		if (confidence > actConfidence && data.size())
		{
			confidence = actConfidence;
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
				input->seek(endDataPos+1, WPX_SEEK_SET);
			else
				break;
		}

		ascii().skipZone(actPos, endDataPos);
#ifdef DEBUG_WITH_FILES
		std::stringstream f2;
		f2 << "Eobj" << entry.id() << "-" << numFind-1;
		libwps::Debug::dumpFile(data, f2.str().c_str());
#endif

		input->seek(endDataPos+1, WPX_SEEK_SET);
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
		for (int i = 0; i < int(m_state->m_objectsId.size()); i++)
		{
			if (m_state->m_objectsId[i] != oleId)
			{
				if (m_state->m_objectsId[i] > maxId) maxId = m_state->m_objectsId[i];
				continue;
			}
			m_state->m_objects[i] = pict;
			if (pictPos.naturalSize().x() > 0 && pictPos.naturalSize().y() > 0)
			{
				float scale = 1.0/pictPos.getInvUnitScale(m_state->m_objectsPosition[i].unit());
				m_state->m_objectsPosition[i].setNaturalSize(scale*pictPos.naturalSize());
			}
			found = true;
		}
		if (!found)
		{
			if (oleId < 0) oleId = maxId+1;
			m_state->m_objects.push_back(pict);
			m_state->m_objectsPosition.push_back(pictPos);
			m_state->m_objectsId.push_back(oleId);
		}
		resId = oleId;
	}
	else
		resId = oleId;

	return resId;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
