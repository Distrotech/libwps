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
 *
 * For further information visit http://libwps.sourceforge.net
 */

#ifndef WPSTEXTSUBDOCUMENT_H
#define WPSTEXTSUBDOCUMENT_H

#include "libwps_internal.h"

#include "WPSSubDocument.h"

class WPSContentListener;
class WPSParser;

/** Basic class used to store a sub document */
class WPSTextSubDocument : public WPSSubDocument
{
public:
	/// constructor
	WPSTextSubDocument(RVNGInputStreamPtr &input, WPSParser *parser, int id=0);
	/// destructor
	virtual ~WPSTextSubDocument();

	/// returns the input
	RVNGInputStreamPtr &getInput()
	{
		return m_input;
	}
	/// returns the parser
	WPSParser *parser() const
	{
		return m_parser;
	}
	/// an operator =
	virtual bool operator==(shared_ptr<WPSTextSubDocument> const &doc) const;

	/** virtual parse function
	 *
	 * this function is called to parse the subdocument */
	virtual void parse(shared_ptr<WPSContentListener> &listener, libwps::SubDocumentType subDocumentType) = 0;

protected:
	WPSParser *m_parser;
private:
	explicit WPSTextSubDocument(const WPSTextSubDocument &);
	WPSTextSubDocument &operator=(const WPSTextSubDocument &);
};
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */

