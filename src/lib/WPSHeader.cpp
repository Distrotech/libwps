/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002-2004 Marc Maurer (uwog@uwog.net)
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

#include <string.h>

#include "libwps_internal.h"
#include "WPSOLEStream.h"

#include "WPSHeader.h"

WPSHeader::WPSHeader(WPXInputStreamPtr &input, shared_ptr<libwps::Storage> &storage, uint8_t majorVersion) :
	m_input(input), m_oleStorage(storage),
	m_majorVersion(majorVersion)
{
}

WPSHeader::~WPSHeader()
{
}


/**
 * So far, we have identified three categories of Works documents.
 *
 * Works documents versions 3 and later use a MS OLE container, so we detect
 * their type by checking for OLE stream names.  Works version 2 is like
 * Works 3 without OLE, so those two types use the same parser.
 *
 */
WPSHeader *WPSHeader::constructHeader(WPXInputStreamPtr &input)
{
	WPS_DEBUG_MSG(("WPSHeader::constructHeader()\n"));

	shared_ptr<libwps::Storage> oleStorage(new libwps::Storage(input));
	if (oleStorage && !oleStorage->isOLEStream())
		oleStorage.reset();
	if (!oleStorage)
	{
		input->seek(0, WPX_SEEK_SET);
		if (libwps::readU8(input.get()) < 6
		        && 0xFE == libwps::readU8(input.get()))
		{
			WPS_DEBUG_MSG(("Microsoft Works v2 format detected\n"));
			return new WPSHeader(input, oleStorage, 2);
		}
		return 0;
	}

	WPXInputStreamPtr document_mn0(oleStorage->getDocumentOLEStream("MN0"));
	if (document_mn0)
	{
		// can be a mac or a pc document
		// each must contains a MM Ole which begins by 0x444e: Mac or 0x4e44: PC
		WPXInputStreamPtr document_mm(oleStorage->getDocumentOLEStream("MM"));
		if (document_mm && libwps::readU16(document_mm) == 0x4e44)
		{
			WPS_DEBUG_MSG(("Microsoft Works Mac v4 format detected\n"));
			return 0;
		}
		WPS_DEBUG_MSG(("Microsoft Works v4 format detected\n"));
		return new WPSHeader(document_mn0, oleStorage, 4);
	}

	WPXInputStreamPtr document_contents(oleStorage->getDocumentOLEStream("CONTENTS"));
	if (document_contents)
	{
		/* check the Works 2000/7/8 format magic */
		document_contents->seek(0, WPX_SEEK_SET);

		char fileMagic[8];
		for (int i=0; i<7 && !document_contents->atEOS(); i++)
			fileMagic[i] = char(libwps::readU8(document_contents.get()));
		fileMagic[7] = '\0';

		/* Works 7/8 */
		if (0 == strcmp(fileMagic, "CHNKWKS"))
		{
			WPS_DEBUG_MSG(("Microsoft Works v8 (maybe 7) format detected\n"));
			return new WPSHeader(document_contents, oleStorage, 8);
		}

		/* Works 2000 */
		if (0 == strcmp(fileMagic, "CHNKINK"))
		{
			return new WPSHeader(document_contents, oleStorage, 5);
		}
	}

	return NULL;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
