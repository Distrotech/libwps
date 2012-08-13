/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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
 */

#ifndef WPS8_TEXTSTYLE_H
#define WPS8_TEXTSTYLE_H

#include <vector>

#include "libwps_internal.h"

#include "WPSDebug.h"

class WPSEntry;

typedef class WPSContentListener WPS8ContentListener;
typedef shared_ptr<WPS8ContentListener> WPS8ContentListenerPtr;

namespace WPS8Struct
{
struct FileData;
}

namespace WPS8TextStyleInternal
{
struct State;
}

class WPS8Text;

class WPS8TextStyle
{
	friend class WPS8Text;
public:
	WPS8TextStyle(WPS8Text &parser);
	~WPS8TextStyle();

	//! sets the listener
	void setListener(WPS8ContentListenerPtr &listen)
	{
		m_listener = listen;
	}

	//! finds all entries which correspond to the styles, parses them and stores data
	bool readStructures();

protected:
	//! reads the font names
	bool readFontNames(WPSEntry const &entry);
	//! reads a font properties
	bool readFont(long endPos, int &id, std::string &mess);

	void sendFont(int fId, uint16_t &specialCode, int &fieldType);

	//! the paragraph
	bool readParagraph(long endPos, int &id, std::string &mess);

	void sendParagraph(int pId);

	/** \brief reads a style sheet zone
		\warning the read data are NOT used*/
	bool readSTSH(WPSEntry const &entry);

	/** \brief parses a SGP zone: style general property ?*/
	bool readSGP(WPSEntry const &entry);

	//----------------------------------------
	// FDP parsing
	//----------------------------------------

	/** finds the FDPC/FDPP structure using the BTEC/BTEP entries
		which == 0 means FDPP, 1 means FDPC */
	bool findFDPStructures(int which, std::vector<WPSEntry> &result);
	/** finds the FDPC/FDPP structure by searching after the text zone
		which == 0 means FDPP, 1 means FDPC */
	bool findFDPStructuresByHand(int which, std::vector<WPSEntry> &result);

protected:
	//! a DebugFile used to write what we recognize when we parse the document
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}

private:
	//! private copy constructor: forbidden
	WPS8TextStyle(WPS8TextStyle const &orig );
	//! private copy operator: forbidden
	WPS8TextStyle &operator=(WPS8TextStyle const &orig);

protected:
	//! the main parser
	WPS8Text &m_mainParser;
	//! the main input
	WPXInputStreamPtr m_input;
	//! the listener
	WPS8ContentListenerPtr m_listener;
	//! the internal state
	mutable shared_ptr<WPS8TextStyleInternal::State> m_state;
	//! the ascii file
	libwps::DebugFile &m_asciiFile;
protected:
};


#endif /* WPS8_TEXT_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
