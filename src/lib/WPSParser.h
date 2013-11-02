/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
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

#ifndef WPSPARSER_H
#define WPSPARSER_H

#include <map>
#include <string>

#include "libwps_internal.h"

#include "WPSDebug.h"

class RVNGTextInterface;

class WPSTextParser;

class WPSParser
{
	friend class WPSTextParser;
public:
	WPSParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header);
	virtual ~WPSParser();

	virtual void parse(RVNGTextInterface *documentInterface) = 0;

	//! a map to retrieve a file entry by name
	typedef std::multimap<std::string, WPSEntry> NameMultiMap;

protected:
	RVNGInputStreamPtr &getInput()
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
	RVNGInputStreamPtr m_input;
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
