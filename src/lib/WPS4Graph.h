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

#ifndef WPS4_GRAPH
#  define WPS4_GRAPH

#include <vector>

#include "libwps_internal.h"

#include "WPSDebug.h"

class WPXBinaryData;

class WPSEntry;
class WPS4Parser;
class WPSPosition;

typedef class WPSContentListener WPS4ContentListener;
typedef shared_ptr<WPS4ContentListener> WPS4ContentListenerPtr;

namespace WPS4GraphInternal
{
struct State;
}

////////////////////////////////////////////////////////////
//
// class to parse the object
//
////////////////////////////////////////////////////////////

/** \brief the main class to read/store picture in a Pc MS Works document v1-4
 *
 * This class must be associated with a WPS4Parser.
 * \note As the pictures seems always be given with character' position, many functions
 * which exists to maintain the same structures that exist(will) in WPS8TextGraph class
 * do almost nothing.
 */
class WPS4Graph
{
	friend class WPS4Parser;
public:
	//! constructor given the main parser of the MN0 zone
	WPS4Graph(WPS4Parser &parser);

	//! destructor
	~WPS4Graph();

	//! sets the listener
	void setListener(WPS4ContentListenerPtr &listen)
	{
		m_listener = listen;
	}

	/** computes the position of all found figures.
	 *
	 * In reality, as all the pictures seemed to be given with characters positions,
	 * it does almost nothing, ie it only updates some internal bool to know the picture which
	 * have been sent to the listener.
	 */
	void computePositions() const;

	//! returns the number page where we find a picture. In practice, 0/1
	int numPages() const;

protected:
	//! returns the file version
	int version() const;

	//! store a list of object
	void storeObjects(std::vector<WPXBinaryData> const &objects,
	                  std::vector<int> const &ids,
	                  std::vector<WPSPosition> const &positions);

	/** tries to find a picture in the zone pointed by \a entry
	 * \return the object id or -1 if find nothing
	 * \note the content of this zone is mainly unknown,
	 * so this function may failed to retrieved valid data
	 */
	int readObject(WPXInputStreamPtr input, WPSEntry const &entry);

	//! sends an object with identificator \a id as a character with given size
	void sendObject(Vec2f const &size, int id);

	/** send all the objects of a given page:
	 * \param page if page < 0, sends all the pictures which have not been used,
	 *
	 * As all the pictures seemed to be given with characters positions, this
	 * function only does something if page < 0.
	 */
	void sendObjects(int page);

	//! returns the debug file
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}

private:
	WPS4Graph(WPS4Graph const &orig);
	WPS4Graph &operator=(WPS4Graph const &orig);
protected:
	//! the listener
	WPS4ContentListenerPtr m_listener;

	//! the main parser
	WPS4Parser &m_mainParser;

	//! the state
	mutable shared_ptr<WPS4GraphInternal::State> m_state;

	//! the ascii file
	libwps::DebugFile &m_asciiFile;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */

