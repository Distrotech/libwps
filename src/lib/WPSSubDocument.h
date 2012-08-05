/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * For further information visit http://libwps.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef WPSSUBDOCUMENT_H
#define WPSSUBDOCUMENT_H

#include "libwps_internal.h"

class WPSContentListener;
class WPSParser;

/** Basic class used to store a sub document */
class WPSSubDocument
{
public:
	/// constructor
	WPSSubDocument(WPXInputStreamPtr &input, WPSParser *parser, int id=0);
	/// destructor
	virtual ~WPSSubDocument();

	/// returns the input
	WPXInputStreamPtr &getInput()
	{
		return m_input;
	}
	/// returns the parser
	WPSParser *parser() const
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
	virtual bool operator==(shared_ptr<WPSSubDocument> const &doc) const;
	bool operator!=(shared_ptr<WPSSubDocument> const &doc) const
	{
		return !operator==(doc);
	}

	/** virtual parse function
	 *
	 * this function is called to parse the subdocument */
	virtual void parse(shared_ptr<WPSContentListener> &listener, libwps::SubDocumentType subDocumentType) = 0;

protected:
	WPXInputStreamPtr m_input;
	WPSParser *m_parser;
	int m_id;
private:
	WPSSubDocument(const WPSSubDocument &);
	WPSSubDocument &operator=(const WPSSubDocument &);

};

typedef shared_ptr<WPSSubDocument> WPSSubDocumentPtr;
#endif

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */

