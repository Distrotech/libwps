/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2005 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
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

#ifndef HTML_DOCUMENT_GENERATOR_H
#define HTML_DOCUMENT_GENERATOR_H

#include <ostream>
#include <sstream>
#include <librevenge/librevenge.h>

#include "wps2html_internal.h"

namespace HtmlDocumentGeneratorInternal
{
struct State;
}

class HtmlDocumentGenerator : public librevenge::RVNGTextInterface
{
public:
	HtmlDocumentGenerator(char const *fName=0);
	virtual ~HtmlDocumentGenerator();

	virtual void setDocumentMetaData(const librevenge::RVNGPropertyList &propList);

	virtual void startDocument();
	virtual void endDocument();

	virtual void definePageStyle(const librevenge::RVNGPropertyList &) {}
	virtual void openPageSpan(const librevenge::RVNGPropertyList &propList);
	virtual void closePageSpan();
	virtual void openHeader(const librevenge::RVNGPropertyList &propList);
	virtual void closeHeader();
	virtual void openFooter(const librevenge::RVNGPropertyList &propList);
	virtual void closeFooter();

	virtual void defineSectionStyle(const librevenge::RVNGPropertyList &, const librevenge::RVNGPropertyListVector &) {}
	virtual void openSection(const librevenge::RVNGPropertyList & /* propList */, const librevenge::RVNGPropertyListVector & /* columns */) {}
	virtual void closeSection() {}

	virtual void defineParagraphStyle(const librevenge::RVNGPropertyList &, const librevenge::RVNGPropertyListVector &) {}
	virtual void openParagraph(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &tabStops);
	virtual void closeParagraph();

	virtual void defineCharacterStyle(const librevenge::RVNGPropertyList &) {}
	virtual void openSpan(const librevenge::RVNGPropertyList &propList);
	virtual void closeSpan();

	virtual void insertTab();
	virtual void insertText(const librevenge::RVNGString &text);
	virtual void insertSpace();
	virtual void insertLineBreak();
	virtual void insertField(const librevenge::RVNGString &/*type*/, const librevenge::RVNGPropertyList &/*propList*/) {}

	virtual void defineOrderedListLevel(const librevenge::RVNGPropertyList &propList);
	virtual void defineUnorderedListLevel(const librevenge::RVNGPropertyList &propList);
	virtual void openOrderedListLevel(const librevenge::RVNGPropertyList &propList);
	virtual void openUnorderedListLevel(const librevenge::RVNGPropertyList &propList);
	virtual void closeOrderedListLevel();
	virtual void closeUnorderedListLevel();
	virtual void openListElement(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &tabStops);
	virtual void closeListElement();

	virtual void openFootnote(const librevenge::RVNGPropertyList &propList);
	virtual void closeFootnote();
	virtual void openEndnote(const librevenge::RVNGPropertyList &propList);
	virtual void closeEndnote();
	virtual void openComment(const librevenge::RVNGPropertyList &propList);
	virtual void closeComment();
	virtual void openTextBox(const librevenge::RVNGPropertyList &propList);
	virtual void closeTextBox();

	virtual void openTable(const librevenge::RVNGPropertyList &propList, const librevenge::RVNGPropertyListVector &columns);
	virtual void openTableRow(const librevenge::RVNGPropertyList &propList);
	virtual void closeTableRow();
	virtual void openTableCell(const librevenge::RVNGPropertyList &propList);
	virtual void closeTableCell();
	virtual void insertCoveredTableCell(const librevenge::RVNGPropertyList & /* propList */) {}
	virtual void closeTable();

	virtual void openFrame(const librevenge::RVNGPropertyList & /* propList */) {}
	virtual void closeFrame() {}

	virtual void insertBinaryObject(const librevenge::RVNGPropertyList & /* propList */, const librevenge::RVNGBinaryData & /* data */) {}
	virtual void insertEquation(const librevenge::RVNGPropertyList & /* propList */, const librevenge::RVNGString & /* data */) {}

private:
	shared_ptr<HtmlDocumentGeneratorInternal::State> m_state;

	shared_ptr<std::ostream> m_output;
	// Unimplemented to prevent compiler from creating crasher ones
	HtmlDocumentGenerator(const HtmlDocumentGenerator &);
	HtmlDocumentGenerator &operator=(const HtmlDocumentGenerator &);
};

#endif /* HTMLLISTENERIMPL_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
