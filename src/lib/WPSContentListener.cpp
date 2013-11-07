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

#include <iomanip>
#include <sstream>
#include <stdio.h>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSContentListener.h"

#include "WPSCell.h"
#include "WPSFont.h"
#include "WPSList.h"
#include "WPSPageSpan.h"
#include "WPSParagraph.h"
#include "WPSPosition.h"
#include "WPSSubDocument.h"

////////////////////////////////////////////////////////////
//! the document state
struct WPSDocumentParsingState
{
	//! constructor
	WPSDocumentParsingState(std::vector<WPSPageSpan> const &pageList);
	//! destructor
	~WPSDocumentParsingState();

	std::vector<WPSPageSpan> m_pageList;
	librevenge::RVNGPropertyList m_metaData;

	int m_footNoteNumber /** footnote number*/, m_endNoteNumber /** endnote number*/;
	int m_newListId; // a new free id

	bool m_isDocumentStarted, m_isHeaderFooterStarted;
	std::vector<WPSSubDocumentPtr> m_subDocuments; /** list of document actually open */

private:
	WPSDocumentParsingState(const WPSDocumentParsingState &);
	WPSDocumentParsingState &operator=(const WPSDocumentParsingState &);
};

WPSDocumentParsingState::WPSDocumentParsingState(std::vector<WPSPageSpan> const &pageList) :
	m_pageList(pageList),
	m_metaData(),
	m_footNoteNumber(0), m_endNoteNumber(0), m_newListId(0),
	m_isDocumentStarted(false), m_isHeaderFooterStarted(false), m_subDocuments()
{
}

WPSDocumentParsingState::~WPSDocumentParsingState()
{
}

////////////////////////////////////////////////////////////
//! the content state
struct WPSContentParsingState
{
	WPSContentParsingState();
	~WPSContentParsingState();

	librevenge::RVNGString m_textBuffer;
	int m_numDeferredTabs;

	WPSFont m_font;
	WPSParagraph m_paragraph;
	shared_ptr<WPSList> m_list;

	bool m_isParagraphColumnBreak;
	bool m_isParagraphPageBreak;

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

	std::vector<bool> m_listOrderedLevels; //! a stack used to know what is open

	bool m_inSubDocument;

	bool m_isNote;
	libwps::SubDocumentType m_subDocumentType;

private:
	WPSContentParsingState(const WPSContentParsingState &);
	WPSContentParsingState &operator=(const WPSContentParsingState &);
};

WPSContentParsingState::WPSContentParsingState() :
	m_textBuffer(""), m_numDeferredTabs(0),

	m_font(), m_paragraph(), m_list(),
	m_isParagraphColumnBreak(false), m_isParagraphPageBreak(false),

	m_isPageSpanOpened(false), m_isSectionOpened(false), m_isFrameOpened(false),
	m_isPageSpanBreakDeferred(false),
	m_isHeaderFooterWithoutParagraph(false),

	m_isSpanOpened(false), m_isParagraphOpened(false), m_isListElementOpened(false),

	m_firstParagraphInPageSpan(true),

	m_numRowsToSkip(),
	m_isTableOpened(false), m_isTableRowOpened(false), m_isTableColumnOpened(false),
	m_isTableCellOpened(false),

	m_currentPage(0), m_numPagesRemainingInSpan(0), m_currentPageNumber(1),

	m_sectionAttributesChanged(false),
	m_numColumns(1),
	m_textColumns(),
	m_isTextColumnWithoutParagraph(false),

	m_pageFormLength(11.0),
	m_pageFormWidth(8.5f),
	m_pageFormOrientationIsPortrait(true),

	m_pageMarginLeft(1.0),
	m_pageMarginRight(1.0),
	m_pageMarginTop(1.0),
	m_pageMarginBottom(1.0),

	m_listOrderedLevels(),

	m_inSubDocument(false),
	m_isNote(false),
	m_subDocumentType(libwps::DOC_NONE)
{
	m_font.m_size=12.0;
	m_font.m_name="Times New Roman";
}

WPSContentParsingState::~WPSContentParsingState()
{
}

WPSContentListener::WPSContentListener(std::vector<WPSPageSpan> const &pageList, librevenge::RVNGTextInterface *documentInterface) :
	m_ds(new WPSDocumentParsingState(pageList)), m_ps(new WPSContentParsingState), m_psStack(),
	m_documentInterface(documentInterface)
{
	_updatePageSpanDependent(true);
}

WPSContentListener::~WPSContentListener()
{
}

///////////////////
// text data
///////////////////
void WPSContentListener::insertCharacter(uint8_t character)
{
	if (character >= 0x80)
	{
		insertUnicode(character);
		return;
	}
	_flushDeferredTabs ();
	if (!m_ps->m_isSpanOpened) _openSpan();
	m_ps->m_textBuffer.append(char(character));
}

void WPSContentListener::insertUnicode(uint32_t val)
{
	// undef character, we skip it
	if (val == 0xfffd) return;
	_flushDeferredTabs ();
	if (!m_ps->m_isSpanOpened) _openSpan();
	appendUnicode(val, m_ps->m_textBuffer);
}

void WPSContentListener::insertUnicodeString(librevenge::RVNGString const &str)
{
	_flushDeferredTabs ();
	if (!m_ps->m_isSpanOpened) _openSpan();
	m_ps->m_textBuffer.append(str);
}

void WPSContentListener::appendUnicode(uint32_t val, librevenge::RVNGString &buffer)
{
	if (val < 0x20)
	{
		WPS_DEBUG_MSG(("WPSContentListener::appendUnicode: find an old char %x, skip it\n", val));
		return;
	}
	uint8_t first;
	int len;
	if (val < 0x80)
	{
		first = 0;
		len = 1;
	}
	else if (val < 0x800)
	{
		first = 0xc0;
		len = 2;
	}
	else if (val < 0x10000)
	{
		first = 0xe0;
		len = 3;
	}
	else if (val < 0x200000)
	{
		first = 0xf0;
		len = 4;
	}
	else if (val < 0x4000000)
	{
		first = 0xf8;
		len = 5;
	}
	else
	{
		first = 0xfc;
		len = 6;
	}

	uint8_t outbuf[6] = { 0, 0, 0, 0, 0, 0 };
	int i;
	for (i = len - 1; i > 0; --i)
	{
		outbuf[i] = uint8_t((val & 0x3f) | 0x80);
		val >>= 6;
	}
	outbuf[0] = uint8_t(val | first);
	for (i = 0; i < len; i++) buffer.append(char(outbuf[i]));
}

void WPSContentListener::insertEOL(bool soft)
{
	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
		_openSpan();
	_flushDeferredTabs();

	if (soft)
	{
		if (m_ps->m_isSpanOpened)
			_flushText();
		m_documentInterface->insertLineBreak();
	}
	else if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	// sub/superscript must not survive a new line
	static const uint32_t s_subsuperBits = WPS_SUBSCRIPT_BIT | WPS_SUPERSCRIPT_BIT;
	if (m_ps->m_font.m_attributes & s_subsuperBits)
		m_ps->m_font.m_attributes &= ~s_subsuperBits;
}

void WPSContentListener::insertTab()
{
	if (!m_ps->m_isParagraphOpened)
	{
		m_ps->m_numDeferredTabs++;
		return;
	}
	if (m_ps->m_isSpanOpened) _flushText();
	m_ps->m_numDeferredTabs++;
	_flushDeferredTabs();
}

void WPSContentListener::insertBreak(const uint8_t breakType)
{
	switch (breakType)
	{
	case WPS_COLUMN_BREAK:
		if (!m_ps->m_isPageSpanOpened && !m_ps->m_inSubDocument)
			_openSpan();
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		m_ps->m_isParagraphColumnBreak = true;
		m_ps->m_isTextColumnWithoutParagraph = true;
		break;
	case WPS_PAGE_BREAK:
		if (!m_ps->m_isPageSpanOpened && !m_ps->m_inSubDocument)
			_openSpan();
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		m_ps->m_isParagraphPageBreak = true;
		break;
	default:
		break;
	}

	if (m_ps->m_inSubDocument)
		return;

	switch (breakType)
	{
	case WPS_PAGE_BREAK:
	case WPS_SOFT_PAGE_BREAK:
		if (m_ps->m_numPagesRemainingInSpan > 0)
			m_ps->m_numPagesRemainingInSpan--;
		else
		{
			if (!m_ps->m_isTableOpened && !m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
				_closePageSpan();
			else
				m_ps->m_isPageSpanBreakDeferred = true;
		}
		m_ps->m_currentPageNumber++;
		break;
	default:
		break;
	}
}

void WPSContentListener::_insertBreakIfNecessary(librevenge::RVNGPropertyList &propList)
{
	if (m_ps->m_isParagraphPageBreak && !m_ps->m_inSubDocument)
	{
		// no hard page-breaks in subdocuments
		propList.insert("fo:break-before", "page");
		m_ps->m_isParagraphPageBreak = false;
	}
	else if (m_ps->m_isParagraphColumnBreak)
	{
		if (m_ps->m_numColumns > 1)
			propList.insert("fo:break-before", "column");
		else
			propList.insert("fo:break-before", "page");
	}
}

///////////////////
// font/character format
///////////////////
void WPSContentListener::setFont(const WPSFont &font)
{
	WPSFont newFont(font);
	if (font.m_size<=0)
		newFont.m_size=m_ps->m_font.m_size;
	if (font.m_name.empty())
		newFont.m_name=m_ps->m_font.m_name;
	if (font.m_languageId <= 0)
		newFont.m_languageId=m_ps->m_font.m_languageId;
	if (m_ps->m_font==newFont) return;
	_closeSpan();
	m_ps->m_font=newFont;
}

WPSFont const &WPSContentListener::getFont() const
{
	return m_ps->m_font;
}

///////////////////
// paragraph tabs, tabs/...
///////////////////
bool WPSContentListener::isParagraphOpened() const
{
	return m_ps->m_isParagraphOpened;
}

WPSParagraph const &WPSContentListener::getParagraph() const
{
	return m_ps->m_paragraph;
}

void WPSContentListener::setParagraph(const WPSParagraph &para)
{
	// check if we need to update the list
	if (para.m_listLevelIndex >= 1)
	{
		WPSList::Level level = para.m_listLevel;
		level.m_labelWidth = (para.m_margins[1]-level.m_labelIndent);
		if (level.m_labelWidth<0.1)
			level.m_labelWidth = 0.1;
		level.m_labelIndent = 0;

		shared_ptr<WPSList> theList = getCurrentList();
		if (!theList)
		{
			theList = shared_ptr<WPSList>(new WPSList);
			theList->set(para.m_listLevelIndex, level);
			setCurrentList(theList);
		}
		else
			theList->set(para.m_listLevelIndex, level);
	}
	m_ps->m_paragraph=para;
}

///////////////////
// List: Minimal implementation
///////////////////
void WPSContentListener::setCurrentList(shared_ptr<WPSList> list)
{
	m_ps->m_list=list;
	if (list && list->getId() <= 0 && list->numLevels())
		list->setId(++m_ds->m_newListId);
}
shared_ptr<WPSList> WPSContentListener::getCurrentList() const
{
	return m_ps->m_list;
}

///////////////////
// field :
///////////////////
#include <time.h>
void WPSContentListener::insertField(WPSContentListener::FieldType type)
{
	switch(type)
	{
	case None:
		break;
	case PageNumber:
	{
		_flushText();
		_openSpan();
		librevenge::RVNGPropertyList propList;
		propList.insert("style:num-format", libwps::numberingTypeToString(libwps::ARABIC).c_str());
		m_documentInterface->insertField(librevenge::RVNGString("text:page-number"), propList);
		break;
	}
	case Database:
	{
		librevenge::RVNGString tmp("#DATAFIELD#");
		insertUnicodeString(tmp);
		break;
	}
	case Title:
	{
		librevenge::RVNGString tmp("#TITLE#");
		insertUnicodeString(tmp);
		break;
	}
	case Date:
		insertDateTimeField("%m/%d/%y");
		break;
	case Time:
		insertDateTimeField("%I:%M:%S %p");
		break;
	case Link:
	default:
		WPS_DEBUG_MSG(("WPSContentListener::insertField: must not be called with type=%d\n", int(type)));
		break;
	}
}

void WPSContentListener::insertDateTimeField(char const *format)
{
	if (!format)
	{
		WPS_DEBUG_MSG(("WPSContentListener::insertDateTimeField: oops, can not find the format\n"));
		return;
	}
	time_t now = time ( 0L );
	struct tm timeinfo;
	if (localtime_r(&now, &timeinfo))
	{
		char buf[256];
		strftime(buf, 256, format, &timeinfo);
		insertUnicodeString(librevenge::RVNGString(buf));
	}
}

///////////////////
// section
///////////////////
bool WPSContentListener::isSectionOpened() const
{
	return m_ps->m_isSectionOpened;
}

int WPSContentListener::getSectionNumColumns() const
{
	return m_ps->m_numColumns;
}

bool WPSContentListener::openSection(std::vector<int> colsWidth, librevenge::RVNGUnit unit)
{
	if (m_ps->m_isSectionOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openSection: a section is already opened\n"));
		return false;
	}

	if (m_ps->m_isTableOpened || (m_ps->m_inSubDocument && m_ps->m_subDocumentType != libwps::DOC_TEXT_BOX))
	{
		WPS_DEBUG_MSG(("WPSContentListener::openSection: impossible to open a section\n"));
		return false;
	}

	size_t numCols = colsWidth.size();
	if (numCols <= 1)
		m_ps->m_textColumns.resize(0);
	else
	{
		float factor = 1.0;
		switch(unit)
		{
		case librevenge::RVNG_POINT:
		case librevenge::RVNG_TWIP:
			factor = WPSPosition::getScaleFactor(unit, librevenge::RVNG_INCH);
			break;
		case librevenge::RVNG_INCH:
			break;
		case librevenge::RVNG_PERCENT:
		case librevenge::RVNG_GENERIC:
		case librevenge::RVNG_UNIT_ERROR:
		default:
			WPS_DEBUG_MSG(("WPSContentListener::openSection: unknown unit\n"));
			return false;
		}
		m_ps->m_textColumns.resize(numCols);
		m_ps->m_numColumns=int(numCols);
		for (size_t col = 0; col < numCols; col++)
		{
			WPSColumnDefinition column;
			column.m_width = factor*float(colsWidth[col]);
			m_ps->m_textColumns[col] = column;
		}
	}
	_openSection();
	return true;
}

bool WPSContentListener::closeSection()
{
	if (!m_ps->m_isSectionOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::closeSection: no section are already opened\n"));
		return false;
	}

	if (m_ps->m_isTableOpened || (m_ps->m_inSubDocument && m_ps->m_subDocumentType != libwps::DOC_TEXT_BOX))
	{
		WPS_DEBUG_MSG(("WPSContentListener::closeSection: impossible to close a section\n"));
		return false;
	}

	_closeSection();
	return true;
}

///////////////////
// document
///////////////////
void WPSContentListener::setDocumentLanguage(int lcid)
{
	if (lcid <= 0) return;
	std::string lang = libwps_tools_win::Language::localeName(lcid);
	if (!lang.length()) return;
	m_ds->m_metaData.insert("librevenge:language", lang.c_str());
}

void WPSContentListener::startDocument()
{
	if (m_ds->m_isDocumentStarted)
	{
		WPS_DEBUG_MSG(("WPSContentListener::startDocument: the document is already started\n"));
		return;
	}

	// FIXME: this is stupid, we should store a property list filled with the relevant metadata
	// and then pass that directly..
	m_documentInterface->setDocumentMetaData(m_ds->m_metaData);

	m_documentInterface->startDocument();
	m_ds->m_isDocumentStarted = true;
}

void WPSContentListener::endDocument()
{
	if (!m_ds->m_isDocumentStarted)
	{
		WPS_DEBUG_MSG(("WPSContentListener::startDocument: the document is not started\n"));
		return;
	}

	if (!m_ps->m_isPageSpanOpened)
		_openSpan();

	if (m_ps->m_isTableOpened)
		closeTable();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	m_ps->m_paragraph.m_listLevelIndex = 0;
	_changeList(); // flush the list exterior

	// close the document nice and tight
	_closeSection();
	_closePageSpan();
	m_documentInterface->endDocument();
	m_ds->m_isDocumentStarted = false;
}

///////////////////
// page
///////////////////
void WPSContentListener::_openPageSpan()
{
	if (m_ps->m_isPageSpanOpened)
		return;

	if (!m_ds->m_isDocumentStarted)
		startDocument();

	if (m_ds->m_pageList.size()==0)
	{
		WPS_DEBUG_MSG(("WPSContentListener::_openPageSpan: can not find any page\n"));
		throw libwps::ParseException();
	}
	unsigned actPage = 0;
	std::vector<WPSPageSpan>::iterator it = m_ds->m_pageList.begin();
	while(actPage < m_ps->m_currentPage)
	{
		actPage+=(unsigned) it++->getPageSpan();
		if (it == m_ds->m_pageList.end())
		{
			WPS_DEBUG_MSG(("WPSContentListener::_openPageSpan: can not find current page\n"));
			throw libwps::ParseException();
		}
	}
	WPSPageSpan &currentPage = *it;

	librevenge::RVNGPropertyList propList;
	currentPage.getPageProperty(propList);
	propList.insert("librevenge:is-last-page-span", ((m_ps->m_currentPage + 1 == m_ds->m_pageList.size()) ? true : false));

	if (!m_ps->m_isPageSpanOpened)
		m_documentInterface->openPageSpan(propList);

	m_ps->m_isPageSpanOpened = true;

	_updatePageSpanDependent(false);
	m_ps->m_pageFormLength = currentPage.getFormLength();
	m_ps->m_pageFormWidth = currentPage.getFormWidth();
	m_ps->m_pageMarginLeft = currentPage.getMarginLeft();
	m_ps->m_pageMarginRight = currentPage.getMarginRight();
	m_ps->m_pageFormOrientationIsPortrait =
	    currentPage.getFormOrientation() == WPSPageSpan::PORTRAIT;
	m_ps->m_pageMarginTop = currentPage.getMarginTop();
	m_ps->m_pageMarginBottom = currentPage.getMarginBottom();
	_updatePageSpanDependent(true);

	// we insert the header footer
	currentPage.sendHeaderFooters(this, m_documentInterface);

	// first paragraph in span (necessary for resetting page number)
	m_ps->m_firstParagraphInPageSpan = true;
	m_ps->m_numPagesRemainingInSpan = (currentPage.getPageSpan() - 1);
	m_ps->m_currentPage++;
}

void WPSContentListener::_closePageSpan()
{
	if (!m_ps->m_isPageSpanOpened)
		return;

	if (m_ps->m_isSectionOpened)
		_closeSection();

	m_documentInterface->closePageSpan();
	m_ps->m_isPageSpanOpened = m_ps->m_isPageSpanBreakDeferred = false;
}

void WPSContentListener::_updatePageSpanDependent(bool /*set*/)
{
}

///////////////////
// section
///////////////////
void WPSContentListener::_openSection()
{
	if (m_ps->m_isSectionOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::_openSection: a section is already opened\n"));
		return;
	}

	if (!m_ps->m_isPageSpanOpened)
		_openPageSpan();

	m_ps->m_numColumns = int(m_ps->m_textColumns.size());

	librevenge::RVNGPropertyList propList;
	propList.insert("fo:margin-left", 0.);
	propList.insert("fo:margin-right", 0.);
	if (m_ps->m_numColumns > 1)
		propList.insert("text:dont-balance-text-columns", false);

	librevenge::RVNGPropertyListVector columns;
	for (size_t i = 0; i < m_ps->m_textColumns.size(); i++)
	{
		WPSColumnDefinition const &col = m_ps->m_textColumns[i];
		librevenge::RVNGPropertyList column;
		// The "style:rel-width" is expressed in twips (1440 twips per inch) and includes the left and right Gutter
		column.insert("style:rel-width", col.m_width * 1440.0, librevenge::RVNG_TWIP);
		column.insert("fo:start-indent", col.m_leftGutter);
		column.insert("fo:end-indent", col.m_rightGutter);
		columns.append(column);
	}
	m_documentInterface->openSection(propList, columns);

	m_ps->m_sectionAttributesChanged = false;
	m_ps->m_isSectionOpened = true;
}

void WPSContentListener::_closeSection()
{
	if (!m_ps->m_isSectionOpened ||m_ps->m_isTableOpened)
		return;

	if (m_ps->m_isParagraphOpened)
		_closeParagraph();
	_changeList();

	m_documentInterface->closeSection();

	m_ps->m_numColumns = 1;
	m_ps->m_sectionAttributesChanged = false;
	m_ps->m_isSectionOpened = false;
}

///////////////////
// paragraph
///////////////////
void WPSContentListener::_openParagraph()
{
	if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
		return;

	if (m_ps->m_isParagraphOpened || m_ps->m_isListElementOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::_openParagraph: a paragraph (or a list) is already opened"));
		return;
	}
	if (!m_ps->m_isTableOpened && (!m_ps->m_inSubDocument || m_ps->m_subDocumentType == libwps::DOC_TEXT_BOX))
	{
		if (m_ps->m_sectionAttributesChanged)
			_closeSection();

		if (!m_ps->m_isSectionOpened)
			_openSection();
	}

	librevenge::RVNGPropertyList propList;
	librevenge::RVNGPropertyListVector tabStops;
	_appendParagraphProperties(propList, tabStops);

	if (!m_ps->m_isParagraphOpened)
		m_documentInterface->openParagraph(propList, tabStops);

	_resetParagraphState();
	m_ps->m_firstParagraphInPageSpan = false;
}

void WPSContentListener::_closeParagraph()
{
	if (m_ps->m_isListElementOpened)
	{
		_closeListElement();
		return;
	}

	if (m_ps->m_isParagraphOpened)
	{
		if (m_ps->m_isSpanOpened)
			_closeSpan();

		m_documentInterface->closeParagraph();
	}

	m_ps->m_isParagraphOpened = false;
	m_ps->m_paragraph.m_listLevelIndex = 0;

	if (!m_ps->m_isTableOpened && m_ps->m_isPageSpanBreakDeferred && !m_ps->m_inSubDocument)
		_closePageSpan();
}

void WPSContentListener::_resetParagraphState(const bool isListElement)
{
	m_ps->m_isParagraphColumnBreak = false;
	m_ps->m_isParagraphPageBreak = false;
	if (isListElement)
	{
		m_ps->m_isListElementOpened = true;
		m_ps->m_isParagraphOpened = true;
	}
	else
	{
		m_ps->m_isListElementOpened = false;
		m_ps->m_isParagraphOpened = true;
	}
	m_ps->m_isTextColumnWithoutParagraph = false;
	m_ps->m_isHeaderFooterWithoutParagraph = false;
}

void WPSContentListener::_appendParagraphProperties
(librevenge::RVNGPropertyList &propList, librevenge::RVNGPropertyListVector &tabStops, const bool /*isListElement*/)
{
	m_ps->m_paragraph.addTo(propList, tabStops, m_ps->m_isTableOpened);

	if (!m_ps->m_inSubDocument && m_ps->m_firstParagraphInPageSpan)
	{
		unsigned actPage = 1;
		std::vector<WPSPageSpan>::const_iterator it = m_ds->m_pageList.begin();
		while(actPage < m_ps->m_currentPage)
		{
			if (it == m_ds->m_pageList.end())
				break;

			actPage+=unsigned(it++->getPageSpan());
		}
		if (it != m_ds->m_pageList.end())
		{
			WPSPageSpan const &currentPage = *it;
			if (currentPage.getPageNumber() >= 0)
				propList.insert("style:page-number", currentPage.getPageNumber());
		}
	}

	_insertBreakIfNecessary(propList);
}

///////////////////
// list
///////////////////
void WPSContentListener::_openListElement()
{
	if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
		return;

	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
	{
		if (!m_ps->m_isTableOpened && (!m_ps->m_inSubDocument || m_ps->m_subDocumentType == libwps::DOC_TEXT_BOX))
		{
			if (m_ps->m_sectionAttributesChanged)
				_closeSection();

			if (!m_ps->m_isSectionOpened)
				_openSection();
		}

		librevenge::RVNGPropertyList propList;
		librevenge::RVNGPropertyListVector tabStops;
		_appendParagraphProperties(propList, tabStops, true);

		if (!m_ps->m_isListElementOpened)
			m_documentInterface->openListElement(propList, tabStops);
		_resetParagraphState(true);
	}
}

void WPSContentListener::_closeListElement()
{
	if (m_ps->m_isListElementOpened)
	{
		if (m_ps->m_isSpanOpened)
			_closeSpan();

		m_documentInterface->closeListElement();
	}

	m_ps->m_isListElementOpened = m_ps->m_isParagraphOpened = false;
	m_ps->m_paragraph.m_listLevelIndex = 0;

	if (!m_ps->m_isTableOpened && m_ps->m_isPageSpanBreakDeferred && !m_ps->m_inSubDocument)
		_closePageSpan();
}

void WPSContentListener::_changeList()
{
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	if (!m_ps->m_isSectionOpened && !m_ps->m_inSubDocument && !m_ps->m_isTableOpened)
		_openSection();

	// FIXME: even if nobody really care, if we close an ordered or an unordered
	//      elements, we must keep the previous to close this part...
	size_t actualListLevel = m_ps->m_listOrderedLevels.size();
	for (size_t i=actualListLevel; int(i) > m_ps->m_paragraph.m_listLevelIndex; i--)
	{
		if (m_ps->m_listOrderedLevels[i-1])
			m_documentInterface->closeOrderedListLevel();
		else
			m_documentInterface->closeUnorderedListLevel();
	}

	librevenge::RVNGPropertyList propList2;
	if (m_ps->m_paragraph.m_listLevelIndex)
	{
		if (!m_ps->m_list.get())
		{
			WPS_DEBUG_MSG(("WPSContentListener::_handleListChange: can not find any list\n"));
			return;
		}
		m_ps->m_list->setLevel(m_ps->m_paragraph.m_listLevelIndex);
		m_ps->m_list->openElement();

		if (m_ps->m_list->mustSendLevel(m_ps->m_paragraph.m_listLevelIndex))
		{
			if (actualListLevel == (size_t) m_ps->m_paragraph.m_listLevelIndex)
			{
				if (m_ps->m_listOrderedLevels[actualListLevel-1])
					m_documentInterface->closeOrderedListLevel();
				else
					m_documentInterface->closeUnorderedListLevel();
				actualListLevel--;
			}
			if (m_ps->m_paragraph.m_listLevelIndex==1)
			{
				// we must change the listID for writerperfect
				int prevId;
				if ((prevId=m_ps->m_list->getPreviousId()) > 0)
					m_ps->m_list->setId(prevId);
				else
					m_ps->m_list->setId(++m_ds->m_newListId);
			}
			m_ps->m_list->sendTo(*m_documentInterface, m_ps->m_paragraph.m_listLevelIndex);
		}

		propList2.insert("librevenge:id", m_ps->m_list->getId());
		m_ps->m_list->closeElement();
	}

	if ((int)actualListLevel == m_ps->m_paragraph.m_listLevelIndex) return;

	m_ps->m_listOrderedLevels.resize((size_t)m_ps->m_paragraph.m_listLevelIndex, false);
	for (size_t i=actualListLevel+1; i<= (size_t)m_ps->m_paragraph.m_listLevelIndex; i++)
	{
		if (m_ps->m_list->isNumeric(int(i)))
		{
			m_ps->m_listOrderedLevels[i-1] = true;
			m_documentInterface->openOrderedListLevel(propList2);
		}
		else
		{
			m_ps->m_listOrderedLevels[i-1] = false;
			m_documentInterface->openUnorderedListLevel(propList2);
		}
	}
}

///////////////////
// span
///////////////////
void WPSContentListener::_openSpan()
{
	if (m_ps->m_isSpanOpened)
		return;

	if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
		return;

	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
	{
		_changeList();
		if (m_ps->m_paragraph.m_listLevelIndex == 0)
			_openParagraph();
		else
			_openListElement();
	}

	librevenge::RVNGPropertyList propList;
	m_ps->m_font.addTo(propList);

	m_documentInterface->openSpan(propList);

	m_ps->m_isSpanOpened = true;
}

void WPSContentListener::_closeSpan()
{
	if (!m_ps->m_isSpanOpened)
		return;

	_flushText();
	m_documentInterface->closeSpan();
	m_ps->m_isSpanOpened = false;
}

///////////////////
// text (send data)
///////////////////
void WPSContentListener::_flushDeferredTabs()
{
	if (m_ps->m_numDeferredTabs == 0) return;

	// CHECKME: the tab are not underline even if the underline bit is set
	uint32_t oldTextAttributes = m_ps->m_font.m_attributes;
	uint32_t newAttributes = oldTextAttributes & uint32_t(~WPS_UNDERLINE_BIT) &
	                         uint32_t(~WPS_OVERLINE_BIT);
	if (oldTextAttributes != newAttributes)
	{
		_closeSpan();
		m_ps->m_font.m_attributes=newAttributes;
	}
	if (!m_ps->m_isSpanOpened) _openSpan();
	for (; m_ps->m_numDeferredTabs > 0; m_ps->m_numDeferredTabs--)
		m_documentInterface->insertTab();
	if (oldTextAttributes != newAttributes)
	{
		_closeSpan();
		m_ps->m_font.m_attributes=oldTextAttributes;
	}
}

void WPSContentListener::_flushText()
{
	if (m_ps->m_textBuffer.len() == 0) return;

	// when some many ' ' follows each other, call insertSpace
	librevenge::RVNGString tmpText;
	int numConsecutiveSpaces = 0;
	librevenge::RVNGString::Iter i(m_ps->m_textBuffer);
	for (i.rewind(); i.next();)
	{
		if (*(i()) == 0x20) // this test is compatible with unicode format
			numConsecutiveSpaces++;
		else
			numConsecutiveSpaces = 0;

		if (numConsecutiveSpaces > 1)
		{
			if (tmpText.len() > 0)
			{
				m_documentInterface->insertText(tmpText);
				tmpText.clear();
			}
			m_documentInterface->insertSpace();
		}
		else
			tmpText.append(i());
	}
	m_documentInterface->insertText(tmpText);
	m_ps->m_textBuffer.clear();
}

///////////////////
// Note/Comment/picture/textbox
///////////////////
void WPSContentListener::insertNote(const NoteType noteType, WPSSubDocumentPtr &subDocument)
{
	if (m_ps->m_isNote)
	{
		WPS_DEBUG_MSG(("WPSContentListener::insertNote try to insert a note recursively (ingnored)\n"));
		return;
	}
	librevenge::RVNGString label("");
	insertLabelNote(noteType, label, subDocument);
}

void WPSContentListener::insertLabelNote(const NoteType noteType, librevenge::RVNGString const &label, WPSSubDocumentPtr &subDocument)
{
	if (m_ps->m_isNote)
	{
		WPS_DEBUG_MSG(("WPSContentListener::insertLabelNote try to insert a note recursively (ingnored)\n"));
		return;
	}

	m_ps->m_isNote = true;
	if (m_ds->m_isHeaderFooterStarted)
	{
		WPS_DEBUG_MSG(("WPSContentListener::insertLabelNote try to insert a note in a header/footer\n"));
		/** Must not happen excepted in corrupted document, so we do the minimum.
			Note that we have no choice, either we begin by closing the paragraph,
			... or we reprogram handleSubDocument.
		*/
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		int prevListLevel = m_ps->m_paragraph.m_listLevelIndex;
		m_ps->m_paragraph.m_listLevelIndex = 0;
		_changeList(); // flush the list exterior
		handleSubDocument(subDocument, libwps::DOC_NOTE);
		m_ps->m_paragraph.m_listLevelIndex = (uint8_t)prevListLevel;
	}
	else
	{
		if (!m_ps->m_isParagraphOpened)
			_openParagraph();
		else
		{
			_flushText();
			_closeSpan();
		}

		librevenge::RVNGPropertyList propList;
		if (label.len())
			propList.insert("text:label", label);
		if (noteType == FOOTNOTE)
		{
			propList.insert("librevenge:number", ++(m_ds->m_footNoteNumber));
			m_documentInterface->openFootnote(propList);
		}
		else
		{
			propList.insert("librevenge:number", ++(m_ds->m_endNoteNumber));
			m_documentInterface->openEndnote(propList);
		}

		handleSubDocument(subDocument, libwps::DOC_NOTE);

		if (noteType == FOOTNOTE)
			m_documentInterface->closeFootnote();
		else
			m_documentInterface->closeEndnote();
	}
	m_ps->m_isNote = false;
}

void WPSContentListener::insertComment(WPSSubDocumentPtr &subDocument)
{
	if (m_ps->m_isNote)
	{
		WPS_DEBUG_MSG(("WPSContentListener::insertComment try to insert a note recursively (ingnored)"));
		return;
	}

	if (!m_ps->m_isParagraphOpened)
		_openParagraph();
	else
	{
		_flushText();
		_closeSpan();
	}

	librevenge::RVNGPropertyList propList;
	m_documentInterface->openComment(propList);

	m_ps->m_isNote = true;
	handleSubDocument(subDocument, libwps::DOC_COMMENT_ANNOTATION);

	m_documentInterface->closeComment();
	m_ps->m_isNote = false;
}

void WPSContentListener::insertTextBox
(WPSPosition const &pos, WPSSubDocumentPtr subDocument, librevenge::RVNGPropertyList frameExtras)
{
	if (!_openFrame(pos, frameExtras)) return;

	librevenge::RVNGPropertyList propList;
	m_documentInterface->openTextBox(propList);
	handleSubDocument(subDocument, libwps::DOC_TEXT_BOX);
	m_documentInterface->closeTextBox();

	_closeFrame();
}

void WPSContentListener::insertPicture
(WPSPosition const &pos, const librevenge::RVNGBinaryData &binaryData, std::string type,
 librevenge::RVNGPropertyList frameExtras)
{
	if (!_openFrame(pos, frameExtras)) return;

	librevenge::RVNGPropertyList propList;
	propList.insert("librevenge:mime-type", type.c_str());
	m_documentInterface->insertBinaryObject(propList, binaryData);

	_closeFrame();
}

///////////////////
// frame
///////////////////
bool WPSContentListener::_openFrame(WPSPosition const &pos, librevenge::RVNGPropertyList extras)
{
	if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openFrame: called in table but cell is not opened\n"));
		return false;
	}
	if (m_ps->m_isFrameOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openFrame: called but a frame is already opened\n"));
		return false;
	}

	switch(pos.m_anchorTo)
	{
	case WPSPosition::Page:
		break;
	case WPSPosition::Paragraph:
		if (m_ps->m_isParagraphOpened)
			_flushText();
		else
			_openParagraph();
		break;
	case WPSPosition::CharBaseLine:
	case WPSPosition::Char:
		if (m_ps->m_isSpanOpened)
			_flushText();
		else
			_openSpan();
		break;
	default:
		WPS_DEBUG_MSG(("WPSContentListener::openFrame: can not determine the anchor\n"));
		return false;
	}

	librevenge::RVNGPropertyList propList(extras);
	_handleFrameParameters(propList, pos);
	m_documentInterface->openFrame(propList);

	m_ps->m_isFrameOpened = true;
	return true;
}

void WPSContentListener::_closeFrame()
{
	if (!m_ps->m_isFrameOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::closeFrame: called but no frame is already opened\n"));
		return;
	}
	m_documentInterface->closeFrame();
	m_ps->m_isFrameOpened = false;
}

void WPSContentListener::_handleFrameParameters
( librevenge::RVNGPropertyList &propList, WPSPosition const &pos)
{
	Vec2f origin = pos.origin();
	librevenge::RVNGUnit unit = pos.unit();
	float inchFactor=pos.getInvUnitScale(librevenge::RVNG_INCH);
	float pointFactor = pos.getInvUnitScale(librevenge::RVNG_POINT);

	propList.insert("svg:width", double(pos.size()[0]), unit);
	propList.insert("svg:height", double(pos.size()[1]), unit);
	if (pos.naturalSize().x() > 4*pointFactor && pos.naturalSize().y() > 4*pointFactor)
	{
		propList.insert("librevenge:naturalWidth", pos.naturalSize().x(), pos.unit());
		propList.insert("librevenge:naturalHeight", pos.naturalSize().y(), pos.unit());
	}

	if ( pos.m_wrapping ==  WPSPosition::WDynamic)
		propList.insert( "style:wrap", "dynamic" );
	else if ( pos.m_wrapping ==  WPSPosition::WRunThrough)
	{
		propList.insert( "style:wrap", "run-through" );
		propList.insert( "style:run-through", "background" );
	}
	else
		propList.insert( "style:wrap", "none" );

	if (pos.m_anchorTo == WPSPosition::Paragraph)
	{
		propList.insert("text:anchor-type", "paragraph");
		propList.insert("style:vertical-rel", "paragraph" );
		propList.insert("style:horizontal-rel", "paragraph");
		double w = m_ps->m_pageFormWidth - m_ps->m_pageMarginLeft
		           - m_ps->m_pageMarginRight - m_ps->m_paragraph.m_margins[1]
		           - m_ps->m_paragraph.m_margins[2];
		w *= inchFactor;
		switch ( pos.m_xPos)
		{
		case WPSPosition::XRight:
			if (origin[0] < 0.0 || origin[0] > 0.0)
			{
				propList.insert( "style:horizontal-pos", "from-left");
				propList.insert( "svg:x", double(origin[0] - pos.size()[0] + w), unit);
			}
			else
				propList.insert("style:horizontal-pos", "right");
			break;
		case WPSPosition::XCenter:
			if (origin[0] < 0.0 || origin[0] > 0.0)
			{
				propList.insert( "style:horizontal-pos", "from-left");
				propList.insert( "svg:x", double(origin[0] - pos.size()[0]/2.0 + w/2.0), unit);
			}
			else
				propList.insert("style:horizontal-pos", "center");
			break;
		case WPSPosition::XLeft:
		case WPSPosition::XFull:
		default:
			if (origin[0] < 0.0 || origin[0] > 0.0)
			{
				propList.insert( "style:horizontal-pos", "from-left");
				propList.insert( "svg:x", double(origin[0]), unit);
			}
			else
				propList.insert("style:horizontal-pos", "left");
			break;
		}

		if (origin[1] < 0.0 || origin[1] > 0.0)
		{
			propList.insert( "style:vertical-pos", "from-top" );
			propList.insert( "svg:y", double(origin[1]), unit);
		}
		else
			propList.insert( "style:vertical-pos", "top" );

		return;
	}
	if ( pos.m_anchorTo == WPSPosition::Page )
	{
		// Page position seems to do not use the page margin...
		propList.insert("text:anchor-type", "page");
		if (pos.page() > 0) propList.insert("text:anchor-page-number", pos.page());
		double  w = m_ps->m_pageFormWidth;
		double h = m_ps->m_pageFormLength;
		w *= inchFactor;
		h *= inchFactor;

		propList.insert("style:vertical-rel", "page" );
		propList.insert("style:horizontal-rel", "page" );

		double newPosition;
		switch ( pos.m_yPos)
		{
		case WPSPosition::YFull:
			propList.insert("svg:height", double(h), unit);
		case WPSPosition::YTop:
			if (origin[1] < 0.0 || origin[1] > 0.0)
			{
				propList.insert("style:vertical-pos", "from-top" );
				newPosition = origin[1];
				if (newPosition > h -pos.size()[1])
					newPosition = h - pos.size()[1];
				propList.insert("svg:y", double(newPosition), unit);
			}
			else
				propList.insert("style:vertical-pos", "top" );
			break;
		case WPSPosition::YCenter:
			if (origin[1] < 0.0 || origin[1] > 0.0)
			{
				propList.insert("style:vertical-pos", "from-top" );
				newPosition = (h - pos.size()[1])/2.0;
				if (newPosition > h -pos.size()[1]) newPosition = h - pos.size()[1];
				propList.insert("svg:y", double(newPosition), unit);
			}
			else
				propList.insert("style:vertical-pos", "middle" );
			break;
		case WPSPosition::YBottom:
			if (origin[1] < 0.0 || origin[1] > 0.0)
			{
				propList.insert("style:vertical-pos", "from-top" );
				newPosition = h - pos.size()[1]-origin[1];
				if (newPosition > h -pos.size()[1]) newPosition = h -pos.size()[1];
				else if (newPosition < 0) newPosition = 0;
				propList.insert("svg:y", double(newPosition), unit);
			}
			else
				propList.insert("style:vertical-pos", "bottom" );
			break;
		default:
			break;
		}

		switch ( pos.m_xPos )
		{
		case WPSPosition::XFull:
			propList.insert("svg:width", double(w), unit);
		case WPSPosition::XLeft:
			if (origin[0] < 0.0 || origin[0] > 0.0)
			{
				propList.insert( "style:horizontal-pos", "from-left");
				propList.insert( "svg:x", double(origin[0]), unit);
			}
			else
				propList.insert( "style:horizontal-pos", "left");
			break;
		case WPSPosition::XRight:
			if (origin[0] < 0.0 || origin[0] > 0.0)
			{
				propList.insert( "style:horizontal-pos", "from-left");
				propList.insert( "svg:x",double( w - pos.size()[0] + origin[0]), unit);
			}
			else
				propList.insert( "style:horizontal-pos", "right");
			break;
		case WPSPosition::XCenter:
		default:
			if (origin[0] < 0.0 || origin[0] > 0.0)
			{
				propList.insert( "style:horizontal-pos", "from-left");
				propList.insert( "svg:x", double((w - pos.size()[0])/2. + origin[0]), unit);
			}
			else
				propList.insert( "style:horizontal-pos", "center" );
			break;
		}
		return;
	}
	if ( pos.m_anchorTo != WPSPosition::Char &&
	        pos.m_anchorTo != WPSPosition::CharBaseLine) return;

	propList.insert("text:anchor-type", "as-char");
	if ( pos.m_anchorTo == WPSPosition::CharBaseLine)
		propList.insert( "style:vertical-rel", "baseline" );
	else
		propList.insert( "style:vertical-rel", "line" );
	switch ( pos.m_yPos )
	{
	case WPSPosition::YFull:
	case WPSPosition::YTop:
		if (origin[1] < 0.0 || origin[1] > 0.0)
		{
			propList.insert( "style:vertical-pos", "from-top" );
			propList.insert( "svg:y", double(origin[1]), unit);
		}
		else
			propList.insert( "style:vertical-pos", "top" );
		break;
	case WPSPosition::YCenter:
		if (origin[1] < 0.0 || origin[1] > 0.0)
		{
			propList.insert( "style:vertical-pos", "from-top" );
			propList.insert( "svg:y", double(origin[1] - pos.size()[1]/2.0), unit);
		}
		else
			propList.insert( "style:vertical-pos", "middle" );
		break;
	case WPSPosition::YBottom:
	default:
		if (origin[1] < 0.0 || origin[1] > 0.0)
		{
			propList.insert( "style:vertical-pos", "from-top" );
			propList.insert( "svg:y", double(origin[1] - pos.size()[1]), unit);
		}
		else
			propList.insert( "style:vertical-pos", "bottom" );
		break;
	}
}

///////////////////
// subdocument
///////////////////
void WPSContentListener::handleSubDocument(WPSSubDocumentPtr &subDocument, libwps::SubDocumentType subDocumentType)
{
	_pushParsingState();
	_startSubDocument();
	m_ps->m_subDocumentType = subDocumentType;

	m_ps->m_isPageSpanOpened = true;
	m_ps->m_list.reset();

	switch(subDocumentType)
	{
	case libwps::DOC_TEXT_BOX:
		m_ps->m_pageMarginLeft = m_ps->m_pageMarginRight =
		                             m_ps->m_pageMarginTop = m_ps->m_pageMarginBottom = 0.0;
		m_ps->m_sectionAttributesChanged = true;
		break;
	case libwps::DOC_HEADER_FOOTER:
		m_ps->m_isHeaderFooterWithoutParagraph = true;
		m_ds->m_isHeaderFooterStarted = true;
		break;
	case libwps::DOC_NONE:
	case libwps::DOC_NOTE:
	case libwps::DOC_TABLE:
	case libwps::DOC_COMMENT_ANNOTATION:
	default:
		break;
	}

	// Check whether the document is calling itself
	bool sendDoc = true;
	for (size_t i = 0; i < m_ds->m_subDocuments.size(); i++)
	{
		if (!subDocument)
			break;
		if (subDocument == m_ds->m_subDocuments[i])
		{
			WPS_DEBUG_MSG(("WPSContentListener::handleSubDocument: recursif call, stop...\n"));
			sendDoc = false;
			break;
		}
	}
	if (sendDoc)
	{
		if (subDocument)
		{
			m_ds->m_subDocuments.push_back(subDocument);
			shared_ptr<WPSContentListener> listen(this, WPS_shared_ptr_noop_deleter<WPSContentListener>());
			try
			{
				subDocument->parse(listen, subDocumentType);
			}
			catch(...)
			{
				WPS_DEBUG_MSG(("Works: WPSContentListener::handleSubDocument exception catched \n"));
			}
			m_ds->m_subDocuments.pop_back();
		}
		if (m_ps->m_isHeaderFooterWithoutParagraph)
			_openSpan();
	}

	switch (m_ps->m_subDocumentType)
	{
	case libwps::DOC_TEXT_BOX:
		_closeSection();
		break;
	case libwps::DOC_HEADER_FOOTER:
		m_ds->m_isHeaderFooterStarted = false;
		break;
	case libwps::DOC_NONE:
	case libwps::DOC_NOTE:
	case libwps::DOC_TABLE:
	case libwps::DOC_COMMENT_ANNOTATION:
	default:
		break;
	}
	_endSubDocument();
	_popParsingState();
}

bool WPSContentListener::isHeaderFooterOpened() const
{
	return m_ds->m_isHeaderFooterStarted;
}

void WPSContentListener::_startSubDocument()
{
	m_ds->m_isDocumentStarted = true;
	m_ps->m_inSubDocument = true;
}

void WPSContentListener::_endSubDocument()
{
	if (m_ps->m_isTableOpened)
		closeTable();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	m_ps->m_paragraph.m_listLevelIndex = 0;
	_changeList(); // flush the list exterior
}

///////////////////
// table
///////////////////
void WPSContentListener::openTable(std::vector<float> const &colWidth, librevenge::RVNGUnit unit)
{
	if (m_ps->m_isTableOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openTable: called with m_isTableOpened=true\n"));
		return;
	}

	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	_pushParsingState();
	_startSubDocument();
	m_ps->m_subDocumentType = libwps::DOC_TABLE;

	librevenge::RVNGPropertyList propList;
	propList.insert("table:align", "left");
	propList.insert("fo:margin-left", 0.0);

	float tableWidth = 0;
	librevenge::RVNGPropertyListVector columns;

	size_t nCols = colWidth.size();
	for (size_t c = 0; c < nCols; c++)
	{
		librevenge::RVNGPropertyList column;
		column.insert("style:column-width", colWidth[c], unit);
		columns.append(column);

		tableWidth += colWidth[c];
	}
	propList.insert("style:width", tableWidth, unit);
	m_documentInterface->openTable(propList, columns);
	m_ps->m_isTableOpened = true;
}

void WPSContentListener::closeTable()
{
	if (!m_ps->m_isTableOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::closeTable: called with m_isTableOpened=false\n"));
		return;
	}

	m_ps->m_isTableOpened = false;
	_endSubDocument();
	m_documentInterface->closeTable();

	_popParsingState();
}

void WPSContentListener::openTableRow(float h, librevenge::RVNGUnit unit, bool headerRow)
{
	if (m_ps->m_isTableRowOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openTableRow: called with m_isTableRowOpened=true\n"));
		return;
	}
	if (!m_ps->m_isTableOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openTableRow: called with m_isTableOpened=false\n"));
		return;
	}
	librevenge::RVNGPropertyList propList;
	propList.insert("librevenge:is-header-row", headerRow);

	if (h > 0)
		propList.insert("style:row-height", h, unit);
	else if (h < 0)
		propList.insert("style:min-row-height", -h, unit);
	m_documentInterface->openTableRow(propList);
	m_ps->m_isTableRowOpened = true;
}

void WPSContentListener::closeTableRow()
{
	if (!m_ps->m_isTableRowOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openTableRow: called with m_isTableRowOpened=false\n"));
		return;
	}
	m_ps->m_isTableRowOpened = false;
	m_documentInterface->closeTableRow();
}

void WPSContentListener::addEmptyTableCell(Vec2i const &pos, Vec2i span)
{
	if (!m_ps->m_isTableRowOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::addEmptyTableCell: called with m_isTableRowOpened=false\n"));
		return;
	}
	if (m_ps->m_isTableCellOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::addEmptyTableCell: called with m_isTableCellOpened=true\n"));
		closeTableCell();
	}
	librevenge::RVNGPropertyList propList;
	propList.insert("librevenge:column", pos[0]);
	propList.insert("librevenge:row", pos[1]);
	propList.insert("table:number-columns-spanned", span[0]);
	propList.insert("table:number-rows-spanned", span[1]);
	m_documentInterface->openTableCell(propList);
	m_documentInterface->closeTableCell();
}

void WPSContentListener::openTableCell(WPSCell const &cell, librevenge::RVNGPropertyList const &extras)
{
	if (!m_ps->m_isTableRowOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openTableCell: called with m_isTableRowOpened=false\n"));
		return;
	}
	if (m_ps->m_isTableCellOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::openTableCell: called with m_isTableCellOpened=true\n"));
		closeTableCell();
	}

	librevenge::RVNGPropertyList propList(extras);
	propList.insert("librevenge:column", cell.position()[0]);
	propList.insert("librevenge:row", cell.position()[1]);

	propList.insert("table:number-columns-spanned", cell.numSpannedCells()[0]);
	propList.insert("table:number-rows-spanned", cell.numSpannedCells()[1]);

	std::vector<WPSBorder> const &borders = cell.borders();
	for (size_t c = 0; c < borders.size(); c++)
	{
		std::string property = borders[c].getPropertyValue();
		if (property.length() == 0) continue;
		switch(c)
		{
		case WPSBorder::Left:
			propList.insert("fo:border-left", property.c_str());
			break;
		case WPSBorder::Right:
			propList.insert("fo:border-right", property.c_str());
			break;
		case WPSBorder::Top:
			propList.insert("fo:border-top", property.c_str());
			break;
		case WPSBorder::Bottom:
			propList.insert("fo:border-bottom", property.c_str());
			break;
		default:
			WPS_DEBUG_MSG(("WPSContentListener::openTableCell: can not send %d border\n",int(c)));
			break;
		}
	}
	if (cell.backgroundColor() != 0xFFFFFF)
	{
		char color[20];
		sprintf(color,"#%06x",cell.backgroundColor());
		propList.insert("fo:background-color", color);
	}

	m_ps->m_isTableCellOpened = true;
	m_documentInterface->openTableCell(propList);
}

void WPSContentListener::closeTableCell()
{
	if (!m_ps->m_isTableCellOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::closeTableCell: called with m_isTableCellOpened=false\n"));
		return;
	}

	_closeParagraph();
	m_ps->m_paragraph.m_listLevelIndex = 0;
	_changeList(); // flush the list exterior

	m_ps->m_isTableCellOpened = false;
	m_documentInterface->closeTableCell();
}

///////////////////
// others
///////////////////

// ---------- state stack ------------------
shared_ptr<WPSContentParsingState> WPSContentListener::_pushParsingState()
{
	shared_ptr<WPSContentParsingState> actual = m_ps;
	m_psStack.push_back(actual);
	m_ps.reset(new WPSContentParsingState);

	// BEGIN: copy page properties into the new parsing state
	m_ps->m_pageFormLength = actual->m_pageFormLength;
	m_ps->m_pageFormWidth = actual->m_pageFormWidth;
	m_ps->m_pageFormOrientationIsPortrait =	actual->m_pageFormOrientationIsPortrait;
	m_ps->m_pageMarginLeft = actual->m_pageMarginLeft;
	m_ps->m_pageMarginRight = actual->m_pageMarginRight;
	m_ps->m_pageMarginTop = actual->m_pageMarginTop;
	m_ps->m_pageMarginBottom = actual->m_pageMarginBottom;

	m_ps->m_isNote = actual->m_isNote;

	return actual;
}

void WPSContentListener::_popParsingState()
{
	if (m_psStack.size()==0)
	{
		WPS_DEBUG_MSG(("WPSContentListener::_popParsingState: psStack is empty()\n"));
		throw libwps::ParseException();
	}
	m_ps = m_psStack.back();
	m_psStack.pop_back();
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
