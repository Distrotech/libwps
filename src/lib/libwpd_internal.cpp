/* libwpd
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */
#include "libwpd_internal.h"
#include "WPXStream.h"
#include <ctype.h>

uint8_t readU8(WPXInputStream *input)
{
	size_t numBytesRead;
	uint8_t const * p = input->read(sizeof(uint8_t), numBytesRead);
	
  	if (!p || numBytesRead != sizeof(uint8_t))
  		throw FileException();

	return WPD_LE_GET_GUINT8(p);
}

int8_t read8(WPXInputStream *input)
{
	size_t numBytesRead;
	int8_t const * p = (int8_t const *) input->read(sizeof(int8_t), numBytesRead);

  	if (!p || numBytesRead != sizeof(int8_t))
  		throw FileException();

	return (int8_t)*(p);
}

uint16_t readU16(WPXInputStream *input, bool bigendian)
{
	size_t numBytesRead;
	uint16_t const *val = (uint16_t const *)input->read(sizeof(uint16_t), numBytesRead);

	if (!val || numBytesRead != sizeof(uint16_t))
  		throw FileException();

	if (bigendian)
		return WPD_BE_GET_GUINT16(val);
	return WPD_LE_GET_GUINT16(val);
}

uint32_t readU32(WPXInputStream *input, bool bigendian)
{
	size_t numBytesRead;
	uint32_t const *val = (uint32_t const *)input->read(sizeof(uint32_t), numBytesRead);

	if (!val || numBytesRead != sizeof(uint32_t))
  		throw FileException();

	if (bigendian)
		return WPD_BE_GET_GUINT32(val);
	return WPD_LE_GET_GUINT32(val);
}

WPXString readPascalString(WPXInputStream *input)
{
	int pascalStringLength = readU8(input);
	WPXString tmpString;
	for (int i=0; i<pascalStringLength; i++)
		tmpString.append((char)readU8(input));
	return tmpString;
}

WPXString readCString(WPXInputStream *input)
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

#include "WPXFileStructure.h"
#include "libwpd_math.h"

uint16_t fixedPointToWPUs(const uint32_t fixedPointNumber)
{
	int16_t fixedPointNumberIntegerPart = (int16_t)((fixedPointNumber & 0xFFFF0000) >> 16);
	float fixedPointNumberFractionalPart = (float)((double)(fixedPointNumber & 0x0000FFFF)/(double)0xFFFF);
	uint16_t numberWPU = (uint16_t)rint((((float)fixedPointNumberIntegerPart + fixedPointNumberFractionalPart)*50)/3);
	return numberWPU;
}

_RGBSColor::_RGBSColor(uint8_t r, uint8_t g, uint8_t b, uint8_t s)
	:	m_r(r),
		m_g(g),
		m_b(b),
		m_s(s)
{
}

_RGBSColor::_RGBSColor()
	:	m_r(0),
		m_g(0),
		m_b(0),
		m_s(0)
{
}

_RGBSColor::_RGBSColor(uint16_t red, uint16_t green, uint16_t blue)
{
	int minRGB = red;
	if (minRGB > green)
		minRGB = green;
	if (minRGB > blue)
		minRGB = blue;
		
	if (minRGB >= 65535)
	{
		m_r = 255;
		m_g = 255;
		m_b = 255;
		m_s = 100;
	}
	else
	{
		m_r = (uint8_t)rint(255*((double)(red - minRGB))/((double)(65535 - minRGB)));
		m_g = (uint8_t)rint(255*((double)(green - minRGB))/((double)(65535 - minRGB)));
		m_b = (uint8_t)rint(255*((double)(blue - minRGB))/((double)(65535 - minRGB)));
		m_s = (uint8_t)rint(100*((double)(65535 - minRGB))/(double)65535);
	}		
}

_WPXTabStop::_WPXTabStop(float position, WPXTabAlignment alignment, uint16_t leaderCharacter, uint8_t leaderNumSpaces)
	:	m_position(position),
		m_alignment(alignment),
		m_leaderCharacter(leaderCharacter),
		m_leaderNumSpaces(leaderNumSpaces)
{
}

_WPXTabStop::_WPXTabStop()
	:	m_position(0.0f),
		m_alignment(LEFT),
		m_leaderCharacter('\0'),
		m_leaderNumSpaces(0)
{
}

_WPXColumnDefinition::_WPXColumnDefinition()
	:	m_width(0.0f),
		m_leftGutter(0.0f),
		m_rightGutter(0.0f)
{
}

_WPXColumnProperties::_WPXColumnProperties()
	:	m_attributes(0x00000000),
		m_alignment(0x00)
{
}

// HACK: this function is really cheesey
int _extractNumericValueFromRoman(const char romanChar)
{
	switch (romanChar)
	{
	case 'I':
	case 'i':
		return 1;
	case 'V':
	case 'v':
		return 5;
	case 'X':
	case 'x':
		return 10;
	default:
		throw ParseException();
	}
	return 1;
}

// _extractDisplayReferenceNumberFromBuf: given a nuWP6_DEFAULT_FONT_SIZEmber string in UCS2 represented
// as letters, numbers, or roman numerals.. return an integer value representing its number
// HACK: this function is really cheesey
// NOTE: if the input is not valid, the output is unspecified
int _extractDisplayReferenceNumberFromBuf(const WPXString &buf, const WPXNumberingType listType)
{
	if (listType == LOWERCASE_ROMAN || listType == UPPERCASE_ROMAN)
	{
		int currentSum = 0;
		int lastMark = 0;
		WPXString::Iter i(buf);
		for (i.rewind(); i.next();)
		{
			int currentMark = _extractNumericValueFromRoman(*(i()));
			if (lastMark < currentMark) {
				currentSum = currentMark - lastMark;
			}
			else
				currentSum+=currentMark;
			lastMark = currentMark;
		}
		return currentSum;
	}
	else if (listType == LOWERCASE || listType == UPPERCASE)
	{
		// FIXME: what happens to a lettered list that goes past z? ah
		// the sweet mysteries of life
		if (buf.len()==0)
			throw ParseException();
		char c = buf.cstr()[0];
		if (listType==LOWERCASE)
			c = toupper(c);
		return (c - 64);
	}
	else if (listType == ARABIC)
	{
		int currentSum = 0;
		WPXString::Iter i(buf);
		for (i.rewind(); i.next();)
		{
			currentSum *= 10;
			currentSum+=(*(i())-48);
		}
		return currentSum;
	}

	return 1;
}

WPXNumberingType _extractWPXNumberingTypeFromBuf(const WPXString &buf, const WPXNumberingType putativeWPXNumberingType)
{
	WPXString::Iter i(buf);
	for (i.rewind(); i.next();)
	{
		if ((*(i()) == 'I' || *(i()) == 'V' || *(i()) == 'X') &&
		    (putativeWPXNumberingType == LOWERCASE_ROMAN || putativeWPXNumberingType == UPPERCASE_ROMAN))
			return UPPERCASE_ROMAN;
		else if ((*(i()) == 'i' || *(i()) == 'v' || *(i()) == 'x') &&
		    (putativeWPXNumberingType == LOWERCASE_ROMAN || putativeWPXNumberingType == UPPERCASE_ROMAN))
			return LOWERCASE_ROMAN;
		else if (*(i()) >= 'A' && *(i()) <= 'Z')
			return UPPERCASE;
		else if (*(i()) >= 'a' && *(i()) <= 'z')
			return LOWERCASE;
	}

	return ARABIC;
}

WPXString _numberingTypeToString(WPXNumberingType t)
{
	WPXString sListTypeSymbol("1");
	switch (t)
	{
	case ARABIC:
		sListTypeSymbol.sprintf("1");
		break;	
	case LOWERCASE:
		sListTypeSymbol.sprintf("a");
		break;	
	case UPPERCASE:
		sListTypeSymbol.sprintf("A");
		break;	
 	case LOWERCASE_ROMAN:
		sListTypeSymbol.sprintf("i");
		break;	
 	case UPPERCASE_ROMAN:
		sListTypeSymbol.sprintf("I");
		break;
	}

	return sListTypeSymbol;
}

