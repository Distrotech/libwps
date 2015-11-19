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

#ifndef MS_WRITE_H
#define MS_WRITE_H

#include <vector>

#include <librevenge-stream/librevenge-stream.h>
#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSParser.h"
#include "WPSEntry.h"
#include "WPSFont.h"
#include "WPSPageSpan.h"
#include "WPSParagraph.h"

namespace MSWriteParserInternal
{
class SubDocument;

struct Paragraph :  public WPSParagraph
{
	enum Location { MAIN, HEADER, FOOTER, FOOTNOTE };
	Paragraph() : WPSParagraph(), m_fcFirst(0), m_fcLim(0),
		m_Location(MAIN), m_graphics(false), m_firstpage(false),
		m_skiptab(false), m_headerUseMargin(false), m_interLine(0.0),
		m_HeaderFooterOccurrence(WPSPageSpan::ALL)  { }
	uint32_t m_fcFirst, m_fcLim;
	Location m_Location;
	bool m_graphics, m_firstpage, m_skiptab, m_headerUseMargin;
	double m_interLine;
	WPSPageSpan::HeaderFooterOccurrence m_HeaderFooterOccurrence;
};

struct Font : public WPSFont
{
	Font() : WPSFont(), m_fcFirst(0), m_fcLim(0), m_special(false),
		m_footnote(false), m_annotation(false),
		m_encoding(libwps_tools_win::Font::UNKNOWN) { }
	uint32_t m_fcFirst, m_fcLim;
	bool m_special, m_footnote, m_annotation;
	libwps_tools_win::Font::Type m_encoding;
};

struct Footnote
{
	Footnote() : fcFtn(0), fcRef(0) { }
	uint32_t fcFtn, fcRef;
};

}

/**
 * This class parses Microsoft Write 3.0 and 3.1
 *
 */
class MSWriteParser : public WPSParser
{
	friend class MSWriteParserInternal::SubDocument;
	friend struct MSWriteParserInternal::Paragraph;
	friend struct MSWriteParserInternal::Font;

public:
	MSWriteParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header,
	              libwps_tools_win::Font::Type encoding=libwps_tools_win::Font::WIN3_WEUROPE);


	~MSWriteParser();
	void parse(librevenge::RVNGTextInterface *documentInterface);

private:
	MSWriteParser(const MSWriteParser &);
	MSWriteParser &operator=(const MSWriteParser &);

	shared_ptr<WPSContentListener> createListener(librevenge::RVNGTextInterface *interface);
protected:
	void readStructures();
	virtual libwps_tools_win::Font::Type getFileEncoding(libwps_tools_win::Font::Type hint);
	void readFIB();
	virtual void readFFNTB();
	void readSECT();
	void readFOD(unsigned page, void (MSWriteParser::*parseFOD)(uint32_t fcFirst, uint32_t fcLim, unsigned size));
	virtual void readPAP(uint32_t fcFirst, uint32_t fcLim, unsigned cch);
	virtual void readCHP(uint32_t fcFirst, uint32_t fcLim, unsigned cch);
	virtual void readSUMD();
	virtual void readFNTB();
	void readText(WPSEntry e, MSWriteParserInternal::Paragraph::Location location);
	int numPages();
	void processObject(WPSPosition &pos, unsigned long lastPos);
	bool processDDB(librevenge::RVNGBinaryData &bmpdata, WPSPosition &pos, unsigned width, unsigned height, unsigned byte_width, unsigned planes, unsigned bits_pixel, unsigned size);
	bool processDIB(librevenge::RVNGBinaryData &bmpdata, unsigned size);
	bool processWMF(librevenge::RVNGBinaryData &wmfdata, unsigned size);
	void processEmbeddedOLE(WPSPosition &pos, unsigned long lastPos);
	bool processStaticOLE(librevenge::RVNGBinaryData &, std::string &mimetype, WPSPosition &pos, unsigned long lastPos);
	bool readString(std::string &res, unsigned long lastPos);
	virtual void insertSpecial(uint8_t val, uint32_t fc, MSWriteParserInternal::Paragraph::Location location);
	virtual void insertControl(uint8_t val);
	void insertNote(bool annotation, uint32_t fcPos, librevenge::RVNGString &label);
	unsigned insertString(const unsigned char *str, unsigned size, libwps_tools_win::Font::Type type);

	//! check if the file position is correct or not
	bool checkFilePosition(uint32_t pos) const
	{
		return pos<=m_fileLength;
	}
	// State
	//! the last file position
	uint32_t m_fileLength;
	uint32_t m_fcMac;

	std::vector<MSWriteParserInternal::Paragraph> m_paragraphList;
	std::vector<MSWriteParserInternal::Font> m_fontList;
	std::vector<MSWriteParserInternal::Footnote> m_footnotes;
	std::vector<librevenge::RVNGString> m_fonts;
	WPSPageSpan m_pageSpan;
	libwps_tools_win::Font::Type m_fontType;

	shared_ptr<WPSContentListener> m_listener; /* the listener (if set)*/

	WPSEntry m_Main;
	librevenge::RVNGPropertyList m_metaData;
};

#endif /* MS_WRITE_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
