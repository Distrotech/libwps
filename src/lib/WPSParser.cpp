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

#include "libwps_internal.h"

#include "WPSEntry.h"
#include "WPSHeader.h"

#include "WPSParser.h"

WPSParser::WPSParser(WPXInputStreamPtr &input, WPSHeaderPtr &header) :
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
