/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 */

#include "WPSStream.h"

WPSStream::WPSStream(RVNGInputStreamPtr input, libwps::DebugFile &ascii) : m_input(input), m_ascii(ascii), m_eof(-1)
{
	if (!input || input->seek(0, librevenge::RVNG_SEEK_END)!=0)
		return;
	m_eof=input->tell();
	input->seek(0, librevenge::RVNG_SEEK_SET);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
