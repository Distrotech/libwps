/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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

#ifndef WPSLISTENER_H
#define WPSLISTENER_H

#include <librevenge/librevenge.h>

#include "libwps_internal.h"

struct WPSParagraph;
struct WPSTabStop;

//! virtual class for content listener
class WPSListener
{
public:
	WPSListener() {}
	virtual ~WPSListener() {}

	virtual void setDocumentLanguage(int lcid)=0;

	// ------ text data -----------

	//! adds a basic character, ..
	virtual void insertCharacter(uint8_t character)=0;
	/** adds an unicode character
	 *
	 * by convention if \a character=0xfffd(undef), no character is added */
	virtual void insertUnicode(uint32_t character)=0;
	//! adds a unicode string
	virtual void insertUnicodeString(librevenge::RVNGString const &str)=0;
	//! adds an unicode character to a string ( with correct encoding ).

	virtual void insertTab()=0;
	virtual void insertEOL(bool softBreak=false)=0;
	virtual void insertBreak(const uint8_t breakType)=0;

	// ------ text format -----------
	//! set the actual font
	virtual void setFont(const WPSFont &font)=0;
	//! returns the actual font
	virtual WPSFont const &getFont() const=0;

	// ------ paragraph format -----------
	//! returns true if a paragraph or a list is opened
	virtual bool isParagraphOpened() const=0;
	//! sets the actual paragraph
	virtual void setParagraph(const WPSParagraph &para)=0;
	//! returns the actual paragraph
	virtual WPSParagraph const &getParagraph() const=0;

	// ------- fields ----------------
	/** Defines some basic type for field */
	enum FieldType { None, PageNumber, Date, Time, Title, Link, Database };
	//! adds a field type
	virtual void insertField(FieldType type) = 0;
	//! insert a date/time field with given format (see strftime)
	virtual void insertDateTimeField(char const *format)=0;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
