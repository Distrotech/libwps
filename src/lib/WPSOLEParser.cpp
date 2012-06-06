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

/*
 *  freely inspired from istorage :
 * ------------------------------------------------------------
 *      Generic OLE Zones furnished with a copy of the file header
 *
 * Compound Storage (32 bit version)
 * Storage implementation
 *
 * This file contains the compound file implementation
 * of the storage interface.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Thuy Nguyen
 * Copyright 2005 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * NOTES
 *  The compound file implementation of IStorage used for create
 *  and manage substorages and streams within a storage object
 *  residing in a compound file object.
 *
 * ------------------------------------------------------------
 */

#include <cstring>
#include <map>
#include <sstream>
#include <string>

#include <libwpd/WPXBinaryData.h>

#include "WPSOLEStream.h"
#include "WPSPosition.h"

#include "WPSOLEParser.h"

//////////////////////////////////////////////////
// internal structure
//////////////////////////////////////////////////
namespace WPSOLEParserInternal
{
/** Internal: internal method to compobj definition */
class CompObj
{
public:
	//! the constructor
	CompObj() : m_mapCls()
	{
		initCLSMap();
	}

	/** return the CLS Name corresponding to an identifier */
	char const *getCLSName(unsigned long v)
	{
		if (m_mapCls.find(v) == m_mapCls.end() ) return 0L;
		return m_mapCls[v];
	}

protected:
	/** map CLSId <-> name */
	std::map<unsigned long, char const *> m_mapCls;

	/** initialise a map CLSId <-> name */
	void initCLSMap()
	{
		// source: binfilter/bf_so3/source/inplace/embobj.cxx
		m_mapCls[0x00000319]="Picture"; // addon Enhanced Metafile ( find in some file)

		m_mapCls[0x00021290]="MSClipArtGalley2";
		m_mapCls[0x000212F0]="MSWordArt"; // or MSWordArt.2
		m_mapCls[0x00021302]="MSWorksWPDoc"; // addon

		// MS Apps
		m_mapCls[0x00030000]= "ExcelWorksheet";
		m_mapCls[0x00030001]= "ExcelChart";
		m_mapCls[0x00030002]= "ExcelMacrosheet";
		m_mapCls[0x00030003]= "WordDocument";
		m_mapCls[0x00030004]= "MSPowerPoint";
		m_mapCls[0x00030005]= "MSPowerPointSho";
		m_mapCls[0x00030006]= "MSGraph";
		m_mapCls[0x00030007]= "MSDraw"; // find also ca003 ?
		m_mapCls[0x00030008]= "Note-It";
		m_mapCls[0x00030009]= "WordArt";
		m_mapCls[0x0003000a]= "PBrush";
		m_mapCls[0x0003000b]= "Equation"; // "Microsoft Equation Editor"
		m_mapCls[0x0003000c]= "Package";
		m_mapCls[0x0003000d]= "SoundRec";
		m_mapCls[0x0003000e]= "MPlayer";
		// MS Demos
		m_mapCls[0x0003000f]= "ServerDemo"; // "OLE 1.0 Server Demo"
		m_mapCls[0x00030010]= "Srtest"; // "OLE 1.0 Test Demo"
		m_mapCls[0x00030011]= "SrtInv"; //  "OLE 1.0 Inv Demo"
		m_mapCls[0x00030012]= "OleDemo"; //"OLE 1.0 Demo"

		// Coromandel / Dorai Swamy / 718-793-7963
		m_mapCls[0x00030013]= "CoromandelIntegra";
		m_mapCls[0x00030014]= "CoromandelObjServer";

		// 3-d Visions Corp / Peter Hirsch / 310-325-1339
		m_mapCls[0x00030015]= "StanfordGraphics";

		// Deltapoint / Nigel Hearne / 408-648-4000
		m_mapCls[0x00030016]= "DGraphCHART";
		m_mapCls[0x00030017]= "DGraphDATA";

		// Corel / Richard V. Woodend / 613-728-8200 x1153
		m_mapCls[0x00030018]= "PhotoPaint"; // "Corel PhotoPaint"
		m_mapCls[0x00030019]= "CShow"; // "Corel Show"
		m_mapCls[0x0003001a]= "CorelChart";
		m_mapCls[0x0003001b]= "CDraw"; // "Corel Draw"

		// Inset Systems / Mark Skiba / 203-740-2400
		m_mapCls[0x0003001c]= "HJWIN1.0"; // "Inset Systems"

		// Mark V Systems / Mark McGraw / 818-995-7671
		m_mapCls[0x0003001d]= "ObjMakerOLE"; // "MarkV Systems Object Maker"

		// IdentiTech / Mike Gilger / 407-951-9503
		m_mapCls[0x0003001e]= "FYI"; // "IdentiTech FYI"
		m_mapCls[0x0003001f]= "FYIView"; // "IdentiTech FYI Viewer"

		// Inventa Corporation / Balaji Varadarajan / 408-987-0220
		m_mapCls[0x00030020]= "Stickynote";

		// ShapeWare Corp. / Lori Pearce / 206-467-6723
		m_mapCls[0x00030021]= "ShapewareVISIO10";
		m_mapCls[0x00030022]= "ImportServer"; // "Spaheware Import Server"

		// test app SrTest
		m_mapCls[0x00030023]= "SrvrTest"; // "OLE 1.0 Server Test"

		// test app ClTest.  Doesn't really work as a server but is in reg db
		m_mapCls[0x00030025]= "Cltest"; // "OLE 1.0 Client Test"

		// Microsoft ClipArt Gallery   Sherry Larsen-Holmes
		m_mapCls[0x00030026]= "MS_ClipArt_Gallery";
		// Microsoft Project  Cory Reina
		m_mapCls[0x00030027]= "MSProject";

		// Microsoft Works Chart
		m_mapCls[0x00030028]= "MSWorksChart";

		// Microsoft Works Spreadsheet
		m_mapCls[0x00030029]= "MSWorksSpreadsheet";

		// AFX apps - Dean McCrory
		m_mapCls[0x0003002A]= "MinSvr"; // "AFX Mini Server"
		m_mapCls[0x0003002B]= "HierarchyList"; // "AFX Hierarchy List"
		m_mapCls[0x0003002C]= "BibRef"; // "AFX BibRef"
		m_mapCls[0x0003002D]= "MinSvrMI"; // "AFX Mini Server MI"
		m_mapCls[0x0003002E]= "TestServ"; // "AFX Test Server"

		// Ami Pro
		m_mapCls[0x0003002F]= "AmiProDocument";

		// WordPerfect Presentations For Windows
		m_mapCls[0x00030030]= "WPGraphics";
		m_mapCls[0x00030031]= "WPCharts";

		// MicroGrafx Charisma
		m_mapCls[0x00030032]= "Charisma";
		m_mapCls[0x00030033]= "Charisma_30"; // v 3.0
		m_mapCls[0x00030034]= "CharPres_30"; // v 3.0 Pres
		// MicroGrafx Draw
		m_mapCls[0x00030035]= "Draw"; //"MicroGrafx Draw"
		// MicroGrafx Designer
		m_mapCls[0x00030036]= "Designer_40"; // "MicroGrafx Designer 4.0"

		// STAR DIVISION
		//m_mapCls[0x000424CA]= "StarMath"; // "StarMath 1.0"
		m_mapCls[0x00043AD2]= "FontWork"; // "Star FontWork"
		//m_mapCls[0x000456EE]= "StarMath2"; // "StarMath 2.0"
	}
};

/** Internal: internal method to keep ole definition */
struct OleDef
{
	OleDef() : m_id(-1), m_subId(-1), m_dir(""), m_name("") { }
	int m_id /**main id*/, m_subId /**subsversion id */ ;
	std::string m_dir/**the directory*/, m_name/**the base name*/;
};
}

// constructor/destructor
WPSOLEParser::WPSOLEParser(const std::string &mainName)
	: m_avoidOLE(mainName), m_unknownOLEs(),
	  m_objects(), m_objectsPosition(), m_objectsId(), m_compObjIdName()
{
}

WPSOLEParser::~WPSOLEParser()
{
}

// interface
bool WPSOLEParser::getObject(int id, WPXBinaryData &obj, WPSPosition &pos)  const
{
	for (size_t i = 0; i < m_objectsId.size(); i++)
	{
		if (m_objectsId[i] != id) continue;
		obj = m_objects[i];
		pos = m_objectsPosition[i];
		return true;
	}
	obj.clear();
	return false;
}

void WPSOLEParser::setObject(int id, WPXBinaryData const &obj, WPSPosition const &pos)
{
	for (size_t i = 0; i < m_objectsId.size(); i++)
	{
		if (m_objectsId[i] != id) continue;
		m_objects[i] = obj;
		m_objectsPosition[i] = pos;
		return;
	}
	m_objects.push_back(obj);
	m_objectsPosition.push_back(pos);
	m_objectsId.push_back(id);
}

// parsing
bool WPSOLEParser::parse(shared_ptr<libwps::Storage> file)
{
	if (!m_compObjIdName)
		m_compObjIdName.reset(new WPSOLEParserInternal::CompObj);

	m_unknownOLEs.resize(0);
	m_objects.resize(0);
	m_objectsId.resize(0);

	if (!file.get()) return false;

	if (!file->isOLEStream()) return false;

	std::vector<std::string> namesList = file->getOLENames();

	//
	// we begin by grouping the Ole by their potential main id
	//
	std::multimap<int, WPSOLEParserInternal::OleDef> listsById;
	std::vector<int> listIds;
	for (size_t i = 0; i < namesList.size(); i++)
	{
		std::string const &name = namesList[i];

		// separated the directory and the name
		//    MatOST/MatadorObject1/Ole10Native
		//      -> dir="MatOST/MatadorObject1", base="Ole10Native"
		std::string::size_type pos = name.find_last_of('/');

		std::string dir, base;
		if (pos == std::string::npos) base = name;
		else if (pos == 0) base = name.substr(1);
		else
		{
			dir = name.substr(0,pos);
			base = name.substr(pos+1);
		}
		if (dir == "" && base == m_avoidOLE) continue;

#define PRINT_OLE_NAME
#ifdef PRINT_OLE_NAME
		WPS_DEBUG_MSG(("WPSOLEParser::parse: find OLEName=%s\n", name.c_str()));
#endif
		WPSOLEParserInternal::OleDef data;
		data.m_name = name;
		data.m_dir = base;

		// try to retrieve the identificator stored in the directory
		//  MatOST/MatadorObject1/ -> 1, -1
		//  Object 2/ -> 2, -1
		dir+='/';
		pos = dir.find('/');
		int id[2] = { -1, -1};
		while (pos != std::string::npos)
		{
			if (pos >= 1 && dir[pos-1] >= '0' && dir[pos-1] <= '9')
			{
				std::string::size_type idP = pos-1;
				while (idP >=1 && dir[idP-1] >= '0' && dir[idP-1] <= '9')
					idP--;
				int val = atoi(dir.substr(idP, idP-pos).c_str());
				if (id[0] == -1) id[0] = val;
				else
				{
					id[1] = val;
					break;
				}
			}
			pos = dir.find('/', pos+1);
		}
		data.m_id = id[0];
		data.m_subId = id[1];
		if (listsById.find(data.m_id) == listsById.end())
			listIds.push_back(data.m_id);
		listsById.insert(std::multimap<int, WPSOLEParserInternal::OleDef>::value_type(data.m_id, data));
	}

	for (size_t i = 0; i < listIds.size(); i++)
	{
		int id = listIds[i];

		std::multimap<int, WPSOLEParserInternal::OleDef>::iterator pos =
		    listsById.lower_bound(id);

		// try to find a representation for each id
		// FIXME: maybe we must also find some for each subid
		WPXBinaryData pict;
		int confidence = -1000;
		WPSPosition actualPos, potentialSize;

		while (pos != listsById.end())
		{
			WPSOLEParserInternal::OleDef const &dOle = pos->second;
			if (pos->first != id) break;
			pos++;

			WPXInputStreamPtr ole = file->getDocumentOLEStream(dOle.m_name);
			if (!ole.get())
			{
				WPS_DEBUG_MSG(("WPSOLEParser: error: can not find OLE part: \"%s\"\n", dOle.m_name.c_str()));
				continue;
			}

			libwps::DebugFile asciiFile(ole);
			asciiFile.open(dOle.m_name);

			WPXBinaryData data;
			bool hasData = false;
			int newConfidence = -2000;
			bool ok = true;
			WPSPosition pictPos;

			try
			{
				if (readMM(ole, dOle.m_dir, asciiFile));
				else if (readObjInfo(ole, dOle.m_dir, asciiFile));
				else if (readOle(ole, dOle.m_dir, asciiFile));
				else if (isOlePres(ole, dOle.m_dir) &&
				         readOlePres(ole, data, pictPos, asciiFile))
				{
					hasData = true;
					newConfidence = 2;
				}
				else if (isOle10Native(ole, dOle.m_dir) &&
				         readOle10Native(ole, data, asciiFile))
				{
					hasData = true;
					// small size can be a symptom that this is a link to a
					// basic msworks data file, so we reduce confidence
					newConfidence = data.size() > 1000 ? 4 : 2;
				}
				else if (readCompObj(ole, dOle.m_dir, asciiFile));
				else if (readContents(ole, dOle.m_dir, data, pictPos, asciiFile))
				{
					hasData = true;
					newConfidence = 3;
				}
				else if (readCONTENTS(ole, dOle.m_dir, data, pictPos, asciiFile))
				{
					hasData = true;
					newConfidence = 3;
				}
				else
					ok = false;
			}
			catch (...)
			{
				ok = false;
			}
			if (!ok)
			{
				m_unknownOLEs.push_back(dOle.m_name);
				asciiFile.reset();
				continue;
			}

			if (hasData && data.size())
			{
				// probably only a subs data
				if (dOle.m_subId != -1) newConfidence -= 10;

				if (newConfidence > confidence ||
				        (newConfidence == confidence && pict.size() < data.size()))
				{
					confidence = newConfidence;
					pict = data;
					actualPos = pictPos;
				}

				if (actualPos.naturalSize().x() > 0 && actualPos.naturalSize().y() > 0)
					potentialSize = actualPos;
#ifdef DEBUG_WITH_FILES
				libwps::Debug::dumpFile(data, dOle.m_name.c_str());
#endif
			}

			asciiFile.reset();

#ifndef DEBUG
			if (confidence >= 3) break;
#endif
		}

		if (pict.size())
		{
			m_objects.push_back(pict);
			if (actualPos.naturalSize().x() <= 0. || actualPos.naturalSize().y() <= 0.)
			{
				Vec2f size = potentialSize.naturalSize();
				if (size.x() > 0 && size.y() > 0)
					actualPos.setNaturalSize(actualPos.getInvUnitScale(potentialSize.unit())*size);
			}
			m_objectsPosition.push_back(actualPos);
			m_objectsId.push_back(id);
		}
	}

	return true;
}



////////////////////////////////////////
//
// small structure
//
////////////////////////////////////////
bool WPSOLEParser::readOle(WPXInputStreamPtr &ip, std::string const &oleName,
                           libwps::DebugFile &ascii)
{
	if (!ip.get()) return false;

	if (strcmp("Ole",oleName.c_str()) != 0) return false;

	if (ip->seek(20, WPX_SEEK_SET) != 0 || ip->tell() != 20) return false;

	ip->seek(0, WPX_SEEK_SET);

	int val[20];
	for (int i= 0; i < 20; i++)
	{
		val[i] = libwps::read8(ip);
		if (val[i] < -10 || val[i] > 10) return false;
	}

	libwps::DebugStream f;
	f << "@@Ole: ";
	// always 1, 0, 2, 0*
	for (int i = 0; i < 20; i++)
		if (val[i]) f << "f" << i << "=" << val[i] << ",";
	ascii.addPos(0);
	ascii.addNote(f.str().c_str());

	if (!ip->atEOS())
	{
		ascii.addPos(20);
		ascii.addNote("@@Ole:###");
	}

	return true;
}

bool WPSOLEParser::readObjInfo(WPXInputStreamPtr &input, std::string const &oleName,
                               libwps::DebugFile &ascii)
{
	if (strcmp(oleName.c_str(),"ObjInfo") != 0) return false;

	input->seek(14, WPX_SEEK_SET);
	if (input->tell() != 6 || !input->atEOS()) return false;

	input->seek(0, WPX_SEEK_SET);
	libwps::DebugStream f;
	f << "@@ObjInfo:";

	// always 0, 3, 4 ?
	for (int i = 0; i < 3; i++) f << libwps::read16(input) << ",";

	ascii.addPos(0);
	ascii.addNote(f.str().c_str());

	return true;
}

bool WPSOLEParser::readMM(WPXInputStreamPtr &input, std::string const &oleName,
                          libwps::DebugFile &ascii)
{
	if (strcmp(oleName.c_str(),"MM") != 0) return false;

	input->seek(14, WPX_SEEK_SET);
	if (input->tell() != 14 || !input->atEOS()) return false;

	input->seek(0, WPX_SEEK_SET);
	int entete = libwps::readU16(input);
	if (entete != 0x444e)
	{
		if (entete == 0x4e44)
		{
			WPS_DEBUG_MSG(("WPSOLEParser::readMM: ERROR: endian mode probably bad, potentially bad PC/Mac mode detection.\n"));
		}
		return false;
	}
	libwps::DebugStream f;
	f << "@@MM:";

	int val[6];
	for (int i = 0; i < 6; i++)
		val[i] = libwps::read16(input);

	switch(val[5])
	{
	case 0:
		f << "conversion,";
		break;
	case 2:
		f << "Works3,";
		break;
	case 4:
		f << "Works4,";
		break;
	default:
		f << "version=unknown,";
		break;
	}

	// 1, 0, 0, 0, 0 : Mac file
	// 0, 1, 0, [0,1,2,4,6], 0 : Pc file
	for (int i = 0; i < 5; i++)
	{
		if ((i%2)!=1 && val[i]) f << "###";
		f << val[i] << ",";
	}

	ascii.addPos(0);
	ascii.addNote(f.str().c_str());

	return true;
}


bool WPSOLEParser::readCompObj(WPXInputStreamPtr &ip, std::string const &oleName, libwps::DebugFile &ascii)
{
	if (strncmp(oleName.c_str(), "CompObj", 7) != 0) return false;

	// check minimal size
	const int minSize = 12 + 14+ 16 + 12; // size of header, clsid, footer, 3 string size
	if (ip->seek(minSize,WPX_SEEK_SET) != 0 || ip->tell() != minSize) return false;

	libwps::DebugStream f;
	f << "@@CompObj(Header): ";
	ip->seek(0,WPX_SEEK_SET);

	for (int i = 0; i < 6; i++)
	{
		long val = long(libwps::readU16(ip));
		f << val << ", ";
	}

	ascii.addPos(0);
	ascii.addNote(f.str().c_str());

	ascii.addPos(12);
	// the clsid
	unsigned long clsData[4]; // ushort n1, n2, n3, b8, ... b15
	for (int i = 0; i < 4; i++)
		clsData[i] = libwps::readU32(ip);

	f.str("");
	f << "@@CompObj(CLSID):";
	if (clsData[1] == 0 && clsData[2] == 0xC0 && clsData[3] == 0x46000000L)
	{
		// normally, a referenced object
		char const *clsName = m_compObjIdName->getCLSName(clsData[0]);
		if (clsName)
			f << "'" << clsName << "'";
		else
		{
			WPS_DEBUG_MSG(("WPSOLEParser::readCompObj: unknown clsid=%ld\n", clsData[0]));
			f << "unknCLSID='" << std::hex << clsData[0] << "'";
		}
	}
	else
	{
		/* I found:
		  c1dbcd28e20ace11a29a00aa004a1a72     for MSWorks.Table
		  c2dbcd28e20ace11a29a00aa004a1a72     for Microsoft Works/MSWorksWPDoc
		  a3bcb394c2bd1b10a18306357c795b37     for Microsoft Drawing 1.01/MSDraw.1.01
		  b25aa40e0a9ed111a40700c04fb932ba     for Quill96 Story Group Class ( basic MSWorks doc?)
		  796827ed8bc9d111a75f00c04fb9667b     for MSWorks4Sheet
		*/
		f << "data0=(" << std::hex << clsData[0] << "," << clsData[1] << "), "
		  << "data1=(" << clsData[2] << "," << clsData[3] << ")";
	}
	ascii.addNote(f.str().c_str());
	f << std::dec;
	for (int ch = 0; ch < 3; ch++)
	{
		long actPos = ip->tell();
		long sz = libwps::read32(ip);
		bool waitNumber = sz == -1;
		if (waitNumber) sz = 4;
		if (sz < 0 || ip->seek(actPos+4+sz,WPX_SEEK_SET) != 0 ||
		        ip->tell() != actPos+4+sz) return false;
		ip->seek(actPos+4,WPX_SEEK_SET);

		std::string st;
		if (waitNumber)
		{
			f.str("");
			f << libwps::read32(ip) << "[val*]";
			st = f.str();
		}
		else
		{
			for (int i = 0; i < sz; i++)
				st += char(libwps::readU8(ip));
		}

		f.str("");
		f << "@@CompObj:";
		switch (ch)
		{
		case 0:
			f << "UserType=";
			break;
		case 1:
			f << "ClipName=";
			break;
		case 2:
			f << "ProgIdName=";
			break;
		default:
			break;
		}
		f << st;
		ascii.addPos(actPos);
		ascii.addNote(f.str().c_str());
	}

	if (ip->atEOS()) return true;

	long actPos = ip->tell();
	long nbElt = 4;
	if (ip->seek(actPos+16,WPX_SEEK_SET) != 0 ||
	        ip->tell() != actPos+16)
	{
		if ((ip->tell()-actPos)%4) return false;
		nbElt = (ip->tell()-actPos)/4;
	}

	f.str("");
	f << "@@CompObj(Footer): " << std::hex;
	ip->seek(actPos,WPX_SEEK_SET);
	for (int i = 0; i < nbElt; i++)
		f << libwps::readU32(ip) << ",";
	ascii.addPos(actPos);
	ascii.addNote(f.str().c_str());

	ascii.addPos(ip->tell());

	return true;
}

//////////////////////////////////////////////////
//
// OlePres001 seems to contained standart picture file and size
//    extract the picture if it is possible
//
//////////////////////////////////////////////////

bool WPSOLEParser::isOlePres(WPXInputStreamPtr &ip, std::string const &oleName)
{
	if (!ip.get()) return false;

	if (strncmp("OlePres",oleName.c_str(),7) != 0) return false;

	if (ip->seek(40, WPX_SEEK_SET) != 0 || ip->tell() != 40) return false;

	ip->seek(0, WPX_SEEK_SET);
	for (int i= 0; i < 2; i++)
	{
		long val = libwps::read32(ip);
		if (val < -10 || val > 10) return false;
	}

	long actPos = ip->tell();
	int hSize = libwps::read32(ip);
	if (hSize < 4) return false;
	if (ip->seek(actPos+hSize+28, WPX_SEEK_SET) != 0
	        || ip->tell() != actPos+hSize+28)
		return false;

	ip->seek(actPos+hSize, WPX_SEEK_SET);
	for (int i= 3; i < 7; i++)
	{
		long val = libwps::read32(ip);
		if (val < -10 || val > 10)
		{
			if (i != 5 || val > 256) return false;
		}
	}

	ip->seek(8, WPX_SEEK_CUR);
	long size = libwps::read32(ip);

	if (size <= 0) return ip->atEOS();

	actPos = ip->tell();
	if (ip->seek(actPos+size, WPX_SEEK_SET) != 0
	        || ip->tell() != actPos+size)
		return false;

	return true;
}

bool WPSOLEParser::readOlePres(WPXInputStreamPtr &ip, WPXBinaryData &data, WPSPosition &pos,
                               libwps::DebugFile &ascii)
{
	data.clear();
	if (!isOlePres(ip, "OlePres")) return false;

	pos = WPSPosition();
	pos.setUnit(WPX_POINT);
	libwps::DebugStream f;
	f << "@@OlePress(header): ";
	ip->seek(0,WPX_SEEK_SET);
	for (int i = 0; i < 2; i++)
	{
		long val = libwps::read32(ip);
		f << val << ", ";
	}

	long actPos = ip->tell();
	int hSize = libwps::read32(ip);
	if (hSize < 4) return false;
	f << "hSize = " << hSize;
	ascii.addPos(0);
	ascii.addNote(f.str().c_str());

	long endHPos = actPos+hSize;
	if (hSize > 4)   // CHECKME
	{
		bool ok = true;
		f.str("");
		f << "@@OlePress(headerA): ";
		if (hSize < 14) ok = false;
		else
		{
			// 12,21,32|48,0
			for (int i = 0; i < 4; i++) f << libwps::read16(ip) << ",";
			// 3 name of creator
			for (int ch=0; ch < 3; ch++)
			{
				std::string str;
				bool find = false;
				while (ip->tell() < endHPos)
				{
					unsigned char c = libwps::readU8(ip);
					if (c == 0)
					{
						find = true;
						break;
					}
					str += char(c);
				}
				if (!find)
				{
					ok = false;
					break;
				}
				f << ", name" <<  ch << "=" << str;
			}
			if (ok) ok = ip->tell() == endHPos;
		}
		// FIXME, normally they remain only a few bits (size unknown)
		if (!ok) f << "###";
		ascii.addPos(actPos);
		ascii.addNote(f.str().c_str());
	}
	if (ip->seek(endHPos+28, WPX_SEEK_SET) != 0
	        || ip->tell() != endHPos+28)
		return false;

	ip->seek(endHPos, WPX_SEEK_SET);

	actPos = ip->tell();
	f.str("");
	f << "@@OlePress(headerB): ";
	for (int i = 3; i < 7; i++)
	{
		long val = libwps::read32(ip);
		f << val << ", ";
	}
	// dim in TWIP ?
	long extendX = libwps::readU32(ip);
	long extendY = libwps::readU32(ip);
	if (extendX > 0 && extendY > 0)
		pos.setNaturalSize(Vec2f(float(extendX)/20.f, float(extendY)/20.f));
	long fSize = libwps::read32(ip);
	f << "extendX="<< extendX << ", extendY=" << extendY << ", fSize=" << fSize;

	ascii.addPos(actPos);
	ascii.addNote(f.str().c_str());

	if (fSize == 0) return ip->atEOS();

	data.clear();
	if (!libwps::readData(ip, (unsigned long)fSize, data)) return false;

	if (!ip->atEOS())
	{
		ascii.addPos(ip->tell());
		ascii.addNote("@@OlePress###");
	}

	ascii.skipZone(36+hSize,36+hSize+fSize-1);
	return true;
}

//////////////////////////////////////////////////
//
//  Ole10Native: basic Windows picture, with no size
//          - in general used to store a bitmap
//
//////////////////////////////////////////////////

bool WPSOLEParser::isOle10Native(WPXInputStreamPtr &ip, std::string const &oleName)
{
	if (strncmp("Ole10Native",oleName.c_str(),11) != 0) return false;

	if (ip->seek(4, WPX_SEEK_SET) != 0 || ip->tell() != 4) return false;

	ip->seek(0, WPX_SEEK_SET);
	long size = libwps::read32(ip);

	if (size <= 0) return false;
	if (ip->seek(4+size, WPX_SEEK_SET) != 0 || ip->tell() != 4+size)
		return false;

	return true;
}

bool WPSOLEParser::readOle10Native(WPXInputStreamPtr &ip,
                                   WPXBinaryData &data,
                                   libwps::DebugFile &ascii)
{
	if (!isOle10Native(ip, "Ole10Native")) return false;

	libwps::DebugStream f;
	f << "@@Ole10Native(Header): ";
	ip->seek(0,WPX_SEEK_SET);
	long fSize = libwps::read32(ip);
	f << "fSize=" << fSize;

	ascii.addPos(0);
	ascii.addNote(f.str().c_str());

	data.clear();
	if (!libwps::readData(ip, (unsigned long) fSize, data)) return false;

	if (!ip->atEOS())
	{
		ascii.addPos(ip->tell());
		ascii.addNote("@@Ole10Native###");
	}
	ascii.skipZone(4,4+fSize-1);
	return true;
}

////////////////////////////////////////////////////////////////
//
// In general a picture : a PNG, an JPEG, a basic metafile,
//    find also a MSDraw.1.01 picture (with first bytes 0x78563412="xV4") or WordArt,
//    ( with first bytes "WordArt" )  which are not sucefull read
//    (can probably contain a list of data, but do not know how to
//     detect that)
//
// To check: does this is related to MSO_BLIPTYPE ?
//        or OO/filter/sources/msfilter/msdffimp.cxx ?
//
////////////////////////////////////////////////////////////////
bool WPSOLEParser::readContents(WPXInputStreamPtr &input,
                                std::string const &oleName,
                                WPXBinaryData &pict, WPSPosition &pos,
                                libwps::DebugFile &ascii)
{
	pict.clear();
	if (strcmp(oleName.c_str(),"Contents") != 0) return false;

	libwps::DebugStream f;
	pos = WPSPosition();
	pos.setUnit(WPX_POINT);
	input->seek(0, WPX_SEEK_SET);
	f << "@@Contents:";

	bool ok = true;
	// bdbox 0 : size in the file ?
	int dim[2];
	dim[0] = libwps::read32(input);
	dim[1] = libwps::read32(input);
	f << "bdbox0=(" << dim[0] << "," << dim[1]<<"),";
	for (int i = 0; i < 3; i++)
	{
		/* 0,{10|21|75|101|116}x2 */
		long val = libwps::readU32(input);
		if (val < 1000)
			f << val << ",";
		else
			f << std::hex << "0x" << val << std::dec << ",";
		if (val > 0x10000) ok=false;
	}
	// new bdbox : size of the picture ?
	int naturalSize[2];
	naturalSize[0] = libwps::read32(input);
	naturalSize[1] = libwps::read32(input);
	f << std::dec << "bdbox1=(" << naturalSize[0] << "," << naturalSize[1]<<"),";
	f << "unk=" << libwps::readU32(input) << ","; // 24 or 32
	if (input->atEOS())
	{
		WPS_DEBUG_MSG(("WPSOLEParser: warning: Contents header length\n"));
		return false;
	}
	if (dim[0] > 0 && dim[0] < 3000 &&
	        dim[1] > 0 && dim[1] < 3000)
		pos.setSize(Vec2f(float(dim[0]),float(dim[1])));
	else
	{
		WPS_DEBUG_MSG(("WPSOLEParser: warning: Contents odd size : %d %d\n", dim[0], dim[1]));
	}
	if (naturalSize[0] > 0 && naturalSize[0] < 5000 &&
	        naturalSize[1] > 0 && naturalSize[1] < 5000)
		pos.setNaturalSize(Vec2f(float(naturalSize[0]),float(naturalSize[1])));
	else
	{
		WPS_DEBUG_MSG(("WPSOLEParser: warning: Contents odd naturalsize : %d %d\n", naturalSize[0], naturalSize[1]));
	}

	long actPos = input->tell();
	long size = libwps::readU32(input);
	if (size <= 0) ok = false;
	if (ok)
	{
		input->seek(actPos+size+4, WPX_SEEK_SET);
		if (input->tell() != actPos+size+4 || !input->atEOS())
		{
			ok = false;
			WPS_DEBUG_MSG(("WPSOLEParser: warning: Contents unexpected file size=%ld\n",
			               size));
		}
	}

	if (!ok) f << "###";
	f << "dataSize=" << size;

	ascii.addPos(0);
	ascii.addNote(f.str().c_str());

	input->seek(actPos+4, WPX_SEEK_SET);

	if (ok)
	{
		if (libwps::readData(input,(unsigned long) size, pict))
			ascii.skipZone(actPos+4, actPos+size+4-1);
		else
		{
			input->seek(actPos+4, WPX_SEEK_SET);
			ok = false;
		}
	}

	if (!input->atEOS())
	{
		ascii.addPos(actPos);
		ascii.addNote("@@Contents:###");
	}

	if (!ok)
	{
		WPS_DEBUG_MSG(("WPSOLEParser: warning: read ole Contents: failed\n"));
	}
	return ok;
}

////////////////////////////////////////////////////////////////
//
// Another different type of contents (this time in majuscule)
// we seem to contain the header of a EMF and then the EMF file
//
////////////////////////////////////////////////////////////////
bool WPSOLEParser::readCONTENTS(WPXInputStreamPtr &input,
                                std::string const &oleName,
                                WPXBinaryData &pict, WPSPosition &pos,
                                libwps::DebugFile &ascii)
{
	pict.clear();
	if (strcmp(oleName.c_str(),"CONTENTS") != 0) return false;

	libwps::DebugStream f;

	pos = WPSPosition();
	pos.setUnit(WPX_POINT);
	input->seek(0, WPX_SEEK_SET);
	f << "@@CONTENTS:";

	long hSize = libwps::readU32(input);
	if (input->atEOS()) return false;
	f << "hSize=" << std::hex << hSize << std::dec;

	if (hSize <= 52 || input->seek(hSize+8,WPX_SEEK_SET) != 0
	        || input->tell() != hSize+8)
	{
		WPS_DEBUG_MSG(("WPSOLEParser: warning: CONTENTS headerSize=%ld\n",
		               hSize));
		return false;
	}

	// minimal checking of the "copied" header
	input->seek(4, WPX_SEEK_SET);
	long type = libwps::readU32(input);
	if (type < 0 || type > 4) return false;
	long newSize = libwps::readU32(input);

	f << ", type=" << type;
	if (newSize < 8) return false;

	if (newSize != hSize) // can sometimes happen, pb after a conversion ?
		f << ", ###newSize=" << std::hex << newSize << std::dec;

	// checkme: two bdbox, in document then data : units ?
	//     Maybe first in POINT, second in TWIP ?
	for (int st = 0; st < 2 ; st++)
	{
		int dim[4];
		for (int i = 0; i < 4; i++) dim[i] = libwps::read32(input);

		bool ok = dim[0] >= 0 && dim[2] > dim[0] && dim[1] >= 0 && dim[3] > dim[2];
		if (ok && st==0) pos.setNaturalSize(Vec2f(float(dim[2]-dim[0]), float(dim[3]-dim[1])));
		if (st==0) f << ", bdbox(Text)";
		else f << ", bdbox(Data)";
		if (!ok) f << "###";
		f << "=(" << dim[0] << "x" << dim[1] << "<->" << dim[2] << "x" << dim[3] << ")";
	}
	char dataType[5];
	for (int i = 0; i < 4; i++) dataType[i] = (char) libwps::readU8(input);
	dataType[4] = '\0';
	f << ",typ=\""<<dataType<<"\""; // always " EMF" ?

	for (int i = 0; i < 2; i++)   // always id0=0, id1=1 ?
	{
		int val = libwps::readU16(input);
		if (val) f << ",id"<< i << "=" << val;
	}
	long dataLength = libwps::readU32(input);
	f << ",length=" << dataLength+hSize;

	ascii.addPos(0);
	ascii.addNote(f.str().c_str());

	ascii.addPos(input->tell());
	f.str("");
	f << "@@CONTENTS(2)";
	for (int i = 0; i < 12 && 4*i+52 < hSize; i++)
	{
		// f0=7,f1=1,f5=500,f6=320,f7=1c4,f8=11a
		// or f0=a,f1=1,f2=2,f3=6c,f5=480,f6=360,f7=140,f8=f0
		// or f0=61,f1=1,f2=2,f3=58,f5=280,f6=1e0,f7=a9,f8=7f
		// f3=some header sub size ? f5/f6 and f7/f8 two other bdbox ?
		long val = libwps::readU32(input);
		if (val) f << std::hex << ",f" << i << "=" << val;
	}
	for (int i = 0; 2*i+100 < hSize; i++)
	{
		// g0=e3e3,g1=6,g2=4e6e,g3=4
		// g0=e200,g1=4,g2=a980,g3=3,g4=4c,g5=50
		// ---
		long val = libwps::readU16(input);
		if (val) f << std::hex << ",g" << i << "=" << val;
	}
	ascii.addNote(f.str().c_str());

	if (dataLength <= 0 || input->seek(hSize+4+dataLength,WPX_SEEK_SET) != 0
	        || input->tell() != hSize+4+dataLength || !input->atEOS())
	{
		WPS_DEBUG_MSG(("WPSOLEParser: warning: CONTENTS unexpected file length=%ld\n",
		               dataLength));
		return false;
	}

	input->seek(4+hSize, WPX_SEEK_SET);
	if (!libwps::readDataToEnd(input, pict)) return false;

	ascii.skipZone(hSize+4, input->tell()-1);
	return true;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
