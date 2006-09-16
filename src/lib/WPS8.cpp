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
		WPD_DEBUG_MSG(("FOD  fcLim=%u (0x%04x), bfprop=%u, bfprop_abs=%u\n", 
			fod.fcLim, fod.fcLim, fod.bfprop, fod.bfprop_abs));
		uint32_t len = (*FODs_iter).fcLim - last_fcLim;
		if (len % 2 != 0)
		{
			WPD_DEBUG_MSG(("Works: error: len %i is odd\n", len));
			throw ParseException();
		}
		len /= 2;
		WPD_DEBUG_MSG(("Works: info: txt len=%02i rgchProp=%s\n", 
			len, to_bits((*FODs_iter).fprop.rgchProp).c_str()));
		if ((*FODs_iter).fprop.cch > 0)
			propertyChange((*FODs_iter).fprop.rgchProp, listener);
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
	// fixme: is the cfod at the end of the page ?
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
//		WPD_DEBUG_MSG(("Works: debug: bfprop = %X, bfprop_abs = %x\n",
//                      (*FODs_iter).bfprop, (*FODs_iter).bfprop_abs));
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

#if 0
	WPD_DEBUG_MSG(("Works8: info: header index entry %s with unknown1=%s\n", name.c_str(),
		to_bits(unknown1).c_str()));
#endif

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

	/* Below is temporary code to simply grab the plain text. */
#if 0
	input->seek(0x34, WPX_SEEK_SET);		
	size_t textLength = readU32(input);
	if (0 == textLength)
	{
		WPD_DEBUG_MSG(("Works: error: no text\n"));
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
#endif
	
	listener->endDocument();		
}


/**
 * Process a character property change.  The Works format supplies
 * all the character formatting each time there is any change (as
 * opposed to HTML, for example).
 *
 */
void WPS8Parser::propertyChange(std::string rgchProp, WPS8Listener *listener)
{
	if (0 == rgchProp.length())
		return;
	//fixme: stub
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
