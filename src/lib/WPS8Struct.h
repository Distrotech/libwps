/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
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

#ifndef WPS8_STRUCT_H
#define WPS8_STRUCT_H

#include <string>
#include <vector>

#include "libwps_internal.h"

////////////////////////////////////////////////////////////
//    Internal structure
////////////////////////////////////////////////////////////

/** namespace used to read the structures stored in a WPS8 files */
namespace WPS8Struct
{
struct FileData;
/** tries to read a block zone as a Data */
bool readData(WPXInputStreamPtr input, long endPos, FileData &dt, std::string &error);
/** tries to read a block zone as a list of Data */
bool readBlockData(WPXInputStreamPtr input, long endPos, FileData &dt, std::string &error);
//! operator<< which allows to print for debugging the content of a Data
std::ostream &operator<< (std::ostream &o, FileData const &dt);

/** A recursif structure which seems generally used to store complex memory structures in a file
 *
 * Each element seems to contains
 * - the encoded type of the field (bool, int, strings, lists, entry ...)
 * - an identificator of the field
 * - the field values
 *
 * \note
 * - the distinction between a lists of recursif Data and a list of \<\< local \>\> structures is not clear.
 *   This implies that when the field is a list, readBlockData must be called in order to try reading
 *   the data recursively
 * - the case typeId = 0x2a seems to correspond to an entry id (4 letters + id), it is the only special case ?
 * - the difference between signed and unsigned field must be checked.
 */
struct FileData
{
	//! constructor
	FileData() : m_value(0), m_text(""), m_recursData(), m_type(-1), m_id(-1), m_beginOffset(-1), m_endOffset(-1), m_input() {}
	//! returns true if the field was not read
	bool isBad() const
	{
		return m_type==-1;
	}
	//! returns true if it is a string data
	bool hasStr() const
	{
		return m_text.size() != 0;
	}
	//! returns true if it is a number data
	bool isNumber() const
	{
		return !hasStr() && (m_type & 0x30) != 0;
	}
	//! returns a rgb color by converting the integer value field
	uint32_t getRGBColor() const
	{
		uint32_t col = (uint32_t) (m_value&0xFFFFFF);
		return (((col>>16)&0xFF) |(col&0xFF00)|((col&0xFF)<<16));
	}
	//! returns the border style using the integer value field
	WPSBorder::Style getBorderStyle(std::string &mess) const;

	//! returns true if it is a bool data
	bool isBool() const
	{
		return !hasStr() && (m_type & 0xb0)==0;
	}
	//! returns true if this is a bool and the val is true
	bool isTrue() const
	{
		return m_type == 0xa;
	}
	//! returns true if this is a bool and the val is false
	bool isFalse() const
	{
		return m_type == 0x2;
	}
	//! returns true if this is a list of block or an unstructured list
	bool isArray() const
	{
		return !hasStr() && (m_type & 0x80)==0x80;
	}
	//! returns true if the data are read
	bool isRead() const
	{
		return (m_type & 0x80) !=0x80 || !(m_input.get() && m_beginOffset > 0 && m_endOffset >= m_beginOffset+2);
	}

	//! returns the data type (low level)
	int type() const
	{
		if (m_type == 0xa) return 2;
		return m_type;
	}
	//! returns the identificator
	int id() const
	{
		return m_id;
	}

	//! forces reading the data as a list of block
	bool readArrayBlock() const;

	//! an int value, filled if the data store an val
	long m_value;
	//! the string values
	std::string m_text;
	//! the list of children
	std::vector<FileData> m_recursData;

	//! beginning of data position
	long begin() const
	{
		return m_beginOffset;
	}
	//! end of data position
	long end() const
	{
		return m_endOffset;
	}
protected:
	//! creates a string used to store the unparsed data
	static std::string createErrorString(WPXInputStreamPtr input, long endPos);

	//! an int which indicates the data type
	int m_type;
	//! an identificator
	int m_id;

	long m_beginOffset /** the initial position of the data of this field */, m_endOffset /** the final position of the data of this field */;
	//! the input
	WPXInputStreamPtr m_input;

	//! operator<<
	friend std::ostream &operator<< (std::ostream &o, FileData const &dt);
	//! function which parses an element
	friend bool readData(WPXInputStreamPtr input, long endPos, FileData &dt, std::string &error);
	//! function which parses a set of elements
	friend bool readBlockData(WPXInputStreamPtr input, long endPos, FileData &dt, std::string &error);
};
}

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
