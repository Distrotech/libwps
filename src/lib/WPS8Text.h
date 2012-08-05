/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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
 */

#ifndef WPS8_TEXT_H
#define WPS8_TEXT_H

#include <vector>

#include "libwps_internal.h"

#include "WPS.h"
#include "WPSDebug.h"
#include "WPSEntry.h"

class WPS8Parser;
class WPSPosition;

typedef class WPSContentListener WPS8ContentListener;
typedef shared_ptr<WPS8ContentListener> WPS8ContentListenerPtr;

namespace WPS8Struct
{
struct FileData;
}

namespace WPS8TextInternal
{
class SubDocument;
}

class WPS8Text
{
	friend class WPS8TextInternal::SubDocument;
	friend class WPS8Parser;
public:
	WPS8Text(WPS8Parser &parser);
	~WPS8Text();

	//! sets the listener
	void setListener(WPS8ContentListenerPtr &listen)
	{
		m_listener = listen;
	}

	void parse(WPXDocumentInterface *documentInterface);
protected:
	struct Note;
	struct Stream;
private:
	void readFontsTable(WPXInputStreamPtr &input);
	void readStreams(WPXInputStreamPtr &input);
	void readNotes(std::vector<Note> &dest, WPXInputStreamPtr &input, const char *key);
	void appendUTF16LE(WPXInputStreamPtr &input);
	void readTextRange(WPXInputStreamPtr &input, uint32_t startpos, uint32_t endpos, uint16_t stream);
	bool readFODPage(WPXInputStreamPtr &input, std::vector<WPSFOD> &FODs, uint16_t page_size);
	void parsePages(std::vector<WPSPageSpan> &pageList, WPXInputStreamPtr &input);
	void parse(WPXInputStreamPtr &stream);
	void propertyChangeDelta(uint32_t newTextAttributeBits);
	void propertyChange(WPS8Struct::FileData const &rgchProp, uint16_t &specialCode, int &fieldType);
	void propertyChangePara(WPS8Struct::FileData const &rgchProp);
	// interface with subdocument
	void sendNote(WPXInputStreamPtr &input, int noteId, bool is_endnote);

protected:
	//! returns the debug file
	libwps::DebugFile &ascii()
	{
		return m_asciiFile;
	}
	//! the main input
	WPXInputStreamPtr m_input;

	//! the main parser
	WPS8Parser &m_mainParser;

	//! the listener
	WPS8ContentListenerPtr m_listener;

	uint32_t m_offset_eot; /* stream offset to end of text */
	uint32_t m_oldTextAttributeBits;
	std::vector<WPSFOD> m_CHFODs; /* CHaracter FOrmatting Descriptors */
	std::vector<WPSFOD> m_PAFODs; /* PAragraph FOrmatting Descriptors */
	std::vector<std::string> m_fontNames;
	std::vector<Stream> m_streams;
	std::vector<Note> m_footnotes;
	int m_actualFootnote;
	std::vector<Note> m_endnotes;
	int m_actualEndnote;

	//! the ascii file
	libwps::DebugFile &m_asciiFile;
protected:
	struct Note : public WPSEntry
	{
		Note() : WPSEntry(), m_textOffset(0) {}
		uint32_t m_textOffset;
	};

	struct Stream : public WPSEntry
	{
		Stream() : WPSEntry(), m_type(Z_Dummy) {}

		enum Type {Z_Dummy=0, Z_Body=1, Z_Footnotes=2, Z_Endnotes = 3}  m_type;
	};
};


#endif /* WPS8_TEXT_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
