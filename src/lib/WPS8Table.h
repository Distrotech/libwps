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

#ifndef WPS8_TABLE
#  define WPS8_TABLE

#include "libwps_internal.h"

#include "WPSDebug.h"

class WPSEntry;
class WPS8Parser;
class WPSPosition;

typedef class WPSContentListener WPS8ContentListener;
typedef shared_ptr<WPS8ContentListener> WPS8ContentListenerPtr;

namespace WPS8TableInternal
{
struct State;
}

////////////////////////////////////////////////////////////
//
// class to parse the object
//
////////////////////////////////////////////////////////////

/** \brief the main class to read/store table in a Pc MS Works document v5-8
 *
 * This class must be associated with a WPS8Parser. It contains code to read
 * the MCLD entries which seems to correspond to a table entry (potentially a table
 * entry with only one cells).
 *
 * \note Actually, the relations between the MCLD entries and the FRAM entries (which
 * seems to store all the object positions) are not fully understood ; so the result
 * of this parsing is not used to create the document.
 */
class WPS8Table
{
	friend class WPS8Parser;
public:
	//! constructor
	WPS8Table(WPS8Parser &parser);

	//! destructor
	~WPS8Table();

	//! sets the listener
	void setListener(WPS8ContentListenerPtr &listen)
	{
		m_listener = listen;
	}

	//! computes the final position of all table: does nothing as table anchors seem always a char
	void computePositions() const;

	//! returns the number page where we find a picture. In practice, 0/1
	int numPages() const;

	/** sends the data which have not been send to the listener.
	 *
	 * \note actually, it does not nothing.
	 */
	void flushExtra(WPXInputStreamPtr input);

protected:
	//! finds all structures which correspond to a table
	bool readStructures(WPXInputStreamPtr input);

	/** tries to send a table corresponding to strsid with actual size siz
	 *
	 * \param siz the size of the table in the document
	 * \param tableId the table identificator
	 * \param strsid indicates the text entry (and its corresponding TCD )
	 * which contains the cells' text.
	 * \warning Actually, it only creates a textbox and sends all the cells' text
	 * separated with '#'.
	 */
	bool sendTable(Vec2f const &siz, int tableId, int strsid);

protected: // low level

	//! reads a MCLD zone: a zone which stores the tables structures
	bool readMCLD(WPXInputStreamPtr input, WPSEntry const &entry);

	//! returns the file version
	int version() const;

	//! returns the debug file
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}
private:
	WPS8Table(WPS8Table const &orig);
	WPS8Table &operator=(WPS8Table const &orig);

protected:
	//! the listener
	WPS8ContentListenerPtr m_listener;

	//! the main parser
	WPS8Parser &m_mainParser;

	//! the state
	mutable shared_ptr<WPS8TableInternal::State> m_state;

	//! the ascii file
	libwps::DebugFile &m_asciiFile;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
