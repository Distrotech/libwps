/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
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

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include "WPS8Struct.h"

#include "WPSDebug.h"

namespace WPS8Struct
{
// try to read a block, which can be or not a list of data
bool FileData::readArrayBlock() const
{
	if (isRead()) return isArray();
	long actPos = m_input->tell();
	m_input->seek(m_beginOffset, WPX_SEEK_SET);
	std::string error;
	bool ok = readBlockData(m_input, m_endOffset, const_cast<FileData &>(*this), error);
	m_input->seek(actPos, WPX_SEEK_SET);
	return ok;
}

// create a message to store unparsed data
std::string FileData::createErrorString(WPXInputStreamPtr input, long endPos)
{
	libwps::DebugStream f;
	f << ",###unread=(" << std::hex;
	while (input->tell() < endPos-1) f << libwps::readU16(input) << ", ";
	if (input->tell() < endPos)  f << libwps::readU8(input) << ", ";
	f << ")";

	return f.str();
}

// operator <<
std::ostream &operator<< (std::ostream &o, FileData const &dt)
{
	o << "unkn" << std::hex << dt.id() << "[typ=" << dt.m_type << "]:" << std::dec;
	FileData &DT = const_cast<FileData &>(dt);
	// If the data are unread, try to read them as a block list
	if (!dt.isRead() && !dt.readArrayBlock())
	{
		// if this fails...
		int size = dt.m_endOffset-dt.m_beginOffset-2;
		int sz = (size%4) == 0 ? 4 : (size%2) == 0 ? 2 : 1;
		int numElt = size/sz;

		long actPos = DT.m_input->tell();
		DT.m_input->seek(dt.m_beginOffset, WPX_SEEK_SET);
		o << "###FAILS[sz="<< sz << "]=(" << std::hex;
		long val = (long) libwps::read16(DT.m_input);
		if (val) o << "unkn=" << val <<",";
		for (int i = 0; i < numElt; i++)
		{
			switch(sz)
			{
			case 1:
				o << libwps::readU8(DT.m_input) << ",";
				break;
			case 2:
				o << libwps::readU16(DT.m_input) << ",";
				break;
			case 4:
				o << libwps::readU32(DT.m_input) << ",";
				break;
			default:
				break;
			}
		}
		o << ")" << std::dec;

		DT.m_input->seek(actPos, WPX_SEEK_SET);

		return o;
	}

	if (dt.hasStr()) o << "('" << dt.m_text << "')";
	if ((dt.m_type & 0x30) || dt.m_value)
		o << "=" << dt.m_value << ":" << std::hex << dt.m_value << std::dec;
	size_t numChild = dt.m_recursData.size();
	if (!numChild) return o;

	o << ",ch=(";
	for (size_t i = 0; i < numChild; i++)
	{
		if (dt.m_recursData[i].isBad()) continue;
		o << dt.m_recursData[i] << ",";
	}
	o << ")";
	return o;
}

// try to read a data : which can be an item, a list or unknown zone
bool readBlockData(WPXInputStreamPtr input, long endPos, FileData &dt, std::string &error)
{
	std::string saveError = error;
	long actPos = input->tell();
	dt.m_recursData.resize(0);

	if (actPos+2 > endPos)   // to short
	{
		error += FileData::createErrorString(input, endPos);
		return false;
	}

	dt.m_value = libwps::readU16(input); // normally 0, but who know ...
	dt.m_beginOffset = dt.m_endOffset = -1;

	int prevId = -1;
	bool ok = true;
	while (input->tell() != endPos)
	{
		FileData child;
		if (!readData(input, endPos, child, error))
		{
			ok = false;
			break;
		}
		if (child.isBad()) continue;
		if (prevId > child.id())
		{
			ok = false;
			break;
		}
		prevId = child.id();
		dt.m_recursData.push_back(child);
	}
	if (ok) return true;

	if (dt.m_type == -1) dt.m_type = 0x80;
	dt.m_beginOffset = actPos;
	dt.m_endOffset = endPos;
	dt.m_input = input;

	error = saveError;
	input->seek(endPos, WPX_SEEK_SET);

	return false;
}

// try to read an item
bool readData(WPXInputStreamPtr input, long endPos,
              FileData &dt, std::string &/*error*/)
{
	long actPos = input->tell();
	dt = FileData();

	if (actPos >= endPos) return false;

	long val = (long) libwps::readU16(input);
	dt.m_type = ((val & 0xFF00)>>8);
	dt.m_id = (val & 0xFF);

	if (dt.m_type & 5)
	{
		dt.m_type = -1;
		return false;
	}

	dt.m_value = 0;
	// what is the meaning of dt.m_type & 0xF
	//   maybe :
	//           0x1/0x4 -> never seem
	//           0x2 -> set for the main child ?
	//           0x8 -> signed/unsigned ?
	switch(dt.m_type>>4)
	{
	case 0:
		return true;
	case 1:
		if (actPos+4 > endPos) break;
		dt.m_value = libwps::readU16(input);
		return true;
	case 2:
	{
		if (dt.m_type == 0x2a)   // special case : STR4 + long
		{
			if (actPos+10 > endPos) break;
			for (int i = 0; i < 4; i++) dt.m_text += (char) libwps::readU8(input);
			dt.m_value = libwps::read32(input);
			return true;
		}
		if (actPos+6 > endPos) break;
		dt.m_value = libwps::read32(input);
		return true;
	}
	case 8:
	{
		if (actPos+4 > endPos) break;

		long extraSize = (long) libwps::readU16(input);
		long newEndPos = actPos+2+extraSize;

		if ((extraSize%2) || newEndPos > endPos) break;

		// can either be a list of data or a structured list, so we stored the information
		dt.m_beginOffset = actPos+4;
		dt.m_endOffset = newEndPos;
		dt.m_input = input;
		input->seek(newEndPos, WPX_SEEK_SET);
		return true;
	}
	default:
		break;
	}

	dt.m_type = -1;
	return false;
}

}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
