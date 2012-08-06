/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
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

#ifndef WPSPARSER_H
#define WPSPARSER_H

#include <map>
#include <string>

#include "libwps_internal.h"

#include "WPSDebug.h"

class WPXDocumentInterface;

class WPSEntry;
class WPSHeader;
typedef shared_ptr<WPSHeader> WPSHeaderPtr;

class WPSTextParser;

class WPSParser
{
	friend class WPSTextParser;
public:
	WPSParser(WPXInputStreamPtr &input, WPSHeaderPtr &header);
	virtual ~WPSParser();

	virtual void parse(WPXDocumentInterface *documentInterface) = 0;

protected:
	//! a map to retrieve a file entry by name
	typedef std::multimap<std::string, WPSEntry> NameMultiMap;

	WPXInputStreamPtr &getInput()
	{
		return m_input;
	}
	WPSHeaderPtr &getHeader()
	{
		return m_header;
	}
	int version() const
	{
		return m_version;
	}
	void setVersion(int vers)
	{
		m_version=vers;
	}
	NameMultiMap &getNameEntryMap()
	{
		return m_nameMultiMap;
	}
	NameMultiMap const &getNameEntryMap() const
	{
		return m_nameMultiMap;
	}
	//! a DebugFile used to write what we recognize when we parse the document
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}

private:
	WPSParser(const WPSParser &);
	WPSParser &operator=(const WPSParser &);

	// the main input
	WPXInputStreamPtr m_input;
	// the header
	WPSHeaderPtr m_header;
	// the file version
	int m_version;
	//! a map to retrieve a file entry by name
	NameMultiMap m_nameMultiMap;
	//! the debug file
	libwps::DebugFile m_asciiFile;
};

#endif /* WPSPARSER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
