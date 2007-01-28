/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "WPS4.h"
#include "WPSDocument.h"
#include "WPSStream.h"
#include "WPSHeader.h"
#include "libwps_internal.h"


#define WPS4_FCMAC_OFFSET 0x26	
#define WPS4_TEXT_OFFSET 0x100

/*
WPS4Parser public
*/


/**
 * This class parses Works version 2 through 4.
 *
 */
WPS4Parser::WPS4Parser(WPSInputStream *input, WPSHeader * header) :
	WPSParser(input, header),
	m_worksVersion(header->getMajorVersion())
{
}

WPS4Parser::~WPS4Parser ()
{
}

void WPS4Parser::parse(WPXHLListenerImpl *listenerImpl)
{
	std::list<WPSPageSpan> pageList;
	
	WPS_DEBUG_MSG(("WPS4Parser::parse()\n"));		

	WPSInputStream *input = getInput();		

	/* parse pages */
	WPSPageSpan m_currentPage;
	parsePages(pageList, input);		
	
	/* parse document */
	WPS4ContentListener listener(pageList, listenerImpl);
	parse(input, &listener);	
}


/*
WPS4Parser private
*/


/**
 * Reads fonts table into memory.
 *
 */
void WPS4Parser::readFontsTable(WPSInputStream * input)
{
	/* offset of FFNTB */
	input->seek(0x5E, WPX_SEEK_SET);
	uint32_t offset_FFNTB = readU32(input);
	//fixme: sanity check
	
	/* length of FFNTB */
	input->seek(0x62, WPX_SEEK_SET);	
	uint32_t len_FFNTB = readU16(input);	
	uint32_t offset_end_FFNTB = offset_FFNTB + len_FFNTB;		
	WPS_DEBUG_MSG(("Works: info: offset_FFNTB=0x%X, len_FFNTB=0x%X, offset_end_FFNTB=0x%X\n",
		offset_FFNTB, len_FFNTB, offset_end_FFNTB));
		
	input->seek(offset_FFNTB, WPX_SEEK_SET);

	while (input->tell() < offset_end_FFNTB)
	{

		/* Sometimes the font numbers start at 0 and increment nicely.
		   However, other times the font numbers jump around. */

		uint8_t font_number = readU8(input);
		
		if (fonts.find(font_number) != fonts.end())
		{
			WPS_DEBUG_MSG(("Works: error: at position 0x%lx: font number %i (0x%X) duplicated\n",
				(input->tell())-2, font_number, font_number));		
			throw ParseException();		
		}

		//fixme: what is this byte? maybe a font class
#ifdef DEBUG
		uint8_t unknown_byte = readU8(input);
#else
		readU8(input);
#endif

		std::string s;
		for (uint8_t i = readU8(input); i>0; i--)
		{
			s.append(1, (uint8_t)readU8(input));
		}
		WPS_DEBUG_MSG(("Works: info: count=%i, font_number=%i, unknown=%i, name=%s\n",
			 fonts.size(), font_number, unknown_byte, s.c_str()));
		s.append(1, (char)0);
		fonts[font_number] = s;
	}
}

/**
 * Read a single "page" (128 bytes) that contains formatting descriptors (FODs)
 * for either  characters OR paragraphs.  Starts reading at current position in
 * stream.
 *
 * Return: true if more pages of this type exist, otherwise false
 *
 */
bool WPS4Parser::readFODPage(WPSInputStream * input, std::vector<FOD> * FODs)
{
	uint32_t page_offset = input->tell();

	input->seek(page_offset + 127, WPX_SEEK_SET);	
	uint8_t cfod = readU8(input); /* number of FODs on this page */
//	WPS_DEBUG_MSG(("Works: info: at position 0x%02x, cfod = %i (%X)\n", (input->tell())-1,cfod, cfod));				
	if (cfod > 0x18)
	{
		WPS_DEBUG_MSG(("Works4: error: cfod = %i ", cfod));
		throw ParseException();
	}
	input->seek(page_offset, WPX_SEEK_SET);	
	
	uint32_t fcFirst; /* Byte number of first character covered by this page 
			     of formatting information */	
			    
	fcFirst = readU32(input);
	
	int first_fod = FODs->size();

        /* Read array of fcLim of FODs.  The fcLim refers to the offset of the
           last character covered by the formatting. */
	for (int i = 0; i < cfod; i++)
	{
		FOD fod;
		fod.fcLim = readU32(input);
//		WPS_DEBUG_MSG(("Works: info: fcLim = %i (0x%X)\n", fod.fcLim, fod.fcLim));			
		
		/* check that fcLim is not too large */
		if (fod.fcLim > offset_eot)
		{
			WPS_DEBUG_MSG(("Works: error: length of 'text selection' %i > "
				"total text length %i\n", fod.fcLim, offset_eot));					
			throw ParseException();	
		}

		/* check that fcLim is monotonic */
		if (FODs->size() > 0 && FODs->back().fcLim > fod.fcLim)
		{
			WPS_DEBUG_MSG(("Works: error: character position list must "
				"be monotonic, but found %i, %i\n", FODs->back().fcLim, fod.fcLim));
			throw ParseException();
		}
		FODs->push_back(fod);
	} 	
	
	/* Read array of bfprop of FODs.  The bfprop is the offset where 
	   the FPROP is located. */
	std::vector<FOD>::iterator FODs_iter;
	for (FODs_iter = FODs->begin()+first_fod; FODs_iter!= FODs->end(); FODs_iter++)
	{
		if ((*FODs_iter).fcLim == offset_eot)
			break;
	
		(*FODs_iter).bfprop = readU8(input);
		
		/* check size of bfprop  */
		if (((*FODs_iter).bfprop < (5*cfod) && (*FODs_iter).bfprop > 0) ||
			 (*FODs_iter).bfprop  > (128 - 1))
		{
			WPS_DEBUG_MSG(("Works: error: size of bfprop is bad "
				"%i (0x%X)\n", (*FODs_iter).bfprop, (*FODs_iter).bfprop));		
			throw ParseException();
		}
		
		(*FODs_iter).bfprop_abs = (*FODs_iter).bfprop + page_offset;
//		WPS_DEBUG_MSG(("Works: info: bfprop_abs = "
//			"%i (0x%X)\n", (*FODs_iter).bfprop_abs, (*FODs_iter).bfprop_abs));		
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
			continue;
		}

		input->seek((*FODs_iter).bfprop_abs, WPX_SEEK_SET);
		(*FODs_iter).fprop.cch = readU8(input);
		if (0 == (*FODs_iter).fprop.cch)
		{
			WPS_DEBUG_MSG(("Works: error: 0 == cch at file offset 0x%lx", (input->tell())-1));
			throw ParseException();
		}
		// fixme: what is largest cch?
		/* generally paragraph cch are bigger than character cch */
		if ((*FODs_iter).fprop.cch > 93)
		{
			WPS_DEBUG_MSG(("Works: error: cch = %i, too large ", (*FODs_iter).fprop.cch));
			throw ParseException();
		}

		for (int i = 0; (*FODs_iter).fprop.cch > i; i++)
			(*FODs_iter).fprop.rgchProp.append(1, (uint8_t)readU8(input));
	}
	
	/* go to end of page */
	input->seek(page_offset	+ 128, WPX_SEEK_SET);

	return (!FODs->empty() && (offset_eot > FODs->back().fcLim));
}

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
		WPS_DEBUG_MSG(("WPS4Parser::propertyChangeDelta: attribute %i changed, now = %i\n", attribute, newTextAttributeBits & bit));	
		listener->attributeChange(newTextAttributeBits & bit, attribute);
	}
}

/**
 * @param newTextAttributeBits: all the new, current bits (will be compared against old, and old will be discarded).
 *
 */
void WPS4Parser::propertyChangeDelta(uint32_t newTextAttributeBits, WPS4Listener *listener)
{
	WPS_DEBUG_MSG(("WPS4Parser::propertyChangeDelta(%i, %p)\n", newTextAttributeBits, (void *)listener));	
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_BOLD, WPS_BOLD_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_ITALICS, WPS_ITALICS_BIT, listener);	
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_UNDERLINE, WPS_UNDERLINE_BIT, listener);		
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_STRIKEOUT, WPS_STRIKEOUT_BIT, listener);	
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_SUBSCRIPT, WPS_SUBSCRIPT_BIT, listener);		
	propertyChangeTextAttribute(newTextAttributeBits, WPS4_ATTRIBUTE_SUPERSCRIPT, WPS_SUPERSCRIPT_BIT, listener);		
	oldTextAttributeBits = newTextAttributeBits;
}

/**
 * Works version 2 for DOS supports only a specific set of fonts.
 *
 */
char * WPS2FontNameFromIndex(uint8_t font_n)
{
	switch (font_n)
	{
		case 0:
			return "Courier";

		case 1:
			return "Courier PC";

		case 3:
			return "Univers_Scale";

		case 4:
			return "Universe";

		case 6:
			return "LinePrinterPC";

		case 7:
			return "LinePrinter";

		case 16:
			return "CGTimes_Scale";

		case 24:
			return "CGTimes";
			break;
	}

	WPS_DEBUG_MSG(("Works: error: encountered unknown font %i (0x%02x)ed\n", 
		font_n,font_n ));
	return "Courier"; //temporary fixme
	throw ParseException();
}

/**
 * Process a character property change.  The Works format supplies
 * all the character formatting each time there is any change (as
 * opposed to HTML, for example).  In Works 4, the position in
 * in rgchProp is significant (e.g., bold is always in the first byte).
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
		textAttributeBits |= WPS_BOLD_BIT;
	}
	if (rgchProp[0] & 0x02)
	{
		textAttributeBits |= WPS_ITALICS_BIT;
	}
	if (rgchProp[0] & 0x04)
	{
		textAttributeBits |= WPS_STRIKEOUT_BIT;
	}
	if (rgchProp.length() >= 3)
	{
		/* text font */
		uint8_t font_n = (uint8_t)rgchProp[2];

		if (3 == getHeader()->getMajorVersion())
		{
			if (fonts.find(font_n) == fonts.end())
			{
				WPS_DEBUG_MSG(("Works: error: encountered font %i (0x%02x) which is not indexed\n", 
					font_n,font_n ));
				throw ParseException();
			}
			else
				listener->setTextFont(fonts[font_n].c_str());
		}
		if (2 == getHeader()->getMajorVersion())
		{
			listener->setTextFont(WPS2FontNameFromIndex(font_n));
		}
	}
	if (rgchProp.length() >= 4)
	{
		if ((rgchProp[1] &  0x20) && (rgchProp[3] & 0x20))
			textAttributeBits |= WPS_UNDERLINE_BIT;
	}
	if (rgchProp.length() >= 4 && ((uint8_t)rgchProp[4]) > 0)
	{
		listener->setFontSize(((uint8_t)rgchProp[4])/2);
	}
	else
	{
		/* default font size */
		listener->setFontSize(12);
	}
	if (rgchProp.length() >= 6)
	{
		WPS_DEBUG_MSG(("rgchProp[1] & 0x40 = %i, rgchProp[5]=%i\n", (rgchProp[1] & 0x40), (uint8_t)rgchProp[5]));
		if ((rgchProp[1] & 0x40) && (1 == (uint8_t)rgchProp[5]))
		{
			textAttributeBits |= WPS_SUPERSCRIPT_BIT;			
		}
		
		if ((rgchProp[1] & 0x40) && (128 == (uint8_t)rgchProp[5]))
		{
			textAttributeBits |= WPS_SUBSCRIPT_BIT;					
		}
	}
	propertyChangeDelta(textAttributeBits, listener);
}
/**
 * Take a character in CP850 encoding, convert it
 * and append it to the text buffer as UTF8.
 * Courtesy of glib2 and iconv
 *
 */
void WPS4Parser::appendCP850(const uint8_t readVal, WPS4Listener *listener)
{
// Fridrich: I see some MS Works files with IBM850 encoding ???
	static const uint16_t cp850toUCS4[128] = {
		0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
		0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
		0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
		0x00ff, 0x00d6, 0x00dc, 0x00f8, 0x00a3, 0x00d8, 0x00d7, 0x0192,
		0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
		0x00bf, 0x00ae, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
		0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00c1, 0x00c2, 0x00c0,
		0x00a9, 0x2563, 0x2551, 0x2557, 0x255d, 0x00a2, 0x00a5, 0x2510,
		0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x00e3, 0x00c3,
		0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x00a4,
		0x00f0, 0x00d0, 0x00ca, 0x00cb, 0x00c8, 0x0131, 0x00cd, 0x00ce,
		0x00cf, 0x2518, 0x250c, 0x2588, 0x2584, 0x00a6, 0x00cc, 0x2580,
		0x00d3, 0x00df, 0x00d4, 0x00d2, 0x00f5, 0x00d5, 0x00b5, 0x00fe,
		0x00de, 0x00da, 0x00db, 0x00d9, 0x00fd, 0x00dd, 0x00af, 0x00b4,
		0x00ad, 0x00b1, 0x2017, 0x00be, 0x00b6, 0x00a7, 0x00f7, 0x00b8,
		0x00b0, 0x00a8, 0x00b7, 0x00b9, 0x00b3, 0x00b2, 0x25a0, 0x00a0 };

	uint32_t ucs4Character = 0;
	if (readVal < 0x80)
		ucs4Character = (uint32_t) readVal;
	else {
		ucs4Character = (uint32_t) cp850toUCS4[readVal - 0x80];
	}

	uint8_t first;
	int len;
	if (ucs4Character < 0x80) {
		first = 0; len = 1;
	}
	else if (ucs4Character < 0x800) {
		first = 0xc0; len = 2;
	}
	else if (ucs4Character < 0x10000) {
		first = 0xe0; len = 3;
	}
	else if (ucs4Character < 0x200000) {
		first = 0xf0; len = 4;
	}
	else if (ucs4Character < 0x4000000) {
		first = 0xf8; len = 5;
	}
	else {
		first = 0xfc; len = 6;
	}

	uint8_t outbuf[6] = { 0, 0, 0, 0, 0, 0 };
	int i;
	for (i = len - 1; i > 0; --i) {
		outbuf[i] = (ucs4Character & 0x3f) | 0x80;
		ucs4Character >>= 6;
	}
	outbuf[0] = ucs4Character | first;

	for (i = 0; i < len; i++)
		listener->insertCharacter(outbuf[i]);
}

/**
 * Take a character in CP1252 encoding, convert it
 * and append it to the text buffer as UTF8.
 * Courtesy of glib2 and iconv
 *
 */
void WPS4Parser::appendCP1252(const uint8_t readVal, WPS4Listener *listener)
{
	static const uint16_t cp1252toUCS4[32] = {
		0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
		0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0xfffd, 0x017d, 0xfffd,
		0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
		0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0xfffd, 0x017e, 0x0178 };

	uint32_t ucs4Character = 0;
	if (readVal < 0x80 || readVal >= 0xa0)
		ucs4Character = (uint32_t) readVal;
	else {
		ucs4Character = (uint32_t) cp1252toUCS4[readVal - 0x80];
		if (ucs4Character == 0xfffd)
//			throw GenericException();
			return;
	}

	uint8_t first;
	int len;
	if (ucs4Character < 0x80) {
		first = 0; len = 1;
	}
	else if (ucs4Character < 0x800) {
		first = 0xc0; len = 2;
	}
	else if (ucs4Character < 0x10000) {
		first = 0xe0; len = 3;
	}
	else if (ucs4Character < 0x200000) {
		first = 0xf0; len = 4;
	}
	else if (ucs4Character < 0x4000000) {
		first = 0xf8; len = 5;
	}
	else {
		first = 0xfc; len = 6;
	}

	uint8_t outbuf[6] = { 0, 0, 0, 0, 0, 0 };
	int i;
	for (i = len - 1; i > 0; --i) {
		outbuf[i] = (ucs4Character & 0x3f) | 0x80;
		ucs4Character >>= 6;
	}
	outbuf[0] = ucs4Character | first;

	for (i = 0; i < len; i++)
		listener->insertCharacter(outbuf[i]);
}

/**
 * Read the text of the document using previously-read
 * formatting information.
 *
 */
void WPS4Parser::readText(WPSInputStream * input, WPS4Listener *listener)
{
	oldTextAttributeBits = 0;
	std::vector<FOD>::iterator FODs_iter;	
#if 1
	/* dump for debugging */
	for (FODs_iter = CHFODs.begin(); FODs_iter!= CHFODs.end(); FODs_iter++)
	{
		FOD fod = *(FODs_iter);
//		WPS_DEBUG_MSG(("FOD  fcLim=%u (0x%04x), bfprop=%u, bfprop_abs=%u\n", fod.fcLim, fod.fcLim, fod.bfprop, fod.bfprop_abs));
	}	
#endif

	uint32_t last_fcLim = 0x100;
	for (FODs_iter = CHFODs.begin(); FODs_iter!= CHFODs.end(); FODs_iter++)
	{
		uint32_t len = (*FODs_iter).fcLim - last_fcLim;
		WPS_DEBUG_MSG(("Works: info: txt len=%02i rgchProp=%s\n", 
			len, to_bits((*FODs_iter).fprop.rgchProp).c_str()));
		if ((*FODs_iter).fprop.cch > 0)
			propertyChange((*FODs_iter).fprop.rgchProp, listener);
		input->seek(last_fcLim, WPX_SEEK_SET);			
		for (uint32_t i = len; i>0; i--)
		{
			uint8_t readVal = readU8(input);
			if (0x00 == readVal)
				break;
				
			switch (readVal)
			{
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x04:
				case 0x05:
				case 0x06:
				case 0x07:
				case 0x08:
				case 0x09:
				case 0x0A:
				case 0x0B:
				case 0x0E:
				case 0x0F:
				case 0x10:
				case 0x11:
				case 0x12:
				case 0x13:
				case 0x14:
				case 0x15:
				case 0x16:
				case 0x17:
				case 0x18:
				case 0x19:
				case 0x1A:
				case 0x1B:
				case 0x1C:
				case 0x1D:
				case 0x1E:
				case 0x1F:
					break;

				case 0x0C:
					listener->insertBreak(WPS_PAGE_BREAK);
					break;

				case 0x0D:
					listener->insertEOL();
					break;

				default:
					if (m_worksVersion == 2)
						this->appendCP850(readVal, listener);
					else
						this->appendCP1252(readVal, listener);
					break;
			}
		}	
		last_fcLim = (*FODs_iter).fcLim;	
	}
}


/**
 * Read the page format from the file.  It seems that WPS4 files
 * can only have one page format throughout the whole document.
 *
 */
void WPS4Parser::parsePages(std::list<WPSPageSpan> &pageList, WPSInputStream *input)
{
	/* read page format */
	input->seek(0x64, WPX_SEEK_SET);
	uint16_t margin_top = readU16(input);
	input->seek(0x66, WPX_SEEK_SET);
	uint16_t margin_bottom = readU16(input);
	input->seek(0x68, WPX_SEEK_SET);
	uint16_t margin_left = readU16(input);
	input->seek(0x6A, WPX_SEEK_SET);
	uint16_t margin_right = readU16(input);
	input->seek(0x6C, WPX_SEEK_SET);
	uint16_t page_height = readU16(input);
	input->seek(0x6E, WPX_SEEK_SET);
	uint16_t page_width = readU16(input);
	input->seek(0x7A, WPX_SEEK_SET);
	uint8_t page_orientation = readU8(input);

	/* convert units */
	float margin_top_inches = (float)margin_top / (float)1440;
	float margin_left_inches = (float)margin_left / (float)1440;
	float margin_right_inches = (float)margin_right / (float)1440;
	float margin_bottom_inches = (float)margin_bottom / (float)1440;
	float page_width_inches = (float)page_width / (float)1440;
	float page_height_inches = (float)page_height / (float)1440;
		
	/* check page format */
	WPS_DEBUG_MSG(("Works: info: page margins (t,l,r,b): raw(%i,%i,%i,%i), inches(%f,%f,%f,%f)\n",
		margin_top, margin_left, margin_right, margin_bottom,
		margin_top_inches, margin_left_inches, margin_right_inches, margin_bottom_inches));

	WPS_DEBUG_MSG(("Works: info: page size (w,h): raw(%i,%i), inches(%2.1f,%2.1f)\n",
		page_width, page_height, page_width_inches, page_height_inches));

	if ((margin_left_inches + margin_right_inches) > page_width_inches
		|| (margin_top_inches + margin_bottom_inches) > page_height_inches)
	{
		WPS_DEBUG_MSG(("Works: error: the margins are too large for the page size\n"));
		throw ParseException();
	}
	
	if (page_orientation != 0 && page_orientation != 1)
	{
		WPS_DEBUG_MSG(("Works: error: bad page orientation code\n"));	
		throw ParseException();
	}
		
	/* record page format */
	WPSPageSpan ps;
	ps.setMarginTop(margin_top_inches);
	ps.setMarginBottom(margin_bottom_inches);	
	ps.setMarginLeft(margin_left_inches);		
	ps.setMarginRight(margin_right_inches);			
	ps.setFormLength(page_height_inches);
	ps.setFormWidth(page_width_inches);
	if (0 == page_orientation)
		ps.setFormOrientation(PORTRAIT);
	else
		ps.setFormOrientation(LANDSCAPE);
	
	pageList.push_back(ps);
	
	/* process page breaks */
	input->seek(0x100, WPX_SEEK_SET);
	uint8_t ch;
	while (!input->atEOS() && 0x00 != (ch = readU8(input)))
	{
		if (0x0C == ch)
			pageList.push_back(ps);			
	}
}

void WPS4Parser::parse(WPSInputStream *input, WPS4Listener *listener)
{
	WPS_DEBUG_MSG(("WPS4Parser::parse()\n"));	

	listener->startDocument();

	/* find beginning of character FODs */
	input->seek(WPS4_FCMAC_OFFSET, WPX_SEEK_SET);
	offset_eot = readU32(input); /* stream offset to end of text */
	WPS_DEBUG_MSG(("Works: info: location WPS4_FCMAC_OFFSET (0x%X) has offset_eot = 0x%X (%i)\n", 
		WPS4_FCMAC_OFFSET, offset_eot, offset_eot));			
	uint32_t pnChar = (offset_eot+127)/128; /* page number of character information */
	WPS_DEBUG_MSG(("Works: info: 128*pnChar = 0x%X (%i)\n", pnChar*128, pnChar*128));
	
	/* sanity check */
	if (0 == pnChar)
	{
		WPS_DEBUG_MSG(("Works: error: pnChar is 0, so file may be corrupt\n"));
		throw ParseException();
	}
	input->seek(128*pnChar, WPX_SEEK_SET);
	uint32_t v = readU32(input);
	if (0x0100 != v)
	{
		WPS_DEBUG_MSG(("Works: warning: expected value 0x0100 at location 0x%X but got 0x%X (%i)\n", 
			128*pnChar, v, v));
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
	if ( getHeader()->getMajorVersion() > 2)
		readFontsTable(input);
	
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
	WPS_DEBUG_MSG(("~WPS4ContentParsingState:: m_textBuffer.len() =%i\n", m_textBuffer.len()));		
	m_textBuffer.clear();
}


/*
WPS4ContentListener public
*/

WPS4ContentListener::WPS4ContentListener(std::list<WPSPageSpan> &pageList, WPXHLListenerImpl *listenerImpl) :
	WPS4Listener(),
	WPSContentListener(pageList, listenerImpl),
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
	WPS_DEBUG_MSG(("STUB WPS4ContentListener::insertTab()\n"));		
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
	WPS_DEBUG_MSG(("WPS4ContentListener::attributeChange(%i, %i)\n", isOn, attribute));		
	_closeSpan();
	
	uint32_t textAttributeBit = 0;
	switch (attribute)
	{
		case WPS4_ATTRIBUTE_BOLD:
			textAttributeBit = WPS_BOLD_BIT;
			break;
		case WPS4_ATTRIBUTE_ITALICS:
			textAttributeBit = WPS_ITALICS_BIT;		
			break;
		case WPS4_ATTRIBUTE_UNDERLINE:
			textAttributeBit = WPS_UNDERLINE_BIT;		
			break;	
		case WPS4_ATTRIBUTE_STRIKEOUT:
			textAttributeBit = WPS_STRIKEOUT_BIT;		
			break;				
		case WPS4_ATTRIBUTE_SUBSCRIPT:
			textAttributeBit = WPS_SUBSCRIPT_BIT;		
			break;							
		case WPS4_ATTRIBUTE_SUPERSCRIPT:
			textAttributeBit = WPS_SUPERSCRIPT_BIT;		
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
void WPS4ContentListener::columnChange(const WPSTextColumnType columnType, const uint8_t numColumns, const std::vector<float> &columnWidth,
				const std::vector<bool> &isFixedWidth) {}

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
void WPS4ContentListener::insertNote(const WPSNoteType noteType) {}
void WPS4ContentListener::suppressPage(const uint16_t suppressCode) {}


/*
WPS4ContentListener protected 
*/

void WPS4ContentListener::_openParagraph()
{
//	WPS_DEBUG_MSG(("STUB WPS4ContentListener::_openParagraph()\n"));		
	WPSContentListener::_openParagraph();
}

void WPS4ContentListener::_flushText()
{
	if (m_parseState->m_textBuffer.len())
		m_listenerImpl->insertText(m_parseState->m_textBuffer);
	m_parseState->m_textBuffer.clear();
}
