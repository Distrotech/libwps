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
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 */

#ifndef WPS8_TEXTSTYLE_H
#define WPS8_TEXTSTYLE_H

#include <ostream>
#include <string>
#include <vector>

#include "libwps_internal.h"

#include "WPSDebug.h"

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
	struct FontData;

	WPS8TextStyle(WPS8Text &parser);
	~WPS8TextStyle();

	//! sets the listener
	void setListener(WPSContentListenerPtr &listen)
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

	void sendFont(int fId, FontData &data);

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
	WPS8TextStyle(WPS8TextStyle const &orig);
	//! private copy operator: forbidden
	WPS8TextStyle &operator=(WPS8TextStyle const &orig);

protected:
	//! the main parser
	WPS8Text &m_mainParser;
	//! the main input
	RVNGInputStreamPtr m_input;
	//! the listener
	WPSContentListenerPtr m_listener;
	//! the internal state
	mutable shared_ptr<WPS8TextStyleInternal::State> m_state;
	//! the ascii file
	libwps::DebugFile &m_asciiFile;
public:
	struct FontData
	{
		FontData() : m_type(T_None), m_fieldType(F_None), m_fieldFormat(0) {}
		//! operator <<
		friend std::ostream &operator<<(std::ostream &o, FontData const &fData);
		//! returns a format to used with strftime
		std::string format() const;

		enum { T_None=0, T_Object=2, T_Footnote=3, T_Endnote=4, T_Field=5, T_Comment=6 };
		/** the main type: footnote, ... */
		int m_type;
		enum { F_None=0, F_PageNumber=-1, F_Date=-4, F_Time=-5 };
		/** the field type: pagenumber, data, time, ... */
		int m_fieldType;
		/** the field format */
		int m_fieldFormat;
	};
protected:
};


#endif /* WPS8_TEXT_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
