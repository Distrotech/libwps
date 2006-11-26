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
#include "WPSStream.h"
#include <ctype.h>

uint8_t readU8(libwps::WPSInputStream *input)
{
        return input->getchar();
}

uint16_t readU16(libwps::WPSInputStream *input, bool bigendian)
{
	unsigned short p0 = (unsigned short)readU8(input);
	unsigned short p1 = (unsigned short)readU8(input);
	return p0|(p1<<8);
}

uint32_t readU32(libwps::WPSInputStream *input, bool bigendian)
{
	uint8_t p0 = readU8(input);
	uint8_t p1 = readU8(input);
	uint8_t p2 = readU8(input);
	uint8_t p3 = readU8(input);
        return (uint32_t) ((p0<<0)|(p1<<8)|(p2<<16)|(p3<<24));
}

_WPSTabStop::_WPSTabStop(float position, WPSTabAlignment alignment, uint16_t leaderCharacter, uint8_t leaderNumSpaces)
	:	m_position(position),
		m_alignment(alignment),
		m_leaderCharacter(leaderCharacter),
		m_leaderNumSpaces(leaderNumSpaces)
{
}

_WPSTabStop::_WPSTabStop()
	:	m_position(0.0f),
		m_alignment(LEFT),
		m_leaderCharacter('\0'),
		m_leaderNumSpaces(0)
{
}

_WPSColumnDefinition::_WPSColumnDefinition()
	:	m_width(0.0f),
		m_leftGutter(0.0f),
		m_rightGutter(0.0f)
{
}

_WPSColumnProperties::_WPSColumnProperties()
	:	m_attributes(0x00000000),
		m_alignment(0x00)
{
}

#ifdef DEBUG
std::string to_bits(std::string s)
{
	std::string r;
	for (unsigned int i = 0; i < s.length(); i++)
	{
		std::bitset<8> b(s[i]);	
		r.append(b.to_string());
		char buf[10];
		sprintf(buf, "(%02u,0x%02x)  ", (uint8_t)s[i],(uint8_t)s[i]);
		r.append(buf);
	}
	return r;
}
#endif


