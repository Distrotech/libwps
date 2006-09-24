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
 * Read the text of the document using previously-read
 * formatting information.
 *
 */

// fixme: this method might turn out to be like WPS4Parser::readText()

void WPS8Parser::readText(WPXInputStream * input, WPS8Listener *listener)
{
	WPD_DEBUG_MSG(("WPS8Parser::readText()\n"));

	// fixme: stub
	std::vector<FOD>::iterator FODs_iter;	

	/* dump for debugging */
	uint32_t last_fcLim = 0x200; //fixme: start of text might vary according to header index table?
	for (FODs_iter = CHFODs.begin(); FODs_iter!= CHFODs.end(); FODs_iter++)
	{
		FOD fod = *(FODs_iter);
		uint32_t len = (*FODs_iter).fcLim - last_fcLim;
		if (len % 2 != 0)
		{
			WPD_DEBUG_MSG(("Works: error: len %i is odd\n", len));
			throw ParseException();
		}
		len /= 2;

		/* print rgchProp as hex bytes */
		WPD_DEBUG_MSG(("rgch="));
		for (int blah=0; blah < (*FODs_iter).fprop.rgchProp.length(); blah++)
		{
			WPD_DEBUG_MSG(("%02X ", (uint8_t) (*FODs_iter).fprop.rgchProp[blah]));
		}
		WPD_DEBUG_MSG(("\n"));

		/* process character formatting */
		if ((*FODs_iter).fprop.cch > 0)
			propertyChange((*FODs_iter).fprop.rgchProp, listener);

		/* plain text */
		input->seek(last_fcLim, WPX_SEEK_SET);			
		for (uint32_t i = len; i>0; i--)
		{
			uint16_t readVal = readU16(input);

			if (0x00 == readVal)
				break;
				
			switch (readVal)
			{
				case 0x0A:
					break;

				case 0x0C:
					listener->insertBreak(WPX_PAGE_BREAK);
					break;

				case 0x0D:
					listener->insertEOL();
					break;
				default:
					// fixme: convert UTF-16LE to UTF-8
					listener->insertCharacter( readVal );
					break;
			}
		}	
		last_fcLim = (*FODs_iter).fcLim;	
	}	
}

/**
 * Read a single page (of size page_size bytes) that contains formatting descriptors
 * for either characters OR paragraphs.  Starts reading at current position in stream.
 *
 * Return: true if more pages of this type exist, otherwise false
 *
 */

//fixme: this readFODPage is mostly the same as in WPS4

bool WPS8Parser::readFODPage(WPXInputStream * input, std::vector<FOD> * FODs, uint16_t page_size)
{
	uint32_t page_offset = input->tell();
	uint16_t cfod = readU16(input); /* number of FODs on this page */

	if (cfod > 0x20)
	{
		WPD_DEBUG_MSG(("Works8: error: cfod = %i (0x%x\n", cfod, cfod));
		throw ParseException();
	}

	input->seek(6, WPX_SEEK_CUR);	// fixme: unknown

	int first_fod = FODs->size();

	/* Read array of fcLim of FODs.  The fcLim refers to the offset of the
           last character covered by the formatting. */
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

        /* Read array of bfprop of FODs.  The bfprop is the offset where
           the FPROP is located. */
	std::vector<FOD>::iterator FODs_iter;
	for (FODs_iter = FODs->begin() + first_fod; FODs_iter!= FODs->end(); FODs_iter++)
	{
		if ((*FODs_iter).fcLim == offset_eot)
			break;

                (*FODs_iter).bfprop = readU16(input);

		/* check size of bfprop  */
		if (((*FODs_iter).bfprop < (8 + (6*cfod)) && (*FODs_iter).bfprop > 0) ||
			(*FODs_iter).bfprop  > (page_size - 1))
		{
			WPD_DEBUG_MSG(("Works: error: size of bfprop is bad "
				"%i (0x%x)\n", (*FODs_iter).bfprop, (*FODs_iter).bfprop));
			throw FileException();
		}

		(*FODs_iter).bfprop_abs = (*FODs_iter).bfprop + page_offset;
//		WPD_DEBUG_MSG(("Works: debug: bfprop = 0x%03X, bfprop_abs = 0x%03X\n",
//                       (*FODs_iter).bfprop, (*FODs_iter).bfprop_abs));
	}

	
	/* Read array of FPROPs.  These contain the actual formatting
	   codes (bold, alignment, etc.) */
	for (FODs_iter = FODs->begin()+first_fod; FODs_iter!= FODs->end(); FODs_iter++)
	{
		if ((*FODs_iter).fcLim == offset_eot)
			break;

		if (0 == (*FODs_iter).bfprop)
		{
			(*FODs_iter).fprop.cch = 0;
			break;
		}

		input->seek((*FODs_iter).bfprop_abs, WPX_SEEK_SET);
		(*FODs_iter).fprop.cch = readU8(input);
		if (0 == (*FODs_iter).fprop.cch)
		{
			WPD_DEBUG_MSG(("Works: error: 0 == cch at file offset 0x%x", (input->tell())-1));
			throw FileException();
		}
		// fixme: what is largest cch?
		if ((*FODs_iter).fprop.cch > 93)
		{
			WPD_DEBUG_MSG(("Works: error: cch = %i, too large ", (*FODs_iter).fprop.cch));
			throw FileException();
		}

		(*FODs_iter).fprop.cch--;

		for (int i = 0; (*FODs_iter).fprop.cch > i; i++)
			(*FODs_iter).fprop.rgchProp.append(1, (uint8_t)readU8(input));
	}
	
	/* go to end of page */
	input->seek(page_offset	+ page_size, WPX_SEEK_SET);

	return (offset_eot > FODs->back().fcLim);

	
}

/**
 * Parse an index entry in the file format's header.  For example,
 * this function may be called multiple times to parse several FDPP
 * entries.  This functions begins at the current position of the
 * input stream, which will be advanced.
 *
 */

void WPS8Parser::parseHeaderIndexEntry(WPXInputStream * input)
{
	WPD_DEBUG_MSG(("Works8: debug: parseHeaderIndexEntry() at file pos %x\n", input->tell()));

	uint16_t cch = readU16(input);
	if (0x18 != cch)
	{
		WPD_DEBUG_MSG(("Works8: cch = %i (0x%x)\n", cch, cch));
		throw ParseException();
	}

	std::string name;

	// sanity check
	for (int i =0; i < 4; i++)
	{
		name.append(1, readU8(input));

		if ((uint8_t)name[i] != 0 && (uint8_t)name[i] != 0x20 &&
			(41 > (uint8_t)name[i] || (uint8_t)name[i] > 90))
		{
			WPD_DEBUG_MSG(("Works8: error: bad character=%u (0x%02x) in name in header index\n", 
				(uint8_t)name[i], (uint8_t)name[i]));
			throw ParseException();
		}
	}
	name.append(1, 0);

	size_t numBytesRead;
	std::string unknown1;
	for (int i = 0; i < 6; i ++)
		unknown1.append(1, readU8(input));

	std::string name2;
	for (int i =0; i < 4; i++)
		name2.append(1, readU8(input));
	name2.append(1, 0);

	if (name != name2)
	{
		WPD_DEBUG_MSG(("Works8: error: name != name2, %s != %s\n", 
			name.c_str(), name2.c_str()));
		// fixme: what to do with this?
//		throw ParseException();
	}

	HeaderIndexEntries hie;
	hie.offset = readU32(input);
	hie.length = readU32(input);

	WPD_DEBUG_MSG(("Works8: info: header index entry %s with offset=%i, length=%i\n", 
		name.c_str(), hie.offset, hie.length));

	headerIndexTable.insert(make_pair(name, hie));
}

/**
 * In the header, parse the index to the different sections of
 * the CONTENTS stream.
 * 
 */
void WPS8Parser::parseHeaderIndex(WPXInputStream * input)
{
	input->seek(0x0C, WPX_SEEK_SET);		
	uint16_t n_entries = readU16(input);
	// fixme: sanity check n_entries

	input->seek(0x18, WPX_SEEK_SET);		
	do
	{
		uint16_t unknown1 = readU16(input);
		if (0x01F8 != unknown1)
		{
			WPD_DEBUG_MSG(("Works8: error: unknown1=%x\n", unknown1));
#if 0
			throw ParseException();
#endif
		}

		uint16_t n_entries_local = readU16(input);

		if (n_entries_local > 0x20)
		{
			WPD_DEBUG_MSG(("Works8: error: n_entries_local=%i\n", n_entries_local));
			throw ParseException();	
		}

		uint32_t next_index_table = readU32(input);

		do
		{
			parseHeaderIndexEntry(input);
			n_entries--;
			n_entries_local--;
		} while (n_entries > 0 && n_entries_local);
		
		if (0xFFFFFFFF == next_index_table && n_entries > 0)
		{
			WPD_DEBUG_MSG(("Works8: error: expected more header index entries\n"));
			throw ParseException();
		}

		if (0xFFFFFFFF == next_index_table)
			break;

		WPD_DEBUG_MSG(("Works8: debug: seeking to position %x\n", next_index_table));
		input->seek(next_index_table, WPX_SEEK_SET);
	} while (n_entries > 0);
}

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

	/* header index */
	parseHeaderIndex(input);

	HeaderIndexMultiMap::iterator pos;
	for (pos = headerIndexTable.begin(); pos != headerIndexTable.end(); ++pos)
	{
		WPD_DEBUG_MSG(("Works: debug: headerIndexTable: %s, offset=%X, length=%X\n",
			pos->first.c_str(), pos->second.offset, pos->second.length));
	}

	/* What is the total length of the text? */
	pos = headerIndexTable.lower_bound("TEXT");
	if (headerIndexTable.end() == pos)
	{
		WPD_DEBUG_MSG(("Works: error: no TEXT in header index table\n"));
	}
	offset_eot = pos->second.offset + pos->second.length;
	WPD_DEBUG_MSG(("Works: debug: TEXT offset_eot = %x\n", offset_eot));

	/* read character FODs (FOrmatting Descriptors) */
	for (pos = headerIndexTable.begin(); pos != headerIndexTable.end(); ++pos)
	{
		if (0 != strcmp("FDPC",pos->first.c_str()))
			continue;

		WPD_DEBUG_MSG(("Works: debug: FDPC (%s) offset=%x, length=%x\n",
				pos->first.c_str(), pos->second.offset, pos->second.length));

		input->seek(pos->second.offset, WPX_SEEK_SET);
		if (pos->second.length != 512)
		{
			WPD_DEBUG_MSG(("Works: warning: FDPC offset=%x, length=%x\n", 
				pos->second.offset, pos->second.length));
		}
		readFODPage(input, &CHFODs, pos->second.length);
	}

	/* process text file using previously-read character formatting */
	readText(input, listener);

	listener->endDocument();		
}



#define WPS8_ATTRIBUTE_BOLD 0
#define WPS8_ATTRIBUTE_ITALICS 1
#define WPS8_ATTRIBUTE_UNDERLINE 2
#define WPS8_ATTRIBUTE_STRIKEOUT 3
#define WPS8_ATTRIBUTE_SUBSCRIPT 4
#define WPS8_ATTRIBUTE_SUPERSCRIPT 5

void WPS8Parser::propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS8Listener *listener)
{
	if ((oldTextAttributeBits ^ newTextAttributeBits) & bit)
		listener->attributeChange(newTextAttributeBits & bit, attribute);
}

/**
 * @param newTextAttributeBits: all the new, current bits (will be compared against old, and old will be discarded).
 *
 */
void WPS8Parser::propertyChangeDelta(uint32_t newTextAttributeBits, WPS8Listener *listener)
{
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_BOLD, WPX_BOLD_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_ITALICS, WPX_ITALICS_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_UNDERLINE, WPX_UNDERLINE_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_STRIKEOUT, WPX_STRIKEOUT_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_SUBSCRIPT, WPX_SUBSCRIPT_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_SUPERSCRIPT, WPX_SUPERSCRIPT_BIT, listener);
	oldTextAttributeBits = newTextAttributeBits;
}



/**
 * Process a character property change.  The Works format supplies
 * all the character formatting each time there is any change (as
 * opposed to HTML, for example).  In Works 8, the position in
 * rgchProp is not significant because there are some kind of
 * codes.
 *
 */
void WPS8Parser::propertyChange(std::string rgchProp, WPS8Listener *listener)
{
	//fixme: this method is immature
	/* check */
	/* sometimes, the rgchProp is blank */
	if (0 == rgchProp.length())
		return;
	/* other than blank, the shortest should be 9 bytes */
	if (rgchProp.length() < 9)
	{
		WPD_DEBUG_MSG(("Works8: error: rgchProp.length() < 9\n"));
		throw ParseException();
	}
	if (0 == (rgchProp.length() % 2))
	{
		WPD_DEBUG_MSG(("Works8: error: rgchProp.length() is even\n"));
		throw ParseException();
	}
        if (0 != rgchProp[0] || 0 != rgchProp[1] || 0 != rgchProp[2])
	{
		WPD_DEBUG_MSG(("Works8: error: rgchProp does not begin 0x000000\n"));		
		throw ParseException();
	}

	/* set default properties */
        uint32_t textAttributeBits = 0;

	/* set difference from default properties */
	for (uint32_t x = 3; x < rgchProp.length(); x += 2)
	{
		if (0x0A == rgchProp[x+1])
		{
			switch(rgchProp[x])
			{
				case 0x02:
					textAttributeBits |= WPX_BOLD_BIT;
					break;
				case 0x03:
					textAttributeBits |= WPX_ITALICS_BIT;
					break;
				case 0x04:
					//fixme: outline
					break;
				case 0x05:
					//fixme: shadow
					break;
				case 0x10:
					textAttributeBits |= WPX_STRIKEOUT_BIT;
					break;
				case 0x13:
					//fixme: small caps
					break;
				case 0x14:
					//fixme: all caps
					break;
				case 0x16:
					//fixme: emboss
					break;
				case 0x17:
					//fixme: engrave
					break;
				default:
					WPD_DEBUG_MSG(("Works8: error: unknown 0x0A format code 0x%04X\n", rgchProp[x]));
 					throw ParseException();
			}
			continue;
		}

		uint16_t format_code = rgchProp[x] | (rgchProp[x+1] << 8);
		
		switch (format_code)
		{
			case 0x0000:
				break;

			case 0x120F:
				if (1 == rgchProp[x+2])
					textAttributeBits |= WPX_SUPERSCRIPT_BIT;
				if (2 == rgchProp[x+2])
					textAttributeBits |= WPX_SUBSCRIPT_BIT;
				x += 2;
				break;

			case 0x2212:
				x += 2;
				// fixme: always 0x22120409?
				break;

			case 0x121E:
				// fixme: there are various styles of underline
				textAttributeBits |= WPX_UNDERLINE_BIT;
				x += 2;
				break;

			case 0x8A24:
				// fixme: font change
				x++;
				x += rgchProp[x];
				break;

			case 0x220C:
			{
				/* font size */
				uint32_t font_size =  WPD_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
				listener->setFontSize(font_size/12700);
				x += 4;
				break;
			}

			case 0x222E:
				//fixme: color
				x += 4;
				break;
			
			default:
				WPD_DEBUG_MSG(("Works8: error: unknown format code 0x%04X\n", format_code));		
				throw ParseException();
				break;


		}
	}

	propertyChangeDelta(textAttributeBits, listener);
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
	
	uint32_t textAttributeBit = 0;
	switch (attribute)
	{
		case WPS8_ATTRIBUTE_BOLD:
			textAttributeBit = WPX_BOLD_BIT;
			break;
		case WPS8_ATTRIBUTE_ITALICS:
			textAttributeBit = WPX_ITALICS_BIT;		
			break;
		case WPS8_ATTRIBUTE_UNDERLINE:
			textAttributeBit = WPX_UNDERLINE_BIT;		
			break;	
		case WPS8_ATTRIBUTE_STRIKEOUT:
			textAttributeBit = WPX_STRIKEOUT_BIT;		
			break;				
		case WPS8_ATTRIBUTE_SUBSCRIPT:
			textAttributeBit = WPX_SUBSCRIPT_BIT;		
			break;							
		case WPS8_ATTRIBUTE_SUPERSCRIPT:
			textAttributeBit = WPX_SUPERSCRIPT_BIT;		
			break;							
	}
	if (isOn)
		m_ps->m_textAttributeBits |= textAttributeBit;
	else
		m_ps->m_textAttributeBits ^= textAttributeBit;
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
