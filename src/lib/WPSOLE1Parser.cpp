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
	OLEZone(int levl) : m_level(levl), m_defPosition(0), m_varIdToValueMap(), m_idsList(), m_beginList(), m_lengthList(), m_childList()
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
};

/** Internal: internal method to keep ole1 state */
struct State
{
	/// constructor
	State(shared_ptr<WPSStream> fileStream) : m_fileStream(fileStream) { }
	/// the file stream
	shared_ptr<WPSStream> m_fileStream;
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

	std::map<int, WPSOLE1ParserInternal::OLEZone> idToOLEMap;
	std::map<int, std::string> idToNames;
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
		f << "level=" << level << ",";
		// main varD maxId
		// 2:4 ole1Struct, 2:5 file

		// wk3: 1,1,1,2,1 or 1,_,1,1,1
		// fm3: 1,3,1,4,1,
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
		if (level>=int(parentOLE.size())) parentOLE.resize(size_t(level-1));
		if (level==1)
		{
			// the first entry is a special 1:1, rename it with id=-1
			int id=idToOLEMap.empty() ? -1 : listIds[0];
			if (idToOLEMap.find(id) != idToOLEMap.end())
			{
				WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: find a dupplicated id\n"));
				f << "##dupplicated],";
				badOLE=WPSOLE1ParserInternal::OLEZone(level);
				ole = &badOLE;
			}
			else
			{
				idToOLEMap.insert(std::map<int, WPSOLE1ParserInternal::OLEZone>::value_type(id,WPSOLE1ParserInternal::OLEZone(level)));
				ole=&idToOLEMap.find(id)->second;
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
				idToNames[listIds[0]]=name;
				f << "name=" << name;
				input->seek(actPos, librevenge::RVNG_SEEK_SET);
			}
			else
			{
				ascFile.addPos(ole->m_beginList[0]);
				ascFile.addNote("_");
			}
		}
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
	}
	for (std::map<int, WPSOLE1ParserInternal::OLEZone>::iterator oIt=idToOLEMap.begin();
	        oIt!=idToOLEMap.end(); ++oIt)
	{
		WPSOLE1ParserInternal::OLEZone &ole=oIt->second;
		f.str("");
		f << "[";
		for (size_t i=1; i<3; ++i)
		{
			if (2*i+1 >= ole.m_idsList.size() || ole.m_idsList[2*i+1]!=1) continue;
			int nameId=ole.m_idsList[2*i];
			if (idToNames.find(nameId)!=idToNames.end())
			{
				ole.m_names[i-1]=idToNames.find(nameId)->second;
				f << ole.m_names[i-1] << ":";
			}
			else
			{
				WPS_DEBUG_MSG(("WPSOLE1Parser::createZones: oops can not find some names\n"));
				f << "##nameId=" << nameId << ",";
			}
			if (i==1) f << ":";
		}
		f << "]";
		ascFile.addPos(ole.m_defPosition);
		ascFile.addNote(f.str().c_str());
	}

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

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
