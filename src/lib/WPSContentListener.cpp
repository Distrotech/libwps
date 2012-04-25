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

/*
 * This file is in sync with CVS
 * /libwpd2/src/lib/WPXContentListener.cpp 1.12
 */

#include <stdio.h>

#include <libwpd/WPXDocumentInterface.h>
#include <libwpd/WPXProperty.h>

#include "libwps_tools_win.h"
#include "WPSPageSpan.h"

#include "WPSList.h"

#include "WPSContentListener.h"

namespace WPSContentListenerInternal
{
struct ListSignature
{
	uint16_t m_numbering,m_numstyle,m_numsep;

	bool operator == (ListSignature &y)
	{
		return (m_numbering == y.m_numbering) && (m_numstyle == y.m_numstyle) && (m_numsep == y.m_numsep);
	}
};
}

WPSContentParsingState::WPSContentParsingState() :
	m_textAttributeBits(0),
	m_fontSize(12.0f),
	m_fontName("Times New Roman"),
	m_languageId(0x409/*en-US*/),
	m_textcolor(0),

	m_fieldcode(0),

	m_isParagraphColumnBreak(false),
	m_isParagraphPageBreak(false),
	m_paragraphJustification(libwps::JustificationLeft),
	m_paragraphLineSpacing(1.0f),

	m_isDocumentStarted(false),
	m_isPageSpanOpened(false),m_isSectionOpened(false),
	m_isPageSpanBreakDeferred(false),

	m_isSpanOpened(false), m_isParagraphOpened(false),

	m_inSubDocument(false), m_isListElementOpened(false),
	m_isTableOpened(false), m_isTableCellOpened(false),

	m_isParaListItem(false),

	m_nextPageSpanIter(),
	m_numPagesRemainingInSpan(0),

	m_sectionAttributesChanged(false),

	m_pageFormLength(11.0f),
	m_pageFormWidth(8.5f),
	m_pageFormOrientationIsPortrait(true),

	m_pageMarginLeft(1.0f),
	m_pageMarginRight(1.0f),

	m_paragraphMarginLeft(0.0f),
	m_paragraphMarginRight(0.0f),
	m_paragraphMarginTop(0.0f),
	m_paragraphMarginBottom(0.0f),
	m_paragraphTextIndent(0.0f),

	m_textBuffer(),
	m_list(), m_listOrderedLevels(),
	m_actualListId(0), m_currentListLevel(0),
	m_listReferencePosition(0.0),  m_listBeginPosition(0.0)
{
}

WPSContentParsingState::~WPSContentParsingState()
{
}

WPSContentListener::WPSContentListener(std::vector<WPSPageSpan> const &pageList, WPXDocumentInterface *documentInterface) :
	m_ps(new WPSContentParsingState),
	m_documentInterface(documentInterface),
	m_metaData(), m_tabs(),
	m_pageList(pageList),
	m_listFormats()
{
	m_ps->m_nextPageSpanIter = m_pageList.begin();
}

WPSContentListener::~WPSContentListener()
{
}

void WPSContentListener::startDocument()
{
	if (!m_ps->m_isDocumentStarted)
	{
		// FIXME: this is stupid, we should store a property list filled with the relevant metadata
		// and then pass that directly..

		m_documentInterface->setDocumentMetaData(m_metaData);

		m_documentInterface->startDocument();
	}

	m_ps->m_isDocumentStarted = true;
}

void WPSContentListener::endDocument()
{
	if (!m_ps->m_isPageSpanOpened)
		_openSpan();

	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	m_ps->m_currentListLevel = 0;
	_changeList(); // flush the list exterior

	// close the document nice and tight
	_closeSection();
	_closePageSpan();
	m_documentInterface->endDocument();
}

void WPSContentListener::_openSection()
{
	if (!m_ps->m_isSectionOpened)
	{
		if (!m_ps->m_isPageSpanOpened)
			_openPageSpan();

		WPXPropertyList propList;

		WPXPropertyListVector columns;
		if (!m_ps->m_isSectionOpened)
			m_documentInterface->openSection(propList, columns);

		m_ps->m_sectionAttributesChanged = false;
		m_ps->m_isSectionOpened = true;
	}
}

void WPSContentListener::_closeSection()
{
	if (!m_ps->m_isSectionOpened)
		return;

	if (m_ps->m_isParagraphOpened)
		_closeParagraph();
	_changeList();

	m_documentInterface->closeSection();

	m_ps->m_sectionAttributesChanged = false;
	m_ps->m_isSectionOpened = false;
}

void WPSContentListener::_openPageSpan()
{
	if (m_ps->m_isPageSpanOpened)
		return;

	if (!m_ps->m_isDocumentStarted)
		startDocument();

	if ( m_pageList.empty() || (m_ps->m_nextPageSpanIter == m_pageList.end()))
	{
		WPS_DEBUG_MSG(("m_pageList.empty() || (m_ps->m_nextPageSpanIter == m_pageList.end())\n"));
		throw libwps::ParseException();
	}

	WPSPageSpan &currentPage = (*m_ps->m_nextPageSpanIter);
	//currentPage.makeConsistent(1);

	if (!m_ps->m_isPageSpanOpened)
	{
		WPXPropertyList propList;
		currentPage.getPageProperty(propList);
		std::vector<WPSPageSpan>::iterator lastPageSpan = --m_pageList.end();
		propList.insert("libwpd:is-last-page-span", ((m_ps->m_nextPageSpanIter == lastPageSpan) ? true : false));
		m_documentInterface->openPageSpan(propList);
	}

	m_ps->m_isPageSpanOpened = true;

	currentPage.sendHeaderFooters(this, m_documentInterface);

	/* Some of this would maybe not be necessary, but it does not do any harm
	 * and apparently solves some troubles */
	m_ps->m_pageFormLength = currentPage.getFormLength();
	m_ps->m_pageFormWidth = currentPage.getFormWidth();
	m_ps->m_pageFormOrientationIsPortrait =
	    (currentPage.getFormOrientation()==WPSPageSpan::PORTRAIT);
	m_ps->m_pageMarginLeft = currentPage.getMarginLeft();
	m_ps->m_pageMarginRight = currentPage.getMarginRight();

	m_ps->m_numPagesRemainingInSpan = (currentPage.getPageSpan() - 1);
	m_ps->m_nextPageSpanIter++;
}

void WPSContentListener::_closePageSpan()
{
	if (m_ps->m_isPageSpanOpened)
	{
		if (m_ps->m_isSectionOpened)
			_closeSection();

		m_documentInterface->closePageSpan();
	}

	m_ps->m_isPageSpanOpened = false;
	m_ps->m_isPageSpanBreakDeferred = false;
}

void WPSContentListener::_openParagraph()
{
	if (m_ps->m_isParagraphOpened)
		return;

	if (m_ps->m_sectionAttributesChanged)
		_closeSection();

	if (!m_ps->m_isSectionOpened)
		_openSection();

	WPXPropertyListVector tabStops;
	WPXPropertyList propList;
	_appendParagraphProperties(propList, false);
	_getTabStops(tabStops);
	_resetParagraphState();

	m_documentInterface->openParagraph(propList, tabStops);

	m_ps->m_isParagraphColumnBreak = false;
	m_ps->m_isParagraphPageBreak = false;
	m_ps->m_isParagraphOpened = true;
}

void WPSContentListener::_closeParagraph()
{
	if (m_ps->m_isParagraphOpened)
	{
		if (m_ps->m_isSpanOpened)
			_closeSpan();

		if (m_ps->m_isListElementOpened)
			_closeListElement();
		else
			m_documentInterface->closeParagraph();
	}

	m_ps->m_isParagraphOpened = false;

	if (m_ps->m_isPageSpanBreakDeferred)
		_closePageSpan();
}

void WPSContentListener::_openSpan()
{
	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
		_changeList();
	if (m_ps->m_currentListLevel == 0)
		_openParagraph();
	else
		_openListElement();

	uint8_t fontSizeAttributes = (uint8_t)(m_ps->m_textAttributeBits & 0x0000001f);
	float fontSizeChange = 0.0f;
	switch (fontSizeAttributes)
	{
	case 0x01:  // Extra large
		fontSizeChange = 2.0f;
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
		fontSizeChange = 1.0f;
		break;
	}

	WPXPropertyList propList;
	if (m_ps->m_textAttributeBits & WPS_SUPERSCRIPT_BIT)
		propList.insert("style:text-position", "super 58%");
	else if (m_ps->m_textAttributeBits & WPS_SUBSCRIPT_BIT)
		propList.insert("style:text-position", "sub 58%");
	if (m_ps->m_textAttributeBits & WPS_ITALICS_BIT)
		propList.insert("fo:font-style", "italic");
	if (m_ps->m_textAttributeBits & WPS_BOLD_BIT)
		propList.insert("fo:font-weight", "bold");
	if (m_ps->m_textAttributeBits & WPS_STRIKEOUT_BIT)
		propList.insert("style:text-line-through-type", "single");
	if (m_ps->m_textAttributeBits & WPS_DOUBLE_UNDERLINE_BIT)
		propList.insert("style:text-underline-type", "double");
	else if (m_ps->m_textAttributeBits & WPS_UNDERLINE_BIT)
		propList.insert("style:text-underline-type", "single");
	if (m_ps->m_textAttributeBits & WPS_OUTLINE_BIT)
		propList.insert("style:text-outline", "true");
	if (m_ps->m_textAttributeBits & WPS_SMALL_CAPS_BIT)
		propList.insert("fo:font-variant", "small-caps");
	if (m_ps->m_textAttributeBits & WPS_ALL_CAPS_BIT)
		propList.insert("fo:text-transform", "uppercase");
	if (m_ps->m_textAttributeBits & WPS_BLINK_BIT)
		propList.insert("style:text-blinking", "true");
	if (m_ps->m_textAttributeBits & WPS_SHADOW_BIT)
		propList.insert("fo:text-shadow", "1pt 1pt");
	if (m_ps->m_textAttributeBits & WPS_EMBOSS_BIT)
		propList.insert("style:font-relief", "embossed");
	else if (m_ps->m_textAttributeBits & WPS_ENGRAVE_BIT)
		propList.insert("style:font-relief", "engraved");
	if (m_ps->m_languageId)
	{
		std::string lang = libwps_tools_win::Language::localeName(m_ps->m_languageId);
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

	if (m_ps->m_fontName.len())
		propList.insert("style:font-name", m_ps->m_fontName.cstr());
	propList.insert("fo:font-size", fontSizeChange*m_ps->m_fontSize, WPX_POINT);
	// Here we give the priority to the redline bit over the font color.
	// When redline finishes, the color is back.
	if (m_ps->m_textAttributeBits & WPS_REDLINE_BIT)
		propList.insert("fo:color", "#ff3333");  // #ff3333 = a nice bright red
	else
	{
		char color[20];
		sprintf(color,"#%06x",m_ps->m_textcolor);
		propList.insert("fo:color", color);
	}

	if (!m_ps->m_isSpanOpened)
		m_documentInterface->openSpan(propList);

	m_ps->m_isSpanOpened = true;
}

void WPSContentListener::_closeSpan()
{
	if (m_ps->m_isSpanOpened)
	{
		_flushText();

		m_documentInterface->closeSpan();
	}

	m_ps->m_isSpanOpened = false;
}

void WPSContentListener::insertBreak(const uint8_t breakType)
{
	switch (breakType)
	{
	case WPS_COLUMN_BREAK:
		if (!m_ps->m_isPageSpanOpened)
			_openSpan();
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		m_ps->m_isParagraphColumnBreak = true;
		break;
	case WPS_PAGE_BREAK:
		if (!m_ps->m_isPageSpanOpened)
			_openSpan();
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		m_ps->m_isParagraphPageBreak = true;
		break;
		// TODO: (.. line break?)
	}

	switch (breakType)
	{
	case WPS_PAGE_BREAK:
	case WPS_SOFT_PAGE_BREAK:
		if (m_ps->m_numPagesRemainingInSpan > 0)
			m_ps->m_numPagesRemainingInSpan--;
		else
		{
			if (!m_ps->m_isParagraphOpened)
				_closePageSpan();
			else
				m_ps->m_isPageSpanBreakDeferred = true;
		}
	default:
		break;
	}
}

void WPSContentListener::setTextFont(const WPXString fontName)
{
	_closeSpan();
	m_ps->m_fontName = fontName;
}

void WPSContentListener::setFontSize(const uint16_t fontSize)
{
	_closeSpan();
	m_ps->m_fontSize=float(fontSize);
}

void WPSContentListener::setFontAttributes(const uint32_t fontAttributes)
{
	_closeSpan();
	m_ps->m_textAttributeBits=fontAttributes;
}

void WPSContentListener::setTextLanguage(const uint32_t lcid)
{
	_closeSpan();
	m_ps->m_languageId=lcid;
}

void WPSContentListener::setTextColor(const unsigned int rgb)
{
	_closeSpan();
	m_ps->m_textcolor = rgb;
}

void WPSContentListener::setAlign(libwps::Justification align)
{
	m_ps->m_paragraphJustification = align;
}

void WPSContentListener::setParagraphTextIndent(double margin)
{
	m_ps->m_paragraphTextIndent = margin;
}

void WPSContentListener::setParagraphMargin(double margin, int pos)
{
	switch(pos)
	{
	case WPS_LEFT:
		m_ps->m_paragraphMarginLeft = margin;
		break;
	case WPS_RIGHT:
		m_ps->m_paragraphMarginRight = margin;
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

void WPSContentListener::setTabs(std::vector<WPSTabStop> &tabs)
{
	m_tabs = tabs;
}

void WPSContentListener::insertEOL()
{
	if (!m_ps->m_isParagraphOpened)
		_openSpan();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();
}

void WPSContentListener::insertCharacter(const uint16_t character)
{
	if (!m_ps->m_isSpanOpened)
		_openSpan();
	m_ps->m_textBuffer.append(character);
}

void WPSContentListener::insertUnicodeCharacter(uint32_t character)
{
	if (!m_ps->m_isSpanOpened)
		_openSpan();
	if (character == 0xfffd)
		return;
	uint8_t first;
	int len;
	if (character < 0x80)
	{
		first = 0;
		len = 1;
	}
	else if (character < 0x800)
	{
		first = 0xc0;
		len = 2;
	}
	else if (character < 0x10000)
	{
		first = 0xe0;
		len = 3;
	}
	else if (character < 0x200000)
	{
		first = 0xf0;
		len = 4;
	}
	else if (character < 0x4000000)
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
		outbuf[i] = (character & 0x3f) | 0x80;
		character >>= 6;
	}
	outbuf[0] = character | first;

	for (i = 0; i < len; i++)
		m_ps->m_textBuffer.append(outbuf[i]);
}

void WPSContentListener::setFieldType(uint16_t code)
{
	m_ps->m_fieldcode = code;
}

void WPSContentListener::insertField()
{
	WPXPropertyList pl;

	if (m_ps->m_fieldcode == WPS_FIELD_PAGE)
	{
		_flushText();
		pl.insert("style:num-format","1");
		m_documentInterface->insertField("text:page-number",pl);
	}
}

void WPSContentListener::_flushText()
{
	_insertText(m_ps->m_textBuffer);
	m_ps->m_textBuffer.clear();
}

void WPSContentListener::_insertText(const WPXString &textBuffer)
{
	if (textBuffer.len() <= 0)
		return;

	WPXString tmpText;
	const char ASCII_SPACE = 0x0020;

	int numConsecutiveSpaces = 0;
	WPXString::Iter i(textBuffer);
	for (i.rewind(); i.next();)
	{
		if (*(i()) == ASCII_SPACE)
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
		{
			tmpText.append(i());
		}
	}

	m_documentInterface->insertText(tmpText);
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
#if 0
	m_ps->m_paragraphMarginLeft = m_ps->m_leftMarginByPageMarginChange + m_ps->m_leftMarginByParagraphMarginChange;
	m_ps->m_paragraphMarginRight = m_ps->m_rightMarginByPageMarginChange + m_ps->m_rightMarginByParagraphMarginChange;
	m_ps->m_leftMarginByTabs = 0.0;
	m_ps->m_rightMarginByTabs = 0.0;
	m_ps->m_paragraphTextIndent = m_ps->m_textIndentByParagraphIndentChange;
	m_ps->m_textIndentByTabs = 0.0;
	m_ps->m_isCellWithoutParagraph = false;
	m_ps->m_isTextColumnWithoutParagraph = false;
	m_ps->m_isHeaderFooterWithoutParagraph = false;
	m_ps->m_tempParagraphJustification = 0;
#endif
	m_ps->m_listReferencePosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
	m_ps->m_listBeginPosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
}

void WPSContentListener::_appendParagraphProperties(WPXPropertyList &propList, const bool /*isListElement*/)
{
	switch (m_ps->m_paragraphJustification)
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
		WPS_DEBUG_MSG(("WPSContentListener::_appendParagraphProperties: unimplemented justification\n"));
		propList.insert("fo:text-align", "left");
		break;
	}

	if (!m_ps->m_isTableOpened)
	{
		// these properties are not appropriate when a table is opened..
#ifdef NEW_VERSION
		if (isListElement)
			propList.insert("fo:margin-left", (m_ps->m_listBeginPosition - m_ps->m_paragraphTextIndent));
		else
#endif
			propList.insert("fo:margin-left", m_ps->m_paragraphMarginLeft);
		propList.insert("fo:text-indent", m_ps->m_paragraphTextIndent);
		propList.insert("fo:margin-right", m_ps->m_paragraphMarginRight);
	}
	// WPS : osnola
	propList.insert("fo:margin-top", m_ps->m_paragraphMarginTop);
	propList.insert("fo:margin-bottom", m_ps->m_paragraphMarginBottom);
	propList.insert("fo:line-height", m_ps->m_paragraphLineSpacing, WPX_PERCENT);

	if (m_ps->m_isParagraphColumnBreak)
		propList.insert("fo:break-before", "column");
	else if (m_ps->m_isParagraphPageBreak)
		propList.insert("fo:break-before", "page");
}

void WPSContentListener::_getTabStops(WPXPropertyListVector &tabStops)
{
	for (int i=0; i<(int)m_tabs.size(); i++)
		m_tabs[i].addTo(tabStops);
}

///////////////////
//
// List: Minimal implementation
//
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
		list->setId(++m_ps->m_actualListId);
}
shared_ptr<WPSList> WPSContentListener::getCurrentList() const
{
	return m_ps->m_list;
}


void WPSContentListener::_openListElement()
{
	if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
		return;

	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
	{
		if (!m_ps->m_isTableOpened && (!m_ps->m_inSubDocument/* OSNOLA: fixme || m_ps->m_subDocumentType == WPS_SUBDOCUMENT_TEXT_BOX*/))
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

	m_ps->m_isListElementOpened = false;
	m_ps->m_currentListLevel = 0;

	if (!m_ps->m_isTableOpened && m_ps->m_isPageSpanBreakDeferred && !m_ps->m_inSubDocument)
		_closePageSpan();
}

void WPSContentListener::_changeList()
{
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();
	_handleListChange();
}


// basic model of unordered list
void WPSContentListener::_handleListChange()
{
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
					m_ps->m_list->setId(++m_ps->m_actualListId);
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
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
