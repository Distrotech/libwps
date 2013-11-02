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
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
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

#include "libwps_internal.h"

#include "WPSEntry.h"
#include "WPSHeader.h"

#include "WPSParser.h"

WPSParser::WPSParser(RVNGInputStreamPtr &input, WPSHeaderPtr &header) :
	m_input(input), m_header(header), m_version(0),
	m_nameMultiMap(), m_asciiFile()
{
	if (header)
		m_version = header->getMajorVersion();
}

WPSParser::~WPSParser()
{
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
