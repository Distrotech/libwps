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

#ifndef WPS8_H
#define WPS8_H

#include <vector>
#include <list>
#include <map>
#include <libwpd/WPXString.h>

#include "libwps_internal.h"
#include "WPS.h"
#include "WPSContentListener.h"
#include "WPSStream.h"
#include "WPSParser.h"

/**
 * Starting near beginning of CONTENTS stream, there is an index
 * to various types of pages in the document.
 *
 */
class HeaderIndexEntries
{
public:
	uint32_t offset;
	uint32_t length;
};

typedef std::multimap <std::string, HeaderIndexEntries> HeaderIndexMultiMap; /* string is name */

class WPS8ContentListener : public WPSContentListener
{
public:
	WPS8ContentListener(std::list<WPSPageSpan> &pageList, WPXHLListenerImpl *listenerImpl);
	~WPS8ContentListener();

	void attributeChange(const bool isOn, const uint8_t attribute);

private:
	WPS8ContentListener(const WPS8ContentListener&);
	WPS8ContentListener& operator=(const WPS8ContentListener&);
};

class WPS8Parser : public WPSParser
{
public:
	WPS8Parser(WPSInputStream *input, WPSHeader * header);
	~WPS8Parser();

	void parse(WPXHLListenerImpl *listenerImpl);
private:
	void readFontsTable(WPSInputStream * input);
	void appendUTF16LE(WPSInputStream *input, WPS8ContentListener *listener);
	void readText(WPSInputStream * input, WPS8ContentListener *listener);
	bool readFODPage(WPSInputStream * input, std::vector<FOD> * FODs, uint16_t page_size);
	void parseHeaderIndexEntry(WPSInputStream * input);
	void parseHeaderIndex(WPSInputStream * input);
	void parsePages(std::list<WPSPageSpan> &pageList, WPSInputStream *input);
	void parse(WPSInputStream *stream, WPS8ContentListener *listener);
	void propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS8ContentListener *listener);
	void propertyChangeDelta(uint32_t newTextAttributeBits, WPS8ContentListener *listener);
	void propertyChange(std::string rgchProp, WPS8ContentListener *listener);
	uint32_t offset_eot; /* stream offset to end of text */
	uint32_t oldTextAttributeBits;
	HeaderIndexMultiMap headerIndexTable;
	std::vector<FOD> CHFODs; /* CHaracter FOrmatting Descriptors */		
	std::vector<std::string> fonts;
};


#endif /* WPS8_H */
