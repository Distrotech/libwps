/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwpd
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

#ifndef WPS4_H
#define WPS4_H

#include <vector>
#include <map>

#include "libwps_internal.h"
#include "WPS.h"
#include "WPSContentListener.h"
#include "WPSHeader.h"
#include <libwpd-stream/WPXStream.h>
#include <libwpd/WPXString.h>
#include "WPSParser.h"

typedef WPSContentListener WPS4ContentListener;

namespace WPS4ParserInternal
{
struct FontName;
}

class WPS4Parser : public WPSParser
{
public:
	WPS4Parser(WPXInputStreamPtr &input, WPSHeaderPtr &header);
	~WPS4Parser();

	void parse(WPXDocumentInterface *documentInterface);
private:
	void parsePages(std::list<WPSPageSpan> &pageList, WPXInputStream *input);
	void parse(WPXInputStream *stream, WPS4ContentListener *listener);
	void readFontsTable(WPXInputStream *input);
	bool readFODPage(WPXInputStream *input, std::vector<WPSFOD> * FODs);
	void propertyChangeDelta(uint32_t newTextAttributeBits, WPS4ContentListener *listener);
	void propertyChange(std::string rgchProp, WPS4ContentListener *listener);
	void propertyChangePara(std::string rgchProp, WPS4ContentListener *listener);
	void appendCP(const uint8_t readVal, int codepage, WPS4ContentListener *listener);
	void appendUCS(const uint16_t readVal, WPS4ContentListener *listener);
	void appendCP850(const uint8_t readVal, WPS4ContentListener *listener);
	void appendCP1252(const uint8_t readVal, WPS4ContentListener *listener);
	void readText(WPXInputStream *input, WPS4ContentListener *listener);

#ifdef DEBUG
	static std::string to_bits(std::string s);
#endif
	uint32_t m_oldTextAttributeBits;
	uint32_t m_offset_eot; /* stream offset to end of text */
	std::vector<WPSFOD> m_CHFODs; /* CHaracter FOrmatting Descriptors */
	std::vector<WPSFOD> m_PAFODs; /* PAragraph FOrmatting Descriptors */
	std::map<uint8_t, WPS4ParserInternal::FontName> m_fonts; /* fonts in format <index code, <font name, codepage>>  */
	const uint8_t m_worksVersion;
};

#endif /* WPS4_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
