/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002-2003 Marc Maurer (uwog@uwog.net)
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
 * /libwpd2/src/lib/WPXHeader.cpp 1.17
 */

#ifndef WPSHEADER_H
#define WPSHEADER_H

#include "libwps_internal.h"
#include <libwpd-stream/libwpd-stream.h>

class WPSHeader
{
public:
	WPSHeader(WPXInputStreamPtr &input, uint8_t majorVersion);
	virtual ~WPSHeader();

	static WPSHeader *constructHeader(WPXInputStreamPtr &input);

	WPXInputStreamPtr &getInput()
	{
		return m_input;
	}

	uint8_t getMajorVersion() const
	{
		return m_majorVersion;
	}

private:
	WPSHeader(const WPSHeader &);
	WPSHeader &operator=(const WPSHeader &);
	WPXInputStreamPtr m_input;
	uint8_t m_majorVersion;
};

typedef shared_ptr<WPSHeader> WPSHeaderPtr;

#endif /* WPSHEADER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
