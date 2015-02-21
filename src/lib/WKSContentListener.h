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

#ifndef WKSCONTENTLISTENER_H
#define WKSCONTENTLISTENER_H

#include <vector>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"

#include "WPSEntry.h"
#include "WPSGraphicStyle.h"

#include "WPSListener.h"

class WPSCellFormat;
class WPSGraphicShape;
class WPSGraphicStyle;
class WPSList;
class WPSPageSpan;
struct WPSParagraph;
struct WPSTabStop;

struct WKSContentParsingState;
struct WKSDocumentParsingState;

class WKSContentListener : public WPSListener
{
public:
	//! small class use to define a formula instruction
	struct FormulaInstruction
	{
		enum What { F_Operator, F_Function, F_Cell, F_CellList, F_Long, F_Double, F_Text };
		//! constructor
		FormulaInstruction() : m_type(F_Text), m_content(""), m_longValue(0), m_doubleValue(0), m_sheetName("")
		{
			for (int i=0; i<2; ++i)
			{
				m_position[i]=Vec2i(0,0);
				m_positionRelative[i]=Vec2b(false,false);
			}
		}
		//! return a proplist corresponding to a instruction
		librevenge::RVNGPropertyList getPropertyList() const;
		//! operator<<
		friend std::ostream &operator<<(std::ostream &o, FormulaInstruction const &inst);
		//! the type
		What m_type;
		//! the content ( if type == F_Operator or type = F_Function or type==F_Text)
		std::string m_content;
		//! value ( if type==F_Long )
		double m_longValue;
		//! value ( if type==F_Double )
		double m_doubleValue;
		//! cell position ( if type==F_Cell or F_CellList )
		Vec2i m_position[2];
		//! relative cell position ( if type==F_Cell or F_CellList )
		Vec2b m_positionRelative[2];
		//! the sheet name
		librevenge::RVNGString m_sheetName;
	};
	//! small class use to define a sheet cell content
	struct CellContent
	{
		/** the different types of cell's field */
		enum ContentType { C_NONE, C_TEXT, C_NUMBER, C_FORMULA, C_UNKNOWN };
		/// constructor
		CellContent() : m_contentType(C_UNKNOWN), m_value(0.0), m_valueSet(false), m_textEntry(), m_formula() { }
		/// destructor
		~CellContent() {}
		//! operator<<
		friend std::ostream &operator<<(std::ostream &o, CellContent const &cell);

		//! returns true if the cell has no content
		bool empty() const
		{
			if (m_contentType == C_NUMBER) return false;
			if (m_contentType == C_TEXT && !m_textEntry.valid()) return false;
			if (m_contentType == C_FORMULA && (m_formula.size() || isValueSet())) return false;
			return true;
		}
		//! sets the double value
		void setValue(double value)
		{
			m_value = value;
			m_valueSet = true;
		}
		//! returns true if the value has been setted
		bool isValueSet() const
		{
			return m_valueSet;
		}
		//! returns true if the text is set
		bool hasText() const
		{
			return m_textEntry.valid();
		}
		/** conversion beetween double days since 1900 and date */
		static bool double2Date(double val, int &Y, int &M, int &D);
		/** conversion beetween double: second since 0:00 and time */
		static bool double2Time(double val, int &H, int &M, int &S);

		//! the content type ( by default unknown )
		ContentType m_contentType;
		//! the cell value
		double m_value;
		//! true if the value has been set
		bool m_valueSet;
		//! the cell string
		WPSEntry m_textEntry;
		//! the formula list of instruction
		std::vector<FormulaInstruction> m_formula;
	};

	WKSContentListener(std::vector<WPSPageSpan> const &pageList, librevenge::RVNGSpreadsheetInterface *documentInterface);
	virtual ~WKSContentListener();

	void setDocumentLanguage(int lcid);

	void startDocument();
	void endDocument();
	void handleSubDocument(WPSSubDocumentPtr &subDocument, libwps::SubDocumentType subDocumentType);

	// ------ text data -----------

	//! adds a basic character, ..
	void insertCharacter(uint8_t character);
	/** adds an unicode character
	 *
	 * by convention if \a character=0xfffd(undef), no character is added */
	void insertUnicode(uint32_t character);
	//! adds a unicode string
	void insertUnicodeString(librevenge::RVNGString const &str);

	void insertTab();
	void insertEOL(bool softBreak=false);
	void insertBreak(const uint8_t breakType);

	// ------ text format -----------
	//! set the actual font
	void setFont(const WPSFont &font);
	//! returns the actual font
	WPSFont const &getFont() const;

	// ------ paragraph format -----------
	//! returns true if a paragraph or a list is opened
	bool isParagraphOpened() const;
	//! sets the actual paragraph
	void setParagraph(const WPSParagraph &para);
	//! returns the actual paragraph
	WPSParagraph const &getParagraph() const;

	// ------- fields ----------------
	//! adds a field type
	void insertField(FieldType type);
	//! insert a date/time field with given format (see strftime)
	void insertDateTimeField(char const *format);

	// ------- subdocument -----------------
	/** adds comment */
	void insertComment(WPSSubDocumentPtr &subDocument);
	/** adds a picture in given position */
	void insertPicture(WPSPosition const &pos, const librevenge::RVNGBinaryData &binaryData,
	                   std::string type="image/pict", WPSGraphicStyle const &style=WPSGraphicStyle::emptyStyle());
	/** adds a picture in given position */
	void insertPicture(WPSPosition const &pos, WPSGraphicShape const &shape, WPSGraphicStyle const &style);
	/** adds a textbox in given position */
	void insertTextBox(WPSPosition const &pos, WPSSubDocumentPtr subDocument,
					   WPSGraphicStyle const &frameStyle=WPSGraphicStyle::emptyStyle());

	// ------- sheet -----------------
	/** open a sheet*/
	void openSheet(std::vector<float> const &colWidth, librevenge::RVNGUnit unit, librevenge::RVNGString const &name="");
	/** closes this sheet */
	void closeSheet();
	/** open a row with given height. If h<0, use min-row-heigth */
	void openSheetRow(float h, librevenge::RVNGUnit unit, bool headerRow=false);
	/** closes this row */
	void closeSheetRow();
	/** low level function to define a cell.
		\param cell the cell position, alignement, ...
		\param content the cell content
		\param extras to be used to pass extra data, for instance spreadsheet data*/
	void openSheetCell(WPSCell const &cell, CellContent const &content, librevenge::RVNGPropertyList const &extras=librevenge::RVNGPropertyList());
	/** close a cell */
	void closeSheetCell();

protected:
	void _openPageSpan();
	void _closePageSpan();

	void _handleFrameParameters(librevenge::RVNGPropertyList &propList, WPSPosition const &pos);
	bool _openFrame(WPSPosition const &pos, WPSGraphicStyle const &style);
	void _closeFrame();

	void _startSubDocument();
	void _endSubDocument();

	void _openParagraph();
	void _closeParagraph();
	void _appendParagraphProperties(librevenge::RVNGPropertyList &propList, const bool isListElement=false);
	void _resetParagraphState(const bool isListElement=false);

	void _openSpan();
	void _closeSpan();

	void _flushText();
	void _flushDeferredTabs();

	void _insertBreakIfNecessary(librevenge::RVNGPropertyList &propList);

	/** creates a new parsing state (copy of the actual state)
	 *
	 * \return the old one */
	shared_ptr<WKSContentParsingState> _pushParsingState();
	//! resets the previous parsing state
	void _popParsingState();

protected:
	shared_ptr<WKSDocumentParsingState> m_ds; // main parse state
	shared_ptr<WKSContentParsingState> m_ps; // parse state
	std::vector<shared_ptr<WKSContentParsingState> > m_psStack;
	librevenge::RVNGSpreadsheetInterface *m_documentInterface;

private:
	WKSContentListener(const WKSContentListener &);
	WKSContentListener &operator=(const WKSContentListener &);
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
