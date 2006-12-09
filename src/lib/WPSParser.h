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

/*
 * This file is in sync with CVS
 * /libwpd2/src/lib/WPXParser.h 1.22
 */


#ifndef WPSPARSER_H
#define WPSPARSER_H

#include "WPSHeader.h"

class WPXHLListenerImpl;

class WPSParser
{
public:
	WPSParser(WPSInputStream * input, WPSHeader *header);
	virtual ~WPSParser();

	virtual void parse(WPXHLListenerImpl *listenerImpl) = 0;

protected:
	WPSHeader * getHeader() { return m_header; }
	WPSInputStream * getInput() { return m_input; }
	
private:
	WPSInputStream * m_input;

	WPSHeader * m_header;
};

#endif /* WPSPARSER_H */
