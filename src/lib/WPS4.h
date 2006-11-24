/* libwpd
 * Copyright (C) 2006 Andrew Ziem
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

#include "libwpd_internal.h"
#include "WPS.h"
#include "WPXContentListener.h"
#include "WPXHeader.h"
#include "WPSStream.h"
#include <libwpd/WPXString.h>
#include "WPXParser.h"

class WPS4Listener;

class WPS4PrefixDataPacket
{
public:
	WPS4PrefixDataPacket(libwps::WPSInputStream * input);	
	virtual ~WPS4PrefixDataPacket() {}
	virtual void parse(WPS4Listener *listener) const {}

//	static WPS4PrefixDataPacket * constructPrefixDataPacket(libwps::WPSInputStream * input, WPS4PrefixIndice *prefixIndice);

protected:
	virtual void _readContents(libwps::WPSInputStream *input) = 0;
 	void _read(libwps::WPSInputStream *input, uint32_t dataOffset, uint32_t dataSize);
};


class WPS4Listener
{
public:
	WPS4Listener();
	virtual ~WPS4Listener() {}

	virtual void startDocument() = 0;
	virtual void insertCharacter(const uint16_t character) = 0;
	virtual void insertTab(const uint8_t tabType, float tabPosition) = 0;
	virtual void insertBreak(const uint8_t breakType) = 0;
	virtual void insertEOL() = 0;
	virtual void lineSpacingChange(const float lineSpacing) = 0;
	virtual void attributeChange(const bool isOn, const uint8_t attribute) = 0;
	virtual void pageMarginChange(const uint8_t side, const uint16_t margin) = 0;
	virtual void pageFormChange(const uint16_t length, const uint16_t width, const WPXFormOrientation orientation, const bool isPersistent) = 0;
	virtual void marginChange(const uint8_t side, const uint16_t margin) = 0;
	virtual void indentFirstLineChange(const int16_t offset) = 0;
	virtual void columnChange(const WPXTextColumnType columnType, const uint8_t numColumns, const std::vector<float> &columnWidth,
					const std::vector<bool> &isFixedWidth) = 0;
	virtual void endDocument() = 0;

	virtual void undoChange(const uint8_t undoType, const uint16_t undoLevel) = 0;
	virtual void justificationChange(const uint8_t justification) = 0;
	virtual void setTextColor(const RGBSColor * fontColor) = 0;
	virtual void setTextFont(const WPXString fontName) = 0;
	virtual void setFontSize(const uint16_t fontSize) = 0;
	virtual void insertPageNumber(const WPXString &pageNumber) = 0;
	virtual void insertNoteReference(const WPXString &noteReference) = 0;
	virtual void suppressPage(const uint16_t suppressCode) = 0;
};

class WPS4Parser : public WPXParser
{
public:
	WPS4Parser(libwps::WPSInputStream *input, WPXHeader * header);
	~WPS4Parser();

	void parse(WPXHLListenerImpl *listenerImpl);
private:
	void parsePages(std::list<WPXPageSpan> &pageList, libwps::WPSInputStream *input);
	void parse(libwps::WPSInputStream *stream, WPS4Listener *listener);
	void readFontsTable(libwps::WPSInputStream * input);
	bool readFODPage(libwps::WPSInputStream * input, std::vector<FOD> * FODs);	
	void propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS4Listener *listener);
	void propertyChangeDelta(uint32_t newTextAttributeBits, WPS4Listener *listener);
	void propertyChange(std::string rgchProp, WPS4Listener *listener);
	void insertCharacter(iconv_t cd, char readVal, WPS4Listener *listener);
	void readText(libwps::WPSInputStream * input, WPS4Listener *listener);
	uint32_t oldTextAttributeBits;
	uint32_t offset_eot; /* stream offset to end of text */
	uint32_t offset_eos; /* stream offset to end of MN0 */	
	std::vector<FOD> CHFODs; /* CHaracter FOrmatting Descriptors */		
	std::vector<FOD> PAFODs; /* PAragraph FOrmatting Descriptors */			
	std::map<uint8_t, std::string> fonts; /* fonts in format <index code, font name>  */
};


typedef struct _WPS4ContentParsingState WPS4ContentParsingState;
struct _WPS4ContentParsingState
{
	_WPS4ContentParsingState();
	~_WPS4ContentParsingState();
	WPXString m_textBuffer;
};


class WPS4ContentListener : public WPS4Listener, protected WPXContentListener
{
public:
	WPS4ContentListener(std::list<WPXPageSpan> &pageList, WPXHLListenerImpl *listenerImpl);
	~WPS4ContentListener();

	void startDocument() { WPXContentListener::startDocument(); };
	void insertCharacter(const uint16_t character);
	void insertTab(const uint8_t tabType, float tabPosition);
	void insertBreak(const uint8_t breakType) { WPXContentListener::insertBreak(breakType); };
	void insertEOL();
	void attributeChange(const bool isOn, const uint8_t attribute);
	void lineSpacingChange(const float lineSpacing) { WPXContentListener::lineSpacingChange(lineSpacing); };
	void pageMarginChange(const uint8_t side, const uint16_t margin) {};
	void pageFormChange(const uint16_t length, const uint16_t width, const WPXFormOrientation orientation, const bool isPersistent) {};
	void marginChange(const uint8_t side, const uint16_t margin);
	void indentFirstLineChange(const int16_t offset);
	void columnChange(const WPXTextColumnType columnType, const uint8_t numColumns, const std::vector<float> &columnWidth,
					const std::vector<bool> &isFixedWidth);
	void endDocument() { WPXContentListener::endDocument(); };

	void undoChange(const uint8_t undoType, const uint16_t undoLevel);
	void justificationChange(const uint8_t justification);
	void setTextColor(const RGBSColor * fontColor);
	void setTextFont(const WPXString fontName);
	void setFontSize(const uint16_t fontSize);
	void insertPageNumber(const WPXString &pageNumber);
	void insertNoteReference(const WPXString &noteReference);
	void insertNote(const WPXNoteType noteType);
	void suppressPage(const uint16_t suppressCode);
	
protected:
	void _openParagraph();

	void _flushText();
	void _changeList() {};
	
private:
	WPS4ContentParsingState *m_parseState;	
};

#endif /* WPS6_H */
