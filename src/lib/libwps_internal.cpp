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

#include <string>
#include <stdlib.h>

#include <libwpd/WPXBinaryData.h>

#include "libwps_internal.h"

namespace libwps
{
uint8_t readU8(WPXInputStream *input)
{
	unsigned long numBytesRead;
	unsigned char const *p = input->read(sizeof(uint8_t), numBytesRead);

	if (!p || numBytesRead != sizeof(uint8_t))
	{
		static bool first = true;
		if (first)
		{
			first = false;
			WPS_DEBUG_MSG(("libwps::readU8: can not read data\n"));
		}
		return 0;
	}

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

bool readData(WPXInputStreamPtr &input, unsigned long size, WPXBinaryData &data)
{
	data.clear();
	if (size == 0) return true;

	const unsigned char *readData;
	unsigned long sizeRead;
	while (size > 2048 && (readData=input->read(2048, sizeRead)) != 0 && sizeRead)
	{
		data.append(readData, sizeRead);
		size -= sizeRead;
	}
	if (size > 2048) return false;

	readData=input->read(size, sizeRead);
	if (size != sizeRead) return false;
	data.append(readData, sizeRead);

	return true;
}

bool readDataToEnd(WPXInputStreamPtr &input, WPXBinaryData &data)
{
	data.clear();

	const unsigned char *readData;
	unsigned long sizeRead;
	while ((readData=input->read(2048, sizeRead)) != 0 && sizeRead)
		data.append(readData, sizeRead);

	return input->atEOS();
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

std::ostream &operator<<(std::ostream &o, WPSFont const &ft)
{
	int flags = ft.m_attributes;
	if (!ft.m_name.empty())
		o << "nam='" << ft.m_name << "',";
	if (ft.m_size > 0) o << "sz=" << ft.m_size << ",";

	if (flags) o << "fl=";
	if (flags&WPS_BOLD_BIT) o << "b:";
	if (flags&WPS_ITALICS_BIT) o << "it:";
	if (flags&WPS_UNDERLINE_BIT) o << "underL:";
	if (flags&WPS_OVERLINE_BIT) o << "overL:";
	if (flags&WPS_EMBOSS_BIT) o << "emboss:";
	if (flags&WPS_SHADOW_BIT) o << "shadow:";
	if (flags&WPS_OUTLINE_BIT) o << "outline:";
	if (flags&WPS_DOUBLE_UNDERLINE_BIT) o << "2underL:";
	if (flags&WPS_STRIKEOUT_BIT) o << "strikeout:";
	if (flags&WPS_SMALL_CAPS_BIT) o << "smallCaps:";
	if (flags&WPS_ALL_CAPS_BIT) o << "allCaps:";
	if (flags&WPS_HIDDEN_BIT) o << "hidden:";
	if (flags&WPS_SMALL_PRINT_BIT) o << "consended:";
	if (flags&WPS_LARGE_BIT) o << "extended:";
	if (flags&WPS_SUPERSCRIPT_BIT) o << "superS:";
	if (flags&WPS_SUBSCRIPT_BIT) o << "subS:";
	if (flags) o << ",";

	if (ft.m_color) o << "col=" << std::hex << ft.m_color << ",";
	if (ft.m_extra.length()) o << "extra=" << ft.m_extra << ",";
	return o;
}

bool WPSFont::operator==(WPSFont const &ft) const
{
	if (m_size != ft.m_size || m_attributes != ft.m_attributes
	        || m_color != ft.m_color || m_languageId != ft.m_languageId)
		return false;
	if (m_name.compare(ft.m_name) || m_extra.compare(ft.m_extra))
		return false;
	return true;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
