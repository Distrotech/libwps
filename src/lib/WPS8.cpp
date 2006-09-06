/* libwps
 * Copyright (C) 2006 Andrew Ziem
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#ifdef DEBUG
#include <bitset>
#endif
#include <errno.h>
#include <iconv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "WPS8.h"
#include "WPDocument.h"
#include "WPXHeader.h"
#include "WPXStream.h"
#include "libwpd_internal.h"


#define WPS8_PAGES_HEADER_OFFSET 0x22

/*
WPS8Parser public
*/


WPS8Parser::WPS8Parser(WPXInputStream *input, WPXHeader * header) :
	WPXParser(input, header)
{
}

WPS8Parser::~WPS8Parser ()
{
}

void WPS8Parser::parse(WPXHLListenerImpl *listenerImpl)
{
	std::list<WPXPageSpan> pageList;
	//fixme: subDocuments not used
	std::vector<WPS8SubDocument *> subDocuments;	
	
	WPD_DEBUG_MSG(("WPS8Parser::parse()\n"));		

	WPXInputStream *input = getInput();		

	/* parse pages */
	WPXPageSpan m_currentPage;
	parsePages(pageList, input);		
	
	/* parse document */
	WPS8ContentListener listener(pageList, subDocuments, listenerImpl);
	parse(input, &listener);	
}


/*
WPS8Parser private
*/

/**
 * Read the page format from the file.  It seems that WPS8 files
 * can only have one page format throughout the whole document.
 *
 */
void WPS8Parser::parsePages(std::list<WPXPageSpan> &pageList, WPXInputStream *input)
{
	//fixme: this method doesn't do much

	/* record page format */
	WPXPageSpan ps;
	pageList.push_back(ps);
}

void WPS8Parser::parse(WPXInputStream *input, WPS8Listener *listener)
{
	WPD_DEBUG_MSG(("WPS8Parser::parse()\n"));	

	listener->startDocument();

	input->seek(0x34, WPX_SEEK_SET);		
	size_t textLength = readU32(input);
	if (0 == textLength)
	{
		WPD_DEBUG_MSG(("Works: error: no text\n", textLength, textLength));
		throw ParseException();
	}

	WPD_DEBUG_MSG(("Works: info: textLength = %u (0x%x)\n", textLength, textLength));
	
	// fixme: the conversion code is not so good
	// read the text contents
	input->seek(0x200, WPX_SEEK_SET);	
	uint8_t * text = (uint8_t *) malloc(textLength + 2);
	if (!text)
		throw GenericException();	
	for (uint32_t x = 0; x<=textLength; x+=2)
	{
		text[x]=read8(input);
		text[x+1]=read8(input);		
	}
	
	text[textLength-2]=text[textLength-1]=0;
	
	// fixme: the conversion code is messy
	iconv_t cd; // conversion descriptor
	cd = iconv_open("UTF-8", "UTF-16LE"); //fixme: guessing
	if ((iconv_t)-1 == cd)
	{
		WPD_DEBUG_MSG(("Works: error: iconv_open() failed\n"));
		throw GenericException();
	}
	size_t outBytesLeft = textLength*2;
	char *outBuffer = (char *)malloc(outBytesLeft+1);
	if (NULL == outBuffer)
	{
		iconv_close(cd);	
		throw GenericException();
	}
	char *source = (char *) text;
	char *result = outBuffer;	
	size_t rc = iconv(cd, (char**)&text, &textLength, &outBuffer, &outBytesLeft);
	WPD_DEBUG_MSG(("Works: info: iconv() returns with rc=%i, textLength=%i, outBytesLeft=%i\n", 
		rc, textLength, outBytesLeft));
	if ((size_t)-1 == rc)
	{
		WPD_DEBUG_MSG(("Works: error: iconv() failed with errno=%i\n", errno));
		iconv_close(cd);
		free(outBuffer);
		throw GenericException();
	}
	iconv_close(cd);
	
	uint32_t end_of_result = strlen(result);
	
	WPD_DEBUG_MSG(("Works: info: text=%p, outBuffer=%p, end_of_result=%i\n",
		text, outBuffer, end_of_result));	
	
	for (uint32_t x = 0; x<end_of_result; x++)
	{
		uint8_t * ch = (uint8_t *) &result[x];
		switch (*ch)
		{
			case 0x0A:
				break;
			case 0x0D:
				listener->insertEOL();
				break;
			default:
				listener->insertCharacter((uint8_t)*ch);
				break;
		}	
	}
	free(source);
	free(result);
	
	listener->endDocument();		
}


/*
WPS8Listener public
*/

WPS8Listener::WPS8Listener()
{
}


/*
WPS8ContentParsingState public
*/

_WPS8ContentParsingState::_WPS8ContentParsingState()
{
	m_textBuffer.clear();
}

_WPS8ContentParsingState::~_WPS8ContentParsingState()
{
	WPD_DEBUG_MSG(("~WPS8ContentParsingState:: m_textBuffer.len() =%i\n", m_textBuffer.len()));		
	m_textBuffer.clear();
}


/*
WPS8ContentListener public
*/

WPS8ContentListener::WPS8ContentListener(std::list<WPXPageSpan> &pageList, std::vector<WPS8SubDocument *> &subDocuments, WPXHLListenerImpl *listenerImpl) :
	WPS8Listener(),
	WPXContentListener(pageList, listenerImpl),
	m_parseState(new WPS8ContentParsingState)
{
}

WPS8ContentListener::~WPS8ContentListener()
{
	delete m_parseState;
}

void WPS8ContentListener::insertCharacter(const uint16_t character) 
{
	if (!m_ps->m_isSpanOpened)
		_openSpan();
	m_parseState->m_textBuffer.append(character);
}

void WPS8ContentListener::insertTab(const uint8_t tabType, float tabPosition) 
{
	WPD_DEBUG_MSG(("STUB WPS8ContentListener::insertTab()\n"));		
}

void WPS8ContentListener::insertEOL() 
{
	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
		_openSpan();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();
	if (m_ps->m_isListElementOpened)
		_closeListElement();
}

void WPS8ContentListener::attributeChange(const bool isOn, const uint8_t attribute)
{
	WPD_DEBUG_MSG(("WPS8ContentListener::attributeChange(%i, %i)\n", isOn, attribute));		
	_closeSpan();

	//fixme: stub	
}

void WPS8ContentListener::marginChange(const uint8_t side, const uint16_t margin) {}
void WPS8ContentListener::indentFirstLineChange(const int16_t offset) {}
void WPS8ContentListener::columnChange(const WPXTextColumnType columnType, const uint8_t numColumns, const std::vector<float> &columnWidth,
				const std::vector<bool> &isFixedWidth) {}

void WPS8ContentListener::defineTable(const uint8_t position, const uint16_t leftOffset) {}
void WPS8ContentListener::addTableColumnDefinition(const uint32_t width, const uint32_t leftGutter, const uint32_t rightGutter,
				const uint32_t attributes, const uint8_t alignment) {}
void WPS8ContentListener::startTable() {}
void WPS8ContentListener::insertRow() {}
void WPS8ContentListener::insertCell() {}
void WPS8ContentListener::closeCell() {}
void WPS8ContentListener::closeRow() {}
void WPS8ContentListener::setTableCellSpan(const uint16_t colSpan, const uint16_t rowSpan) {}
void WPS8ContentListener::setTableCellFillColor(const RGBSColor * cellFillColor) {}
void WPS8ContentListener::endTable() {}
void WPS8ContentListener::undoChange(const uint8_t undoType, const uint16_t undoLevel) {}
void WPS8ContentListener::justificationChange(const uint8_t justification) {}
void WPS8ContentListener::setTextColor(const RGBSColor * fontColor) { }

void WPS8ContentListener::setTextFont(const WPXString fontName)
{
	_closeSpan();
	*(m_ps->m_fontName) = fontName;	
}

void WPS8ContentListener::setFontSize(const uint16_t fontSize)
{
	_closeSpan();
	m_ps->m_fontSize=float(fontSize);
}

void WPS8ContentListener::insertPageNumber(const WPXString &pageNumber) {}
void WPS8ContentListener::insertNoteReference(const WPXString &noteReference) {}
void WPS8ContentListener::insertNote(const WPXNoteType noteType) {}
void WPS8ContentListener::suppressPage(const uint16_t suppressCode) {}


/*
WPS8ContentListener protected 
*/

void WPS8ContentListener::_handleSubDocument(const WPXSubDocument *subDocument, const bool isHeaderFooter, WPXTableList tableList, int nextTableIndice)
{
	WPD_DEBUG_MSG(("STUB WPS8ContentListener::_handleSubDocument()\n"));		
}

void WPS8ContentListener::_openParagraph()
{
//	WPD_DEBUG_MSG(("STUB WPS8ContentListener::_openParagraph()\n"));		
	WPXContentListener::_openParagraph();
}

void WPS8ContentListener::_flushText()
{
	if (m_parseState->m_textBuffer.len())
		m_listenerImpl->insertText(m_parseState->m_textBuffer);
	m_parseState->m_textBuffer.clear();
}
