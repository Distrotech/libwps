/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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

#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include <bitset>
#endif

#include <libwpd-stream/WPXStream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSContentListener.h"
#include "WPSHeader.h"
#include "WPSPageSpan.h"

#include "WPS4.h"

namespace WPS4ParserInternal
{
struct Font
{
	Font(std::string const &name="",
	     libwps_tools_win::Font::Type type = libwps_tools_win::Font::DOS_850) :
		m_name(name), m_type(type) {}

	/** Works version 2 for DOS supports only a specific set of fonts. */
	static std::string getWps2Name(uint8_t font_n);

	/** the font name */
	std::string m_name;
	/** the font type */
	libwps_tools_win::Font::Type m_type;
};


std::string Font::getWps2Name(uint8_t font_n)
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
}

// TODO: Verify exact values
const uint32_t s_colorMap[]=
{
	// 0x00RRGGBB
	0, // auto
	0,
	0x0000FF,
	0x00FFFF,
	0x00FF00,
	0xFF00FF,
	0xFF0000,
	0xFFFF00,
	0x808080,
	0xFFFFFF,
	0x000080,
	0x008080,
	0x008000,
	0x800080,
	0x808000,
	0xC0C0C0
};

}

/*
WPS4Parser public
*/

/**
 * This class parses Works version 2 through 4.
 *
 */
WPS4Parser::WPS4Parser(WPXInputStreamPtr &input, WPSHeaderPtr &header) :
	WPSParser(input, header),
	m_oldTextAttributeBits(0),
	m_offset_eot(0),
	m_CHFODs(),
	m_PAFODs(),
	m_listener(),
	m_fonts(),
	m_worksVersion(header->getMajorVersion())
{
}

WPS4Parser::~WPS4Parser ()
{
}

void WPS4Parser::parse(WPXDocumentInterface *documentInterface)
{
	std::vector<WPSPageSpan> pageList;

	WPS_DEBUG_MSG(("WPS4Parser::parse()\n"));

	/* parse pages */
	parsePages(pageList, getInput());

	/* parse document */
	m_listener.reset(new WPS4ContentListener(pageList, documentInterface));
	parse(getInput());
	m_listener.reset();
}


/*
WPS4Parser private
*/

#ifdef _MSC_VER
#define strcasecmp _strcmpi
#endif


/**
 * Reads fonts table into memory.
 *
 */
void WPS4Parser::readFontsTable(WPXInputStreamPtr &input)
{
	/* offset of FFNTB */
	input->seek(0x5E, WPX_SEEK_SET);
	uint32_t offset_FFNTB = libwps::readU32(input);
	//fixme: sanity check

	/* length of FFNTB */
	input->seek(0x62, WPX_SEEK_SET);
	uint32_t len_FFNTB = libwps::readU16(input);
	uint32_t offset_end_FFNTB = offset_FFNTB + len_FFNTB;
	WPS_DEBUG_MSG(("Works: info: offset_FFNTB=0x%X, len_FFNTB=0x%X, offset_end_FFNTB=0x%X\n",
	               offset_FFNTB, len_FFNTB, offset_end_FFNTB));

	input->seek(offset_FFNTB, WPX_SEEK_SET);

	while (input->tell() < (long)offset_end_FFNTB) //FIXME: be sure we are not overflowing here. Adding cast just to remove warnings
	{

		/* Sometimes the font numbers start at 0 and increment nicely.
		   However, other times the font numbers jump around. */

		uint8_t font_number = libwps::readU8(input);

		if (m_fonts.find(font_number) != m_fonts.end())
		{
			WPS_DEBUG_MSG(("Works: error: at position 0x%lx: font number %i (0x%X) duplicated\n",
			               (input->tell())-2, font_number, font_number));
			throw libwps::ParseException();
		}

		//fixme: what is this byte? maybe a font class
#ifdef DEBUG
		uint8_t unknown_byte = libwps::readU8(input);
#else
		libwps::readU8(input);
#endif

		std::string s;
		for (uint8_t i = libwps::readU8(input); i>0; i--)
		{
			s.append(1, (uint8_t)libwps::readU8(input));
		}
		WPS_DEBUG_MSG(("Works: info: count=%i, font_number=%i, unknown=%i, name=%s\n",
		               int(m_fonts.size()), font_number, unknown_byte, s.c_str()));
		WPS4ParserInternal::Font f;
		f.m_type = libwps_tools_win::Font::getWin3Type(s);
		f.m_name=s;
		m_fonts[font_number] = f;
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
bool WPS4Parser::readFODPage(WPXInputStreamPtr &input, std::vector<WPSFOD> &FODs)
{
	uint32_t page_offset = input->tell();

	input->seek(page_offset + 127, WPX_SEEK_SET);
	uint8_t cfod = libwps::readU8(input); /* number of FODs on this page */
//	WPS_DEBUG_MSG(("Works: info: at position 0x%02x, cfod = %i (%X)\n", (input->tell())-1,cfod, cfod));
	if (cfod > 0x18)
	{
		WPS_DEBUG_MSG(("Works4: error: cfod = %i ", cfod));
		throw libwps::ParseException();
	}
	input->seek(page_offset, WPX_SEEK_SET);

	libwps::readU32(input);

	int first_fod = FODs.size();

	/* Read array of fcLim of FODs.  The fcLim refers to the offset of the
	   last character covered by the formatting. */
	for (int i = 0; i < cfod; i++)
	{
		WPSFOD fod;
		fod.m_fcLim = libwps::readU32(input);
//		WPS_DEBUG_MSG(("Works: info: fcLim = %i (0x%X)\n", fod.m_fcLim, fod.m_fcLim));

		/* check that fcLim is not too large */
		if (fod.m_fcLim > m_offset_eot)
		{
			WPS_DEBUG_MSG(("Works: error: length of 'text selection' %i > "
			               "total text length %i\n", fod.m_fcLim, m_offset_eot));
			throw libwps::ParseException();
		}

		/* check that fcLim is monotonic */
		if (FODs.size() > 0 && FODs.back().m_fcLim > fod.m_fcLim)
		{
			WPS_DEBUG_MSG(("Works: error: character position list must "
			               "be monotonic, but found %i, %i\n", FODs.back().m_fcLim, fod.m_fcLim));
			throw libwps::ParseException();
		}
		FODs.push_back(fod);
	}

	/* Read array of bfprop of FODs.  The bfprop is the offset where
	   the FPROP is located. */
	std::vector<WPSFOD>::iterator FODs_iter;
	for (FODs_iter = FODs.begin()+first_fod; FODs_iter!= FODs.end(); FODs_iter++)
	{
		if ((*FODs_iter).m_fcLim == m_offset_eot)
			break;

		(*FODs_iter).m_bfprop = libwps::readU8(input);

		/* check size of bfprop  */
		if (((*FODs_iter).m_bfprop < (5*cfod) && (*FODs_iter).m_bfprop > 0) ||
		        (*FODs_iter).m_bfprop  > (128 - 1))
		{
			WPS_DEBUG_MSG(("Works: error: size of bfprop is bad "
			               "%i (0x%X)\n", (*FODs_iter).m_bfprop, (*FODs_iter).m_bfprop));
			throw libwps::ParseException();
		}

		(*FODs_iter).m_bfpropAbs = (*FODs_iter).m_bfprop + page_offset;
//		WPS_DEBUG_MSG(("Works: info: bfpropAbs = "
//			"%i (0x%X)\n", (*FODs_iter).m_bfpropAbs, (*FODs_iter).m_bfpropAbs));
	}

	/* Read array of FPROPs.  These contain the actual formatting
	   codes (bold, alignment, etc.) */
	for (FODs_iter = FODs.begin()+first_fod; FODs_iter!= FODs.end(); FODs_iter++)
	{
		if ((*FODs_iter).m_fcLim == m_offset_eot)
			break;

		if (0 == (*FODs_iter).m_bfprop)
		{
			(*FODs_iter).m_fprop.m_cch = 0;
			continue;
		}

		input->seek((*FODs_iter).m_bfpropAbs, WPX_SEEK_SET);
		(*FODs_iter).m_fprop.m_cch = libwps::readU8(input);
		if (0 == (*FODs_iter).m_fprop.m_cch)
		{
			WPS_DEBUG_MSG(("Works: error: 0 == cch at file offset 0x%lx", (input->tell())-1));
			throw libwps::ParseException();
		}
		// fixme: what is largest cch?
		/* generally paragraph cch are bigger than character cch */
		if ((*FODs_iter).m_fprop.m_cch > 93)
		{
			WPS_DEBUG_MSG(("Works: error: cch = %i, too large ", (*FODs_iter).m_fprop.m_cch));
			throw libwps::ParseException();
		}

		for (int i = 0; (*FODs_iter).m_fprop.m_cch > i; i++)
			(*FODs_iter).m_fprop.m_rgchProp.append(1, (uint8_t)libwps::readU8(input));
	}

	/* go to end of page */
	input->seek(page_offset	+ 128, WPX_SEEK_SET);

	return (!FODs.empty() && (m_offset_eot > FODs.back().m_fcLim));
}

/**
 * @param newTextAttributeBits: all the new, current bits (will be compared against old, and old will be discarded).
 *
 */
void WPS4Parser::propertyChangeDelta(uint32_t newTextAttributeBits)
{
	WPS_DEBUG_MSG(("WPS4Parser::propertyChangeDelta(%i)\n", newTextAttributeBits));
	if (newTextAttributeBits == m_oldTextAttributeBits)
		return;

	m_listener->setFontAttributes(newTextAttributeBits);
#ifdef DEBUG
	static uint32_t const listAttributes[6] = { WPS_BOLD_BIT, WPS_ITALICS_BIT, WPS_UNDERLINE_BIT, WPS_STRIKEOUT_BIT, WPS_SUBSCRIPT_BIT, WPS_SUPERSCRIPT_BIT };
	uint32_t diffAttributes = (m_oldTextAttributeBits ^ newTextAttributeBits);
	for (int i = 0; i < 6; i++)
	{
		if (diffAttributes & listAttributes[i])
		{
			WPS_DEBUG_MSG(("WPS4Parser::propertyChangeDelta: attribute %i changed, now = %i\n", i, newTextAttributeBits & listAttributes[i]));
		}
	}
#endif
	m_oldTextAttributeBits = newTextAttributeBits;
}

/**
 * Process a character property change.  The Works format supplies
 * all the character formatting each time there is any change (as
 * opposed to HTML, for example).  In Works 4, the position in
 * in rgchProp is significant (e.g., bold is always in the first byte).
 *
 */
void WPS4Parser::propertyChange(std::string rgchProp, WPS4ParserInternal::Font &font)
{
	/* set default properties */
	uint32_t textAttributeBits = 0;
	uint8_t  colorid = 0;

	m_listener->setFontSize(12);

	if (0 == rgchProp.length())
		return;

//fixme: this method is immature
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

		if (3 <= m_worksVersion)
		{
			if (m_fonts.find(font_n) == m_fonts.end())
			{
				WPS_DEBUG_MSG(("Works: error: encountered font %i (0x%02x) which is not indexed\n",
				               font_n,font_n ));
				throw libwps::ParseException();
			}
			else
			{
				font = m_fonts[font_n];
				m_listener->setTextFont(font.m_name.c_str());
			}
		}
		else if (2 == m_worksVersion)
		{
			font.m_name = WPS4ParserInternal::Font::getWps2Name(font_n);
			m_listener->setTextFont(font.m_name.c_str());
		}
	}
	if (rgchProp.length() >= 4)
	{
		if ((rgchProp[1] &  0x20) && (rgchProp[3] & 0x20))
			textAttributeBits |= WPS_UNDERLINE_BIT;
	}
	if (rgchProp.length() >= 4 && ((uint8_t)rgchProp[4]) > 0)
	{
		m_listener->setFontSize(((uint8_t)rgchProp[4])/2);
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
	if (rgchProp.length() >= 8)
	{
		colorid = (uint8_t)rgchProp[7] & 0xF;
	}
	propertyChangeDelta(textAttributeBits);
	m_listener->setTextColor(WPS4ParserInternal::s_colorMap[colorid]);
}

void WPS4Parser::propertyChangePara(std::string rgchProp)
{
	static const libwps::Justification _align[]=
	{
		libwps::JustificationLeft, libwps::JustificationCenter,
		libwps::JustificationRight, libwps::JustificationFull
	};

	std::vector<WPSTabStop> tabs;
	m_listener->setTabs(tabs);

	int pf = 0;

	if (4 > rgchProp.length()) return;

	unsigned x = 3;
	double margins[5] = {0.0, 0.0, 0.0, 0.0, 0.0}; // left, right, top, bottom, textIndent
	while (x < rgchProp.length())
	{
		char code = rgchProp[x++];

		switch (code)
		{
		case 0x5:	// alignment
		{
			uint8_t a = rgchProp[x];
			if (a < 4)
				m_listener->setParagraphJustification(_align[a]);
			x++;
		}
		break;
		case 0x7:
			pf |= WPS_PARAGRAPH_LAYOUT_NO_BREAK;
			x++;
			break;
		case 0x8:
			pf |= WPS_PARAGRAPH_LAYOUT_WITH_NEXT;
			x++;
			break;
		case 0x11:
			margins[0]=WPS_LE_GET_GUINT16(&rgchProp.c_str()[x])/1440.0;
			x+=2;
			break;
		case 0x12:
			margins[1]=WPS_LE_GET_GUINT16(&rgchProp.c_str()[x])/1440.0;
			x+=2;
			break;
		case 0x14:
			margins[4]=int16_t(WPS_LE_GET_GUINT16(&rgchProp.c_str()[x]))/1440.0;
			x+=2;
			break;
		case 0x16:
			margins[2]=WPS_LE_GET_GUINT16(&rgchProp.c_str()[x])/1440.0;
			x+=2;
			break;
		case 0x17:
			margins[3]=WPS_LE_GET_GUINT16(&rgchProp.c_str()[x])/1440.0;
			x+=2;
			break;
			/*
				case 0x9:
				case 0xA:
				case 0xB:
				case 0xC:
				case 0xD:
				case 0xE:
				case 0x18:
				case 0x1b:
					// noop, skip 1
					c+=2;
					break;
				case 0x15:
					// noop, skip 2
					c+=3;
					break;
			*/
		case 0xF:
		{
			const char *td = rgchProp.c_str();
			uint16_t count;
			WPSTabStop pos;

			count = WPS_LE_GET_GUINT16(&td[x+2]);
			if (count <= 20)
			{
				for (int i=0; i<count; i++)
				{
					char v;
					pos.m_position = WPS_LE_GET_GUINT16(&td[x+4+i*2]) / 1440.0;
					v = td[x+4+count*2+i];
					switch (v&3)
					{
					case 1:
						pos.m_alignment = WPSTabStop::CENTER;
						break;
					case 2:
						pos.m_alignment = WPSTabStop::RIGHT;
						break;
					case 3:
						pos.m_alignment = WPSTabStop::DECIMAL;
						break;
					default:
						pos.m_alignment = WPSTabStop::LEFT;
					}
					// TODO: leader
					tabs.push_back(pos);
				}
				m_listener->setTabs(tabs);
			}
			x+=rgchProp[x]+(rgchProp[x+1]<<8)+1;
		}
		break;
		default:
			WPS_DEBUG_MSG(("Unknown parafmt code: %02X\n",code));
			break;
		}
	}
	m_listener->setParagraphMargin(margins[0], WPS_LEFT);
	m_listener->setParagraphMargin(margins[1], WPS_RIGHT);
	m_listener->setParagraphMargin(margins[2], WPS_TOP);
	m_listener->setParagraphMargin(margins[3], WPS_BOTTOM);
	m_listener->setParagraphTextIndent(margins[4]);
}

/**
 * Read the text of the document using previously-read
 * formatting information.
 *
 */
void WPS4Parser::readText(WPXInputStreamPtr &input)
{
	m_oldTextAttributeBits = 0;
	std::vector<WPSFOD>::iterator FODs_iter;
	std::vector<WPSFOD>::iterator PFOD_iter;
#if 0
	/* dump for debugging */
	for (FODs_iter = m_CHFODs.begin(); FODs_iter!= m_CHFODs.end(); FODs_iter++)
	{
		WPSFOD fod = *(FODs_iter);
		WPS_DEBUG_MSG(("FOD  fcLim=%u (0x%04x), bfprop=%u, bfpropAbs=%u\n", fod.m_fcLim, fod.m_fcLim, fod.m_bfprop, fod.m_bfpropAbs));
	}
#endif

	uint32_t last_fcLim = 0x100;
	PFOD_iter = m_PAFODs.begin();
	WPS4ParserInternal::Font font;
	font.m_type = m_worksVersion <= 2 ?
	              libwps_tools_win::Font::DOS_850 : libwps_tools_win::Font::WIN3_WEUROPE;
	for (FODs_iter = m_CHFODs.begin(); FODs_iter!= m_CHFODs.end(); FODs_iter++)
	{
		uint32_t len1 = (*FODs_iter).m_fcLim - last_fcLim;
		do
		{
			uint32_t len2 = (*PFOD_iter).m_fcLim - last_fcLim;
			uint32_t len = len1;
			if (len2 < len) len=len2;
			WPS_DEBUG_MSG(("Works: info: txt len=%02i rgchProp=%s\n",
			               len, to_bits((*FODs_iter).m_fprop.m_rgchProp).c_str()));

			if ((*FODs_iter).m_fprop.m_cch > 0)
				propertyChange((*FODs_iter).m_fprop.m_rgchProp, font);
			if ((*PFOD_iter).m_fprop.m_cch > 0)
				propertyChangePara((*PFOD_iter).m_fprop.m_rgchProp);
			input->seek(last_fcLim, WPX_SEEK_SET);
			for (uint32_t i = len; i>0; i--)
			{
				uint8_t readVal = libwps::readU8(input);
				if (0x00 == readVal)
					break;

				switch (readVal)
				{
				case 0x01:
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

				case 0x02:
					// TODO: check special bit
					m_listener->insertField(WPSContentListener::PageNumber);
					break;

				case 0x0C:
					m_listener->insertBreak(WPS_PAGE_BREAK);
					break;

				case 0x0D:
					m_listener->insertEOL();
					break;

				default:
				{
					unsigned long unicode =
					    libwps_tools_win::Font::unicode(readVal, font.m_type);
					if (unicode != 0xfffd)
						m_listener->insertUnicode(unicode);
					break;
				}
				}
			}

			len1 -= len;
			len2 -= len;

			if (len2 == 0) PFOD_iter++;

			last_fcLim += len;
		}
		while (len1 > 0);
	}
}


/**
 * Read the page format from the file.  It seems that WPS4 files
 * can only have one page format throughout the whole document.
 *
 */
void WPS4Parser::parsePages(std::vector<WPSPageSpan> &pageList, WPXInputStreamPtr &input)
{
	/* read page format */
	input->seek(0x64, WPX_SEEK_SET);
	uint16_t margin_top = libwps::readU16(input);
	//input->seek(0x66, WPX_SEEK_SET);
	uint16_t margin_bottom = libwps::readU16(input);
	//input->seek(0x68, WPX_SEEK_SET);
	uint16_t margin_left = libwps::readU16(input);
	//input->seek(0x6A, WPX_SEEK_SET);
	uint16_t margin_right = libwps::readU16(input);
	//input->seek(0x6C, WPX_SEEK_SET);
	uint16_t page_height = libwps::readU16(input);
	//input->seek(0x6E, WPX_SEEK_SET);
	uint16_t page_width = libwps::readU16(input);
	input->seek(0x7A, WPX_SEEK_SET);
	uint8_t page_orientation = libwps::readU8(input);

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
		throw libwps::ParseException();
	}

	if (page_orientation != 0 && page_orientation != 1)
	{
		WPS_DEBUG_MSG(("Works: error: bad page orientation code\n"));
		throw libwps::ParseException();
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
		ps.setFormOrientation(WPSPageSpan::PORTRAIT);
	else
		ps.setFormOrientation(WPSPageSpan::LANDSCAPE);

	pageList.push_back(ps);

	/* process page breaks */
	input->seek(0x100, WPX_SEEK_SET);
	uint8_t ch;
	while (!input->atEOS() && 0x00 != (ch = libwps::readU8(input)))
	{
		if (0x0C == ch)
			pageList.push_back(ps);
	}
}

void WPS4Parser::parse(WPXInputStreamPtr &input)
{
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("WPS4Parser::parse(): the listener is not set, stop parsing\n"));
		return;
	}
	WPS_DEBUG_MSG(("WPS4Parser::parse()\n"));

	m_listener->startDocument();

	/* find beginning of character FODs */
	int const fcmacOffset=0x26;
	input->seek(fcmacOffset, WPX_SEEK_SET);
	m_offset_eot = libwps::readU32(input); /* stream offset to end of text */
	WPS_DEBUG_MSG(("Works: info: location WPS4_FCMAC_OFFSET (0x%X) has offset_eot = 0x%X (%i)\n",
	               fcmacOffset, m_offset_eot, m_offset_eot));
	uint32_t pnChar = (m_offset_eot+127)/128; /* page number of character information */
	WPS_DEBUG_MSG(("Works: info: 128*pnChar = 0x%X (%i)\n", pnChar*128, pnChar*128));

	/* sanity check */
	if (0 == pnChar)
	{
		WPS_DEBUG_MSG(("Works: error: pnChar is 0, so file may be corrupt\n"));
		throw libwps::ParseException();
	}
	input->seek(128*pnChar, WPX_SEEK_SET);
	uint32_t v = libwps::readU32(input);
	if (0x0100 != v)
	{
		WPS_DEBUG_MSG(("Works: warning: expected value 0x0100 at location 0x%X but got 0x%X (%i)\n",
		               128*pnChar, v, v));
	}

	/* go to beginning of character FODs */
	input->seek(128*pnChar, WPX_SEEK_SET);

	/* read character FODs */
	while (readFODPage(input, m_CHFODs))
	{
	}
	// fixme: verify: final FOD covers end of text

	/* read paragraph formatting */
	while (readFODPage(input, m_PAFODs))
	{
	}

	/* read fonts table */
	if ( m_worksVersion > 2)
		readFontsTable(input);

	/* process text file using previously-read character formatting */
	readText(input);

	m_listener->endDocument();
}

#ifdef DEBUG
std::string WPS4Parser::to_bits(std::string s)
{
	std::string r;
	for (unsigned int i = 0; i < s.length(); i++)
	{
		std::bitset<8> b(s[i]);
		r.append(b.to_string<char,std::char_traits<char>,std::allocator<char> >());
		WPXString buf;
		buf.sprintf("(%02u,0x%02x)  ", (uint8_t)s[i],(uint8_t)s[i]);
		r.append(buf.cstr());
	}
	return r;
}
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
