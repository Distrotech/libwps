/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwps.sourceforge.net
 */

#include <iomanip>
#include <map>
#include <sstream>
#include <stdio.h>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WKSContentListener.h"

#include "WPSCell.h"
#include "WPSFont.h"
#include "WPSGraphicShape.h"
#include "WPSPageSpan.h"
#include "WPSParagraph.h"
#include "WPSPosition.h"
#include "WKSSubDocument.h"

////////////////////////////////////////////////////////////
//! the document state
struct WKSDocumentParsingState
{
	//! constructor
	explicit WKSDocumentParsingState(std::vector<WPSPageSpan> const &pageList);
	//! destructor
	~WKSDocumentParsingState();

	std::vector<WPSPageSpan> m_pageList;
	librevenge::RVNGPropertyList m_metaData;

	bool m_isDocumentStarted, m_isHeaderFooterStarted;
	std::vector<WPSSubDocumentPtr> m_subDocuments; /** list of document actually open */

	/** a map cell's format to id */
	std::map<WPSCellFormat,int,WPSCellFormat::CompareFormat> m_numberingIdMap;

private:
	WKSDocumentParsingState(const WKSDocumentParsingState &);
	WKSDocumentParsingState &operator=(const WKSDocumentParsingState &);
};

WKSDocumentParsingState::WKSDocumentParsingState(std::vector<WPSPageSpan> const &pageList) :
	m_pageList(pageList), m_metaData(), m_isDocumentStarted(false), m_isHeaderFooterStarted(false), m_subDocuments(), m_numberingIdMap()
{
}

WKSDocumentParsingState::~WKSDocumentParsingState()
{
}

////////////////////////////////////////////////////////////
//! the spreadsheet state
struct WKSContentParsingState
{
	WKSContentParsingState();
	~WKSContentParsingState();

	bool m_isPageSpanOpened;
	bool m_isFrameOpened;

	unsigned m_currentPage;
	int m_numPagesRemainingInSpan;
	int m_currentPageNumber;

	double m_pageFormLength;
	double m_pageFormWidth;
	bool m_pageFormOrientationIsPortrait;

	double m_pageMarginLeft;
	double m_pageMarginRight;
	double m_pageMarginTop;
	double m_pageMarginBottom;

	librevenge::RVNGString m_textBuffer;
	int m_numDeferredTabs;

	WPSFont m_font;
	WPSParagraph m_paragraph;

	bool m_isParagraphColumnBreak;
	bool m_isParagraphPageBreak;

	bool m_isSpanOpened;
	bool m_isParagraphOpened;

	bool m_isSheetOpened;
	bool m_isSheetRowOpened;
	bool m_isSheetColumnOpened;
	bool m_isSheetCellOpened;

	bool m_inSubDocument;

	bool m_isNote;
	libwps::SubDocumentType m_subDocumentType;

private:
	WKSContentParsingState(const WKSContentParsingState &);
	WKSContentParsingState &operator=(const WKSContentParsingState &);
};

WKSContentParsingState::WKSContentParsingState() :
	m_isPageSpanOpened(false), m_isFrameOpened(false), m_currentPage(0), m_numPagesRemainingInSpan(0), m_currentPageNumber(1),
	m_pageFormLength(11.0),	m_pageFormWidth(8.5f), m_pageFormOrientationIsPortrait(true),
	m_pageMarginLeft(1.0), m_pageMarginRight(1.0), m_pageMarginTop(1.0), m_pageMarginBottom(1.0),

	m_textBuffer(""), m_numDeferredTabs(0),

	m_font(), m_paragraph(),

	m_isParagraphColumnBreak(false), m_isParagraphPageBreak(false),

	m_isSpanOpened(false), m_isParagraphOpened(false),

	m_isSheetOpened(false), m_isSheetRowOpened(false), m_isSheetColumnOpened(false),
	m_isSheetCellOpened(false),

	m_inSubDocument(false),
	m_isNote(false),
	m_subDocumentType(libwps::DOC_NONE)
{
	m_font.m_size=12.0;
	m_font.m_name="Times New Roman";
}

WKSContentParsingState::~WKSContentParsingState()
{
}

WKSContentListener::WKSContentListener(std::vector<WPSPageSpan> const &pageList, librevenge::RVNGSpreadsheetInterface *documentInterface) :
	m_ds(new WKSDocumentParsingState(pageList)), m_ps(new WKSContentParsingState), m_psStack(),
	m_documentInterface(documentInterface)
{
}

WKSContentListener::~WKSContentListener()
{
}

///////////////////
// text data
///////////////////
void WKSContentListener::insertCharacter(uint8_t character)
{
	if (character >= 0x80)
	{
		insertUnicode(character);
		return;
	}
	_flushDeferredTabs();
	if (!m_ps->m_isSpanOpened) _openSpan();
	m_ps->m_textBuffer.append(char(character));
}

void WKSContentListener::insertUnicode(uint32_t val)
{
	// undef character, we skip it
	if (val == 0xfffd) return;
	_flushDeferredTabs();
	if (!m_ps->m_isSpanOpened) _openSpan();
	libwps::appendUnicode(val, m_ps->m_textBuffer);
}

void WKSContentListener::insertUnicodeString(librevenge::RVNGString const &str)
{
	_flushDeferredTabs();
	if (!m_ps->m_isSpanOpened) _openSpan();
	m_ps->m_textBuffer.append(str);
}

void WKSContentListener::insertEOL(bool soft)
{
	if (!m_ps->m_isParagraphOpened)
		_openSpan();
	_flushDeferredTabs();

	if (soft)
	{
		if (m_ps->m_isSpanOpened)
			_flushText();
		m_documentInterface->insertLineBreak();
	}
	else if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	// sub/superscript must not survive a new line
	static const uint32_t s_subsuperBits = WPS_SUBSCRIPT_BIT | WPS_SUPERSCRIPT_BIT;
	if (m_ps->m_font.m_attributes & s_subsuperBits)
		m_ps->m_font.m_attributes &= ~s_subsuperBits;
}

void WKSContentListener::insertTab()
{
	if (!m_ps->m_isParagraphOpened)
	{
		m_ps->m_numDeferredTabs++;
		return;
	}
	if (m_ps->m_isSpanOpened) _flushText();
	m_ps->m_numDeferredTabs++;
	_flushDeferredTabs();
}

void WKSContentListener::insertBreak(const uint8_t breakType)
{
	switch (breakType)
	{
	case WPS_COLUMN_BREAK:
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		m_ps->m_isParagraphColumnBreak = true;
		break;
	case WPS_PAGE_BREAK:
		if (m_ps->m_isParagraphOpened)
			_closeParagraph();
		m_ps->m_isParagraphPageBreak = true;
		break;
	default:
		break;
	}
}

void WKSContentListener::_insertBreakIfNecessary(librevenge::RVNGPropertyList &propList)
{
	if (m_ps->m_isParagraphPageBreak && !m_ps->m_inSubDocument)
	{
		// no hard page-breaks in subdocuments
		propList.insert("fo:break-before", "page");
		m_ps->m_isParagraphPageBreak = false;
	}
}

///////////////////
// font/character format
///////////////////
void WKSContentListener::setFont(const WPSFont &font)
{
	WPSFont newFont(font);
	if (font.m_size<=0)
		newFont.m_size=m_ps->m_font.m_size;
	if (font.m_name.empty())
		newFont.m_name=m_ps->m_font.m_name;
	if (font.m_languageId <= 0)
		newFont.m_languageId=m_ps->m_font.m_languageId;
	if (m_ps->m_font==newFont) return;
	_closeSpan();
	m_ps->m_font=newFont;
}

WPSFont const &WKSContentListener::getFont() const
{
	return m_ps->m_font;
}

///////////////////
// paragraph tabs, tabs/...
///////////////////
bool WKSContentListener::isParagraphOpened() const
{
	return m_ps->m_isParagraphOpened;
}

WPSParagraph const &WKSContentListener::getParagraph() const
{
	return m_ps->m_paragraph;
}

void WKSContentListener::setParagraph(const WPSParagraph &para)
{
	m_ps->m_paragraph=para;
}

///////////////////
// field :
///////////////////
#include <time.h>
void WKSContentListener::insertField(WKSContentListener::FieldType type)
{
	switch (type)
	{
	case None:
		break;
	case PageNumber:
	case NextPageNumber:
	{
		_flushText();
		_openSpan();
		librevenge::RVNGPropertyList propList;
		propList.insert("style:num-format", libwps::numberingTypeToString(libwps::ARABIC).c_str());
		propList.insert("librevenge:field-type", "text:page-number");
		if (type==NextPageNumber)
			propList.insert("text:select-page", "next");
		m_documentInterface->insertField(propList);
		break;
	}
	case Database:
	{
		librevenge::RVNGString tmp("#DATAFIELD#");
		insertUnicodeString(tmp);
		break;
	}
	case Title:
	{
		librevenge::RVNGString tmp("#TITLE#");
		insertUnicodeString(tmp);
		break;
	}
	case Date:
		insertDateTimeField("%m/%d/%y");
		break;
	case Time:
		insertDateTimeField("%I:%M:%S %p");
		break;
	case Link:
	default:
		WPS_DEBUG_MSG(("WKSContentListener::insertField: must not be called with type=%d\n", int(type)));
		break;
	}
}

void WKSContentListener::insertDateTimeField(char const *format)
{
	if (!format)
	{
		WPS_DEBUG_MSG(("WKSContentListener::insertDateTimeField: oops, can not find the format\n"));
		return;
	}
	time_t now = time(0L);
	struct tm timeinfo;
	if (localtime_r(&now, &timeinfo))
	{
		char buf[256];
		strftime(buf, 256, format, &timeinfo);
		insertUnicodeString(librevenge::RVNGString(buf));
	}
}

///////////////////
// document
///////////////////
void WKSContentListener::setDocumentLanguage(int lcid)
{
	if (lcid <= 0) return;
	std::string lang = libwps_tools_win::Language::localeName(lcid);
	if (!lang.length()) return;
	m_ds->m_metaData.insert("librevenge:language", lang.c_str());
}

void WKSContentListener::startDocument()
{
	if (m_ds->m_isDocumentStarted)
	{
		WPS_DEBUG_MSG(("WKSContentListener::startDocument: the document is already started\n"));
		return;
	}

	m_documentInterface->startDocument(librevenge::RVNGPropertyList());
	m_ds->m_isDocumentStarted = true;

	// FIXME: this is stupid, we should store a property list filled with the relevant metadata
	// and then pass that directly..
	m_documentInterface->setDocumentMetaData(m_ds->m_metaData);
}

void WKSContentListener::endDocument()
{
	if (!m_ds->m_isDocumentStarted)
	{
		WPS_DEBUG_MSG(("WKSContentListener::startDocument: the document is not started\n"));
		return;
	}

	if (m_ps->m_isSheetOpened)
		closeSheet();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	// close the document nice and tight
	_closePageSpan();
	m_documentInterface->endDocument();
	m_ds->m_isDocumentStarted = false;
}

///////////////////
// paragraph
///////////////////
void WKSContentListener::_openParagraph()
{
	if (m_ps->m_isSheetOpened && !m_ps->m_isSheetCellOpened)
		return;

	if (!m_ps->m_isPageSpanOpened)
		_openPageSpan();

	if (m_ps->m_isParagraphOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::_openParagraph: a paragraph (or a list) is already opened"));
		return;
	}
	librevenge::RVNGPropertyList propList;
	_appendParagraphProperties(propList);

	if (!m_ps->m_isParagraphOpened)
		m_documentInterface->openParagraph(propList);

	_resetParagraphState();
}

void WKSContentListener::_closeParagraph()
{
	if (m_ps->m_isParagraphOpened)
	{
		if (m_ps->m_isSpanOpened)
			_closeSpan();

		m_documentInterface->closeParagraph();
	}

	m_ps->m_isParagraphOpened = false;
	m_ps->m_paragraph.m_listLevelIndex = 0;
}

void WKSContentListener::_resetParagraphState(const bool /*isListElement*/)
{
	m_ps->m_isParagraphColumnBreak = false;
	m_ps->m_isParagraphPageBreak = false;
	m_ps->m_isParagraphOpened = true;
}

void WKSContentListener::_appendParagraphProperties(librevenge::RVNGPropertyList &propList, const bool /*isListElement*/)
{
	m_ps->m_paragraph.addTo(propList, m_ps->m_isSheetOpened);

	_insertBreakIfNecessary(propList);
}

///////////////////
// span
///////////////////
void WKSContentListener::_openSpan()
{
	if (m_ps->m_isSpanOpened)
		return;

	if (m_ps->m_isSheetOpened && !m_ps->m_isSheetCellOpened)
		return;

	if (!m_ps->m_isParagraphOpened)
		_openParagraph();

	librevenge::RVNGPropertyList propList;
	m_ps->m_font.addTo(propList);

	m_documentInterface->openSpan(propList);

	m_ps->m_isSpanOpened = true;
}

void WKSContentListener::_closeSpan()
{
	if (!m_ps->m_isSpanOpened)
		return;

	_flushText();
	m_documentInterface->closeSpan();
	m_ps->m_isSpanOpened = false;
}

///////////////////
// text (send data)
///////////////////
void WKSContentListener::_flushDeferredTabs()
{
	if (m_ps->m_numDeferredTabs == 0) return;

	// CHECKME: the tab are not underline even if the underline bit is set
	uint32_t oldTextAttributes = m_ps->m_font.m_attributes;
	uint32_t newAttributes = oldTextAttributes & uint32_t(~WPS_UNDERLINE_BIT) &
	                         uint32_t(~WPS_OVERLINE_BIT);
	if (oldTextAttributes != newAttributes)
	{
		_closeSpan();
		m_ps->m_font.m_attributes=newAttributes;
	}
	if (!m_ps->m_isSpanOpened) _openSpan();
	for (; m_ps->m_numDeferredTabs > 0; m_ps->m_numDeferredTabs--)
		m_documentInterface->insertTab();
	if (oldTextAttributes != newAttributes)
	{
		_closeSpan();
		m_ps->m_font.m_attributes=oldTextAttributes;
	}
}

void WKSContentListener::_flushText()
{
	if (m_ps->m_textBuffer.len() == 0) return;

	// when some many ' ' follows each other, call insertSpace
	librevenge::RVNGString tmpText;
	int numConsecutiveSpaces = 0;
	librevenge::RVNGString::Iter i(m_ps->m_textBuffer);
	for (i.rewind(); i.next();)
	{
		if (*(i()) == 0x20) // this test is compatible with unicode format
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
			tmpText.append(i());
	}
	m_documentInterface->insertText(tmpText);
	m_ps->m_textBuffer.clear();
}

///////////////////
// Comment
///////////////////
void WKSContentListener::insertComment(WPSSubDocumentPtr &subDocument)
{
	if (m_ps->m_isNote)
	{
		WPS_DEBUG_MSG(("WKSContentListener::insertComment try to insert a note recursively (ingnored)"));
		return;
	}

	if (!m_ps->m_isParagraphOpened)
		_openParagraph();
	else
	{
		_flushText();
		_closeSpan();
	}

	librevenge::RVNGPropertyList propList;
	m_documentInterface->openComment(propList);

	m_ps->m_isNote = true;
	handleSubDocument(subDocument, libwps::DOC_COMMENT_ANNOTATION);

	m_documentInterface->closeComment();
	m_ps->m_isNote = false;
}

void WKSContentListener::insertTextBox
(WPSPosition const &pos, WPSSubDocumentPtr subDocument, WPSGraphicStyle const &frameStyle)
{
	if (!_openFrame(pos, frameStyle)) return;

	librevenge::RVNGPropertyList propList;
	m_documentInterface->openTextBox(propList);
	handleSubDocument(subDocument, libwps::DOC_TEXT_BOX);
	m_documentInterface->closeTextBox();

	_closeFrame();
}

void WKSContentListener::insertPicture
(WPSPosition const &pos, const librevenge::RVNGBinaryData &binaryData, std::string type,
 WPSGraphicStyle const &style)
{
	if (!_openFrame(pos, style)) return;

	librevenge::RVNGPropertyList propList;
	propList.insert("librevenge:mime-type", type.c_str());
	propList.insert("office:binary-data", binaryData);
	m_documentInterface->insertBinaryObject(propList);

	_closeFrame();
}

void WKSContentListener::insertObject
(WPSPosition const &pos, const WPSEmbeddedObject &obj, WPSGraphicStyle const &style)
{
	if (!_openFrame(pos, style)) return;

	librevenge::RVNGPropertyList propList;
	if (obj.addTo(propList))
		m_documentInterface->insertBinaryObject(propList);

	_closeFrame();
}

void WKSContentListener::insertPicture
(WPSPosition const &pos, const WPSGraphicShape &shape, WPSGraphicStyle const &style)
{
	librevenge::RVNGPropertyList shapePList;
	_handleFrameParameters(shapePList, pos);
	shapePList.remove("svg:x");
	shapePList.remove("svg:y");

	librevenge::RVNGPropertyList styleList;
	style.addTo(styleList, shape.getType()==WPSGraphicShape::Line);
	float factor=pos.getScaleFactor(pos.unit(), librevenge::RVNG_POINT);
	Vec2f decal = factor*pos.origin();
	switch (shape.addTo(decal, style.hasSurface(), shapePList))
	{
	case WPSGraphicShape::C_Ellipse:
		m_documentInterface->defineGraphicStyle(styleList);
		m_documentInterface->drawEllipse(shapePList);
		break;
	case WPSGraphicShape::C_Path:
		m_documentInterface->defineGraphicStyle(styleList);
		m_documentInterface->drawPath(shapePList);
		break;
	case WPSGraphicShape::C_Polyline:
		m_documentInterface->defineGraphicStyle(styleList);
		m_documentInterface->drawPolyline(shapePList);
		break;
	case WPSGraphicShape::C_Polygon:
		m_documentInterface->defineGraphicStyle(styleList);
		m_documentInterface->drawPolygon(shapePList);
		break;
	case WPSGraphicShape::C_Rectangle:
		m_documentInterface->defineGraphicStyle(styleList);
		m_documentInterface->drawRectangle(shapePList);
		break;
	case WPSGraphicShape::C_Bad:
		break;
	default:
		WPS_DEBUG_MSG(("WKSSpreadsheetListener::insertPicture: unexpected shape\n"));
		break;
	}
}

///////////////////
// frame
///////////////////
bool WKSContentListener::_openFrame(WPSPosition const &pos, WPSGraphicStyle const &style)
{
	if (m_ps->m_isFrameOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::openFrame: called but a frame is already opened\n"));
		return false;
	}

	switch (pos.m_anchorTo)
	{
	case WPSPosition::Page:
	case WPSPosition::PageContent:
		break;
	case WPSPosition::Paragraph:
	case WPSPosition::ParagraphContent:
		if (m_ps->m_isParagraphOpened)
			_flushText();
		else
			_openParagraph();
		break;
	case WPSPosition::CharBaseLine:
	case WPSPosition::Char:
		if (m_ps->m_isSpanOpened)
			_flushText();
		else
			_openSpan();
		break;
	default:
		WPS_DEBUG_MSG(("WKSContentListener::openFrame: can not determine the anchor\n"));
		return false;
	}

	librevenge::RVNGPropertyList propList;
	style.addFrameTo(propList);
	if (!propList["draw:fill"])
		propList.insert("draw:fill","none");
	_handleFrameParameters(propList, pos);
	m_documentInterface->openFrame(propList);

	m_ps->m_isFrameOpened = true;
	return true;
}

void WKSContentListener::_closeFrame()
{
	if (!m_ps->m_isFrameOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::closeFrame: called but no frame is already opened\n"));
		return;
	}
	m_documentInterface->closeFrame();
	m_ps->m_isFrameOpened = false;
}

void WKSContentListener::_handleFrameParameters
(librevenge::RVNGPropertyList &propList, WPSPosition const &pos)
{
	Vec2f origin = pos.origin();
	librevenge::RVNGUnit unit = pos.unit();
	float inchFactor=pos.getInvUnitScale(librevenge::RVNG_INCH);
	float pointFactor = pos.getInvUnitScale(librevenge::RVNG_POINT);

	propList.insert("svg:width", double(pos.size()[0]), unit);
	propList.insert("svg:height", double(pos.size()[1]), unit);
	if (pos.naturalSize().x() > 4*pointFactor && pos.naturalSize().y() > 4*pointFactor)
	{
		propList.insert("librevenge:naturalWidth", pos.naturalSize().x(), pos.unit());
		propList.insert("librevenge:naturalHeight", pos.naturalSize().y(), pos.unit());
	}

	if (pos.m_wrapping ==  WPSPosition::WDynamic)
		propList.insert("style:wrap", "dynamic");
	else if (pos.m_wrapping ==  WPSPosition::WRunThrough)
	{
		propList.insert("style:wrap", "run-through");
		propList.insert("style:run-through", "background");
	}
	else
		propList.insert("style:wrap", "none");

	if (pos.m_anchorTo != WPSPosition::Page && pos.m_anchorTo != WPSPosition::PageContent)
	{
		WPS_DEBUG_MSG(("WKSContentListener::openFrame: only implemented for page anchor\n"));
		return;
	}

	// Page position seems to do not use the page margin...
	propList.insert("text:anchor-type", "page");
	if (pos.page() > 0) propList.insert("text:anchor-page-number", pos.page());
	double  w = m_ps->m_pageFormWidth;
	double h = m_ps->m_pageFormLength;
	w *= inchFactor;
	h *= inchFactor;

	librevenge::RVNGString relPos(pos.m_anchorTo == WPSPosition::Page ? "page" : "page-content");
	propList.insert("style:vertical-rel", relPos);
	propList.insert("style:horizontal-rel", relPos);

	double newPosition;
	switch (pos.m_yPos)
	{
	case WPSPosition::YFull:
		propList.insert("svg:height", double(h), unit);
	// fall-through intended
	case WPSPosition::YTop:
		if (origin[1] < 0.0 || origin[1] > 0.0)
		{
			propList.insert("style:vertical-pos", "from-top");
			newPosition = origin[1];
			if (newPosition > h -pos.size()[1])
				newPosition = h - pos.size()[1];
			propList.insert("svg:y", double(newPosition), unit);
		}
		else
			propList.insert("style:vertical-pos", "top");
		break;
	case WPSPosition::YCenter:
		if (origin[1] < 0.0 || origin[1] > 0.0)
		{
			propList.insert("style:vertical-pos", "from-top");
			newPosition = (h - pos.size()[1])/2.0;
			if (newPosition > h -pos.size()[1]) newPosition = h - pos.size()[1];
			propList.insert("svg:y", double(newPosition), unit);
		}
		else
			propList.insert("style:vertical-pos", "middle");
		break;
	case WPSPosition::YBottom:
		if (origin[1] < 0.0 || origin[1] > 0.0)
		{
			propList.insert("style:vertical-pos", "from-top");
			newPosition = h - pos.size()[1]-origin[1];
			if (newPosition > h -pos.size()[1]) newPosition = h -pos.size()[1];
			else if (newPosition < 0) newPosition = 0;
			propList.insert("svg:y", double(newPosition), unit);
		}
		else
			propList.insert("style:vertical-pos", "bottom");
		break;
	default:
		break;
	}

	switch (pos.m_xPos)
	{
	case WPSPosition::XFull:
		propList.insert("svg:width", double(w), unit);
	// fallthrough intended
	case WPSPosition::XLeft:
		if (origin[0] < 0.0 || origin[0] > 0.0)
		{
			propList.insert("style:horizontal-pos", "from-left");
			propList.insert("svg:x", double(origin[0]), unit);
		}
		else
			propList.insert("style:horizontal-pos", "left");
		break;
	case WPSPosition::XRight:
		if (origin[0] < 0.0 || origin[0] > 0.0)
		{
			propList.insert("style:horizontal-pos", "from-left");
			propList.insert("svg:x",double(w - pos.size()[0] + origin[0]), unit);
		}
		else
			propList.insert("style:horizontal-pos", "right");
		break;
	case WPSPosition::XCenter:
	default:
		if (origin[0] < 0.0 || origin[0] > 0.0)
		{
			propList.insert("style:horizontal-pos", "from-left");
			propList.insert("svg:x", double((w - pos.size()[0])/2. + origin[0]), unit);
		}
		else
			propList.insert("style:horizontal-pos", "center");
		break;
	}
}

///////////////////
// subdocument
///////////////////
void WKSContentListener::handleSubDocument(WPSSubDocumentPtr &subDocument, libwps::SubDocumentType subDocumentType)
{
	_pushParsingState();
	_startSubDocument();

	m_ps->m_subDocumentType = subDocumentType;
	m_ps->m_isPageSpanOpened = true;

	// Check whether the document is calling itself
	bool sendDoc = true;
	for (size_t i = 0; i < m_ds->m_subDocuments.size(); i++)
	{
		if (!subDocument)
			break;
		if (subDocument == m_ds->m_subDocuments[i])
		{
			WPS_DEBUG_MSG(("WKSContentListener::handleSubDocument: recursif call, stop...\n"));
			sendDoc = false;
			break;
		}
	}
	if (sendDoc)
	{
		if (subDocument)
		{
			m_ds->m_subDocuments.push_back(subDocument);
			shared_ptr<WKSContentListener> listen(this, WPS_shared_ptr_noop_deleter<WKSContentListener>());
			try
			{
				WKSSubDocument *subDoc=dynamic_cast<WKSSubDocument *>(subDocument.get());
				if (subDoc)
					subDoc->parse(listen, subDocumentType);
				else
				{
					WPS_DEBUG_MSG(("Works: WKSContentListener::handleSubDocument bad subdocument\n"));
				}
			}
			catch (...)
			{
				WPS_DEBUG_MSG(("Works: WKSContentListener::handleSubDocument exception catched \n"));
			}
			m_ds->m_subDocuments.pop_back();
		}
	}

	_endSubDocument();
	_popParsingState();
}

void WKSContentListener::_startSubDocument()
{
	m_ds->m_isDocumentStarted = true;
	m_ps->m_inSubDocument = true;
}

void WKSContentListener::_endSubDocument()
{
	if (m_ps->m_isSheetOpened)
		closeSheet();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();
}

///////////////////
// sheet
///////////////////
void WKSContentListener::openSheet(std::vector<float> const &colWidth, librevenge::RVNGUnit unit, librevenge::RVNGString const &name)
{
	if (m_ps->m_isSheetOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::openSheet: called with m_isSheetOpened=true\n"));
		return;
	}
	if (!m_ps->m_isPageSpanOpened)
		_openPageSpan();
	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	_pushParsingState();
	_startSubDocument();
	m_ps->m_subDocumentType = libwps::DOC_TABLE;
	m_ps->m_isPageSpanOpened = true;

	librevenge::RVNGPropertyList propList;

	librevenge::RVNGPropertyListVector columns;

	size_t nCols = colWidth.size();
	for (size_t c = 0; c < nCols; c++)
	{
		librevenge::RVNGPropertyList column;
		column.insert("style:column-width", colWidth[c], unit);
		columns.append(column);
	}
	propList.insert("librevenge:columns", columns);
	if (!name.empty())
		propList.insert("librevenge:sheet-name", name);
	m_documentInterface->openSheet(propList);
	m_ps->m_isSheetOpened = true;
}

void WKSContentListener::closeSheet()
{
	if (!m_ps->m_isSheetOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::closeSheet: called with m_isSheetOpened=false\n"));
		return;
	}

	m_ps->m_isSheetOpened = false;
	_endSubDocument();
	m_documentInterface->closeSheet();

	_popParsingState();
}

void WKSContentListener::openSheetRow(float h, librevenge::RVNGUnit unit, bool headerRow)
{
	if (m_ps->m_isSheetRowOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::openSheetRow: called with m_isSheetRowOpened=true\n"));
		return;
	}
	if (!m_ps->m_isSheetOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::openSheetRow: called with m_isSheetOpened=false\n"));
		return;
	}
	librevenge::RVNGPropertyList propList;
	propList.insert("librevenge:is-header-row", headerRow);

	if (h > 0)
		propList.insert("style:row-height", h, unit);
	else if (h < 0)
		propList.insert("style:min-row-height", -h, unit);
	m_documentInterface->openSheetRow(propList);
	m_ps->m_isSheetRowOpened = true;
}

void WKSContentListener::closeSheetRow()
{
	if (!m_ps->m_isSheetRowOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::openSheetRow: called with m_isSheetRowOpened=false\n"));
		return;
	}
	m_ps->m_isSheetRowOpened = false;
	m_documentInterface->closeSheetRow();
}

void WKSContentListener::openSheetCell(WPSCell const &cell, WKSContentListener::CellContent const &content, librevenge::RVNGPropertyList const &extras)
{
	if (!m_ps->m_isSheetRowOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::openSheetCell: called with m_isSheetRowOpened=false\n"));
		return;
	}
	if (m_ps->m_isSheetCellOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::openSheetCell: called with m_isSheetCellOpened=true\n"));
		closeSheetCell();
	}

	librevenge::RVNGPropertyList propList(extras);
	cell.addTo(propList);
	cell.getFont().addTo(propList);
	if (!cell.hasBasicFormat())
	{
		int numberingId=-1;
		std::stringstream name;
		if (m_ds->m_numberingIdMap.find(cell)!=m_ds->m_numberingIdMap.end())
		{
			numberingId=m_ds->m_numberingIdMap.find(cell)->second;
			name << "Numbering" << numberingId;
		}
		else
		{
			numberingId=(int) m_ds->m_numberingIdMap.size();
			name << "Numbering" << numberingId;

			librevenge::RVNGPropertyList numList;
			if (cell.getNumberingProperties(numList))
			{
				numList.insert("librevenge:name", name.str().c_str());
				m_documentInterface->defineSheetNumberingStyle(numList);
				m_ds->m_numberingIdMap[cell]=numberingId;
			}
			else
				numberingId=-1;
		}
		if (numberingId>=0)
			propList.insert("librevenge:numbering-name", name.str().c_str());
	}
	// formula
	if (content.m_formula.size())
	{
		librevenge::RVNGPropertyListVector formulaVect;
		for (size_t i=0; i < content.m_formula.size(); ++i)
			formulaVect.append(content.m_formula[i].getPropertyList());
		propList.insert("librevenge:formula", formulaVect);
	}
	bool hasFormula=!content.m_formula.empty();
	if (content.isValueSet() || hasFormula)
	{
		bool hasValue=content.isValueSet();
		if (hasFormula && (content.m_value >= 0 && content.m_value <= 0))
			hasValue=false;
		switch (cell.getFormat())
		{
		case WPSCellFormat::F_TEXT:
			if (!hasValue) break;
			propList.insert("librevenge:value-type", cell.getValueType().c_str());
			propList.insert("librevenge:value", content.m_value, librevenge::RVNG_GENERIC);
			break;
		case WPSCellFormat::F_NUMBER:
			propList.insert("librevenge:value-type", cell.getValueType().c_str());
			if (!hasValue) break;
			propList.insert("librevenge:value", content.m_value, librevenge::RVNG_GENERIC);
			break;
		case WPSCellFormat::F_BOOLEAN:
			propList.insert("librevenge:value-type", "boolean");
			if (!hasValue) break;
			propList.insert("librevenge:value", content.m_value, librevenge::RVNG_GENERIC);
			break;
		case WPSCellFormat::F_DATE:
		{
			propList.insert("librevenge:value-type", "date");
			if (!hasValue) break;
			int Y=0, M=0, D=0;
			if (!CellContent::double2Date(content.m_value, Y, M, D)) break;
			propList.insert("librevenge:year", Y);
			propList.insert("librevenge:month", M);
			propList.insert("librevenge:day", D);
			break;
		}
		case WPSCellFormat::F_TIME:
		{
			propList.insert("librevenge:value-type", "time");
			if (!hasValue) break;
			int H=0, M=0, S=0;
			if (!CellContent::double2Time(content.m_value,H,M,S))
				break;
			propList.insert("librevenge:hours", H);
			propList.insert("librevenge:minutes", M);
			propList.insert("librevenge:seconds", S);
			break;
		}
		case WPSCellFormat::F_UNKNOWN:
			if (!hasValue) break;
			propList.insert("librevenge:value-type", cell.getValueType().c_str());
			propList.insert("librevenge:value", content.m_value, librevenge::RVNG_GENERIC);
			break;
		default:
			break;
		}
	}

	m_ps->m_isSheetCellOpened = true;
	m_documentInterface->openSheetCell(propList);
}

void WKSContentListener::closeSheetCell()
{
	if (!m_ps->m_isSheetCellOpened)
	{
		WPS_DEBUG_MSG(("WKSContentListener::closeSheetCell: called with m_isSheetCellOpened=false\n"));
		return;
	}

	_closeParagraph();

	m_ps->m_isSheetCellOpened = false;
	m_documentInterface->closeSheetCell();
}

///////////////////
// page
///////////////////
void WKSContentListener::_openPageSpan()
{
	if (m_ps->m_isPageSpanOpened)
		return;

	if (!m_ds->m_isDocumentStarted)
		startDocument();

	if (m_ds->m_pageList.size()==0)
	{
		WPS_DEBUG_MSG(("WKSContentListener::_openPageSpan: can not find any page\n"));
		throw libwps::ParseException();
	}
	unsigned actPage = 0;
	std::vector<WPSPageSpan>::iterator it = m_ds->m_pageList.begin();
	while (actPage < m_ps->m_currentPage)
	{
		actPage+=(unsigned) it++->getPageSpan();
		if (it == m_ds->m_pageList.end())
		{
			WPS_DEBUG_MSG(("WKSContentListener::_openPageSpan: can not find current page\n"));
			throw libwps::ParseException();
		}
	}
	WPSPageSpan &currentPage = *it;

	librevenge::RVNGPropertyList propList;
	currentPage.getPageProperty(propList);
	propList.insert("librevenge:is-last-page-span", ((m_ps->m_currentPage + 1 == m_ds->m_pageList.size()) ? true : false));

	if (!m_ps->m_isPageSpanOpened)
		m_documentInterface->openPageSpan(propList);

	m_ps->m_isPageSpanOpened = true;

	m_ps->m_pageFormLength = currentPage.getFormLength();
	m_ps->m_pageFormWidth = currentPage.getFormWidth();
	m_ps->m_pageMarginLeft = currentPage.getMarginLeft();
	m_ps->m_pageMarginRight = currentPage.getMarginRight();
	m_ps->m_pageFormOrientationIsPortrait =
	    currentPage.getFormOrientation() == WPSPageSpan::PORTRAIT;
	m_ps->m_pageMarginTop = currentPage.getMarginTop();
	m_ps->m_pageMarginBottom = currentPage.getMarginBottom();

	// we insert the header footer
	currentPage.sendHeaderFooters(this, m_documentInterface);

	// first paragraph in span (necessary for resetting page number)
	m_ps->m_numPagesRemainingInSpan = (currentPage.getPageSpan() - 1);
	m_ps->m_currentPage++;
}

void WKSContentListener::_closePageSpan()
{
	if (!m_ps->m_isPageSpanOpened)
		return;

	if (m_ps->m_isParagraphOpened)
		_closeParagraph();

	m_documentInterface->closePageSpan();
	m_ps->m_isPageSpanOpened = false;
}

///////////////////
// others
///////////////////

// ---------- state stack ------------------
shared_ptr<WKSContentParsingState> WKSContentListener::_pushParsingState()
{
	shared_ptr<WKSContentParsingState> actual = m_ps;
	m_psStack.push_back(actual);
	m_ps.reset(new WKSContentParsingState);

	// BEGIN: copy page properties into the new parsing state
	m_ps->m_pageFormLength = actual->m_pageFormLength;
	m_ps->m_pageFormWidth = actual->m_pageFormWidth;
	m_ps->m_pageFormOrientationIsPortrait =	actual->m_pageFormOrientationIsPortrait;
	m_ps->m_pageMarginLeft = actual->m_pageMarginLeft;
	m_ps->m_pageMarginRight = actual->m_pageMarginRight;
	m_ps->m_pageMarginTop = actual->m_pageMarginTop;
	m_ps->m_pageMarginBottom = actual->m_pageMarginBottom;

	m_ps->m_isNote = actual->m_isNote;

	return actual;
}

void WKSContentListener::_popParsingState()
{
	if (m_psStack.size()==0)
	{
		WPS_DEBUG_MSG(("WKSContentListener::_popParsingState: psStack is empty()\n"));
		throw libwps::ParseException();
	}
	m_ps = m_psStack.back();
	m_psStack.pop_back();
}

// ---------- WKSContentListener::FormulaInstruction ------------------
librevenge::RVNGPropertyList WKSContentListener::FormulaInstruction::getPropertyList() const
{
	librevenge::RVNGPropertyList pList;
	switch (m_type)
	{
	case F_Operator:
		pList.insert("librevenge:type","librevenge-operator");
		pList.insert("librevenge:operator",m_content.c_str());
		break;
	case F_Function:
		pList.insert("librevenge:type","librevenge-function");
		pList.insert("librevenge:function",m_content.c_str());
		break;
	case F_Text:
		pList.insert("librevenge:type","librevenge-text");
		pList.insert("librevenge:text",m_content.c_str());
		break;
	case F_Double:
		pList.insert("librevenge:type","librevenge-number");
		pList.insert("librevenge:number",m_doubleValue, librevenge::RVNG_GENERIC);
		break;
	case F_Long:
		pList.insert("librevenge:type","librevenge-number");
		pList.insert("librevenge:number",m_longValue, librevenge::RVNG_GENERIC);
		break;
	case F_Cell:
		pList.insert("librevenge:type","librevenge-cell");
		pList.insert("librevenge:column",m_position[0][0], librevenge::RVNG_GENERIC);
		pList.insert("librevenge:row",m_position[0][1], librevenge::RVNG_GENERIC);
		pList.insert("librevenge:column-absolute",!m_positionRelative[0][0]);
		pList.insert("librevenge:row-absolute",!m_positionRelative[0][1]);
		if (!m_sheetName.empty())
			pList.insert("librevenge:sheet-name",m_sheetName.cstr());
		break;
	case F_CellList:
		pList.insert("librevenge:type","librevenge-cells");
		pList.insert("librevenge:start-column",m_position[0][0], librevenge::RVNG_GENERIC);
		pList.insert("librevenge:start-row",m_position[0][1], librevenge::RVNG_GENERIC);
		pList.insert("librevenge:start-column-absolute",!m_positionRelative[0][0]);
		pList.insert("librevenge:start-row-absolute",!m_positionRelative[0][1]);
		pList.insert("librevenge:end-column",m_position[1][0], librevenge::RVNG_GENERIC);
		pList.insert("librevenge:end-row",m_position[1][1], librevenge::RVNG_GENERIC);
		pList.insert("librevenge:end-column-absolute",!m_positionRelative[1][0]);
		pList.insert("librevenge:end-row-absolute",!m_positionRelative[1][1]);
		if (!m_sheetName.empty())
			pList.insert("librevenge:sheet-name",m_sheetName.cstr());
		break;
	default:
		WPS_DEBUG_MSG(("WKSContentListener::FormulaInstruction::getPropertyList: unexpected type\n"));
	}
	return pList;
}

std::ostream &operator<<(std::ostream &o, WKSContentListener::FormulaInstruction const &inst)
{
	if (inst.m_type==WKSContentListener::FormulaInstruction::F_Double)
		o << inst.m_doubleValue;
	else if (inst.m_type==WKSContentListener::FormulaInstruction::F_Long)
		o << inst.m_longValue;
	else if (inst.m_type==WKSContentListener::FormulaInstruction::F_Cell)
	{
		if (!inst.m_positionRelative[0][0]) o << "$";
		if (inst.m_position[0][0]>=26) o << (char)(inst.m_position[0][0]/26-1 + 'A');
		o << (char)(inst.m_position[0][0]%26+'A');
		if (!inst.m_positionRelative[0][1]) o << "$";
		o << inst.m_position[0][1];
		if (!inst.m_sheetName.empty())
			o << "[" << inst.m_sheetName.cstr() << "]";
	}
	else if (inst.m_type==WKSContentListener::FormulaInstruction::F_CellList)
	{
		for (int l=0; l<2; ++l)
		{
			if (!inst.m_positionRelative[l][0]) o << "$";
			if (inst.m_position[l][0]>=26) o << (char)(inst.m_position[l][0]/26-1 + 'A');
			o << (char)(inst.m_position[l][0]%26+'A');
			if (!inst.m_positionRelative[l][1]) o << "$";
			o << inst.m_position[l][1];
			if (l==0) o << ":";
		}
		if (!inst.m_sheetName.empty())
			o << "[" << inst.m_sheetName.cstr() << "]";
	}
	else if (inst.m_type==WKSContentListener::FormulaInstruction::F_Text)
		o << "\"" << inst.m_content << "\"";
	else
		o << inst.m_content;
	return o;
}

// ---------- WKSContentListener::CellContent ------------------
bool WKSContentListener::CellContent::double2Date(double val, int &Y, int &M, int &D)
{
	// number of day since 1/1/1970
	time_t date= time_t((val-24107-1462+0.4)*24.*3600);
	struct tm dateTm;
	if (!gmtime_r(&date,&dateTm)) return false;

	Y = dateTm.tm_year+1900;
	M=dateTm.tm_mon+1;
	D=dateTm.tm_mday;
	return true;
}

bool WKSContentListener::CellContent::double2Time(double val, int &H, int &M, int &S)
{
	if (val < 0.0 || val > 1.0) return false;
	double time = 24.*3600.*val+0.5;
	H=int(time/3600.);
	time -= H*3600.;
	M=int(time/60.);
	time -= M*60.;
	S=int(time);
	return true;
}

std::ostream &operator<<(std::ostream &o, WKSContentListener::CellContent const &cell)
{

	switch (cell.m_contentType)
	{
	case WKSContentListener::CellContent::C_NONE:
		break;
	case WKSContentListener::CellContent::C_TEXT:
		o << ",text=\"" << cell.m_textEntry << "\"";
		break;
	case WKSContentListener::CellContent::C_NUMBER:
	{
		o << ",val=";
		bool textAndVal = false;
		if (cell.hasText())
		{
			o << "entry=" << cell.m_textEntry;
			textAndVal = cell.isValueSet();
		}
		if (textAndVal) o << "[";
		if (cell.isValueSet()) o << cell.m_value;
		if (textAndVal) o << "]";
	}
	break;
	case WKSContentListener::CellContent::C_FORMULA:
		o << ",formula=";
		for (size_t l=0; l < cell.m_formula.size(); ++l)
			o << cell.m_formula[l];
		if (cell.isValueSet()) o << "[" << cell.m_value << "]";
		break;
	case WKSContentListener::CellContent::C_UNKNOWN:
		break;
	default:
		o << "###unknown type,";
		break;
	}
	return o;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
