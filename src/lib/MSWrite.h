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
	void readFIB();
	void readFFNTB();
	void readSECT();
	void readPAP();
	void readCHP();
	void findZones();
	void readText(WPSEntry e);
	unsigned numPages();
	void processObject(WPSPosition &pos, const uint8_t *, size_t);
	void processDDB(WPSPosition &pos, const uint8_t *bm, const uint8_t *data, unsigned size);
	void processDIB(WPSPosition &pos, const uint8_t *data, unsigned size);
	void processWMF(WPSPosition &pos, const uint8_t *data, const uint8_t *obj, unsigned size);
	void processStaticOLE(WPSPosition &pos, const uint8_t *data, const uint8_t *obj, unsigned size);
	void processEmbeddedOLE(WPSPosition &pos, const uint8_t *data, const uint8_t *obj, unsigned size);
	unsigned getObjectOffset(const uint8_t *, size_t);
	shared_ptr<std::string> convert(const uint8_t *text, size_t len);

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

	WPSEntry m_Main, m_Header, m_Footer;
	bool m_HeaderPage1, m_FooterPage1;
};

#endif /* WRITE_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
