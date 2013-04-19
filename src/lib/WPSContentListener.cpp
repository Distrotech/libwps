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

#include <libwpd/libwpd.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSContentListener.h"

#include "WPSCell.h"
#include "WPSList.h"
#include "WPSPageSpan.h"
#include "WPSParagraph.h"
#include "WPSPosition.h"
#include "WPSSubDocument.h"

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

WPSContentParsingState::WPSContentParsingState() :
	m_textBuffer(""), m_numDeferredTabs(0),

	m_textAttributeBits(0),	m_fontSize(12.0), m_fontName("Times New Roman"),
	m_fontColor(0),	m_textLanguage(-1),

	m_isParagraphColumnBreak(false), m_isParagraphPageBreak(false),

	m_paragraphJustification(libwps::JustificationLeft),
	m_paragraphLineSpacing(1.0), m_paragraphLineSpacingUnit(WPX_PERCENT),
	m_paragraphBackgroundColor(0xFFFFFF),
	m_paragraphBorders(0), m_paragraphBordersStyle(),

	m_list(), m_currentListLevel(0),

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

	m_listReferencePosition(0.0), m_listBeginPosition(0.0), m_listOrderedLevels(),

	m_alignmentCharacter('.'),
	m_tabStops(),
	m_isTabPositionRelative(false),

	m_inSubDocument(false),
	m_isNote(false),
	m_subDocumentType(libwps::DOC_NONE)
{
}

WPSContentParsingState::~WPSContentParsingState()
{
}

WPSContentListener::WPSContentListener(std::vector<WPSPageSpan> const &pageList, WPXDocumentInterface *documentInterface) :
	m_ds(new WPSDocumentParsingState(pageList)), m_ps(new WPSContentParsingState), m_psStack(),
	m_documentInterface(documentInterface)
{
	_updatePageSpanDependent(true);
	_recomputeParagraphPositions();
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
	if (m_ps->m_textAttributeBits & s_subsuperBits)
		m_ps->m_textAttributeBits &= ~s_subsuperBits;
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

void WPSContentListener::_insertBreakIfNecessary(WPXPropertyList &propList)
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
	setFontAttributes(font.m_attributes);
	if (font.m_size > 0)
		setFontSize((uint16_t) font.m_size);
	if (!font.m_name.empty())
		setTextFont(font.m_name.c_str());
	setTextColor(font.m_color);
	if (font.m_languageId > 0)
		setTextLanguage(font.m_languageId);
}

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
	if (m_ps->m_fontSize<fSize || m_ps->m_fontSize>fSize)
	{
		_closeSpan();
		m_ps->m_fontSize=fSize;
	}
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
	_recomputeParagraphPositions();
}

void WPSContentListener::setParagraphMargin(double margin, int pos)
{
	switch(pos)
	{
	case WPS_LEFT:
		m_ps->m_leftMarginByParagraphMarginChange = margin;
		_recomputeParagraphPositions();
		break;
	case WPS_RIGHT:
		m_ps->m_rightMarginByParagraphMarginChange = margin;
		_recomputeParagraphPositions();
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

void WPSContentListener::setParagraphBackgroundColor(uint32_t color)
{
	m_ps->m_paragraphBackgroundColor = color;
}

void WPSContentListener::setParagraphBorders(int which, WPSBorder style)
{
	m_ps->m_paragraphBorders = which;
	m_ps->m_paragraphBordersStyle = style;
	if (style.m_width <= 0)
		m_ps->m_paragraphBordersStyle.m_width = 1;
}

///////////////////
// List: Minimal implementation
///////////////////
void WPSContentListener::setCurrentListLevel(int level)
{
	m_ps->m_currentListLevel = (uint8_t)level;
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
	case Link:
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

	size_t numCols = colsWidth.size();
	if (numCols <= 1)
		m_ps->m_textColumns.resize(0);
	else
	{
		float factor = 1.0;
		switch(unit)
		{
		case WPX_POINT:
		case WPX_TWIP:
			factor = WPSPosition::getScaleFactor(unit, WPX_INCH);
			break;
		case WPX_INCH:
			break;
		case WPX_PERCENT:
		case WPX_GENERIC:
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
	m_ds->m_metaData.insert("libwpd:language", lang.c_str());
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

	m_ps->m_currentListLevel = 0;
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
		actPage+=(unsigned) it->getPageSpan();
		it++;
		if (it == m_ds->m_pageList.end())
		{
			WPS_DEBUG_MSG(("WPSContentListener::_openPageSpan: can not find current page\n"));
			throw libwps::ParseException();
		}
	}
	WPSPageSpan &currentPage = *it;

	WPXPropertyList propList;
	currentPage.getPageProperty(propList);
	propList.insert("libwpd:is-last-page-span", ((m_ps->m_currentPage + 1 == m_ds->m_pageList.size()) ? true : false));

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
	_recomputeParagraphPositions();

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
	if (m_ps->m_sectionMarginLeft < 0 || m_ps->m_sectionMarginLeft > 0)
		m_ps->m_sectionMarginLeft += deltaLeft;
	if (m_ps->m_sectionMarginRight < 0 || m_ps->m_sectionMarginRight > 0)
		m_ps->m_sectionMarginRight += deltaRight;
	m_ps->m_listReferencePosition += deltaLeft;
	m_ps->m_listBeginPosition += deltaLeft;
}

void WPSContentListener::_recomputeParagraphPositions()
{
	m_ps->m_paragraphMarginLeft = m_ps->m_leftMarginByPageMarginChange
	                              + m_ps->m_leftMarginByParagraphMarginChange + m_ps->m_leftMarginByTabs;
	m_ps->m_paragraphMarginRight = m_ps->m_rightMarginByPageMarginChange
	                               + m_ps->m_rightMarginByParagraphMarginChange + m_ps->m_rightMarginByTabs;
	m_ps->m_paragraphTextIndent = m_ps->m_textIndentByParagraphIndentChange + m_ps->m_textIndentByTabs;
	m_ps->m_listBeginPosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
	m_ps->m_listReferencePosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
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

	WPXPropertyList propList;
	propList.insert("fo:margin-left", m_ps->m_sectionMarginLeft);
	propList.insert("fo:margin-right", m_ps->m_sectionMarginRight);
	if (m_ps->m_numColumns > 1)
		propList.insert("text:dont-balance-text-columns", false);
	if (m_ps->m_sectionMarginTop < 0 || m_ps->m_sectionMarginTop > 0)
		propList.insert("libwpd:margin-top", m_ps->m_sectionMarginTop);
	if (m_ps->m_sectionMarginBottom < 0 || m_ps->m_sectionMarginBottom > 0)
		propList.insert("libwpd:margin-bottom", m_ps->m_sectionMarginBottom);

	WPXPropertyListVector columns;
	for (size_t i = 0; i < m_ps->m_textColumns.size(); i++)
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
	m_ps->m_leftMarginByTabs = 0.0;
	m_ps->m_rightMarginByTabs = 0.0;
	m_ps->m_textIndentByTabs = 0.0;
	m_ps->m_isTextColumnWithoutParagraph = false;
	m_ps->m_isHeaderFooterWithoutParagraph = false;
	_recomputeParagraphPositions();
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
	default:
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
		if (m_ps->m_paragraphBackgroundColor !=  0xFFFFFF)
		{
			std::stringstream stream;
			stream << "#" << std::hex << std::setfill('0') << std::setw(6)
			       << (m_ps->m_paragraphBackgroundColor&0xFFFFFF);
			propList.insert("fo:background-color", stream.str().c_str());
		}
		if (m_ps->m_paragraphBorders &&
		        m_ps->m_paragraphBordersStyle.m_style != WPSBorder::None)
		{
			std::string style = m_ps->m_paragraphBordersStyle.getPropertyValue();
			int border = m_ps->m_paragraphBorders;
			if (border == 0xF)
			{
				propList.insert("fo:border", style.c_str());
				return;
			}
			if (border & WPSBorder::LeftBit)
				propList.insert("fo:border-left", style.c_str());
			if (border & WPSBorder::RightBit)
				propList.insert("fo:border-right", style.c_str());
			if (border & WPSBorder::TopBit)
				propList.insert("fo:border-top", style.c_str());
			if (border & WPSBorder::BottomBit)
				propList.insert("fo:border-bottom", style.c_str());
		}
	}
	propList.insert("fo:margin-top", m_ps->m_paragraphMarginTop, m_ps->m_paragraphMarginBottomUnit);
	propList.insert("fo:margin-bottom", m_ps->m_paragraphMarginBottom, m_ps->m_paragraphMarginBottomUnit);
	propList.insert("fo:line-height", m_ps->m_paragraphLineSpacing, m_ps->m_paragraphLineSpacingUnit);


	if (!m_ps->m_inSubDocument && m_ps->m_firstParagraphInPageSpan)
	{
		unsigned actPage = 1;
		std::vector<WPSPageSpan>::const_iterator it = m_ds->m_pageList.begin();
		while(actPage < m_ps->m_currentPage)
		{
			if (it == m_ds->m_pageList.end())
				break;

			actPage+=unsigned(it->getPageSpan());
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
	for (size_t i=0; i< m_ps->m_tabStops.size(); i++)
		m_ps->m_tabStops[i].addTo(tabStops, decalX);
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
	size_t actualListLevel = m_ps->m_listOrderedLevels.size();
	for (size_t i=actualListLevel; i > m_ps->m_currentListLevel; i--)
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
					m_ps->m_list->setId(++m_ds->m_newListId);
			}
			m_ps->m_list->sendTo(*m_documentInterface, m_ps->m_currentListLevel);
		}

		propList2.insert("libwpd:id", m_ps->m_list->getId());
		m_ps->m_list->closeElement();
	}

	if (actualListLevel == m_ps->m_currentListLevel) return;

	m_ps->m_listOrderedLevels.resize(m_ps->m_currentListLevel, false);
	for (size_t i=actualListLevel+1; i<= m_ps->m_currentListLevel; i++)
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
		if (m_ps->m_currentListLevel == 0)
			_openParagraph();
		else
			_openListElement();
	}

	uint32_t attributeBits = m_ps->m_textAttributeBits;
	double fontSizeChange = 1.0;
	switch (attributeBits& 0x0000001f)
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
		propList.insert("style:text-position", "super 58%");
	else if (attributeBits & WPS_SUBSCRIPT_BIT)
		propList.insert("style:text-position", "sub 58%");
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

	WPXString color;
	color.sprintf("#%06x",m_ps->m_fontColor);
	propList.insert("fo:color", color);

	if (m_ps->m_textLanguage < 0)
		_addLanguage(0x409, propList);
	if (m_ps->m_textLanguage > 0)
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

///////////////////
// text (send data)
///////////////////
void WPSContentListener::_flushDeferredTabs()
{
	if (m_ps->m_numDeferredTabs == 0) return;

	// CHECKME: the tab are not underline even if the underline bit is set
	uint32_t oldTextAttributes = m_ps->m_textAttributeBits;
	uint32_t newAttributes = oldTextAttributes & uint32_t(~WPS_UNDERLINE_BIT) &
	                         uint32_t(~WPS_OVERLINE_BIT);
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
	WPXString label("");
	insertLabelNote(noteType, label, subDocument);
}

void WPSContentListener::insertLabelNote(const NoteType noteType, WPXString const &label, WPSSubDocumentPtr &subDocument)
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
		int prevListLevel = m_ps->m_currentListLevel;
		m_ps->m_currentListLevel = 0;
		_changeList(); // flush the list exterior
		handleSubDocument(subDocument, libwps::DOC_NOTE);
		m_ps->m_currentListLevel = (uint8_t)prevListLevel;
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

		WPXPropertyList propList;
		if (label.len())
			propList.insert("text:label", label);
		if (noteType == FOOTNOTE)
		{
			propList.insert("libwpd:number", ++(m_ds->m_footNoteNumber));
			m_documentInterface->openFootnote(propList);
		}
		else
		{
			propList.insert("libwpd:number", ++(m_ds->m_endNoteNumber));
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

	WPXPropertyList propList;
	m_documentInterface->openComment(propList);

	m_ps->m_isNote = true;
	handleSubDocument(subDocument, libwps::DOC_COMMENT_ANNOTATION);

	m_documentInterface->closeComment();
	m_ps->m_isNote = false;
}

void WPSContentListener::insertTextBox
(WPSPosition const &pos, WPSSubDocumentPtr subDocument, WPXPropertyList frameExtras)
{
	if (!_openFrame(pos, frameExtras)) return;

	WPXPropertyList propList;
	m_documentInterface->openTextBox(propList);
	handleSubDocument(subDocument, libwps::DOC_TEXT_BOX);
	m_documentInterface->closeTextBox();

	_closeFrame();
}

void WPSContentListener::insertPicture
(WPSPosition const &pos, const WPXBinaryData &binaryData, std::string type,
 WPXPropertyList frameExtras)
{
	if (!_openFrame(pos, frameExtras)) return;

	WPXPropertyList propList;
	propList.insert("libwpd:mimetype", type.c_str());
	m_documentInterface->insertBinaryObject(propList, binaryData);

	_closeFrame();
}

///////////////////
// frame
///////////////////
bool WPSContentListener::_openFrame(WPSPosition const &pos, WPXPropertyList extras)
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

	WPXPropertyList propList(extras);
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
( WPXPropertyList &propList, WPSPosition const &pos)
{
	Vec2f origin = pos.origin();
	WPXUnit unit = pos.unit();
	float inchFactor=pos.getInvUnitScale(WPX_INCH);
	float pointFactor = pos.getInvUnitScale(WPX_POINT);

	propList.insert("svg:width", double(pos.size()[0]), unit);
	propList.insert("svg:height", double(pos.size()[1]), unit);
	if (pos.naturalSize().x() > 4*pointFactor && pos.naturalSize().y() > 4*pointFactor)
	{
		propList.insert("libwpd:naturalWidth", pos.naturalSize().x(), pos.unit());
		propList.insert("libwpd:naturalHeight", pos.naturalSize().y(), pos.unit());
	}

	double newPosition;

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
		           - m_ps->m_pageMarginRight - m_ps->m_sectionMarginLeft
		           - m_ps->m_sectionMarginRight - m_ps->m_paragraphMarginLeft
		           - m_ps->m_paragraphMarginRight;
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

	m_ps->m_currentListLevel = 0;
	_changeList(); // flush the list exterior
}

///////////////////
// table
///////////////////
void WPSContentListener::openTable(std::vector<float> const &colWidth, WPXUnit unit)
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

	WPXPropertyList propList;
	propList.insert("table:align", "left");
	propList.insert("fo:margin-left", 0.0);

	float tableWidth = 0;
	WPXPropertyListVector columns;

	size_t nCols = colWidth.size();
	for (size_t c = 0; c < nCols; c++)
	{
		WPXPropertyList column;
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

void WPSContentListener::openTableRow(float h, WPXUnit unit, bool headerRow)
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
	WPXPropertyList propList;
	propList.insert("libwpd:is-header-row", headerRow);

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
	WPXPropertyList propList;
	propList.insert("libwpd:column", pos[0]);
	propList.insert("libwpd:row", pos[1]);
	propList.insert("table:number-columns-spanned", span[0]);
	propList.insert("table:number-rows-spanned", span[1]);
	m_documentInterface->openTableCell(propList);
	m_documentInterface->closeTableCell();
}

void WPSContentListener::openTableCell(WPSCell const &cell, WPXPropertyList const &extras)
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

	WPXPropertyList propList(extras);
	propList.insert("libwpd:column", cell.position()[0]);
	propList.insert("libwpd:row", cell.position()[1]);

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
	m_ps->m_currentListLevel = 0;
	_changeList(); // flush the list exterior

	m_ps->m_isTableCellOpened = false;
	m_documentInterface->closeTableCell();
}

///////////////////
// others
///////////////////
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
