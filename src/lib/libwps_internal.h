/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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
#include <libwpd-stream/libwpd-stream.h>
#include <libwpd/libwpd.h>
#include <string>

#if defined(_MSC_VER) || defined(__DJGPP__)
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
#else /* !_MSC_VER && !__DJGPP__*/
#include <inttypes.h>
#endif /* _MSC_VER || __DJGPP__*/

/* ---------- memory  --------------- */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(SHAREDPTR_TR1)
#include <tr1/memory>
using std::tr1::shared_ptr;
#elif defined(SHAREDPTR_STD)
#include <memory>
using std::shared_ptr;
#else
#include <boost/shared_ptr.hpp>
using boost::shared_ptr;
#endif

/** an noop deleter used to transform a libwpd pointer in a false shared_ptr */
template <class T>
struct WPS_shared_ptr_noop_deleter
{
	void operator() (T *) {}
};

/* ---------- typedef ------------- */
typedef shared_ptr<WPXInputStream> WPXInputStreamPtr;

/* ---------- debug  --------------- */
#ifdef DEBUG
#define WPS_DEBUG_MSG(M) printf M
#else
#define WPS_DEBUG_MSG(M)
#endif

/* ---------- exception  ------------ */
namespace libwps
{
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
}

/* ---------- input ----------------- */
namespace libwps
{
uint8_t readU8(WPXInputStream *input);
uint16_t readU16(WPXInputStream *input);
uint32_t readU32(WPXInputStream *input);

int8_t read8(WPXInputStream *input);
int16_t read16(WPXInputStream *input);
int32_t read32(WPXInputStream *input);

inline uint8_t readU8(WPXInputStreamPtr &input)
{
	return readU8(input.get());
}
inline uint16_t readU16(WPXInputStreamPtr &input)
{
	return readU16(input.get());
}
inline uint32_t readU32(WPXInputStreamPtr &input)
{
	return readU32(input.get());
}

inline int8_t read8(WPXInputStreamPtr &input)
{
	return read8(input.get());
}
inline int16_t read16(WPXInputStreamPtr &input)
{
	return read16(input.get());
}
inline int32_t read32(WPXInputStreamPtr &input)
{
	return read32(input.get());
}
}

#define WPS_LE_GET_GUINT16(p)				  \
        (uint16_t)((((uint8_t const *)(p))[0] << 0)  |    \
                  (((uint8_t const *)(p))[1] << 8))
#define WPS_LE_GET_GUINT32(p) \
        (uint32_t)((((uint8_t const *)(p))[0] << 0)  |    \
                  (((uint8_t const *)(p))[1] << 8)  |    \
                  (((uint8_t const *)(p))[2] << 16) |    \
                  (((uint8_t const *)(p))[3] << 24))

// Various helper structures for the parser..
namespace libwps
{
enum HeaderFooterType { HEADER, FOOTER };
enum HeaderFooterOccurence { ODD, EVEN, ALL, NEVER };
enum FormOrientation { PORTRAIT, LANDSCAPE };
}

// ATTRIBUTE bits
#define WPS_EXTRA_LARGE_BIT 1
#define WPS_VERY_LARGE_BIT 2
#define WPS_LARGE_BIT 4
#define WPS_SMALL_PRINT_BIT 8
#define WPS_FINE_PRINT_BIT 0x10
#define WPS_SUPERSCRIPT_BIT 0x20
#define WPS_SUBSCRIPT_BIT 0x40
#define WPS_OUTLINE_BIT 0x80
#define WPS_ITALICS_BIT 0x100
#define WPS_SHADOW_BIT 0x200
#define WPS_REDLINE_BIT 0x400
#define WPS_DOUBLE_UNDERLINE_BIT 0x800
#define WPS_BOLD_BIT 0x1000
#define WPS_STRIKEOUT_BIT 0x2000
#define WPS_UNDERLINE_BIT 0x4000
#define WPS_SMALL_CAPS_BIT 0x8000
#define WPS_BLINK_BIT 0x10000L
#define WPS_REVERSEVIDEO_BIT 0x20000L
#define WPS_ALL_CAPS_BIT 0x40000L
#define WPS_EMBOSS_BIT 0x80000L
#define WPS_ENGRAVE_BIT 0x100000L
#define WPS_SPECIAL_BIT 0x200000L

// JUSTIFICATION bits
#define WPS_PARAGRAPH_JUSTIFICATION_LEFT 0x00
#define WPS_PARAGRAPH_JUSTIFICATION_FULL 0x01
#define WPS_PARAGRAPH_JUSTIFICATION_CENTER 0x02
#define WPS_PARAGRAPH_JUSTIFICATION_RIGHT 0x03
#define WPS_PARAGRAPH_JUSTIFICATION_FULL_ALL_LINES 0x04
#define WPS_PARAGRAPH_JUSTIFICATION_DECIMAL_ALIGNED 0x05

#define WPS_PARAGRAPH_LAYOUT_NO_BREAK  0x01
#define WPS_PARAGRAPH_LAYOUT_WITH_NEXT 0x02

#define WPS_TAB_LEFT 0x00
#define WPS_TAB_CENTER 0x01
#define WPS_TAB_RIGHT 0x02
#define WPS_TAB_DECIMAL 0x03
#define WPS_TAB_BAR 0x04

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

// Field codes
#define WPS_FIELD_PAGE 1
#define WPS_FIELD_DATE 2
#define WPS_FIELD_TIME 3
#define WPS_FIELD_FILE 4

// Bullets and numbering

#define WPS_NUMBERING_NONE   0
#define WPS_NUMBERING_BULLET 1
#define WPS_NUMBERING_NUMBER 2

#define WPS_NUM_STYLE_NONE   0
#define WPS_NUM_STYLE_ARABIC 2
#define WPS_NUM_STYLE_LLATIN 3
#define WPS_NUM_STYLE_ULATIN 4
#define WPS_NUM_STYLE_LROMAN 5
#define WPS_NUM_STYLE_UROMAN 6

#endif /* LIBWPS_INTERNAL_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
