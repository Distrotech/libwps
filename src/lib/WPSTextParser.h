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

#ifndef WPS_TEXT_PARSER_H
#define WPS_TEXT_PARSER_H

#include <vector>

#include "libwps_internal.h"

#include "WPSDebug.h"
#include "WPSEntry.h"

class WPSParser;

/** class used to defined the ancestor of parser which manages the text data */
class WPSTextParser
{
public:
	//! virtual destructor
	virtual ~WPSTextParser();

	//! returns the file version
	int version() const;

	//! returns the actual input
	WPXInputStreamPtr &getInput()
	{
		return m_input;
	}

protected:
	//! constructor
	WPSTextParser(WPSParser &parser, WPXInputStreamPtr &input);

	//! returns the map type->entry
	std::multimap<std::string, WPSEntry> &getNameEntryMap();

	//! returns the map type->entry
	std::multimap<std::string, WPSEntry> const &getNameEntryMap() const;

protected:
	//! structure which retrieves data information which correspond to a text position
	struct DataFOD
	{
		/** different type which can be associated to a text position
		 *
		 * ATTR_TEXT: all text attributes (font, size, ...)
		 * ATTR_PARAG: all paragraph attributes (margin, tabs, ...)
		 * ATTR_PLC: other attribute (note, fields ... )
		 */
		enum Type { ATTR_TEXT, ATTR_PARAG, ATTR_PLC, ATTR_UNKN };

		//! the constructor
		DataFOD() : m_type(ATTR_UNKN), m_pos(-1), m_defPos(0), m_id(-1) {}

		//! the type of the attribute
		Type m_type;
		//! the offset position of the text modified by this attribute
		long m_pos;
		//! the offset position of the definition of the attribute in the file
		long m_defPos;
		//! an identificator (which must be unique by category)
		int m_id;
	};

	/** function which takes two sorted list of attribute (by text position).
	    \return a list of attribute */
	std::vector<DataFOD> mergeSortedFODLists
	(std::vector<DataFOD> const &lst1, std::vector<DataFOD> const &lst2) const;

	/** callback when a new attribute is found in an FDPP/FDPC entry
	 *
	 * \param input, endPos: defined the zone in the file
	 * \return true and filled id if this attribute can be parsed
	 * \note mess can be filled to add a message in debugFile */
	typedef bool (WPSTextParser::* FDPParser) (long endPos,
	        int &id, std::string &mess);

	/** parses a FDPP or a FDPC entry (which contains a list of ATTR_TEXT/ATTR_PARAG
	 * with their definition ) and adds found data in listFODs */
	bool readFDP(WPSEntry const &entry,
	             std::vector<DataFOD> &fods, FDPParser parser);

protected:
	//! a DebugFile used to write what we recognize when we parse the document
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}

private:
	//! private copy constructor: forbidden
	WPSTextParser(WPSTextParser const &parser );
	//! private copy operator: forbidden
	WPSTextParser &operator=(WPSTextParser const &parser);

protected:
	//! the file version
	mutable int m_version;
	//! the main input
	WPXInputStreamPtr m_input;
	//! pointer to the main zone parser;
	WPSParser &m_mainParser;
	//! an entry which corresponds to the complete text zone
	WPSEntry m_textPositions;
	//! the list of a FOD
	std::vector<DataFOD> m_FODList;
	//! the ascii file
	libwps::DebugFile &m_asciiFile;
};


#endif /* WPSTEXTPARSER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */