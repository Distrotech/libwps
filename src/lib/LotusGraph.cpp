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
	Zone() : m_type(Unknown), m_subType(0), m_box(), m_ordering(0),
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
	State() :  m_eof(-1), m_version(-1), m_actualSheetId(-1), m_sheetIdZoneMap(), m_actualZone()
	{
	}
	//! returns a color corresponding to an id
	bool getColor(int id, WPSColor &color) const;
	//! returns the pattern percent corresponding to a pattern id
	bool getPatternPercent(int id, float &percent) const;

	//! the last file position
	long m_eof;
	//! the file version
	int m_version;
	//! the actual sheet id
	int m_actualSheetId;
	//! a map sheetid to zone
	std::multimap<int, shared_ptr<Zone> > m_sheetIdZoneMap;
	//! a pointer to the actual zone
	shared_ptr<Zone> m_actualZone;
};

bool State::getColor(int id, WPSColor &color) const
{
	if (id<0||id>=256)
	{
		WPS_DEBUG_MSG(("LotusGraphInteranl::State::getColor(): unknown color id: %d\n",id));
		return false;
	}
	static uint32_t colorMap[]=
	{
		0xffffff, 0xffcc99, 0xffffcc, 0xccff99, 0x99ff33, 0x99ff66, 0x99ff99, 0xccffcc,
		0xccffff, 0x99ccff, 0x6699ff, 0xccccff, 0xcc99ff, 0xffccff, 0xff99cc, 0xffffff,
		0xffcccc, 0xffcc66, 0xffff99, 0xccff66, 0x99ff00, 0x66ff33, 0x66ff99, 0x99ffcc,
		0x99ffff, 0x3399ff, 0x6666ff, 0x9999ff, 0xcc66ff, 0xff99ff, 0xff66cc, 0xeeeeee,
		0xff9999, 0xff9966, 0xffff66, 0xccff33, 0x66ff00, 0x66ff66, 0x33ff99, 0x66ffcc,
		0x66ffff, 0x0099ff, 0x3366ff, 0x9966ff, 0xcc66cc, 0xff66ff, 0xff6699, 0xdddddd,
		0xff6666, 0xff9933, 0xffff33, 0xccff00, 0x33ff00, 0x33ff66, 0x00ff99, 0x33ffcc,
		0x33ffff, 0x0066ff, 0x0066cc, 0x9966cc, 0xcc33ff, 0xff33ff, 0xff3399, 0xcccccc,
		0xff3333, 0xff6633, 0xffff00, 0xcccc33, 0x00ff00, 0x00ff66, 0x66cc99, 0x00ffcc,
		0x00ffff, 0x0033ff, 0x3366cc, 0x9933ff, 0xcc00ff, 0xff33cc, 0xff3366, 0xbbbbbb,
		0xff0000, 0xff6600, 0xffcc33, 0xcccc00, 0x00ee00, 0x33ff33, 0x33cc99, 0x66cccc,
		0x66ccff, 0x0000ee, 0x3333ff, 0x9900ff, 0xcc00cc, 0xff00cc, 0xff0066, 0xaaaaaa,
		0xcc0000, 0xff3300, 0xffcc00, 0x99cc33, 0x00dd00, 0x00ff33, 0x00cc99, 0x33cccc,
		0x33ccff, 0x0000dd, 0x3300ff, 0x6666cc, 0x9933cc, 0xcc33cc, 0xff0033, 0x999999,
		0xbb0000, 0xee0000, 0xff9900, 0x99cc00, 0x00bb00, 0x33cc00, 0x33cc66, 0x00cccc,
		0x00ccff, 0x0000bb, 0x0000ff, 0x6633ff, 0x993399, 0xcc3399, 0xcc0033, 0x888888,
		0xaa0000, 0xdd0000, 0xcc9933, 0x999933, 0x00aa00, 0x33cc33, 0x00cc66, 0x009999,
		0x0099cc, 0x0000aa, 0x0033cc, 0x6633cc, 0x9900cc, 0xcc0099, 0xcc0066, 0x777777,
		0x990000, 0xcc3333, 0xcc9900, 0x999900, 0x008800, 0x00cc00, 0x339966, 0x339999,
		0x3399cc, 0x000088, 0x0000cc, 0x6600ff, 0x663399, 0x993366, 0xcc3366, 0x666666,
		0x660000, 0xcc3300, 0xcc6633, 0x669900, 0x007700, 0x339933, 0x009966, 0x336666,
		0x336699, 0x000077, 0x3300cc, 0x3333cc, 0x663366, 0x990066, 0x990033, 0x555555,
		0x550000, 0x993300, 0xcc6600, 0x669933, 0x005500, 0x339900, 0x336633, 0x006666,
		0x006699, 0x000055, 0x000099, 0x333399, 0x6600cc, 0x990099, 0x880000, 0x444444,
		0x330000, 0x663300, 0x996633, 0x336600, 0x004400, 0x009900, 0x006633, 0x333333,
		0x003399, 0x000044, 0x000066, 0x330099, 0x660099, 0x660066, 0x770000, 0x333333,
		0x220000, 0x440000, 0x996600, 0x333300, 0x002200, 0x006600, 0x003300, 0x003333,
		0x003366, 0x000022, 0x000033, 0x330066, 0x330033, 0x660033, 0x440000, 0x222222,
		0xcc9966, 0xcc6666, 0xcccc99, 0xcccc66, 0x99cc66, 0x66cc66, 0x99cc99, 0x99ffcc,
		0x99cccc, 0x999999, 0x6699cc, 0x9999cc, 0xcc99cc, 0xcc9999, 0xcc6699, 0x111111,
		0x996666, 0x993333, 0x999966, 0x666633, 0x66cc33, 0x009933, 0x669966, 0x66cc99,
		0x669999, 0x666666, 0x666699, 0x333366, 0x996699, 0x663333, 0x663366, 0x000000
	};
	color=WPSColor(colorMap[id]);
	return true;
}

bool State::getPatternPercent(int id, float &percent) const
{
	if (id<=0 || id>=49)
	{
		WPS_DEBUG_MSG(("LotusGraphInteranl::State::getPatternPercent(): unknown pattern id: %d\n",id));
		return false;
	}
	static float const(percentValues[])=
	{
		1.000000, 0.000000, 0.250000, 0.375000, 0.250000, 0.125000, 0.250000, 0.750000,
		0.500000, 0.250000, 0.250000, 0.375000, 0.250000, 0.125000, 0.500000, 0.250000,
		0.375000, 0.218750, 0.468750, 0.250000, 0.437500, 0.234375, 0.125000, 0.515625,
		0.875000, 0.750000, 0.500000, 0.250000, 0.125000, 0.125000, 0.062500, 0.093750,
		0.093750, 0.015625, 0.375000, 0.343750, 0.203125, 0.234375, 0.250000, 0.203125,
		0.281250, 0.312500, 0.593750, 0.250000, 0.500000, 0.500000,
		0.218750/* or gradient0*/, 0.218750 /*or gradient 1*/
	};
	percent=percentValues[id-1];
	return true;
}

//! Internal: the subdocument of a LotusGraphc
class SubDocument : public WKSSubDocument
{
public:
	//! constructor for a text entry
	SubDocument(RVNGInputStreamPtr input, LotusGraph &graphParser, WPSEntry &entry) :
		WKSSubDocument(input, &graphParser.m_mainParser), m_graphParser(graphParser), m_entry(entry) {}
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
		return m_entry == sDoc->m_entry;
	}

	//! the parser function
	void parse(shared_ptr<WKSContentListener> &listener, libwps::SubDocumentType subDocumentType);
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

	m_graphParser.sendTextBox(m_entry);
}

}

// constructor, destructor
LotusGraph::LotusGraph(LotusParser &parser) :
	m_input(parser.getInput()), m_listener(), m_mainParser(parser), m_styleManager(parser.m_styleManager),
	m_state(new LotusGraphInternal::State),	m_asciiFile(parser.ascii())
{
	m_state.reset(new LotusGraphInternal::State);
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

bool LotusGraph::checkFilePosition(long pos)
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


////////////////////////////////////////////////////////////
// low level

////////////////////////////////////////////////////////////
// zones
////////////////////////////////////////////////////////////
bool LotusGraph::readZoneBegin(long endPos)
{
	libwps::DebugStream f;
	f << "Entries(GraphBegin):";
	long pos = m_input->tell();
	if (endPos-pos!=4)
	{
		WPS_DEBUG_MSG(("LotusParser::readZoneBegin: the zone seems bad\n"));
		f << "###";
		ascii().addPos(pos-6);
		ascii().addNote(f.str().c_str());

		return true;
	}
	m_state->m_actualSheetId=(int) libwps::readU8(m_input);
	f << "sheet[id]=" << m_state->m_actualSheetId << ",";
	for (int i=0; i<3; ++i)   // f0=1
	{
		int val=(int) libwps::readU8(m_input);
		if (val)
			f << "f" << i << "=" << val << ",";
	}
	m_state->m_actualZone.reset();
	ascii().addPos(pos-6);
	ascii().addNote(f.str().c_str());
	return true;

}

bool LotusGraph::readZoneData(long endPos, int type)
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long sz=endPos-pos;

	shared_ptr<LotusGraphInternal::Zone> zone(new LotusGraphInternal::Zone);
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
		ascii().addPos(pos-6);
		ascii().addNote(f.str().c_str());
		return true;
	}
	zone->m_ordering=(int) libwps::readU8(m_input);
	for (int i=0; i<4; ++i)   // always 0?
	{
		int val=(int) libwps::read8(m_input);
		if (val)
			f << "f" << i << "=" << val << ",";
	}
	int dim[4];
	for (int i=0; i<4; ++i)   // dim3[high]=0|100
	{
		dim[i]=(int) libwps::read16(m_input);
		if (i==3)
			break;
		int val=(int) libwps::read16(m_input);
		if (val) f << "dim" << i << "[high]=" << std::hex << val << std::dec << ",";
	}
	zone->m_box=Box2i(Vec2i(dim[1],dim[0]),Vec2i(dim[3],dim[2]));
	int val=(int) libwps::read8(m_input);
	if (val) // always 0
		f << "f4=" << val << ",";
	int fl;
	switch (zone->m_type)
	{
	case LotusGraphInternal::Zone::Line:
		val=(int) libwps::readU8(m_input);
		fl=(int) libwps::readU8(m_input);
		if (val)
		{
			if (fl!=0x10)
				f << "#line[fl]=" << std::hex << fl << std::dec << ",";
			zone->m_lineId=val;
		}
		val=(int) libwps::readU8(m_input); // always 1?
		if (val!=1)
			f << "g0=" << val << ",";
		// the arrows &1 means end, &2 means begin
		zone->m_values[0]=(int) libwps::readU8(m_input);
		if (sz<26)
		{
			WPS_DEBUG_MSG(("LotusGraph::readZoneData: the line zone seems too short\n"));
			f << "###sz,";
			break;
		}
		for (int i=0; i<2; ++i)   // always g1=0, g2=3 ?
		{
			val=(int) libwps::readU8(m_input);
			if (val!=3*i)
				f << "g" << i+1 << "=" << val << ",";
		}
		break;
	case LotusGraphInternal::Zone::Rect:
		val=(int) libwps::readU8(m_input); // always 1?
		if (val!=1)
			f << "g0=" << val << ",";
		zone->m_subType=(int) libwps::readU8(m_input);
		if (sz<28)
		{
			WPS_DEBUG_MSG(("LotusGraph::readZoneData: the rect zone seems too short\n"));
			f << "###sz,";
			break;
		}
		for (int i=0; i<2; ++i)
		{
			val=(int) libwps::readU8(m_input);
			fl=(int) libwps::readU8(m_input);
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
		val=(int) libwps::read16(m_input); // always 3?
		if (val!=3)
			f << "g1=" << val << ",";
		break;
	case LotusGraphInternal::Zone::Frame:
		val=(int) libwps::readU8(m_input); // always 1?
		if (val!=1)
			f << "g0=" << val << ",";
		// small value 1-4
		zone->m_subType=(int) libwps::readU8(m_input);
		val=(int) libwps::readU8(m_input);
		fl=(int) libwps::readU8(m_input);
		if (!val) break;;
		if (fl!=0x40)
			f << "#graphic[fl]=" << std::hex << fl << std::dec << ",";
		zone->m_graphicId=val;
		// can be followed by 000000000100 : some way to determine the content ?
		break;
	case LotusGraphInternal::Zone::Arc:
		val=(int) libwps::readU8(m_input); // always 1?
		if (val!=1)
			f << "g0=" << val << ",";
		// always 3
		zone->m_subType=(int) libwps::readU8(m_input);
		val=(int) libwps::readU8(m_input);
		fl=(int) libwps::readU8(m_input);
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
		val=(int) libwps::read16(m_input); // always 0? maybe the angle
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
	ascii().addPos(pos-6);
	ascii().addNote(f.str().c_str());
	return true;
}

bool LotusGraph::readTextBoxData(long endPos)
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long sz=endPos-pos;
	f << "Entries(GraphTextBox):";
	if (sz<1)
	{
		WPS_DEBUG_MSG(("LotusGraph::readTextBoxData: Oops the zone seems too short\n"));
		f << "###";
		ascii().addPos(pos-6);
		ascii().addNote(f.str().c_str());
		return true;
	}

	if (!m_state->m_actualZone || m_state->m_actualZone->m_type != LotusGraphInternal::Zone::Frame)
	{
		WPS_DEBUG_MSG(("LotusGraph::readTextBoxData: Oops can not find the parent frame\n"));
	}
	else
	{
		m_state->m_actualZone->m_textBoxEntry.setBegin(m_input->tell());
		m_state->m_actualZone->m_textBoxEntry.setEnd(endPos);
		m_state->m_actualZone.reset();
	}

	m_state->m_actualZone.reset();
	ascii().addPos(pos-6);
	ascii().addNote(f.str().c_str());
	m_input->seek(endPos, librevenge::RVNG_SEEK_SET);
	return true;
}

bool LotusGraph::readPictureDefinition(long endPos)
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long sz=endPos-pos;

	f << "Entries(PictDef):";
	if (sz!=13)
	{
		WPS_DEBUG_MSG(("LotusGraph::readPictureDefinition: the picture def seems bad\n"));
		f << "###";
		ascii().addPos(pos-6);
		ascii().addNote(f.str().c_str());
		return true;
	}
	if (!m_state->m_actualZone || m_state->m_actualZone->m_type != LotusGraphInternal::Zone::Frame)
	{
		WPS_DEBUG_MSG(("LotusGraph::readPictureDefinition: Oops can not find the parent frame\n"));
	}
	int val=(int) libwps::readU8(m_input); // always 0?
	if (val)
		f << "f0=" << val << ",";
	int dim[2];
	dim[0]=(int) libwps::readU16(m_input);
	for (int i=0; i<2; ++i)
	{
		val=(int) libwps::readU8(m_input);
		if (val)
			f << "f" << i+1 << "=" << val << ",";
	}
	dim[1]=(int) libwps::readU16(m_input);
	f << "dim=" << Vec2i(dim[0], dim[1]) << ",";
	val=(int) libwps::readU8(m_input);
	if (val)
		f << "f3=" << val << ",";
	int pictSz=(int) libwps::readU16(m_input); // maybe 32bits
	f << "pict[sz]=" << std::hex << pictSz << std::dec << ",";
	for (int i=0; i<3; ++i)   // always 0,0,1
	{
		val=(int) libwps::readU8(m_input);
		if (val)
			f << "g" << i << "=" << val << ",";
	}
	ascii().addPos(pos-6);
	ascii().addNote(f.str().c_str());
	return true;
}

bool LotusGraph::readPictureData(long endPos)
{
	libwps::DebugStream f;
	long pos = m_input->tell();
	long sz=endPos-pos;

	f << "Entries(PictData):";
	if (sz<=1)
	{
		WPS_DEBUG_MSG(("LotusGraph::readPictureData: the picture def seems bad\n"));
		f << "###";
		ascii().addPos(pos-6);
		ascii().addNote(f.str().c_str());
		return true;
	}
	int val=(int) libwps::readU8(m_input); // always 1?
	if (val!=1)
		f << "type?=" << val << ",";
	if (!m_state->m_actualZone || m_state->m_actualZone->m_type != LotusGraphInternal::Zone::Frame)
	{
		WPS_DEBUG_MSG(("LotusGraph::readPictureData: Oops can not find the parent frame\n"));
	}
	else
	{
		m_state->m_actualZone->m_pictureEntry.setBegin(m_input->tell());
		m_state->m_actualZone->m_pictureEntry.setEnd(endPos);
		m_state->m_actualZone.reset();
	}
#ifdef DEBUG_WITH_FILES
	ascii().skipZone(m_input->tell(), endPos-1);
	librevenge::RVNGBinaryData data;
	if (!libwps::readData(m_input, (unsigned long)(endPos-m_input->tell()), data))
		f << "###";
	else
	{
		std::stringstream s;
		static int fileId=0;
		s << "Pict" << ++fileId << ".pct";
		libwps::Debug::dumpFile(data, s.str().c_str());
	}
#endif

	ascii().addPos(pos-6);
	ascii().addNote(f.str().c_str());
	return true;
}

////////////////////////////////////////////////////////////
// send data
////////////////////////////////////////////////////////////
void LotusGraph::sendPicture(LotusGraphInternal::Zone const &zone)
{
	if (!m_listener || !zone.m_pictureEntry.valid())
	{
		WPS_DEBUG_MSG(("LotusGraph::sendPicture: I can not find the listener/picture entry\n"));
		return;
	}
	librevenge::RVNGBinaryData data;
	m_input->seek(zone.m_pictureEntry.begin(), librevenge::RVNG_SEEK_SET);
	if (!libwps::readData(m_input, (unsigned long)(zone.m_pictureEntry.length()), data))
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

void LotusGraph::sendTextBox(WPSEntry const &entry)
{
	if (!m_listener || entry.length()<1)
	{
		WPS_DEBUG_MSG(("LotusGraph::sendTextBox: I can not find the listener/textbox entry\n"));
		return;
	}
	libwps::DebugStream f;
	long pos = entry.begin();
	long sz=entry.length();
	f << "GraphTextBox[data]:";
	m_input->seek(pos, librevenge::RVNG_SEEK_SET);
	int val=(int) libwps::readU8(m_input); // always 1?
	if (val!=1) f << "f0=" << val << ",";
	libwps_tools_win::Font::Type fontType = m_mainParser.getDefaultFontType();
	WPSFont font=WPSFont::getDefault();
	m_listener->setFont(font);
	bool actualFlags[7]= {false, false, false, false, false, false, false };
	for (long i=1; i<sz; ++i)
	{
		char c=(char) libwps::readU8(m_input);
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
		int mod=(int) libwps::readU8(m_input);
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
		long actPos=m_input->tell();
		bool ok=true;
		for (int d=0; d<szParam; ++d)
		{
			int mod1=(int) libwps::readU8(m_input);
			val=(int) libwps::readU8(m_input);
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
		}
		if (!ok)
		{
			m_input->seek(actPos, librevenge::RVNG_SEEK_SET);
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
			static uint32_t const(attrib[])= { WPS_BOLD_BIT, WPS_ITALICS_BIT, WPS_OUTLINE_BIT, WPS_UNDERLINE_BIT, WPS_SHADOW_BIT };
			if (mod<=5)
			{
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
			if (m_styleManager->getColor(param, color))
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
	ascii().addPos(pos);
	ascii().addNote(f.str().c_str());
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
			(new LotusGraphInternal::SubDocument(m_input, *this, zone->m_textBoxEntry));
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

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
