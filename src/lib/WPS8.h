/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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
 */

#ifndef WPS8_H
#define WPS8_H

#include <vector>

#include <libwpd-stream/WPXStream.h>
#include "libwps_internal.h"

#include "WPSParser.h"

class WPXString;
class WPSContentListener;
typedef WPSContentListener WPS8ContentListener;
class WPSEntry;
class WPSPosition;
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
	WPS8Parser(WPXInputStreamPtr &input, WPSHeaderPtr &header);
	//! destructor
	~WPS8Parser();
	//! called by WPSDocument to parse the file
	void parse(WPXDocumentInterface *documentInterface);
protected:
	//! return true if the pos is in the file, update the file size if need
	bool checkInFile(long pos);

	//! adds a new page
	void newPage(int number);
	//! set the listener
	void setListener(shared_ptr<WPS8ContentListener> listener);
	/** creates the main listener */
	shared_ptr<WPS8ContentListener> createListener(WPXDocumentInterface *interface);

	/** tries to parse the main zone, ... */
	bool createStructures();
	/** tries to parse the different OLE zones ( except the main zone ) */
	bool createOLEStructures();

	// interface with text parser

	//! returns the page height, ie. paper size less margin (in inches)
	float pageHeight() const;
	//! returns the page width, ie. paper size less margin (in inches)
	float pageWidth() const;
	//! creates a subdocument to send a textbox which correspond to the strsid text zone
	void sendTextBox(WPSPosition const &/*pos*/, int /*strsid*/) {}

	// interface with graph parser

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

	/** \brief parses a SGP zone
	 *
	 * an unknown zone which seems to have the contains some size
	 * \note the four SGP, STRS, STSH, SYID zones seem to exist in all files
	 */
	bool readSGP(WPSEntry const &entry);

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
	bool readSPELLING(WPXInputStreamPtr input, std::string const &oleName);

	shared_ptr<WPS8ContentListener> m_listener; /* the listener (if set)*/
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
