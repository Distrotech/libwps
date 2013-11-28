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

#ifndef WPS8_TEXT_H
#define WPS8_TEXT_H

#include <vector>

#include "libwps_internal.h"

#include "WPSDebug.h"

#include "WPSTextParser.h"

namespace WPS8Struct
{
struct FileData;
}

namespace WPS8TextInternal
{
struct State;
class SubDocument;
}

class WPS8Parser;
class WPS8TextStyle;

class WPS8Text : public WPSTextParser
{
	friend class WPS8TextInternal::SubDocument;
	friend class WPS8Parser;
	friend class WPS8TextStyle;
public:
	WPS8Text(WPS8Parser &parser);
	~WPS8Text();

	//! sets the listener
	void setListener(WPSContentListenerPtr &listen);

	//! returns the number of pages
	int numPages() const;

	//! sends the data which have not yet been sent to the listener
	void flushExtra();

	//! finds all entries which correspond to the text data, parses them and stores data
	bool readStructures();

	//! returns the number of different text zones
	int getNumTextZones() const;

	/** returns the type of a text zone
	 *
	 * 1: mainzone, 2: footnote, 3: endnote, 4: ???, 5: text in table/textbox
	 * 6: header, 7: footer
	 **/
	int getTextZoneType(int typeId) const;

	//! returns the header entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getHeaderEntry() const;

	//! returns the footer entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getFooterEntry() const;

	//! returns the main zone entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getTextEntry() const;

	//! returns ith zone entry (if such entry exists, if not returns an invalid entry)
	WPSEntry getEntry(int strsId) const;

	//! reads a text section and sends it to a listener
	void readText(WPSEntry const &entry);

	//! reads a cell section and sends it to a listener
	void readTextInCell(int strsId, int cellId);

protected:
	//! return the main parser
	WPS8Parser &mainParser()
	{
		return reinterpret_cast<WPS8Parser &>(m_mainParser);
	}
	//! return the main parser
	WPS8Parser const &mainParser() const
	{
		return reinterpret_cast<WPS8Parser const &>(m_mainParser);
	}

	//
	// interface with WPS8TextStyle
	//
	//! reads a font properties
	bool readFont(long endPos, int &id, std::string &mess);

	//! the paragraph
	bool readParagraph(long endPos, int &id, std::string &mess);

	//
	// String+text functions
	//
	//! reads a string
	bool readString(WPXInputStreamPtr input, long page_size,
	                WPXString &res);
	//! reads a utf16 character, \return 0xfffd if an error
	long readUTF16LE(WPXInputStreamPtr input, long endPos, uint16_t firstC);

	/** \brief the footnote ( FTN or EDN )
	 *
	 * \note this function must be called after the creation of the text zones */
	bool readNotes(WPSEntry const &entry);

	/** \brief creates the notes association : text and notes positions
	 *
	 * \note must be called after all notes have been created */
	void createNotesCorrespondance();

	//----------------------------------------
	// PLC parsing, setting
	//----------------------------------------

	/** definition of the plc data parser (low level)
	 *
	 * \param endPos the end of the properties' definition,
	 * \param bot, \param eot defined the text zone corresponding to these properties
	 * \param id the number of this properties
	 * \param mess a string which can be filled to indicate unparsed data */
	typedef bool (WPS8Text::* DataParser)
	(long bot, long eot, int id, WPS8Struct::FileData const &data,
	 std::string &mess);
	/** definition of the last part of plc data parser (low level)
	 *
	 * \param endPos the end of the properties' definition,
	 * \param textPtrs the list of text positions */
	typedef bool (WPS8Text::* EndDataParser)
	(long endPos, std::vector<long> const &textPtrs);
	/** reads a PLC (Pointer List Composant ?) in zone entry
	 *
	 * \param entry the file zone
	 * \param textPtrs lists of offset in text zones where properties changes
	 * \param listValues lists of properties values (filled only if values are simple types: int, ..)
	 * \param parser the parser to use to read the values
	 * \param endParser the parser to use to read remaining data */
	bool readPLC(WPSEntry const &entry,
	             std::vector<long> &textPtrs, std::vector<long> &listValues,
	             DataParser parser = &WPS8Text::defDataParser,
	             EndDataParser endParser = 0L);
	//! default parser
	bool defDataParser
	(long , long , int , WPS8Struct::FileData const &data, std::string &mess);
	//! the text zones parser: STRS
	bool textZonesDataParser(long bot, long eot, int nId,
	                         WPS8Struct::FileData const &data,
	                         std::string &mess);
	// object
	//! reads a EOBJ properties: an object id and its size, ...
	bool objectDataParser(long bot, long eot, int id,
	                      WPS8Struct::FileData const &data, std::string &mess);
	// field type
	//! reads a field type : TOKN zone
	bool tokenEndDataParser(long endPage, std::vector<long> const &textPtrs);
	/** \brief reads a field type : BMKT zone
		\warning the read data are NOT used*/
	bool bmktEndDataParser(long endPage, std::vector<long> const &textPtrs);

protected:
	//! the listener
	WPSContentListenerPtr m_listener;
	//! the graph parser
	shared_ptr<WPS8TextStyle> m_styleParser;
	//! the internal state
	mutable shared_ptr<WPS8TextInternal::State> m_state;
protected:
};


#endif /* WPS8_TEXT_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
