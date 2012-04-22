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
 * /libwpd2/src/lib/WPXContentListener.h 1.9
 */


#ifndef WPSCONTENTLISTENER_H
#define WPSCONTENTLISTENER_H

#include <vector>

#include <libwpd/WPXPropertyList.h>

#include "libwps_internal.h"

class WPSPageSpan;

struct WPSContentParsingState
{
	WPSContentParsingState();
	~WPSContentParsingState();

	uint32_t m_textAttributeBits;
	float m_fontSize;
	WPXString m_fontName;
	uint32_t m_languageId;
	uint32_t m_textcolor;

	uint16_t m_fieldcode;

	bool m_isParagraphColumnBreak;
	bool m_isParagraphPageBreak;
	uint8_t m_paragraphJustification;
	float m_paragraphLineSpacing;

	uint16_t m_footnoteId;
	uint16_t m_endnoteId;

	uint8_t m_numbering;
	libwps::NumberingType m_numstyle;
	uint16_t m_numsep;

	int m_curListType;
	bool m_isOrdered;

	bool m_isDocumentStarted;
	bool m_isPageSpanOpened;
	bool m_isSectionOpened;
	bool m_isPageSpanBreakDeferred;

	bool m_isSpanOpened;
	bool m_isParagraphOpened;

	bool m_isFootEndNote;

	bool m_isParaListItem;

	std::vector<WPSPageSpan>::iterator m_nextPageSpanIter;
	int m_numPagesRemainingInSpan;

	bool m_sectionAttributesChanged;

	float m_pageFormLength;
	float m_pageFormWidth;
	bool m_pageFormOrientationIsPortrait;

	float m_pageMarginLeft;
	float m_pageMarginRight;
	float m_paragraphMarginLeft;
	float m_paragraphMarginRight;
	float m_paragraphMarginTop;
	float m_paragraphMarginBottom;

	float m_paragraphIndentFirst;

	WPXString m_textBuffer;

private:
	WPSContentParsingState(const WPSContentParsingState &);
	WPSContentParsingState &operator=(const WPSContentParsingState &);
};

namespace WPSContentListenerInternal
{
struct ListSignature;
}

struct WPSTabPos
{
	float m_pos;
	char  m_align;
	char  m_leader;
};

class WPSContentListener
{
public:
	WPSContentListener(std::vector<WPSPageSpan> const &pageList, WPXDocumentInterface *documentInterface);
	virtual ~WPSContentListener();

	void startDocument();
	void endDocument();
	void insertBreak(const uint8_t breakType);
	void insertCharacter(const uint16_t character);
	void insertUnicodeCharacter(uint32_t character);
	void insertField();

	void openFootnote();
	void closeFootnote();
	void openEndnote();
	void closeEndnote();

	void setTextFont(const WPXString fontName);
	void setFontSize(const uint16_t fontSize);
	void setFontAttributes(const uint32_t fontAttributes);
	void setLanguageID(const uint32_t lcid);
	void setColor(const unsigned int rgb);
	void setFieldType(uint16_t code);
	void setFieldFormat(uint16_t code);

	void setAlign(const uint8_t align);
	void setMargins(const float first=0.0, const float left=0.0, const float right=0.0,
	                const float before=0.0, const float after=0.0);
	void setTabs(std::vector<WPSTabPos> &tabs);

	void setNumberingType(const uint8_t style);
	void setNumberingProp(const libwps::NumberingType type, const uint16_t sep);

	void insertEOL();

protected:

	shared_ptr<WPSContentParsingState> m_ps; // parse state
	WPXDocumentInterface *m_documentInterface;
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

	int  _getListId();
private:
	WPSContentListener(const WPSContentListener &);
	WPSContentListener &operator=(const WPSContentListener &);

	std::vector<WPSTabPos> m_tabs;
	std::vector<WPSPageSpan> m_pageList;
	std::vector<WPSContentListenerInternal::ListSignature> m_listFormats;
};

#endif /* WPSCONTENTLISTENER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
