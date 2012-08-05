/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
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

#ifndef WPSPARSER_H
#define WPSPARSER_H

#include "libwps_internal.h"

class WPSHeader;
typedef shared_ptr<WPSHeader> WPSHeaderPtr;

class WPXDocumentInterface;

class WPSParser
{
public:
	WPSParser(WPXInputStreamPtr &input, WPSHeaderPtr &header);
	virtual ~WPSParser();

	virtual void parse(WPXDocumentInterface *documentInterface) = 0;

protected:
	WPSHeaderPtr &getHeader()
	{
		return m_header;
	}
	WPXInputStreamPtr &getInput()
	{
		return m_input;
	}

private:
	WPSParser(const WPSParser &);
	WPSParser &operator=(const WPSParser &);
	WPXInputStreamPtr m_input;

	WPSHeaderPtr m_header;
};

#endif /* WPSPARSER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
