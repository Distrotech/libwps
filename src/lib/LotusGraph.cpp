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
#include <limits>
#include <stack>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WKSContentListener.h"
#include "WKSSubDocument.h"

#include "WPSEntry.h"
#include "WPSFont.h"
#include "WPSGraphicShape.h"
#include "WPSGraphicStyle.h"
#include "WPSParagraph.h"
#include "WPSPosition.h"
#include "WPSStream.h"

#include "Lotus.h"
#include "LotusStyleManager.h"

#include "LotusGraph.h"

namespace LotusGraphInternal
{
//! the graphic zone of a LotusGraph
struct Zone
{
	//! the different type
	enum Type { Arc, Frame, Line, Rect, Unknown };
	//! constructor
	Zone(shared_ptr<WPSStream> stream) :
		m_type(Unknown), m_subType(0), m_stream(stream), m_box(), m_ordering(0),
		m_lineId(0), m_graphicId(0), m_surfaceId(0), m_hasShadow(false),
		m_pictureEntry(), m_textBoxEntry(), m_extra("")
	{
		for (int i=0; i<4; ++i) m_values[i]=0;
	}
	//! returns a graphic shape corresponding to the main form (and the origin)
	bool getGraphicShape(WPSGraphicShape &shape, WPSPosition &pos) const;
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, Zone const &z)
	{
		switch (z.m_type)
		{
		case Arc:
			o << "arc,";
			break;
		case Frame:
			// subType is a small number 2, 3 ?
			o << "frame[" << z.m_subType << "],";
			break;
		case Line:
			o << "line,";
			break;
		case Rect:
			if (z.m_subType==1)
				o << "rect,";
			else if (z.m_subType==2)
				o << "rectOval,";
			else if (z.m_subType==3)
				o << "oval,";
			else
				o << "rect[#" << z.m_subType << "],";
			break;
		case Unknown:
		default:
			break;
		}
		o << z.m_box << ",";
		o << "order=" << z.m_ordering << ",";
		if (z.m_lineId)
			o << "L" << z.m_lineId << ",";
		if (z.m_surfaceId)
			o << "Co" << z.m_surfaceId << ",";
		if (z.m_graphicId)
			o << "G" << z.m_graphicId << ",";
		if (z.m_hasShadow)
			o << "shadow,";
		for (int i=0; i<4; ++i)
		{
			if (z.m_values[i])
				o << "val" << i << "=" << z.m_values[i] << ",";
		}
		o << z.m_extra << ",";
		return o;
	}
	//! the zone type
	Type m_type;
	//! the file modifier type
	int m_subType;
	//! the stream
	shared_ptr<WPSStream> m_stream;
	//! the bdbox
	Box2i m_box;
	//! the ordering
	int m_ordering;
	//! the line style id
	int m_lineId;
	//! the graphic style id, used with rect shadow
	int m_graphicId;
	//! the surface color style id
	int m_surfaceId;
	//! a flag to know if we need to add shadow
	bool m_hasShadow;
	//! the picture entry
	WPSEntry m_pictureEntry;
	//! the text box entry
	WPSEntry m_textBoxEntry;
	//! unknown other value
	int m_values[4];
	//! extra data
	std::string m_extra;
};


bool Zone::getGraphicShape(WPSGraphicShape &shape, WPSPosition &pos) const
{
	pos=WPSPosition(m_box[0],m_box.size(), librevenge::RVNG_POINT);
	pos.setRelativePosition(WPSPosition::Page);
	Box2f box(Vec2f(0,0), m_box.size());
	switch (m_type)
	{
	case Line:
	{
		// we need to recompute the bdbox
		int bounds[4]= {m_box[0][0],m_box[0][1],m_box[1][0],m_box[1][1]};
		for (int i=0; i<2; ++i)
		{
			if (bounds[i]<=bounds[i+2]) continue;
			bounds[i]=bounds[i+2];
			bounds[i+2]=m_box[0][i];
		}
		Box2i realBox(Vec2i(bounds[0],bounds[1]),Vec2i(bounds[2],bounds[3]));
		pos=WPSPosition(realBox[0],realBox.size(), librevenge::RVNG_POINT);
		pos.setRelativePosition(WPSPosition::Page);
		shape=WPSGraphicShape::line(m_box[0]-realBox[0], m_box[1]-realBox[0]);
		return true;
	}
	case Rect:
		switch (m_subType)
		{
		case 2:
			shape=WPSGraphicShape::rectangle(box, Vec2f(5,5));
			return true;
		case 3:
			shape=WPSGraphicShape::circle(box);
			return true;
		default:
		case 1:
			shape=WPSGraphicShape::rectangle(box);
			return true;
		}
	case Frame:
		shape=WPSGraphicShape::rectangle(box);
		return true;
	case Arc:
		// changeme if the shape box if defined with different angle
		shape=WPSGraphicShape::arc(box, Box2f(Vec2f(-box[1][0],0),Vec2f(box[1][0],2*box[1][1])), Vec2f(0,90));
		return true;
	case Unknown:
	default:
		break;
	}
	return false;
}

//! the state of LotusGraph
struct State
{
	//! constructor
	State() :  m_version(-1), m_actualSheetId(-1), m_sheetIdZoneMap(), m_actualZone()
	{
	}

	//! the file version
	int m_version;
	//! the actual sheet id
	int m_actualSheetId;
	//! a map sheetid to zone
	std::multimap<int, shared_ptr<Zone> > m_sheetIdZoneMap;
	//! a pointer to the actual zone
	shared_ptr<Zone> m_actualZone;
};

//! Internal: the subdocument of a LotusGraphc
class SubDocument : public WKSSubDocument
{
public:
	//! constructor for a text entry
	SubDocument(shared_ptr<WPSStream> stream, LotusGraph &graphParser, WPSEntry &entry) :
		WKSSubDocument(RVNGInputStreamPtr(), &graphParser.m_mainParser), m_stream(stream), m_graphParser(graphParser), m_entry(entry) {}
	//! destructor
	~SubDocument() {}

	//! operator==
	virtual bool operator==(shared_ptr<WKSSubDocument> const &doc) const
	{
		if (!doc || !WKSSubDocument::operator==(doc))
			return false;
		SubDocument const *sDoc = dynamic_cast<SubDocument const *>(doc.get());
		if (!sDoc) return false;
		if (&m_graphParser != &sDoc->m_graphParser) return false;
		if (m_stream.get() != sDoc->m_stream.get()) return false;
		return m_entry == sDoc->m_entry;
	}

	//! the parser function
	void parse(shared_ptr<WKSContentListener> &listener, libwps::SubDocumentType subDocumentType);
	//! the stream
	shared_ptr<WPSStream> m_stream;
	//! the graph parser
	LotusGraph &m_graphParser;
	//! a flag to known if we need to send the entry or the footer
	WPSEntry m_entry;
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

	m_graphParser.sendTextBox(m_stream, m_entry);
}

}

// constructor, destructor
LotusGraph::LotusGraph(LotusParser &parser) :
	m_listener(), m_mainParser(parser), m_styleManager(parser.m_styleManager),
	m_state(new LotusGraphInternal::State)
{
}

LotusGraph::~LotusGraph()
{
}

void LotusGraph::cleanState()
{
	m_state.reset(new LotusGraphInternal::State);
}

int LotusGraph::version() const
{
	if (m_state->m_version<0)
		m_state->m_version=m_mainParser.version();
	return m_state->m_version;
}

bool LotusGraph::hasGraphics(int sheetId) const
{
	return m_state->m_sheetIdZoneMap.find(sheetId)!=m_state->m_sheetIdZoneMap.end();
}

////////////////////////////////////////////////////////////
// low level

////////////////////////////////////////////////////////////
// zones
////////////////////////////////////////////////////////////
bool LotusGraph::readZoneBegin(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	f << "Entries(GraphBegin):";
	long pos = input->tell();
	if (endPos-pos!=4)
	{
		WPS_DEBUG_MSG(("LotusParser::readZoneBegin: the zone seems bad\n"));
		f << "###";
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());

		return true;
	}
	m_state->m_actualSheetId=(int) libwps::readU8(input);
	f << "sheet[id]=" << m_state->m_actualSheetId << ",";
	for (int i=0; i<3; ++i)   // f0=1
	{
		int val=(int) libwps::readU8(input);
		if (val)
			f << "f" << i << "=" << val << ",";
	}
	m_state->m_actualZone.reset();
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;

}

bool LotusGraph::readZoneData(shared_ptr<WPSStream> stream, long endPos, int type)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	long sz=endPos-pos;

	shared_ptr<LotusGraphInternal::Zone> zone(new LotusGraphInternal::Zone(stream));
	m_state->m_actualZone=zone;

	switch (type)
	{
	case 0x2332:
		zone->m_type=LotusGraphInternal::Zone::Line;
		break;
	case 0x2346:
		zone->m_type=LotusGraphInternal::Zone::Rect;
		break;
	case 0x2350:
		zone->m_type=LotusGraphInternal::Zone::Arc;
		break;
	case 0x2352:
		zone->m_type=LotusGraphInternal::Zone::Frame;
		zone->m_hasShadow=true;
		break;
	case 0x23f0:
		zone->m_type=LotusGraphInternal::Zone::Frame;
		break;
	default:
		WPS_DEBUG_MSG(("LotusGraph::readZoneData: find unexpected graph data\n"));
		f << "###";
	}
	if (sz<24)
	{
		WPS_DEBUG_MSG(("LotusGraph::readZoneData: the zone seems too short\n"));
		f << "Entries(GraphMac):" << *zone << "###";
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	zone->m_ordering=(int) libwps::readU8(input);
	for (int i=0; i<4; ++i)   // always 0?
	{
		int val=(int) libwps::read8(input);
		if (val)
			f << "f" << i << "=" << val << ",";
	}
	int dim[4];
	for (int i=0; i<4; ++i)   // dim3[high]=0|100
	{
		dim[i]=(int) libwps::read16(input);
		if (i==3)
			break;
		int val=(int) libwps::read16(input);
		if (val) f << "dim" << i << "[high]=" << std::hex << val << std::dec << ",";
	}
	zone->m_box=Box2i(Vec2i(dim[1],dim[0]),Vec2i(dim[3],dim[2]));
	int val=(int) libwps::read8(input);
	if (val) // always 0
		f << "f4=" << val << ",";
	int fl;
	switch (zone->m_type)
	{
	case LotusGraphInternal::Zone::Line:
		val=(int) libwps::readU8(input);
		fl=(int) libwps::readU8(input);
		if (val)
		{
			if (fl!=0x10)
				f << "#line[fl]=" << std::hex << fl << std::dec << ",";
			zone->m_lineId=val;
		}
		val=(int) libwps::readU8(input); // always 1?
		if (val!=1)
			f << "g0=" << val << ",";
		// the arrows &1 means end, &2 means begin
		zone->m_values[0]=(int) libwps::readU8(input);
		if (sz<26)
		{
			WPS_DEBUG_MSG(("LotusGraph::readZoneData: the line zone seems too short\n"));
			f << "###sz,";
			break;
		}
		for (int i=0; i<2; ++i)   // always g1=0, g2=3 ?
		{
			val=(int) libwps::readU8(input);
			if (val!=3*i)
				f << "g" << i+1 << "=" << val << ",";
		}
		break;
	case LotusGraphInternal::Zone::Rect:
		val=(int) libwps::readU8(input); // always 1?
		if (val!=1)
			f << "g0=" << val << ",";
		zone->m_subType=(int) libwps::readU8(input);
		if (sz<28)
		{
			WPS_DEBUG_MSG(("LotusGraph::readZoneData: the rect zone seems too short\n"));
			f << "###sz,";
			break;
		}
		for (int i=0; i<2; ++i)
		{
			val=(int) libwps::readU8(input);
			fl=(int) libwps::readU8(input);
			if (!val) continue;
			if (i==0)
			{
				if (fl!=0x10)
					f << "#line[fl]=" << std::hex << fl << std::dec << ",";
				zone->m_lineId=val;
				continue;
			}
			if (fl!=0x20)
				f << "#surface[fl]=" << std::hex << fl << std::dec << ",";
			zone->m_surfaceId=val;
		}
		val=(int) libwps::read16(input); // always 3?
		if (val!=3)
			f << "g1=" << val << ",";
		break;
	case LotusGraphInternal::Zone::Frame:
		val=(int) libwps::readU8(input); // always 1?
		if (val!=1)
			f << "g0=" << val << ",";
		// small value 1-4
		zone->m_subType=(int) libwps::readU8(input);
		val=(int) libwps::readU8(input);
		fl=(int) libwps::readU8(input);
		if (!val) break;;
		if (fl!=0x40)
			f << "#graphic[fl]=" << std::hex << fl << std::dec << ",";
		zone->m_graphicId=val;
		// can be followed by 000000000100 : some way to determine the content ?
		break;
	case LotusGraphInternal::Zone::Arc:
		val=(int) libwps::readU8(input); // always 1?
		if (val!=1)
			f << "g0=" << val << ",";
		// always 3
		zone->m_subType=(int) libwps::readU8(input);
		val=(int) libwps::readU8(input);
		fl=(int) libwps::readU8(input);
		if (val)
		{
			if (fl!=0x10)
				f << "#line[fl]=" << std::hex << fl << std::dec << ",";
			zone->m_lineId=val;
		}
		if (sz<26)
		{
			WPS_DEBUG_MSG(("LotusGraph::readZoneData: the arc zone seems too short\n"));
			f << "###sz,";
			break;
		}
		val=(int) libwps::read16(input); // always 0? maybe the angle
		if (val)
			f << "g1=" << val << ",";
		break;
	case LotusGraphInternal::Zone::Unknown:
	default:
		break;
	}

	if (m_state->m_actualSheetId<0)
	{
		WPS_DEBUG_MSG(("LotusGraph::readZoneData: oops no sheet zone is opened\n"));
		f << "###sheetId,";
	}
	else
		m_state->m_sheetIdZoneMap.insert(std::multimap<int, shared_ptr<LotusGraphInternal::Zone> >::value_type
		                                 (m_state->m_actualSheetId, zone));
	zone->m_extra=f.str();
	f.str("");
	f << "Entries(GraphMac):" << *zone;
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusGraph::readTextBoxData(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	long sz=endPos-pos;
	f << "Entries(GraphTextBox):";
	if (sz<1)
	{
		WPS_DEBUG_MSG(("LotusGraph::readTextBoxData: Oops the zone seems too short\n"));
		f << "###";
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());
		return true;
	}

	if (!m_state->m_actualZone || m_state->m_actualZone->m_type != LotusGraphInternal::Zone::Frame)
	{
		WPS_DEBUG_MSG(("LotusGraph::readTextBoxData: Oops can not find the parent frame\n"));
	}
	else
	{
		m_state->m_actualZone->m_textBoxEntry.setBegin(input->tell());
		m_state->m_actualZone->m_textBoxEntry.setEnd(endPos);
		m_state->m_actualZone.reset();
	}

	m_state->m_actualZone.reset();
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	input->seek(endPos, librevenge::RVNG_SEEK_SET);
	return true;
}

bool LotusGraph::readPictureDefinition(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	long sz=endPos-pos;

	f << "Entries(PictDef):";
	if (sz!=13)
	{
		WPS_DEBUG_MSG(("LotusGraph::readPictureDefinition: the picture def seems bad\n"));
		f << "###";
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	if (!m_state->m_actualZone || m_state->m_actualZone->m_type != LotusGraphInternal::Zone::Frame)
	{
		WPS_DEBUG_MSG(("LotusGraph::readPictureDefinition: Oops can not find the parent frame\n"));
	}
	int val=(int) libwps::readU8(input); // always 0?
	if (val)
		f << "f0=" << val << ",";
	int dim[2];
	dim[0]=(int) libwps::readU16(input);
	for (int i=0; i<2; ++i)
	{
		val=(int) libwps::readU8(input);
		if (val)
			f << "f" << i+1 << "=" << val << ",";
	}
	dim[1]=(int) libwps::readU16(input);
	f << "dim=" << Vec2i(dim[0], dim[1]) << ",";
	val=(int) libwps::readU8(input);
	if (val)
		f << "f3=" << val << ",";
	int pictSz=(int) libwps::readU16(input); // maybe 32bits
	f << "pict[sz]=" << std::hex << pictSz << std::dec << ",";
	for (int i=0; i<3; ++i)   // always 0,0,1
	{
		val=(int) libwps::readU8(input);
		if (val)
			f << "g" << i << "=" << val << ",";
	}
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusGraph::readPictureData(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = input->tell();
	long sz=endPos-pos;

	f << "Entries(PictData):";
	if (sz<=1)
	{
		WPS_DEBUG_MSG(("LotusGraph::readPictureData: the picture def seems bad\n"));
		f << "###";
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	int val=(int) libwps::readU8(input); // always 1?
	if (val!=1)
		f << "type?=" << val << ",";
	if (!m_state->m_actualZone || m_state->m_actualZone->m_type != LotusGraphInternal::Zone::Frame)
	{
		WPS_DEBUG_MSG(("LotusGraph::readPictureData: Oops can not find the parent frame\n"));
	}
	else
	{
		m_state->m_actualZone->m_pictureEntry.setBegin(input->tell());
		m_state->m_actualZone->m_pictureEntry.setEnd(endPos);
		m_state->m_actualZone.reset();
	}
#ifdef DEBUG_WITH_FILES
	ascFile.skipZone(input->tell(), endPos-1);
	librevenge::RVNGBinaryData data;
	if (!libwps::readData(input, (unsigned long)(endPos-input->tell()), data))
		f << "###";
	else
	{
		std::stringstream s;
		static int fileId=0;
		s << "Pict" << ++fileId << ".pct";
		libwps::Debug::dumpFile(data, s.str().c_str());
	}
#endif

	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
// send data
////////////////////////////////////////////////////////////
void LotusGraph::sendPicture(LotusGraphInternal::Zone const &zone)
{
	if (!m_listener || !zone.m_stream || !zone.m_stream->m_input || !zone.m_pictureEntry.valid())
	{
		WPS_DEBUG_MSG(("LotusGraph::sendPicture: I can not find the listener/picture entry\n"));
		return;
	}
	RVNGInputStreamPtr input=zone.m_stream->m_input;
	librevenge::RVNGBinaryData data;
	input->seek(zone.m_pictureEntry.begin(), librevenge::RVNG_SEEK_SET);
	if (!libwps::readData(input, (unsigned long)(zone.m_pictureEntry.length()), data))
	{
		WPS_DEBUG_MSG(("LotusGraph::sendPicture: I can not find the picture\n"));
		return;
	}
	WPSGraphicShape shape;
	WPSPosition pos;
	if (!zone.getGraphicShape(shape, pos))
		return;
	WPSGraphicStyle style;
	if (zone.m_graphicId)
		m_styleManager->updateGraphicStyle(zone.m_graphicId, style);
	m_listener->insertPicture(pos, data, "image/pict", style);
}

void LotusGraph::sendTextBox(shared_ptr<WPSStream> stream, WPSEntry const &entry)
{
	if (!stream || !m_listener || entry.length()<1)
	{
		WPS_DEBUG_MSG(("LotusGraph::sendTextBox: I can not find the listener/textbox entry\n"));
		return;
	}
	RVNGInputStreamPtr input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;
	long pos = entry.begin();
	long sz=entry.length();
	f << "GraphTextBox[data]:";
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	int val=(int) libwps::readU8(input); // always 1?
	if (val!=1) f << "f0=" << val << ",";
	libwps_tools_win::Font::Type fontType = m_mainParser.getDefaultFontType();
	WPSFont font=WPSFont::getDefault();
	m_listener->setFont(font);
	bool actualFlags[7]= {false, false, false, false, false, false, false };
	for (long i=1; i<sz; ++i)
	{
		char c=(char) libwps::readU8(input);
		if (c==0)
		{
			if (i+2<sz)
			{
				WPS_DEBUG_MSG(("LotusGraph::sendTextBox: find a 0 char\n"));
				f << "[###0]";
			}
			continue;
		}
		if (c!=0xe && c!=0xf)
		{
			f << c;
			m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::unicode((unsigned char)c,fontType));
			continue;
		}
		if (i+1>=sz)
		{
			WPS_DEBUG_MSG(("LotusGraph::sendTextBox: find modifier in last pos\n"));
			f << "[###" << int(c) << "]";
		}
		int mod=(int) libwps::readU8(input);
		++i;
		if (c==0xf)
		{
			if (mod==45)
			{
				f << "[break]";
				m_listener->insertEOL();
			}
			else
			{
				WPS_DEBUG_MSG(("LotusGraph::sendTextBox: find unknown modifier f\n"));
				f << "[###f:" << mod << "]";
			}
			continue;
		}
		int szParam=(mod==0x80) ? 4 : (mod>=0x40 && mod<=0x44) ? 2 : 0;
		if (i+1+2*szParam>=sz)
		{
			WPS_DEBUG_MSG(("LotusGraph::sendTextBox: the param size seems bad\n"));
			f << "[##e:" << std::hex << mod << std::dec << "]";
			continue;
		}
		int param=0;
		long actPos=input->tell();
		bool ok=true;
		for (int d=0; d<szParam; ++d)
		{
			int mod1=(int) libwps::readU8(input);
			val=(int) libwps::readU8(input);
			static int const decal[]= {1,0,3,2};
			if (mod1==0xe && (val>='0'&&val<='9'))
			{
				param += (val-'0')<<(4*decal[d]);
				continue;
			}
			else if (mod1==0xe && (val>='A'&&val<='F'))
			{
				param += (val-'A'+10)<<(4*decal[d]);
				continue;
			}
			WPS_DEBUG_MSG(("LotusGraph::sendTextBox: something when bad when reading param\n"));
			f << "[##e:" << std::hex << mod << ":" << param << std::dec << "]";
			ok=false;
		}
		if (!ok)
		{
			input->seek(actPos, librevenge::RVNG_SEEK_SET);
			continue;
		}
		i+=2*szParam;
		switch (mod)
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		{
			bool newFlag=actualFlags[mod-1]=!actualFlags[mod-1];
			static char const *(wh[])= {"b", "it", "outline", "underline", "shadow", "condensed", "extended"};
			f << "[";
			if (!newFlag) f << "/";
			f << wh[mod-1] << "]";
			if (mod<=5)
			{
				static uint32_t const(attrib[])= { WPS_BOLD_BIT, WPS_ITALICS_BIT, WPS_UNDERLINE_BIT, WPS_OUTLINE_BIT, WPS_SHADOW_BIT };
				if (newFlag)
					font.m_attributes |= attrib[mod-1];
				else
					font.m_attributes &= ~attrib[mod-1];
			}
			else
			{
				font.m_spacing=0;
				if (actualFlags[5]) font.m_spacing-=2;
				if (actualFlags[6]) font.m_spacing+=2;
			}
			m_listener->setFont(font);
			break;
		}
		case 0x40:
		{
			WPSFont newFont;
			f << "[FN" << param<< "]";
			if (m_mainParser.getFont(param, newFont, fontType))
			{
				font.m_name=newFont.m_name;
				m_listener->setFont(font);
			}
			else
				f << "###";
			break;
		}
		case 0x41:
		{
			f << "[color=" << param << "]";
			WPSColor color;
			if (m_styleManager->getColor256(param, color))
			{
				font.m_color=color;
				m_listener->setFont(font);
			}
			else
				f << "###";
			break;
		}
		case 0x44:
		{
			WPSParagraph para;
			switch (param)
			{
			case 1:
				f << "align[left]";
				para.m_justify=libwps::JustificationLeft;
				break;
			case 2:
				f << "align[right]";
				para.m_justify=libwps::JustificationRight;
				break;
			case 3:
				f << "align[center]";
				para.m_justify=libwps::JustificationCenter;
				break;
			default:
				f << "#align=" << param << ",";
				break;
			};
			m_listener->setParagraph(para);
			break;
		}
		case 0x80:
		{
			f << "[fSz=" << param/32. << "]";
			font.m_size=param/32.;
			m_listener->setFont(font);
			break;
		}
		default:
			WPS_DEBUG_MSG(("LotusGraph::sendTextBox: Oops find unknown modifier e\n"));
			f << "[##e:" << std::hex << mod << "=" << param << std::dec << "]";
			break;
		}
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
}

void LotusGraph::sendGraphics(int sheetId)
{
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("LotusGraph::sendGraphics: I can not find the listener\n"));
		return;
	}
	std::multimap<int, shared_ptr<LotusGraphInternal::Zone> >::const_iterator it
	    =m_state->m_sheetIdZoneMap.lower_bound(sheetId);
	while (it!=m_state->m_sheetIdZoneMap.end() && it->first==sheetId)
	{
		shared_ptr<LotusGraphInternal::Zone> zone=(it++)->second;
		if (!zone) continue;
		if (zone->m_pictureEntry.valid())
		{
			sendPicture(*zone);
			continue;
		}
		WPSGraphicShape shape;
		WPSPosition pos;
		if (!zone->getGraphicShape(shape, pos))
			continue;
		WPSGraphicStyle style;
		if (zone->m_lineId)
			m_styleManager->updateLineStyle(zone->m_lineId, style);
		if (zone->m_surfaceId)
			m_styleManager->updateSurfaceStyle(zone->m_surfaceId, style);
		if (zone->m_graphicId)
			m_styleManager->updateGraphicStyle(zone->m_graphicId, style);
		if (zone->m_textBoxEntry.valid())
		{
			shared_ptr<LotusGraphInternal::SubDocument> doc
			(new LotusGraphInternal::SubDocument(zone->m_stream, *this, zone->m_textBoxEntry));
			m_listener->insertTextBox(pos, doc, style);
			continue;
		}
		if (zone->m_type==LotusGraphInternal::Zone::Line)
		{
			if (zone->m_values[0]&1)
				style.m_arrows[1]=true;
			if (zone->m_values[0]&2)
				style.m_arrows[0]=true;
		}
		m_listener->insertPicture(pos, shape, style);
	}
}

bool LotusGraph::readGraphic(shared_ptr<WPSStream> stream)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	long type = long(libwps::read16(input));
	if (type!=0xca)
	{
		WPS_DEBUG_MSG(("LotusGraph::readGraphic: not a sheet header\n"));
		return false;
	}
	long sz = long(libwps::readU16(input));
	f << "Entries(FMTGraphic):";
	if (sz < 0x23)
	{
		WPS_DEBUG_MSG(("LotusGraph::readGraphic: the zone size seems bad\n"));
		f << "###";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	int mType=int(libwps::readU8(input));
	if (mType==2) f << "graph,";
	else if (mType==4) f << "group,";
	else if (mType==0xa) f << "textbox,";
	else if (mType==0xb) f << "cell[border],";
	else f << "type[main]=" << mType << ",";
	int lType=int(libwps::readU8(input));
	if (lType==1) f << "line,";
	else if (lType==2) f << "poly,";
	else if (lType==4) f << "arc,";
	else if (lType==5) f << "spline,";
	else if (lType==6) f << "rect,";
	else if (lType==7) f << "rect[round],";
	else if (lType==8) f << "oval,";
	else if (lType==0xa) f << "group,";
	else if (lType==0xd) f << "button,"; // checkme
	else if (lType==0xe) f << "textbox,";
	else if (lType==0x10) f << "cell[bord],";
	else f << "type[local]=" << lType << ",";
	int val=int(libwps::readU8(input)); // 0|80
	if (val) f << "fl0=" << std::hex << val << std::dec << ",";
	int id=int(libwps::readU16(input));
	f << "id=" << id << ",";

	f << "line=[";
	val=int(libwps::readU8(input));
	WPSColor color;
	if (!m_styleManager->getColor256(val, color))
	{
		WPS_DEBUG_MSG(("LotusGraph::readGraphic: can not read a color\n"));
		f << "###colId=" << val << ",";
	}
	else if (!color.isBlack()) f << color << ",";
	val=int(libwps::readU8(input));
	if (val!=1 && val<8)
	{
		char const *(wh[])= {"none", "normal",  "dash7x3", "dot4x4",
		                     "dash6x2:4x2", "dash4x2:2x2:2x2", "dot2x2", "dot1x1"
		                    };
		f << wh[val] << ",";
	}
	else if (val!=1)
	{
		WPS_DEBUG_MSG(("LotusGraph::readGraphic: can not read the line's style\n"));
		f << "###style=" << val << ",";
	}
	val=int(libwps::readU8(input));
	if (val && val<8) f << "w=" << val+1 << ",";
	else if (val)
	{
		WPS_DEBUG_MSG(("LotusGraph::readGraphic: can not read the line's width\n"));
		f << "###width=" << val << ",";
	}
	f << "],";
	f << "surf=[";
	for (int i=0; i<2; ++i)
	{
		val=int(libwps::readU8(input));
		if (!m_styleManager->getColor256(val, color))
		{
			WPS_DEBUG_MSG(("LotusGraph::readGraphic: can not read a color\n"));
			f << "###colId" << i << "=" << val << ",";
		}
		else if ((i==0 && !color.isBlack()) || (i==1 && !color.isWhite()))
			f << "col" << i << "=" << color << ",";
	}
	val=int(libwps::readU8(input)); // 0=none
	if (val) f << "patId=" << val << ",";
	f << "],";
	f << "shadow=["; // border design
	val=int(libwps::readU8(input));
	if (!m_styleManager->getColor256(val, color))
	{
		WPS_DEBUG_MSG(("LotusGraph::readGraphic: can not read a color\n"));
		f << "###colId=" << val << ",";
	}
	else if (!color.isBlack()) f << color << ",";
	val=int(libwps::readU8(input)); // 0=none
	if (val) f << "type=" << val << ",";
	f << "],";
	f << "mat=["; // id=1,0,0 1,0,0 : mat 2x2 + translation
	for (int i=0; i<6; ++i) f << float(libwps::read32(input))/65536.f << ",";
	f << "],";
	unsigned long lVal=libwps::readU32(input); // unsure maybe related to some angle
	if (lVal) f << "unkn=" << std::hex << lVal << std::dec << ",";
	for (int i=0; i<2; ++i)   // f0=0|1|2, f1=0|5|6
	{
		val=int(libwps::readU8(input));
		if (val) f << "f" << i << "=" << val << ",";
	}
	switch (lType)
	{
	case 1:
		if (sz!=0x37)
			break;
		val=int(libwps::readU16(input));
		if (val!=2) f << "g0=" << val << ",";
		val=int(libwps::readU16(input));
		if (val&1) f << "arrow[beg],";
		if (val&2) f << "arrow[end],";
		val&=0xFFFC;
		if (val) f << "g1=" << std::hex << val << std::dec << ",";
		f << "pts=";
		for (int i=0; i<4; ++i)
		{
			f << int(libwps::readU16(input));
			if (i==1) f << "<->";
			else if (i==3) f << ",";
			else f << "x";
		}
		break;
	case 4:
		if (sz!=0x3b)
			break;
		val=int(libwps::readU16(input));
		if (val!=3) f << "g0=" << val << ",";
		val=int(libwps::readU16(input)); // 0208
		if (val) f << "g1=" << std::hex << val << std::dec << ",";
		f << "pts=[";
		for (int i=0; i<3; ++i)
			f << int(libwps::readU16(input)) << "x" << int(libwps::readU16(input)) << ",";
		f << "],";
		break;
	case 2:
	case 5:
	{
		int N=int(libwps::readU16(input));
		if (sz!=4*N+0x2f) break;
		val=int(libwps::readU16(input)); // 0
		if (val) f << "g0=" << val << ",";
		f << "pts=[";
		for (int i=0; i<N; ++i)
			f << int(libwps::readU16(input)) << "x" << int(libwps::readU16(input)) << ",";
		f << "],";
		break;
	}
	case 6:
	case 7:
	case 8:
		if (sz!=0x3f)
			break;
		val=int(libwps::readU16(input));
		if (val!=4) f << "g0=" << val << ",";
		for (int i=0; i<2; ++i)   // g1=4, g2=0|1
		{
			val=int(libwps::readU8(input));
			if (i==1)
			{
				if (val&1)
					f << "round,";
				else if (val&2)
					f << "oval,";
				val &= 0xFC;
			}
			if (val)
				f << "g" << i+1 << "=" << val << ",";
		}
		f << "pts=[";
		for (int i=0; i<8; ++i)
			f << int(libwps::readU16(input)) << ((i%2) ? "," : "x");
		f << "],";
		break;
	case 10:
		if (sz!=0x35)
			break;
		f << "pts=[";
		for (int i=0; i<2; ++i)
			f << int(libwps::readU16(input)) << "x" << int(libwps::readU16(input)) << ",";
		f << "],";
		val=int(libwps::readU16(input));
		if (val!=1) f << "g0=" << val << ",";
		break;
	case 0xd:
	case 0xe:
		if (sz!=0x35) break;
		f << "pts=";
		for (int i=0; i<4; ++i)
		{
			f << int(libwps::readU16(input));
			if (i==1) f << "<->";
			else if (i==3) f << ",";
			else f << "x";
		}
		val=int(libwps::readU16(input)); // small number
		f << "g0=" << val << ",";
		break;
	case 0x10:
		if (sz!=0x34) break;
		for (int i=0; i<2; ++i)   // 0
		{
			val=int(libwps::readU16(input));
			if (val) f << "g" << i << "=" << val << ",";
		}
		for (int i=0; i<3; ++i)   // g2=[0-f][0|8], g3=0-2 g4=??
		{
			val=int(libwps::readU8(input));
			if (val) f << "g" << i+2 << "=" << std::hex << val << std::dec << ",";
		}
		val=int(libwps::readU16(input)); // 0-2
		if (val) f << "g5=" << val << ",";
		break;
	default:
		break;
	}
	if (input->tell() != pos+sz)
		ascFile.addDelimiter(input->tell(), '|');
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusGraph::readFrame(shared_ptr<WPSStream> stream)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	long type = long(libwps::read16(input));
	if (type!=0xcc)
	{
		WPS_DEBUG_MSG(("LotusGraph::readFrame: not a sheet header\n"));
		return false;
	}
	long sz = long(libwps::readU16(input));
	f << "Entries(FMTFrame):";
	if (sz != 0x13)
	{
		WPS_DEBUG_MSG(("LotusGraph::readFrame: the zone size seems bad\n"));
		f << "###";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	int row=int(libwps::readU16(input));
	f << "row=" << row << ",";
	int col=int(libwps::readU8(input));
	f << "col=" << col << ",";
	// X X X:flsymetry?
	ascFile.addDelimiter(input->tell(), '|');
	input->seek(9, librevenge::RVNG_SEEK_CUR);
	ascFile.addDelimiter(input->tell(), '|');
	int val=int(libwps::readU16(input)); // 0-c
	if (val) f << "f0=" << val << ",";
	int dim[2];
	for (int i=0; i<2; ++i) dim[i]=int(libwps::readU16(input));
	f << "dim=" << Vec2i(dim[0], dim[1]) << ",";
	val=int(libwps::readU8(input)); // 1|2
	if (val!=1) f << "fl0=" << val << ",";
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusGraph::readPicture(shared_ptr<WPSStream> stream)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	long type = long(libwps::read16(input));
	if (type==0 || (type&0xF0FF) || libwps::read16(input))
	{
		WPS_DEBUG_MSG(("LotusGraph::readPicture: not a picture header\n"));
		return false;
	}
	f << "Entries(Picture)[" << (type>>8) << "]:";
	int val=int(libwps::readU8(input)); // 0
	if (val) f << "f0=" << val << ",";
	int sSz=int(libwps::readU16(input));
	if (!sSz || !stream->checkFilePosition(pos+21+sSz+3))
	{
		WPS_DEBUG_MSG(("LotusGraph::readPicture: name size seems bad\n"));
		return false;
	}
	val=int(libwps::readU16(input)); // 0
	if (val) f << "f1=" << val << ",";
	std::string name;
	for (int i=0; i<sSz; ++i) name+=char(libwps::readU8(input));
	f << name << ",";
	for (int i=0; i<4; ++i)   // always 0?
	{
		val=int(libwps::readU16(input));
		if (val) f << "g" << i << "=" << val << ",";
	}
	unsigned long dSz=libwps::readU32(input);
	if (dSz>0x40000000 || dSz<10 || !stream->checkFilePosition(pos+21+long(dSz)+3))
	{
		WPS_DEBUG_MSG(("LotusGraph::readPicture: pict size seems bad\n"));
		return false;
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());

	pos=input->tell();
	ascFile.addPos(pos);
	ascFile.addNote("_");

	input->seek(pos+long(dSz), librevenge::RVNG_SEEK_SET);
	pos=input->tell();
	ascFile.addPos(pos);
	ascFile.addNote("Picture-A:");
	// 010500=means continue, a picture follow
	// 000000=end, 00XX00 means skip to ?
	input->seek(pos+3, librevenge::RVNG_SEEK_SET);

	return true;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
