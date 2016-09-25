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

#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <string>

#include <librevenge/librevenge.h>

#include "WPSOLE1Parser.h"

#include "WPSDebug.h"
#include "WPSStream.h"
#include "WPSStringStream.h"

using namespace libwps;

//////////////////////////////////////////////////
// internal structure
//////////////////////////////////////////////////
namespace WPSOLE1ParserInternal
{
//! an OLE Zone
struct OLEZone
{
	//! constructor
	OLEZone(int levl) : m_level(levl), m_defPosition(0), m_varIdToValueMap(), m_idsList(), m_beginList(), m_lengthList(), m_childList(), m_parsed(false)
	{
	}
	//! the level
	int m_level;
	//! the position where this zone is defined
	long m_defPosition;
	//! a list of variable
	std::map<int,unsigned long> m_varIdToValueMap;
	//! the list of pair id:type
	std::vector<int> m_idsList;
	//! the list of pointers
	std::vector<long> m_beginList;
	//! the list of length
	std::vector<long> m_lengthList;
	//! the list of child
	std::vector<OLEZone> m_childList;
	//! the list of names
	std::string m_names[2];
	//! a flag to know if the zone is parsed
	mutable bool m_parsed;
};

/** Internal: internal method to keep ole1 state */
struct State
{
	/// constructor
	State(shared_ptr<WPSStream> fileStream) : m_fileStream(fileStream), m_idToZoneMap(), m_idToTypeNameMap() { }
	/// the file stream
	shared_ptr<WPSStream> m_fileStream;
	/// the map id to zone
	std::map<int, OLEZone> m_idToZoneMap;
	/// the map id to zone type
	std::map<int, std::string> m_idToTypeNameMap;
};
}

////////////////////////////////////////////////////////////
// constructor/destructor
////////////////////////////////////////////////////////////
WPSOLE1Parser::WPSOLE1Parser(shared_ptr<WPSStream> fileStream)
	: m_state(new WPSOLE1ParserInternal::State(fileStream))
{
}

WPSOLE1Parser::~WPSOLE1Parser()
{
#ifdef DEBUG
	for (std::map<int, WPSOLE1ParserInternal::OLEZone>::const_iterator oIt=m_state->m_idToZoneMap.begin();
	        oIt!=m_state->m_idToZoneMap.end(); ++oIt)
	{
		if (oIt->first>=0) checkIfParsed(oIt->second);
	}
#endif
}

////////////////////////////////////////////////////////////
// read the file structure
////////////////////////////////////////////////////////////
bool WPSOLE1Parser::createZones()
{
	if (!m_state->m_fileStream) return false;
	WPSStream &stream=*(m_state->m_fileStream);
	if (!stream.checkFilePosition(20))
		return false;
	RVNGInputStreamPtr &input = stream.m_input;
	libwps::DebugFile &ascFile=stream.m_ascii;
	libwps::DebugStream f;
	input->seek(-8, librevenge::RVNG_SEEK_END);
	long pos=long(libwps::readU32(input));
	long sz=long(libwps::readU32(input));
	long endPos=pos+sz;
	if (pos<=0||sz<=10 || pos+sz<=0 || !stream.checkFilePosition(endPos))
		return false;
	ascFile.addPos(pos);
	ascFile.addNote("Entries(OLE1Struct):");
	input->seek(pos, librevenge::RVNG_SEEK_SET);

	WPSOLE1ParserInternal::OLEZone badOLE(0);
	std::vector<WPSOLE1ParserInternal::OLEZone *> parentOLE;
	while (!input->isEnd())
	{
		pos=input->tell();
		if (pos+1>=endPos) break;
		f.str("");
		f << "OLE1Struct:";
		int level=int(libwps::readU8(input));
		if (level==0x18)
		{
			// can be followed by many FF
			ascFile.addPos(pos);
			ascFile.addNote(f.str().c_str());
			break;
		}
		if (pos+10>=endPos) break;
		if (level<1 && level>3)
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			break;
		}
		if (level!=1)
			f << "level=" << level << ",";
		int const nIds=8-2*level;
		if (pos+1+2*nIds+1>=endPos) break;
		f << "ids=[";
		std::vector<int> listIds;
		for (int i=0; i<nIds; ++i)   // f0=0|1, f2=1-18: some type?, f3=0-1
		{
			int val=int(libwps::readU16(input));
			listIds.push_back(val);
			if (val) f << val << ",";
			else f << "_,";
		}
		f << "],";

		WPSOLE1ParserInternal::OLEZone *ole;
		if (level<=int(parentOLE.size())) parentOLE.resize(size_t(level-1));
		if (level==1)
		{
			// the first entry is a special 1:1, rename it with id=-1, it contains
			//    in varD the maxId
			//    in child 2:4 ole1Struct, 2:5 file
			int id=m_state->m_idToZoneMap.empty() ? -1 : listIds[0];
			if (m_state->m_idToZoneMap.find(id) != m_state->m_idToZoneMap.end())
			{
				WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: find a dupplicated id\n"));
				f << "##dupplicated,";
				badOLE=WPSOLE1ParserInternal::OLEZone(level);
				ole = &badOLE;
			}
			else
			{
				m_state->m_idToZoneMap.insert(std::map<int, WPSOLE1ParserInternal::OLEZone>::value_type(id,WPSOLE1ParserInternal::OLEZone(level)));
				ole=&m_state->m_idToZoneMap.find(id)->second;
				parentOLE.push_back(ole);
			}
		}
		else if (level-2>=int(parentOLE.size()))
		{
			WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: can not find some parent\n"));
			f << "##parent[no],";
			badOLE=WPSOLE1ParserInternal::OLEZone(level);
			ole = &badOLE;
		}
		else
		{
			WPSOLE1ParserInternal::OLEZone *parent=parentOLE[size_t(level-2)];
			parent->m_childList.push_back(WPSOLE1ParserInternal::OLEZone(level));
			ole=&parent->m_childList.back();
			parentOLE.push_back(ole);
		}
		ole->m_idsList=listIds;
		ole->m_defPosition=pos;
		bool ok=false;
		while (true)
		{
			long actPos=input->tell();
			if (actPos+1>endPos) break;
			int type=int(libwps::readU8(input));
			bool done=false;
			switch (type)
			{
			case 4: // 1|2 seems related to the zone type 1:main? 2:auxiliary?
			case 0xa: // never seems
			case 0xb: // always with 1,0
			case 0xd:   // zone ptr?
			{
				if (actPos+5>endPos) break;
				ok=true;
				unsigned long val=libwps::readU32(input);
				if (ole->m_varIdToValueMap.find(type)!=ole->m_varIdToValueMap.end())
				{
					WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: oops some value are already set\n"));
					f << "###";
				}
				else
					ole->m_varIdToValueMap[type]=val;
				f << "var" << std::hex << type << std::dec << "=" << std::hex << val << std::dec << ",";
				done=(type==0xa || type==0xb || type==0xd);
				break;
			}
			case 5:
			case 6:
			{
				if (actPos+9>endPos) break;
				ok=true;
				long dPtr=long(libwps::readU32(input));
				long dSz=long(libwps::readU32(input));
				if (dSz>0)
				{
					if (dPtr<0 || dSz<0 || dPtr+dSz<0 || !stream.checkFilePosition(dPtr+dSz))
					{
						WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: oops some zone seems bad\n"));
						f << "###";
					}
					else
					{
						ole->m_beginList.push_back(dPtr);
						ole->m_lengthList.push_back(dSz);
					}
					f << "ptr" << type << "=" << std::hex << dPtr << "<->" << dPtr+dSz << std::dec << ",";
				}
				done=type==5;
				break;
			}
			case 9:
				f << "data9,";
				if (ole->m_varIdToValueMap.find(type)!=ole->m_varIdToValueMap.end())
				{
					WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: oops some value are already set\n"));
					f << "###";
				}
				else
					ole->m_varIdToValueMap[type]=0;
				done=ok=true;
				break;
			default:
				break;
			}
			if (done) break;
			if (!ok) break;
		}
		if (!ok)
		{
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			break;
		}
		if (!ole->m_lengthList.empty())
		{
			if (level==1 && listIds[4]==21 && listIds[5]==0 && ole->m_lengthList.size()==1 && ole->m_lengthList[0]<100)
			{
				// basic string
				long actPos=input->tell();
				libwps::DebugStream f2;
				input->seek(ole->m_beginList[0], librevenge::RVNG_SEEK_SET);
				f2 << "OLE1Struct[name]:";
				std::string name;
				for (int i=0; i<int(ole->m_lengthList[0])-1; ++i)	name+=char(libwps::readU8(input));
				f2 << name;
				ascFile.addPos(ole->m_beginList[0]);
				ascFile.addNote(f2.str().c_str());
				m_state->m_idToTypeNameMap[listIds[0]]=name;
				f << "name=" << name;
				ole->m_parsed=true;
				input->seek(actPos, librevenge::RVNG_SEEK_SET);
			}
		}
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
	}
	for (std::map<int, WPSOLE1ParserInternal::OLEZone>::iterator oIt=m_state->m_idToZoneMap.begin();
	        oIt!=m_state->m_idToZoneMap.end(); ++oIt)
		updateZoneNames(oIt->second);

	if (input->tell()+4<endPos)
	{
		WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: I have loose the trail!!!\n"));
		ascFile.addPos(input->tell());
		ascFile.addNote("OLE1Struct-###:");
	}
	ascFile.addPos(endPos);
	ascFile.addNote("OLE1Struct-end:");
	return true;
}

shared_ptr<WPSStream> WPSOLE1Parser::getStreamForName(std::string const &name) const
{
	if (name.empty()) return shared_ptr<WPSStream>();
	for (std::map<int, WPSOLE1ParserInternal::OLEZone>::const_iterator oIt=m_state->m_idToZoneMap.begin();
	        oIt!=m_state->m_idToZoneMap.end(); ++oIt)
	{
		if (oIt->second.m_names[1]==name)
			return getStream(oIt->second);
	}
	WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: can not find any stream with name=%s\n", name.c_str()));
	return shared_ptr<WPSStream>();
}

shared_ptr<WPSStream> WPSOLE1Parser::getStreamForId(int id) const
{
	if (m_state->m_idToZoneMap.find(id)==m_state->m_idToZoneMap.end())
	{
		WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: can not find any stream with id=%d\n", id));
		return shared_ptr<WPSStream>();
	}
	return getStream(m_state->m_idToZoneMap.find(id)->second);
}

bool WPSOLE1Parser::updateZoneNames(WPSOLE1ParserInternal::OLEZone &ole) const
{
	libwps::DebugStream f;
	f << "[";
	size_t maxId=ole.m_idsList.size()/2;
	size_t firstId=ole.m_level==1 ? 1 : 0;
	for (size_t i=firstId; i<maxId; ++i)
	{
		if (ole.m_idsList[2*i+1]!=1 || (i==0 && maxId==3)) continue;
		int nameId=ole.m_idsList[2*i];
		if (m_state->m_idToTypeNameMap.find(nameId)!=m_state->m_idToTypeNameMap.end())
		{
			ole.m_names[i-firstId]=m_state->m_idToTypeNameMap.find(nameId)->second;
			f << ole.m_names[i-firstId];
		}
		else
		{
			WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: oops can not find some names\n"));
			f << "##nameId=" << nameId << ",";
		}
		if (i+1!=maxId) f << "/";
	}
	f << "]";
	for (size_t c=0; c<ole.m_childList.size(); ++c)
		updateZoneNames(ole.m_childList[c]);
	if (m_state->m_fileStream.get()!=0)
	{
		m_state->m_fileStream->m_ascii.addPos(ole.m_defPosition);
		m_state->m_fileStream->m_ascii.addNote(f.str().c_str());
	}
	return true;
}

shared_ptr<WPSStream> WPSOLE1Parser::getStream(WPSOLE1ParserInternal::OLEZone const &zone) const
{
	shared_ptr<WPSStream> res;
	zone.m_parsed=true;
	if (zone.m_beginList.empty() || !m_state->m_fileStream ||
	        zone.m_idsList.empty() || zone.m_beginList.size()!=zone.m_lengthList.size())
		return res;
	RVNGInputStreamPtr input=m_state->m_fileStream->m_input;
	if (zone.m_beginList.size()==1)
	{
		res.reset(new WPSStream(input, m_state->m_fileStream->m_ascii));
		res->m_eof=zone.m_beginList[0]+zone.m_lengthList[0];
		input->seek(zone.m_beginList[0], librevenge::RVNG_SEEK_SET);
		return res;
	}
	shared_ptr<WPSStringStream> newInput;
	for (size_t i=0; i<zone.m_beginList.size(); ++i)
	{
		input->seek(zone.m_beginList[i], librevenge::RVNG_SEEK_SET);
		unsigned long numRead;
		const unsigned char *data=input->read(static_cast<unsigned long>(zone.m_lengthList[i]), numRead);
		if (!data || long(numRead)!=zone.m_lengthList[i])
		{
			WPS_DEBUG_MSG(("WPSOLE1Parser::getStream: can not read some data\n"));
			return res;
		}
		if (i==0)
			newInput.reset(new WPSStringStream(data, unsigned(numRead)));
		else
			newInput->append(data, unsigned(numRead));
		m_state->m_fileStream->m_ascii.skipZone(zone.m_beginList[i], zone.m_beginList[i]+zone.m_lengthList[i]-1);
	}
	res.reset(new WPSStream(newInput));
	newInput->seek(0, librevenge::RVNG_SEEK_SET);
	std::stringstream s;
	s << "Data" << zone.m_idsList[0];
	res->m_ascii.open(s.str());
	res->m_ascii.setStream(newInput);
	return res;
}

bool WPSOLE1Parser::updateEmbeddedObject(int id, WPSEmbeddedObject &object) const
{
	if (m_state->m_idToZoneMap.find(id)==m_state->m_idToZoneMap.end())
	{
		WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: can not find any zone with id=%d\n", id));
		return false;
	}
	WPSOLE1ParserInternal::OLEZone const &zone=m_state->m_idToZoneMap.find(id)->second;
	if (zone.m_names[1]!="Lotus:TOOLS:OEMString")
	{
		WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: the zone name \"%s\"seems odd\n", zone.m_names[1].empty() ? "" : zone.m_names[1].c_str()));
	}
	zone.m_parsed=true;
	bool done=true;
	/* normally two children:
	   - first with name "Lotus:TOOLS:OEMString" which contains .ole in varD
	   - second with name "Lotus:TOOLS:ByteStream" which contains the data
	*/
	for (size_t c=0; c<zone.m_childList.size(); ++c)
	{
		if (!zone.m_childList[c].m_beginList.empty())
		{
			shared_ptr<WPSStream> stream=getStream(zone);
			if (stream)	done |= readPicture(stream, object);
		}
	}
	if (!done)
	{
		WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: can not find any picture child for zone with id=%d\n", id));
	}
	return done;
}

bool WPSOLE1Parser::updateMetaData(librevenge::RVNGPropertyList &metadata, libwps_tools_win::Font::Type encoding) const
{
	for (std::map<int, WPSOLE1ParserInternal::OLEZone>::const_iterator oIt=m_state->m_idToZoneMap.begin();
	        oIt!=m_state->m_idToZoneMap.end(); ++oIt)
	{
		if (oIt->second.m_names[1]!="Doc Info Object")
			continue;
		WPSOLE1ParserInternal::OLEZone const &zone=oIt->second;
		// either a node which regroup all document info or a list of node with informations
		size_t numChilds=zone.m_childList.size();
		for (size_t c=0; c<(numChilds ? numChilds : 1); ++c)
		{
			WPSOLE1ParserInternal::OLEZone const &child=numChilds ? zone.m_childList[c] : zone;
			if (child.m_beginList.empty()) continue;
			shared_ptr<WPSStream> childStream=getStream(child);
			if (!childStream) continue;

			RVNGInputStreamPtr input=childStream->m_input;
			long pos=input->tell();
			libwps::DebugFile &ascFile=childStream->m_ascii;
			libwps::DebugStream f;
			f << "Entries(MetaData)[" << child.m_names[0] << "]:";
			if (!childStream->checkFilePosition(pos+4))
			{
				WPS_DEBUG_MSG(("WPSOLE1Parser::updateMetaData: a meta data zone seems too short\n"));
				f << "###";
				ascFile.addPos(pos);
				ascFile.addNote(f.str().c_str());
				continue;
			}
			int id=int(libwps::readU16(input));
			int dSz=int(libwps::readU16(input));
			if (!childStream->checkFilePosition(pos+4+dSz))
			{
				WPS_DEBUG_MSG(("WPSOLE1Parser::updateMetaData: a meta data zone seems too short\n"));
				f << "###";
				ascFile.addPos(pos);
				ascFile.addNote(f.str().c_str());
				continue;
			}
			int wh=-1;
			if (child.m_names[0]=="Doc Info Author") wh=9;
			else if (child.m_names[0]=="Doc Info Last Revisor") wh=5;
			else if (child.m_names[0]=="Doc Info Comments") wh=0;
			else if (child.m_names[0]=="Doc Info Property") wh=1; // find always sSz=0
			else if (child.m_names[0]=="Doc Info Editing Time") wh=6; // sSz=4 + 2 int
			else if (child.m_names[0]=="Doc Info Revisions Count") wh=0xc; // sSz=2 + count
			else if (child.m_names[0]=="Doc Info Creation Date") wh=7; // sz=a or c
			else if (child.m_names[0]=="Doc Info Last Revision Date") wh=0xa;
			else if (child.m_names[0]=="Doc Info Last Printed Date") wh=0xd;
			if (wh==-1 || wh!=(id&0xFE7F))
			{
				WPS_DEBUG_MSG(("WPSOLE1Parser::updateMetaData: find unknown data\n"));
				f << "###unknown";
				ascFile.addPos(pos);
				ascFile.addNote(f.str().c_str());
				continue;
			}
			bool ok=false;
			switch (wh)
			{
			case 1:
				ok=dSz==0;
				break;
			case 0:
			case 5:
			case 9:
			{
				librevenge::RVNGString name("");
				for (int i=0; i<dSz; ++i)
				{
					unsigned char ch=static_cast<unsigned char>(libwps::readU8(input));
					if (!ch) break;
					libwps::appendUnicode((uint32_t)libwps_tools_win::Font::unicode(ch,encoding), name);
				}
				ok=true;
				if (name.empty()) break;
				if (wh==9)
					metadata.insert("dc:creator", name);
				else if (wh==0)
					metadata.insert("dc:description", name);
				f << name.cstr() << ",";
				break;
			}
			case 0xc:
				if (dSz!=2) break;
				ok=true;
				f << "rev=" << libwps::readU16(input) << ",";
				break;
			case 6:
			{
				if (dSz!=4) break;
				ok=true;
				f << "time=" << libwps::readU16(input) << ",";
				int val=libwps::readU16(input); // 0
				if (val) f << "f0=" << val << ",";
				break;
			}
			case 7:
			case 0xa:
			case 0xd:
			{
				if (dSz!=10 && dSz!=12) break;
				ok=true;
				f << "date=" << libwps::readU16(input) << ",";
				int const numData=(dSz-2)/2;
				for (int i=0; i<numData; i++)   // f0=0, f1=0-16, f2=0-36, f3=0-39, f4=0
				{
					int val=libwps::readU16(input); // 0
					if (val) f << "f" << i << "=" << val << ",";
				}
				break;
			}
			default:
				ok=false;
				break;
			}
			if (!ok)
			{
				WPS_DEBUG_MSG(("WPSOLE1Parser::updateMetaData: can not read some data\n"));
				f << "##unknown,";
			}
			if (input->tell()!=pos+4+dSz)
				ascFile.addDelimiter(input->tell(),'|');
			ascFile.addPos(pos);
			ascFile.addNote(f.str().c_str());
		}
	}
	return false;
}

void WPSOLE1Parser::checkIfParsed(WPSOLE1ParserInternal::OLEZone const &zone) const
{
	if (zone.m_parsed) return;
	for (size_t c=0; c<zone.m_childList.size(); ++c) checkIfParsed(zone.m_childList[c]);
	if (zone.m_beginList.empty() || !m_state->m_fileStream) return;
	if (zone.m_names[1]=="Lotus:TOOLS:ByteStream")
	{
		shared_ptr<WPSStream> stream=getStream(zone);
		WPSEmbeddedObject object;
		if (stream && readPicture(stream, object)) return;
	}
	libwps::DebugStream f;
	f << "Entries(Unparsed):";
	for (int i=0; i<2; ++i)
	{
		if (!zone.m_names[i].empty()) f << zone.m_names[i] << ",";
	}
	m_state->m_fileStream->m_ascii.addPos(zone.m_beginList[0]);
	m_state->m_fileStream->m_ascii.addNote(f.str().c_str());
}

bool WPSOLE1Parser::readPicture(shared_ptr<WPSStream> stream, WPSEmbeddedObject &object) const
{
	if (!stream) return false;
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	while (stream->checkFilePosition(input->tell()+24))
	{
		long pos = input->tell();
		long type = long(libwps::read16(input));
		if (type!=0x501 || !stream->checkFilePosition(pos+24))
		{
			WPS_DEBUG_MSG(("WPSOLE1Parser::readPicture: not a picture header\n"));
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			break;
		}
		f.str("");
		f << "Entries(Picture):";
		for (int i=0; i<3; ++i)   // f1=2|5
		{
			int val=int(libwps::readU16(input));
			if (val) f << "f" << i << "=" << val << ",";
		}
		int sSz=int(libwps::readU16(input));
		if (!sSz || !stream->checkFilePosition(pos+24+sSz))
		{
			WPS_DEBUG_MSG(("WPSOLE1Parser::readPicture: name size seems bad\n"));
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			break;
		}
		int val=int(libwps::readU16(input)); // 0
		if (val) f << "f4=" << val << ",";
		std::string name;
		for (int i=0; i<sSz; ++i)
		{
			char c=char(libwps::readU8(input));
			if (c) name+=c;
		}
		// find Paint.Picture, WangImage.Document, METAFILEPICT
		f << name << ",";
		for (int i=0; i<4; ++i)   // g0, g2, g3: some application id?
		{
			val=int(libwps::readU16(input));
			if (val) f << "g" << i << "=" << std::hex << val << std::dec << ",";
		}
		unsigned long dSz=libwps::readU32(input);
		if (dSz>0x40000000 || dSz<10 || !stream->checkFilePosition(pos+24+long(dSz)))
		{
			WPS_DEBUG_MSG(("WPSOLE1Parser::readPicture: pict size seems bad\n"));
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			break;
		}
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());

		pos=input->tell();
		librevenge::RVNGBinaryData data;
		if (!libwps::readData(input, dSz, data))
		{
			WPS_DEBUG_MSG(("WPSOLE1Parser::readPicture: I can not find the picture\n"));
			input->seek(pos, librevenge::RVNG_SEEK_SET);
			break;
		}
		// LibreOffice does not handle metafile pict, so ignore
		if (name!="METAFILEPICT")
			object.add(data);
#ifdef DEBUG_WITH_FILES
		ascFile.skipZone(pos, pos+long(dSz)-1);
		std::stringstream s;
		static int fileId=0;
		if (name=="METAFILEPICT")
			s << "PictOLE" << ++fileId << ".metafile";
		else
			s << "PictOLE" << ++fileId << ".pct";
		libwps::Debug::dumpFile(data, s.str().c_str());
#endif
	}
	long pos=input->tell();
	if (pos!=stream->m_eof)
	{
		WPS_DEBUG_MSG(("WPSOLE1Parser::readPicture: find some extra data\n"));
		ascFile.addPos(pos);
		ascFile.addNote("Picture:###");
	}
	return true;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
