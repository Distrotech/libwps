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
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 */

#ifndef WPS8_H
#define WPS8_H

#include <vector>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"
#include "WPSParser.h"

class WPSPageSpan;

namespace WPS8ParserInternal
{
class SubDocument;
struct State;
}

class WPS8Graph;
class WPS8Table;
class WPS8Text;

/**
 * This class parses Works version 2000 through 8.
 *
 */
class WPS8Parser : public WPSParser
{
	friend class WPS8ParserInternal::SubDocument;
	friend class WPS8Graph;
	friend class WPS8Table;
	friend class WPS8Text;

public:
	//! constructor
	WPS8Parser(RVNGInputStreamPtr &input, WPSHeaderPtr &header);
	//! destructor
	~WPS8Parser();
	//! called by WPSDocument to parse the file
	void parse(librevenge::RVNGTextInterface *documentInterface);
protected:
	//! return true if the pos is in the file, update the file size if need
	bool checkInFile(long pos);

	//! adds a new page
	void newPage(int number);
	//! set the listener
	void setListener(shared_ptr<WPSContentListener> listener);
	/** creates the main listener */
	shared_ptr<WPSContentListener> createListener(librevenge::RVNGTextInterface *interface);

	/** tries to parse the main zone, ... */
	bool createStructures();
	/** tries to parse the different OLE zones ( except the main zone ) */
	bool createOLEStructures();

	//! returns the page height, ie. paper size less margin (in inches)
	float pageHeight() const;
	//! returns the page width, ie. paper size less margin (in inches)
	float pageWidth() const;
	//! returns the number of columns
	int numColumns() const;

	//! send the frames which corresponds to a given page to the listener
	void sendPageFrames();
	// interface with text parser

	//! creates a subdocument to send a textbox which correspond to the strsid text zone
	void sendTextBox(WPSPosition const &pos, int strsid,
	                 librevenge::RVNGPropertyList frameExtras=librevenge::RVNGPropertyList());

	//! sends text corresponding to the entry to the listener (via WPS8Text)
	void send(WPSEntry const &entry);
	//! sends text corresponding to the strsId to the listener (via WPS8Text)
	void send(int strsId);

	//! send the text of a cell to a listener (via WPS8Text)
	void sendTextInCell(int strsId, int cellId);

	// interface with table parser

	//! sends a table as a character with given size ( via its WPS8Table )
	bool sendTable(Vec2f const &size, int objectId);
	//! retrieve the strsId corresponding to a table ( mainly for debug)
	int getTableSTRSId(int tableId) const;

	// interface with graph parser

	/** sends an object as a character with given size (via its WPS8Graph )
	 *
	 * \param size the size of the object in the document pages
	 * \param objectId the object identificator
	 * \param ole indicated if we look for objects coming from a ole zone or objects coming from a Pict zone */
	bool sendObject(Vec2f const &size, int objectId, bool ole);

	//
	// low level
	//

	//! parses an index entry
	bool parseHeaderIndexEntry();
	/** function which is called, if some data remains after the basic content
		of an entry: by default does nothing */
	bool parseHeaderIndexEntryEnd(long endPos, WPSEntry &hie,std::string &mess);

	/** tries to find the beginning of the list of indices,
	 * then try to find all entries in this list.
	 *
	 * Stores result in nameTable, offsetTable */
	bool parseHeaderIndex();

	/** \brief reads the DOP zone: the document properties
	 *
	 * Contains the page dimension, the background picture, ... */
	bool readDocProperties(WPSEntry const &entry, WPSPageSpan &page);

	/** \brief reads the FRAM zone: ie a zone which can contains textbox, picture, ...
		and have some borders */
	bool readFRAM(WPSEntry const &entry);

	/** \brief parses a SYID zone
	 *
	 * an unknown zone which seems to have the same number of entries
	 * than the text zones (STRS) */
	bool readSYID(WPSEntry const &entry, std::vector<int> &listId);

	/** \brief parses the WNPR zone : a zone which seems to contain the printer preferences.
	 *
	 * \warning: read data are not used */
	bool readWNPR(WPSEntry const &entry);

	//! finds the structures of the Ole zone "SPELLING"
	bool readSPELLING(RVNGInputStreamPtr input, std::string const &oleName);

	shared_ptr<WPSContentListener> m_listener; /* the listener (if set)*/
	//! the graph parser
	shared_ptr<WPS8Graph> m_graphParser;
	//! the table parser
	shared_ptr<WPS8Table> m_tableParser;
	//! the text parser
	shared_ptr<WPS8Text> m_textParser;
	//! the internal state
	shared_ptr<WPS8ParserInternal::State> m_state;
};

#endif /* WPS8_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
