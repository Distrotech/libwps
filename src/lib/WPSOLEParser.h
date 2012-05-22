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
 * ------------------------------------------------------------
 */

#ifndef WPS_OLE_PARSER_H
#define WPS_OLE_PARSER_H

#include <string>
#include <vector>

#include <libwpd-stream/WPXStream.h>

#include "libwps_internal.h"

#include "WPSDebug.h"

class WPXBinaryData;

namespace libwps
{
class Storage;
}
class WPSPosition;
namespace WPSOLEParserInternal
{
class CompObj;
}

/** \brief a class used to parse some basic oles
    Tries to read the different ole parts and stores their contents in form of picture.
 */
class WPSOLEParser
{
public:
	/** constructor
	    \param mainName: name of the main ole, we must avoid to parse */
	WPSOLEParser(const std::string &mainName);

	/** destructor */
	~WPSOLEParser();

	/** tries to parse basic OLE (excepted mainName)
	    \return false if fileInput is not an Ole file */
	bool parse(shared_ptr<libwps::Storage> fileInput);

	//! returns the list of unknown ole
	std::vector<std::string> const &getNotParse() const
	{
		return m_unknownOLEs;
	}

	//! returns the list of id for which we have find a representation
	std::vector<int> const &getObjectsId() const
	{
		return m_objectsId;
	}
	//! returns the list of data positions which have been read
	std::vector<WPSPosition> const &getObjectsPosition() const
	{
		return m_objectsPosition;
	}
	//! returns the list of data which have been read
	std::vector<WPXBinaryData> const &getObjects() const
	{
		return m_objects;
	}

	//! returns the picture corresponding to an id
	bool getObject(int id, WPXBinaryData &obj, WPSPosition &pos) const;

	/*! \brief sets an object
	 * just in case, the external parsing find another representation
	 */
	void setObject(int id, WPXBinaryData const &obj, WPSPosition const &pos);
protected:

	//!  the "Ole" small structure : unknown contain
	bool readOle(WPXInputStreamPtr &ip, std::string const &oleName,
	             libwps::DebugFile &ascii);
	//!  the "MM" small structure : seems to contain the file versions
	bool readMM(WPXInputStreamPtr &input, std::string const &oleName,
	            libwps::DebugFile &ascii);
	//!  the "ObjInfo" small structure : seems to contain 3 ints=0,3,4
	bool readObjInfo(WPXInputStreamPtr &input, std::string const &oleName,
	                 libwps::DebugFile &ascii);
	//!  the "CompObj" contains : UserType,ClipName,ProgIdName
	bool readCompObj(WPXInputStreamPtr &ip, std::string const &oleName,
	                 libwps::DebugFile &ascii);

	/** the OlePres001 seems to contain standart picture file and size */
	bool isOlePres(WPXInputStreamPtr &ip, std::string const &oleName);
	/** extracts the picture of OlePres001 if it is possible */
	bool readOlePres(WPXInputStreamPtr &ip, WPXBinaryData &data, WPSPosition &pos,
	                 libwps::DebugFile &ascii);

	//! theOle10Native : basic Windows© picture, with no size
	bool isOle10Native(WPXInputStreamPtr &ip, std::string const &oleName);
	/** extracts the picture if it is possible */
	bool readOle10Native(WPXInputStreamPtr &ip, WPXBinaryData &data,
	                     libwps::DebugFile &ascii);

	/** \brief the Contents : in general a picture : a PNG, an JPEG, a basic metafile,
	 * I find also a Word art picture, which are not sucefully read
	 */
	bool readContents(WPXInputStreamPtr &input, std::string const &oleName,
	                  WPXBinaryData &pict, WPSPosition &pos, libwps::DebugFile &ascii);

	/** the CONTENTS : seems to store a header size, the header
	 * and then a object in EMF (with the same header)...
	 * \note I only find such lib in 2 files, so the parsing may be incomplete
	 *  and many such Ole rejected
	 */
	bool readCONTENTS(WPXInputStreamPtr &input, std::string const &oleName,
	                  WPXBinaryData &pict, WPSPosition &pos, libwps::DebugFile &ascii);


	//! if filled, does not parse content with this name
	std::string m_avoidOLE;
	//! list of ole which can not be parsed
	std::vector<std::string> m_unknownOLEs;

	//! list of pictures read
	std::vector<WPXBinaryData> m_objects;
	//! list of picture size ( if known)
	std::vector<WPSPosition> m_objectsPosition;
	//! list of pictures id
	std::vector<int> m_objectsId;

	//! a smart ptr used to stored the list of compobj id->name
	shared_ptr<WPSOLEParserInternal::CompObj> m_compObjIdName;

};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
