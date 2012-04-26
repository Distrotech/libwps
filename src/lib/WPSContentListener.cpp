/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <libwpd/WPXProperty.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSList.h"
#include "WPSPageSpan.h"
#include "WPSSubDocument.h"

#include "WPSContentListener.h"

WPSContentParsingState::WPSContentParsingState() :
	m_textBuffer(""), m_numDeferredTabs(0),

	m_textAttributeBits(0),	m_fontSize(12.0), m_fontName("Times New Roman"),
	m_fontColor(0),	m_textLanguage(-1),

	m_isParagraphColumnBreak(false), m_isParagraphPageBreak(false),

	m_paragraphJustification(libwps::JustificationLeft),
	m_paragraphLineSpacing(1.0), m_paragraphLineSpacingUnit(WPX_PERCENT),
	m_paragraphBorders(0), m_paragraphLanguage(-1),

	m_list(), m_currentListLevel(0),

	m_isDocumentStarted(false),	m_isPageSpanOpened(false), m_isSectionOpened(false),
	m_isPageSpanBreakDeferred(false),
	m_isHeaderFooterWithoutParagraph(false),

	m_isSpanOpened(false), m_isParagraphOpened(false), m_isListElementOpened(false),

	m_firstParagraphInPageSpan(true),

	m_numRowsToSkip(),
#if 0
	m_tableDefinition(),
#endif
	m_currentTableCol(0), m_currentTableRow(0), m_currentTableCellNumberInRow(0),
	m_isTableOpened(false), m_isTableRowOpened(false), m_isTableColumnOpened(false),
	m_isTableCellOpened(false),
	m_wasHeaderRow(false),
	m_isCellWithoutParagraph(false),
	m_isRowWithoutCell(false),
	m_cellAttributeBits(0x00000000),
	m_paragraphJustificationBeforeTable(libwps::JustificationLeft),

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

	m_sectionMarginLeft(0.0),
	m_sectionMarginRight(0.0),
	m_sectionMarginTop(0.0),
	m_sectionMarginBottom(0.0),

	m_paragraphMarginLeft(0.0),
	m_paragraphMarginRight(0.0),
	m_paragraphMarginTop(0.0), m_paragraphMarginTopUnit(WPX_INCH),
	m_paragraphMarginBottom(0.0), m_paragraphMarginBottomUnit(WPX_INCH),

	m_leftMarginByPageMarginChange(0.0),
	m_rightMarginByPageMarginChange(0.0),
	m_leftMarginByParagraphMarginChange(0.0),
	m_rightMarginByParagraphMarginChange(0.0),
	m_leftMarginByTabs(0.0),
	m_rightMarginByTabs(0.0),

	m_paragraphTextIndent(0.0),
	m_textIndentByParagraphIndentChange(0.0),
	m_textIndentByTabs(0.0),

	m_newListId(0),
	m_listReferencePosition(0.0), m_listBeginPosition(0.0), m_listOrderedLevels(),

	m_alignmentCharacter('.'),
	m_tabStops(),
	m_isTabPositionRelative(false),

	m_subDocuments(),

	m_inSubDocument(false),
	m_isNote(false), m_footNoteNumber(0), m_endNoteNumber(0),
	m_subDocumentType(libwps::DOC_NONE)
{
}

WPSContentParsingState::~WPSContentParsingState()
{
}

WPSContentListener::WPSContentListener(std::vector<WPSPageSpan> const &pageList, WPXDocumentInterface *documentInterface) :
	m_ps(new WPSContentParsingState), m_psStack(),
	m_documentInterface(documentInterface),
	m_pageList(pageList),
	m_metaData()
{
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
	m_ps->m_textBuffer.append(character);
}

void WPSContentListener::insertUnicode(uint32_t val)
{
	// undef character, we skip it
	if (val == 0xfffd) return;

	_flushDeferredTabs ();
	if (!m_ps->m_isSpanOpened) _openSpan();
	appendUnicode(val, m_ps->m_textBuffer);
}

void WPSContentListener::insertUnicodeString(WPXString const &str)
{
	_flushDeferredTabs ();
	if (!m_ps->m_isSpanOpened) _openSpan();
	m_ps->m_textBuffer.append(str);
}

void WPSContentListener::appendUnicode(uint32_t val, WPXString &buffer)
{
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
		outbuf[i] = (val & 0x3f) | 0x80;
		val >>= 6;
	}
	outbuf[0] = val | first;
	for (i = 0; i < len; i++) buffer.append(outbuf[i]);
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
	if (m_ps->m_textAttributeBits & (WPS_SUBSCRIPT_BIT | WPS_SUPERSCRIPT_BIT))
		m_ps->m_textAttributeBits &= ~(WPS_SUBSCRIPT_BIT | WPS_SUPERSCRIPT_BIT);
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
		// TODO: (.. line break?)
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

void WPSContentListener::_insertBreakIfNecessary(WPXPropertyList &propList)
{
	if (m_ps->m_isParagraphPageBreak && !m_ps->m_inSubDocument)
	{
		// no hard page-breaks in subdocuments
		propList.insert("fo:break-before", "page");
		m_ps->m_isParagraphPageBreak = false; // osnola
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
void WPSContentListener::setFontAttributes(const uint32_t attribute)
{
	if (attribute == m_ps->m_textAttributeBits) return;
	_closeSpan();

	m_ps->m_textAttributeBits = attribute;
}

void WPSContentListener::setTextFont(const WPXString &fontName)
{
	if (fontName == m_ps->m_fontName) return;
	_closeSpan();
	// FIXME verify that fontName does not contain bad characters,
	//       if so, pass a unicode string
	m_ps->m_fontName = fontName;
}

void WPSContentListener::setFontSize(const uint16_t fontSize)
{
	float fSize = fontSize;
	if (m_ps->m_fontSize==fSize) return;

	_closeSpan();
	m_ps->m_fontSize=fSize;
}

void WPSContentListener::setTextColor(const uint32_t rgb)
{
	if (m_ps->m_fontColor==rgb) return;
	_closeSpan();
	m_ps->m_fontColor = rgb;
}

void WPSContentListener::setTextLanguage(int lcid)
{
	if (m_ps->m_textLanguage==lcid) return;
	_closeSpan();
	m_ps->m_textLanguage=lcid;
}

///////////////////
// paragraph tabs, tabs/...
///////////////////
bool WPSContentListener::isParagraphOpened() const
{
	return m_ps->m_isParagraphOpened;
}

void WPSContentListener::setParagraphLineSpacing(const double lineSpacing, WPXUnit unit)
{
	m_ps->m_paragraphLineSpacing = lineSpacing;
	m_ps->m_paragraphLineSpacingUnit = unit;
}

void WPSContentListener::setParagraphJustification(libwps::Justification justification, bool force)
{
	if (justification == m_ps->m_paragraphJustification) return;

	if (force)
	{
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();

		m_ps->m_currentListLevel = 0;
	}
	m_ps->m_paragraphJustification = justification;
}

void WPSContentListener::setParagraphTextIndent(double margin)
{
	m_ps->m_textIndentByParagraphIndentChange = margin;

	m_ps->m_paragraphTextIndent = m_ps->m_textIndentByParagraphIndentChange
	                              + m_ps->m_textIndentByTabs;
	m_ps->m_listReferencePosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
}

void WPSContentListener::setParagraphMargin(double margin, int pos)
{
	switch(pos)
	{
	case WPS_LEFT:
		m_ps->m_leftMarginByParagraphMarginChange = margin;
		m_ps->m_paragraphMarginLeft = m_ps->m_leftMarginByPageMarginChange
		                              + m_ps->m_leftMarginByParagraphMarginChange
		                              + m_ps->m_leftMarginByTabs;
		m_ps->m_listReferencePosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
		if (m_ps->m_currentListLevel)
			m_ps->m_listBeginPosition =
			    m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;

		break;
	case WPS_RIGHT:
		m_ps->m_rightMarginByParagraphMarginChange = margin;
		m_ps->m_paragraphMarginRight = m_ps->m_rightMarginByPageMarginChange
		                               + m_ps->m_rightMarginByParagraphMarginChange
		                               + m_ps->m_rightMarginByTabs;
		break;
	case WPS_TOP:
		m_ps->m_paragraphMarginTop = margin;
		break;
	case WPS_BOTTOM:
		m_ps->m_paragraphMarginBottom = margin;
		break;
	default:
		WPS_DEBUG_MSG(("WPSContentListener::setParagraphMargin: unknown pos"));
	}
}

void WPSContentListener::setTabs(const std::vector<WPSTabStop> &tabStops)
{
	m_ps->m_isTabPositionRelative = true;
	m_ps->m_tabStops = tabStops;
}

void WPSContentListener::setParagraphBorders(int which, bool flag)
{
	if (flag) m_ps->m_paragraphBorders |= which;
	else m_ps->m_paragraphBorders &= (~which);
}

void WPSContentListener::setParagraphBorders(bool flag)
{
	m_ps->m_paragraphBorders = flag ? libwps::LeftBorderBit | libwps::RightBorderBit | libwps::TopBorderBit| libwps::BottomBorderBit : 0;
}

///////////////////
// List: Minimal implementation
///////////////////
void WPSContentListener::setCurrentListLevel(int level)
{
	m_ps->m_currentListLevel = level;
	// to be compatible with WPSContentListerner
	if (level)
		m_ps->m_listBeginPosition =
		    m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
	else
		m_ps->m_listBeginPosition = 0;
}
void WPSContentListener::setCurrentList(shared_ptr<WPSList> list)
{
	m_ps->m_list=list;
	if (list && list->getId() <= 0 && list->numLevels())
		list->setId(++m_ps->m_newListId);
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
		WPXPropertyList propList;
		propList.insert("style:num-format", libwps::numberingTypeToString(libwps::ARABIC).c_str());
		m_documentInterface->insertField(WPXString("text:page-number"), propList);
		break;
	}
	case Database:
	{
		WPXString tmp("#DATAFIELD#");
		insertUnicodeString(tmp);
		break;
	}
	case Title:
	{
		WPXString tmp("#TITLE#");
		insertUnicodeString(tmp);
		break;
	}
	case Date:
		insertDateTimeField("%m/%d/%y");
		break;
	case Time:
		insertDateTimeField("%I:%M:%S %p");
		break;
	default:
		WPS_DEBUG_MSG(("WPSContentListener::insertField: must not be called with type=%d\n", int(type)));
		break;
	}
}

void WPSContentListener::insertDateTimeField(char const *format)

{
	time_t now = time ( 0L );
	struct tm timeinfo = *(localtime ( &now));
	char buf[256];
	strftime(buf, 256, format, &timeinfo);
	WPXString tmp(buf);
	insertUnicodeString(tmp);
}

///////////////////
// Note
///////////////////
void WPSContentListener::insertNote(const NoteType noteType, WPSSubDocumentPtr &subDocument)
{
	if (m_ps->m_isNote)
	{
		WPS_DEBUG_MSG(("WPSContentListener::insertNote try to insert a note recursively (ingnored)"));
		return;
	}

	if (!m_ps->m_isParagraphOpened)
		_openParagraph();
	else
	{
		_flushText();
		_closeSpan();
	}

	m_ps->m_isNote = true;

	WPXPropertyList propList;

	if (noteType == FOOTNOTE)
	{
		propList.insert("libwpd:number", ++(m_ps->m_footNoteNumber));
		m_documentInterface->openFootnote(propList);
	}
	else
	{
		propList.insert("libwpd:number", ++(m_ps->m_endNoteNumber));
		m_documentInterface->openEndnote(propList);
	}

	handleSubDocument(subDocument, libwps::DOC_NOTE);

	if (noteType == FOOTNOTE)
		m_documentInterface->closeFootnote();
	else
		m_documentInterface->closeEndnote();
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

	WPXPropertyList propList;
	m_documentInterface->openComment(propList);

	m_ps->m_isNote = true;
	handleSubDocument(subDocument, libwps::DOC_COMMENT_ANNOTATION);


	m_documentInterface->closeComment();
	m_ps->m_isNote = false;
}

///////////////////
// section
///////////////////
bool WPSContentListener::isSectionOpened() const
{
	return m_ps->m_isSectionOpened;
}

bool WPSContentListener::openSection(std::vector<int> colsWidth, WPXUnit unit)
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

	int numCols = colsWidth.size();
	if (numCols <= 1)
	{
		m_ps->m_textColumns.resize(0);
		m_ps->m_numColumns=1;
	}
	else
	{
		float factor = 1.0;
		switch(unit)
		{
		case WPX_POINT:
			factor = 1/72.;
			break;
		case WPX_TWIP:
			factor = 1/1440.;
			break;
		case WPX_INCH:
			break;
		default:
			WPS_DEBUG_MSG(("WPSContentListener::openSection: unknown unit\n"));
			return false;
		}
		m_ps->m_textColumns.resize(numCols);
		m_ps->m_numColumns=numCols;
		for (int col = 0; col < numCols; col++)
		{
			WPSColumnDefinition column;
			column.m_width = factor*colsWidth[col];
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

////////////////////////////////////////////////////////////
void WPSContentListener::setDocumentLanguage(int lcid)
{
	if (lcid <= 0) return;
	std::string lang = libwps_tools_win::Language::localeName(lcid);
	if (!lang.length()) return;
	m_metaData.insert("libwpd:language", lang.c_str());
}

void WPSContentListener::startDocument()
{
	if (m_ps->m_isDocumentStarted)
	{
		WPS_DEBUG_MSG(("WPSContentListener::startDocument: the document is already started\n"));
		return;
	}

	// FIXME: this is stupid, we should store a property list filled with the relevant metadata
	// and then pass that directly..
	m_documentInterface->setDocumentMetaData(m_metaData);

	m_documentInterface->startDocument();
	m_ps->m_isDocumentStarted = true;
}

void WPSContentListener::startSubDocument()
{
	m_ps->m_isDocumentStarted = true;
	m_ps->m_inSubDocument = true;
}

void WPSContentListener::endDocument()
{
	if (!m_ps->m_isDocumentStarted)
	{
		WPS_DEBUG_MSG(("WPSContentListener::startDocument: the document is not started\n"));
		return;
	}

	if (!m_ps->m_isPageSpanOpened)
		_openSpan();

	if (m_ps->m_isTableOpened)
		_closeTable();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	m_ps->m_currentListLevel = 0;
	_changeList(); // flush the list exterior

	// close the document nice and tight
	_closeSection();
	_closePageSpan();
	m_documentInterface->endDocument();
	m_ps->m_isDocumentStarted = false;
}

void WPSContentListener::endSubDocument()
{
	if (m_ps->m_isTableOpened)
		_closeTable();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	m_ps->m_currentListLevel = 0;
	_changeList(); // flush the list exterior
}

////////////////////////////////////////////////////////////
void WPSContentListener::_openSection()
{
	if (m_ps->m_isSectionOpened)
	{
		WPS_DEBUG_MSG(("WPSContentListener::_openSection: a section is already opened\n"));
		return;
	}

	if (!m_ps->m_isPageSpanOpened)
		_openPageSpan();

	WPXPropertyList propList;
#ifdef NEW_VERSION
	propList.insert("fo:margin-left", m_ps->m_sectionMarginLeft);
	propList.insert("fo:margin-right", m_ps->m_sectionMarginRight);
#endif
	if (m_ps->m_numColumns > 1)
		propList.insert("text:dont-balance-text-columns", false);
	if (m_ps->m_sectionMarginTop)
		propList.insert("libwpd:margin-top", m_ps->m_sectionMarginTop);
	if (m_ps->m_sectionMarginBottom)
		propList.insert("libwpd:margin-bottom", m_ps->m_sectionMarginBottom);

	WPXPropertyListVector columns;
	for (int i = 0; i < int(m_ps->m_textColumns.size()); i++)
	{
		WPSColumnDefinition const &col = m_ps->m_textColumns[i];
		WPXPropertyList column;
		// The "style:rel-width" is expressed in twips (1440 twips per inch) and includes the left and right Gutter
		column.insert("style:rel-width", col.m_width * 1440.0, WPX_TWIP);
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

	m_ps->m_sectionAttributesChanged = false;
	m_ps->m_isSectionOpened = false;
}

////////////////////////////////////////////////////////////
void WPSContentListener::_openPageSpan()
{
	if (m_ps->m_isPageSpanOpened)
		return;

	if (!m_ps->m_isDocumentStarted)
		startDocument();

	unsigned actPage = 0;
	std::vector<WPSPageSpan>::iterator it = m_pageList.begin();
	while(actPage < m_ps->m_currentPage)
	{
		if (it == m_pageList.end())
		{
			WPS_DEBUG_MSG(("WPSContentListener::_openPageSpan: can not find current page\n"));
			throw libwps::ParseException();
		}

		actPage+=it->getPageSpan();
		it++;
	}
	WPSPageSpan &currentPage = *it;

	WPXPropertyList propList;
	currentPage.getPageProperty(propList);
	propList.insert("libwpd:is-last-page-span", ((m_ps->m_currentPage + 1 == m_pageList.size()) ? true : false));

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

	m_ps->m_paragraphMarginLeft = m_ps->m_leftMarginByPageMarginChange + m_ps->m_leftMarginByParagraphMarginChange
	                              + m_ps->m_leftMarginByTabs;
	m_ps->m_paragraphMarginRight = m_ps->m_rightMarginByPageMarginChange + m_ps->m_rightMarginByParagraphMarginChange
	                               + m_ps->m_rightMarginByTabs;
	m_ps->m_paragraphTextIndent = m_ps->m_textIndentByParagraphIndentChange + m_ps->m_textIndentByTabs;

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

void WPSContentListener::_updatePageSpanDependent(bool set)
{
	double deltaRight = set ? -m_ps->m_pageMarginRight : m_ps->m_pageMarginRight;
	double deltaLeft = set ? -m_ps->m_pageMarginLeft : m_ps->m_pageMarginLeft;
	if (m_ps->m_leftMarginByPageMarginChange != 0)
		m_ps->m_leftMarginByPageMarginChange += deltaLeft;
	if (m_ps->m_rightMarginByPageMarginChange != 0)
		m_ps->m_rightMarginByPageMarginChange += deltaRight;
	if (m_ps->m_sectionMarginLeft != 0)
		m_ps->m_sectionMarginLeft += deltaLeft;
	if (m_ps->m_sectionMarginRight != 0)
		m_ps->m_sectionMarginRight += deltaRight;
	if (m_ps->m_listReferencePosition != 0)
		m_ps->m_listReferencePosition += deltaLeft;
	if (m_ps->m_listBeginPosition != 0)
		m_ps->m_listBeginPosition += deltaLeft;
}

///////////////////////////////////////////////////////////
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

	WPXPropertyListVector tabStops;
	_getTabStops(tabStops);

	WPXPropertyList propList;
	_appendParagraphProperties(propList);

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
	m_ps->m_currentListLevel = 0;

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
	m_ps->m_paragraphMarginLeft = m_ps->m_leftMarginByPageMarginChange + m_ps->m_leftMarginByParagraphMarginChange;
	m_ps->m_paragraphMarginRight = m_ps->m_rightMarginByPageMarginChange + m_ps->m_rightMarginByParagraphMarginChange;
	m_ps->m_leftMarginByTabs = 0.0;
	m_ps->m_rightMarginByTabs = 0.0;
	m_ps->m_paragraphTextIndent = m_ps->m_textIndentByParagraphIndentChange;
	m_ps->m_textIndentByTabs = 0.0;
	m_ps->m_isCellWithoutParagraph = false;
	m_ps->m_isTextColumnWithoutParagraph = false;
	m_ps->m_isHeaderFooterWithoutParagraph = false;
	m_ps->m_listReferencePosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
	m_ps->m_listBeginPosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
}

void WPSContentListener::_appendJustification(WPXPropertyList &propList, libwps::Justification justification)
{
	switch (justification)
	{
	case libwps::JustificationLeft:
		// doesn't require a paragraph prop - it is the default
		propList.insert("fo:text-align", "left");
		break;
	case libwps::JustificationCenter:
		propList.insert("fo:text-align", "center");
		break;
	case libwps::JustificationRight:
		propList.insert("fo:text-align", "end");
		break;
	case libwps::JustificationFull:
		propList.insert("fo:text-align", "justify");
		break;
	case libwps::JustificationFullAllLines:
		propList.insert("fo:text-align", "justify");
		propList.insert("fo:text-align-last", "justify");
		break;
	}
}

void WPSContentListener::_appendParagraphProperties(WPXPropertyList &propList, const bool isListElement)
{
	_appendJustification(propList, m_ps->m_paragraphJustification);

	if (!m_ps->m_isTableOpened)
	{
		// these properties are not appropriate when a table is opened..
		if (isListElement)
		{
			propList.insert("fo:margin-left", (m_ps->m_listBeginPosition - m_ps->m_paragraphTextIndent));
			propList.insert("fo:text-indent", m_ps->m_paragraphTextIndent);
		}
		else
		{
			propList.insert("fo:margin-left", m_ps->m_paragraphMarginLeft);
			propList.insert("fo:text-indent", m_ps->m_listReferencePosition - m_ps->m_paragraphMarginLeft);
		}
		propList.insert("fo:margin-right", m_ps->m_paragraphMarginRight);
		if (m_ps->m_paragraphBorders)
		{
			int border = m_ps->m_paragraphBorders;
			if (border == 0xF)
			{
				propList.insert("fo:border", "0.03cm solid #000000");
				return;
			}
			if (border & libwps::LeftBorderBit)
				propList.insert("fo:border-left", "0.03cm solid #000000");
			if (border & libwps::RightBorderBit)
				propList.insert("fo:border-right", "0.03cm solid #000000");
			if (border & libwps::TopBorderBit)
				propList.insert("fo:border-top", "0.03cm solid #000000");
			if (border & libwps::BottomBorderBit)
				propList.insert("fo:border-bottom", "0.03cm solid #000000");
		}
		m_ps->m_paragraphLanguage = m_ps->m_textLanguage;
		_addLanguage(m_ps->m_paragraphLanguage, propList);
	}
	else
		m_ps->m_paragraphLanguage = -1;
	propList.insert("fo:margin-top", m_ps->m_paragraphMarginTop, m_ps->m_paragraphMarginBottomUnit);
	propList.insert("fo:margin-bottom", m_ps->m_paragraphMarginBottom, m_ps->m_paragraphMarginBottomUnit);
	propList.insert("fo:line-height", m_ps->m_paragraphLineSpacing, m_ps->m_paragraphLineSpacingUnit);


	if (!m_ps->m_inSubDocument && m_ps->m_firstParagraphInPageSpan)
	{
		unsigned actPage = 1;
		std::vector<WPSPageSpan>::const_iterator it = m_pageList.begin();
		while(actPage < m_ps->m_currentPage)
		{
			if (it == m_pageList.end())
				break;

			actPage+=it->getPageSpan();
			it++;
		}
		WPSPageSpan const &currentPage = *it;
		if (currentPage.getPageNumber() >= 0)
			propList.insert("style:page-number", currentPage.getPageNumber());
	}

	_insertBreakIfNecessary(propList);
}

void WPSContentListener::_getTabStops(WPXPropertyListVector &tabStops)
{
	double decalX = m_ps->m_isTabPositionRelative ? -m_ps->m_leftMarginByTabs :
	                -m_ps->m_paragraphMarginLeft-m_ps->m_sectionMarginLeft-m_ps->m_pageMarginLeft;
	for (int i=0; i<(int)m_ps->m_tabStops.size(); i++)
		m_ps->m_tabStops[i].addTo(tabStops, decalX);
}

////////////////////////////////////////////////////////////
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

		WPXPropertyList propList;
		_appendParagraphProperties(propList, true);

		WPXPropertyListVector tabStops;
		_getTabStops(tabStops);

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
	m_ps->m_currentListLevel = 0;

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
	int actualListLevel = m_ps->m_listOrderedLevels.size();
	for (int i=actualListLevel; i > m_ps->m_currentListLevel; i--)
	{
		if (m_ps->m_listOrderedLevels[i-1])
			m_documentInterface->closeOrderedListLevel();
		else
			m_documentInterface->closeUnorderedListLevel();
	}

	WPXPropertyList propList2;
	if (m_ps->m_currentListLevel)
	{
		if (!m_ps->m_list.get())
		{
			WPS_DEBUG_MSG(("WPSContentListener::_handleListChange: can not find any list\n"));
			return;
		}
		m_ps->m_list->setLevel(m_ps->m_currentListLevel);
		m_ps->m_list->openElement();

		if (m_ps->m_list->mustSendLevel(m_ps->m_currentListLevel))
		{
			if (actualListLevel == m_ps->m_currentListLevel)
			{
				if (m_ps->m_listOrderedLevels[actualListLevel-1])
					m_documentInterface->closeOrderedListLevel();
				else
					m_documentInterface->closeUnorderedListLevel();
				actualListLevel--;
			}
			if (m_ps->m_currentListLevel==1)
			{
				// we must change the listID for writerperfect
				int prevId;
				if ((prevId=m_ps->m_list->getPreviousId()) > 0)
					m_ps->m_list->setId(prevId);
				else
					m_ps->m_list->setId(++m_ps->m_newListId);
			}
			m_ps->m_list->sendTo(*m_documentInterface, m_ps->m_currentListLevel);
		}

		propList2.insert("libwpd:id", m_ps->m_list->getId());
		m_ps->m_list->closeElement();
	}

	if (actualListLevel == m_ps->m_currentListLevel) return;

	m_ps->m_listOrderedLevels.resize(m_ps->m_currentListLevel, false);
	for (int i=actualListLevel+1; i<= m_ps->m_currentListLevel; i++)
	{
		if (m_ps->m_list->isNumeric(i))
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

////////////////////////////////////////////////////////////
static char const *WPS_DEFAULT_SUPER_SUB_SCRIPT = "58";
void WPSContentListener::_openSpan()
{
	if (m_ps->m_isSpanOpened)
		return;

	if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
		return;

	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
		_changeList();
	if (m_ps->m_currentListLevel == 0)
		_openParagraph();
	else
		_openListElement();

	// The behaviour of WP6+ is following: if an attribute bit is set in the cell attributes, we cannot
	// unset it; if it is set, we can set or unset it
	uint32_t attributeBits = (m_ps->m_textAttributeBits | m_ps->m_cellAttributeBits);
	uint8_t fontSizeAttributes;
	double fontSizeChange;
	// the font size attribute bits are mutually exclusive and the cell attributes prevail
	if ((m_ps->m_cellAttributeBits & 0x0000001f) != 0x00000000)
		fontSizeAttributes = (uint8_t)(m_ps->m_cellAttributeBits & 0x0000001f);
	else
		fontSizeAttributes = (uint8_t)(m_ps->m_textAttributeBits & 0x0000001f);
	switch (fontSizeAttributes)
	{
	case 0x01:  // Extra large
		fontSizeChange = 2.0;
		break;
	case 0x02: // Very large
		fontSizeChange = 1.5f;
		break;
	case 0x04: // Large
		fontSizeChange = 1.2f;
		break;
	case 0x08: // Small print
		fontSizeChange = 0.8f;
		break;
	case 0x10: // Fine print
		fontSizeChange = 0.6f;
		break;
	default: // Normal
		fontSizeChange = 1.0;
		break;
	}

	WPXPropertyList propList;
	if (attributeBits & WPS_SUPERSCRIPT_BIT)
	{
		WPXString sSuperScript;
		sSuperScript.sprintf("super %s", WPS_DEFAULT_SUPER_SUB_SCRIPT);
		sSuperScript.append("%");
		propList.insert("style:text-position", sSuperScript);
	}
	else if (attributeBits & WPS_SUBSCRIPT_BIT)
	{
		WPXString sSubScript;
		sSubScript.sprintf("sub %s", WPS_DEFAULT_SUPER_SUB_SCRIPT);
		sSubScript.append("%");
		propList.insert("style:text-position", sSubScript);
	}
	if (attributeBits & WPS_ITALICS_BIT)
		propList.insert("fo:font-style", "italic");
	if (attributeBits & WPS_BOLD_BIT)
		propList.insert("fo:font-weight", "bold");
	if (attributeBits & WPS_STRIKEOUT_BIT)
		propList.insert("style:text-line-through-type", "single");
	if (attributeBits & WPS_DOUBLE_UNDERLINE_BIT)
		propList.insert("style:text-underline-type", "double");
	else if (attributeBits & WPS_UNDERLINE_BIT)
		propList.insert("style:text-underline-type", "single");
	if (attributeBits & WPS_OVERLINE_BIT)
		propList.insert("style:text-overline-type", "single");
	if (attributeBits & WPS_OUTLINE_BIT)
		propList.insert("style:text-outline", "true");
	if (attributeBits & WPS_SMALL_CAPS_BIT)
		propList.insert("fo:font-variant", "small-caps");
	if (attributeBits & WPS_BLINK_BIT)
		propList.insert("style:text-blinking", "true");
	if (attributeBits & WPS_SHADOW_BIT)
		propList.insert("fo:text-shadow", "1pt 1pt");
	if (attributeBits & WPS_HIDDEN_BIT)
		propList.insert("text:display", "none");
	if (attributeBits & WPS_ALL_CAPS_BIT)
		propList.insert("fo:text-transform", "uppercase");
	if (attributeBits & WPS_EMBOSS_BIT)
		propList.insert("style:font-relief", "embossed");
	else if (attributeBits & WPS_ENGRAVE_BIT)
		propList.insert("style:font-relief", "engraved");

	if (m_ps->m_fontName.len())
		propList.insert("style:font-name", m_ps->m_fontName.cstr());

	propList.insert("fo:font-size", fontSizeChange*m_ps->m_fontSize, WPX_POINT);

	char color[20];
	sprintf(color,"#%06x",m_ps->m_fontColor);
	propList.insert("fo:color", color);

	if (m_ps->m_textLanguage < 0)
		_addLanguage(0x409, propList);
	if (m_ps->m_textLanguage > 0 && m_ps->m_textLanguage != m_ps->m_paragraphLanguage)
		_addLanguage(m_ps->m_textLanguage, propList);

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

////////////////////////////////////////////////////////////
void WPSContentListener::_flushDeferredTabs()
{
	if (m_ps->m_numDeferredTabs == 0) return;

	// CHECKME: the tab are not underline even if the underline bit is set
	uint32_t oldTextAttributes = m_ps->m_textAttributeBits;
	uint32_t newAttributes = oldTextAttributes & (~WPS_UNDERLINE_BIT) &
	                         (~WPS_OVERLINE_BIT);
	if (oldTextAttributes != newAttributes) setFontAttributes(newAttributes);
	if (!m_ps->m_isSpanOpened) _openSpan();
	for (; m_ps->m_numDeferredTabs > 0; m_ps->m_numDeferredTabs--)
		m_documentInterface->insertTab();
	if (oldTextAttributes != newAttributes) setFontAttributes(oldTextAttributes);
}

void WPSContentListener::_flushText()
{
	if (m_ps->m_textBuffer.len() == 0) return;

	// when some many ' ' follows each other, call insertSpace
	WPXString tmpText;
	int numConsecutiveSpaces = 0;
	WPXString::Iter i(m_ps->m_textBuffer);
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

/**
Creates an new document state. Saves the old state on a "stack".
*/
void WPSContentListener::handleSubDocument(WPSSubDocumentPtr &subDocument, libwps::SubDocumentType subDocumentType)
{
	_pushParsingState();
	m_ps->m_subDocumentType = subDocumentType;
	m_ps->m_inSubDocument = true;

	m_ps->m_isDocumentStarted = true;
	m_ps->m_isPageSpanOpened = true;
	m_ps->m_list.reset();

	if (m_ps->m_subDocumentType == libwps::DOC_TEXT_BOX)
	{
		m_ps->m_pageMarginLeft = 0.0;
		m_ps->m_pageMarginRight = 0.0;
		m_ps->m_sectionAttributesChanged = true;
	}

	// Check whether the document is calling itself
	bool sendDoc = true;
	for (int i = 0; i < int(m_ps->m_subDocuments.size()); i++)
	{
		if (!subDocument)
			break;
		if (subDocument == m_ps->m_subDocuments[i])
		{
			WPS_DEBUG_MSG(("WPSContentListener::handleSubDocument: recursif call, stop...\n"));
			sendDoc = false;
			break;
		}
	}
	if (sendDoc)
	{
		if (subDocumentType == libwps::DOC_HEADER_FOOTER)
			m_ps->m_isHeaderFooterWithoutParagraph = true;
		if (subDocument)
		{
			m_ps->m_inSubDocument = true;
			m_ps->m_subDocumentType = subDocumentType;
			m_ps->m_subDocuments.push_back(subDocument);

			shared_ptr<WPSContentListener> listen(this, WPS_shared_ptr_noop_deleter<WPSContentListener>());
			try
			{
				subDocument->parse(listen, subDocumentType);
			}
			catch(...)
			{
				WPS_DEBUG_MSG(("Works: WPSContentListener::handleSubDocument exception catched \n"));
			}
		}
		if (m_ps->m_isHeaderFooterWithoutParagraph)
		{
			_openSpan();
			_closeParagraph();
		}
	}

	if (m_ps->m_subDocumentType == libwps::DOC_TEXT_BOX)
		_closeSection();
	_popParsingState();
}

void WPSContentListener::_closeTable()
{
}
double WPSContentListener::_movePositionToFirstColumn(double position)
{
	if (m_ps->m_numColumns <= 1)
		return position;
	double tempSpaceRemaining = position - m_ps->m_pageMarginLeft - m_ps->m_sectionMarginLeft;
	position -= m_ps->m_textColumns[0].m_leftGutter;
	for (int i = 0; i < (int)(m_ps->m_textColumns.size() - 1); i++)
	{
		if ((tempSpaceRemaining -= m_ps->m_textColumns[i].m_width - m_ps->m_textColumns[i].m_rightGutter) > 0)
		{
			position -= m_ps->m_textColumns[i].m_width - m_ps->m_textColumns[i].m_leftGutter
			            + m_ps->m_textColumns[i+1].m_leftGutter;
			tempSpaceRemaining -= m_ps->m_textColumns[i].m_rightGutter;
		}
		else
			return position;
	}
	return position;
}

// ---------- others ------------------
void WPSContentListener::_addLanguage(int lcid, WPXPropertyList &propList)
{
	if (lcid < 0) return;
	std::string lang = libwps_tools_win::Language::localeName(lcid);
	if (lang.length())
	{
		std::string language(lang);
		std::string country("none");
		if (lang.length() > 3 && lang[2]=='_')
		{
			country=lang.substr(3);
			language=lang.substr(0,2);
		}
		propList.insert("fo:language", language.c_str());
		propList.insert("fo:country", country.c_str());
	}
	else
	{
		propList.insert("fo:language", "none");
		propList.insert("fo:country", "none");
	}
}

// ---------- state stack ------------------
shared_ptr<WPSContentParsingState> WPSContentListener::_pushParsingState()
{
	shared_ptr<WPSContentParsingState> actual = m_ps;
	m_psStack.push_back(actual);
	m_ps.reset(new WPSContentParsingState(*actual.get()));
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
