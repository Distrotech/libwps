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

namespace libwpsOLE
{
class Storage;
}

class WPSHeader
{
public:
	WPSHeader(RVNGInputStreamPtr &input, shared_ptr<libwpsOLE::Storage> &ole,
	          uint8_t majorVersion);
	virtual ~WPSHeader();

	static WPSHeader *constructHeader(RVNGInputStreamPtr &input);

	RVNGInputStreamPtr &getInput()
	{
		return m_input;
	}

	shared_ptr<libwpsOLE::Storage> &getOLEStorage()
	{
		return m_oleStorage;
	}

	uint8_t getMajorVersion() const
	{
		return m_majorVersion;
	}

private:
	WPSHeader(const WPSHeader &);
	WPSHeader &operator=(const WPSHeader &);
	RVNGInputStreamPtr m_input;
	shared_ptr<libwpsOLE::Storage> m_oleStorage;
	uint8_t m_majorVersion;
};

#endif /* WPSHEADER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
