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
 * Copyright (C) 2002-2003 Marc Maurer (uwog@uwog.net)
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

#ifndef WPSHEADER_H
#define WPSHEADER_H

#include "libwps_internal.h"
#include <librevenge-stream/librevenge-stream.h>

#include "libwps/libwps.h"

class WPSHeader
{
public:
	WPSHeader(RVNGInputStreamPtr &input, RVNGInputStreamPtr &fileInput, uint8_t majorVersion, libwps::WPSKind kind=libwps::WPS_TEXT, libwps::WPSCreator creator=libwps::WPS_MSWORKS);
	virtual ~WPSHeader();

	static WPSHeader *constructHeader(RVNGInputStreamPtr &input);

	RVNGInputStreamPtr &getInput()
	{
		return m_input;
	}

	RVNGInputStreamPtr &getFileInput()
	{
		return m_fileInput;
	}

	libwps::WPSCreator getCreator() const
	{
		return m_creator;
	}

	void setCreator(libwps::WPSCreator creator)
	{
		m_creator=creator;
	}

	libwps::WPSKind getKind() const
	{
		return m_kind;
	}

	void setKind(libwps::WPSKind kind)
	{
		m_kind=kind;
	}

	bool getNeedEncoding() const
	{
		return m_needEncodingFlag;
	}

	void setNeedEncoding(bool needEncoding)
	{
		m_needEncodingFlag=needEncoding;
	}

	uint8_t getMajorVersion() const
	{
		return m_majorVersion;
	}

	void setMajorVersion(uint8_t version)
	{
		m_majorVersion=version;
	}

private:
	WPSHeader(const WPSHeader &);
	WPSHeader &operator=(const WPSHeader &);
	RVNGInputStreamPtr m_input;
	RVNGInputStreamPtr m_fileInput;
	uint8_t m_majorVersion;
	libwps::WPSKind m_kind;
	libwps::WPSCreator m_creator;
	//! a flag to know if we need to have the character set encoding
	bool m_needEncodingFlag;
};

#endif /* WPSHEADER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
