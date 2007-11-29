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

#include "WPSContentListener.h"
#include "WPSPageSpan.h"
#include "libwps_internal.h"
#include <libwpd/WPXProperty.h>
#ifdef _MSC_VER
#include <minmax.h>
#define LIBWPS_MIN min
#define LIBWPS_MAX max
#else
#define LIBWPS_MIN std::min
#define LIBWPS_MAX std::max
#endif

namespace {

WPXString doubleToString(const double value)
{
  WPXString tempString;
  tempString.sprintf("%.4f", value);
  std::string decimalPoint(localeconv()->decimal_point);
  if ((decimalPoint.size() == 0) || (decimalPoint == "."))
    return tempString;
  std::string stringValue(tempString.cstr());
  if (!stringValue.empty())
  {
    std::string::size_type pos;
    while ((pos = stringValue.find(decimalPoint)) != std::string::npos)
          stringValue.replace(pos,decimalPoint.size(),".");
  }
  return WPXString(stringValue.c_str());
}

} // namespace

_WPSContentParsingState::_WPSContentParsingState() :
	m_textAttributeBits(0),
	m_fontSize(12.0f/*WP6_DEFAULT_FONT_SIZE*/), // FIXME ME!!!!!!!!!!!!!!!!!!! HELP WP6_DEFAULT_FONT_SIZE
	m_fontName(new WPXString(/*WP6_DEFAULT_FONT_NAME*/"Times New Roman")), // EN PAS DEFAULT FONT AAN VOOR WP5/6/etc

	m_isParagraphColumnBreak(false),
	m_isParagraphPageBreak(false),
	m_paragraphJustification(WPS_PARAGRAPH_JUSTIFICATION_LEFT),
	m_paragraphLineSpacing(1.0f),

	m_isDocumentStarted(false),
	m_isPageSpanOpened(false),
	m_isSectionOpened(false),
	m_isPageSpanBreakDeferred(false),
	m_isHeaderFooterWithoutParagraph(false),

	m_isSpanOpened(false),
	m_isParagraphOpened(false),
	m_isListElementOpened(false),
	m_cellAttributeBits(0),
	m_paragraphJustificationBeforeColumns(WPS_PARAGRAPH_JUSTIFICATION_LEFT),

	m_numPagesRemainingInSpan(0),

	m_sectionAttributesChanged(false),
	m_numColumns(1),
	m_isTextColumnWithoutParagraph(false),

	m_pageFormLength(11.0f),
	m_pageFormWidth(8.5f),
	m_pageFormOrientation(PORTRAIT),

	m_pageMarginLeft(1.0f),
	m_pageMarginRight(1.0f),

	m_paragraphMarginLeft(0.0f),
	m_paragraphMarginRight(0.0f),
	m_paragraphMarginTop(0.0f),
	m_paragraphMarginBottom(0.0f),
	m_leftMarginByPageMarginChange(0.0f),
	m_rightMarginByPageMarginChange(0.0f),
	m_sectionMarginLeft(0.0f),
	m_sectionMarginRight(0.0f),
	m_leftMarginByParagraphMarginChange(0.0f),
	m_rightMarginByParagraphMarginChange(0.0f),
	m_leftMarginByTabs(0.0f),
	m_rightMarginByTabs(0.0f),
	
	m_listReferencePosition(0.0f),
	m_listBeginPosition(0.0f),

	m_paragraphTextIndent(0.0f),
	m_textIndentByParagraphIndentChange(0.0f),
	m_textIndentByTabs(0.0f),
	m_currentListLevel(0),
{
}

_WPSContentParsingState::~_WPSContentParsingState()
{
	DELETEP(m_fontName);
}

WPSContentListener::WPSContentListener(std::list<WPSPageSpan> &pageList, WPXDocumentInterface *documentInterface) :
	m_ps(new WPSContentParsingState),
	m_documentInterface(documentInterface),
	m_pageList(pageList),
	m_isUndoOn(false)
{
	m_ps->m_nextPageSpanIter = pageList.begin();
}

WPSContentListener::~WPSContentListener()
{
	DELETEP(m_ps);
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
	if (m_ps->m_isListElementOpened)
		_closeListElement();

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

		propList.insert("fo:margin-left", m_ps->m_sectionMarginLeft);
		propList.insert("fo:margin-right", m_ps->m_sectionMarginRight);
		if (m_ps->m_numColumns > 1)
		{
			propList.insert("fo:margin-bottom", 1.0f);
			propList.insert("text:dont-balance-text-columns", false);
		}
		else
			propList.insert("fo:margin-bottom", 0.0f);

		WPXPropertyListVector columns;
		if (!m_ps->m_isSectionOpened)
			m_documentInterface->openSection(propList, columns);

		m_ps->m_sectionAttributesChanged = false;
		m_ps->m_isSectionOpened = true;
	}
}

void WPSContentListener::_closeSection()
{
	if (m_ps->m_isSectionOpened)
	{
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		if (m_ps->m_isListElementOpened)
			_closeListElement();
		_changeList();

		m_documentInterface->closeSection();

		m_ps->m_sectionAttributesChanged = false;
		m_ps->m_isSectionOpened = false;
	}
}

void WPSContentListener::_openPageSpan()
{
	if (m_ps->m_isPageSpanOpened)
		return;

	if (!m_ps->m_isDocumentStarted)
		startDocument();

	// Hack to be sure that the paragraph margins are consistent even if the page margin changes
	if (m_ps->m_leftMarginByPageMarginChange != 0)
		m_ps->m_leftMarginByPageMarginChange += m_ps->m_pageMarginLeft;
	if (m_ps->m_rightMarginByPageMarginChange != 0)
		m_ps->m_rightMarginByPageMarginChange += m_ps->m_pageMarginRight;
	if (m_ps->m_sectionMarginLeft != 0)
		m_ps->m_sectionMarginLeft += m_ps->m_pageMarginLeft;
	if (m_ps->m_sectionMarginRight != 0)
		m_ps->m_sectionMarginRight += m_ps->m_pageMarginRight;
	m_ps->m_listReferencePosition += m_ps->m_pageMarginLeft;
	m_ps->m_listBeginPosition += m_ps->m_pageMarginLeft;
	
	if ( m_pageList.empty() || (m_ps->m_nextPageSpanIter == m_pageList.end()))
	{
		WPS_DEBUG_MSG(("m_pageList.empty() || (m_ps->m_nextPageSpanIter == m_pageList.end())\n"));
		throw ParseException();
	}

	WPSPageSpan currentPage = (*m_ps->m_nextPageSpanIter);
	currentPage.makeConsistent(1);
	
	WPXPropertyList propList;
	propList.insert("libwpd:num-pages", currentPage.getPageSpan());

	std::list<WPSPageSpan>::iterator lastPageSpan = --m_pageList.end(); 
	propList.insert("libwpd:is-last-page-span", ((m_ps->m_nextPageSpanIter == lastPageSpan) ? true : false));
	propList.insert("fo:page-height", currentPage.getFormLength());
	propList.insert("fo:page-width", currentPage.getFormWidth());
	if (currentPage.getFormOrientation() == LANDSCAPE)
		propList.insert("style:print-orientation", "landscape"); 
	else
		propList.insert("style:print-orientation", "portrait"); 
	propList.insert("fo:margin-left", currentPage.getMarginLeft());
	propList.insert("fo:margin-right", currentPage.getMarginRight());
	propList.insert("fo:margin-top", currentPage.getMarginTop());
	propList.insert("fo:margin-bottom", currentPage.getMarginBottom());
	
	if (!m_ps->m_isPageSpanOpened)
		m_documentInterface->openPageSpan(propList);

	m_ps->m_isPageSpanOpened = true;

	m_ps->m_pageFormWidth = currentPage.getFormWidth();
	m_ps->m_pageMarginLeft = currentPage.getMarginLeft();
	m_ps->m_pageMarginRight = currentPage.getMarginRight();

	// Hack to be sure that the paragraph margins are consistent even if the page margin changes
	// Compute new values
	if (m_ps->m_leftMarginByPageMarginChange != 0)
		m_ps->m_leftMarginByPageMarginChange -= m_ps->m_pageMarginLeft;
	if (m_ps->m_rightMarginByPageMarginChange != 0)
		m_ps->m_rightMarginByPageMarginChange -= m_ps->m_pageMarginRight;
	if (m_ps->m_sectionMarginLeft != 0)
		m_ps->m_sectionMarginLeft -= m_ps->m_pageMarginLeft;
	if (m_ps->m_sectionMarginRight != 0)
		m_ps->m_sectionMarginRight -= m_ps->m_pageMarginRight;
	m_ps->m_listReferencePosition -= m_ps->m_pageMarginLeft;
	m_ps->m_listBeginPosition -= m_ps->m_pageMarginLeft;

	m_ps->m_paragraphMarginLeft = m_ps->m_leftMarginByPageMarginChange + m_ps->m_leftMarginByParagraphMarginChange
			+ m_ps->m_leftMarginByTabs;
	m_ps->m_paragraphMarginRight = m_ps->m_rightMarginByPageMarginChange + m_ps->m_rightMarginByParagraphMarginChange
			+ m_ps->m_rightMarginByTabs;

	std::vector<WPSHeaderFooter> headerFooterList = currentPage.getHeaderFooterList();
	for (std::vector<WPSHeaderFooter>::iterator iter = headerFooterList.begin(); iter != headerFooterList.end(); iter++)
	{
		if (!currentPage.getHeaderFooterSuppression((*iter).getInternalType()))
		{
			propList.clear();
			switch ((*iter).getOccurence())
			{
			case ODD:
				propList.insert("libwpd:occurence", "odd");
				break;
			case EVEN:
				propList.insert("libwpd:occurence", "even");
				break;
			case ALL:
				propList.insert("libwpd:occurence", "all");
				break;
			case NEVER:
			default:
				break;
			}

			if ((*iter).getType() == HEADER)
				m_documentInterface->openHeader(propList); 
			else
				m_documentInterface->openFooter(propList); 

			if ((*iter).getType() == HEADER)
				m_documentInterface->closeHeader();
			else
				m_documentInterface->closeFooter(); 

			WPS_DEBUG_MSG(("Header Footer Element: type: %i occurence: %i\n",
				       (*iter).getType(), (*iter).getOccurence()));
		}
	}

	/* Some of this would maybe not be necessary, but it does not do any harm 
	 * and apparently solves some troubles */
	m_ps->m_pageFormLength = currentPage.getFormLength();
	m_ps->m_pageFormWidth = currentPage.getFormWidth();
	m_ps->m_pageFormOrientation = currentPage.getFormOrientation();
	m_ps->m_pageMarginLeft = currentPage.getMarginLeft();
	m_ps->m_pageMarginRight = currentPage.getMarginRight();

	m_ps->m_paragraphMarginLeft = m_ps->m_leftMarginByPageMarginChange + m_ps->m_leftMarginByParagraphMarginChange
			+ m_ps->m_leftMarginByTabs;
	m_ps->m_paragraphMarginRight = m_ps->m_rightMarginByPageMarginChange + m_ps->m_rightMarginByParagraphMarginChange
			+ m_ps->m_rightMarginByTabs;

	m_ps->m_paragraphTextIndent = m_ps->m_textIndentByParagraphIndentChange + m_ps->m_textIndentByTabs;

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
	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
	{
		if (m_ps->m_sectionAttributesChanged)
			_closeSection();

		if (!m_ps->m_isSectionOpened)
			_openSection();

		WPXPropertyListVector tabStops;

		WPXPropertyList propList;
		_appendParagraphProperties(propList);

		if (!m_ps->m_isParagraphOpened)
			m_documentInterface->openParagraph(propList, tabStops);

		_resetParagraphState();
	}
}

void WPSContentListener::_resetParagraphState(const bool isListElement)
{
	m_ps->m_isParagraphColumnBreak = false;
	m_ps->m_isParagraphPageBreak = false;
	if (isListElement)
	{
		m_ps->m_isListElementOpened = true;
		m_ps->m_isParagraphOpened = false;
	}
	else
	{
		m_ps->m_isListElementOpened = false;
		m_ps->m_isParagraphOpened = true;
	}
	m_ps->m_paragraphMarginLeft = m_ps->m_leftMarginByPageMarginChange + m_ps->m_leftMarginByParagraphMarginChange;
	m_ps->m_paragraphMarginRight = m_ps->m_rightMarginByPageMarginChange + m_ps->m_rightMarginByParagraphMarginChange;
	m_ps->m_leftMarginByTabs = 0.0f;
	m_ps->m_rightMarginByTabs = 0.0f;
	m_ps->m_paragraphTextIndent = m_ps->m_textIndentByParagraphIndentChange;
	m_ps->m_textIndentByTabs = 0.0f;
	m_ps->m_isCellWithoutParagraph = false;
	m_ps->m_isTextColumnWithoutParagraph = false;
	m_ps->m_isHeaderFooterWithoutParagraph = false;
	m_ps->m_listReferencePosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
	m_ps->m_listBeginPosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
}

void WPSContentListener::_appendJustification(WPXPropertyList &propList, int justification)
{
	switch (justification)
	{
	case WPS_PARAGRAPH_JUSTIFICATION_LEFT:
		// doesn't require a paragraph prop - it is the default
		propList.insert("fo:text-align", "left");
		break;
	case WPS_PARAGRAPH_JUSTIFICATION_CENTER:
		propList.insert("fo:text-align", "center");
		break;
	case WPS_PARAGRAPH_JUSTIFICATION_RIGHT:
		propList.insert("fo:text-align", "end");
		break;
	case WPS_PARAGRAPH_JUSTIFICATION_FULL:
		propList.insert("fo:text-align", "justify");
		break;
	case WPS_PARAGRAPH_JUSTIFICATION_FULL_ALL_LINES:
		propList.insert("fo:text-align", "justify");
		propList.insert("fo:text-align-last", "justify");
		break;
	}
}

void WPSContentListener::_appendParagraphProperties(WPXPropertyList &propList, const bool isListElement)
{
	_appendJustification(propList, m_ps->m_paragraphJustification);

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
	propList.insert("fo:margin-top", m_ps->m_paragraphMarginTop);
	propList.insert("fo:margin-bottom", m_ps->m_paragraphMarginBottom);
	propList.insert("fo:line-height", m_ps->m_paragraphLineSpacing, WPX_PERCENT);
	if (m_ps->m_isParagraphColumnBreak)
		propList.insert("fo:break-before", "column");
	else if (m_ps->m_isParagraphPageBreak)
		propList.insert("fo:break-before", "page");
}

void WPSContentListener::_closeParagraph()
{
	if (m_ps->m_isParagraphOpened)
	{
		if (m_ps->m_isSpanOpened)
			_closeSpan();

		m_documentInterface->closeParagraph();
	}

	m_ps->m_isParagraphOpened = false;
	m_ps->m_currentListLevel = 0;

	if (m_ps->m_isPageSpanBreakDeferred)
		_closePageSpan();
}

void WPSContentListener::_openListElement()
{
	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
	{
		if (!m_ps->m_isSectionOpened)
			_openSection();

		WPXPropertyList propList;
		_appendParagraphProperties(propList, true);

		WPXPropertyListVector tabStops;

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

	if (m_ps->m_isPageSpanBreakDeferred)
		_closePageSpan();
}

const float WPS_DEFAULT_SUPER_SUB_SCRIPT = 58.0f; 

void WPSContentListener::_openSpan()
{
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
	float fontSizeChange;
	// the font size attribute bits are mutually exclusive and the cell attributes prevail
	if ((m_ps->m_cellAttributeBits & 0x0000001f) != 0x00000000)
		fontSizeAttributes = (uint8_t)(m_ps->m_cellAttributeBits & 0x0000001f);
	else
		fontSizeAttributes = (uint8_t)(m_ps->m_textAttributeBits & 0x0000001f);
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
 	if (attributeBits & WPS_SUPERSCRIPT_BIT) {
		WPXString sSuperScript("super ");
		sSuperScript.append(doubleToString(WPS_DEFAULT_SUPER_SUB_SCRIPT));
		sSuperScript.append("%");
		propList.insert("style:text-position", sSuperScript);
	}
 	else if (attributeBits & WPS_SUBSCRIPT_BIT) {
		WPXString sSubScript("sub ");
		sSubScript.append(doubleToString(WPS_DEFAULT_SUPER_SUB_SCRIPT));
		sSubScript.append("%");
		propList.insert("style:text-position", sSubScript);
	}
	if (attributeBits & WPS_ITALICS_BIT)
		propList.insert("fo:font-style", "italic");
	if (attributeBits & WPS_BOLD_BIT)
		propList.insert("fo:font-weight", "bold");
	if (attributeBits & WPS_STRIKEOUT_BIT)
		propList.insert("style:text-crossing-out", "single-line");
	if (attributeBits & WPS_DOUBLE_UNDERLINE_BIT) 
		propList.insert("style:text-underline", "double");
 	else if (attributeBits & WPS_UNDERLINE_BIT) 
		propList.insert("style:text-underline", "single");
	if (attributeBits & WPS_OUTLINE_BIT) 
		propList.insert("style:text-outline", "true");
	if (attributeBits & WPS_SMALL_CAPS_BIT) 
		propList.insert("fo:font-variant", "small-caps");
	if (attributeBits & WPS_BLINK_BIT) 
		propList.insert("style:text-blinking", "true");
	if (attributeBits & WPS_SHADOW_BIT) 
		propList.insert("fo:text-shadow", "1pt 1pt");

	if (m_ps->m_fontName)
		propList.insert("style:font-name", m_ps->m_fontName->cstr());
	propList.insert("fo:font-size", fontSizeChange*m_ps->m_fontSize, WPX_POINT);

	// Here we give the priority to the redline bit over the font color.
	// When redline finishes, the color is back.
	if (attributeBits & WPS_REDLINE_BIT)
		propList.insert("fo:color", "#ff3333");  // #ff3333 = a nice bright red
	else
		propList.insert("fo:color", "#000000");

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
	if (!isUndoOn())
	{
		switch (breakType)
		{
		case WPS_COLUMN_BREAK:
			if (!m_ps->m_isPageSpanOpened)
				_openSpan();				
			if (m_ps->m_isParagraphOpened)
				_closeParagraph();
			if (m_ps->m_isListElementOpened)
				_closeListElement();
			m_ps->m_isParagraphColumnBreak = true;
			m_ps->m_isTextColumnWithoutParagraph = true;
			break;
		case WPS_PAGE_BREAK:
			if (!m_ps->m_isPageSpanOpened)
				_openSpan();				
			if (m_ps->m_isParagraphOpened)
				_closeParagraph();
			if (m_ps->m_isListElementOpened)
				_closeListElement();
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
			    if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
				_closePageSpan();
			    else
				m_ps->m_isPageSpanBreakDeferred = true;
			}
		default:
			break;
		}
	}
}

void WPSContentListener::lineSpacingChange(const float lineSpacing)
{
	if (!isUndoOn())
	{
		m_ps->m_paragraphLineSpacing = lineSpacing;
	}
}
