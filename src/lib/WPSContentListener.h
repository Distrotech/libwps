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
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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

#ifndef WPSCONTENTLISTENER_H
#define WPSCONTENTLISTENER_H

#include <vector>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"

#include "WPSListener.h"

class WPSList;
class WPSPageSpan;
struct WPSParagraph;
struct WPSTabStop;

struct WPSContentParsingState;
struct WPSDocumentParsingState;

class WPSContentListener : public WPSListener
{
public:
	WPSContentListener(std::vector<WPSPageSpan> const &pageList, librevenge::RVNGTextInterface *documentInterface);
	virtual ~WPSContentListener();

	void setDocumentLanguage(int lcid);

	void startDocument();
	void endDocument();
	void handleSubDocument(WPSSubDocumentPtr &subDocument, libwps::SubDocumentType subDocumentType);
	bool isHeaderFooterOpened() const;

	// ------ text data -----------

	//! adds a basic character, ..
	void insertCharacter(uint8_t character);
	/** adds an unicode character
	 *
	 * by convention if \a character=0xfffd(undef), no character is added */
	void insertUnicode(uint32_t character);
	//! adds a unicode string
	void insertUnicodeString(librevenge::RVNGString const &str);

	void insertTab();
	void insertEOL(bool softBreak=false);
	void insertBreak(const uint8_t breakType);

	// ------ text format -----------
	//! set the actual font
	void setFont(const WPSFont &font);
	//! returns the actual font
	WPSFont const &getFont() const;

	// ------ paragraph format -----------
	//! returns true if a paragraph or a list is opened
	bool isParagraphOpened() const;
	//! sets the actual paragraph
	void setParagraph(const WPSParagraph &para);
	//! returns the actual paragraph
	WPSParagraph const &getParagraph() const;

	// ------ list format -----------
	/** function to set the actual list */
	void setCurrentList(shared_ptr<WPSList> list);
	/** returns the current list */
	shared_ptr<WPSList> getCurrentList() const;

	// ------- fields ----------------
	//! adds a field type
	void insertField(FieldType type);
	//! insert a date/time field with given format (see strftime)
	void insertDateTimeField(char const *format);

	// ------- subdocument -----------------
	/** defines the footnote type */
	enum NoteType { FOOTNOTE, ENDNOTE };
	/** adds note */
	void insertNote(const NoteType noteType, WPSSubDocumentPtr &subDocument);
	/** adds a label note */
	void insertLabelNote(const NoteType noteType, librevenge::RVNGString const &label, WPSSubDocumentPtr &subDocument);
	/** adds comment */
	void insertComment(WPSSubDocumentPtr &subDocument);

	/** adds a picture in given position */
	void insertPicture(WPSPosition const &pos, const librevenge::RVNGBinaryData &binaryData,
	                   std::string type="image/pict",
	                   librevenge::RVNGPropertyList frameExtras=librevenge::RVNGPropertyList());
	/** adds a textbox in given position */
	void insertTextBox(WPSPosition const &pos, WPSSubDocumentPtr subDocument,
	                   librevenge::RVNGPropertyList frameExtras=librevenge::RVNGPropertyList());


	// ------- table -----------------
	/** open a table*/
	void openTable(std::vector<float> const &colWidth, librevenge::RVNGUnit unit);
	/** closes this table */
	void closeTable();
	/** open a row with given height. If h<0, use min-row-heigth */
	void openTableRow(float h, librevenge::RVNGUnit unit, bool headerRow=false);
	/** closes this row */
	void closeTableRow();
	/** low level function to define a cell.
		\param cell the cell position, alignement, ...
		\param extras to be used to pass extra data, for instance spreadsheet data*/
	void openTableCell(WPSCell const &cell, librevenge::RVNGPropertyList const &extras=librevenge::RVNGPropertyList());
	/** close a cell */
	void closeTableCell();
	/** add empty cell */
	void addEmptyTableCell(Vec2i const &pos, Vec2i span=Vec2i(1,1));

	// ------- section ---------------
	//! returns true if a section is opened
	bool isSectionOpened() const;
	//! returns the actual number of columns ( or 1 if no section is opened )
	int getSectionNumColumns() const;
	//! open a section if possible
	bool openSection(std::vector<int> colsWidth=std::vector<int>(), librevenge::RVNGUnit unit=librevenge::RVNG_INCH);
	//! close a section
	bool closeSection();

protected:
	void _openSection();
	void _closeSection();

	void _openPageSpan();
	void _closePageSpan();
	void _updatePageSpanDependent(bool set);

	void _startSubDocument();
	void _endSubDocument();

	void _handleFrameParameters( librevenge::RVNGPropertyList &propList, WPSPosition const &pos);
	bool _openFrame(WPSPosition const &pos, librevenge::RVNGPropertyList extras=librevenge::RVNGPropertyList());
	void _closeFrame();

	void _openParagraph();
	void _closeParagraph();
	void _appendParagraphProperties(librevenge::RVNGPropertyList &propList, const bool isListElement=false);
	void _resetParagraphState(const bool isListElement=false);

	void _openListElement();
	void _closeListElement();
	void _changeList();

	void _openSpan();
	void _closeSpan();

	void _flushText();
	void _flushDeferredTabs();

	void _insertBreakIfNecessary(librevenge::RVNGPropertyList &propList);

	/** creates a new parsing state (copy of the actual state)
	 *
	 * \return the old one */
	shared_ptr<WPSContentParsingState> _pushParsingState();
	//! resets the previous parsing state
	void _popParsingState();

protected:
	shared_ptr<WPSDocumentParsingState> m_ds; // main parse state
	shared_ptr<WPSContentParsingState> m_ps; // parse state
	std::vector<shared_ptr<WPSContentParsingState> > m_psStack;
	librevenge::RVNGTextInterface *m_documentInterface;

private:
	WPSContentListener(const WPSContentListener &);
	WPSContentListener &operator=(const WPSContentListener &);
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
