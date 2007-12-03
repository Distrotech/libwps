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
#include "WPSContentListener.h"
#include <libwpd/WPXPropertyListVector.h>
#include <libwpd/WPXHLListenerImpl.h>
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

	bool m_isParagraphColumnBreak;
	bool m_isParagraphPageBreak;
	uint8_t m_paragraphJustification;
	float m_paragraphLineSpacing;

	bool m_isDocumentStarted;
	bool m_isPageSpanOpened;
	bool m_isSectionOpened;
	bool m_isPageSpanBreakDeferred;
	bool m_isHeaderFooterWithoutParagraph;

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
private:
	_WPSContentParsingState(const _WPSContentParsingState&);
	_WPSContentParsingState& operator=(const _WPSContentParsingState&);
};

class WPSContentListener
{
public:
	WPSContentListener(std::list<WPSPageSpan> &pageList, WPXHLListenerImpl *listenerImpl);
	virtual ~WPSContentListener();

	void startDocument();
	void endDocument();
	void insertBreak(const uint8_t breakType);

	WPSContentParsingState *m_ps; // parse state
	WPXHLListenerImpl * m_listenerImpl;
	WPXPropertyList m_metaData;

	virtual void _flushText() = 0;

	void _openSection();
	void _closeSection();

	void _openPageSpan();
	void _closePageSpan();

	virtual void _openParagraph();
	void _closeParagraph();

	void _openSpan();
	void _closeSpan();

private:
	WPSContentListener(const WPSContentListener&);
	WPSContentListener& operator=(const WPSContentListener&);

	std::list<WPSPageSpan> &m_pageList;
};

#endif /* WPSCONTENTLISTENER_H */
