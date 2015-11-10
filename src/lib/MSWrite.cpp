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
#include "WPSEntry.h"
#include "WPSPageSpan.h"
#include "WPSParagraph.h"
#include "WPSPosition.h"
#include "WPSFont.h"
#include "WPSTextSubDocument.h"

#include "MSWrite.h"

#include <algorithm>
#include <cstring>

namespace MSWriteParserInternal
{
//! Internal: the subdocument of a MSWriteParser
class SubDocument : public WPSTextSubDocument
{
public:
	//! constructor for a text entry
	SubDocument(RVNGInputStreamPtr input, MSWriteParser &pars, WPSEntry const &entry) :
		WPSTextSubDocument(input, &pars), m_entry(entry) {}
	//! destructor
	~SubDocument() {}

	//! operator==
	virtual bool operator==(shared_ptr<WPSTextSubDocument> const &doc) const
	{
		if (!doc || !WPSTextSubDocument::operator==(doc))
			return false;
		SubDocument const *sDoc = dynamic_cast<SubDocument const *>(doc.get());
		if (!sDoc) return false;
		return m_entry == sDoc->m_entry;
	}

	//! the parser function
	void parse(shared_ptr<WPSContentListener> &listener, libwps::SubDocumentType subDocumentType);
	//! the entry
	WPSEntry m_entry;
};

void SubDocument::parse(shared_ptr<WPSContentListener> &listener, libwps::SubDocumentType subDocumentType)
{
	if (!listener.get())
	{
		WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: no listener\n"));
		return;
	}
	if (!dynamic_cast<WPSContentListener *>(listener.get()))
	{
		WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: bad listener\n"));
		return;
	}

	WPSContentListenerPtr &listen = reinterpret_cast<WPSContentListenerPtr &>(listener);
	if (!m_parser)
	{
		listen->insertCharacter(' ');
		WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: bad parser\n"));
		return;
	}

	if (m_entry.isParsed() && subDocumentType != libwps::DOC_HEADER_FOOTER)
	{
		listen->insertCharacter(' ');
		WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: this zone is already parsed\n"));
		return;
	}
	m_entry.setParsed(true);
	if (m_entry.type() != "TEXT")
	{
		listen->insertCharacter(' ');
		WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: send not Text entry is not implemented\n"));
		return;
	}

	if (!m_entry.valid() || !m_input)
	{
		WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: empty document found...\n"));
		return;
	}

	MSWriteParser *mnParser = dynamic_cast<MSWriteParser *>(m_parser);
	if (!mnParser)
	{
		WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: bad parser...\n"));
		listen->insertCharacter(' ');
		return;
	}
	long actPos=m_input->tell();
	mnParser->readText(m_entry);
	m_input->seek(actPos, librevenge::RVNG_SEEK_SET);
}

struct SEP
{
	//! constructor
	SEP() : m_yaMac(11), m_xaMac(8.5), m_yaTop(1), m_dyaText(9), m_xaLeft(1.25), m_dxaText(6),
		m_startPageNumber(0xffff), m_yaHeader(0.75), m_yaFooter(10.25) /* 11-0.75inch*/
	{
	}
	double m_yaMac, m_xaMac;
	double m_yaTop;
	double m_dyaText;
	double m_xaLeft;
	double m_dxaText;
	uint16_t m_startPageNumber;
	double m_yaHeader;
	double m_yaFooter;
};

struct PAP
{
	//! constructor
	PAP() : m_reserved1(0), m_justification(0),
		m_dxaRight(0), m_dxaLeft(0), m_dxaLeft1(0), m_dyaLine(0),
		m_dyaBefore(0), m_dyaAfter(0), m_rhcPage(0)
	{
		m_reserved2[0]=m_reserved2[1]=0;
		for (int i=0; i<5; ++i) m_reserved3[i]=0;
	}
	uint8_t	m_reserved1;
	uint8_t	m_justification;
	uint8_t	m_reserved2[2];
	uint16_t m_dxaRight, m_dxaLeft, m_dxaLeft1, m_dyaLine;
	uint16_t m_dyaBefore, m_dyaAfter;
	uint8_t	m_rhcPage;
	uint8_t	m_reserved3[5];
	struct TBD
	{
		TBD() : m_dxa(0), m_jcTab(0), m_chAlign(0)
		{
		}
		uint16_t    m_dxa;
		uint8_t	    m_jcTab;
		uint8_t	    m_chAlign;
	} m_TBD[14];
};

struct CHP
{
	CHP() : m_reserved1(0), m_fBold(0), m_hps(0), m_fUline(0), m_ftcXtra(0), m_hpsPos(0)
	{
	}
	uint8_t	m_reserved1;
	uint8_t	m_fBold;
	uint8_t	m_hps;
	uint8_t	m_fUline;
	uint8_t	m_ftcXtra;
	uint8_t	m_hpsPos;
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
	HEADER_W_PNSEP = 22,
	HEADER_W_PNSETB = 24,
	HEADER_W_PNBFTB = 26,
	HEADER_W_PNFFNTB = 28,
	HEADER_W_PNMAC = 96
};

// pictures in Write 3.0, either DDB of WMF
enum PicOffset
{
	PIC_W_MM = 0,
	PIC_W_XEXT = 2,
	PIC_W_YEXT = 4,
	PIC_W_DXAOFFSET = 8,
	PIC_W_DXASIZE = 10,
	PIC_W_DYASIZE = 12,
	PIC_W_BMWIDTH = 18,
	PIC_W_BMHEIGHT = 20,
	PIC_W_BMWIDTHBYTES = 22,
	PIC_B_BMPLANES = 24,
	PIC_B_BMBITSPIXEL = 25,
	PIC_D_CBSIZE = 32,
	PIC_W_MX = 36,
	PIC_W_MY = 38
};

// OLE objects in Write 3.1
enum OleOffset
{
	OLE_W_MM = 0,
	OLE_W_OBJECTTYPE = 6,
	OLE_W_DXAOFFSET = 8,
	OLE_W_DXASIZE = 10,
	OLE_W_DYASIZE = 12,
	OLE_D_DWDATASIZE = 16,
	OLE_W_MX = 36,
	OLE_W_MY = 38
};

enum BitmapInfoHeaderV3Offset
{
	BM_INFO_V3_SIZE = 0,
	BM_INFO_V3_WIDTH = 4,
	BM_INFO_V3_HEIGHT = 8,
	BM_INFO_V3_PLANES = 12,
	BM_INFO_V3_BITS_PIXEL = 14,
	BM_INFO_V3_COMPRESSION = 16,
	BM_INFO_V3_BITMAP_SIZE = 20,
	BM_INFO_V3_HORZ_RES = 24,
	BM_INFO_V3_VERT_RES = 28,
	BM_INFO_V3_COLORS_USED = 32,
	BM_INFO_V3_COLORS_IMPORTANT = 36,
	BM_INFO_V3_STRUCT_SIZE = 40
};

enum BitmapFileHeaderOffset
{
	BM_FILE_MAGIC = 0,
	BM_FILE_SIZE = 2,
	BM_FILE_RESERVED = 6,
	BM_FILE_OFFSET = 10,
	BM_FILE_STRUCT_SIZE = 14
};

enum BitmapInfoHeaderV2Offset
{
	BM_INFO_V2_SIZE = 0,
	BM_INFO_V2_WIDTH = 4,
	BM_INFO_V2_HEIGHT = 6,
	BM_INFO_V2_PLANES = 8,
	BM_INFO_V2_BITS_PIXEL = 10,
	BM_INFO_V2_STRUCT_SIZE = 12
};

struct BitmapPalette
{
	uint8_t m_r, m_g, m_b;
};

static void appendU16(librevenge::RVNGBinaryData &b, uint16_t val)
{
	b.append((unsigned char)(val));
	b.append((unsigned char)(val >> 8));
}

static void appendU32(librevenge::RVNGBinaryData &b, uint32_t val)
{
	b.append((unsigned char)(val));
	b.append((unsigned char)(val >> 8));
	b.append((unsigned char)(val >> 16));
	b.append((unsigned char)(val >> 24));
}

}

MSWriteParser::MSWriteParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
                             libwps_tools_win::Font::Type encoding):
	WPSParser(input, header), m_fileLength(0), m_fcMac(0),
	m_paragraphList(0), m_fontList(0), m_footnotes(0), m_fonts(0),
	m_pageSpan(), m_fontType(encoding), m_listener(), m_Main(), m_metaData()
{
	if (!input)
	{
		WPS_DEBUG_MSG(("MSWriteParser::MSWriteParser: called without input\n"));
		throw libwps::ParseException();
	}

	input->seek(0, librevenge::RVNG_SEEK_END);
	m_fileLength=(uint32_t) input->tell();
	input->seek(0, librevenge::RVNG_SEEK_SET);
	m_fontType = getFileEncoding(encoding);
}

MSWriteParser::~MSWriteParser()
{
}

libwps_tools_win::Font::Type MSWriteParser::getFileEncoding(libwps_tools_win::Font::Type encoding)
{
	if (encoding == libwps_tools_win::Font::UNKNOWN)
		encoding = libwps_tools_win::Font::WIN3_WEUROPE;

	return encoding;
}

void MSWriteParser::readFIB()
{
	RVNGInputStreamPtr input = getInput();

	if (!checkFilePosition(0x100))
	{
		WPS_DEBUG_MSG(("MSWriteParser::readFIB File is too short to be a MSWrite file\n"));
		throw (libwps::ParseException());
	}

	input->seek(MSWriteParserInternal::HEADER_D_FCMAC, librevenge::RVNG_SEEK_SET);
	m_fcMac = libwps::readU32(input);

	if (m_fcMac <= 0x80)
	{
		WPS_DEBUG_MSG(("MSWriteParser::readFIB fcMac %u is impossible\n", m_fcMac));
		throw (libwps::ParseException());
	}

	if (!checkFilePosition(m_fcMac))
	{
		WPS_DEBUG_MSG(("MSWriteParser::readFIB File is corrupt, position %u beyond end of file\n", m_fcMac));
		throw (libwps::ParseException());
	}
}

void MSWriteParser::readFFNTB()
{
	unsigned font_count = 0, pnFfntb, pnMac;
	RVNGInputStreamPtr input = getInput();

	input->seek(MSWriteParserInternal::HEADER_W_PNFFNTB, librevenge::RVNG_SEEK_SET);
	pnFfntb = libwps::readU16(input);

	input->seek(MSWriteParserInternal::HEADER_W_PNMAC, librevenge::RVNG_SEEK_SET);
	pnMac = libwps::readU16(input);

	if (pnFfntb != 0 && pnFfntb != pnMac)
	{
		if (!checkFilePosition(pnFfntb * 0x80 + 2))
		{
			WPS_DEBUG_MSG(("MSWriteParser::readFFNTB Font table is missing\n"));
		}
		else
		{
			input->seek(pnFfntb * 0x80, librevenge::RVNG_SEEK_SET);
			font_count = libwps::readU16(input);
		}
	}

	if (font_count)
	{
		// Ignore font_count, some write files have more
		unsigned offset = 0;

		for (;;)
		{
			offset += 2;
			if (!checkFilePosition(pnFfntb * 0x80 + offset))
			{
				WPS_DEBUG_MSG(("MSWriteParser::readFFNTB Font table is truncated\n"));
				break;
			}

			unsigned int cbFfn = libwps::readU16(input);
			if (cbFfn == 0)
				break;

			if (cbFfn == 0xffff)
			{
				pnFfntb++;

				if (!checkFilePosition(pnFfntb * 0x80 + 2))
				{
					WPS_DEBUG_MSG(("MSWriteParser::readFFNTB Font table is truncated\n"));
					break;
				}

				offset = 0;
				input->seek(pnFfntb * 0x80, librevenge::RVNG_SEEK_SET);
				continue;
			}

			offset += cbFfn;
			if (offset > 0x80)
			{
				WPS_DEBUG_MSG(("MSWriteParser::readFFNTB Font straddles page, file corrupt\n"));
				break;
			}

			if (!checkFilePosition(pnFfntb * 0x80 + offset))
			{
				WPS_DEBUG_MSG(("MSWriteParser::readFFNTB Font table is truncated\n"));
				break;
			}

			// We're not interested in the font family
			input->seek(1, librevenge::RVNG_SEEK_CUR);

			// Read font name
			unsigned fnlen = cbFfn - 1;
			unsigned long read_bytes;
			const unsigned char *fn = input->read(fnlen, read_bytes);
			if (read_bytes != fnlen)
			{
				WPS_DEBUG_MSG(("MSWriteParser::readFFNTB failed to read font name\n"));
				throw (libwps::ParseException());
			}

			librevenge::RVNGString fontname;

			// Remove trailing 0s
			while (fnlen > 0 && !fn[fnlen - 1])
				fnlen--;

			fontname = libwps_tools_win::Font::unicodeString(fn, fnlen, m_fontType);

			m_fonts.push_back(fontname);
		}
	}
	if (m_fonts.empty())
		m_fonts.push_back("Arial");
}

void MSWriteParser::readSECT()
{
	unsigned pnSetb, pnBftb;
	RVNGInputStreamPtr input = getInput();

	input->seek(MSWriteParserInternal::HEADER_W_PNSETB, librevenge::RVNG_SEEK_SET);
	pnSetb = libwps::readU16(input);

	input->seek(MSWriteParserInternal::HEADER_W_PNBFTB, librevenge::RVNG_SEEK_SET);
	pnBftb = libwps::readU16(input);

	MSWriteParserInternal::SEP sep;

	if (pnSetb && pnSetb != pnBftb)
	{
		if (!checkFilePosition(pnSetb * 0x80 + 14))
		{
			WPS_DEBUG_MSG(("Section is truncated\n"));
			throw (libwps::ParseException());
		}

		input->seek(pnSetb * 0x80, librevenge::RVNG_SEEK_SET);
		uint16_t cset = libwps::readU16(input);

		// ignore csetMax, cp, fn
		input->seek(8, librevenge::RVNG_SEEK_CUR);

		uint32_t fcSep = libwps::readU32(input);
		if (cset > 1 && checkFilePosition(fcSep + 22))
		{
			input->seek(fcSep, librevenge::RVNG_SEEK_SET);
			uint8_t headerSize = libwps::readU8(input);
			if (headerSize<22 || !checkFilePosition(fcSep+2+headerSize))
			{
				WPS_DEBUG_MSG(("MSWriteParser::readSECT: can not read the structure, using default\n"));
			}
			else
			{
				input->seek(2, librevenge::RVNG_SEEK_CUR); // skip reserved 1
				// read section
				sep.m_yaMac=double(libwps::readU16(input))/1440.;
				sep.m_xaMac=double(libwps::readU16(input))/1440.;
				sep.m_startPageNumber=libwps::readU16(input);
				sep.m_yaTop=double(libwps::readU16(input))/1440.;
				sep.m_dyaText=double(libwps::readU16(input))/1440.;
				sep.m_xaLeft=double(libwps::readU16(input))/1440.;
				sep.m_dxaText=double(libwps::readU16(input))/1440.;
				input->seek(2, librevenge::RVNG_SEEK_CUR); // skip reserved 2
				sep.m_yaHeader=double(libwps::readU16(input))/1440.;
				sep.m_yaFooter=double(libwps::readU16(input))/1440.;
			}
		}
	}

	m_pageSpan.setFormLength(sep.m_yaMac);
	m_pageSpan.setFormWidth(sep.m_xaMac);

	if (sep.m_yaTop < sep.m_yaMac && sep.m_yaMac - sep.m_yaTop - sep.m_dyaText >= 0 &&
	        sep.m_yaMac - sep.m_dyaText < sep.m_yaMac)
	{
		m_pageSpan.setMarginTop(sep.m_yaTop);
		m_pageSpan.setMarginBottom(sep.m_yaMac - sep.m_yaTop - sep.m_dyaText);
	}
	else
	{
		WPS_DEBUG_MSG(("MSWriteParser::readSECT: the top bottom margin seems bad\n"));
	}
	if (sep.m_xaLeft < sep.m_xaMac && sep.m_xaMac - sep.m_xaLeft - sep.m_dxaText >= 0 &&
	        sep.m_xaMac - sep.m_dxaText < sep.m_xaMac)
	{
		m_pageSpan.setMarginLeft(sep.m_xaLeft);
		m_pageSpan.setMarginRight(sep.m_xaMac - sep.m_xaLeft - sep.m_dxaText);
	}
	else
	{
		WPS_DEBUG_MSG(("MSWriteParser::readSECT: the left right margin seems bad\n"));
	}

	if (sep.m_startPageNumber != 0xffff)
		m_pageSpan.setPageNumber(sep.m_startPageNumber);
}

int MSWriteParser::numPages()
{
	int numPage = 1;

	RVNGInputStreamPtr input = getInput();

	std::vector<MSWriteParserInternal::Paragraph>::iterator paps;

	for (paps = m_paragraphList.begin(); paps != m_paragraphList.end(); ++paps)
	{
		if (paps->m_graphics)
			continue;

		uint32_t fc = paps->m_fcFirst;
		input->seek(fc, librevenge::RVNG_SEEK_SET);

		while (fc < paps->m_fcLim && fc < m_fcMac)
		{
			if (libwps::readU8(input) == 0x0C) numPage++;
			fc++;
		}
	}

	return numPage;
}

void MSWriteParser::readFOD(unsigned page, void (MSWriteParser::*parseFOD)(uint32_t fcFirst, uint32_t fcLim, unsigned size))
{
	RVNGInputStreamPtr input = getInput();
	unsigned fcLim, fc = 0x80;

	for (;;)
	{
		long const pageBegin=long(page * 0x80);

		if (!checkFilePosition(page * 0x80 + 0x7f))
		{
			WPS_DEBUG_MSG(("MSWriteParser::readFOD: FOD list is truncated\n"));
			break;
		}

		input->seek(pageBegin + 0x7f, librevenge::RVNG_SEEK_SET);
		uint8_t cfod = libwps::readU8(input);

		if (cfod > 20)
		{
			WPS_DEBUG_MSG(("MSWriteParser::readFOD: Too many FODs %d on one page\n", cfod));
			cfod = 20;
		}

		for (unsigned fod = 0; fod < cfod; ++fod)
		{
			input->seek(pageBegin + int(fod) * 6 + 4, librevenge::RVNG_SEEK_SET);
			fcLim = libwps::readU32(input);
			uint16_t bfProp = libwps::readU16(input);
			unsigned cch = 0;

			if (bfProp < 0x7f - 4)
			{
				input->seek(pageBegin + long(bfProp) + 4, librevenge::RVNG_SEEK_SET);
				cch = libwps::readU8(input);

				// Does it fit on the page
				if ((bfProp + cch + 4) >= 0x80)
				{
					cch = 0;
					WPS_DEBUG_MSG(("MSWriteParser::readFOD: entry %d on page %d invalid\n", fod, page));
				}
			}

			(this->*parseFOD)(fc, fcLim, cch);

			if (fcLim >= m_fcMac)
				return;

			fc = fcLim;
		}
		page++;
	}
}

void MSWriteParser::readPAP(uint32_t fcFirst, uint32_t fcLim, unsigned cch)
{
	RVNGInputStreamPtr input = getInput();

	struct MSWriteParserInternal::PAP pap;

	WPS_LE_PUT_GUINT16(&pap.m_dyaLine, 240);

	if (cch)
	{
		if (cch > sizeof(pap))
			cch = sizeof(pap);

		unsigned long read_bytes;
		const unsigned char *p = input->read(cch, read_bytes);
		if (read_bytes != cch)
		{
			WPS_DEBUG_MSG(("MSWriteParser::readPAP failed to read PAP\n"));
			throw (libwps::ParseException());
		}

		memcpy(&pap, p, cch);
	}

	int16_t dxaLeft = (int16_t) WPS_LE_GET_GUINT16(&pap.m_dxaLeft);
	int16_t dxaLeft1 = (int16_t) WPS_LE_GET_GUINT16(&pap.m_dxaLeft1);
	int16_t dxaRight = (int16_t) WPS_LE_GET_GUINT16(&pap.m_dxaRight);

	MSWriteParserInternal::Paragraph para;
	int i;

	for (i=0; i<14; i++)
	{
		uint16_t pos = WPS_LE_GET_GUINT16(&pap.m_TBD[i].m_dxa);

		if (!pos)
			break;

		WPSTabStop tab(pos / 1440., (pap.m_TBD[i].m_jcTab & 3) == 3 ?
		               WPSTabStop::DECIMAL : WPSTabStop::LEFT);

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

	uint16_t dyaLine = WPS_LE_GET_GUINT16(&pap.m_dyaLine);
	para.m_interLine = dyaLine / 240.0;

	para.m_fcFirst = fcFirst;
	para.m_fcLim = fcLim;
	if (pap.m_rhcPage & 0x10)
	{
		para.m_graphics = true;
		/* MS Write doesn't take first indent into
		    account for images, but odt does */
		para.m_margins[0] = 0.0;
	}
	else
	{
		if (pap.m_rhcPage & 6)
		{
			if (pap.m_rhcPage & 1)
			{
				para.m_Location = MSWriteParserInternal::Paragraph::FOOTER;
			}
			else
			{
				para.m_Location = MSWriteParserInternal::Paragraph::HEADER;
			}
			para.m_firstpage = (pap.m_rhcPage & 0x08) != 0;
		}
	}

	m_paragraphList.push_back(para);
}

void MSWriteParser::readCHP(uint32_t fcFirst, uint32_t fcLim, unsigned cch)
{
	RVNGInputStreamPtr input = getInput();

	struct MSWriteParserInternal::CHP chp;

	chp.m_hps = 24;

	if (cch)
	{
		if (cch > sizeof(chp))
			cch = sizeof(chp);

		unsigned long read_bytes;
		const unsigned char *p = input->read(cch, read_bytes);
		if (read_bytes != cch)
		{
			WPS_DEBUG_MSG(("MSWriteParser::readCHP failed to read CHP entry\n"));
			throw (libwps::ParseException());
		}

		memcpy(&chp, p, cch);
	}

	MSWriteParserInternal::Font font;

	unsigned ftc = (chp.m_fBold / 4) | ((chp.m_ftcXtra & 7) * 64);
	if (ftc >= m_fonts.size()) ftc = 0;

	font.m_name = m_fonts[ftc];
	font.m_size = chp.m_hps / 2.0;
	if (chp.m_fBold & 1)
		font.m_attributes |= WPS_BOLD_BIT;
	if (chp.m_fBold & 2)
		font.m_attributes |= WPS_ITALICS_BIT;
	if (chp.m_fUline & 1)
		font.m_attributes |= WPS_UNDERLINE_BIT;
	if (chp.m_fUline & 0x40)
		font.m_special = true;
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

	m_fontList.push_back(font);
}

shared_ptr<WPSContentListener> MSWriteParser::createListener(librevenge::RVNGTextInterface *interface)
{
	bool headerPage1 = true, footerPage1 = true;
	std::vector<WPSPageSpan> pageList;
	WPSPageSpan ps(m_pageSpan);
	WPSEntry empty, footer, header;
	WPSPageSpan::HeaderFooterOccurrence headerOccurrence = WPSPageSpan::ALL;
	WPSPageSpan::HeaderFooterOccurrence footerOccurrence = WPSPageSpan::ALL;

	MSWriteParserInternal::Paragraph::Location location = MSWriteParserInternal::Paragraph::MAIN;

	unsigned first = 0, i;

	for (i = 0; i < m_paragraphList.size(); i++)
	{
		MSWriteParserInternal::Paragraph &p = m_paragraphList[i];

		if (p.m_Location == MSWriteParserInternal::Paragraph::FOOTNOTE)
			break;

		if (p.m_Location != location)
		{
			if (location == MSWriteParserInternal::Paragraph::HEADER)
			{
				headerPage1 = m_paragraphList[first].m_firstpage;
				headerOccurrence = m_paragraphList[first].m_HeaderFooterOccurrence;
				header.setBegin(m_paragraphList[first].m_fcFirst);
				header.setEnd(m_paragraphList[i - 1].m_fcLim);
				header.setType("TEXT");
			}
			else if (location == MSWriteParserInternal::Paragraph::FOOTER)
			{
				footerPage1 = m_paragraphList[first].m_firstpage;
				footerOccurrence = m_paragraphList[first].m_HeaderFooterOccurrence;
				footer.setBegin(m_paragraphList[first].m_fcFirst);
				footer.setEnd(m_paragraphList[i - 1].m_fcLim);
				footer.setType("TEXT");
			}

			location = p.m_Location;
			first = i;
		}
	}

	if (i < 1)
	{
		WPS_DEBUG_MSG(("MSWriteParser::createListener: missing body text\n"));
		throw (libwps::ParseException());
	}

	m_Main.setBegin(m_paragraphList[first].m_fcFirst);
	m_Main.setEnd(m_paragraphList[i - 1].m_fcLim);
	if (m_Main.end() > m_fcMac)
		m_Main.setEnd(m_fcMac);
	m_Main.setType("TEXT");

	empty.setType("TEXT");

	WPSSubDocumentPtr subemptydoc(new MSWriteParserInternal::SubDocument
	                              (getInput(), *this, empty));

	if (header.valid())
	{
		WPSSubDocumentPtr subdoc(new MSWriteParserInternal::SubDocument
		                         (getInput(), *this, header));
		ps.setHeaderFooter(WPSPageSpan::HEADER, WPSPageSpan::ALL, subdoc);

		if (!headerPage1)
			ps.setHeaderFooter(WPSPageSpan::HEADER, headerOccurrence, subemptydoc);
	}

	if (footer.valid())
	{
		WPSSubDocumentPtr subdoc(new MSWriteParserInternal::SubDocument
		                         (getInput(), *this, footer));
		ps.setHeaderFooter(WPSPageSpan::FOOTER, footerOccurrence, subdoc);

		if (!footerPage1)
			ps.setHeaderFooter(WPSPageSpan::FOOTER, WPSPageSpan::FIRST, subemptydoc);
	}

	ps.setPageSpan(numPages());
	pageList.push_back(ps);
	return shared_ptr<WPSContentListener>
	       (new WPSContentListener(pageList, interface));
}

void MSWriteParser::readText(WPSEntry e)
{
	uint32_t fc = (uint32_t) e.begin();
	std::vector<MSWriteParserInternal::Paragraph>::iterator paps;
	std::vector<MSWriteParserInternal::Font>::iterator chps;
	paps = m_paragraphList.begin();
	chps = m_fontList.begin();
	RVNGInputStreamPtr input = getInput();
	float lastObjectHeight = 0.0;

	while (fc < uint32_t(e.end()))
	{
		bool skiptab = false;

		while (fc >= paps->m_fcLim)
		{
			if (++paps == m_paragraphList.end())
			{
				WPS_DEBUG_MSG(("MSWriteParser::readText PAP not found for offset %u\n", fc));
				throw (libwps::ParseException());
			}
			skiptab = paps->m_skiptab;
		}

		if (paps->m_graphics)
		{
			m_listener->setParagraph(*paps);

			// The last pap can have m_fcLim of greater than m_fcMac
			unsigned fcLim = std::min(paps->m_fcLim, m_fcMac);

			if (fcLim - fc < 40)
			{
				WPS_DEBUG_MSG(("MSWriteParser::readText object too short\n"));
				fc = paps->m_fcLim;
				continue;
			}

			WPSPosition pos;
			WPSPosition::XPos align;

			switch (paps->m_justify)
			{
			case libwps::JustificationFull:
			case libwps::JustificationFullAllLines:
			case libwps::JustificationLeft:
			default:
				align = WPSPosition::XLeft;
				break;
			case libwps::JustificationRight:
				align = WPSPosition::XRight;
				break;
			case libwps::JustificationCenter:
				align = WPSPosition::XCenter;
				break;
			}

			pos.setRelativePosition(WPSPosition::ParagraphContent, align);

			input->seek(fc + 8, librevenge::RVNG_SEEK_SET);
			uint16_t dxaOffset = libwps::readU16(input);
			uint16_t dxaSize = libwps::readU16(input);
			uint16_t dyaSize = libwps::readU16(input);

			input->seek(22, librevenge::RVNG_SEEK_CUR);
			uint16_t mx = libwps::readU16(input);
			uint16_t my = libwps::readU16(input);

			WPS_DEBUG_MSG(("MSWriteParser::readText object found %utwx%utw, offset %utw, mx=%u my=%u\n", dxaSize, dyaSize, dxaOffset, mx, my));

			Vec2f size(Vec2f(dxaSize/1440.0f, dyaSize/1440.0f));
			if ((mx > 10 && my > 10) && !(mx == 1000 && my == 1000))
			{
				size[0] *= mx / 1000.0f;
				size[1] *= my / 1000.0f;
			}

			pos.setUnit(librevenge::RVNG_INCH);
			pos.setSize(size);

			Vec2f origin;
			if (dxaOffset && align == WPSPosition::XLeft)
				origin[0] = dxaOffset/1440.0f;
			origin[1] = lastObjectHeight;
			pos.setOrigin(origin);
			input->seek(fc, librevenge::RVNG_SEEK_SET);

			processObject(pos, fcLim);
			lastObjectHeight += pos.size()[1];
			fc = paps->m_fcLim;
			continue;
		}

		lastObjectHeight = 0.0f;

		while (fc >= chps->m_fcLim)
		{
			if (++chps == m_fontList.end())
			{
				WPS_DEBUG_MSG(("MSWriteParser::readText CHP not found for offset %u\n", fc));
				throw (libwps::ParseException());
			}
		}

		MSWriteParserInternal::Paragraph para = *paps;
		para.setInterline((paps->m_interLine * chps->m_size)/72., librevenge::RVNG_INCH, WPSParagraph::AtLeast);

		if (!para.m_headerUseMargin && (para.m_Location == MSWriteParserInternal::Paragraph::HEADER ||
		                                para.m_Location == MSWriteParserInternal::Paragraph::FOOTER))
		{
			// Indents in header/footer are off paper, not margins
			para.m_margins[1] -= m_pageSpan.getMarginLeft();
			para.m_margins[2] -= m_pageSpan.getMarginRight();
		}

		m_listener->setParagraph(para);
		m_listener->setFont(*chps);

		uint32_t lim = std::min(chps->m_fcLim, paps->m_fcLim);
		lim = std::min(lim, m_fcMac);

		while (fc < lim)
		{
			unsigned size = lim - fc;
			unsigned long read_bytes;

			input->seek(fc, librevenge::RVNG_SEEK_SET);
			const unsigned char *p = input->read(size, read_bytes);
			if (!p || read_bytes != size)
			{
				WPS_DEBUG_MSG(("MSWriteParser::readText failed to read\n"));
				throw (libwps::ParseException());
			}

			if (chps->m_special)
			{
				insertSpecial(p[0], fc);
				size = 1;
			}
			else if (chps->m_footnote)
			{
				if (paps->m_Location != MSWriteParserInternal::Paragraph::FOOTNOTE)
				{
					librevenge::RVNGString label = libwps_tools_win::Font::unicodeString(p, size, chps->m_encoding);
					insertFootnote(false, fc, label);
				}

			}
			else
			{
				if (p[0] == 9 && skiptab)
				{
					size--;
					p++;
					fc++;
				}
				if (size)
					size = insertString(p, size, chps->m_encoding);
			}
			fc += size;

			skiptab = false;
		}
	}
}

void MSWriteParser::insertSpecial(uint8_t val, uint32_t /*fc*/)
{
	if (val == 1)
		m_listener->insertField(WPSContentListener::PageNumber);
}

void MSWriteParser::insertControl(uint8_t val)
{
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
	case 30:
		m_listener->insertUnicode(0x20);
		break;
	case 31: // soft hyphen (ignored by Write)
	case 13: // carriage return
		break;
	default:
		// MS Write displays these as boxes
		m_listener->insertUnicode(0x25af);
		break;
	}
}

unsigned MSWriteParser::insertString(const unsigned char *str, unsigned size, libwps_tools_win::Font::Type type)
{
	unsigned len = 0;

	while (len < size && str[len] >= ' ')
		len++;

	if (len == 0)
	{
		insertControl(str[0]);
		len = 1;
	}
	else
	{
		librevenge::RVNGString convert;

		convert = libwps_tools_win::Font::unicodeString(str, len, type);

		m_listener->insertUnicodeString(convert);
	}

	return len;
}

bool MSWriteParser::readString(std::string &res, unsigned long lastPos)
{
	RVNGInputStreamPtr input = getInput();
	unsigned size = libwps::readU32(input);
	if ((unsigned long)input->tell() + size > lastPos || !checkFilePosition((uint32_t) lastPos))
	{
		WPS_DEBUG_MSG(("MSWriteParser::readString too short\n"));
		return false;
	}

	if (size == 0)
	{
		res = std::string();
		return true;
	}

	unsigned long read_bytes;
	const unsigned char *data = input->read(size, read_bytes);
	if (read_bytes != size)
	{
		WPS_DEBUG_MSG(("MSWriteParser::readString failed to read\n"));
		throw (libwps::ParseException());
	}

	// Must be ascii
	for (unsigned i=0; i < size - 1; i++)
	{
		if (data[i] < ' ' || data[i] >= 0x7f)
		{
			WPS_DEBUG_MSG(("MSWriteParser::readString non-ascii found\n"));
			return false;
		}
	}
	if (data[size - 1])
		return false;

	res = std::string((const char *)data, size - 1u);

	return true;
}

void MSWriteParser::processObject(WPSPosition &pos, unsigned long lastPos)
{
	if (!checkFilePosition((uint32_t) lastPos))
	{
		WPS_DEBUG_MSG(("MSWriteParser::processObject last position is bad\n"));
		return;
	}
	RVNGInputStreamPtr input = getInput();
	unsigned mm = libwps::readU16(input);

	if (mm == 0x88)   // Windows metafile (.wmf)
	{
		// Step over unused fields
		input->seek(30, librevenge::RVNG_SEEK_CUR);

		unsigned cbSize = libwps::readU32(input);

		// Step over unused fields
		input->seek(4, librevenge::RVNG_SEEK_CUR);

		if ((unsigned long)input->tell() + cbSize > lastPos)
		{
			WPS_DEBUG_MSG(("MSWriteParser::processObject bad size for wmf\n"));
			return;
		}
		WPS_DEBUG_MSG(("MSWriteParser::processObject 3.0 WMF object\n"));

		librevenge::RVNGBinaryData wmfdata;
		if (processWMF(wmfdata, cbSize))
		{
			m_listener->insertPicture(pos, wmfdata, "application/x-wmf");
		}
	}
	else if (mm == 0xe3)     // this is a picture
	{
		// Step over unused fields
		input->seek(16, librevenge::RVNG_SEEK_CUR);

		// read BITMAP structure
		unsigned width = libwps::readU16(input);
		unsigned height = libwps::readU16(input);
		unsigned byte_width = libwps::readU16(input);
		unsigned planes = libwps::readU8(input);
		unsigned bits_pixel = libwps::readU8(input);

		// Step over unused fields
		input->seek(6, librevenge::RVNG_SEEK_CUR);

		unsigned cbSize = libwps::readU32(input);

		input->seek(4, librevenge::RVNG_SEEK_CUR);

		if ((unsigned long)input->tell() + cbSize > lastPos)
		{
			WPS_DEBUG_MSG(("MSWriteParser::processObject bad size for DDB\n"));
			return;
		}

		WPS_DEBUG_MSG(("MSWriteParser::processObject 3.0 DDB object %ux%u\n", width, height));

		librevenge::RVNGBinaryData bmpdata;
		if (processDDB(bmpdata, pos, width, height, byte_width, planes, bits_pixel, cbSize))
		{
			m_listener->insertPicture(pos, bmpdata, "image/bmp");
		}
	}
	else if (mm == 0xe4)     // OLE object
	{
		// Step over unused fields
		input->seek(38, librevenge::RVNG_SEEK_CUR);

		unsigned ole_id = libwps::readU32(input);
		unsigned type = libwps::readU32(input);

		if (ole_id != 0x501)
		{
			WPS_DEBUG_MSG(("MSWriteParser::processObject Unknown ole identifier %x\n", ole_id));
			return;
		}

		switch (type)
		{
		case 3: // static OLE
		{
			librevenge::RVNGBinaryData staticOle;
			std::string mimetype;
			if (processStaticOLE(staticOle, mimetype, pos, lastPos))
			{
				m_listener->insertPicture(pos, staticOle, mimetype);
			}
			return;
		}
		case 2: // Embedded OLE
			processEmbeddedOLE(pos, lastPos);
			return;
		case 1:
			WPS_DEBUG_MSG(("MSWriteParser::processObject:OLE link\n"));
			return;
		default:
			WPS_DEBUG_MSG(("MSWriteParser::processObject:unknown ole type %d\n", type));
			return;
		}
	}
	else
	{
		WPS_DEBUG_MSG(("MSWriteParser::processObject:Unknown object encountered (mm=%02x)\n", mm));
	}
}

bool MSWriteParser::processDDB(librevenge::RVNGBinaryData &bmpdata, WPSPosition &pos, unsigned width, unsigned height, unsigned byte_width, unsigned planes, unsigned bits_pixel, unsigned size)
{
	/* A ddb image has no color palette; it is BMP Version 1, but this format
	   is not widely supported. Create BMP version 2 file with a color palette.
	 */
	static const MSWriteParserInternal::BitmapPalette pal1[2] =
	{
		{ 0,0,0 }, { 255,255,255 }
	};
	static const MSWriteParserInternal::BitmapPalette pal4[16] =
	{
		{ 0,0,0 }, // black
		{ 0,0,255 }, // Blue
		{ 0,128,0 },	// Cyan
		{ 0,255,0 }, // Green
		{ 192,0,192 }, // Magenta
		{ 255,0,0 }, // Red
		{ 255,255,0 }, // Yellow
		{ 255,255,255 }, // White

		{ 0,0,128 }, // Dark blue
		{ 0,128,128 }, // Dark cyan
		{ 0,128,0 }, // Dark green
		{ 128,0,128 }, // Dark magenta
		{ 128,0,0  }, // Dark red
		{ 192,192,0 }, // Dark yellow
		{ 192,192,192 }, // Light gray
		{ 128,128,128 }, // Dark gray
	};
	static const MSWriteParserInternal::BitmapPalette pal8[256] =
	{
		{ 0,0,0 }, { 128,0,0 }, { 0,128,0 }, { 128,128,0 },
		{ 0,0,128 }, { 128,0,128 }, { 0,128,128 }, { 192,192,192 },
		{ 192,220,192 }, { 166,202,240 }, { 42,63,170 }, { 42,63,255 },
		{ 42,95,0 }, { 42,95,85 }, { 42,95,170 }, { 42,95,255 },
		{ 42,127,0 }, { 42,127,85 }, { 42,127,170 }, { 42,127,255 },
		{ 42,159,0 }, { 42,159,85 }, { 42,159,170 }, { 42,159,255 },
		{ 42,191,0 }, { 42,191,85 }, { 42,191,170 }, { 42,191,255 },
		{ 42,223,0 }, { 42,223,85 }, { 42,223,170 }, { 42,223,255 },
		{ 42,255,0 }, { 42,255,85 }, { 42,255,170 }, { 42,255,255 },
		{ 85,0,0 }, { 85,0,85 }, { 85,0,170 }, { 85,0,255 },
		{ 85,31,0 }, { 85,31,85 }, { 85,31,170 }, { 85,31,255 },
		{ 85,63,0 }, { 85,63,85 }, { 85,63,170 }, { 85,63,255 },
		{ 85,95,0 }, { 85,95,85 }, { 85,95,170 }, { 85,95,255 },
		{ 85,127,0 }, { 85,127,85 }, { 85,127,170 }, { 85,127,255 },
		{ 85,159,0 }, { 85,159,85 }, { 85,159,170 }, { 85,159,255 },
		{ 85,191,0 }, { 85,191,85 }, { 85,191,170 }, { 85,191,255 },
		{ 85,223,0 }, { 85,223,85 }, { 85,223,170 }, { 85,223,255 },
		{ 85,255,0 }, { 85,255,85 }, { 85,255,170 }, { 85,255,255 },
		{ 127,0,0 }, { 127,0,85 }, { 127,0,170 }, { 127,0,255 },
		{ 127,31,0 }, { 127,31,85 }, { 127,31,170 }, { 127,31,255 },
		{ 127,63,0 }, { 127,63,85 }, { 127,63,170 }, { 127,63,255 },
		{ 127,95,0 }, { 127,95,85 }, { 127,95,170 }, { 127,95,255 },
		{ 127,127,0 }, { 127,127,85 }, { 127,127,170 }, { 127,127,255 },
		{ 127,159,0 }, { 127,159,85 }, { 127,159,170 }, { 127,159,255 },
		{ 127,191,0 }, { 127,191,85 }, { 127,191,170 }, { 127,191,255 },
		{ 127,223,0 }, { 127,223,85 }, { 127,223,170 }, { 127,223,255 },
		{ 127,255,0 }, { 127,255,85 }, { 127,255,170 }, { 127,255,255 },
		{ 170,0,0 }, { 170,0,85 }, { 170,0,170 }, { 170,0,255 },
		{ 170,31,0 }, { 170,31,85 }, { 170,31,170 }, { 170,31,255 },
		{ 170,63,0 }, { 170,63,85 }, { 170,63,170 }, { 170,63,255 },
		{ 170,95,0 }, { 170,95,85 }, { 170,95,170 }, { 170,95,255 },
		{ 170,127,0 }, { 170,127,85 }, { 170,127,170 }, { 170,127,255 },
		{ 170,159,0 }, { 170,159,85 }, { 170,159,170 }, { 170,159,255 },
		{ 170,191,0 }, { 170,191,85 }, { 170,191,170 }, { 170,191,255 },
		{ 170,223,0 }, { 170,223,85 }, { 170,223,170 }, { 170,223,255 },
		{ 170,255,0 }, { 170,255,85 }, { 170,255,170 }, { 170,255,255 },
		{ 212,0,0 }, { 212,0,85 }, { 212,0,170 }, { 212,0,255 },
		{ 212,31,0 }, { 212,31,85 }, { 212,31,170 }, { 212,31,255 },
		{ 212,63,0 }, { 212,63,85 }, { 212,63,170 }, { 212,63,255 },
		{ 212,95,0 }, { 212,95,85 }, { 212,95,170 }, { 212,95,255 },
		{ 212,127,0 }, { 212,127,85 }, { 212,127,170 }, { 212,127,255 },
		{ 212,159,0 }, { 212,159,85 }, { 212,159,170 }, { 212,159,255 },
		{ 212,191,0 }, { 212,191,85 }, { 212,191,170 }, { 212,191,255 },
		{ 212,223,0 }, { 212,223,85 }, { 212,223,170 }, { 212,223,255 },
		{ 212,255,0 }, { 212,255,85 }, { 212,255,170 }, { 212,255,255 },
		{ 255,0,85 }, { 255,0,170 }, { 255,31,0 }, { 255,31,85 },
		{ 255,31,170 }, { 255,31,255 }, { 255,63,0 }, { 255,63,85 },
		{ 255,63,170 }, { 255,63,255 }, { 255,95,0 }, { 255,95,85 },
		{ 255,95,170 }, { 255,95,255 }, { 255,127,0 }, { 255,127,85 },
		{ 255,127,170 }, { 255,127,255 }, { 255,159,0 }, { 255,159,85 },
		{ 255,159,170 }, { 255,159,255 }, { 255,191,0 }, { 255,191,85 },
		{ 255,191,170 }, { 255,191,255 }, { 255,223,0 }, { 255,223,85 },
		{ 255,223,170 }, { 255,223,255 }, { 255,255,85 }, { 255,255,170 },
		{ 204,204,255 }, { 255,204,255 }, { 51,255,255 }, { 102,255,255 },
		{ 153,255,255 }, { 204,255,255 }, { 0,127,0 }, { 0,127,85 },
		{ 0,127,170 }, { 0,127,255 }, { 0,159,0 }, { 0,159,85 },
		{ 0,159,170 }, { 0,159,255 }, { 0,191,0 }, { 0,191,85 },
		{ 0,191,170 }, { 0,191,255 }, { 0,223,0 }, { 0,223,85 },
		{ 0,223,170 }, { 0,223,255 }, { 0,255,85 }, { 0,255,170 },
		{ 42,0,0 }, { 42,0,85 }, { 42,0,170 }, { 42,0,255 },
		{ 42,31,0 }, { 42,31,85 }, { 42,31,170 }, { 42,31,255 },
		{ 42,63,0 }, { 42,63,85 }, { 255,251,240 }, { 160,160,164 },
		{ 128,128,128 }, { 255,0,0 }, { 0,255,0 }, { 255,255,0 },
		{ 0,0,255 }, { 255,0,255 }, { 0,255,255 }, { 255,255,255 }
	};

	if (size < byte_width * height)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processDDB DDB image data missing, skipping\n"));
		return false;
	}

	if (planes != 1)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processDDB DDB image has planes %d, must be 1\n", planes));
		return false;
	}

	WPS_DEBUG_MSG(("MSWriteParser::processDDB DDB size %ux%ux%u, byte width %u\n", width, height,
	               bits_pixel, byte_width));

	unsigned colors;
	switch (bits_pixel)
	{
	case 1:
		colors = 2;
		break;
	case 4:
		colors = 16;
		break;
	case 8:
		colors = 256;
		break;
	default: // no palette
		colors = 0;
		break;
	}

	unsigned offset = MSWriteParserInternal::BM_FILE_STRUCT_SIZE + MSWriteParserInternal::BM_INFO_V2_STRUCT_SIZE + colors * unsigned(sizeof(MSWriteParserInternal::BitmapPalette));

	// File header
	bmpdata.append('B');
	bmpdata.append('M');
	MSWriteParserInternal::appendU32(bmpdata, offset + height * byte_width);
	MSWriteParserInternal::appendU32(bmpdata, 0);
	MSWriteParserInternal::appendU32(bmpdata, offset);

	// Info v2 header
	MSWriteParserInternal::appendU32(bmpdata, 12);
	MSWriteParserInternal::appendU16(bmpdata, (uint16_t) width);
	MSWriteParserInternal::appendU16(bmpdata, (uint16_t) height);
	MSWriteParserInternal::appendU16(bmpdata, (uint16_t) planes);
	MSWriteParserInternal::appendU16(bmpdata, (uint16_t) bits_pixel);

	switch (bits_pixel)
	{
	case 1:
		bmpdata.append((const unsigned char *)&pal1, sizeof(pal1));
		break;
	case 4:
		bmpdata.append((const unsigned char *)&pal4, sizeof(pal4));
		break;
	case 8:
		bmpdata.append((const unsigned char *)&pal8, sizeof(pal8));
		break;
	default:
		break;
	}

	RVNGInputStreamPtr input = getInput();
	unsigned long read_bytes;
	const unsigned char *data = input->read(size, read_bytes);
	if (read_bytes != size)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processDDB failed to read object\n"));
		throw (libwps::ParseException());
	}

	// Scanlines must be on 4-byte boundary, but some DDB images have
	// byte_width which is not on a 4-byte boundary. Also flip the image
	// vertically, DDB are stored upside down
	for (unsigned i=0; i<height; i++)
	{
		bmpdata.append(data + byte_width * (height - i - 1), byte_width);
		if (byte_width % 4)
			bmpdata.append(data, 4 - byte_width % 4);
	}

	// default is 96 dpi
	pos.setSize(Vec2f(float(width) / 96.0f, float(height) / 96.0f));

	return true;
}

bool MSWriteParser::processDIB(librevenge::RVNGBinaryData &bmpdata, unsigned size)
{
	if (size < MSWriteParserInternal::BM_INFO_V3_STRUCT_SIZE)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processDIB DIB image missing data, skipping\n"));
		return false;
	}

	RVNGInputStreamPtr input = getInput();
	unsigned long read_bytes;
	const unsigned char *data = input->read(size, read_bytes);
	if (read_bytes != size)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processDIB failed to read object\n"));
		throw (libwps::ParseException());
	}

	if (WPS_LE_GET_GUINT32(data + MSWriteParserInternal::BM_INFO_V3_SIZE) != MSWriteParserInternal::BM_INFO_V3_STRUCT_SIZE)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processDIB DIB header incorrect size, skipping\n"));
		return false;
	}

	unsigned bits_pixel = WPS_LE_GET_GUINT16(data + MSWriteParserInternal::BM_INFO_V3_BITS_PIXEL);
	unsigned colors_used = WPS_LE_GET_GUINT16(data + MSWriteParserInternal::BM_INFO_V3_COLORS_USED);

	unsigned colors = 0;
	if (bits_pixel && bits_pixel <= 8)
	{
		colors = colors_used ? colors_used : 1 << bits_pixel;
	}

	bmpdata.append('B');
	bmpdata.append('M');
	MSWriteParserInternal::appendU32(bmpdata, size + MSWriteParserInternal::BM_FILE_STRUCT_SIZE);
	MSWriteParserInternal::appendU32(bmpdata, 0);
	MSWriteParserInternal::appendU32(bmpdata, MSWriteParserInternal::BM_FILE_STRUCT_SIZE + MSWriteParserInternal::BM_INFO_V3_STRUCT_SIZE + 4 * colors);

	bmpdata.append(data, size);

	return true;
}

bool MSWriteParser::processStaticOLE(librevenge::RVNGBinaryData &b, std::string &mimetype, WPSPosition &pos, unsigned long lastPos)
{
	RVNGInputStreamPtr input = getInput();
	std::string objtype;

	if (!readString(objtype, lastPos))
	{
		return false;
	}
	WPS_DEBUG_MSG(("MSWriteParser::processStaticOLE object %s\n", objtype.c_str()));

	// Step over unused fields
	input->seek(8, librevenge::RVNG_SEEK_CUR);

	unsigned cbSize = libwps::readU32(input);

	if ((unsigned long)input->tell() + cbSize > lastPos)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processStaticOLE bad size\n"));
		return false;
	}

	if (objtype == "BITMAP")
	{
		if (cbSize < 10)
		{
			WPS_DEBUG_MSG(("MSWriteParser::processStaticOLE bad size for DDB\n"));
			return false;
		}
		input->seek(2, librevenge::RVNG_SEEK_CUR);
		unsigned width = libwps::readU16(input);
		unsigned height = libwps::readU16(input);
		unsigned byte_width = libwps::readU16(input);
		unsigned planes = libwps::readU8(input);
		unsigned bits_pixel = libwps::readU8(input);

		mimetype = "image/bmp";

		return processDDB(b, pos, width, height, byte_width, planes, bits_pixel, cbSize - 10);
	}
	else if (objtype == "DIB")
	{
		mimetype = "image/bmp";

		return processDIB(b, cbSize);
	}
	else if (objtype == "METAFILEPICT" && cbSize>8)
	{
		// Step over unused fields
		input->seek(8, librevenge::RVNG_SEEK_CUR);
		mimetype = "application/x-wmf";
		return processWMF(b, cbSize - 8);
	}

	return true;
}

void MSWriteParser::processEmbeddedOLE(WPSPosition &pos, unsigned long lastPos)
{
	if (!checkFilePosition((uint32_t) lastPos))
	{
		WPS_DEBUG_MSG(("MSWriteParser::processEmbeddedOLE last position is bad\n"));
		return;
	}
	RVNGInputStreamPtr input = getInput();

	// First read entire Object including ole_id and type
	long startPos = input->tell();
	input->seek(-8, librevenge::RVNG_SEEK_CUR);

	unsigned long read_bytes;
	unsigned long embeddedSize = lastPos - (unsigned long) input->tell();
	const unsigned char *data = input->read(embeddedSize, read_bytes);
	if (read_bytes != embeddedSize)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processEmbeddedOLE: failed to read object\n"));
		throw (libwps::ParseException());
	}

	librevenge::RVNGBinaryData embeddedOle;
	embeddedOle.append(data, embeddedSize);

	// Seek back to beginning; parse in search of replacement object
	input->seek(startPos, librevenge::RVNG_SEEK_SET);

	std::string strings[3];
	unsigned i;

	for (i=0; i<3; i++)
	{
		if (!readString(strings[i], lastPos))
		{
			return;
		}
	}

	WPS_DEBUG_MSG(("MSWriteParser::processEmbeddedOLE Embedded OLE object %s,filename=%s,parameter=%s\n",
	               strings[0].c_str(), strings[1].c_str(), strings[2].c_str()));
	unsigned size = libwps::readU32(input);

	if ((unsigned long) input->tell() + size > lastPos)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processEmbeddedOLE: bad size for object\n"));
		return;
	}

	if (strings[0] == "PBrush" || strings[0] == "Paint.Picture")
	{
		// fish out the bmp file
		data = input->read(size, read_bytes);
		if (read_bytes != size)
		{
			WPS_DEBUG_MSG(("MSWriteParser::processEmbeddedOLE: failed to read %s object\n", strings[0].c_str()));
			throw (libwps::ParseException());
		}

		librevenge::RVNGBinaryData bmpdata(data, size);
		m_listener->insertPicture(pos, bmpdata, "image/bmp");
		return;
	}

	input->seek(size, librevenge::RVNG_SEEK_CUR);

	WPSEmbeddedObject obj(embeddedOle, "object/ole");
	unsigned ole_id = libwps::readU32(input);
	unsigned type = libwps::readU32(input);
	librevenge::RVNGBinaryData replacement;
	std::string mimetype;

	if (ole_id == 0x501 && type == 5 && processStaticOLE(replacement, mimetype, pos, lastPos))
	{
		WPS_DEBUG_MSG(("MSWriteParser::processEmbeddedOLE found replacement object of mime %s\n", mimetype.c_str()));
		obj.add(replacement, mimetype);
	}
	m_listener->insertObject(pos, obj);
}

bool MSWriteParser::processWMF(librevenge::RVNGBinaryData &wmfdata, unsigned size)
{
	WPS_DEBUG_MSG(("MSWriteParser::processWMF Windows metafile (wmf)\n"));


	RVNGInputStreamPtr input = getInput();
	unsigned long read_bytes;
	const unsigned char *data = input->read(size, read_bytes);
	if (read_bytes != size)
	{
		WPS_DEBUG_MSG(("MSWriteParser::processWMF failed to read object\n"));
		throw (libwps::ParseException());
	}

	wmfdata.append(data, size);

	return true;
}

void MSWriteParser::readSUMD()
{
	// no metadata stored in MS Write
}

void MSWriteParser::readFNTB()
{
	// no footnotes supported in MS Write
}

void MSWriteParser::readStructures()
{
	RVNGInputStreamPtr input = getInput();

	readFIB();
	readFFNTB();
	readSECT();
	readSUMD();
	readFNTB();

	input->seek(MSWriteParserInternal::HEADER_W_PNPARA, librevenge::RVNG_SEEK_SET);
	unsigned pnPara = libwps::readU16(input);
	readFOD(pnPara, &MSWriteParser::readPAP);

	if (m_paragraphList.empty())
	{
		WPS_DEBUG_MSG(("MSWriteParser::parse: failed to read any PAP entries\n"));
		throw (libwps::ParseException());
	}

	readFOD((m_fcMac + 127) / 128, &MSWriteParser::readCHP);
	if (m_fontList.empty())
	{
		WPS_DEBUG_MSG(("MSWriteParser::parse: failed to read any CHP entries\n"));
		throw (libwps::ParseException());
	}
}

void MSWriteParser::insertFootnote(bool annotation, uint32_t fcPos, librevenge::RVNGString &label)
{
	std::vector<MSWriteParserInternal::Footnote>::iterator iter;

	for (iter = m_footnotes.begin(); iter != m_footnotes.end(); ++iter)
	{
		if (fcPos == iter->fcRef)
		{
			WPSEntry e;
			e.setBegin(iter->fcFtn);
			if (++iter == m_footnotes.end())
			{
				WPS_DEBUG_MSG(("MSWriteParser::insertFootnote missing sentinel footnote\n"));
				return;
			}
			e.setEnd(iter->fcFtn);
			e.setType("TEXT");

			if (e.valid() && e.begin() >= m_Main.end() && e.end() <= long(m_fcMac))
			{
				WPSSubDocumentPtr subdoc(new MSWriteParserInternal::SubDocument
				                         (getInput(), *this, e));
				if (annotation)
					m_listener->insertComment(subdoc);
				else if (label.size())
					m_listener->insertLabelNote(WPSContentListener::FOOTNOTE, label, subdoc);
				else
					m_listener->insertNote(WPSContentListener::FOOTNOTE, subdoc);
			}

			return;
		}
	}

	WPS_DEBUG_MSG(("MSWriteParser::insertFootnote footnote not found!\n"));
}

void MSWriteParser::parse(librevenge::RVNGTextInterface *document)
{
	readStructures();

	m_listener = createListener(document);
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("MSWriteParser::parse: can not create the listener\n"));

		throw (libwps::ParseException());
	}

	m_listener->setMetaData(m_metaData);
	m_listener->startDocument();
	if (m_Main.valid())
	{
		readText(m_Main);
	}
	m_listener->endDocument();
	m_listener.reset();
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
