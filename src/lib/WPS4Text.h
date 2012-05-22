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

#ifndef WPS4_TEXT_H
#define WPS4_TEXT_H

#include <ostream>
#include <vector>

#include "WPSEntry.h"
#include "WPSDebug.h"

class WPS4Parser;
namespace WPS4TextInternal
{
struct DataFOD;
struct Font;
struct Paragraph;
struct State;
}

typedef class WPSContentListener WPS4ContentListener;
typedef shared_ptr<WPS4ContentListener> WPS4ContentListenerPtr;

/** The class which parses text zones in a pc MS Works document v1-4
 *
 * This class must be associated with a WPS4Parser. It finds and reads:
 * - TEXT[3] : the text limits ( header, footer, main text with notes)
 * - SHdr, SFtr : a string to store header/footer in v1-2 (?)
 * - BTEC : the fonts properties
 * - BTEP : the paragraph properties
 * - FONT : the font names
 * - FTNp, FTNd : the footnote positions (text position and text of notes)
 * - BKMK : a comment field ( contain a string )
 * - CHRT : a chart ( unknown format )
 * It reads:
 * - DTTM : field contents ( only parsed)
 * - EOBJ : the text position with the position and size of an object
 *
 * \note It also reads the size of the document because this size is stored
 * between the "entries" which defines the text positions and the BTEC
 * positions...
 */
class WPS4Text
{
	friend class WPS4Parser;

public:
	//! contructor
	WPS4Text(WPS4Parser &parser, WPXInputStreamPtr &input);

	//! destructor
	~WPS4Text();

	//! sets the listener
	void setListener(WPS4ContentListenerPtr &listen)
	{
		m_listener = listen;
	}

	//! returns the number of pages
	int numPages() const;


	//! sends the data which have not yet been sent to the listener
	void flushExtra();

protected:
	//! returns the header entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getHeaderEntry() const;

	//! returns the footer entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getFooterEntry() const;

	//! returns the main text entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getMainTextEntry() const;

	//! returns the text position
	WPSEntry getAllTextEntry() const;

	//! reads a text section and sends it to a listener
	bool readText(WPSEntry const &entry);

	//! finds all text entries (TEXT, SHdr, SFtr, BTEC, BTEP, FTNp, FTNp, BKMK, FONT, CHRT)
	bool readEntries();
	//! parsed all the text entries
	bool readStructures();

protected:
	//----------------------------------------
	// FDP parsing
	//----------------------------------------

	//! finds the FDPC/FDPP structure using the BTEC/BTEP entries
	//! \param which == 0 means FDPP
	bool findFDPStructures(int which);
	//! finds the FDPC/FDPP structure by searching after the text zone
	//! \param which == 0 means FDPP
	bool findFDPStructuresByHand(int which);

	//----------------------------------------
	// PLC parsing, setting
	//----------------------------------------

	/** callback when a new attribute is found in an FDPP/FDPC entry
	 *
	 * \return true and filled id if this attribute can be parsed
	 * \note mess can be filled to add a message in debugFile */
	typedef bool (WPS4Text::* FDPParser) (long endPos, int &id, std::string &mess);

	/** parses a FDPP or a FDPC entry (which contains a list of ATTR_TEXT/ATTR_PARAG
	 * with their definition ) and adds found data in listFODs
	 *
	 * this data are stored similarly in Mac v4 and all PC version
	 * \note only their contents definition differs */
	bool readFDP(WPSEntry const &entry,
	             std::vector<WPS4TextInternal::DataFOD> &fods, FDPParser parser);

	/** definition of the plc data parser (low level)
	 *
	 * \param input the file's input
	 * \param endPos the end of the properties' definition,
	 * \param bot, \param eot defined the text zone corresponding to these properties
	 * \param id the number of this properties
	 * \param mess a string which can be filled to indicate unparsed data */
	typedef bool (WPS4Text::* DataParser)
	(long bot, long eot, int id, long endPos, std::string &mess);

	/** reads a PLC (Pointer List Composant ?) in zone entry
	 *
	 * \param zone the zone of the data in the file,
	 * \param textPtrs lists of offset in text zones where properties changes
	 * \param listValues lists of properties values (filled only if values are simple types: int, ..)
	 * \param parser the parser to use to read the values */
	bool readPLC(WPSEntry const &zone,
	             std::vector<long> &textPtrs, std::vector<long> &listValues,
	             DataParser parser = 0L);

	//! default plc reader
	bool defDataParser (long bot, long eot, int id, long endPos, std::string &mess);

	//! reads the font names
	bool readFontNames(WPSEntry const &entry);

	//! reads a font properties
	bool readFont(long endPos, int &id, std::string &mess);
	/** sends a font to the listener
	 *
	 * \param font the font's properties */
	void setProperty(WPS4TextInternal::Font const &font);

	//! reads a paragraph properties
	bool readParagraph(long endPos, int &id, std::string &mess);
	/** sends a paragraph properties to the listener
	 *
	 * \param para the paragraph's properties */
	void setProperty(WPS4TextInternal::Paragraph const &para);

	/** reads the ZZDLink ( a list of filename ) */
	bool readDosLink(WPSEntry const &entry);

	//! reads a object properties ( position in text, size and definition in file)
	bool objectDataParser (long bot, long eot, int id,
	                       long endPos, std::string &mess);

	//! reads the footnotes positions and definitions ( zones FTNd and FTNp)
	bool readFootNotes(WPSEntry const &ftnD, WPSEntry const &ftnP);

	//! reads a book mark property ( string)
	bool footNotesDataParser (long bot, long eot, int id, long endPos, std::string &mess);

	//! reads a book mark property ( string)
	bool bkmkDataParser (long bot, long eot, int id, long endPos, std::string &mess);

	//! reads a date time property
	bool dttmDataParser (long bot, long eot, int id, long endPos, std::string &mess);

protected:
	//! returns the file version : 1-4
	int version() const;

	//! returns the debug file
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}
protected:
	//! the main input
	WPXInputStreamPtr m_input;

	//! the main parser
	WPS4Parser &m_mainParser;

	//! the listener
	WPS4ContentListenerPtr m_listener;

	//! the internal state
	mutable shared_ptr<WPS4TextInternal::State> m_state;

	//! the ascii file
	libwps::DebugFile &m_asciiFile;
};
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
