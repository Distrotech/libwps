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
	uint16_t m_spec;
	float m_fontSize;
	WPXString m_fontName;
	uint32_t m_lcid;
	uint32_t m_textcolor;

	uint16_t m_fieldcode;

	int m_codepage;

	bool m_isParagraphColumnBreak;
	bool m_isParagraphPageBreak;
	uint8_t m_paragraphJustification;
	float m_paragraphLineSpacing;
	uint32_t m_paraLayoutFlags;

	uint16_t m_footnoteId;
	uint16_t m_endnoteId;

	uint8_t m_numbering;
	uint16_t m_numstyle;
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

	float m_paragraphIndentFirst;

	WPXString m_textBuffer;

private:
	_WPSContentParsingState(const _WPSContentParsingState&);
	_WPSContentParsingState& operator=(const _WPSContentParsingState&);
};

struct ListSignature
{
	uint16_t a,b,c;

	bool operator == (ListSignature &y)
	{
		return (a == y.a) && (b == y.b) && (c == y.c);
	}
};

struct TabPos
{
	float pos;
	char  align;
	char  leader;
};

class WPSContentListener
{
public:
	void startDocument();
	void endDocument();
	void insertBreak(const uint8_t breakType);
	void insertCharacter(const uint16_t character);
	void insertField();

	void openFootnote();
	void closeFootnote();
	void openEndnote();
	void closeEndnote();

	void setSpec(const uint16_t specCode);
	void setTextFont(const WPXString fontName);
	void setFontSize(const uint16_t fontSize);
	void setLCID(const uint32_t lcid);
	void setCodepage(const int codepage);
	void setColor(const unsigned int rgb);
	void setFieldType(uint16_t code);
	void setFieldFormat(uint16_t code);

	void setAlign(const uint8_t align);
	void setParaFlags(const uint32_t flags);
	void setMargins(const float first=0.0, const float left=0.0, const float right=0.0,
			const float before=0.0, const float after=0.0);
	void setTabs(std::vector<TabPos> &tabs);

	void setNumberingType(const uint8_t style);
	void setNumberingProp(const uint16_t type, const uint16_t sep);

	void insertEOL();

	const uint16_t getSpec() const;
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

	int  _getListId();
private:
	WPSContentListener(const WPSContentListener&);
	WPSContentListener& operator=(const WPSContentListener&);

	std::vector<TabPos> m_tabs;
	std::list<WPSPageSpan> &m_pageList;
	std::vector<ListSignature> m_listFormats;
};

#endif /* WPSCONTENTLISTENER_H */
