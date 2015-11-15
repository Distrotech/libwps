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

#include <stdlib.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"

namespace libwps
{
uint8_t readU8(librevenge::RVNGInputStream *input)
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

int8_t read8(librevenge::RVNGInputStream *input)
{
	return (int8_t) readU8(input);
}

uint16_t readU16(librevenge::RVNGInputStream *input)
{
	uint8_t p0 = readU8(input);
	uint8_t p1 = readU8(input);
	return (uint16_t)(p0|(p1<<8));
}

int16_t read16(librevenge::RVNGInputStream *input)
{
	return (int16_t) readU16(input);
}

uint32_t readU32(librevenge::RVNGInputStream *input)
{
	uint8_t p0 = readU8(input);
	uint8_t p1 = readU8(input);
	uint8_t p2 = readU8(input);
	uint8_t p3 = readU8(input);
	return (uint32_t)((p0<<0)|(p1<<8)|(p2<<16)|(p3<<24));
}

int32_t read32(librevenge::RVNGInputStream *input)
{
	return (int32_t) readU32(input);
}

bool readDouble4(RVNGInputStreamPtr &input, double &res, bool &isNaN)
{
	isNaN=false;
	res = 0;
	long pos = input->tell();
	if (input->seek(4, librevenge::RVNG_SEEK_CUR) || input->tell()!=pos+4)
	{
		WPS_DEBUG_MSG(("libwps::readDouble4: the zone seems too short\n"));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	input->seek(pos, librevenge::RVNG_SEEK_SET);

	float mantisse = 0;
	/** (first&3)==1: is used to decide if we store 100*N or N.,
		(first&3)==2: indicates a basic int number (appears mainly when converting a dos file in a windows file)
		(first&3)==3: Can this exist ? What does this mean: 100*a basic int ?
		The other bytes seem to have classic meaning...
	*/
	int first = (int) readU8(input);
	if ((first&3)==2)
	{
		// so read it as a normal number
		input->seek(-1, librevenge::RVNG_SEEK_CUR);
		long val=long(readU16(input)>>2);
		val+=long(readU16(input))<<14;
		if (val&0x20000000)
			res = double(val-0x40000000);
		else
			res = double(val);
		return true;
	}
	mantisse = float(first & 0xFC)/256.f + (float) readU8(input);
	int mantExp = (int) readU8(input);
	mantisse = (mantisse/256.f + float(0x10+(mantExp & 0x0F)))/16.f;
	int exp = ((mantExp&0xF0)>>4)+int(readU8(input)<<4);
	int sign = 1;
	if (exp & 0x800)
	{
		exp &= 0x7ff;
		sign = -1;
	}

	if (exp == 0)
	{
		if ((double) mantisse > 1.-1e-4)  return true; // ok zero
		// fixme find Nan representation
		return false;
	}
	if (exp == 0x7FF)
	{
		if ((double) mantisse > 1.-1e-4)
		{
			res=std::numeric_limits<double>::quiet_NaN();
			isNaN=true;
			/* 0x7FFFF.. are nan(infinite, ...):ok

			   0xFFFFF.. are nan(in the sense, not a number but
			   text...). In this case wps2csv and wps2text will
			   display a nan. Not good, but difficult to retrieve the
			   cell's content without excuting the formula associated
			   to this cell :-~
			 */
			return true;
		}
		return false;
	}

	exp -= 0x3ff;
	res = std::ldexp(mantisse, exp);
	if (sign == -1)
	{
		res *= -1.;
	}
	if (first & 1) res/=100;
	if (first & 2)
	{
		// CHECKME...
		WPS_DEBUG_MSG(("libwps::readDouble4: ARRGGGGGGGGGG find a float with first & 3 ARRGGGGGGGGGG in pos%lx,\n some float can be broken\n", (unsigned long) pos));
	}
	return true;
}

bool readDouble8(RVNGInputStreamPtr &input, double &res, bool &isNaN)
{
	isNaN=false;
	res = 0;
	long pos = input->tell();
	if (input->seek(8, librevenge::RVNG_SEEK_CUR) || input->tell()!=pos+8)
	{
		WPS_DEBUG_MSG(("libwps::readDouble8: the zone seems too short\n"));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	double mantisse = 0;
	for (int i = 0; i < 6; i++)
		mantisse = mantisse/256 + (double)readU8(input);
	int mantExp = (int) readU8(input);
	mantisse = (mantisse/256 + double(0x10+(mantExp & 0x0F)))/16;
	int exp = ((mantExp&0xF0)>>4)+int(readU8(input)<<4);
	int sign = 1;
	if (exp & 0x800)
	{
		exp &= 0x7ff;
		sign = -1;
	}

	double const epsilon=1.e-5;
	if (exp == 0)
	{
		if (mantisse > 1-epsilon && mantisse < 1+epsilon)  return true; // ok zero
		// fixme find Nan representation
		return false;
	}
	if (exp == 0x7FF)
	{
		if (mantisse >= 1-epsilon)
		{
			res=std::numeric_limits<double>::quiet_NaN();
			return true; // ok 0x7FF and 0xFFF are nan
		}
		return false;
	}

	exp -= 0x3ff;
	res = std::ldexp(mantisse, exp);
	if (sign == -1)
	{
		res *= -1.;
	}
	return true;
}

bool readDouble10(RVNGInputStreamPtr &input, double &res, bool &isNaN)
{
	isNaN=false;
	res = 0;
	long pos = input->tell();
	if (input->seek(10, librevenge::RVNG_SEEK_CUR) || input->tell()!=pos+10)
	{
		WPS_DEBUG_MSG(("libwps::readDouble8: the zone seems too short\n"));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	double mantisse = 0;
	for (int i = 0; i < 8; i++)
		mantisse = mantisse/256 + (double)readU8(input)/128;
	int exp = (int)readU16(input);
	int sign = 1;
	if (exp & 0x8000)
	{
		exp &= 0x7fff;
		sign = -1;
	}

	double const epsilon=1.e-5;
	if (exp == 0)
	{
		if (mantisse < epsilon)  return true; // checkme is this zero or a nan
		// fixme find Nan representation
		return false;
	}
	if (exp == 0x7FFf)
	{
		if (mantisse >= 1-epsilon)
		{
			res=std::numeric_limits<double>::quiet_NaN();
			return true; // ok 0x7FF and 0xFFF are nan
		}
		return false;
	}

	exp -= 0x3fff;
	res = std::ldexp(mantisse, exp);
	if (sign == -1)
	{
		res *= -1.;
	}
	return true;
}

bool readDouble2Inv(RVNGInputStreamPtr &input, double &res, bool &isNaN)
{
	isNaN=false;
	res = 0;
	long pos = input->tell();
	if (input->seek(2, librevenge::RVNG_SEEK_CUR) || input->tell()!=pos+2)
	{
		WPS_DEBUG_MSG(("libwps::readDouble2Inv: the zone seems too short\n"));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	int val=(int) libwps::readU16(input);
	int exp=val&0xf;
	if ((exp&1)==1)
	{
		int mantisse=(val>>4);
		if ((mantisse&0x800))
			mantisse -= 0x1000;
		exp/=2;
		const double factors[8]= { 5000, 500, 0.05, 0.005, 0.0005, 0.00005, 1/16., 1/64. };
		res=double(mantisse)*factors[exp];
		return true;
	}
	if ((val&0x8000))
		val-=0x10000;
	res=double(val>>1);
	return true;
}

bool readDouble4Inv(RVNGInputStreamPtr &input, double &res, bool &isNaN)
{
	isNaN=false;
	res = 0;
	long pos = input->tell();
	if (input->seek(4, librevenge::RVNG_SEEK_CUR) || input->tell()!=pos+4)
	{
		WPS_DEBUG_MSG(("libwps::readDouble4Inv: the zone seems too short\n"));
		input->seek(pos, librevenge::RVNG_SEEK_SET);
		return false;
	}
	input->seek(pos, librevenge::RVNG_SEEK_SET);
	long val=(long) libwps::readU32(input);
	int exp=int(val&0xf);
	int mantisse=int(val>>6);
	if (val&0x20)
		mantisse *= -1;
	if (exp)
	{
		if (val&0x10)
			res=mantisse/std::pow(10., exp);
		else
			res=mantisse*std::pow(10., exp);
		return true;
	}
	res=double(mantisse);
	return true;
}

bool readData(RVNGInputStreamPtr &input, unsigned long size, librevenge::RVNGBinaryData &data)
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

bool readDataToEnd(RVNGInputStreamPtr &input, librevenge::RVNGBinaryData &data)
{
	data.clear();
	long pos=input->tell();
	input->seek(0,librevenge::RVNG_SEEK_END);
	long sz=input->tell()-pos;
	if (sz < 0) return false;
	input->seek(pos,librevenge::RVNG_SEEK_SET);
	return readData(input, (unsigned long) sz, data) && input->isEnd();
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

////////////////////////////////////////////////////////////
// color
////////////////////////////////////////////////////////////

// color function
WPSColor WPSColor::barycenter(float alpha, WPSColor const &colA,
                              float beta, WPSColor const &colB)
{
	uint32_t res = 0;
	for (int i=0, depl=0; i<4; i++, depl+=8)
	{
		float val=alpha*float((colA.m_value>>depl)&0xFF)+beta*float((colB.m_value>>depl)&0xFF);
		if (val < 0) val=0;
		if (val > 256) val=256;
		unsigned char comp= (unsigned char)val;
		res+=uint32_t(comp<<depl);
	}
	return WPSColor(res);
}

std::ostream &operator<< (std::ostream &o, WPSColor const &c)
{
	const std::streamsize width = o.width();
	const char fill = o.fill();
	o << "#" << std::hex << std::setfill('0') << std::setw(6)
	  << (c.m_value&0xFFFFFF)
	  // std::ios::width() takes/returns std::streamsize (long), but
	  // std::setw() takes int. Go figure...
	  << std::dec << std::setfill(fill) << std::setw(static_cast<int>(width));
	return o;
}

std::string WPSColor::str() const
{
	std::stringstream stream;
	stream << *this;
	return stream.str();
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
	if (m_color > orig.m_color) return 1;
	if (m_widthsList.size() != orig.m_widthsList.size())
		return m_widthsList.size() < orig.m_widthsList.size() ? -1 : 1;
	for (size_t i=0; i<m_widthsList.size(); ++i)
	{
		if (m_widthsList[i]<orig.m_widthsList[i]) return -1;
		if (m_widthsList[i]>orig.m_widthsList[i]) return 1;
	}
	return 0;
}

bool WPSBorder::addTo(librevenge::RVNGPropertyList &propList, std::string const which) const
{
	std::stringstream stream, field;
	stream << m_width << "pt ";
	if (m_type==WPSBorder::Double || m_type==WPSBorder::Triple)
	{
		static bool first = true;
		if (first && m_style!=Simple)
		{
			WPS_DEBUG_MSG(("WPSBorder::addTo: find double or tripe border with complex style\n"));
			first = false;
		}
		stream << "double";
	}
	else
	{
		switch (m_style)
		{
		case Dot:
		case LargeDot:
			stream << "dotted";
			break;
		case Dash:
			stream << "dashed";
			break;
		case Simple:
			stream << "solid";
			break;
		case None:
		default:
			stream << "none";
			break;
		}
	}
	stream << " " << m_color.str();
	field << "fo:border";
	if (which.length())
		field << "-" << which;
	propList.insert(field.str().c_str(), stream.str().c_str());
	size_t numRelWidth=m_widthsList.size();
	if (!numRelWidth)
		return true;
	if (m_type!=WPSBorder::Double || numRelWidth!=3)
	{
		static bool first = true;
		if (first)
		{
			WPS_DEBUG_MSG(("WPSBorder::addTo: relative width is only implemented with double style\n"));
			first = false;
		}
		return true;
	}
	double totalWidth=0;
	for (size_t w=0; w < numRelWidth; w++)
		totalWidth+=m_widthsList[w];
	if (totalWidth <= 0)
	{
		WPS_DEBUG_MSG(("WPSBorder::addTo: can not compute total width\n"));
		return true;
	}
	double factor=m_width/totalWidth;
	stream.str("");
	for (size_t w=0; w < numRelWidth; w++)
	{
		stream << factor *m_widthsList[w]<< "pt";
		if (w+1!=numRelWidth)
			stream << " ";
	}
	field.str("");
	field << "style:border-line-width";
	if (which.length())
		field << "-" << which;
	propList.insert(field.str().c_str(), stream.str().c_str());
	return true;
}

std::ostream &operator<< (std::ostream &o, WPSBorder::Style const &style)
{
	switch (style)
	{
	case WPSBorder::None:
		o << "none";
		break;
	case WPSBorder::Simple:
		break;
	case WPSBorder::Dot:
		o << "dot";
		break;
	case WPSBorder::LargeDot:
		o << "large dot";
		break;
	case WPSBorder::Dash:
		o << "dash";
		break;
	default:
		WPS_DEBUG_MSG(("WPSBorder::operator<<: find unknown style\n"));
		o << "#style=" << int(style);
		break;
	}
	return o;
}

std::ostream &operator<< (std::ostream &o, WPSBorder const &border)
{
	o << border.m_style << ":";
	switch (border.m_type)
	{
	case WPSBorder::Single:
		break;
	case WPSBorder::Double:
		o << "double:";
		break;
	case WPSBorder::Triple:
		o << "triple:";
		break;
	default:
		WPS_DEBUG_MSG(("WPSBorder::operator<<: find unknown type\n"));
		o << "#type=" << int(border.m_type) << ":";
		break;
	}
	if (border.m_width > 1 || border.m_width < 1) o << "w=" << border.m_width << ":";
	if (!border.m_color.isBlack())
		o << "col=" << std::hex << border.m_color << std::dec << ":";
	o << ",";
	size_t numRelWidth=border.m_widthsList.size();
	if (numRelWidth)
	{
		o << "bordW[rel]=[";
		for (size_t i=0; i < numRelWidth; i++)
			o << border.m_widthsList[i] << ",";
		o << "]:";
	}
	o << border.m_extra;
	return o;
}

////////////////////////////////////////////////////////////
// object
////////////////////////////////////////////////////////////
bool WPSEmbeddedObject::addTo(librevenge::RVNGPropertyList &propList) const
{
	bool firstSet=false;
	librevenge::RVNGPropertyListVector auxiliarVector;
	for (size_t i=0; i<m_dataList.size(); ++i)
	{
		if (m_dataList[i].empty()) continue;
		std::string type=m_typeList.size() ? m_typeList[i] : "image/pict";
		if (!firstSet)
		{
			propList.insert("librevenge:mime-type", type.c_str());
			propList.insert("office:binary-data", m_dataList[i]);
			firstSet=true;
			continue;
		}
		librevenge::RVNGPropertyList auxiList;
		auxiList.insert("librevenge:mime-type", type.c_str());
		auxiList.insert("office:binary-data", m_dataList[i]);
		auxiliarVector.append(auxiList);
	}
	if (!auxiliarVector.empty())
		propList.insert("librevenge:replacement-objects", auxiliarVector);
	if (!firstSet)
	{
		WPS_DEBUG_MSG(("WPSEmbeddedObject::addTo: called without picture\n"));
		return false;
	}
	return true;
}

std::ostream &operator<<(std::ostream &o, WPSEmbeddedObject const &pict)
{
	if (pict.isEmpty()) return o;
	o << "[";
	for (size_t i=0; i<pict.m_typeList.size(); ++i)
	{
		if (pict.m_typeList[i].empty())
			o << "_,";
		else
			o << pict.m_typeList[i] << ",";
	}
	o << "],";
	return o;
}

////////////////////////////////////////////////////////////
// unicode
////////////////////////////////////////////////////////////
namespace libwps
{
void appendUnicode(uint32_t val, librevenge::RVNGString &buffer)
{
	if (val < 0x20)
	{
		WPS_DEBUG_MSG(("libwps::appendUnicode: find an old char %x, skip it\n", val));
		return;
	}
	uint8_t first;
	int len;
	if (val < 0x80)
	{
		first = 0;
		len = 1;
	}
	else if (val < 0x800)
	{
		first = 0xc0;
		len = 2;
	}
	else if (val < 0x10000)
	{
		first = 0xe0;
		len = 3;
	}
	else if (val < 0x200000)
	{
		first = 0xf0;
		len = 4;
	}
	else if (val < 0x4000000)
	{
		first = 0xf8;
		len = 5;
	}
	else
	{
		first = 0xfc;
		len = 6;
	}

	char outbuf[7];
	int i;
	for (i = len - 1; i > 0; --i)
	{
		outbuf[i] = char((val & 0x3f) | 0x80);
		val >>= 6;
	}
	outbuf[0] = char(val | first);
	outbuf[len] = 0;
	buffer.append(outbuf);
}
}

////////////////////////////////////////////////////////////
// debug
////////////////////////////////////////////////////////////

namespace libwps
{
#ifdef DEBUG
void printDebugMsg(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, format, args);
	va_end(args);
}
#endif
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
