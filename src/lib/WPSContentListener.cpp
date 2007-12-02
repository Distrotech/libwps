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

_WPSContentParsingState::_WPSContentParsingState() :
	m_textAttributeBits(0),
	m_fontSize(12.0f/*WP6_DEFAULT_FONT_SIZE*/),
	m_fontName(WPXString(/*WP6_DEFAULT_FONT_NAME*/"Times New Roman")),

	m_isParagraphColumnBreak(false),
	m_isParagraphPageBreak(false),
	m_paragraphJustification(WPS_PARAGRAPH_JUSTIFICATION_LEFT),
	m_paragraphLineSpacing(1.0f),

	m_isDocumentStarted(false),
	m_isPageSpanOpened(false),
	m_isSectionOpened(false),
	m_isPageSpanBreakDeferred(false),

	m_isSpanOpened(false),
	m_isParagraphOpened(false),

	m_numPagesRemainingInSpan(0),

	m_sectionAttributesChanged(false),

	m_pageFormLength(11.0f),
	m_pageFormWidth(8.5f),
	m_pageFormOrientation(PORTRAIT),

	m_pageMarginLeft(1.0f),
	m_pageMarginRight(1.0f),

	m_paragraphMarginLeft(0.0f),
	m_paragraphMarginRight(0.0f),
	m_paragraphMarginTop(0.0f),
	m_paragraphMarginBottom(0.0f)
{
}

_WPSContentParsingState::~_WPSContentParsingState()
{
}

WPSContentListener::WPSContentListener(std::list<WPSPageSpan> &pageList, WPXDocumentInterface *documentInterface) :
	m_ps(new WPSContentParsingState),
	m_documentInterface(documentInterface),
	m_pageList(pageList)
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
	if (m_ps->m_isSectionOpened)
	{
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();

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
	if (!m_ps->m_isParagraphOpened)
	{
		if (m_ps->m_sectionAttributesChanged)
			_closeSection();

		if (!m_ps->m_isSectionOpened)
			_openSection();

		WPXPropertyListVector tabStops;

		WPXPropertyList propList;
		switch (m_ps->m_paragraphJustification)
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

		propList.insert("fo:margin-left", m_ps->m_paragraphMarginLeft);
		propList.insert("fo:margin-right", m_ps->m_paragraphMarginRight);
		propList.insert("fo:margin-top", m_ps->m_paragraphMarginTop);
		propList.insert("fo:margin-bottom", m_ps->m_paragraphMarginBottom);
		propList.insert("fo:line-height", m_ps->m_paragraphLineSpacing, WPX_PERCENT);

		if (m_ps->m_isParagraphColumnBreak)
			propList.insert("fo:break-before", "column");
		else if (m_ps->m_isParagraphPageBreak)
			propList.insert("fo:break-before", "page");

		if (!m_ps->m_isParagraphOpened)
			m_documentInterface->openParagraph(propList, tabStops);

		m_ps->m_isParagraphColumnBreak = false;
		m_ps->m_isParagraphPageBreak = false;
		m_ps->m_isParagraphOpened = true;
	}
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

	if (m_ps->m_isPageSpanBreakDeferred)
		_closePageSpan();
}

void WPSContentListener::_openSpan()
{
	if (!m_ps->m_isParagraphOpened)
		_openParagraph();
	
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
		propList.insert("style:text-position", "58%");
 	else if (m_ps->m_textAttributeBits & WPS_SUBSCRIPT_BIT)
		propList.insert("style:text-position", "58%");
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

	if (m_ps->m_fontName.len())
		propList.insert("style:font-name", m_ps->m_fontName.cstr());
	propList.insert("fo:font-size", fontSizeChange*m_ps->m_fontSize, WPX_POINT);

	// Here we give the priority to the redline bit over the font color.
	// When redline finishes, the color is back.
	if (m_ps->m_textAttributeBits & WPS_REDLINE_BIT)
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
