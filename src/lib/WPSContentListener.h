/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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
 * For further information visit http://libwps.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef WPSCONTENTLISTENER_H
#define WPSCONTENTLISTENER_H

#include <vector>

#include <libwpd/WPXPropertyList.h>

#include "libwps_internal.h"

class WPXBinaryData;
class WPXDocumentInterface;
class WPXString;
class WPXPropertyListVector;

class WPSCell;
class WPSList;
class WPSPageSpan;
class WPSPosition;
class WPSSubDocument;
struct WPSTabStop;

typedef shared_ptr<WPSSubDocument> WPSSubDocumentPtr;

struct WPSDocumentParsingState
{
	WPSDocumentParsingState(std::vector<WPSPageSpan> const &pageList);
	~WPSDocumentParsingState();

	std::vector<WPSPageSpan> m_pageList;
	WPXPropertyList m_metaData;

	int m_footNoteNumber /** footnote number*/, m_endNoteNumber /** endnote number*/;
	int m_newListId; // a new free id

	bool m_isDocumentStarted, m_isHeaderFooterStarted;
	std::vector<WPSSubDocumentPtr> m_subDocuments; /** list of document actually open */

private:
	WPSDocumentParsingState(const WPSDocumentParsingState &);
	WPSDocumentParsingState &operator=(const WPSDocumentParsingState &);
};

struct WPSContentParsingState
{
	WPSContentParsingState();
	~WPSContentParsingState();

	WPXString m_textBuffer;
	int m_numDeferredTabs;

	uint32_t m_textAttributeBits;
	double m_fontSize;
	WPXString m_fontName;
	uint32_t m_fontColor;
	int m_textLanguage;

	bool m_isParagraphColumnBreak;
	bool m_isParagraphPageBreak;
	libwps::Justification m_paragraphJustification;
	double m_paragraphLineSpacing;
	WPXUnit m_paragraphLineSpacingUnit;
	uint32_t m_paragraphBackgroundColor;
	int m_paragraphBorders;
	libwps::BorderStyle m_paragraphBordersStyle;
	int m_paragraphBordersWidth;
	uint32_t m_paragraphBordersColor;

	shared_ptr<WPSList> m_list;
	uint8_t m_currentListLevel;

	bool m_isPageSpanOpened;
	bool m_isSectionOpened;
	bool m_isFrameOpened;
	bool m_isPageSpanBreakDeferred;
	bool m_isHeaderFooterWithoutParagraph;

	bool m_isSpanOpened;
	bool m_isParagraphOpened;
	bool m_isListElementOpened;

	bool m_firstParagraphInPageSpan;

	std::vector<unsigned int> m_numRowsToSkip;
	bool m_isTableOpened;
	bool m_isTableRowOpened;
	bool m_isTableColumnOpened;
	bool m_isTableCellOpened;

	unsigned m_currentPage;
	int m_numPagesRemainingInSpan;
	int m_currentPageNumber;

	bool m_sectionAttributesChanged;
	int m_numColumns;
	std::vector < WPSColumnDefinition > m_textColumns;
	bool m_isTextColumnWithoutParagraph;

	double m_pageFormLength;
	double m_pageFormWidth;
	bool m_pageFormOrientationIsPortrait;

	double m_pageMarginLeft;
	double m_pageMarginRight;
	double m_pageMarginTop;
	double m_pageMarginBottom;

	double m_sectionMarginLeft;  // In multicolumn sections, the above two will be rather interpreted
	double m_sectionMarginRight; // as section margin change
	double m_sectionMarginTop;
	double m_sectionMarginBottom;
	double m_paragraphMarginLeft;  // resulting paragraph margin that is one of the paragraph
	double m_paragraphMarginRight; // properties
	double m_paragraphMarginTop;
	WPXUnit m_paragraphMarginTopUnit;
	double m_paragraphMarginBottom;
	WPXUnit m_paragraphMarginBottomUnit;
	double m_leftMarginByPageMarginChange;  // part of the margin due to the PAGE margin change
	double m_rightMarginByPageMarginChange; // inside a page that already has content.
	double m_leftMarginByParagraphMarginChange;  // part of the margin due to the PARAGRAPH
	double m_rightMarginByParagraphMarginChange; // margin change (in WP6)
	double m_leftMarginByTabs;  // part of the margin due to the LEFT or LEFT/RIGHT Indent; the
	double m_rightMarginByTabs; // only part of the margin that is reset at the end of a paragraph

	double m_paragraphTextIndent; // resulting first line indent that is one of the paragraph properties
	double m_textIndentByParagraphIndentChange; // part of the indent due to the PARAGRAPH indent (WP6???)
	double m_textIndentByTabs; // part of the indent due to the "Back Tab" or "Left Tab"

	double m_listReferencePosition; // position from the left page margin of the list number/bullet
	double m_listBeginPosition; // position from the left page margin of the beginning of the list
	std::vector<bool> m_listOrderedLevels; //! a stack used to know what is open

	uint16_t m_alignmentCharacter;
	std::vector<WPSTabStop> m_tabStops;
	bool m_isTabPositionRelative;

	bool m_inSubDocument;

	bool m_isNote;
	libwps::SubDocumentType m_subDocumentType;

private:
	WPSContentParsingState(const WPSContentParsingState &);
	WPSContentParsingState &operator=(const WPSContentParsingState &);
};

class WPSContentListener
{
public:
	WPSContentListener(std::vector<WPSPageSpan> const &pageList, WPXDocumentInterface *documentInterface);
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
	void insertUnicodeString(WPXString const &str);
	//! adds an unicode character to a string ( with correct encoding ).
	static void appendUnicode(uint32_t val, WPXString &buffer);

	void insertTab();
	void insertEOL(bool softBreak=false);
	void insertBreak(const uint8_t breakType);

	// ------ text format -----------
	void setTextFont(const WPXString &fontName);
	void setFontSize(const uint16_t fontSize);
	void setFontAttributes(const uint32_t fontAttributes);
	void setTextLanguage(int lcid);
	void setTextColor(const uint32_t rgb);
	void setFont(const WPSFont &font);

	// ------ paragraph format -----------
	//! returns true if a paragraph or a list is opened
	bool isParagraphOpened() const;
	void setParagraphLineSpacing(const double lineSpacing, WPXUnit unit=WPX_PERCENT);
	/** Define the paragraph justification. You can set force=true to
		force a break if there is a justification change */
	void setParagraphJustification(libwps::Justification justification, bool force=false);
	//! sets the first paragraph text indent. \warning unit are given in inches
	void setParagraphTextIndent(double margin);
	/** sets the paragraph margin.
	 * \param margin is given in inches
	 * \param pos in WPS_LEFT, WPS_RIGHT, WPS_TOP, WPS_BOTTOM
	 */
	void setParagraphMargin(double margin, int pos);
	/** sets the tabulations.
	 * \param tabStops the tabulations
	 */
	void setTabs(const std::vector<WPSTabStop> &tabStops);
	/** sets the paragraph background color */
	void setParagraphBackgroundColor(uint32_t color=0xFFFFFF);
	/** indicates that the paragraph has a basic borders (ie. a black line)
	 * \param which = libwps::LeftBorderBit | ...
	 * \param style = libwps::BorderSingle | libwps::BorderDouble
	 * \param width = 1,2,3,...
	 * \param color: the border color
	 */
	void setParagraphBorders(int which, libwps::BorderStyle style=libwps::BorderSingle, int width=1, uint32_t color=0);

	// ------ list format -----------
	/** function to set the actual list */
	void setCurrentList(shared_ptr<WPSList> list);
	/** returns the current list */
	shared_ptr<WPSList> getCurrentList() const;
	/** function to set the level of the current list
	 * \warning minimal implementation...*/
	void setCurrentListLevel(int level);

	// ------- fields ----------------
	/** Defines some basic type for field */
	enum FieldType { None, PageNumber, Date, Time, Title, Link, Database };
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
	void insertLabelNote(const NoteType noteType, WPXString const &label, WPSSubDocumentPtr &subDocument);
	/** adds comment */
	void insertComment(WPSSubDocumentPtr &subDocument);

	/** adds a picture in given position */
	void insertPicture(WPSPosition const &pos, const WPXBinaryData &binaryData,
	                   std::string type="image/pict",
	                   WPXPropertyList frameExtras=WPXPropertyList());
	/** adds a textbox in given position */
	void insertTextBox(WPSPosition const &pos, WPSSubDocumentPtr subDocument,
	                   WPXPropertyList frameExtras=WPXPropertyList());


	// ------- table -----------------
	/** open a table*/
	void openTable(std::vector<float> const &colWidth, WPXUnit unit);
	/** closes this table */
	void closeTable();
	/** open a row with given height*/
	void openTableRow(float h, WPXUnit unit, bool headerRow=false);
	/** closes this row */
	void closeTableRow();
	/** low level function to define a cell.
		\param cell the cell position, alignement, ...
		\param extras to be used to pass extra data, for instance spreadsheet data*/
	void openTableCell(WPSCell const &cell, WPXPropertyList const &extras);
	/** close a cell */
	void closeTableCell();

	// ------- section ---------------
	//! returns true if a section is opened
	bool isSectionOpened() const;
	//! open a section if possible
	bool openSection(std::vector<int> colsWidth=std::vector<int>(), WPXUnit unit=WPX_INCH);
	//! close a section
	bool closeSection();

protected:
	void _openSection();
	void _closeSection();

	void _openPageSpan();
	void _closePageSpan();
	void _updatePageSpanDependent(bool set);
	void _recomputeParagraphPositions();

	void _startSubDocument();
	void _endSubDocument();

	void _handleFrameParameters( WPXPropertyList &propList, WPSPosition const &pos);
	bool _openFrame(WPSPosition const &pos, WPXPropertyList extras=WPXPropertyList());
	void _closeFrame();

	void _openParagraph();
	void _closeParagraph();
	void _appendParagraphProperties(WPXPropertyList &propList, const bool isListElement=false);
	void _getTabStops(WPXPropertyListVector &tabStops);
	void _appendJustification(WPXPropertyList &propList, libwps::Justification justification);
	void _resetParagraphState(const bool isListElement=false);

	void _openListElement();
	void _closeListElement();
	void _changeList();

	void _openSpan();
	void _closeSpan();

	void _flushText();
	void _flushDeferredTabs();

	void _insertBreakIfNecessary(WPXPropertyList &propList);

	static void _addLanguage(int lcid, WPXPropertyList &propList);

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
	WPXDocumentInterface *m_documentInterface;

private:
	WPSContentListener(const WPSContentListener &);
	WPSContentListener &operator=(const WPSContentListener &);
};

typedef shared_ptr<WPSContentListener> WPSContentListenerPtr;

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
