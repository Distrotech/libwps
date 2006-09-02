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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "WPS4.h"
#include "WPDocument.h"
#include "WPXHeader.h"
#include "WPXStream.h"
#include "libwpd_internal.h"


#define WPS4_FCMAC_OFFSET 0x26	
#define WPS4_TEXT_OFFSET 0x100

/*
WPS4Parser public
*/


WPS4Parser::WPS4Parser(WPXInputStream *input, WPXHeader * header) :
	WPXParser(input, header)
{
	//fixme: don't ask for a header that we don't use
	stream_length = input->tell();
}

WPS4Parser::~WPS4Parser ()
{

}

void WPS4Parser::parse(WPXHLListenerImpl *listenerImpl)
{
	std::list<WPXPageSpan> pageList;
	//fixme: subDocuments not used
	std::vector<WPS4SubDocument *> subDocuments;	
	
	WPD_DEBUG_MSG(("WPS4Parser::parse()\n"));		

	WPXInputStream *input = getInput();		
	WPXPageSpan m_currentPage;
	parsePages(pageList, input);		
	WPS4ContentListener listener(pageList, subDocuments, listenerImpl);
	parse(input, &listener);	
}


/*
WPS4Parser private
*/


/**
 * Reads fonts table.
 *
 */
void WPS4Parser::readFontsTable(WPXInputStream * input)
{
	//fixme: location is 0xAA or 0x92?
#if 0
	input->seek(0x80, WPX_SEEK_SET);
	uint32_t offset_end_FFNTB = readU32(input); /* end of fonts table */
	//fixme: sanity check offset_end_FFNTB
	if (offset_end_FFNTB < (offset_eot+256))
	{
		WPD_DEBUG_MSG(("Works: error: offset_end_FFNTB=0x%x, too small\n"));
		throw FileException();
	}
	
	uint32_t pnFfntb = (offset_end_FFNTB)/128; /* page number of character information */
	input->seek((128*pnFfntb)+0x14, WPX_SEEK_SET);
	
	WPD_DEBUG_MSG(("Works: info: offset_end_FFNTB=0x%x, pnFfntb=%i, 128*pnFfntb=0x%x\n",
		offset_end_FFNTB, pnFfntb, 128*pnFfntb));
#else
	/* offset of FFNTB */
	input->seek(0x5E, WPX_SEEK_SET);
	uint32_t offset_FFNTB = readU32(input);
	//fixme: sanity check

	/* length of FFNTB */
	input->seek(0x62, WPX_SEEK_SET);	
	uint32_t len_FFNTB = readU16(input);
	
	uint32_t offset_end_FFNTB = offset_FFNTB + len_FFNTB;	
	
	WPD_DEBUG_MSG(("Works: info: offset_FFNTB=0x%x, len_FFNTB=0x%x, offset_end_FFNTB=0x%x\n",
		offset_FFNTB, len_FFNTB, offset_end_FFNTB));
	input->seek(offset_FFNTB, WPX_SEEK_SET);

#endif

	while (input->tell() < offset_end_FFNTB)
	{
		uint16_t font_number = readU16(input);
/* fixme: Sometimes, the fonts are nicely indexed.  Other times
 * many start with (usually) 4112 or 8200.  In the latter case, each font
 * is uniquely assigned to a number.  However, some numbers
 * refer to two or more fonts.
 */

		if (font_number != fonts.size())
		{
			WPD_DEBUG_MSG(("Works: error: at position 0x%x expected font number %i but got %i (0x%x)\n",
				(input->tell())-2, fonts.size(), font_number, font_number));		
#if 0							
			throw FileException();
#endif			
		}

		std::string s;
		for (uint8_t i = readU8(input); i>0; i--)
		{
			s.append(1, (uint8_t)readU8(input));
		}
		WPD_DEBUG_MSG(("Works: info: font_number =%i =%s\n",font_number, s.c_str()));
		s.append(1, 0);
		fonts.push_back(s);
	}
}

/**
 * Read a single page (128 bytes) that contains formatting descriptors for either
 * characters OR paragraphs.  Starts reading at current position in stream.
 *
 * Return: true if more pages of this type exist, otherwise false
 *
 */
bool WPS4Parser::readFODPage(WPXInputStream * input, std::vector<FOD> * FODs)
{
	uint8_t cfod; /* number of FODs on this page */
	uint32_t page_offset = input->tell();
	
	input->seek(127, WPX_SEEK_CUR);	
	cfod = readU8(input);
	WPD_DEBUG_MSG(("Works: info: cfod = %i (%x)\n", cfod, cfod));				
	if (cfod > 0x18)
	{
		throw FileException();
	}
	input->seek(-128, WPX_SEEK_CUR);	
	
	uint32_t fcFirst; /* Byte number of first character covered by this page 
			     of formatting information */	
			    
	fcFirst = readU32(input);
	
	int first_fod = FODs->size();

	/* read array of fcLim of FODs */	
	for (int i = 0; i < cfod; i++)
	{
		FOD fod;
		fod.fcLim = readU32(input);
//		WPD_DEBUG_MSG(("Works: info: fcLim = %i (%x)\n", fod.fcLim, fod.fcLim));			
		
		/* check that fcLim is not too large */
		if (fod.fcLim > offset_eot)
		{
			WPD_DEBUG_MSG(("Works: error: length of 'text selection' %i > "
				"total text length %i\n", fod.fcLim, offset_eot));					
			throw FileException();	
		}

		/* check that fcLim is monotonic */
		if (FODs->size() > 0 && FODs->back().fcLim > fod.fcLim)
		{
			WPD_DEBUG_MSG(("Works: error: character position list must "
				"be monotonic, but found %i, %i\n", FODs->back().fcLim, fod.fcLim));
			throw FileException();
		}
		FODs->push_back(fod);
	} 	
	
	/* read array of bfprop of FODs */
	std::vector<FOD>::iterator FODs_iter;
	for (FODs_iter = FODs->begin()+first_fod; FODs_iter!= FODs->end(); FODs_iter++)
	{
		if ((*FODs_iter).fcLim == offset_eot)
		{
			break;
		}	
	
		(*FODs_iter).bfprop = readU8(input);
		
		/* check size of bfprop  */
		if (((*FODs_iter).bfprop < (5*cfod) && (*FODs_iter).bfprop > 0) ||
			 (*FODs_iter).bfprop  > (128 - 1))
		{
			WPD_DEBUG_MSG(("Works: error: size of bfprop is bad "
				"%i (0x%x)\n", (*FODs_iter).bfprop, (*FODs_iter).bfprop));		
			throw FileException();
		}
		
		(*FODs_iter).bfprop_abs = (*FODs_iter).bfprop + page_offset;
//		WPD_DEBUG_MSG(("Works: info: bfprop_abs = "
//			"%i (%x)\n", (*FODs_iter).bfprop_abs, (*FODs_iter).bfprop_abs));		
	}
	
	/* read array of FPROPs */
	for (FODs_iter = FODs->begin()+first_fod; FODs_iter!= FODs->end(); FODs_iter++)
	{
		if ((*FODs_iter).fcLim == offset_eot)
		{
			break;
		}
		if (0 == (*FODs_iter).bfprop)
		{
			(*FODs_iter).fprop.cch = 0;
			break;
		}
		input->seek((*FODs_iter).bfprop_abs, WPX_SEEK_SET);
		(*FODs_iter).fprop.cch = readU8(input);
//		WPD_DEBUG_MSG(("Works: cch = %i\n", (*FODs_iter).fprop.cch));		
		if (0 == (*FODs_iter).fprop.cch)
		{
			WPD_DEBUG_MSG(("Works: error: 0 == cch at file offset 0x%x", (input->tell())-1));
			throw FileException();
		}
		// fixme: what is largest cch?
		/* generally paragraph cch are bigger than character cch */
		if ((*FODs_iter).fprop.cch > 27)
		{
			WPD_DEBUG_MSG(("Works: error: cch = %i, too large ", (*FODs_iter).fprop.cch));
			throw FileException();
		}
		for (int i = 0; (*FODs_iter).fprop.cch > i; i++)
		{
			(*FODs_iter).fprop.rgchProp.append(1, (uint8_t)readU8(input));
		}
	}
	
	/* go to end of page */
	input->seek(page_offset	+ 128, WPX_SEEK_SET);

	return (offset_eot < FODs->back().fcLim);
}

#ifdef DEBUG
std::string to_bits(std::string s)
{
	std::string r;
	for (int i = 0; i < s.length(); i++)
	{
		std::bitset<8> b(s[i]);	
		r.append(b.to_string());
		char buf[10];
		sprintf(buf, " (%u,0x%x)  ", (uint8_t)s[i],(uint8_t)s[i]);
		r.append(buf);
	}
	return r;
}
#endif

#define WPS4_ATTRIBUTE_BOLD 0
#define WPS4_ATTRIBUTE_ITALICS 1
#define WPS4_ATTRIBUTE_UNDERLINE 2
#define WPS4_ATTRIBUTE_STRIKEOUT 3
#define WPS4_ATTRIBUTE_SUBSCRIPT 4
#define WPS4_ATTRIBUTE_SUPERSCRIPT 5

void WPS4Parser::propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS4Listener *listener)
{
	if ((oldTextAttributeBits ^ newTextAttributeBits) & bit)
	{
		WPD_DEBUG_MSG(("WPS4Parser::propertyChangeDelta: attribute %i changed, now = %i\n", attribute, newTextAttributeBits & bit));	
		listener->attributeChange(newTextAttributeBits & bit, attribute);
	}
}

/**
 * @param newTextAttributeBits: all the new, current bits (will be compared against old, and old will be discarded).
 *
 */
void WPS4Parser::propertyChangeDelta(uint32_t newTextAttributeBits, WPS4Listener *listener)
{
	WPD_DEBUG_MSG(("WPS4Parser::propertyChangeDelta(%i, %p)\n", newTextAttributeBits, listener));	
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_BOLD, WPX_BOLD_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_ITALICS, WPX_ITALICS_BIT, listener);	
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_UNDERLINE, WPX_UNDERLINE_BIT, listener);		
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_STRIKEOUT, WPX_STRIKEOUT_BIT, listener);	
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_SUBSCRIPT, WPX_SUBSCRIPT_BIT, listener);		
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_SUPERSCRIPT, WPX_SUPERSCRIPT_BIT, listener);		
	oldTextAttributeBits = newTextAttributeBits;
}

/**
 * Process a character property change.
 *
 */
void WPS4Parser::propertyChange(std::string rgchProp, WPS4Listener *listener)
{
	if (0 == rgchProp.length())
		return;

//fixme: this method is immature
	/* set default properties */
	uint32_t textAttributeBits = 0;
	
	/* set difference from default properties */
	if (rgchProp[0] & 0x01)
	{
		textAttributeBits |= WPX_BOLD_BIT;
	}
	if (rgchProp[0] & 0x02)
	{
		textAttributeBits |= WPX_ITALICS_BIT;
	}
	if (rgchProp[0] & 0x04)
	{
		textAttributeBits |= WPX_STRIKEOUT_BIT;
	}
	if (rgchProp.length() >= 3)
	{
		uint8_t font_n = (uint8_t)rgchProp[2];
		if ((fonts.size()-1) < font_n)
		{
			WPD_DEBUG_MSG(("Works: error: expected font %i but there are only %i\n", 
				font_n,(fonts.size()-1) ));			
/* fixme: some files don't index fonts nicely--some other format is used */				
#if 0				
			throw FileException();
#endif			
		}
		else
			listener->setTextFont(fonts.at(font_n).c_str());
	}
	if (rgchProp.length() >= 4)
	{
		if ((rgchProp[1] &  0x20) && (rgchProp[3] & 0x20))
			textAttributeBits |= WPX_UNDERLINE_BIT;
	}
	if (rgchProp.length() >= 5)
	{
		listener->setFontSize(((uint8_t)rgchProp[4])/2);
	}
	else
	{
		listener->setFontSize(12);
	}
	if (rgchProp.length() >= 6)
	{
		WPD_DEBUG_MSG(("rgchProp[1] & 0x40 = %i, rgchProp[5]=%i\n", (rgchProp[1] & 0x40), (uint8_t)rgchProp[5]));
		if ((rgchProp[1] & 0x40) && (1 == (uint8_t)rgchProp[5]))
		{
			textAttributeBits |= WPX_SUPERSCRIPT_BIT;			
		}
		
		if ((rgchProp[1] & 0x40) && (128 == (uint8_t)rgchProp[5]))
		{
			textAttributeBits |= WPX_SUBSCRIPT_BIT;					
		}
	}
	propertyChangeDelta(textAttributeBits, listener);
}

/**
 * Read the text of the document using previously-read
 * formatting information.
 *
 */
void WPS4Parser::readText(WPXInputStream * input, WPS4Listener *listener)
{
	oldTextAttributeBits = 0;
	std::vector<FOD>::iterator FODs_iter;	
#if 0
	/* dump for debugging */
	for (FODs_iter = CHFODs.begin(); FODs_iter!= CHFODs.end(); FODs_iter++)
	{
		FOD fod = *(FODs_iter);
		printf("FOD  fcLim=%u (0x%04x), bfprop=%u, bfprop_abs=%u\n", fod.fcLim, fod.fcLim, fod.bfprop, fod.bfprop_abs);
	}	
#endif	
	
	uint32_t last_fcLim = 0x100;
	for (FODs_iter = CHFODs.begin(); FODs_iter!= CHFODs.end(); FODs_iter++)
	{
		uint32_t len = (*FODs_iter).fcLim - last_fcLim;
		WPD_DEBUG_MSG(("Works: info: txt l=%02i rgchProp=%s\n", 
			len, to_bits((*FODs_iter).fprop.rgchProp).c_str()));
		if ((*FODs_iter).fprop.cch > 0)
			propertyChange((*FODs_iter).fprop.rgchProp, listener);
		input->seek(last_fcLim, WPX_SEEK_SET);			
		for (uint32_t i = len; i>0; i--)
		{
			uint8_t readVal = readU8(input);
//			WPD_DEBUG_MSG(("Works: info: position %x = %c (0x%02x)\n", (input->tell())-1, readVal, readVal));
			if (0x0D == readVal)
			{
				listener->insertEOL();
			}
			else if (0x00 == readVal)
				break;
			if (0x0A != readVal)
				listener->insertCharacter( (uint16_t)readVal );
		}	
		last_fcLim = (*FODs_iter).fcLim;	
	}

}


/**
 * Read the page format from the file.  It seems that WPS4 files
 * can only have one page format throughout the whole document.
 *
 */
void WPS4Parser::parsePages(std::list<WPXPageSpan> &pageList, WPXInputStream *input)
// fixme: this function is immature
{
	/* read page format */
	input->seek(0x64, WPX_SEEK_SET);
	unsigned int margin_top = readU16(input);
	input->seek(0x68, WPX_SEEK_SET);
	unsigned int margin_left =readU16(input);
	input->seek(0x6A, WPX_SEEK_SET);
	unsigned int margin_right =readU16(input);	
	input->seek(0x78, WPX_SEEK_SET);
	unsigned int margin_bottom =readU16(input);		
	
	/* check page format */
	//todo: check the bottom margin which acted funny and has a strange offset
	//fixme: assert margins within reasonable limits	
	WPD_DEBUG_MSG(("Works: info: page margins (t,l,r,b): raw(%i,%i,%i,%i), inches(%f,%f,%f,%f\n",
		margin_top, margin_left, margin_right, margin_bottom,
		margin_top/1440, margin_left/1440, margin_right/1440, margin_bottom/1440));		
		
	/* record page format */
	WPXPageSpan ps;
	ps.setMarginTop(margin_top/1440);
//	ps.setMarginBottom(margin_bottom/1440);	
	ps.setMarginBottom(0.5);	
	ps.setMarginLeft(margin_left/1440);		
	ps.setMarginRight(margin_right/1440);			
	
	pageList.push_back(ps);
}

void WPS4Parser::parse(WPXInputStream *input, WPS4Listener *listener)
{
	uint8_t readVal;
	size_t text_length;
	
	WPD_DEBUG_MSG(("WPS4Parser::parse()\n"));	

	listener->startDocument();

#if 0 /* not always valid */
	input->seek(0xA2, WPX_SEEK_SET);
	offset_eos = readU32(input);
	if (offset_eos < 3584)
	{
		WPD_DEBUG_MSG(("Works: error: offset_eos = %x (%i), too small\n", offset_eos, offset_eos));				
		throw FileException();
	}
#endif

	/* find beginning of character FODs */
	input->seek(WPS4_FCMAC_OFFSET, WPX_SEEK_SET);
	offset_eot = readU32(input); /* stream offset to end of text */
	WPD_DEBUG_MSG(("Works: info: offset_eot at WPS4_FCMAC_OFFSET = %x (%i)\n", offset_eot, offset_eot));			
	uint32_t pnChar = (offset_eot+127)/128; /* page number of character information */
	WPD_DEBUG_MSG(("Works: info: 128*pnChar = %x (%i)\n", pnChar*128, pnChar*128));
	
	/* sanity check */
	input->seek(128*pnChar, WPX_SEEK_SET);
	uint32_t v = readU32(input);
	if (0x0100 != v)
	{
		WPD_DEBUG_MSG(("Works: warning: expected value 0x0100 at location %x but got %x (%i)\n", 128*pnChar, v, v));		
	}
	
	/* go to beginning of character FODs */
	input->seek(128*pnChar, WPX_SEEK_SET);

	/* read character FODs */	
	while (readFODPage(input, &CHFODs))
	{
	}	
	// fixme: verify: final FOD covers end of text
	
	/* read paragraph formatting */
	while (readFODPage(input, &PAFODs))
	{
	}		
	
	/* read fonts table */
#if 1
	readFontsTable(input);
#else
	fonts.push_back("Times New Roman");
#endif
	
	/* process text file using previously-read character formatting */
	readText(input, listener);
	
	listener->endDocument();		
}


/*
WPS4Listener public
*/

WPS4Listener::WPS4Listener()
{
}


/*
WPS4ContentParsingState public
*/

_WPS4ContentParsingState::_WPS4ContentParsingState()
{
	m_textBuffer.clear();
}

_WPS4ContentParsingState::~_WPS4ContentParsingState()
{
	WPD_DEBUG_MSG(("~WPS4ContentParsingState:: m_textBuffer.len() =%i\n", m_textBuffer.len()));		
	m_textBuffer.clear();
}


/*
WPS4ContentListener public
*/

WPS4ContentListener::WPS4ContentListener(std::list<WPXPageSpan> &pageList, std::vector<WPS4SubDocument *> &subDocuments, WPXHLListenerImpl *listenerImpl) :
	WPS4Listener(),
	WPXContentListener(pageList, listenerImpl),
	m_parseState(new WPS4ContentParsingState)
{
}

WPS4ContentListener::~WPS4ContentListener()
{
	delete m_parseState;
}

void WPS4ContentListener::insertCharacter(const uint16_t character) 
{
	if (!m_ps->m_isSpanOpened)
		_openSpan();
	m_parseState->m_textBuffer.append(character);
}

void WPS4ContentListener::insertTab(const uint8_t tabType, float tabPosition) 
{
	WPD_DEBUG_MSG(("STUB WPS4ContentListener::insertTab()\n"));		
}

void WPS4ContentListener::insertEOL() 
{
	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
		_openSpan();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();
	if (m_ps->m_isListElementOpened)
		_closeListElement();
}

void WPS4ContentListener::attributeChange(const bool isOn, const uint8_t attribute)
{
	WPD_DEBUG_MSG(("WPS4ContentListener::attributeChange(%i, %i)\n", isOn, attribute));		
	_closeSpan();
	
	uint32_t textAttributeBit = 0;
	switch (attribute)
	{
		case WPS4_ATTRIBUTE_BOLD:
			textAttributeBit = WPX_BOLD_BIT;
			break;
		case WPS4_ATTRIBUTE_ITALICS:
			textAttributeBit = WPX_ITALICS_BIT;		
			break;
		case WPS4_ATTRIBUTE_UNDERLINE:
			textAttributeBit = WPX_UNDERLINE_BIT;		
			break;	
		case WPS4_ATTRIBUTE_STRIKEOUT:
			textAttributeBit = WPX_STRIKEOUT_BIT;		
			break;				
		case WPS4_ATTRIBUTE_SUBSCRIPT:
			textAttributeBit = WPX_SUBSCRIPT_BIT;		
			break;							
		case WPS4_ATTRIBUTE_SUPERSCRIPT:
			textAttributeBit = WPX_SUPERSCRIPT_BIT;		
			break;							
	}
	if (isOn)
		m_ps->m_textAttributeBits |= textAttributeBit;
	else
		m_ps->m_textAttributeBits ^= textAttributeBit;
}

#if 0
void WPS4ContentListener::attributeChange(const uint32_t attributeBits)
{
	_closeSpan();
	m_ps->m_textAttributeBits = attributeBits;
}
#endif

void WPS4ContentListener::marginChange(const uint8_t side, const uint16_t margin) {}
void WPS4ContentListener::indentFirstLineChange(const int16_t offset) {}
void WPS4ContentListener::columnChange(const WPXTextColumnType columnType, const uint8_t numColumns, const std::vector<float> &columnWidth,
				const std::vector<bool> &isFixedWidth) {}

void WPS4ContentListener::defineTable(const uint8_t position, const uint16_t leftOffset) {}
void WPS4ContentListener::addTableColumnDefinition(const uint32_t width, const uint32_t leftGutter, const uint32_t rightGutter,
				const uint32_t attributes, const uint8_t alignment) {}
void WPS4ContentListener::startTable() {}
void WPS4ContentListener::insertRow() {}
void WPS4ContentListener::insertCell() {}
void WPS4ContentListener::closeCell() {}
void WPS4ContentListener::closeRow() {}
void WPS4ContentListener::setTableCellSpan(const uint16_t colSpan, const uint16_t rowSpan) {}
void WPS4ContentListener::setTableCellFillColor(const RGBSColor * cellFillColor) {}
void WPS4ContentListener::endTable() {}
void WPS4ContentListener::undoChange(const uint8_t undoType, const uint16_t undoLevel) {}
void WPS4ContentListener::justificationChange(const uint8_t justification) {}
void WPS4ContentListener::setTextColor(const RGBSColor * fontColor) { }

void WPS4ContentListener::setTextFont(const WPXString fontName)
{
	_closeSpan();
	*(m_ps->m_fontName) = fontName;	
}

void WPS4ContentListener::setFontSize(const uint16_t fontSize)
{
	_closeSpan();
	m_ps->m_fontSize=float(fontSize);
}

void WPS4ContentListener::insertPageNumber(const WPXString &pageNumber) {}
void WPS4ContentListener::insertNoteReference(const WPXString &noteReference) {}
void WPS4ContentListener::insertNote(const WPXNoteType noteType) {}
void WPS4ContentListener::suppressPage(const uint16_t suppressCode) {}


/*
WPS4ContentListener protected 
*/

void WPS4ContentListener::_handleSubDocument(const WPXSubDocument *subDocument, const bool isHeaderFooter, WPXTableList tableList, int nextTableIndice)
{
	WPD_DEBUG_MSG(("STUB WPS4ContentListener::_handleSubDocument()\n"));		
}

void WPS4ContentListener::_openParagraph()
{
//	WPD_DEBUG_MSG(("STUB WPS4ContentListener::_openParagraph()\n"));		
	WPXContentListener::_openParagraph();
}

void WPS4ContentListener::_flushText()
{
	if (m_parseState->m_textBuffer.len())
		m_listenerImpl->insertText(m_parseState->m_textBuffer);
	m_parseState->m_textBuffer.clear();
}
