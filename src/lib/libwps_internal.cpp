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

int8_t read8(libwps::WPSInputStream *input)
{
	char buffer;
	size_t numBytesRead = input->read(sizeof(int8_t), &buffer);

  	if (numBytesRead != sizeof(int8_t))
  		throw FileException();

	return (int8_t)(buffer);
}

uint16_t readU16(libwps::WPSInputStream *input, bool bigendian)
{
	unsigned char buffer[2];
	size_t numBytesRead = input->read(sizeof(uint16_t), (char *)buffer);

	if (numBytesRead != sizeof(uint16_t))
  		throw FileException();

	if (bigendian)
		return WPD_BE_GET_GUINT16(buffer);
	return WPD_LE_GET_GUINT16(buffer);
}

uint32_t readU32(libwps::WPSInputStream *input, bool bigendian)
{
	char buffer[4];
	size_t numBytesRead = input->read(sizeof(uint32_t), (char *) buffer);

	if (numBytesRead != sizeof(uint32_t))
  		throw FileException();

	if (bigendian)
		return WPD_BE_GET_GUINT32(buffer);
	return WPD_LE_GET_GUINT32(buffer);
}

WPXString readPascalString(libwps::WPSInputStream *input)
{
	int pascalStringLength = readU8(input);
	WPXString tmpString;
	for (int i=0; i<pascalStringLength; i++)
		tmpString.append((char)readU8(input));
	return tmpString;
}

WPXString readCString(libwps::WPSInputStream *input)
{
	WPXString tmpString;
	char character;
	while ((character = (char)readU8(input)) != '\0')
		tmpString.append(character);
	return tmpString;
}

// the ascii map appears stupid, but we need the const 16-bit data for now
static const uint16_t asciiMap[] =
{
	  0,   1,   2,   3,   4,   5,   6,   7,
	  8,   9,  10,  11,  12,  13,  14,  15,
	 16,  17,  18,  19,  20,  21,  22,  23,
	 24,  25,  26,  27,  28,  29,  30,  31,
	 32,  33,  34,  35,  36,  37,  38,  39,
	 40,  41,  42,  43,  44,  45,  46,  47,
	 48,  49,  50,  51,  52,  53,  54,  55,
	 56,  57,  58,  59,  60,  61,  62,  63,
	 64,  65,  66,  67,  68,  69,  70,  71,
	 72,  73,  74,  75,  76,  77,  78,  79,
	 80,  81,  82,  83,  84,  85,  86,  87,
	 88,  89,  90,  91,  92,  93,  94,  95,
	 96,  97,  98,  99, 100, 101, 102, 103,
	104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135,
	136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151,
	152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167,
	168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183,
	184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199,
	200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215,
	216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231,
	232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247,
	248, 249, 250, 251, 252, 253, 254, 255,
};

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
	for (int i = 0; i < s.length(); i++)
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


