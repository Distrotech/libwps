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

class WPSList;
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
	libwps::Justification m_paragraphJustification;
	float m_paragraphLineSpacing;

	bool m_isDocumentStarted;
	bool m_isPageSpanOpened;
	bool m_isSectionOpened;
	bool m_isPageSpanBreakDeferred;

	bool m_isSpanOpened;
	bool m_isParagraphOpened;

	bool m_inSubDocument;
	bool m_isListElementOpened;
	bool m_isTableOpened, m_isTableCellOpened;

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

	float m_paragraphTextIndent;

	WPXString m_textBuffer;

	//! the list
	shared_ptr<WPSList> m_list;
	//! a stack used to know what is open
	std::vector<bool> m_listOrderedLevels;
	//! the actual list id
	int m_actualListId;
	//! the actual list level
	int m_currentListLevel;

	double m_listReferencePosition; // position from the left page margin of the list number/bullet
	double m_listBeginPosition; // position from the left page margin of the beginning of the list
private:
};

namespace WPSContentListenerInternal
{
struct ListSignature;
}

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

	void setTextFont(const WPXString fontName);
	void setFontSize(const uint16_t fontSize);
	void setFontAttributes(const uint32_t fontAttributes);
	void setTextLanguage(const uint32_t lcid);
	void setTextColor(const unsigned int rgb);
	void setFieldType(uint16_t code);
	void setFieldFormat(uint16_t code);

	void setAlign(libwps::Justification align);
	//! sets the first paragraph text indent. \warning unit are given in inches
	void setParagraphTextIndent(double margin);
	/** sets the paragraph margin.
	 *
	 * \param margin is given in inches
	 * \param pos in WPS_LEFT, WPS_RIGHT, WPS_TOP, WPS_BOTTOM
	 */
	void setParagraphMargin(double margin, int pos);

	void setTabs(std::vector<WPSTabStop> &tabs);

	/** function to set the actual list
	 *
	 * \note set the listid if not set
	 */
	void setCurrentList(shared_ptr<WPSList> list);

	/** returns the current list */
	shared_ptr<WPSList> getCurrentList() const;

	/** function to create an unordered list
	 *
	 * \warning minimal implementation...*/
	void setCurrentListLevel(int level);

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

	void _resetParagraphState(const bool isListElement=false);
	void _appendParagraphProperties(WPXPropertyList &propList, const bool isListElement=false);
	void _getTabStops(WPXPropertyListVector &tabStops);

	void _openListElement();
	void _closeListElement();
	//! closes the previous item (if needed) and open the new one
	void _changeList();
	//! sets list properties when the list changes
	void _handleListChange();
private:
	WPSContentListener(const WPSContentListener &);
	WPSContentListener &operator=(const WPSContentListener &);

	std::vector<WPSTabStop> m_tabs;
	std::vector<WPSPageSpan> m_pageList;
	std::vector<WPSContentListenerInternal::ListSignature> m_listFormats;
};

#endif /* WPSCONTENTLISTENER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
