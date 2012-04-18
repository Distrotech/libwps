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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include <bitset>
#endif

#include <libwpd-stream/WPXStream.h>

#include "libwps_internal.h"

#include "WPSDocument.h"
#include "WPSHeader.h"

#include "WPS4.h"

#define WPS4_FCMAC_OFFSET 0x26
#define WPS4_TEXT_OFFSET 0x100

namespace WPS4ParserInternal
{
struct FontName
{
	FontName() : m_name(), m_cp(0) {}

	std::string m_name;
	int m_cp;
};

static const struct FontMapping
{
	const char *m_name;
	const char *m_repl;
	int m_codepage;
} s_fontMapping[] =
{
	// Built-in Windows
	{"Arial CYR","Arial",1251},
	{"Arial CE","Arial",1250},
	{"Courier New CYR","Courier New",1251},
	{"Courier New CE","Courier New",1250},
	{"Times New Roman CYR","Times New Roman",1251},
	{"Times New Roman CE","Times New Roman",1250},

	// Custom
	{"Baltica",NULL,1251},
	{"Pragmatica",NULL,1251}
};
/**
Maps some of legacy fonts to modern ones and set encoding accordingly.
\param[in,out] str May contain mapped font name
*/

int getCodepage(std::string &str)
{
	const char *s = str.c_str();
	for (unsigned i=0; i < (sizeof(s_fontMapping)/sizeof(s_fontMapping[0])); i++)
	{
		if (!strcasecmp(s,s_fontMapping[i].m_name))
		{
			if (s_fontMapping[i].m_repl) str = s_fontMapping[i].m_repl;
			return s_fontMapping[i].m_codepage;
		}
	}
	return 0;
}

/**
 * Works version 2 for DOS supports only a specific set of fonts.
 *
 */
const char *getWps2FontNameFromIndex(uint8_t font_n)
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
	m_fonts(),
	m_worksVersion(header->getMajorVersion())
{
}

WPS4Parser::~WPS4Parser ()
{
}

void WPS4Parser::parse(WPXDocumentInterface *documentInterface)
{
	std::list<WPSPageSpan> pageList;

	WPS_DEBUG_MSG(("WPS4Parser::parse()\n"));

	WPXInputStream *input = getInput().get();

	/* parse pages */
	parsePages(pageList, input);

	/* parse document */
	WPS4ContentListener listener(pageList, documentInterface);
	parse(input, &listener);
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
void WPS4Parser::readFontsTable(WPXInputStream *input)
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
		s.append(1, (char)0);
		WPS4ParserInternal::FontName f;
		f.m_cp=	WPS4ParserInternal::getCodepage(s);
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
bool WPS4Parser::readFODPage(WPXInputStream *input, std::vector<WPSFOD> * FODs)
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

	int first_fod = FODs->size();

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
		if (FODs->size() > 0 && FODs->back().m_fcLim > fod.m_fcLim)
		{
			WPS_DEBUG_MSG(("Works: error: character position list must "
			               "be monotonic, but found %i, %i\n", FODs->back().m_fcLim, fod.m_fcLim));
			throw libwps::ParseException();
		}
		FODs->push_back(fod);
	}

	/* Read array of bfprop of FODs.  The bfprop is the offset where
	   the FPROP is located. */
	std::vector<WPSFOD>::iterator FODs_iter;
	for (FODs_iter = FODs->begin()+first_fod; FODs_iter!= FODs->end(); FODs_iter++)
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
	for (FODs_iter = FODs->begin()+first_fod; FODs_iter!= FODs->end(); FODs_iter++)
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

	return (!FODs->empty() && (m_offset_eot > FODs->back().m_fcLim));
}

/**
 * @param newTextAttributeBits: all the new, current bits (will be compared against old, and old will be discarded).
 * @param listener:
 *
 */
void WPS4Parser::propertyChangeDelta(uint32_t newTextAttributeBits, WPS4ContentListener *listener)
{
	WPS_DEBUG_MSG(("WPS4Parser::propertyChangeDelta(%i, %p)\n", newTextAttributeBits, (void *)listener));
	if (newTextAttributeBits == m_oldTextAttributeBits)
		return;

	listener->setFontAttributes(newTextAttributeBits);
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
void WPS4Parser::propertyChange(std::string rgchProp, WPS4ContentListener *listener)
{
	/* set default properties */
	uint32_t textAttributeBits = 0;
	uint8_t  colorid = 0;

	listener->setFontSize(12);

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

		if (3 <= getHeader()->getMajorVersion())
		{
			if (m_fonts.find(font_n) == m_fonts.end())
			{
				WPS_DEBUG_MSG(("Works: error: encountered font %i (0x%02x) which is not indexed\n",
				               font_n,font_n ));
				throw libwps::ParseException();
			}
			else
			{
				listener->setTextFont(m_fonts[font_n].m_name.c_str());
				listener->setCodepage(m_fonts[font_n].m_cp);
			}
		}
		if (2 == getHeader()->getMajorVersion())
		{
			listener->setTextFont(WPS4ParserInternal::getWps2FontNameFromIndex(font_n));
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
	propertyChangeDelta(textAttributeBits, listener);
	listener->setColor(WPS4ParserInternal::s_colorMap[colorid]);
}

void WPS4Parser::propertyChangePara(std::string rgchProp, WPS4ContentListener *listener)
{
	static const uint8_t _align[]= {WPS_PARAGRAPH_JUSTIFICATION_LEFT,
	                                WPS_PARAGRAPH_JUSTIFICATION_CENTER,WPS_PARAGRAPH_JUSTIFICATION_RIGHT,
	                                WPS_PARAGRAPH_JUSTIFICATION_FULL
	                               };

	int pindent = 0, pbefore = 0, pafter = 0;
	int pleft =0, pright =0;

	std::vector<WPSTabPos> tabs;
	listener->setTabs(tabs);

	int pf = 0;

	if (4 > rgchProp.length()) return;

	unsigned x = 3;

	while (x < rgchProp.length())
	{
		char code = rgchProp[x++];

		switch (code)
		{
		case 0x5:	// alignment
		{
			uint8_t a = rgchProp[x];
			if (a < 4)
				listener->setAlign(_align[a]);
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
			pleft = WPS_LE_GET_GUINT16(&rgchProp.c_str()[x]);
			x+=2;
			break;
		case 0x12:
			pright = WPS_LE_GET_GUINT16(&rgchProp.c_str()[x]);
			x+=2;
			break;
		case 0x14:
			pindent = (int16_t) WPS_LE_GET_GUINT16(&rgchProp.c_str()[x]);
			x+=2;
			break;
		case 0x16:
			pbefore = WPS_LE_GET_GUINT16(&rgchProp.c_str()[x]);
			x+=2;
			break;
		case 0x17:
			pafter = WPS_LE_GET_GUINT16(&rgchProp.c_str()[x]);
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
			WPSTabPos pos;

			count = WPS_LE_GET_GUINT16(&td[x+2]);
			if (count <= 20)
			{
				for (int i=0; i<count; i++)
				{
					char v;
					pos.m_pos = WPS_LE_GET_GUINT16(&td[x+4+i*2]) / 1440.0;
					v = td[x+4+count*2+i];
					switch (v&3)
					{
					case 1:
						pos.m_align = WPS_TAB_CENTER;
						break;
					case 2:
						pos.m_align = WPS_TAB_RIGHT;
						break;
					case 3:
						pos.m_align = WPS_TAB_DECIMAL;
						break;
					default:
						pos.m_align = WPS_TAB_LEFT;
					}
					// TODO: leader
					tabs.push_back(pos);
				}
				listener->setTabs(tabs);
			}
			x+=rgchProp[x]+(rgchProp[x+1]<<8)+1;
		}
		break;
		default:
			WPS_DEBUG_MSG(("Unknown parafmt code: %02X\n",code));
			break;
		}
	}

	if (pf != 0) listener->setParaFlags(pf);

	listener->setMargins(pindent/1440.0,pleft/1440.0,pright/1440.0,
	                     pbefore/1440.0,pafter/1440.0);
}

/**
 * Take a character in CP850 encoding, convert it
 * and append it to the text buffer as UTF8.
 * Courtesy of glib2 and iconv
 *
 */
void WPS4Parser::appendCP850(const uint8_t readVal, WPS4ContentListener *listener)
{
// Fridrich: I see some MS Works files with IBM850 encoding ???
	static const uint16_t cp850toUCS4[128] =
	{
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
		0x00b0, 0x00a8, 0x00b7, 0x00b9, 0x00b3, 0x00b2, 0x25a0, 0x00a0
	};

	uint32_t ucs4Character = 0;
	if (readVal < 0x80)
		ucs4Character = (uint32_t) readVal;
	else
	{
		ucs4Character = (uint32_t) cp850toUCS4[readVal - 0x80];
	}

	appendUCS(ucs4Character,listener);
}

/**
 * Take a character in CP1252 encoding, convert it
 * and append it to the text buffer as UTF8.
 * Courtesy of glib2 and iconv
 *
 */
void WPS4Parser::appendCP1252(const uint8_t readVal, WPS4ContentListener *listener)
{
	static const uint16_t cp1252toUCS4[32] =
	{
		0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
		0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0xfffd, 0x017d, 0xfffd,
		0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
		0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0xfffd, 0x017e, 0x0178
	};

	uint32_t ucs4Character = 0;
	if (readVal < 0x80 || readVal >= 0xa0)
		ucs4Character = (uint32_t) readVal;
	else
	{
		ucs4Character = (uint32_t) cp1252toUCS4[readVal - 0x80];
		if (ucs4Character == 0xfffd)
//			throw GenericException();
			return;
	}

	appendUCS(ucs4Character,listener);
}

uint32_t s_CP1250(const uint8_t readVal)
{
	static const uint16_t cp1250toUCS4[128] =
	{
		0x20ac, 0xfffd, 0x201a, 0xfffd, 0x201e, 0x2026, 0x2020, 0x2021,
		0xfffd, 0x2030, 0x0160, 0x2039, 0x015a, 0x0164, 0x017d, 0x0179,
		0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
		0xfffd, 0x2122, 0x0161, 0x203a, 0x015b, 0x0165, 0x017e, 0x017a,
		0x00a0, 0x02c7, 0x02d8, 0x0141, 0x00a4, 0x0104, 0x00a6, 0x00a7,
		0x00a8, 0x00a9, 0x015e, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x017b,
		0x00b0, 0x00b1, 0x02db, 0x0142, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
		0x00b8, 0x0105, 0x015f, 0x00bb, 0x013d, 0x02dd, 0x013e, 0x017c,

		0x0154, 0x00c1, 0x00c2, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021,
		0x20ac, 0x2030, 0x0409, 0x2039, 0x040a, 0x040c, 0x040b, 0x040f,
		0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
		0xfffd, 0x2122, 0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f,
		0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7,
		0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f,
		0x0111, 0x0144, 0x0148, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x00b7,
		0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9
	};

	uint32_t ucs4Character = 0;
	if (readVal < 0x80)
	{
		ucs4Character = (uint32_t) readVal;
	}
	else
	{
		ucs4Character = (uint32_t) cp1250toUCS4[readVal - 0x80];
	}

	return ucs4Character;
}

uint32_t s_CP1251(const uint8_t readVal)
{
	static const uint16_t cp1251toUCS4[64] =
	{
		0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021,
		0x20ac, 0x2030, 0x0409, 0x2039, 0x040a, 0x040c, 0x040b, 0x040f,
		0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
		0xfffd, 0x2122, 0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f,
		0x00a0, 0x040e, 0x045e, 0x0408, 0x00a4, 0x0490, 0x00a6, 0x00a7,
		0x0401, 0x00a9, 0x0404, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x0407,
		0x00b0, 0x00b1, 0x0406, 0x0456, 0x0491, 0x00b5, 0x00b6, 0x00b7,
		0x0451, 0x2116, 0x0454, 0x00bb, 0x0458, 0x0405, 0x0455, 0x0457
	};

	uint32_t ucs4Character = 0;
	if (readVal < 0x80)
	{
		ucs4Character = (uint32_t) readVal;
	}
	else if (readVal >= 0xc0)
	{
		ucs4Character = (uint32_t) readVal+0x410-0xc0;
	}
	else
	{
		ucs4Character = (uint32_t) cp1251toUCS4[readVal - 0x80];
	}

	return ucs4Character;
}

void WPS4Parser::appendCP(const uint8_t readVal, int codepage, WPS4ContentListener *listener)
{
	if (!codepage) codepage=listener->getCodepage();
	if (codepage == 1251)
	{
		appendUCS(s_CP1251(readVal),listener);
		return;
	}
	else if (codepage == 1250)
	{
		appendUCS(s_CP1250(readVal),listener);
		return;
	}
	else if (codepage == 850)
	{
		appendCP850(readVal,listener);
		return;
	}
	appendCP1252(readVal,listener);
}

void WPS4Parser::appendUCS(const uint16_t readVal, WPS4ContentListener *listener)
{
	uint32_t ucs4Character = (uint32_t) readVal;
	if (ucs4Character == 0xfffd)
//		throw GenericException();
		return;

	uint8_t first;
	int len;
	if (ucs4Character < 0x80)
	{
		first = 0;
		len = 1;
	}
	else if (ucs4Character < 0x800)
	{
		first = 0xc0;
		len = 2;
	}
	else if (ucs4Character < 0x10000)
	{
		first = 0xe0;
		len = 3;
	}
	else if (ucs4Character < 0x200000)
	{
		first = 0xf0;
		len = 4;
	}
	else if (ucs4Character < 0x4000000)
	{
		first = 0xf8;
		len = 5;
	}
	else
	{
		first = 0xfc;
		len = 6;
	}

	uint8_t outbuf[6] = { 0, 0, 0, 0, 0, 0 };
	int i;
	for (i = len - 1; i > 0; --i)
	{
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
void WPS4Parser::readText(WPXInputStream *input, WPS4ContentListener *listener)
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
				propertyChange((*FODs_iter).m_fprop.m_rgchProp, listener);
			if ((*PFOD_iter).m_fprop.m_cch > 0)
				propertyChangePara((*PFOD_iter).m_fprop.m_rgchProp, listener);
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
					listener->setFieldType(WPS_FIELD_PAGE);
					listener->insertField();
					break;

				case 0x0C:
					listener->insertBreak(WPS_PAGE_BREAK);
					break;

				case 0x0D:
					listener->insertEOL();
					break;

				default:
					if (m_worksVersion == 2)
						this->appendCP(readVal, 850, listener);
					else
						this->appendCP(readVal, 0, listener);
					break;
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
void WPS4Parser::parsePages(std::list<WPSPageSpan> &pageList, WPXInputStream *input)
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
		ps.setFormOrientation(libwps::PORTRAIT);
	else
		ps.setFormOrientation(libwps::LANDSCAPE);

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

void WPS4Parser::parse(WPXInputStream *input, WPS4ContentListener *listener)
{
	WPS_DEBUG_MSG(("WPS4Parser::parse()\n"));

	listener->startDocument();

	/* find beginning of character FODs */
	input->seek(WPS4_FCMAC_OFFSET, WPX_SEEK_SET);
	m_offset_eot = libwps::readU32(input); /* stream offset to end of text */
	WPS_DEBUG_MSG(("Works: info: location WPS4_FCMAC_OFFSET (0x%X) has offset_eot = 0x%X (%i)\n",
	               WPS4_FCMAC_OFFSET, m_offset_eot, m_offset_eot));
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
	while (readFODPage(input, &m_CHFODs))
	{
	}
	// fixme: verify: final FOD covers end of text

	/* read paragraph formatting */
	while (readFODPage(input, &m_PAFODs))
	{
	}

	/* read fonts table */
	if ( getHeader()->getMajorVersion() > 2)
		readFontsTable(input);

	/* process text file using previously-read character formatting */
	readText(input, listener);

	listener->endDocument();
}

#ifdef DEBUG
std::string WPS4Parser::to_bits(std::string s)
{
	std::string r;
	for (unsigned int i = 0; i < s.length(); i++)
	{
		std::bitset<8> b(s[i]);
		r.append(b.to_string());
		char buf[20];
		sprintf(buf, "(%02u,0x%02x)  ", (uint8_t)s[i],(uint8_t)s[i]);
		r.append(buf);
	}
	return r;
}
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
