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

class WPS8Listener;


class WPS8PrefixDataPacket
{
public:
	WPS8PrefixDataPacket(WPXInputStream * input);	
	virtual ~WPS8PrefixDataPacket() {}
	virtual void parse(WPS8Listener *listener) const {}

protected:
	virtual void _readContents(WPXInputStream *input) = 0;
 	void _read(WPXInputStream *input, uint32_t dataOffset, uint32_t dataSize);
};

class WPS8Listener
{
public:
	WPS8Listener();
	virtual ~WPS8Listener() {}

	virtual void startDocument() = 0;
	virtual void insertCharacter(const uint16_t character) = 0;
	virtual void insertTab(const uint8_t tabType, float tabPosition) = 0;
	virtual void insertBreak(const uint8_t breakType) = 0;
	virtual void insertEOL() = 0;
	virtual void lineSpacingChange(const float lineSpacing) = 0;
	virtual void attributeChange(const bool isOn, const uint8_t attribute) = 0;
	virtual void pageMarginChange(const uint8_t side, const uint16_t margin) = 0;
	virtual void pageFormChange(const uint16_t length, const uint16_t width, const WPSFormOrientation orientation, const bool isPersistent) = 0;
	virtual void endDocument() = 0;

	virtual void setTextFont(const WPXString fontName) = 0;
	virtual void setFontSize(const uint16_t fontSize) = 0;
};

typedef std::multimap <std::string, HeaderIndexEntries> HeaderIndexMultiMap; /* string is name */

class WPS8Parser : public WPSParser
{
public:
	WPS8Parser(WPXInputStream *input, WPSHeader * header);
	~WPS8Parser();

	void parse(WPXDocumentInterface *documentInterface);
private:
	void readFontsTable(WPXInputStream * input);
	void appendUTF16LE(WPXInputStream *input, WPS8Listener *listener);
	void readText(WPXInputStream * input, WPS8Listener *listener);
	bool readFODPage(WPXInputStream * input, std::vector<FOD> * FODs, uint16_t page_size);
	void parseHeaderIndexEntry(WPXInputStream * input);
	void parseHeaderIndex(WPXInputStream * input);
	void parsePages(std::list<WPSPageSpan> &pageList, WPXInputStream *input);
	void parse(WPXInputStream *stream, WPS8Listener *listener);
	void propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS8Listener *listener);
	void propertyChangeDelta(uint32_t newTextAttributeBits, WPS8Listener *listener);
	void propertyChange(std::string rgchProp, WPS8Listener *listener);
	uint32_t offset_eot; /* stream offset to end of text */
	uint32_t oldTextAttributeBits;
	HeaderIndexMultiMap headerIndexTable;
	std::vector<FOD> CHFODs; /* CHaracter FOrmatting Descriptors */		
	std::vector<std::string> fonts;
};


typedef struct _WPS8ContentParsingState WPS8ContentParsingState;
struct _WPS8ContentParsingState
{
	_WPS8ContentParsingState();
	~_WPS8ContentParsingState();
	WPXString m_textBuffer;
};


class WPS8ContentListener : public WPS8Listener, protected WPSContentListener
{
public:
	WPS8ContentListener(std::list<WPSPageSpan> &pageList, WPXDocumentInterface *documentInterface);
	~WPS8ContentListener();

	void startDocument() { WPSContentListener::startDocument(); };
	void insertCharacter(const uint16_t character);
	void insertTab(const uint8_t tabType, float tabPosition);
	void insertBreak(const uint8_t breakType) { WPSContentListener::insertBreak(breakType); };
	void insertEOL();
	void attributeChange(const bool isOn, const uint8_t attribute);
	void lineSpacingChange(const float lineSpacing) { WPSContentListener::lineSpacingChange(lineSpacing); };
	void pageMarginChange(const uint8_t side, const uint16_t margin) {};
	void pageFormChange(const uint16_t length, const uint16_t width, const WPSFormOrientation orientation, const bool isPersistent) {};
	void endDocument() { WPSContentListener::endDocument(); };

	void setTextFont(const WPXString fontName);
	void setFontSize(const uint16_t fontSize);
	void insertPageNumber(const WPXString &pageNumber);
	void insertNoteReference(const WPXString &noteReference);
	void suppressPage(const uint16_t suppressCode);
	
protected:
	void _openParagraph();

	void _flushText();
	void _changeList() {};
	
private:
	WPS8ContentParsingState *m_parseState;	
};

#endif /* WPS8_H */
