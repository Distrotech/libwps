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
	//! returns true if it is a bool data
	bool isBool() const
	{
		return !hasStr() && (m_type & 0xb0)==0;
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
