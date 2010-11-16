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
#include <libwpd/WPXPropertyListVector.h>
#include <libwpd/WPXDocumentInterface.h>
#include <list>

typedef struct _WPSContentParsingState WPSContentParsingState;
struct _WPSContentParsingState
{
	_WPSContentParsingState();
	~_WPSContentParsingState();

	uint32_t m_textAttributeBits;
	float m_fontSize;
	WPXString m_fontName;

	bool m_isParagraphColumnBreak;
	bool m_isParagraphPageBreak;
	uint8_t m_paragraphJustification;
	float m_paragraphLineSpacing;

	bool m_isDocumentStarted;
	bool m_isPageSpanOpened;
	bool m_isSectionOpened;
	bool m_isPageSpanBreakDeferred;

	bool m_isSpanOpened;
	bool m_isParagraphOpened;

	std::list<WPSPageSpan>::iterator m_nextPageSpanIter;
	int m_numPagesRemainingInSpan;

	bool m_sectionAttributesChanged;

	float m_pageFormLength;
	float m_pageFormWidth;
	WPSFormOrientation m_pageFormOrientation;

	float m_pageMarginLeft;
	float m_pageMarginRight;
	float m_paragraphMarginLeft;
	float m_paragraphMarginRight;
	float m_paragraphMarginTop;
	float m_paragraphMarginBottom;

	WPXString m_textBuffer;

private:
	_WPSContentParsingState(const _WPSContentParsingState&);
	_WPSContentParsingState& operator=(const _WPSContentParsingState&);
};

class WPSContentListener
{
public:
	void startDocument();
	void endDocument();
	void insertBreak(const uint8_t breakType);
	void insertCharacter(const uint16_t character);

	void setTextFont(const WPXString fontName);
	void setFontSize(const uint16_t fontSize);
	
	void insertEOL();

protected:
	WPSContentListener(std::list<WPSPageSpan> &pageList, WPXDocumentInterface *documentInterface);
	virtual ~WPSContentListener();

	WPSContentParsingState *m_ps; // parse state
	WPXDocumentInterface * m_documentInterface;
	WPXPropertyList m_metaData;

	void _flushText();

	void _openSection();
	void _closeSection();

	void _openPageSpan();
	void _closePageSpan();

	void _openParagraph();
	void _closeParagraph();

	void _openSpan();
	void _closeSpan();

	void _insertText(const WPXString &textBuffer);

private:
	WPSContentListener(const WPSContentListener&);
	WPSContentListener& operator=(const WPSContentListener&);

	std::list<WPSPageSpan> &m_pageList;
};

#endif /* WPSCONTENTLISTENER_H */
