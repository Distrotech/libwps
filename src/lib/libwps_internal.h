/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
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

#ifndef LIBWPS_INTERNAL_H
#define LIBWPS_INTERNAL_H
#ifdef DEBUG
#include <bitset>
#endif
#include "WPSStream.h"
//#include <stdio.h>
//#include <string>
#include <libwpd/WPXString.h>

/* Various functions/defines that need not/should not be exported externally */

#define DELETEP(m) if (m) { delete m; m = NULL; }
 
#ifdef DEBUG
#define WPS_DEBUG_MSG(M) printf M
#else
#define WPS_DEBUG_MSG(M)
#endif

#define WPS_LE_GET_GUINT16(p)				  \
        (uint16_t)((((uint8_t const *)(p))[0] << 0)  |    \
                  (((uint8_t const *)(p))[1] << 8))
#define WPS_LE_GET_GUINT32(p) \
        (uint32_t)((((uint8_t const *)(p))[0] << 0)  |    \
                  (((uint8_t const *)(p))[1] << 8)  |    \
                  (((uint8_t const *)(p))[2] << 16) |    \
                  (((uint8_t const *)(p))[3] << 24))

// add more of these as needed for byteswapping
// (the 8-bit functions are just there to make things consistent)
uint8_t readU8(libwps::WPSInputStream *input); 
uint16_t readU16(libwps::WPSInputStream *input, bool bigendian=false);
uint32_t readU32(libwps::WPSInputStream *input, bool bigendian=false);

// Various helper structures for the parser..

uint16_t fixedPointToWPUs(const uint32_t fixedPointNumber);

enum WPSNumberingType { ARABIC, LOWERCASE, UPPERCASE, LOWERCASE_ROMAN, UPPERCASE_ROMAN };
enum WPSNoteType { FOOTNOTE, ENDNOTE };
enum WPSHeaderFooterType { HEADER, FOOTER };
enum WPSHeaderFooterInternalType { HEADER_A, HEADER_B, FOOTER_A, FOOTER_B, DUMMY };
enum WPSHeaderFooterOccurence { ODD, EVEN, ALL, NEVER };
enum WPSFormOrientation { PORTRAIT, LANDSCAPE };
enum WPSTabAlignment { LEFT, RIGHT, CENTER, DECIMAL, BAR };
enum WPSVerticalAlignment { TOP, MIDDLE, BOTTOM, FULL };

enum WPSTextColumnType { NEWSPAPER, NEWSPAPER_VERTICAL_BALANCE, PARALLEL, PARALLEL_PROTECT };

// ATTRIBUTE bits
#define WPS_EXTRA_LARGE_BIT 1
#define WPS_VERY_LARGE_BIT 2
#define WPS_LARGE_BIT 4
#define WPS_SMALL_PRINT_BIT 8
#define WPS_FINE_PRINT_BIT 16
#define WPS_SUPERSCRIPT_BIT 32
#define WPS_SUBSCRIPT_BIT 64
#define WPS_OUTLINE_BIT 128
#define WPS_ITALICS_BIT 256
#define WPS_SHADOW_BIT 512
#define WPS_REDLINE_BIT 1024
#define WPS_DOUBLE_UNDERLINE_BIT 2048
#define WPS_BOLD_BIT 4096
#define WPS_STRIKEOUT_BIT 8192
#define WPS_UNDERLINE_BIT 16384
#define WPS_SMALL_CAPS_BIT 32768
#define WPS_BLINK_BIT 65536
#define WPS_REVERSEVIDEO_BIT 131072

// JUSTIFICATION bits
#define WPS_PARAGRAPH_JUSTIFICATION_LEFT 0x00
#define WPS_PARAGRAPH_JUSTIFICATION_FULL 0x01
#define WPS_PARAGRAPH_JUSTIFICATION_CENTER 0x02
#define WPS_PARAGRAPH_JUSTIFICATION_RIGHT 0x03
#define WPS_PARAGRAPH_JUSTIFICATION_FULL_ALL_LINES 0x04
#define WPS_PARAGRAPH_JUSTIFICATION_DECIMAL_ALIGNED 0x05

// BREAK bits
#define WPS_PAGE_BREAK 0x00
#define WPS_SOFT_PAGE_BREAK 0x01
#define WPS_COLUMN_BREAK 0x02

// Generic bits
#define WPS_LEFT 0x00
#define WPS_RIGHT 0x01
#define WPS_CENTER 0x02
#define WPS_TOP 0x03
#define WPS_BOTTOM 0x04

typedef struct _RGBSColor RGBSColor;
struct _RGBSColor
{
	_RGBSColor(uint8_t r, uint8_t g, uint8_t b, uint8_t s);
	_RGBSColor(uint16_t red, uint16_t green, uint16_t blue); // Construct
	// RBBSColor from double precision RGB color used by WP3.x for Mac
	_RGBSColor(); // initializes all values to 0
	uint8_t m_r;
	uint8_t m_g;
 	uint8_t m_b;
	uint8_t m_s;
};

typedef struct _WPSColumnDefinition WPSColumnDefinition;
struct _WPSColumnDefinition
{
	_WPSColumnDefinition(); // initializes all values to 0
	float m_width;
	float m_leftGutter;
	float m_rightGutter;
};

typedef struct _WPSColumnProperties WPSColumnProperties;
struct _WPSColumnProperties
{
	_WPSColumnProperties();
	uint32_t m_attributes;
	uint8_t m_alignment;
};

typedef struct _WPSTabStop WPSTabStop;
struct _WPSTabStop
{
	_WPSTabStop(float position, WPSTabAlignment alignment, uint16_t leaderCharacter, uint8_t leaderNumSpaces);
	_WPSTabStop();
	float m_position;
	WPSTabAlignment m_alignment;
	uint16_t m_leaderCharacter;
	uint8_t m_leaderNumSpaces;
};

// Various exceptions

class VersionException
{
	// needless to say, we could flesh this class out a bit
};

class FileException
{
	// needless to say, we could flesh this class out a bit
};

class ParseException
{
	// needless to say, we could flesh this class out a bit
};

class GenericException
{
	// needless to say, we could flesh this class out a bit
};

// Various functions

std::string to_bits(std::string s);

#endif /* LIBWPS_INTERNAL_H */
