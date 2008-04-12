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
#include <libwpd-stream/libwpd-stream.h>
#include <libwpd/libwpd.h>
#include <string>

#ifdef _MSC_VER
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
#else /* _MSC_VER */
#include <inttypes.h>
#endif /* _MSC_VER */

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

uint8_t readU8(WPXInputStream *input); 
uint16_t readU16(WPXInputStream *input);
uint32_t readU32(WPXInputStream *input);

// Various helper structures for the parser..

enum WPSHeaderFooterType { HEADER, FOOTER };
enum WPSHeaderFooterInternalType { HEADER_A, HEADER_B, FOOTER_A, FOOTER_B, DUMMY };
enum WPSHeaderFooterOccurence { ODD, EVEN, ALL, NEVER };
enum WPSFormOrientation { PORTRAIT, LANDSCAPE };

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
#define WPS_ALL_CAPS_BIT 262144
#define WPS_EMBOSS_BIT 524288
#define WPS_ENGRAVE_BIT 1048576

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

#ifdef DEBUG
std::string to_bits(std::string s);
#endif

#endif /* LIBWPS_INTERNAL_H */
