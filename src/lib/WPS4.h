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

#ifndef WPS4_H
#define WPS4_H

#include <vector>

#include <libwpd-stream/WPXStream.h>
#include "libwps_internal.h"

#include "WPSParser.h"

class WPXString;
class WPSContentListener;
class WPSEntry;
class WPSPosition;
class WPSPageSpan;

namespace WPS4ParserInternal
{
class SubDocument;
struct State;
}

class WPS4Graph;
class WPS4Text;

/**
 * This class parses Works version 2 through 4.
 *
 */
class WPS4Parser : public WPSParser
{
	friend class WPS4ParserInternal::SubDocument;
	friend class WPS4Graph;
	friend class WPS4Text;

public:
	//! constructor
	WPS4Parser(WPXInputStreamPtr &input, WPSHeaderPtr &header);
	//! destructor
	~WPS4Parser();
	//! called by WPSDocument to parse the file
	void parse(WPXDocumentInterface *documentInterface);
protected:
	//! color
	bool getColor(int id, uint32_t &color) const;

	//! returns the file size (or the ole zone size)
	long getSizeFile() const;
	//! sets the file size ( filled by WPS4Text )
	void setSizeFile(long sz);
	//! return true if the pos is in the file, update the file size if need
	bool checkInFile(long pos);

	//! adds a new page
	void newPage(int number);
	//! set the listener
	void setListener(shared_ptr<WPSContentListener> listener);

	/** tries to parse the main zone, ... */
	bool createStructures();
	/** tries to parse the different OLE zones ( except the main zone ) */
	bool createOLEStructures();
	/** creates the main listener */
	shared_ptr<WPSContentListener> createListener(WPXDocumentInterface *interface);

	// interface with text parser

	//! returns the page height, ie. paper size less margin (in inches)
	float pageHeight() const;
	//! returns the page width, ie. paper size less margin (in inches)
	float pageWidth() const;
	//! returns the number of columns
	int numColumns() const;

	/** creates a document for a comment entry and then send the data
	 *
	 * \note actually all bookmarks (comments) are empty */
	void createDocument(WPSEntry const &entry, libwps::SubDocumentType type);
	/** creates a document for a footnote entry with label and then send the data*/
	void createNote(WPSEntry const &entry, WPXString const &label);
	//! creates a textbox and then send the data
	void createTextBox(WPSEntry const &entry, WPSPosition const &pos, WPXPropertyList &extras);
	//! sends text corresponding to the entry to the listener (via WPS4Text)
	void send(WPSEntry const &entry, libwps::SubDocumentType type);

	// interface with graph parser

	/** tries to read a picture ( via its WPS4Graph )
	 *
	 * \note returns the object id or -1 if find nothing */
	int readObject(WPXInputStreamPtr input, WPSEntry const &entry);

	/** sends an object with given id ( via its WPS4Graph )
	 *
	 * The object is sent as a character with given size \a size */
	void sendObject(Vec2f const &size, int id);

	//
	// low level
	//

	/** finds the different zones (text, print, ...) and updates nameMultiMap */
	bool findZones();

	/** parses an entry
	 *
	 * reads a zone offset and a zone size and checks if this entry is valid */
	bool parseEntry(std::string const &name);

	/** tries to read the document dimension */
	bool readDocDim();

	/// tries to read the printer information
	bool readPrnt(WPSEntry const &entry);

	/** reads the additional windows info

		\note this zone contains many unknown data
	 */
	bool readDocWindowsInfo(WPSEntry const &entry);

	shared_ptr<WPSContentListener> m_listener; /* the listener (if set)*/
	//! the graph parser
	shared_ptr<WPS4Graph> m_graphParser;
	//! the text parser
	shared_ptr<WPS4Text> m_textParser;
	//! the internal state
	shared_ptr<WPS4ParserInternal::State> m_state;
};

#endif /* WPS4_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
