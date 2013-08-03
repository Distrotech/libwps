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

#ifndef WPS8_GRAPH
#  define WPS8_GRAPH

#include <list>
#include <vector>

#include "libwps_internal.h"

#include "WPSDebug.h"

class WPXBinaryData;

class WPS8Parser;

namespace WPS8GraphInternal
{
struct State;
}

/** \brief the main class to read/store pictures in a Pc MS Works document v5-8
 *
 * This class must be associated with a WPS8Parser. It contains code to read
 * the BDR/WBDR, PICT/MEF4, IBGF entries and to store the pictures which are found
 * in the other ole parts.
 *
 * \note As the pictures seems always be given with characters positions, many functions
 * which exists to maintain the same structures that exist in the other WPS*MNGraph classes
 * do almost nothing.
 */
class WPS8Graph
{
	friend class WPS8Parser;
public:
	//! constructor
	WPS8Graph(WPS8Parser &parser);

	//! destructor
	~WPS8Graph();

	//! sets the listener
	void setListener(WPSContentListenerPtr &listen)
	{
		m_listener = listen;
	}

	/** computes the final position of all found figures.
	 *
	 * In reality, as all the pictures seemed to be given with characters positions,
	 * it does nothing*/
	void computePositions() const;

	//! returns the number page where we find a picture. In practice, 0/1
	int numPages() const;

	/** sends an object
	 * \param pos the object position in the document
	 * \param id the object identificator
	 * \param ole indicated if we look for objects coming from a ole zone or objects coming from a Pict zone */
	bool sendObject(WPSPosition const &pos, int id, bool ole);

	//! sends data corresponding to a ibgf entry on a given \a pos position
	bool sendIBGF(WPSPosition const &pos, int ibgfId);

	/** send all the objects of a given page:
	 * \param page if page < 0, sends all the pictures which have not been used,
	 * \param pageToIgnore pictures on this pages are not send
	 *
	 * As all the pictures seemed to be given with characters positions, this
	 * function only does something if page < 0.
	 */
	void sendObjects(int page, int pageToIgnore=-2);

protected:
	//! returns the file version
	int version() const;

	/** sends the border frames.
	 *
	 * Actually, sends the eight consecutive pictures which form a border on 3 consecutive lines*/
	void sendBorder(int borderId);

	//! adds a list of objects with given ids in the ole lists
	void storeObjects(std::vector<WPXBinaryData> const &objects,
	                  std::vector<int> const &ids,
	                  std::vector<WPSPosition> const &positions);

	//! finds all entries which correspond to some pictures, parses them and stores data
	bool readStructures(WPXInputStreamPtr input);

	// low level

	/** reads a PICT/MEF4 entry :  reads uncompressed picture of sx*sy of rgb
	 *
	 * This kind of entry seems mainly used to store a background picture */
	bool readPICT(WPXInputStreamPtr input, WPSEntry const &entry);

	/** reads a IBGF zone: an entry to a background picture
	 *
	 * This small entry seems to contain only an identificator which pointed to a PICT Zone
	 */
	bool readIBGF(WPXInputStreamPtr input, WPSEntry const &entry);

	//! parsed BDR/WBDR zone: a complex border formed with 8 pictures
	bool readBDR(WPXInputStreamPtr input, WPSEntry const &entry);

	/** \brief reads METAFILE/CODE
	 *
	 * \warning we must probably also recognize the enhanced metafile format: EMF */
	bool readMetaFile(WPXInputStreamPtr input, long endPos, WPXBinaryData &pict);

	//! returns the debug file
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}
private:
	WPS8Graph(WPS8Graph const &orig);
	WPS8Graph &operator=(WPS8Graph const &orig);

protected:
	//! the listener
	WPSContentListenerPtr m_listener;

	//! the main parser
	WPS8Parser &m_mainParser;

	//! the state
	mutable shared_ptr<WPS8GraphInternal::State> m_state;

	//! the ascii file
	libwps::DebugFile &m_asciiFile;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
