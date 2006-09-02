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
#include "WPXContentListener.h"
#include "WPXStream.h"
#include "WPXString.h"
#include "WPXSubDocument.h"
#include "WPXParser.h"

/* character or paragraph formatting */
class FPROP
{
public:
	uint8_t	cch; /* number of bytes in this FPROP */
	//fixme: try WPXString
	std::string rgchProp; /* Prefix for a CHP or PAP sufficient to handle differing bits from default */
};

/**
 * FOrmatting Descriptors
 */
class FOD
{
public:
	FOD() { fcLim = bfprop = bfprop_abs = 0; }
	uint32_t	fcLim; /* byte number of last character covered by this FOD */
	uint8_t		bfprop; /* byte offset from beginning of FOD array to corresponding FPROP */
	uint32_t        bfprop_abs; /* bfprop from beginning of stream offset */
	FPROP		fprop;	/* character or paragraph formatting */
};

enum WPS4OutlineLocation { paragraphGroup, indexHeader };

class WPS4Listener;

class WPS4SubDocument : public WPXSubDocument
{
public:
	WPS4SubDocument(uint8_t * streamData, const int dataSize);
	void parse(WPS4Listener *listener) const;
};

class WPS4PrefixDataPacket
{
public:
	WPS4PrefixDataPacket(WPXInputStream * input);	
	virtual ~WPS4PrefixDataPacket() {}
	virtual void parse(WPS4Listener *listener) const {}
	virtual WPS4SubDocument * getSubDocument() const { return NULL; }

//	static WPS4PrefixDataPacket * constructPrefixDataPacket(WPXInputStream * input, WPS4PrefixIndice *prefixIndice);

protected:
	virtual void _readContents(WPXInputStream *input) = 0;
 	void _read(WPXInputStream *input, uint32_t dataOffset, uint32_t dataSize);
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

	virtual void defineTable(const uint8_t position, const uint16_t leftOffset) = 0;
	virtual void addTableColumnDefinition(const uint32_t width, const uint32_t leftGutter, const uint32_t rightGutter,
					const uint32_t attributes, const uint8_t alignment) = 0;
	virtual void startTable() = 0;
 	virtual void closeCell() = 0;
	virtual void closeRow() = 0;
	virtual void setTableCellSpan(const uint16_t colSpan, const uint16_t rowSpan) = 0;
	virtual void setTableCellFillColor(const RGBSColor * cellFillColor) = 0;
 	virtual void endTable() = 0;
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
	WPS4Parser(WPXInputStream *input, WPXHeader * header);
	~WPS4Parser();

	void parse(WPXHLListenerImpl *listenerImpl);
private:
	void parsePages(std::list<WPXPageSpan> &pageList, WPXInputStream *input);
	void parse(WPXInputStream *stream, WPS4Listener *listener);
	void readFontsTable(WPXInputStream * input);
	bool readFODPage(WPXInputStream * input, std::vector<FOD>  * FODs);	
	void propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS4Listener *listener);
	void propertyChangeDelta(uint32_t newTextAttributeBits, WPS4Listener *listener);
	void propertyChange(std::string rgchProp, WPS4Listener *listener);
	void readText(WPXInputStream * input, WPS4Listener *listener);
	uint32_t oldTextAttributeBits;
	uint32_t stream_length; /* total stream length, for sanity checking */
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
	WPS4ContentListener(std::list<WPXPageSpan> &pageList, std::vector<WPS4SubDocument *> &subDocuments, WPXHLListenerImpl *listenerImpl);
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

	void defineTable(const uint8_t position, const uint16_t leftOffset);
	void addTableColumnDefinition(const uint32_t width, const uint32_t leftGutter, const uint32_t rightGutter,
					const uint32_t attributes, const uint8_t alignment);
	void startTable();
 	void insertRow();
 	void insertCell();
 	void closeCell();
	void closeRow();
	void setTableCellSpan(const uint16_t colSpan, const uint16_t rowSpan);
	void setTableCellFillColor(const RGBSColor * cellFillColor);
 	void endTable();
	void undoChange(const uint8_t undoType, const uint16_t undoLevel);
	void justificationChange(const uint8_t justification);
	void setTextColor(const RGBSColor * fontColor);
	void setTextFont(const WPXString fontName);
	void setFontSize(const uint16_t fontSize);
	void insertPageNumber(const WPXString &pageNumber);
	void insertNoteReference(const WPXString &noteReference);
	void insertNote(const WPXNoteType noteType);
//	void headerFooterGroup(const uint8_t headerFooterType, const uint8_t occurenceBits, WP3SubDocument *subDocument);
	void suppressPage(const uint16_t suppressCode);
	
protected:
	void _handleSubDocument(const WPXSubDocument *subDocument, const bool isHeaderFooter, WPXTableList tableList, int nextTableIndice = 0);
	void _openParagraph();

	void _flushText();
	void _changeList() {};
	
private:
	WPS4ContentParsingState *m_parseState;	
};

#endif /* WPS6_H */
