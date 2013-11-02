/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2002-2004 Marc Maurer (uwog@uwog.net)
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

#include <stdio.h>
#include <stdarg.h>
#include "RawDocumentGenerator.h"

#ifdef _U
#undef _U
#endif

#define _U(M, L) \
	m_atLeastOneCallback = true; \
	if (!m_printCallgraphScore) \
			__iuprintf M; \
	else \
		m_callStack.push(L);

#ifdef _D
#undef _D
#endif

#define _D(M, L) \
	m_atLeastOneCallback = true; \
	if (!m_printCallgraphScore) \
			__idprintf M; \
	else \
	{ \
		ListenerCallback lc = m_callStack.top(); \
		if (lc != L) \
			m_callbackMisses++; \
		m_callStack.pop(); \
	}

RawDocumentGenerator::RawDocumentGenerator(bool printCallgraphScore) :
	m_indent(0),
	m_callbackMisses(0),
	m_atLeastOneCallback(false),
	m_printCallgraphScore(printCallgraphScore),
	m_callStack()
{
}

RawDocumentGenerator::~RawDocumentGenerator()
{
	if (m_printCallgraphScore)
		printf("%d\n", m_atLeastOneCallback ? (int)(m_callStack.size()) + m_callbackMisses : -1);
}

void RawDocumentGenerator::__iprintf(const char *format, ...)
{
	m_atLeastOneCallback = true;
	if (m_printCallgraphScore) return;

	va_list args;
	va_start(args, format);
	for (int i=0; i<m_indent; i++)
		printf("  ");
	vprintf(format, args);
	va_end(args);
}

void RawDocumentGenerator::__iuprintf(const char *format, ...)
{
	m_atLeastOneCallback = true;
	va_list args;
	va_start(args, format);
	for (int i=0; i<m_indent; i++)
		printf("  ");
	vprintf(format, args);
	__indentUp();
	va_end(args);
}

void RawDocumentGenerator::__idprintf(const char *format, ...)
{
	m_atLeastOneCallback = true;
	va_list args;
	va_start(args, format);
	__indentDown();
	for (int i=0; i<m_indent; i++)
		printf("  ");
	vprintf(format, args);
	va_end(args);
}

RVNGString getPropString(const RVNGPropertyList &propList)
{
	RVNGString propString;
	RVNGPropertyList::Iter i(propList);
	if (!i.last())
	{
		propString.append(i.key());
		propString.append(": ");
		propString.append(i()->getStr().cstr());
		for (; i.next(); )
		{
			propString.append(", ");
			propString.append(i.key());
			propString.append(": ");
			propString.append(i()->getStr().cstr());
		}
	}

	return propString;
}

RVNGString getPropString(const RVNGPropertyListVector &itemList)
{
	RVNGString propString;

	propString.append("(");
	RVNGPropertyListVector::Iter i(itemList);

	if (!i.last())
	{
		propString.append("(");
		propString.append(getPropString(i()));
		propString.append(")");

		for (; i.next();)
		{
			propString.append(", (");
			propString.append(getPropString(i()));
			propString.append(")");
		}

	}
	propString.append(")");

	return propString;
}

void RawDocumentGenerator::setDocumentMetaData(const RVNGPropertyList &propList)
{
	if (m_printCallgraphScore)
		return;

	__iprintf("setDocumentMetaData(%s)\n", getPropString(propList).cstr());
}

void RawDocumentGenerator::startDocument()
{
	_U(("startDocument()\n"), LC_START_DOCUMENT);
}

void RawDocumentGenerator::endDocument()
{
	_D(("endDocument()\n"), LC_START_DOCUMENT);
}

void RawDocumentGenerator::definePageStyle(const RVNGPropertyList &propList)
{
	__iprintf("definePageStyle(%s)\n", getPropString(propList).cstr());
}

void RawDocumentGenerator::openPageSpan(const RVNGPropertyList &propList)
{
	_U(("openPageSpan(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_PAGE_SPAN);
}

void RawDocumentGenerator::closePageSpan()
{
	_D(("closePageSpan()\n"),
	   LC_OPEN_PAGE_SPAN);
}

void RawDocumentGenerator::openHeader(const RVNGPropertyList &propList)
{
	_U(("openHeader(%s)\n",
	    getPropString(propList).cstr()),
	   LC_OPEN_HEADER_FOOTER);
}

void RawDocumentGenerator::closeHeader()
{
	_D(("closeHeader()\n"),
	   LC_OPEN_HEADER_FOOTER);
}

void RawDocumentGenerator::openFooter(const RVNGPropertyList &propList)
{
	_U(("openFooter(%s)\n",
	    getPropString(propList).cstr()),
	   LC_OPEN_HEADER_FOOTER);
}

void RawDocumentGenerator::closeFooter()
{
	_D(("closeFooter()\n"),
	   LC_OPEN_HEADER_FOOTER);
}

void RawDocumentGenerator::defineParagraphStyle(const RVNGPropertyList &propList, const RVNGPropertyListVector &tabStops)
{
	__iprintf("defineParagraphStyle(%s, tab-stops: %s)\n", getPropString(propList).cstr(), getPropString(tabStops).cstr());
}

void RawDocumentGenerator::openParagraph(const RVNGPropertyList &propList, const RVNGPropertyListVector &tabStops)
{
	_U(("openParagraph(%s, tab-stops: %s)\n", getPropString(propList).cstr(), getPropString(tabStops).cstr()),
	   LC_OPEN_PARAGRAPH);
}

void RawDocumentGenerator::closeParagraph()
{
	_D(("closeParagraph()\n"), LC_OPEN_PARAGRAPH);
}

void RawDocumentGenerator::defineCharacterStyle(const RVNGPropertyList &propList)
{
	__iprintf("defineCharacterStyle(%s)\n", getPropString(propList).cstr());
}

void RawDocumentGenerator::openSpan(const RVNGPropertyList &propList)
{
	_U(("openSpan(%s)\n", getPropString(propList).cstr()), LC_OPEN_SPAN);
}

void RawDocumentGenerator::closeSpan()
{
	_D(("closeSpan()\n"), LC_OPEN_SPAN);
}

void RawDocumentGenerator::defineSectionStyle(const RVNGPropertyList &propList, const RVNGPropertyListVector &columns)
{
	__iprintf("defineSectionStyle(%s, columns: %s)\n", getPropString(propList).cstr(), getPropString(columns).cstr());
}

void RawDocumentGenerator::openSection(const RVNGPropertyList &propList, const RVNGPropertyListVector &columns)
{
	_U(("openSection(%s, columns: %s)\n", getPropString(propList).cstr(), getPropString(columns).cstr()), LC_OPEN_SECTION);
}

void RawDocumentGenerator::closeSection()
{
	_D(("closeSection()\n"), LC_OPEN_SECTION);
}

void RawDocumentGenerator::insertTab()
{
	__iprintf("insertTab()\n");
}

void RawDocumentGenerator::insertSpace()
{
	__iprintf("insertSpace()\n");
}

void RawDocumentGenerator::insertText(const RVNGString &text)
{
	__iprintf("insertText(text: %s)\n", text.cstr());
}

void RawDocumentGenerator::insertLineBreak()
{
	__iprintf("insertLineBreak()\n");
}

void RawDocumentGenerator::insertField(const RVNGString &type, const RVNGPropertyList &propList)
{
	__iprintf("insertField(type: %s, %s)\n", type.cstr(), getPropString(propList).cstr());
}

void RawDocumentGenerator::defineOrderedListLevel(const RVNGPropertyList &propList)
{
	__iprintf("defineOrderedListLevel(%s)\n", getPropString(propList).cstr());
}

void RawDocumentGenerator::defineUnorderedListLevel(const RVNGPropertyList &propList)
{
	__iprintf("defineUnorderedListLevel(%s)\n", getPropString(propList).cstr());
}

void RawDocumentGenerator::openOrderedListLevel(const RVNGPropertyList &propList)
{
	_U(("openOrderedListLevel(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_ORDERED_LIST_LEVEL);
}

void RawDocumentGenerator::openUnorderedListLevel(const RVNGPropertyList &propList)
{
	_U(("openUnorderedListLevel(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_UNORDERED_LIST_LEVEL);
}

void RawDocumentGenerator::closeOrderedListLevel()
{
	_D(("closeOrderedListLevel()\n"),
	   LC_OPEN_ORDERED_LIST_LEVEL);
}

void RawDocumentGenerator::closeUnorderedListLevel()
{
	_D(("closeUnorderedListLevel()\n"), LC_OPEN_UNORDERED_LIST_LEVEL);
}

void RawDocumentGenerator::openListElement(const RVNGPropertyList &propList, const RVNGPropertyListVector &tabStops)
{
	_U(("openListElement(%s, tab-stops: %s)\n", getPropString(propList).cstr(), getPropString(tabStops).cstr()),
	   LC_OPEN_LIST_ELEMENT);
}

void RawDocumentGenerator::closeListElement()
{
	_D(("closeListElement()\n"), LC_OPEN_LIST_ELEMENT);
}

void RawDocumentGenerator::openFootnote(const RVNGPropertyList &propList)
{
	_U(("openFootnote(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_FOOTNOTE);
}

void RawDocumentGenerator::closeFootnote()
{
	_D(("closeFootnote()\n"), LC_OPEN_FOOTNOTE);
}

void RawDocumentGenerator::openEndnote(const RVNGPropertyList &propList)
{
	_U(("openEndnote(number: %s)\n", getPropString(propList).cstr()),
	   LC_OPEN_ENDNOTE);
}

void RawDocumentGenerator::closeEndnote()
{
	_D(("closeEndnote()\n"), LC_OPEN_ENDNOTE);
}

void RawDocumentGenerator::openComment(const RVNGPropertyList &propList)
{
	_U(("openComment(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_COMMENT);
}

void RawDocumentGenerator::closeComment()
{
	_D(("closeComment()\n"), LC_OPEN_COMMENT);
}

void RawDocumentGenerator::openTextBox(const RVNGPropertyList &propList)
{
	_U(("openTextBox(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_TEXT_BOX);
}

void RawDocumentGenerator::closeTextBox()
{
	_D(("closeTextBox()\n"), LC_OPEN_TEXT_BOX);
}

void RawDocumentGenerator::openTable(const RVNGPropertyList &propList, const RVNGPropertyListVector &columns)
{
	_U(("openTable(%s, columns: %s)\n", getPropString(propList).cstr(), getPropString(columns).cstr()), LC_OPEN_TABLE);
}

void RawDocumentGenerator::openTableRow(const RVNGPropertyList &propList)
{
	_U(("openTableRow(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_TABLE_ROW);
}

void RawDocumentGenerator::closeTableRow()
{
	_D(("closeTableRow()\n"), LC_OPEN_TABLE_ROW);
}

void RawDocumentGenerator::openTableCell(const RVNGPropertyList &propList)
{
	_U(("openTableCell(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_TABLE_CELL);
}

void RawDocumentGenerator::closeTableCell()
{
	_D(("closeTableCell()\n"), LC_OPEN_TABLE_CELL);
}

void RawDocumentGenerator::insertCoveredTableCell(const RVNGPropertyList &propList)
{
	__iprintf("insertCoveredTableCell(%s)\n", getPropString(propList).cstr());
}

void RawDocumentGenerator::closeTable()
{
	_D(("closeTable()\n"), LC_OPEN_TABLE);
}

void RawDocumentGenerator::openFrame(const RVNGPropertyList &propList)
{
	_U(("openFrame(%s)\n", getPropString(propList).cstr()),
	   LC_OPEN_FRAME);
}

void RawDocumentGenerator::closeFrame()
{
	_D(("closeFrame()\n"), LC_OPEN_FRAME);
}

void RawDocumentGenerator::insertBinaryObject(const RVNGPropertyList &propList, const RVNGBinaryData & /* data */)
{
	__iprintf("insertBinaryObject(%s)\n", getPropString(propList).cstr());
}

void RawDocumentGenerator::insertEquation(const RVNGPropertyList &propList, const RVNGString &data)
{
	__iprintf("insertEquation(%s, text: %s)\n", getPropString(propList).cstr(), data.cstr());
}
