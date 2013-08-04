/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002, 2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002, 2004 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwps.sourceforge.net
 */

#include <iomanip>
#include <sstream>
#include <string>
#include <stdlib.h>

#include <libwpd/libwpd.h>

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
	uint8_t p0 = readU8(input);
	uint8_t p1 = readU8(input);
	return (uint16_t) (p0|(p1<<8));
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
	if ((readData=input->read(size, sizeRead)) == 0 || sizeRead!=size)
		return false;
	data.append(readData, sizeRead);

	return true;
}

bool readDataToEnd(WPXInputStreamPtr &input, WPXBinaryData &data)
{
	data.clear();
	long pos=input->tell();
	input->seek(0,WPX_SEEK_END);
	long sz=input->tell()-pos;
	if (sz < 0) return false;
	input->seek(pos,WPX_SEEK_SET);
	return readData(input, (unsigned long) sz, data) && input->atEOS();
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
	case NONE:
	case BULLET:
	default:
		break;
	}
	WPS_DEBUG_MSG(("libwps::numberingTypeToString: must not be called with type %d\n", int(type)));
	return "1";
}
}

std::ostream &operator<<(std::ostream &o, WPSFont const &ft)
{
	uint32_t flags = ft.m_attributes;
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

	if (ft.m_color) o << "col=" << std::hex << ft.m_color << std::dec << ",";
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

////////////////////////////////////////////////////////////
// border
////////////////////////////////////////////////////////////

int WPSBorder::compare(WPSBorder const &orig) const
{
	int diff = int(m_style)-int(orig.m_style);
	if (diff) return diff;
	diff = m_width-orig.m_width;
	if (diff) return diff;
	if (m_color < orig.m_color) return -1;
	if (m_color > orig.m_color) return -1;
	return 0;
}

std::string WPSBorder::getPropertyValue() const
{
	if (m_style == None) return "";
	std::stringstream stream;
	stream << m_width*0.03 << "cm";
	switch (m_style)
	{
	case Dot:
	case LargeDot:
		stream << " dotted";
		break;
	case Dash:
		stream << " dashed";
		break;
	case Single:
		stream << " solid";
		break;
	case Double:
		stream << " double";
		break;
	case None:
	default:
		break;
	}
	stream << " #" << std::hex << std::setfill('0') << std::setw(6)
	       << (m_color&0xFFFFFF);
	return stream.str();
}

std::ostream &operator<< (std::ostream &o, WPSBorder const &border)
{
	switch (border.m_style)
	{
	case WPSBorder::None:
		o << "none:";
		break;
	case WPSBorder::Single:
		break;
	case WPSBorder::Dot:
		o << "dot:";
		break;
	case WPSBorder::LargeDot:
		o << "large dot:";
		break;
	case WPSBorder::Dash:
		o << "dash:";
		break;
	case WPSBorder::Double:
		o << "double:";
		break;
	default:
		WPS_DEBUG_MSG(("WPSBorder::operator<<: find unknown style\n"));
		o << "#style=" << int(border.m_style) << ":";
		break;
	}
	if (border.m_width > 1) o << "w=" << border.m_width << ":";
	if (border.m_color)
		o << "col=" << std::hex << border.m_color << std::dec << ":";
	o << ",";
	return o;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
