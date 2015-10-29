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
#include "WPSPageSpan.h"

namespace MSWriteParserInternal
{
class SubDocument;
struct Paragraph;
struct Font;
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
	void parse(librevenge::RVNGTextInterface *const documentInterface);

private:
	MSWriteParser(const MSWriteParser &);
	MSWriteParser &operator=(const MSWriteParser &);

	shared_ptr<WPSContentListener> createListener(librevenge::RVNGTextInterface *interface);
	void readStructures();
	void readFIB();
	void readFFNTB();
	void readSECT();
	void readFOD(unsigned page, void (MSWriteParser::*parseFOD)(uint32_t fcFirst, uint32_t fcLim, unsigned size), unsigned maxSize);
	void readPAP(uint32_t fcFirst, uint32_t fcLim, unsigned cch);
	void readCHP(uint32_t fcFirst, uint32_t fcLim, unsigned cch);
	void readText(WPSEntry e);
	int numPages();
	void processObject(WPSPosition &pos, unsigned long lastPos);
	bool processDDB(librevenge::RVNGBinaryData &bmpdata, WPSPosition &pos, unsigned width, unsigned height, unsigned byte_width, unsigned planes, unsigned bits_pixel, unsigned size);
	bool processDIB(librevenge::RVNGBinaryData &bmpdata, unsigned size);
	bool processWMF(librevenge::RVNGBinaryData &wmfdata, unsigned size);
	void processEmbeddedOLE(WPSPosition &pos, unsigned long lastPos);
	bool processStaticOLE(librevenge::RVNGBinaryData &, std::string &mimetype, WPSPosition &pos, unsigned long lastPos);
	bool readString(std::string &res, unsigned long lastPos);
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
	std::vector<librevenge::RVNGString> m_fonts;
	WPSPageSpan m_pageSpan;
	libwps_tools_win::Font::Type m_fontType;

	shared_ptr<WPSContentListener> m_listener; /* the listener (if set)*/

	WPSEntry m_Main;
};

#endif /* MS_WRITE_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
