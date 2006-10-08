/* libwpd
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include "WPXContentListener.h"
#include "WPXPageSpan.h"
#include "libwpd_internal.h"
#include <libwpd/WPXProperty.h>
#ifdef _MSC_VER
#include <minmax.h>
#define LIBWPD_MIN min
#define LIBWPD_MAX max
#else
#define LIBWPD_MIN std::min
#define LIBWPD_MAX std::max
#endif

_WPXContentParsingState::_WPXContentParsingState() :
	m_textAttributeBits(0),
	m_fontSize(12.0f/*WP6_DEFAULT_FONT_SIZE*/), // FIXME ME!!!!!!!!!!!!!!!!!!! HELP WP6_DEFAULT_FONT_SIZE
	m_fontName(new WPXString(/*WP6_DEFAULT_FONT_NAME*/"Times New Roman")), // EN PAS DEFAULT FONT AAN VOOR WP5/6/etc
	m_fontColor(new RGBSColor(0x00,0x00,0x00,0x64)), //Set default to black. Maybe once it will change, but for the while...
	m_highlightColor(NULL),

	m_isParagraphColumnBreak(false),
	m_isParagraphPageBreak(false),
	m_paragraphJustification(WPX_PARAGRAPH_JUSTIFICATION_LEFT),
	m_tempParagraphJustification(0),
	m_paragraphLineSpacing(1.0f),

	m_isDocumentStarted(false),
	m_isPageSpanOpened(false),
	m_isSectionOpened(false),
	m_isPageSpanBreakDeferred(false),
	m_isHeaderFooterWithoutParagraph(false),

	m_isSpanOpened(false),
	m_isParagraphOpened(false),
	m_isListElementOpened(false),

	m_paragraphJustificationBeforeColumns(WPX_PARAGRAPH_JUSTIFICATION_LEFT),

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

	m_alignmentCharacter('.'),
	m_isTabPositionRelative(false),
	m_isNote(false)
{
}

_WPXContentParsingState::~_WPXContentParsingState()
{
	DELETEP(m_fontName);
	DELETEP(m_fontColor);
	DELETEP(m_highlightColor);
}

WPXContentListener::WPXContentListener(std::list<WPXPageSpan> &pageList, WPXHLListenerImpl *listenerImpl) :
	WPXListener(pageList),
	m_ps(new WPXContentParsingState),
	m_listenerImpl(listenerImpl)
{
	m_ps->m_nextPageSpanIter = pageList.begin();
}

WPXContentListener::~WPXContentListener()
{
	DELETEP(m_ps);
}

void WPXContentListener::startDocument()
{
	if (!m_ps->m_isDocumentStarted)
	{
		// FIXME: this is stupid, we should store a property list filled with the relevant metadata
		// and then pass that directly..

		m_listenerImpl->setDocumentMetaData(m_metaData);

		m_listenerImpl->startDocument();
	}
	
	m_ps->m_isDocumentStarted = true;
}

void WPXContentListener::endDocument()
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
	m_listenerImpl->endDocument();
}

void WPXContentListener::_openSection()
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
 		typedef std::vector<WPXColumnDefinition>::const_iterator CDVIter;
	 	for (CDVIter iter = m_ps->m_textColumns.begin(); iter != m_ps->m_textColumns.end(); iter++)
		{
			WPXPropertyList column;
			// The "style:rel-width" is expressed in twips (1440 twips per inch) and includes the left and right Gutter
			column.insert("style:rel-width", (*iter).m_width * 1440.0f, TWIP);
			column.insert("fo:margin-left", (*iter).m_leftGutter);
			column.insert("fo:margin-right", (*iter).m_rightGutter);
			columns.append(column);
		}
		if (!m_ps->m_isSectionOpened)
			m_listenerImpl->openSection(propList, columns);

		m_ps->m_sectionAttributesChanged = false;
		m_ps->m_isSectionOpened = true;
	}
}

void WPXContentListener::_closeSection()
{
	if (m_ps->m_isSectionOpened)
	{
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		if (m_ps->m_isListElementOpened)
			_closeListElement();
		_changeList();

		m_listenerImpl->closeSection();

		m_ps->m_sectionAttributesChanged = false;
		m_ps->m_isSectionOpened = false;
	}
}

void WPXContentListener::_openPageSpan()
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
		WPD_DEBUG_MSG(("m_pageList.empty() || (m_ps->m_nextPageSpanIter == m_pageList.end())\n"));
		throw ParseException();
	}

	WPXPageSpan currentPage = (*m_ps->m_nextPageSpanIter);
	currentPage.makeConsistent(1);
	
	WPXPropertyList propList;
	propList.insert("libwpd:num-pages", currentPage.getPageSpan());

	std::list<WPXPageSpan>::iterator lastPageSpan = --m_pageList.end(); 
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
		m_listenerImpl->openPageSpan(propList);

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

	std::vector<WPXHeaderFooter> headerFooterList = currentPage.getHeaderFooterList();
	for (std::vector<WPXHeaderFooter>::iterator iter = headerFooterList.begin(); iter != headerFooterList.end(); iter++)
	{
		if (!currentPage.getHeaderFooterSuppression((*iter).getInternalType()))
		{
			WPXPropertyList propList;
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
			}

			if ((*iter).getType() == HEADER)
				m_listenerImpl->openHeader(propList); 
			else
				m_listenerImpl->openFooter(propList); 

			if ((*iter).getType() == HEADER)
				m_listenerImpl->closeHeader();
			else
				m_listenerImpl->closeFooter(); 

			WPD_DEBUG_MSG(("Header Footer Element: type: %i occurence: %i\n",
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

void WPXContentListener::_closePageSpan()
{
	if (m_ps->m_isPageSpanOpened)
	{
		if (m_ps->m_isSectionOpened)
			_closeSection();

		m_listenerImpl->closePageSpan();
	}
	
	m_ps->m_isPageSpanOpened = false;
	m_ps->m_isPageSpanBreakDeferred = false;
}

void WPXContentListener::_openParagraph()
{
	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
	{
		if (m_ps->m_sectionAttributesChanged)
			_closeSection();

		if (!m_ps->m_isSectionOpened)
			_openSection();

		WPXPropertyListVector tabStops;
		_getTabStops(tabStops);

		WPXPropertyList propList;
		_appendParagraphProperties(propList);

		if (!m_ps->m_isParagraphOpened)
			m_listenerImpl->openParagraph(propList, tabStops);

		_resetParagraphState();
	}
}

void WPXContentListener::_resetParagraphState(const bool isListElement)
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
	m_ps->m_tempParagraphJustification = 0;
	m_ps->m_listReferencePosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
	m_ps->m_listBeginPosition = m_ps->m_paragraphMarginLeft + m_ps->m_paragraphTextIndent;
}

void WPXContentListener::_appendJustification(WPXPropertyList &propList, int justification)
{
	switch (justification)
	{
	case WPX_PARAGRAPH_JUSTIFICATION_LEFT:
		// doesn't require a paragraph prop - it is the default
		propList.insert("fo:text-align", "left");
		break;
	case WPX_PARAGRAPH_JUSTIFICATION_CENTER:
		propList.insert("fo:text-align", "center");
		break;
	case WPX_PARAGRAPH_JUSTIFICATION_RIGHT:
		propList.insert("fo:text-align", "end");
		break;
	case WPX_PARAGRAPH_JUSTIFICATION_FULL:
		propList.insert("fo:text-align", "justify");
		break;
	case WPX_PARAGRAPH_JUSTIFICATION_FULL_ALL_LINES:
		propList.insert("fo:text-align", "justify");
		propList.insert("fo:text-align-last", "justify");
		break;
	}
}

void WPXContentListener::_appendParagraphProperties(WPXPropertyList &propList, const bool isListElement)
{
	int justification;
	if (m_ps->m_tempParagraphJustification) 
		justification = m_ps->m_tempParagraphJustification;
	else
		justification = m_ps->m_paragraphJustification;
	_appendJustification(propList, justification);

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
	propList.insert("fo:line-height", m_ps->m_paragraphLineSpacing, PERCENT);
	if (m_ps->m_isParagraphColumnBreak)
		propList.insert("fo:break-before", "column");
	else if (m_ps->m_isParagraphPageBreak)
		propList.insert("fo:break-before", "page");
}

void WPXContentListener::_getTabStops(WPXPropertyListVector &tabStops)
{
	for (int i=0; i<m_ps->m_tabStops.size(); i++)
	{
		WPXPropertyList tmpTabStop;

		// type
		switch (m_ps->m_tabStops[i].m_alignment)
		{
		case RIGHT:
			tmpTabStop.insert("style:type", "right");
			break;
		case CENTER:
			tmpTabStop.insert("style:type", "center");
			break;
		case DECIMAL:
			tmpTabStop.insert("style:type", "char");
			tmpTabStop.insert("style:char", "."); // Assume a decimal point for now
			break;
		default:  // Left alignment is the default and BAR is not handled in OOo
			break;
		}
		
		// leader character
		if (m_ps->m_tabStops[i].m_leaderCharacter != 0x0000)
		{
			WPXString sLeader;
			sLeader.sprintf("%c", m_ps->m_tabStops[i].m_leaderCharacter);
			tmpTabStop.insert("style:leader-char", sLeader);
		}

		// position
		float position = m_ps->m_tabStops[i].m_position;
		if (m_ps->m_isTabPositionRelative)
			position -= m_ps->m_leftMarginByTabs;
		else
			position -= m_ps->m_paragraphMarginLeft + m_ps->m_sectionMarginLeft + m_ps->m_pageMarginLeft;
		tmpTabStop.insert("style:position", position);
		

		/* TODO: fix situations where we have several columns or are inside a table and the tab stop
		 *       positions are absolute (relative to the paper edge). In this case, they have to be
		 *       computed for each column or each cell in table. (Fridrich) */
		tabStops.append(tmpTabStop);
	}
}

void WPXContentListener::_closeParagraph()
{
	if (m_ps->m_isParagraphOpened)
	{
		if (m_ps->m_isSpanOpened)
			_closeSpan();

		m_listenerImpl->closeParagraph();
	}

	m_ps->m_isParagraphOpened = false;
	m_ps->m_currentListLevel = 0;

	if (m_ps->m_isPageSpanBreakDeferred)
		_closePageSpan();
}

void WPXContentListener::_openListElement()
{
	if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
	{
		if (!m_ps->m_isSectionOpened)
			_openSection();

		WPXPropertyList propList;
		_appendParagraphProperties(propList, true);

		WPXPropertyListVector tabStops;
		_getTabStops(tabStops);

		if (!m_ps->m_isListElementOpened)
			m_listenerImpl->openListElement(propList, tabStops);
		_resetParagraphState(true);
	}
}

void WPXContentListener::_closeListElement()
{
	if (m_ps->m_isListElementOpened)
	{
		if (m_ps->m_isSpanOpened)
			_closeSpan();

		m_listenerImpl->closeListElement();
	}
	
	m_ps->m_isListElementOpened = false;
	m_ps->m_currentListLevel = 0;

	if (m_ps->m_isPageSpanBreakDeferred)
		_closePageSpan();
}

const float WPX_DEFAULT_SUPER_SUB_SCRIPT = 58.0f; 

void WPXContentListener::_openSpan()
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
 	if (attributeBits & WPX_SUPERSCRIPT_BIT) {
		WPXString sSuperScript;
		sSuperScript.sprintf("super %f%%", WPX_DEFAULT_SUPER_SUB_SCRIPT);
		propList.insert("style:text-position", sSuperScript);
	}
 	else if (attributeBits & WPX_SUBSCRIPT_BIT) {
		WPXString sSubScript;
		sSubScript.sprintf("sub %f%%", WPX_DEFAULT_SUPER_SUB_SCRIPT);
		propList.insert("style:text-position", sSubScript);
	}
	if (attributeBits & WPX_ITALICS_BIT)
		propList.insert("fo:font-style", "italic");
	if (attributeBits & WPX_BOLD_BIT)
		propList.insert("fo:font-weight", "bold");
	if (attributeBits & WPX_STRIKEOUT_BIT)
		propList.insert("style:text-crossing-out", "single-line");
	if (attributeBits & WPX_DOUBLE_UNDERLINE_BIT) 
		propList.insert("style:text-underline", "double");
 	else if (attributeBits & WPX_UNDERLINE_BIT) 
		propList.insert("style:text-underline", "single");
	if (attributeBits & WPX_OUTLINE_BIT) 
		propList.insert("style:text-outline", "true");
	if (attributeBits & WPX_SMALL_CAPS_BIT) 
		propList.insert("fo:font-variant", "small-caps");
	if (attributeBits & WPX_BLINK_BIT) 
		propList.insert("style:text-blinking", "true");
	if (attributeBits & WPX_SHADOW_BIT) 
		propList.insert("fo:text-shadow", "1pt 1pt");

	if (m_ps->m_fontName)
		propList.insert("style:font-name", m_ps->m_fontName->cstr());
	propList.insert("fo:font-size", fontSizeChange*m_ps->m_fontSize, POINT);

	// Here we give the priority to the redline bit over the font color. This is how WordPerfect behaves:
	// redline overrides font color even if the color is changed when redline was already defined.
	// When redline finishes, the color is back.
	if (attributeBits & WPX_REDLINE_BIT)
		propList.insert("fo:color", "#ff3333");  // #ff3333 = a nice bright red
	else if (m_ps->m_fontColor)
		propList.insert("fo:color", _colorToString(m_ps->m_fontColor));
	if (m_ps->m_highlightColor)
		propList.insert("style:text-background-color", _colorToString(m_ps->m_highlightColor));

	if (!m_ps->m_isSpanOpened)
		m_listenerImpl->openSpan(propList);

	m_ps->m_isSpanOpened = true;
}

void WPXContentListener::_closeSpan()
{
	if (m_ps->m_isSpanOpened)
	{
		_flushText();

		m_listenerImpl->closeSpan();
	}
	
	m_ps->m_isSpanOpened = false;
}

void WPXContentListener::insertBreak(const uint8_t breakType)
{
	if (!isUndoOn())
	{
		switch (breakType)
		{
		case WPX_COLUMN_BREAK:
			if (!m_ps->m_isPageSpanOpened)
				_openSpan();				
			if (m_ps->m_isParagraphOpened)
				_closeParagraph();
			if (m_ps->m_isListElementOpened)
				_closeListElement();
			m_ps->m_isParagraphColumnBreak = true;
			m_ps->m_isTextColumnWithoutParagraph = true;
			break;
		case WPX_PAGE_BREAK:
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
		case WPX_PAGE_BREAK:
		case WPX_SOFT_PAGE_BREAK:
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

void WPXContentListener::lineSpacingChange(const float lineSpacing)
{
	if (!isUndoOn())
	{
		m_ps->m_paragraphLineSpacing = lineSpacing;
	}
}

void WPXContentListener::justificationChange(const uint8_t justification)
{
	if (!isUndoOn())
	{
		// We discovered that if there is not a paragraph break before justificationChange,
		// newer versions of WordPerfect add a temporary hard return just before the code.
		// So, we will mimick them!
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		if (m_ps->m_isListElementOpened)
			_closeListElement();

		m_ps->m_currentListLevel = 0;

		switch (justification)
		{
		case 0x00:
			m_ps->m_paragraphJustification = WPX_PARAGRAPH_JUSTIFICATION_LEFT;
			break;
		case 0x01:
			m_ps->m_paragraphJustification = WPX_PARAGRAPH_JUSTIFICATION_FULL;
			break;
		case 0x02:
			m_ps->m_paragraphJustification = WPX_PARAGRAPH_JUSTIFICATION_CENTER;
			break;
		case 0x03:
			m_ps->m_paragraphJustification = WPX_PARAGRAPH_JUSTIFICATION_RIGHT;
			break;
		case 0x04:
			m_ps->m_paragraphJustification = WPX_PARAGRAPH_JUSTIFICATION_FULL_ALL_LINES;
			break;
		case 0x05:
			m_ps->m_paragraphJustification = WPX_PARAGRAPH_JUSTIFICATION_DECIMAL_ALIGNED;
			break;
		}
	}
}

WPXString WPXContentListener::_colorToString(const RGBSColor * color)
{
	WPXString tmpString;

	if (color) 
	{
		float fontShading = (float)((float)color->m_s/100.0f); //convert the percents to float between 0 and 1
		// Mix fontShading amount of given color with (1-fontShading) of White (#ffffff)
		int fontRed = (int)0xFF + (int)((float)color->m_r*fontShading) - (int)((float)0xFF*fontShading);
		int fontGreen = (int)0xFF + (int)((float)color->m_g*fontShading) - (int)((float)0xFF*fontShading);
		int fontBlue = (int)0xFF + (int)((float)color->m_b*fontShading) - (int)((float)0xFF*fontShading);
		tmpString.sprintf("#%.2x%.2x%.2x", fontRed, fontGreen, fontBlue);
	}
	else
		tmpString.sprintf("#%.2x%.2x%.2x", 0xFF, 0xFF, 0xFF); // default to white: we really shouldn't be calling this function in that case though

	return tmpString;
}

WPXString WPXContentListener::_mergeColorsToString(const RGBSColor *fgColor, const RGBSColor *bgColor)
{
	WPXString tmpColor;
	RGBSColor tmpFgColor, tmpBgColor;

	if (fgColor != NULL) {
		tmpFgColor.m_r = fgColor->m_r;
		tmpFgColor.m_g = fgColor->m_g;
		tmpFgColor.m_b = fgColor->m_b;
		tmpFgColor.m_s = fgColor->m_s;
	}
	else {
		tmpFgColor.m_r = tmpFgColor.m_g = tmpFgColor.m_b = 0xFF;
		tmpFgColor.m_s = 0x64; // 100%
	}
	if (bgColor != NULL) {
		tmpBgColor.m_r = bgColor->m_r;
		tmpBgColor.m_g = bgColor->m_g;
		tmpBgColor.m_b = bgColor->m_b;
		tmpBgColor.m_s = bgColor->m_s;
	}
	else {
		tmpBgColor.m_r = tmpBgColor.m_g = tmpBgColor.m_b = 0xFF;
		tmpBgColor.m_s = 0x64; // 100%
	}

	float fgAmount = (float)tmpFgColor.m_s/100.0f;
	float bgAmount = LIBWPD_MAX(((float)tmpBgColor.m_s-(float)tmpFgColor.m_s)/100.0f, 0.0f);

	int bgRed = LIBWPD_MIN((int)(((float)tmpFgColor.m_r*fgAmount)+((float)tmpBgColor.m_r*bgAmount)), 255);
	int bgGreen = LIBWPD_MIN((int)(((float)tmpFgColor.m_g*fgAmount)+((float)tmpBgColor.m_g*bgAmount)), 255);
	int bgBlue = LIBWPD_MIN((int)(((float)tmpFgColor.m_b*fgAmount)+((float)tmpBgColor.m_b*bgAmount)), 255);

	tmpColor.sprintf("#%.2x%.2x%.2x", bgRed, bgGreen, bgBlue);

	return tmpColor;
}

float WPXContentListener::_movePositionToFirstColumn(float position)
{
	if (m_ps->m_numColumns <= 1)
		return position;
	float tempSpaceRemaining = position - m_ps->m_pageMarginLeft - m_ps->m_sectionMarginLeft;
	position -= m_ps->m_textColumns[0].m_leftGutter;
	for (int i = 0; i < (m_ps->m_textColumns.size() - 1); i++)
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
