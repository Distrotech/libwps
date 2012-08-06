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

#ifndef WPS4_H
#define WPS4_H

#include <vector>

#include <libwpd-stream/WPXStream.h>
#include "libwps_internal.h"

#include "WPSParser.h"

class WPXString;
class WPSContentListener;
typedef WPSContentListener WPS4ContentListener;
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
	void setListener(shared_ptr<WPS4ContentListener> listener);

	/** tries to parse the main zone, ... */
	bool createStructures();
	/** tries to parse the different OLE zones ( except the main zone ) */
	bool createOLEStructures();
	/** creates the main listener */
	shared_ptr<WPS4ContentListener> createListener(WPXDocumentInterface *interface);

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
	//! sends text corresponding to the entry to the listener (via WPS4MNText)
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

	shared_ptr<WPS4ContentListener> m_listener; /* the listener (if set)*/
	//! the graph parser
	shared_ptr<WPS4Graph> m_graphParser;
	//! the text parser
	shared_ptr<WPS4Text> m_textParser;
	//! the internal state
	shared_ptr<WPS4ParserInternal::State> m_state;
};

#endif /* WPS4_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
