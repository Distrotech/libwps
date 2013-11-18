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

#ifndef WKSSUBDOCUMENT_H
#define WKSSUBDOCUMENT_H

#include "libwps_internal.h"

class WKSContentListener;
class WKSParser;

/** Basic class used to store a spreadsheet sub document */
class WKSSubDocument
{
public:
	/// constructor
	WKSSubDocument(RVNGInputStreamPtr &input, WKSParser *parser, int id=0);
	/// destructor
	virtual ~WKSSubDocument();

	/// returns the input
	RVNGInputStreamPtr &getInput()
	{
		return m_input;
	}
	/// returns the parser
	WKSParser *parser() const
	{
		return m_parser;
	}
	/// get the identificator
	int id() const
	{
		return m_id;
	}
	/// set the identificator
	void setId(int i)
	{
		m_id = i;
	}

	/// an operator =
	virtual bool operator==(shared_ptr<WKSSubDocument> const &doc) const;
	bool operator!=(shared_ptr<WKSSubDocument> const &doc) const
	{
		return !operator==(doc);
	}

	/** virtual parse function
	 *
	 * this function is called to parse the subdocument */
	virtual void parse(shared_ptr<WKSContentListener> &listener, libwps::SubDocumentType subDocumentType) = 0;

protected:
	RVNGInputStreamPtr m_input;
	WKSParser *m_parser;
	int m_id;
private:
	WKSSubDocument(const WKSSubDocument &);
	WKSSubDocument &operator=(const WKSSubDocument &);

};
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
