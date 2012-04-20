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
#include <libwpd-stream/WPXStream.h>
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
	WPS8ContentListener(std::list<WPSPageSpan> &pageList, WPXDocumentInterface *documentInterface);
	~WPS8ContentListener();

	void attributeChange(const bool isOn, const uint8_t attribute);

private:
	WPS8ContentListener(const WPS8ContentListener &);
	WPS8ContentListener &operator=(const WPS8ContentListener &);
};

struct WPSRange
{
	uint32_t start;
	uint32_t limit;
	WPSRange() : start(0), limit(0) {}
	~WPSRange() {}
};

struct WPSStream
{
	uint32_t type;
	WPSRange span;
	WPSStream() : type(0), span() {}
	~WPSStream() {}
};

struct WPSNote
{
	uint32_t offset;
	WPSRange contents;
	WPSNote() : offset(0), contents() {}
};

#define WPS_STREAM_DUMMY 0
#define WPS_STREAM_BODY  1
#define WPS_STREAM_FOOTNOTES 2
#define WPS_STREAM_ENDNOTES 3

#define WPS_NUM_SEP_PAR  0x0
#define WPS_NUM_SEP_DOT  0x2

class WPS8Parser : public WPSParser
{
public:
	WPS8Parser(WPXInputStream *input, WPSHeader *header);
	~WPS8Parser();

	void parse(WPXDocumentInterface *documentInterface);
private:
	void readFontsTable(WPXInputStream *input);
	void readStreams(WPXInputStream *input);
	void readNotes(std::vector<WPSNote> &dest, WPXInputStream *input, const char *key);
	void appendUTF16LE(WPXInputStream *input, WPS8ContentListener *listener);
	void readText(WPXInputStream *input, WPS8ContentListener *listener);
	void readTextRange(WPXInputStream *input, WPS8ContentListener *listener, uint32_t startpos, uint32_t endpos, uint16_t stream);
	void readNote(WPXInputStream *input, WPS8ContentListener *listener, bool is_endnote);
	bool readFODPage(WPXInputStream *input, std::vector<FOD> * FODs, uint16_t page_size);
	void parseHeaderIndexEntry(WPXInputStream *input);
	void parseHeaderIndex(WPXInputStream *input);
	void parsePages(std::list<WPSPageSpan> &pageList, WPXInputStream *input);
	void parse(WPXInputStream *stream, WPS8ContentListener *listener);
	void propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS8ContentListener *listener);
	void propertyChangeDelta(uint32_t newTextAttributeBits, WPS8ContentListener *listener);
	void propertyChange(std::string rgchProp, WPS8ContentListener *listener);
	void propertyChangePara(std::string rgchProp, WPS8ContentListener *listener);
	uint32_t offset_eot; /* stream offset to end of text */
	uint32_t oldTextAttributeBits;
	HeaderIndexMultiMap headerIndexTable;
	std::vector<FOD> CHFODs; /* CHaracter FOrmatting Descriptors */
	std::vector<FOD> PAFODs; /* PAragraph FOrmatting Descriptors */
	std::vector<std::string> fonts;
	std::vector<WPSStream> streams;
	std::vector<WPSNote> footnotes;
	std::vector<WPSNote> endnotes;

	std::vector<WPSNote>::iterator fn_iter;
	std::vector<WPSNote>::iterator en_iter;
};


#endif /* WPS8_H */
