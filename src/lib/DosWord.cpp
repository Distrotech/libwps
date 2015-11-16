/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2015 Sean Young <sean@mess.org>
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwps.sourceforge.net
 */

#include <librevenge-stream/librevenge-stream.h>
#include "libwps_internal.h"

#include "WPSContentListener.h"
#include "WPSParagraph.h"
#include "WPSFont.h"
#include "WPSHeader.h"

#include "MSWrite.h"
#include "DosWord.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

namespace DosWordParserInternal
{

struct CHP
{
	CHP() : m_fStyled(0), m_fBold(0), m_hps(0), m_fUline(0), m_unused(0), m_hpsPos(0), m_clr(0)
	{
	}
	uint8_t	m_fStyled;
	uint8_t	m_fBold;
	uint8_t	m_hps;
	uint8_t	m_fUline;
	uint8_t	m_unused;
	uint8_t	m_hpsPos;
	uint8_t m_clr;
};

struct PAP
{
	//! constructor
	PAP() : m_style(0), m_justification(0),
		m_dxaRight(0), m_dxaLeft(0), m_dxaLeft1(0), m_dyaLine(0),
		m_dyaBefore(0), m_dyaAfter(0), m_rhc(0), m_border(0),
		m_shade(0), m_pos(0), m_dxaFromText(0)
	{
		m_reserved2[0]=m_reserved2[1]=0;
	}
	uint8_t	m_style;
	uint8_t	m_justification;
	uint8_t	m_reserved2[2];
	uint16_t m_dxaRight, m_dxaLeft, m_dxaLeft1, m_dyaLine;
	uint16_t m_dyaBefore, m_dyaAfter;
	uint8_t	m_rhc;
	uint8_t m_border;
	uint8_t m_shade;
	uint8_t m_pos;
	uint16_t m_dxaFromText;
	struct TBD
	{
		TBD() : m_dxa(0), m_jcTab(0), m_chAlign(0)
		{
		}
		uint16_t    m_dxa;
		uint8_t	    m_jcTab;
		uint8_t	    m_chAlign;
	} m_TBD[20];
};

// the file header offsets
enum HeaderOffset
{
	HEADER_W_WIDENT = 0,
	HEADER_W_DTY = 2,
	HEADER_W_WTOOL = 4,
	HEADER_D_FCMAC = 14,
	HEADER_W_PNPARA = 18,
	HEADER_W_PNFNTB = 20,
	HEADER_W_PNBKMK = 22,
	HEADER_W_PNSETB = 24,
	HEADER_W_PNBFTB = 26,
	HEADER_W_PNSUMD = 28,
	HEADER_W_PNMAC = 106,
	HEADER_B_VERSION = 116,
	HEADER_B_ASV = 117,
	HEADER_W_CODEPAGE = 126
};

}

DosWordParser::DosWordParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
                             libwps_tools_win::Font::Type encoding):
	MSWriteParser(input, header, encoding)
{
}

DosWordParser::~DosWordParser()
{
}

void DosWordParser::readFFNTB()
{
	// Microsoft Word for DOS does not have this section
}

// basic function to check if the header is ok
bool DosWordParser::checkHeader(WPSHeader *header, bool /*strict*/)
{
	RVNGInputStreamPtr input = getInput();
	if (!input || !checkFilePosition(0x100))
	{
		WPS_DEBUG_MSG(("DosWordParser::checkHeader: file is too short\n"));
		return false;
	}

	input->seek(DosWordParserInternal::HEADER_B_ASV, librevenge::RVNG_SEEK_SET);
	if (libwps::readU8(input) & 2)
	{
		WPS_DEBUG_MSG(("DosWordParser::checkHeader: file is autosaved\n"));
		return false;
	}

	input->seek(DosWordParserInternal::HEADER_B_VERSION, librevenge::RVNG_SEEK_SET);
	uint8_t ver = libwps::readU8(input);

	switch (ver)
	{
	case 0:
		WPS_DEBUG_MSG(("DosWordParser::checkHeader: version 4.0 or earlier\n"));
		header->setMajorVersion(4);
		break;
	case 3:
		WPS_DEBUG_MSG(("DosWordParser::checkHeader: version 5 OEM\n"));
		header->setMajorVersion(5);
		break;
	case 4:
		WPS_DEBUG_MSG(("DosWordParser::checkHeader: version 5\n"));
		header->setMajorVersion(5);
		break;
	case 7:
		WPS_DEBUG_MSG(("DosWordParser::checkHeader: version 5.5\n"));
		header->setMajorVersion(5);
		break;
	case 9:
		WPS_DEBUG_MSG(("DosWordParser::checkHeader: version 6.0\n"));
		header->setMajorVersion(6);
		break;
	default:
		WPS_DEBUG_MSG(("DosWordParser::checkHeader: unknown version %u\n", ver));
		break;
	}

	input->seek(DosWordParserInternal::HEADER_W_CODEPAGE, librevenge::RVNG_SEEK_SET);
	uint16_t codepage = libwps::readU16(input);

	if (!codepage)
		header->setNeedEncoding(true);

	return true;
}

libwps_tools_win::Font::Type DosWordParser::getFileEncoding(libwps_tools_win::Font::Type encoding)
{
	RVNGInputStreamPtr input = getInput();

	input->seek(DosWordParserInternal::HEADER_W_CODEPAGE, librevenge::RVNG_SEEK_SET);
	uint16_t codepage = libwps::readU16(input);

	WPS_DEBUG_MSG(("DosWordParser::getFileEncoding: codepage %u\n", codepage));
	if (codepage)
		encoding = libwps_tools_win::Font::getTypeForOEM(codepage);

	if (encoding == libwps_tools_win::Font::UNKNOWN)
		encoding = libwps_tools_win::Font::CP_437;

	return encoding;
}

WPSColor DosWordParser::color(int clr)
{
	switch (clr)
	{
	default:
	case 0: // black (default)
		return WPSColor(0, 0, 0);
	case 1: // red
		return WPSColor(255, 0, 0);
	case 2: // green
		return WPSColor(0, 255, 0);
	case 3: // blue
		return WPSColor(0, 0, 255);
	case 4: // violet
		return WPSColor(127, 0, 255);
	case 5: // magenta
		return WPSColor(255, 0, 255);
	case 6: // yellow
		return WPSColor(0, 255, 255);
	case 7: // white
		return WPSColor(255, 255, 255);
	}
}

void DosWordParser::readCHP(uint32_t fcFirst, uint32_t fcLim, unsigned cch)
{
	RVNGInputStreamPtr input = getInput();

	DosWordParserInternal::CHP chp;

	chp.m_hps = 24;

	if (cch)
	{
		if (cch > sizeof(chp))
			cch = sizeof(chp);

		unsigned long read_bytes;
		const unsigned char *p = input->read(cch, read_bytes);
		if (read_bytes != cch)
		{
			WPS_DEBUG_MSG(("DosWordParser::readCHP failed to read CHP entry\n"));
			throw (libwps::ParseException());
		}

		memcpy(&chp, p, cch);
	}

	MSWriteParserInternal::Font font;

	if (chp.m_fStyled & 1)
	{
		switch (chp.m_fStyled / 2)
		{
		case 13: // footnote reference
			font.m_footnote = true;
			break;
		case 26: // annotation reference
			font.m_annotation = true;
			break;
		default:
			WPS_DEBUG_MSG(("Style sheet stc=%u %x-%x\n", chp.m_fStyled / 2, fcFirst, fcLim));
			break;
		}
	}

	unsigned ftc = (chp.m_fBold / 4);

	// Note the font depends on the printer driver
	if (ftc <= 15)
		font.m_name.sprintf("modern %c", 'a' + ftc);
	else if (ftc <= 31)
		font.m_name.sprintf("roman %c", 'a' + (ftc - 16));
	else if (ftc <= 39)
		font.m_name.sprintf("script %c", 'a' + (ftc - 32));
	else if (ftc <= 47)
		font.m_name.sprintf("foreign %c", 'a' + (ftc - 40));
	else if (ftc <= 55)
		font.m_name.sprintf("decor %c", 'a' + (ftc - 48));
	else
		font.m_name.sprintf("symbol %c", 'a' + (ftc - 56));

	font.m_size = chp.m_hps / 2.0;
	if (chp.m_fBold & 1)
		font.m_attributes |= WPS_BOLD_BIT;
	if (chp.m_fBold & 2)
		font.m_attributes |= WPS_ITALICS_BIT;
	if (chp.m_fUline & 1)
		font.m_attributes |= WPS_UNDERLINE_BIT;
	if (chp.m_fUline & 2)
		font.m_attributes |= WPS_STRIKEOUT_BIT;
	if (chp.m_fUline & 4)
		font.m_attributes |= WPS_DOUBLE_UNDERLINE_BIT;
	// FIXME: if (chp.m_fUline & 8) marks a text new (not accepted)
	if ((chp.m_fUline & 0x30) == 0x10)
		font.m_attributes |= WPS_ALL_CAPS_BIT;
	else if ((chp.m_fUline & 0x30) == 0x30)
		font.m_attributes |= WPS_SMALL_CAPS_BIT;
	if (chp.m_fUline & 0x40)
		font.m_special = true;
	if (chp.m_fUline & 0x80)
		font.m_attributes |= WPS_HIDDEN_BIT;
	if (chp.m_hpsPos)
	{
		if (chp.m_hpsPos & 0x80)
			font.m_attributes |= WPS_SUBSCRIPT_BIT;
		else
			font.m_attributes |= WPS_SUPERSCRIPT_BIT;
	}

	font.m_fcFirst = fcFirst;
	font.m_fcLim = fcLim;
	font.m_encoding = libwps_tools_win::Font::getFontType(font.m_name);
	if (font.m_encoding == libwps_tools_win::Font::UNKNOWN)
		font.m_encoding = m_fontType;

	font.m_color = color(chp.m_clr & 7);

	m_fontList.push_back(font);
}

void DosWordParser::readPAP(uint32_t fcFirst, uint32_t fcLim, unsigned cch)
{
	RVNGInputStreamPtr input = getInput();

	DosWordParserInternal::PAP pap;

	WPS_LE_PUT_GUINT16(&pap.m_dyaLine, 240);

	if (cch)
	{
		if (cch > sizeof(pap))
			cch = sizeof(pap);

		unsigned long read_bytes;
		const unsigned char *p = input->read(cch, read_bytes);
		if (read_bytes != cch)
		{
			WPS_DEBUG_MSG(("DosWordParser::readPAP failed to read PAP\n"));
			throw (libwps::ParseException());
		}

		memcpy(&pap, p, cch);
	}

	int16_t dxaLeft = (int16_t) WPS_LE_GET_GUINT16(&pap.m_dxaLeft);
	int16_t dxaLeft1 = (int16_t) WPS_LE_GET_GUINT16(&pap.m_dxaLeft1);
	int16_t dxaRight = (int16_t) WPS_LE_GET_GUINT16(&pap.m_dxaRight);

	MSWriteParserInternal::Paragraph para;
	int i;

	for (i=0; i<20; i++)
	{
		uint16_t pos = WPS_LE_GET_GUINT16(&pap.m_TBD[i].m_dxa);

		if (!pos)
			break;

		WPSTabStop::Alignment align;

		switch (pap.m_TBD[i].m_jcTab & 3)
		{
		default:
		case 0:
			align = WPSTabStop::LEFT;
			break;
		case 1:
			align = WPSTabStop::CENTER;
			break;
		case 2:
			align = WPSTabStop::RIGHT;
			break;
		case 3:
			align = WPSTabStop::DECIMAL;
			break;
		}

		unsigned leader = (pap.m_TBD[i].m_jcTab >> 3) & 3;
		WPSTabStop tab(pos / 1440., align, uint16_t("\0.-_"[leader]));

		para.m_tabs.push_back(tab);

		if (dxaLeft + dxaLeft1 == pos)
			para.m_skiptab = true;
	}

	switch (pap.m_justification & 3)
	{
	default:
	case 0:
		para.m_justify = libwps::JustificationLeft;
		break;
	case 1:
		para.m_justify = libwps::JustificationCenter;
		break;
	case 2:
		para.m_justify = libwps::JustificationRight;
		break;
	case 3:
		para.m_justify = libwps::JustificationFull;
		break;
	}

	para.m_margins[0] = dxaLeft1 / 1440.0;
	para.m_margins[1] = dxaLeft / 1440.0;
	para.m_margins[2] = dxaRight / 1440.0;

	// spacings
	int16_t dyaLine = (int16_t) WPS_LE_GET_GUINT16(&pap.m_dyaLine);
	uint16_t dyaBefore = WPS_LE_GET_GUINT16(&pap.m_dyaBefore);
	uint16_t dyaAfter = WPS_LE_GET_GUINT16(&pap.m_dyaAfter);
	// dyaLine = -40 means "auto"
	if (dyaLine > 0)
		para.m_spacings[0] = dyaLine / 240.0;
	para.m_spacings[1] = dyaBefore / 240.0;
	para.m_spacings[2] = dyaAfter / 240.0;

	para.m_fcFirst = fcFirst;
	para.m_fcLim = fcLim;

	if (pap.m_rhc & 0xe)
	{
		if (pap.m_rhc & 1)
			para.m_Location = MSWriteParserInternal::Paragraph::FOOTER;
		else
			para.m_Location = MSWriteParserInternal::Paragraph::HEADER;

		switch ((pap.m_rhc >> 1) & 3)
		{
		default:
		case 3: // all
			para.m_HeaderFooterOccurrence = WPSPageSpan::ALL;
			break;
		case 2: // even
			para.m_HeaderFooterOccurrence = WPSPageSpan::EVEN;
			break;
		case 1: // odd
			para.m_HeaderFooterOccurrence = WPSPageSpan::ODD;
			break;
		case 0: // never; however might be on first page
			para.m_HeaderFooterOccurrence = WPSPageSpan::NEVER;
			break;
		}

		para.m_firstpage = (pap.m_rhc & 0x08) != 0;
	}

	if (pap.m_justification & 0x20)
		para.m_headerUseMargin = true;

	if (pap.m_style & 1)
	{
		switch (pap.m_style / 2)
		{
		case 39: // footnote
		case 87: // annotation
			para.m_Location = MSWriteParserInternal::Paragraph::FOOTNOTE;
			break;
		default:
			WPS_DEBUG_MSG(("DosWordParser::readPAP pap unknown style stc=%u %x-%x\n", pap.m_style / 2, fcFirst, fcLim));
			break;
		}
	}

	// Borders
	if (pap.m_rhc & 0x30)
	{
		if ((pap.m_rhc & 0x30) == 0x10)
			para.m_border = 15;
		else
			para.m_border = pap.m_border & 15;

		WPSBorder::Type type = WPSBorder::Single;
		int width = 1;

		switch (pap.m_rhc & 0xc0)
		{
		default:
		case 0:	// normal
			break;
		case 0x40: // bold
			width = 2;
			break;
		case 0x80: // Double
			type = WPSBorder::Double;
			break;
		case 0xc0: // thick
			width = 8;
			break;
		}

		para.m_borderStyle.m_type = type;
		para.m_borderStyle.m_width = width;
		para.m_borderStyle.m_color = color((pap.m_border / 16) & 7);
	}

	if (pap.m_justification & 4)
		para.m_breakStatus |= libwps::NoBreakBit;
	if (pap.m_justification & 8)
		para.m_breakStatus |= libwps::NoBreakWithNextBit;

	// paragraph shading
	if (pap.m_shade & 0x7f)
	{
		WPSColor c = color((pap.m_pos >> 4) & 7);

		unsigned percent = std::min(pap.m_shade & 0x7fu, 100u);

		// Use percent to increase brightness
		// 100% means color stays the same
		// 0% means white

		unsigned add = (255 * (100 - percent)) / 100;

		para.m_backgroundColor = WPSColor(
		                             (unsigned char)std::min(c.getRed() + add, 255u),
		                             (unsigned char)std::min(c.getGreen() + add, 255u),
		                             (unsigned char)std::min(c.getBlue() + add, 255u));
	}

	// FIXME: side-by-side
	// FIXME: paragraph position

	m_paragraphList.push_back(para);
}

void DosWordParser::insertSpecial(uint8_t val, uint32_t fc, MSWriteParserInternal::Paragraph::Location location)
{
	librevenge::RVNGString empty;

	switch (val)
	{
	case 1: // page name
		m_listener->insertField(WPSContentListener::PageNumber);
		break;
	case 2: // current date
		m_listener->insertField(WPSContentListener::Date);
		break;
	case 3: // current time
		m_listener->insertField(WPSContentListener::Time);
		break;
	case 4: // annotation reference
		if (location == MSWriteParserInternal::Paragraph::MAIN)
			insertNote(true, fc, empty);
		break;
	case 5: // footnote reference
		if (location == MSWriteParserInternal::Paragraph::MAIN)
			insertNote(false, fc, empty);
		break;
	case 7: // sequence mark
		WPS_DEBUG_MSG(("Sequence mark\n"));
		break;
	case 8: // sequence reference mark
		WPS_DEBUG_MSG(("Sequence reference mark\n"));
		break;
	case 9: // next page
		m_listener->insertField(WPSContentListener::NextPageNumber);
		break;
	default:
		WPS_DEBUG_MSG(("DosWordParser::insertSpecial: unknown special %u encountered\n", val));
		break;
	}
}

void DosWordParser::insertControl(uint8_t val)
{
	// 0xc4 = normal hyphen, 0xff = nbsp, already handled by cp437
	switch (val)
	{
	case 9:
		m_listener->insertTab();
		break;
	case 10:
	case 11:
		m_listener->insertEOL();
		break;
	case 12:
		m_listener->insertBreak(WPS_PAGE_BREAK);
		break;
	case 13: // carriage return
		break;
	case 14: // column break
		m_listener->insertBreak(WPS_COLUMN_BREAK);
		break;
	case 15: // em-hyphen
		m_listener->insertUnicode(0x8212);
		break;
	case 31: // soft hyphen
		m_listener->insertUnicode(0xad);
		break;
	default:
		WPS_DEBUG_MSG(("DosWordParser::insertControl: unexpected control %u\n", val));
		break;
	}
}

void DosWordParser::readSUMD()
{
	RVNGInputStreamPtr input = getInput();

	input->seek(DosWordParserInternal::HEADER_W_PNSUMD, librevenge::RVNG_SEEK_SET);
	uint16_t pnSumd = libwps::readU16(input);

	input->seek(DosWordParserInternal::HEADER_W_PNMAC, librevenge::RVNG_SEEK_SET);
	uint16_t pnMac = libwps::readU16(input);

	if (!pnSumd || pnSumd == pnMac)
	{
		// No summary page
		return;
	}

	/*
	 * The page starts with 9 uint16_t values which are offsets. Sometimes
	 * the page contains garbage; when it does not, the first offset is 0.
	 */
	uint32_t fc = pnSumd * 0x80;
	int i;

	if (!checkFilePosition(fc + 20))
	{
		WPS_DEBUG_MSG(("DosWordParser::readSUMD: summary missing\n"));
		return;
	}

	input->seek(fc, librevenge::RVNG_SEEK_SET);
	if (libwps::readU16(input))
	{
		WPS_DEBUG_MSG(("DosWordParser::readSUMD: garbage\n"));
		return;
	}

	// Step over the offsets; it's packed together anyway.
	fc += 0x12;

	static const char *sum_types[] =
	{
		"dc:title", // title
		"dc:creator", // author
		"dc:publisher", // operator
		"meta:keyword", // keywords
		"librevenge:comments", // comments
		"librevenge:version-number", // version
		NULL
	};

	input->seek(fc, librevenge::RVNG_SEEK_SET);

	for (i=0; sum_types[i]; i++)
	{
		librevenge::RVNGString str;

		for (;;)
		{
			if (!checkFilePosition(++fc))
			{
				WPS_DEBUG_MSG(("DosWordParser::readSUMD: summary missing\n"));
				return;
			}

			char ch = char(libwps::readU8(input));
			if (!ch)
				break;
			str.append(ch);
		}

		if (str.size())
		{
			librevenge::RVNGString conv = libwps_tools_win::Font::unicodeString((const unsigned char *)str.cstr(), str.size(), m_fontType);
			WPS_DEBUG_MSG(("DosWordParser::readSUMD: %d %s\n", i, conv.cstr()));
			m_metaData.insert(sum_types[i], conv);
		}
	}

	librevenge::RVNGString creationDate, revisionDate;
	int month, day, year;

	for (i=0; i<8; i++)
	{
		if (!checkFilePosition(++fc))
		{
			WPS_DEBUG_MSG(("DosWordParser::readSUMD: summary missing\n"));
			return;
		}

		char ch = char(libwps::readU8(input));
		if (!ch)
			break;
		creationDate.append(ch);
	}

	// Year is given in two decimals since 1900 so fudge any value
	// < 50 to be after 2000
	if (3 == sscanf(creationDate.cstr(), "%d/%d/%d", &month, &day, &year))
	{
		librevenge::RVNGString str;
		if (year > 50)
			year += 1900;
		else
			year += 2000;
		str.sprintf("%d-%d-%d", year, month, day);
		m_metaData.insert("meta:creation-date", str);
	}

	for (i=0; i<8; i++)
	{
		if (!checkFilePosition(++fc))
		{
			WPS_DEBUG_MSG(("DosWordParser::readSUMD: summary missing\n"));
			return;
		}

		char ch = char(libwps::readU8(input));
		if (!ch)
			break;
		revisionDate.append(ch);
	}

	if (3 == sscanf(revisionDate.cstr(), "%d/%d/%d", &month, &day, &year))
	{
		librevenge::RVNGString str;
		if (year > 50)
			year += 1900;
		else
			year += 2000;
		str.sprintf("%d-%d-%d", year, month, day);
		m_metaData.insert("librevenge:revision-notes", str);
	}
}

void DosWordParser::readFNTB()
{
	RVNGInputStreamPtr input = getInput();

	input->seek(DosWordParserInternal::HEADER_W_PNFNTB, librevenge::RVNG_SEEK_SET);
	uint16_t pnFntb = libwps::readU16(input);

	input->seek(DosWordParserInternal::HEADER_W_PNBKMK, librevenge::RVNG_SEEK_SET);
	uint16_t pnBkmk = libwps::readU16(input);

	if (pnFntb == 0 || pnFntb == pnBkmk)
		return;

	uint32_t fc = pnFntb * 0x80;

	if (!checkFilePosition(fc + 4))
	{
		WPS_DEBUG_MSG(("DosWordParser::readFNTB: footnote table missing\n"));
		return;
	}

	input->seek(fc, librevenge::RVNG_SEEK_SET);

	uint16_t noteCount = libwps::readU16(input);
	uint16_t noteCount2 = libwps::readU16(input);

	if (noteCount != noteCount2)
	{
		WPS_DEBUG_MSG(("DosWordParser::readFNTB: two counts are different %u %u\n", noteCount, noteCount2));
	}

	for (unsigned note=0; note<noteCount; note++)
	{
		if (!checkFilePosition(fc + 8))
		{
			WPS_DEBUG_MSG(("DosWordParser::readFNTB: footnote %u missing\n", note));
			return;
		}
		fc += 8;

		MSWriteParserInternal::Footnote footnote;

		footnote.fcRef = libwps::readU32(input) + 0x80;
		footnote.fcFtn = libwps::readU32(input) + 0x80;

		m_footnotes.push_back(footnote);
	}
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
