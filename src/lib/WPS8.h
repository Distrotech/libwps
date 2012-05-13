/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwpd
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

#ifndef WPS8_H
#define WPS8_H

#include <vector>
#include <map>
#include <libwpd/WPXString.h>

#include "libwps_internal.h"
#include "WPS.h"
#include "WPSContentListener.h"
#include <libwpd-stream/WPXStream.h>
#include "WPSParser.h"

typedef WPSContentListener WPS8ContentListener;

class WPS8Parser : public WPSParser
{
public:
	WPS8Parser(WPXInputStreamPtr &input, WPSHeaderPtr &header);
	~WPS8Parser();

	void parse(WPXDocumentInterface *documentInterface);
protected:
	struct Zone;
	typedef std::multimap<std::string, Zone> IndexMultiMap; /* string is name */
	struct Note;
	struct Stream;
private:
	void readFontsTable(WPXInputStreamPtr &input);
	void readStreams(WPXInputStreamPtr &input);
	void readNotes(std::vector<Note> &dest, WPXInputStreamPtr &input, const char *key);
	void appendUTF16LE(WPXInputStreamPtr &input);
	void readTextRange(WPXInputStreamPtr &input, uint32_t startpos, uint32_t endpos, uint16_t stream);
	void readNote(WPXInputStreamPtr &input, bool is_endnote);
	bool readFODPage(WPXInputStreamPtr &input, std::vector<WPSFOD> &FODs, uint16_t page_size);
	void parseHeaderIndexEntry(WPXInputStreamPtr &input);
	void parseHeaderIndex(WPXInputStreamPtr &input);
	void parsePages(std::vector<WPSPageSpan> &pageList, WPXInputStreamPtr &input);
	void parse(WPXInputStreamPtr &stream);
	void propertyChangeDelta(uint32_t newTextAttributeBits);
	void propertyChange(std::string rgchProp, uint16_t &specialCode);
	void propertyChangePara(std::string rgchProp);
	/// the listener
	shared_ptr<WPS8ContentListener> m_listener;
	uint32_t m_offset_eot; /* stream offset to end of text */
	uint32_t m_oldTextAttributeBits;
	IndexMultiMap m_headerIndexTable;
	std::vector<WPSFOD> m_CHFODs; /* CHaracter FOrmatting Descriptors */
	std::vector<WPSFOD> m_PAFODs; /* PAragraph FOrmatting Descriptors */
	std::vector<std::string> m_fontNames;
	std::vector<Stream> m_streams;
	std::vector<Note> m_footnotes;
	int m_actualFootnote;
	std::vector<Note> m_endnotes;
	int m_actualEndnote;

protected:
	/** Starting near beginning of CONTENTS stream, there is an index
	 * to various types of pages in the document. */
	struct Zone
	{
		Zone() : m_offset(0), m_length(0) {}
		virtual ~Zone() {}
		uint32_t const &begin() const
		{
			return m_offset;
		}
		uint32_t end() const
		{
			return m_offset+m_length;
		}
		uint32_t const &length() const
		{
			return m_length;
		}
		void setBegin(uint32_t pos)
		{
			m_offset = pos;
		}
		void setLength(uint32_t _length)
		{
			m_length = _length;
		}
		void setEnd(uint32_t _end)
		{
			m_length = _end-m_offset;
		}

		bool valid() const
		{
			return m_offset && m_length;
		}
	protected:
		uint32_t m_offset;
		uint32_t m_length;
	};

	struct Note : public Zone
	{
		Note() : Zone(), m_textOffset(0) {}
		uint32_t m_textOffset;
	};

	struct Stream : public Zone
	{
		Stream() : Zone(), m_type(Z_Dummy) {}

		enum Type {Z_Dummy=0, Z_Body=1, Z_Footnotes=2, Z_Endnotes = 3}  m_type;
	};
};


#endif /* WPS8_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
