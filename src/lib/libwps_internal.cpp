/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2002, 2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002, 2004 Marc Maurer (uwog@uwog.net)
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
#include <string>
#include <stdlib.h>

namespace libwps
{
uint8_t readU8(WPXInputStream *input)
{
	unsigned long numBytesRead;
	unsigned char const *p = input->read(sizeof(uint8_t), numBytesRead);

	if (!p || numBytesRead != sizeof(uint8_t))
		throw FileException();

	return *(uint8_t const *)(p);
}

int8_t read8(WPXInputStream *input)
{
	return (int8_t) readU8(input);
}

uint16_t readU16(WPXInputStream *input)
{
	unsigned short p0 = (unsigned short)readU8(input);
	unsigned short p1 = (unsigned short)readU8(input);
	return p0|(p1<<8);
}

int16_t read16(WPXInputStream *input)
{
	return (int16_t) readU16(input);
}

uint32_t readU32(WPXInputStream *input)
{
	uint8_t p0 = readU8(input);
	uint8_t p1 = readU8(input);
	uint8_t p2 = readU8(input);
	uint8_t p3 = readU8(input);
	return (uint32_t) ((p0<<0)|(p1<<8)|(p2<<16)|(p3<<24));
}

int32_t read32(WPXInputStream *input)
{
	return (int32_t) readU32(input);
}

std::string numberingTypeToString(NumberingType type)
{
	switch (type)
	{
	case ARABIC:
		return "1";
	case LOWERCASE:
		return "a";
	case UPPERCASE:
		return "A";
	case LOWERCASE_ROMAN:
		return "i";
	case UPPERCASE_ROMAN:
		return "I";
	default:
		break;
	}
	WPS_DEBUG_MSG(("libwps::numberingTypeToString: must not be called with type %d\n", int(type)));
	return "1";
}

}

void WPSTabStop::addTo(WPXPropertyListVector &propList, double decalX)
{
	WPXPropertyList tab;

	// type
	switch (m_alignment)
	{
	case RIGHT:
		tab.insert("style:type", "right");
		break;
	case CENTER:
		tab.insert("style:type", "center");
		break;
	case DECIMAL:
		tab.insert("style:type", "char");
		tab.insert("style:char", "."); // Assume a decimal point for now
		break;
	default:  // Left alignment is the default and BAR is not handled in OOo
		break;
	}

	// leader character
	if (m_leaderCharacter != 0x0000)
	{
		WPXString sLeader;
		sLeader.sprintf("%c", m_leaderCharacter);
		tab.insert("style:leader-text", sLeader);
		tab.insert("style:leader-style", "solid");
	}

	// position
	double position = m_position+decalX;
	if (position < 0.00005f && position > -0.00005f)
		position = 0.0;
	tab.insert("style:position", position);

	propList.append(tab);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
