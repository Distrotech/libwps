/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2002-2003 Marc Maurer (uwog@uwog.net)
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

#ifndef TEXTLISTENERIMPL_H
#define TEXTLISTENERIMPL_H

#include <librevenge/librevenge.h>

class TextDocumentGenerator : public RVNGTextInterface
{
public:
	TextDocumentGenerator(const bool isInfo=false);
	virtual ~TextDocumentGenerator();

	virtual void setDocumentMetaData(const RVNGPropertyList &propList);

	virtual void startDocument() {}
	virtual void endDocument() {}

	virtual void definePageStyle(const RVNGPropertyList &) {}
	virtual void openPageSpan(const RVNGPropertyList & /* propList */) {}
	virtual void closePageSpan() {}
	virtual void openHeader(const RVNGPropertyList & /* propList */) {}
	virtual void closeHeader() {}
	virtual void openFooter(const RVNGPropertyList & /* propList */) {}
	virtual void closeFooter() {}

	virtual void defineSectionStyle(const RVNGPropertyList &, const RVNGPropertyListVector &) {}
	virtual void openSection(const RVNGPropertyList & /* propList */, const RVNGPropertyListVector & /* columns */) {}
	virtual void closeSection() {}

	virtual void defineParagraphStyle(const RVNGPropertyList &, const RVNGPropertyListVector &) {}
	virtual void openParagraph(const RVNGPropertyList & /* propList */, const RVNGPropertyListVector & /* tabStops */) {}
	virtual void closeParagraph();

	virtual void defineCharacterStyle(const RVNGPropertyList &) {}
	virtual void openSpan(const RVNGPropertyList & /* propList */) {}
	virtual void closeSpan() {}

	virtual void insertTab();
	virtual void insertText(const RVNGString &text);
	virtual void insertSpace();
	virtual void insertLineBreak();
	virtual void insertField(const RVNGString &/*type*/, const RVNGPropertyList &/*propList*/) {}

	virtual void defineOrderedListLevel(const RVNGPropertyList & /* propList */) {}
	virtual void defineUnorderedListLevel(const RVNGPropertyList & /* propList */) {}
	virtual void openOrderedListLevel(const RVNGPropertyList & /* propList */) {}
	virtual void openUnorderedListLevel(const RVNGPropertyList & /* propList */) {}
	virtual void closeOrderedListLevel() {}
	virtual void closeUnorderedListLevel() {}
	virtual void openListElement(const RVNGPropertyList & /* propList */, const RVNGPropertyListVector & /* tabStops */) {}
	virtual void closeListElement() {}

	virtual void openFootnote(const RVNGPropertyList & /* propList */) {}
	virtual void closeFootnote() {}
	virtual void openEndnote(const RVNGPropertyList & /* propList */) {}
	virtual void closeEndnote() {}
	virtual void openComment(const RVNGPropertyList & /* propList */) {}
	virtual void closeComment() {}
	virtual void openTextBox(const RVNGPropertyList & /* propList */) {}
	virtual void closeTextBox() {}

	virtual void openTable(const RVNGPropertyList & /* propList */, const RVNGPropertyListVector & /* columns */) {}
	virtual void openTableRow(const RVNGPropertyList & /* propList */) {}
	virtual void closeTableRow() {}
	virtual void openTableCell(const RVNGPropertyList & /* propList */) {}
	virtual void closeTableCell() {}
	virtual void insertCoveredTableCell(const RVNGPropertyList & /* propList */) {}
	virtual void closeTable() {}

	virtual void openFrame(const RVNGPropertyList & /* propList */) {}
	virtual void closeFrame() {}

	virtual void insertBinaryObject(const RVNGPropertyList & /* propList */, const RVNGBinaryData & /* object */) {}
	virtual void insertEquation(const RVNGPropertyList & /* propList */, const RVNGString & /* data */) {}

private:
	bool m_isInfo;
};

#endif /* TEXTLISTENERIMPL_H */
