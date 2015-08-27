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

#include <cstring>

namespace MSWriteParserInternal
{
//! Internal: the subdocument of a MSWriteParser
class SubDocument : public WPSTextSubDocument
{
public:
	//! type of an entry stored in textId
	enum Type { Unknown, MN };
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

	if (!m_entry.valid())
	{
		if (subDocumentType != libwps::DOC_COMMENT_ANNOTATION)
		{
			WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: empty document found...\n"));
		}
		listen->insertCharacter(' ');
		return;
	}

	MSWriteParser *mnParser = dynamic_cast<MSWriteParser *>(m_parser);
	if (!mnParser)
	{
		WPS_DEBUG_MSG(("MSWriteParserInternal::SubDocument::parse: bad parser...\n"));
		listen->insertCharacter(' ');
		return;
	}
	mnParser->readText(m_entry);
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

struct PAP // CHECKME: does we need packed here
{
	uint8_t	m_reserved1;
	uint8_t	m_justification;
	uint8_t	m_reserved2[2];
	uint16_t	m_dxaRight, m_dxaLeft, m_dxaLeft1, m_dyaLine;
	uint16_t	m_dyaBefore, m_dyaAfter;
	uint8_t	m_rhcPage;
	uint8_t	m_reserved3[5];
	struct
	{
		uint16_t    m_dxa;
		uint8_t	    m_jcTab;
		uint8_t	    m_chAlign;
	} m_TBD[14];
} __attribute__((packed));

struct CHP // CHECKME: does we need packed here
{
	uint8_t	m_reserved1;
	uint8_t	m_fBold;
	uint8_t	m_hps;
	uint8_t	m_fUline;
	uint8_t	m_ftcXtra;
	uint8_t	m_hpsPos;
} __attribute__((packed));

struct Paragraph :  public WPSParagraph
{
	enum Location { NORMAL, HEADER, FOOTER };
	Paragraph() : WPSParagraph(), m_fcFirst(0), m_fcLim(0), m_Location(NORMAL),
		m_graphics(false), m_firstpage(false)  { }
	uint32_t m_fcFirst, m_fcLim;
	Location m_Location;
	bool m_graphics, m_firstpage;
};

struct Font : public WPSFont
{
	Font() : WPSFont(), m_fcFirst(0), m_fcLim(0), m_special(false) { }
	uint32_t m_fcFirst, m_fcLim;
	bool m_special;
};

// the file header offsets
enum HeaderOffset
{
	HEADER_W_WIDENT	= 0,
	HEADER_W_DTY	= 2,
	HEADER_W_WTOOL	= 4,
	HEADER_D_FCMAC	= 14,
	HEADER_W_PNPARA	= 18,
	HEADER_W_PNFNTB	= 20,
	HEADER_W_PNSEP	= 22,
	HEADER_W_PNSETB	= 24,
	HEADER_W_PNBFTB	= 26,
	HEADER_W_PNFFNTB	= 28,
	HEADER_W_PNMAC	= 96
};

// pictures in Write 3.0, either DDB of WMF
enum PicOffset
{
	PIC_W_MM		= 0,
	PIC_W_XEXT		= 2,
	PIC_W_YEXT		= 4,
	PIC_W_DXAOFFSET	= 8,
	PIC_W_DXASIZE	= 10,
	PIC_W_DYASIZE	= 12,
	PIC_W_BMWIDTH	= 18,
	PIC_W_BMHEIGHT	= 20,
	PIC_W_BMWIDTHBYTES	= 22,
	PIC_B_BMPLANES	= 24,
	PIC_B_BMBITSPIXEL	= 25,
	PIC_D_CBSIZE	= 32,
	PIC_W_MX		= 36,
	PIC_W_MY		= 38
};

// OLE objects in Write 3.1
enum OleOffset
{
	OLE_W_MM		= 0,
	OLE_W_OBJECTTYPE	= 6,
	OLE_W_DXAOFFSET	= 8,
	OLE_W_DXASIZE	= 10,
	OLE_W_DYASIZE	= 12,
	OLE_D_DWDATASIZE	= 16,
	OLE_W_MX		= 36,
	OLE_W_MY		= 38
};

}

MSWriteParser::MSWriteParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
                             libwps_tools_win::Font::Type encoding):
	WPSParser(input, header),
	m_fileLength(0), m_fcMac(0), m_paragraphList(0), m_fontList(0),
	m_fonts(0), m_span(), m_fontType(encoding),
	m_listener(), m_Main(), m_Header(), m_Footer(),
	m_HeaderPage1(false), m_FooterPage1(false)
{
	if (!input)
	{
		WPS_DEBUG_MSG(("MSWriteParser::MSWriteParser: called without input\n"));
		throw libwps::ParseException();
	}
	input->seek(0, librevenge::RVNG_SEEK_END);
	m_fileLength=(uint32_t) input->tell();
	input->seek(0, librevenge::RVNG_SEEK_SET);
	if (m_fontType == libwps_tools_win::Font::UNKNOWN)
		m_fontType = libwps_tools_win::Font::WIN3_WEUROPE;
}

MSWriteParser::~MSWriteParser()
{
}

void MSWriteParser::readFIB()
{
	RVNGInputStreamPtr input = getInput();

	input->seek(MSWriteParserInternal::HEADER_D_FCMAC, librevenge::RVNG_SEEK_SET);
	m_fcMac = libwps::readU32(input);
}

void MSWriteParser::readFFNTB()
{
	int font_count, page, pnFfntb, pnMac;
	RVNGInputStreamPtr input = getInput();

	input->seek(MSWriteParserInternal::HEADER_W_PNFFNTB, librevenge::RVNG_SEEK_SET);
	page = pnFfntb = libwps::readU16(input);

	input->seek(MSWriteParserInternal::HEADER_W_PNMAC, librevenge::RVNG_SEEK_SET);
	pnMac = libwps::readU16(input);

	if (page == 0 || page == pnMac)
	{
		font_count = 0;
	}
	else
	{
		input->seek(page * 0x80, librevenge::RVNG_SEEK_SET);
		font_count = libwps::readU16(input);
	}

	if (font_count)
	{
		for (;;)
		{
			unsigned int cbFfn = libwps::readU16(input);
			if (cbFfn == 0)
				break;

			if (cbFfn == 0xffff)
			{
				input->seek(++page * 0x80, librevenge::RVNG_SEEK_SET);
				continue;
			}
			// We're not interested in the font family
			input->seek(1, librevenge::RVNG_SEEK_CUR);

			// Read font name
			unsigned long fnlen = cbFfn - 1, read_bytes;
			const unsigned char *fn = input->read(fnlen, read_bytes);
			if (read_bytes != fnlen) // CHECKME: better to print a message before throwing....
				throw (libwps::ParseException());

			// FIXME: some Japanese fonts have non-ascii font names
			std::string fontname((const char *)fn, (size_t)fnlen);

			m_fonts.push_back(fontname.c_str());
		}
	}
	if (m_fonts.empty())
		m_fonts.push_back("Arial");
}

void MSWriteParser::readSECT()
{
	int pnSetb, pnBftb;
	RVNGInputStreamPtr input = getInput();

	input->seek(MSWriteParserInternal::HEADER_W_PNSETB, librevenge::RVNG_SEEK_SET);
	pnSetb = libwps::readU16(input);

	input->seek(MSWriteParserInternal::HEADER_W_PNBFTB, librevenge::RVNG_SEEK_SET);
	pnBftb = libwps::readU16(input);

	MSWriteParserInternal::SEP sep;

	if (pnSetb && pnSetb != pnBftb)
	{
		input->seek(pnSetb * 0x80, librevenge::RVNG_SEEK_SET);
		uint16_t cset = libwps::readU16(input);

		// ignore csetMax, cp, fn
		input->seek(8, librevenge::RVNG_SEEK_CUR);

		uint32_t fcSep = libwps::readU32(input);
		if (cset > 1 && fcSep != 0xd1d1d1d1 && fcSep != 0xffffffff)
		{
			input->seek(fcSep, librevenge::RVNG_SEEK_SET);
			uint8_t headerSize = libwps::readU8(input);
			if (headerSize<22 || !checkFilePosition(fcSep+2+headerSize))
			{
				WPS_DEBUG_MSG(("MSWriteParser::readSECT: can not read the structure, stop\n"));
				// CHECKME: no need to throw ?
			}
			else
			{
				input->seek(2, librevenge::RVNG_SEEK_CUR); // skip reserved 1
				// read section
				sep.m_yaMac=double(libwps::readU16(input))/1440.;
				sep.m_xaMac=double(libwps::readU16(input))/1440.;
				input->seek(2, librevenge::RVNG_SEEK_CUR); // skip reserved 2
				sep.m_yaTop=double(libwps::readU16(input))/1440.;
				sep.m_dyaText=double(libwps::readU16(input))/1440.;
				sep.m_xaLeft=double(libwps::readU16(input))/1440.;
				sep.m_dxaText=double(libwps::readU16(input))/1440.;
				sep.m_startPageNumber=libwps::readU16(input);
				sep.m_yaHeader=double(libwps::readU16(input))/1440.;
				sep.m_yaFooter=double(libwps::readU16(input))/1440.;
			}
		}
	}

	m_span.setFormLength(sep.m_yaMac);
	m_span.setFormWidth(sep.m_xaMac);

	if (sep.m_yaTop < sep.m_yaMac && sep.m_yaMac - sep.m_yaTop - sep.m_dyaText >= 0 &&
	        sep.m_yaMac - sep.m_dyaText < sep.m_yaMac)
	{
		m_span.setMarginTop(sep.m_yaTop);
		m_span.setMarginBottom(sep.m_yaMac - sep.m_yaTop - sep.m_dyaText);
	}
	else
	{
		WPS_DEBUG_MSG(("MSWriteParser::readSECT: the top bottom margin seems bad\n"));
	}
	if (sep.m_xaLeft < sep.m_xaMac && sep.m_xaMac - sep.m_xaLeft - sep.m_dxaText >= 0 &&
	        sep.m_xaMac - sep.m_dxaText < sep.m_xaMac)
	{
		m_span.setMarginLeft(sep.m_xaLeft);
		m_span.setMarginRight(sep.m_xaMac - sep.m_xaLeft - sep.m_dxaText);
	}
	else
	{
		WPS_DEBUG_MSG(("MSWriteParser::readSECT: the left right margin seems bad\n"));
	}

	if (sep.m_startPageNumber != 0xffff)
		m_span.setPageNumber(sep.m_startPageNumber);
}

void MSWriteParser::readPAP()
{
	RVNGInputStreamPtr input = getInput();

	input->seek(MSWriteParserInternal::HEADER_W_PNPARA, librevenge::RVNG_SEEK_SET);
	unsigned pnPara = libwps::readU16(input);
	unsigned fcLim, fc = 0x80;

	for (;;)
	{
		input->seek(pnPara * 0x80 + 0x7f, librevenge::RVNG_SEEK_SET);
		unsigned cfod = libwps::readU8(input);

		for (unsigned fod = 0; fod < cfod; fod++)
		{
			input->seek(pnPara * 0x80 + fod * 6 + 4, librevenge::RVNG_SEEK_SET);
			fcLim = libwps::readU32(input);
			unsigned bfProp = libwps::readU16(input);
			struct MSWriteParserInternal::PAP pap;

			memset(&pap, 0, sizeof(pap));

			WPS_LE_PUT_GUINT16(&pap.m_dyaLine, 240);

			if (bfProp < 0x7f - 4)
			{
				input->seek(pnPara * 0x80 + bfProp + 4, librevenge::RVNG_SEEK_SET);
				unsigned cch = libwps::readU8(input);
				if (cch <= sizeof(pap) && (4 + bfProp + cch < 0x80))
				{
					unsigned long read_bytes;
					const unsigned char *p = input->read(cch, read_bytes);
					if (read_bytes != cch)
						throw (libwps::ParseException());

					memcpy(&pap, p, cch);
				}
				else
				{
					// CHECKME: better to add the function name in the message (to retrieve the position)
					WPS_DEBUG_MSG(("pap entry %d on page %d invalid\n", fod, pnPara));
				}
			}

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
			}

			switch (pap.m_justification & 3)
			{
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

			para.m_margins[0] = WPS_LE_GET_GUINT16(&pap.m_dxaLeft1) / 1440.0;
			para.m_margins[1] = WPS_LE_GET_GUINT16(&pap.m_dxaLeft) / 1440.0;
			para.m_margins[2] = WPS_LE_GET_GUINT16(&pap.m_dxaRight) / 1440.0;

			uint16_t dyaLine = WPS_LE_GET_GUINT16(&pap.m_dyaLine);
			para.m_spacings[0] = dyaLine / 240.0;

			para.m_fcFirst = fc;
			para.m_fcLim = fcLim;
			if (pap.m_rhcPage & 0x10)
			{
				para.m_graphics = true;
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

			if (fcLim >= m_fcMac)
				return;

			fc = fcLim;
		}

		pnPara++;
	}

	if (m_paragraphList.empty())
		throw (libwps::ParseException());

}

unsigned MSWriteParser::numPages()
{
	unsigned numPage = 1;

	RVNGInputStreamPtr input = getInput();

	std::vector<MSWriteParserInternal::Paragraph>::iterator paps;

	for (paps = m_paragraphList.begin(); paps != m_paragraphList.end(); paps++)
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

void MSWriteParser::readCHP()
{
	RVNGInputStreamPtr input = getInput();
	int page = (m_fcMac + 127) / 128;
	unsigned fcLim, fc = 0x80;

	for (;;)
	{
		long const pageBegin=long(page * 0x80);
		input->seek(pageBegin + 0x7f, librevenge::RVNG_SEEK_SET);
		uint8_t cfod = libwps::readU8(input);

		for (unsigned fod = 0; fod < cfod; ++fod)
		{
			if (!checkFilePosition(uint32_t(pageBegin) + fod*6+8))
			{
				WPS_DEBUG_MSG(("MSWriteParser::readCHP: can not find the chp position for %d\n", int(fod)));
				break;
			}
			input->seek(pageBegin + int(fod) * 6 + 4, librevenge::RVNG_SEEK_SET);
			fcLim = libwps::readU32(input);
			uint16_t bfProp = libwps::readU16(input);
			struct MSWriteParserInternal::CHP chp;

			memset(&chp, 0, sizeof(chp));

			chp.m_hps = 24;

			if (bfProp < 0x7f - 4)
			{
				input->seek(pageBegin + long(bfProp) + 4, librevenge::RVNG_SEEK_SET);
				unsigned cch = libwps::readU8(input);
				// Check length and that it is on the page
				if (cch <= sizeof(chp) && (bfProp + cch + 4) < 0x80)
				{
					unsigned long read_bytes;
					const unsigned char *p = input->read(cch, read_bytes);
					if (read_bytes != cch)
						throw (libwps::ParseException());

					memcpy(&chp, p, cch);
				}
				else
				{
					WPS_DEBUG_MSG(("chp entry %d on page %d invalid\n", fod, page));
				}
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

			font.m_fcFirst = fc;
			font.m_fcLim = fcLim;

			m_fontList.push_back(font);

			if (fcLim >= m_fcMac)
				return;

			fc = fcLim;
		}
		page++;
	}

	if (m_fontList.empty())
		throw (libwps::ParseException());
}

void MSWriteParser::findZones()
{
	MSWriteParserInternal::Paragraph::Location location;

	location = m_paragraphList[0].m_Location;
	if (location == MSWriteParserInternal::Paragraph::NORMAL)
	{
		m_Main.setBegin(m_paragraphList[0].m_fcFirst);
		m_Main.setEnd(m_fcMac);
		m_Main.setType("TEXT");
		return;
	}

	unsigned first = 0, i;

	for (i = 1; i < m_paragraphList.size(); i++)
	{
		MSWriteParserInternal::Paragraph &p = m_paragraphList[i];

		if (p.m_Location != location)
		{
			WPSEntry &e = m_Header;
			if (location == MSWriteParserInternal::Paragraph::HEADER)
			{
				m_HeaderPage1 = m_paragraphList[first].m_firstpage;
			}
			else
			{
				m_FooterPage1 = m_paragraphList[first].m_firstpage;
				e = m_Footer;
			}

			e.setBegin(m_paragraphList[first].m_fcFirst);
			e.setEnd(m_paragraphList[i - 1].m_fcLim);
			e.setType("TEXT");

			location = p.m_Location;
			first = i;
		}

		if (location == MSWriteParserInternal::Paragraph::NORMAL)
		{
			m_Main.setBegin(p.m_fcFirst);
			m_Main.setEnd(m_fcMac);
			m_Main.setType("TEXT");
			break;
		}
	}
}

shared_ptr<WPSContentListener> MSWriteParser::createListener(librevenge::RVNGTextInterface *interface)
{
	std::vector<WPSPageSpan> pageList;
	WPSPageSpan page1(m_span), ps(m_span);

	if (m_Header.valid())
	{
		WPSSubDocumentPtr subdoc(new MSWriteParserInternal::SubDocument
		                         (getInput(), *this, m_Header));
		ps.setHeaderFooter(WPSPageSpan::HEADER, WPSPageSpan::ALL, subdoc);

		if (m_HeaderPage1)
			page1.setHeaderFooter(WPSPageSpan::HEADER, WPSPageSpan::ALL, subdoc);
	}

	if (m_Footer.valid())
	{
		WPSSubDocumentPtr subdoc(new MSWriteParserInternal::SubDocument
		                         (getInput(), *this, m_Footer));
		ps.setHeaderFooter(WPSPageSpan::FOOTER, WPSPageSpan::ALL, subdoc);

		if (m_FooterPage1)
			page1.setHeaderFooter(WPSPageSpan::FOOTER, WPSPageSpan::ALL, subdoc);
	}

	pageList.push_back(page1);
	unsigned pages = numPages();
	for (unsigned i = 1; i < pages; i++) pageList.push_back(ps);

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

	while (fc < uint32_t(e.end()))
	{
		while (fc >= paps->m_fcLim)
		{
			paps++;
			if (paps == m_paragraphList.end())
				throw (libwps::ParseException());
		}

		input->seek(fc, librevenge::RVNG_SEEK_SET);

		if (paps->m_graphics)
		{
			size_t size = paps->m_fcLim - fc;
			unsigned long read_bytes;
			const uint8_t *p = input->read(size, read_bytes);
			if (read_bytes != size)
				throw (libwps::ParseException());

			WPSPosition pos;
			WPSPosition::XPos align;

			switch (paps->m_justify)
			{
			case libwps::JustificationLeft:
				align = WPSPosition::XLeft;
				break;
			case libwps::JustificationRight:
				align = WPSPosition::XRight;
				break;
			case libwps::JustificationCenter:
			case libwps::JustificationFull:
			case libwps::JustificationFullAllLines: // CHECKME
			default:
				align = WPSPosition::XCenter;
				break;
			}
			pos.setRelativePosition(WPSPosition::Paragraph, align);

			processObject(pos, p, size);
			fc = paps->m_fcLim;
			continue;
		}

		m_listener->setParagraph(*paps);

		while (fc >= chps->m_fcLim)
		{
			chps++;
			if (chps == m_fontList.end())
				throw (libwps::ParseException());
		}

		m_listener->setFont(*chps);

		while (fc < chps->m_fcLim && fc < paps->m_fcLim && fc < m_fcMac)
		{
			uint8_t ch = libwps::readU8(input);
			if (chps->m_special)
			{
				if (ch == 1)
					m_listener->insertField(WPSContentListener::PageNumber);
			}
			else
			{
				if (ch >= ' ')
					m_listener->insertUnicode((uint32_t)libwps_tools_win::Font::unicode(ch, m_fontType));
				else switch (ch)
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
					default:
						// ignore
						break;
					}
			}
			fc++;
		}
	}
}

unsigned MSWriteParser::getObjectOffset(const uint8_t *data, size_t size)
{
	if (size >= 40)
		return WPS_LE_GET_GUINT16(data + MSWriteParserInternal::PIC_W_DXAOFFSET);

	return 0;
}

void MSWriteParser::processObject(WPSPosition &pos, const uint8_t *data, size_t size)
{
	int mm;

	if (size < 40)
	{
		WPS_DEBUG_MSG(("object too small, ignoring\n"));
		return;
	}

	// FIXME: dxaOffset not honoured
	mm = WPS_LE_GET_GUINT16(data + MSWriteParserInternal::PIC_W_MM);
	if (mm == 0x88)   // Windows metafile (.wmf)
	{
		uint32_t cbsize = WPS_LE_GET_GUINT32(data + MSWriteParserInternal::PIC_D_CBSIZE);

		processWMF(pos, data + 40, data, cbsize);
	}
	else if (mm == 0xe3)     // this is a picture
	{
		uint32_t cbsize = WPS_LE_GET_GUINT32(data + MSWriteParserInternal::PIC_D_CBSIZE);

		processDDB(pos, data + 16, data + 40, cbsize);
	}
	else if (mm == 0xe4)     // OLE object
	{
		unsigned ole_id = WPS_LE_GET_GUINT32(data + 40);
		unsigned type = WPS_LE_GET_GUINT32(data + 44);
		unsigned len = WPS_LE_GET_GUINT32(data + 48);
		if (ole_id != 0x501)
		{
			WPS_DEBUG_MSG(("MSWriteParser::processObject: Unknown ole identifier %x\n", ole_id));
			return;
		}
		if (data[51 + len])
		{
			WPS_DEBUG_MSG(("MSWriteParser::processObject:OLE name corrupt\n"));
			return;
		}

		switch (type)
		{
		case 3: // static OLE
			if (size<48 || size!=unsigned(size-48)+48)
			{
				WPS_DEBUG_MSG(("MSWriteParser::processObject:unknown bad size for type %d\n", type));
				return;
			}
			processStaticOLE(pos, data + 48, data, unsigned(size - 48));
			return;
		case 2: // Embedded OLE
			// CHECKME: is it really size-18 or size-48 ?
			if (size!=unsigned(size-18)+18)
			{
				WPS_DEBUG_MSG(("MSWriteParser::processObject:unknown bad size for type %d\n", type));
				return;
			}
			processEmbeddedOLE(pos, data + 48, data, unsigned(size - 18));
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

// CHECKME: capitalize class, put them in a name space, is packed needed...
struct bitmapfileheader
{
	uint8_t type[2];
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t offset;
} __attribute__((packed));

struct bitmapinfoheaderv2
{
	uint32_t size;
	uint16_t width;
	uint16_t height;
	uint16_t planes;
	uint16_t bits_pixel;
} __attribute__((packed));

struct bitmapinfoheaderv3
{
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint16_t planes;
	uint16_t bits_pixel;
	uint32_t compresson;
	uint32_t bitmap_size;
	uint32_t horz_res;
	uint32_t vert_res;
	uint32_t colors_used;
	uint32_t colors_important;
} __attribute__((packed));

struct palette
{
	uint8_t r, g, b;
} __attribute__((packed));

static const palette pal1[2] = { { 0,0,0 }, { 255,255,255 } };
static const palette pal4[16] =
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
static const palette pal8[256] =
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

void MSWriteParser::processDDB(WPSPosition &pos, const uint8_t *bm, const uint8_t *data, unsigned size)
{
	/* A ddb image has no color palette; it is BMP Version 1, but this format
	   is not widely supported. Create BMP version 2 file with a color palette.
	 */
	bitmapfileheader file;
	bitmapinfoheaderv2 info;

	unsigned width = WPS_LE_GET_GUINT16(bm + 2);
	unsigned height = WPS_LE_GET_GUINT16(bm + 4);
	unsigned byte_width = WPS_LE_GET_GUINT16(bm + 6);
	unsigned planes = bm[8];
	unsigned bits_pixel = bm[9];

	if (size < byte_width * height)
	{
		WPS_DEBUG_MSG(("DDB image data missing, skipping\n"));
		return;
	}

	if (planes != 1)
	{
		WPS_DEBUG_MSG(("DDB image has planes %d, must be 1\n", planes));
		return;
	}

	WPS_DEBUG_MSG(("DDB size %ux%ux%u, byte width %u\n", width, height,
	               bits_pixel, byte_width));

	unsigned colors = 0;
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
	}

	WPS_LE_PUT_GUINT32(&info.size, 12);
	WPS_LE_PUT_GUINT16(&info.width, width);
	WPS_LE_PUT_GUINT16(&info.height, height);
	WPS_LE_PUT_GUINT16(&info.planes, planes);
	WPS_LE_PUT_GUINT16(&info.bits_pixel, bits_pixel);

	unsigned offset = sizeof(file) + sizeof(info) + colors * sizeof(palette);

	file.type[0] = 'B';
	file.type[1] = 'M';
	file.reserved1 = 0;
	file.reserved2 = 0;
	WPS_LE_PUT_GUINT32(&file.offset, offset);
	WPS_LE_PUT_GUINT32(&file.size, offset + height * byte_width);

	librevenge::RVNGBinaryData bmpdata;

	bmpdata.append((const unsigned char *)&file, sizeof(file));
	bmpdata.append((const unsigned char *)&info, sizeof(info));
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

	// Scanlines must be on 4-byte boundary, but some DDB images have
	// byte_width which is not on a 4-byte boundary. Also mirror the image
	for (unsigned i=0; i<height; i++)
	{
		bmpdata.append(data + byte_width * (height - i - 1), byte_width);
		if (byte_width % 4)
			bmpdata.append(data, 4 - byte_width % 4);
	}

	pos.setUnit(librevenge::RVNG_POINT);
	pos.setSize(Vec2f(float(width), float(height)));

	m_listener->insertPicture(pos, bmpdata, "image/bmp");
}

void MSWriteParser::processDIB(WPSPosition &pos, const uint8_t *data, unsigned size)
{
	if (size < sizeof(bitmapinfoheaderv3))
	{
		WPS_DEBUG_MSG(("DIB image missing data, skipping\n"));
		return;
	}

	bitmapinfoheaderv3 *info = (bitmapinfoheaderv3 *)data;

	if (WPS_LE_GET_GUINT32(&info->size) != sizeof(bitmapinfoheaderv3))
	{
		WPS_DEBUG_MSG(("DIB header incorrect size, skipping\n"));
		return;
	}

	unsigned width = WPS_LE_GET_GUINT32(&info->width);
	unsigned height = WPS_LE_GET_GUINT32(&info->height);
	unsigned bits_pixel = WPS_LE_GET_GUINT16(&info->bits_pixel);
	WPS_DEBUG_MSG(("DIB image %ux%ux%u\n", width, height, bits_pixel));

	unsigned colors = 0;
	if (info->bits_pixel && bits_pixel < 16)
	{
		colors = info->colors_used ?
		         WPS_LE_GET_GUINT32(&info->colors_used) : 1 << bits_pixel;
	}
	bitmapfileheader file;
	file.type[0] = 'B';
	file.type[1] = 'M';
	file.reserved1 = 0;
	file.reserved2 = 0;
	unsigned offset = sizeof(file) + sizeof(*info) + 4 * colors;
	WPS_LE_PUT_GUINT32(&file.offset, offset);
	WPS_LE_PUT_GUINT32(&file.size, size + sizeof(file));

	librevenge::RVNGBinaryData bmpdata;

	bmpdata.append((const unsigned char *)&file, sizeof(file));
	bmpdata.append(data, size);

	pos.setUnit(librevenge::RVNG_POINT);
	pos.setSize(Vec2f(float(width), float(height)));

	m_listener->insertPicture(pos, bmpdata, "image/bmp");
}

void MSWriteParser::processEmbeddedOLE(WPSPosition &pos, const uint8_t *data, const uint8_t * /*obj*/, unsigned size)
{
	const char *strings[3];
	unsigned i, n, offset = 0;

	for (i=0; i<3; i++)
	{
		unsigned strlen = WPS_LE_GET_GUINT32(data + offset);
		if (offset + 4 + strlen >= size)
		{
			WPS_DEBUG_MSG(("Embedded OLE corrupt, skipping\n"));
			return;
		}
		if (strlen)
		{
			strings[i] = (const char *)(data + offset + 4);
			for (n=0; n<strlen - 1; n++)
			{
				if (strings[i][n] < 0x20 || strings[i][n] >= 127)
				{
					WPS_DEBUG_MSG(("Embedded OLE corrupt, skipping\n"));
					return;
				}
				if (strings[i][strlen - 1])
				{
					WPS_DEBUG_MSG(("Embedded OLE corrupt, skipping\n"));
					return;
				}
			}
		}
		else
			strings[i] = NULL;

		offset += strlen + 4;
	}

	WPS_DEBUG_MSG(("Embedded OLE object %s,filename=%s,parameter=%s\n",
	               strings[0] ? strings[0] : "unknown",
	               strings[1] ? strings[1] : "none",
	               strings[2] ? strings[2] : "none"));

	unsigned len = WPS_LE_GET_GUINT32(data + offset);
	offset += 4;

	librevenge::RVNGBinaryData oledata;
	oledata.append(data + offset, len);

	std::string mimetype = "object/ole";

	if (!strcmp(strings[0], "PBrush") || !strcmp(strings[0], "Paint.Picture"))
	{
		bitmapinfoheaderv3 *info = (bitmapinfoheaderv3 *)(data + offset + sizeof(bitmapfileheader));
		if (WPS_LE_GET_GUINT32(&info->size) == sizeof(bitmapinfoheaderv3))
		{
			pos.setUnit(librevenge::RVNG_POINT);
			pos.setSize(Vec2f(float(WPS_LE_GET_GUINT32(&info->width)),
			                  float(WPS_LE_GET_GUINT32(&info->height))));
		}
		mimetype = "image/bmp";
	}
	else if (!strcmp(strings[0], "SoundRec"))
	{
		mimetype = "audio/wav";
	}

	offset += len;

	m_listener->insertPicture(pos, oledata, mimetype);
	// FIXME: Static OLE object / replacement object might follow
}

void MSWriteParser::processStaticOLE(WPSPosition &pos, const uint8_t *data, const uint8_t *obj, unsigned size)
{
	const char *objtype = (const char *)(data + 4);

	if (strcmp("BITMAP", objtype) == 0)
	{
		unsigned len = WPS_LE_GET_GUINT32(data + 4 + 15);
		if (len > size - 4 - 19)
		{
			WPS_DEBUG_MSG(("Static OLE DDB bitmap too short, skipping\n"));
			return;
		}
		processDDB(pos, data + 4 + 19, data + 19 + 14, len - 10);
	}
	else if (strcmp("DIB", objtype) == 0)
	{
		// We have an DIB without the file header.
		unsigned len = WPS_LE_GET_GUINT32(data + 4 + 12);
		if (len > size - 4 - 16)
		{
			WPS_DEBUG_MSG(("Static OLE DIB bitmap too short, skipping\n"));
			return;
		}
		processDIB(pos, data + 4 + 16, len);
	}
	else if (strcmp("METAFILEPICT", objtype) == 0)
	{
		unsigned len = WPS_LE_GET_GUINT32(data + 4 + 21);
		if (len > size - 4 - 25)
		{
			WPS_DEBUG_MSG(("Static OLE metafile too short, skipping\n"));
			return;
		}
		processWMF(pos, data + 4 + 25, obj, len);
	}
	else
	{
		WPS_DEBUG_MSG(("Unknown static OLE object of type %s\n", objtype));
	}
}

void MSWriteParser::processWMF(WPSPosition &pos, const uint8_t *data, const uint8_t *obj, unsigned size)
{
	WPS_DEBUG_MSG(("Windows metafile (wmf)\n"));
	uint16_t picw = WPS_LE_GET_GUINT16(obj + MSWriteParserInternal::PIC_W_XEXT);
	uint16_t pich = WPS_LE_GET_GUINT16(obj + MSWriteParserInternal::PIC_W_YEXT);
	uint16_t mx = WPS_LE_GET_GUINT16(obj + MSWriteParserInternal::PIC_W_MX);
	uint16_t my = WPS_LE_GET_GUINT16(obj + MSWriteParserInternal::PIC_W_MY);

	if (picw) picw = picw * 9 / 32;
	else picw = 4320;
	if (pich) pich = pich * 9 / 32;
	else pich = 4320;

	pos.setUnit(librevenge::RVNG_TWIP);
	pos.setSize(Vec2f(float(picw), float(pich)));

	librevenge::RVNGPropertyList frame;
	frame.insert("style:relative-width", mx / 10.0, librevenge::RVNG_PERCENT);
	frame.insert("style:relative-height", my / 10.0, librevenge::RVNG_PERCENT);

	librevenge::RVNGBinaryData wmfdata(data + 8, size - 8);
	m_listener->insertPicture(pos, wmfdata, "application/x-wmf", frame);
}

void MSWriteParser::parse(librevenge::RVNGTextInterface *document)
{
	readFIB();
	readFFNTB();
	readSECT();
	readPAP();
	readCHP();
	findZones();

	m_listener = createListener(document);
	if (!m_listener)
	{
		WPS_DEBUG_MSG(("MSWriteParser::parse: can not create the listener\n"));

		throw (libwps::ParseException());
	}

	m_listener->startDocument();
	if (m_Main.valid())
	{
		readText(m_Main);
	}
	m_listener->endDocument();
	m_listener.reset();
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
