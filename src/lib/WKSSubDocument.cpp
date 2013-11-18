/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2005 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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

#include "WKSSubDocument.h"

WKSSubDocument::WKSSubDocument(RVNGInputStreamPtr &input, WKSParser *p, int i)  : m_input(input), m_parser(p), m_id(i)
{
}

WKSSubDocument::~WKSSubDocument()
{
}

bool WKSSubDocument::operator==(shared_ptr<WKSSubDocument> const &doc) const
{
	if (!doc) return false;
	if (doc.get() == this) return true;
	if (m_input.get() != doc.get()->m_input.get()) return false;
	if (m_parser != doc.get()->m_parser) return false;
	if (m_id != doc.get()->m_id) return false;
	return true;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */

