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
 * /libwpd2/src/lib/WPXContentListener.h 1.9
 */


#ifndef WPSCONTENTLISTENER_H
#define WPSCONTENTLISTENER_H

#include "libwps_internal.h"
#include "WPSPageSpan.h"
#include "WPSListener.h"
#include <libwpd/WPXPropertyListVector.h>
#include <libwpd/WPXDocumentInterface.h>
#include <vector>
#include <list>
#include <set>

typedef struct _WPSContentParsingState WPSContentParsingState;
struct _WPSContentParsingState
{
	_WPSContentParsingState();
	~_WPSContentParsingState();

	uint32_t m_textAttributeBits;
	float m_fontSize;
	WPXString *m_fontName;
	RGBSColor *m_fontColor;
	RGBSColor *m_highlightColor;

	bool m_isParagraphColumnBreak;
	bool m_isParagraphPageBreak;
	uint8_t m_paragraphJustification;
	uint8_t m_tempParagraphJustification; // TODO: remove this one after the tabs are properly implemented
	float m_paragraphLineSpacing;

	bool m_isDocumentStarted;
	bool m_isPageSpanOpened;
	bool m_isSectionOpened;
	bool m_isPageSpanBreakDeferred;
	bool m_isHeaderFooterWithoutParagraph;

	bool m_isSpanOpened;
	bool m_isParagraphOpened;
	bool m_isListElementOpened;

	std::vector<unsigned int> m_numRowsToSkip;
	bool m_wasHeaderRow;
	bool m_isCellWithoutParagraph;
	uint32_t m_cellAttributeBits;
	uint8_t m_paragraphJustificationBeforeColumns;
	
	std::list<WPSPageSpan>::iterator m_nextPageSpanIter;
	int m_numPagesRemainingInSpan;

	bool m_sectionAttributesChanged;
	int m_numColumns;
	std::vector < WPSColumnDefinition > m_textColumns;
	bool m_isTextColumnWithoutParagraph;

	float m_pageFormLength;
	float m_pageFormWidth;
	WPSFormOrientation m_pageFormOrientation;

	float m_pageMarginLeft;
	float m_pageMarginRight;
	float m_paragraphMarginLeft;  // resulting paragraph margin that is one of the paragraph
	float m_paragraphMarginRight; // properties
	float m_paragraphMarginTop;
	float m_paragraphMarginBottom;
	float m_leftMarginByPageMarginChange;  // part of the margin due to the PAGE margin change
	float m_rightMarginByPageMarginChange; // inside a page that already has content.
	float m_sectionMarginLeft;  // In multicolumn sections, the above two will be rather interpreted
	float m_sectionMarginRight; // as section margin change 
	float m_leftMarginByParagraphMarginChange;  // part of the margin due to the PARAGRAPH
	float m_rightMarginByParagraphMarginChange; // margin change (in WP6)
	float m_leftMarginByTabs;  // part of the margin due to the LEFT or LEFT/RIGHT Indent; the
	float m_rightMarginByTabs; // only part of the margin that is reset at the end of a paragraph

	float m_listReferencePosition; // position from the left page margin of the list number/bullet
	float m_listBeginPosition; // position from the left page margin of the beginning of the list

	float m_paragraphTextIndent; // resulting first line indent that is one of the paragraph properties
	float m_textIndentByParagraphIndentChange; // part of the indent due to the PARAGRAPH indent (WP6???)
	float m_textIndentByTabs; // part of the indent due to the "Back Tab" or "Left Tab"

	uint8_t m_currentListLevel;
	
	uint16_t m_alignmentCharacter;
	std::vector<WPSTabStop> m_tabStops;
	bool m_isTabPositionRelative;

	bool m_isNote;
private:
	_WPSContentParsingState(const _WPSContentParsingState&);
	_WPSContentParsingState& operator=(const _WPSContentParsingState&);
};

class WPSContentListener : public WPSListener
{
protected:
	WPSContentListener(std::list<WPSPageSpan> &pageList, WPXDocumentInterface *listenerImpl);
	virtual ~WPSContentListener();

	void startDocument();
	void endDocument();
	void insertBreak(const uint8_t breakType);
	void lineSpacingChange(const float lineSpacing);
	void justificationChange(const uint8_t justification);

	WPSContentParsingState *m_ps; // parse state
	WPXDocumentInterface * m_documentInterface;
	WPXPropertyList m_metaData;

	virtual void _flushText() = 0;
	virtual void _changeList() = 0;

	void _openSection();
	void _closeSection();

	void _openPageSpan();
	void _closePageSpan();

	void _appendParagraphProperties(WPXPropertyList &propList, const bool isListElement=false);
	void _getTabStops(WPXPropertyListVector &tabStops);
	void _appendJustification(WPXPropertyList &propList, int justification);
	void _resetParagraphState(const bool isListElement=false);
	virtual void _openParagraph();
	void _closeParagraph();

	void _openListElement();
	void _closeListElement();

	void _openSpan();
	void _closeSpan();

	float _movePositionToFirstColumn(float position);

private:
	WPSContentListener(const WPSContentListener&);
	WPSContentListener& operator=(const WPSContentListener&);
	WPXString _colorToString(const RGBSColor * color);
	WPXString _mergeColorsToString(const RGBSColor *fgColor, const RGBSColor *bgColor);
};

#endif /* WPSCONTENTLISTENER_H */
