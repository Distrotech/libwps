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
#include "WPSStream.h"
#include <libwpd/WPXString.h>
#include "WPSParser.h"


class WPS4ContentListener : public WPSContentListener
{
public:
	WPS4ContentListener(std::list<WPSPageSpan> &pageList, WPXHLListenerImpl *listenerImpl);
	~WPS4ContentListener();

	void attributeChange(const bool isOn, const uint8_t attribute);

private:
	WPS4ContentListener(const WPS4ContentListener&);
	WPS4ContentListener& operator=(const WPS4ContentListener&);
};

class WPS4Parser : public WPSParser
{
public:
	WPS4Parser(WPSInputStream *input, WPSHeader * header);
	~WPS4Parser();

	void parse(WPXHLListenerImpl *listenerImpl);

private:
	void parsePages(std::list<WPSPageSpan> &pageList, WPSInputStream *input);
	void parse(WPSInputStream *stream, WPS4ContentListener *listener);
	void readFontsTable(WPSInputStream * input);
	bool readFODPage(WPSInputStream * input, std::vector<FOD> * FODs);	
	void propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS4ContentListener *listener);
	void propertyChangeDelta(uint32_t newTextAttributeBits, WPS4ContentListener *listener);
	void propertyChange(std::string rgchProp, WPS4ContentListener *listener);
	void appendCP850(const uint8_t readVal, WPS4ContentListener *listener);
	void appendCP1252(const uint8_t readVal, WPS4ContentListener *listener);
	void readText(WPSInputStream * input, WPS4ContentListener *listener);
	uint32_t oldTextAttributeBits;
	uint32_t offset_eot; /* stream offset to end of text */
	uint32_t offset_eos; /* stream offset to end of MN0 */	
	std::vector<FOD> CHFODs; /* CHaracter FOrmatting Descriptors */		
	std::vector<FOD> PAFODs; /* PAragraph FOrmatting Descriptors */			
	std::map<uint8_t, std::string> fonts; /* fonts in format <index code, font name>  */
	const uint8_t m_worksVersion;
};

#endif /* WPS6_H */
