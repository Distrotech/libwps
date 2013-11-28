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

#ifndef WPS4_TEXT_H
#define WPS4_TEXT_H

#include <vector>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSEntry.h"
#include "WPSDebug.h"
#include "WPSTextParser.h"

class WPS4Parser;
namespace WPS4TextInternal
{
struct Font;
struct Paragraph;
struct State;
}

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
class WPS4Text : public WPSTextParser
{
	friend class WPS4Parser;

public:
	//! contructor
	WPS4Text(WPS4Parser &parser, WPXInputStreamPtr &input);

	//! destructor
	~WPS4Text();

	//! sets the listener
	void setListener(WPSContentListenerPtr &listen)
	{
		m_listener = listen;
	}

	//! returns the number of pages
	int numPages() const;

	//! sends the data which have not yet been sent to the listener
	void flushExtra();

protected:
	//! return the main parser
	WPS4Parser &mainParser()
	{
		return reinterpret_cast<WPS4Parser &>(m_mainParser);
	}
	//! return the main parser
	WPS4Parser const &mainParser() const
	{
		return reinterpret_cast<WPS4Parser const &>(m_mainParser);
	}

	//! returns the default codepage to use for the document
	libwps_tools_win::Font::Type getDefaultFontType() const;

	//! returns the header entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getHeaderEntry() const;

	//! returns the footer entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getFooterEntry() const;

	//! returns the main text entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getMainTextEntry() const;

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
	bool defDataParser(long bot, long eot, int id, long endPos, std::string &mess);

	//! reads the font names
	bool readFontNames(WPSEntry const &entry);

	//! reads a font properties
	bool readFont(long endPos, int &id, std::string &mess);

	//! reads a paragraph properties
	bool readParagraph(long endPos, int &id, std::string &mess);

	/** reads the ZZDLink ( a list of filename ) */
	bool readDosLink(WPSEntry const &entry);

	//! reads a object properties ( position in text, size and definition in file)
	bool objectDataParser(long bot, long eot, int id,
	                      long endPos, std::string &mess);

	//! reads the footnotes positions and definitions ( zones FTNd and FTNp)
	bool readFootNotes(WPSEntry const &ftnD, WPSEntry const &ftnP);

	//! reads a book mark property ( string)
	bool footNotesDataParser(long bot, long eot, int id, long endPos, std::string &mess);

	//! reads a book mark property ( string)
	bool bkmkDataParser(long bot, long eot, int id, long endPos, std::string &mess);

	//! reads a date time property
	bool dttmDataParser(long bot, long eot, int id, long endPos, std::string &mess);

protected:
	//! the listener
	WPSContentListenerPtr m_listener;

	//! the internal state
	mutable shared_ptr<WPS4TextInternal::State> m_state;
};
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
