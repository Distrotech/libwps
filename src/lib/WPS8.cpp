/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <stdlib.h>
#include <string.h>

#include "libwps_internal.h"

#include "WPSContentListener.h"
#include "WPSList.h"
#include "WPSPageSpan.h"
#include "WPSSubDocument.h"

#include "WPS8.h"

namespace WPS8ParserInternal
{
//! Internal: the subdocument of a WPS8
class SubDocument : public WPSSubDocument
{
public:
	//! type of an entry stored in textId
	enum Type { Unknown, Footnote, Endnote };

	//! constructor for a text entry
	SubDocument(WPXInputStreamPtr input, WPS8Parser &pars, Type type, int id) :
		WPSSubDocument (input, &pars, id), m_type(type) {}
	//! destructor
	~SubDocument() {}

	//! operator==
	virtual bool operator==(WPSSubDocumentPtr const &doc) const
	{
		if (!WPSSubDocument::operator==(doc))
			return false;
		SubDocument const *sDoc = dynamic_cast<SubDocument const *>(doc.get());
		if (m_type != sDoc->m_type)
			return false;
		return true;
	}

	//! the parser function
	void parse(WPSContentListenerPtr &listener, libwps::SubDocumentType type);

	Type m_type;
};

void SubDocument::parse(WPSContentListenerPtr &listener, libwps::SubDocumentType type)
{
	if (!listener.get())
	{
		WPS_DEBUG_MSG(("SubDocument::parse: no listener\n"));
		return;
	}
	if (!dynamic_cast<WPS8ContentListener *>(listener.get()))
	{
		WPS_DEBUG_MSG(("SubDocument::parse: bad listener\n"));
		return;
	}
	WPS8ContentListenerPtr &listen =  reinterpret_cast<WPS8ContentListenerPtr &>(listener);

	if (!m_parser)
	{
		listen->insertCharacter(' ');
		WPS_DEBUG_MSG(("SubDocument::parse: bad parser\n"));
		return;
	}

	if (m_id < 0 || m_type == Unknown)
	{
		WPS_DEBUG_MSG(("SubDocument::parse: empty document found...\n"));
		listen->insertCharacter(' ');
		return;
	}

	long actPos = m_input->tell();
	WPS8Parser *mnParser = reinterpret_cast<WPS8Parser *>(m_parser);
	if (type == libwps::DOC_NOTE)
		mnParser->sendNote(m_input, m_id, m_type == Endnote);
	else
	{
		WPS_DEBUG_MSG(("SubDocument::parse: find unknown type of document...\n"));
	}
	m_input->seek(actPos, WPX_SEEK_SET);
}

}

/*
WPS8Parser public
*/

WPS8Parser::WPS8Parser(WPXInputStreamPtr &input, WPSHeaderPtr &header) :
	WPSParser(input, header),
	m_listener(),
	m_offset_eot(0),
	m_oldTextAttributeBits(0),
	m_headerIndexTable(),
	m_CHFODs(),
	m_PAFODs(),
	m_fontNames(),
	m_streams(),
	m_footnotes(), m_actualFootnote(0),
	m_endnotes(), m_actualEndnote(0)
{
}

WPS8Parser::~WPS8Parser ()
{
}

void WPS8Parser::parse(WPXDocumentInterface *documentInterface)
{
	std::vector<WPSPageSpan> pageList;

	WPS_DEBUG_MSG(("WPS8Parser::parse()\n"));

	/* parse pages */
	parsePages(pageList, getInput());

	/* parse document */
	m_listener.reset(new WPS8ContentListener(pageList, documentInterface));
	parse(getInput());
	m_listener.reset();
}


/*
WPS8Parser private
*/


/**
 * Reads fonts table into memory.
 *
 */
void WPS8Parser::readFontsTable(WPXInputStreamPtr &input)
{
	/* find the fonts page offset, fonts array offset, and ending offset */
	IndexMultiMap::iterator pos;
	pos = m_headerIndexTable.lower_bound("FONT");
	if (m_headerIndexTable.end() == pos)
	{
		WPS_DEBUG_MSG(("Works8: error: no FONT in header index table\n"));
		throw libwps::ParseException();
	}
	Zone const &entry = pos->second;
	input->seek(entry.begin() + 0x04, WPX_SEEK_SET);
	uint32_t n_fonts = libwps::readU32(input);
	input->seek(entry.begin() + 0x10 + (4*n_fonts), WPX_SEEK_SET);

	/* read each font in the table */
	while (input->tell() > 0 && (unsigned long)(input->tell()+8) < entry.end() && m_fontNames.size() < n_fonts)
	{
#ifdef DEBUG
		uint32_t unknown = libwps::readU32(input);
#else
		input->seek(4, WPX_SEEK_CUR);
#endif
		uint16_t string_size = libwps::readU16(input);

		std::string s;
		for (; string_size>0; string_size--)
			s.append(1, (uint16_t)libwps::readU16(input));
		s.append(1, (char)0);
		if (s.empty())
			continue;
		WPS_DEBUG_MSG(("Works: debug: unknown={0x%08X}, name=%s\n",
		               unknown, s.c_str()));
		m_fontNames.push_back(s);
	}

	if (m_fontNames.size() != n_fonts)
	{
		WPS_DEBUG_MSG(("Works: warning: expected %i fonts but only found %i\n",
		               n_fonts, int(m_fontNames.size())));
	}
}

/**
 * Reads streams (subdocuments) information
 */

void WPS8Parser::readStreams(WPXInputStreamPtr &input)
{
	IndexMultiMap::iterator pos;
	pos = m_headerIndexTable.lower_bound("STRS");
	if (m_headerIndexTable.end() == pos)
	{
		WPS_DEBUG_MSG(("Works8: error: no STRS in header index table\n"));
		throw libwps::ParseException();
	}
	Zone const &entry = pos->second;
	uint32_t last_pos = 0;

	uint32_t n_streams;
	input->seek(entry.begin(), WPX_SEEK_SET);
	n_streams = libwps::readU32(input);

	if (n_streams > 100)
	{
		WPS_DEBUG_MSG(("Probably garbled STRS: count = %u\n",n_streams));
	}

	/* skip mysterious header*/
	input->seek(8, WPX_SEEK_CUR);

	Stream s;
	uint32_t offset;
	for (unsigned i=0; i < n_streams; i++)
	{
		offset = libwps::readU32(input);
		// TODO: assert index in within text
		s.setBegin(last_pos);
		s.setLength(offset);
		s.m_type = Stream::Z_Dummy;
		m_streams.push_back(s);

		last_pos += offset;
	}
	offset = libwps::readU32(input);
	if (offset)
	{
		WPS_DEBUG_MSG(("Offset table is not 0-terminated!\n"));
	}

	for (unsigned j=0; j < n_streams; j++)
	{
		uint16_t len;
		Stream::Type type = Stream::Z_Dummy;

		len = libwps::readU16(input);
		if (len > 10)
		{
			WPS_DEBUG_MSG(("Rogue strm[%d] def len (%d)\n",j,len));
			input->seek(len-2,WPX_SEEK_CUR);
		}

		if (len > 4)
		{
			libwps::readU32(input); // assume == 0x22000000
			type = Stream::Type(libwps::readU32(input));
		}
		else input->seek(len-2,WPX_SEEK_CUR);

		m_streams[j].m_type = type;
	}

#ifdef DEBUG
	int bodypos = -1;
	for (unsigned k=0; k < n_streams; k++)
	{
		int z = m_streams[k].m_type;
		if (z == Stream::Z_Dummy) WPS_DEBUG_MSG(("Default strm[%d] type\n",k));
		if (z == Stream::Z_Body)
		{
			if (bodypos < 0) bodypos = k;
			else WPS_DEBUG_MSG(("Duplicating body (strm[%d])\n",k));
		}
	}
	if (bodypos < 0) WPS_DEBUG_MSG(("Doc body not found!\n"));
#endif
}

void WPS8Parser::readNotes(std::vector<Note> &dest, WPXInputStreamPtr &input, const char *key)
{
	IndexMultiMap::iterator pos;
	pos = m_headerIndexTable.lower_bound(key);
	if (m_headerIndexTable.end() == pos)
		return;

	uint32_t boff;
	do
	{
		input->seek(pos->second.begin(),WPX_SEEK_SET);
		uint32_t unk1 = libwps::readU32(input);
		uint32_t count = libwps::readU32(input);
		input->seek(8, WPX_SEEK_CUR);
		if (dest.size() < count)
			dest.resize(count);
		for (unsigned i=0; i < count; i++)
		{
			boff = libwps::readU32(input);
			if (unk1)
			{
				/* the position to characters (unused) */
				dest[i].m_textOffset = boff;
			}
			else
			{
				if (i) dest[i-1].setEnd(boff);
				dest[i].setBegin(boff);
			}
		}
		boff = libwps::readU32(input);
		if (!unk1 && dest.size()>0) dest[dest.size()-1].setEnd(boff);

		while (++pos != m_headerIndexTable.end())
		{
			if (!strcmp(pos->first.c_str(),key)) break;
		}
	}
	while (pos != m_headerIndexTable.end());
	/* some kind of loop needed */
}

/**
 * Read an UTF16 character in LE byte ordering, convert it
 * and append it to the text buffer as UTF8. Courtesy of glib2
 *
 */

#define SURROGATE_VALUE(h,l) (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)

void WPS8Parser::appendUTF16LE(WPXInputStreamPtr &input)
{
	uint16_t high_surrogate = 0;
	bool fail = false;
	uint16_t readVal;
	uint32_t ucs4Character;
	while (true)
	{
		if (input->atEOS())
		{
			fail = true;
			break;
		}
		readVal = libwps::readU16(input);
		if (readVal >= 0xdc00 && readVal < 0xe000) /* low surrogate */
		{
			if (high_surrogate)
			{
				ucs4Character = SURROGATE_VALUE(high_surrogate, readVal);
				high_surrogate = 0;
				break;
			}
			else
			{
				fail = true;
				break;
			}
		}
		else
		{
			if (high_surrogate)
			{
				fail = true;
				break;
			}
			if (readVal >= 0xd800 && readVal < 0xdc00) /* high surrogate */
			{
				high_surrogate = readVal;
			}
			else
			{
				ucs4Character = readVal;
				break;
			}
		}
	}
	if (fail)
		throw libwps::GenericException();

	m_listener->insertUnicode(ucs4Character);
}


/**
 * Read the range of the document text using previously-read
 * formatting information, up to but excluding endpos.
 *
 */

void WPS8Parser::readTextRange(WPXInputStreamPtr &input,
                               uint32_t startpos, uint32_t endpos, uint16_t stream)
{
	WPS_DEBUG_MSG(("WPS8Parser::readTextRange(stream=%d)\n",stream));

	std::vector<WPSFOD>::iterator FODs_iter;
	std::vector<WPSFOD>::iterator PFOD_iter;

	uint32_t last_fcLim = 0x200;
	uint32_t start_fcLim = 0x200 + startpos*2;
	uint32_t total_fcLim = start_fcLim + (endpos - startpos)*2;
	PFOD_iter = m_PAFODs.begin();
	FODs_iter = m_CHFODs.begin();

	while (last_fcLim < start_fcLim)
	{
		uint32_t c_len = (*FODs_iter).m_fcLim - last_fcLim;
		uint32_t p_len = (*PFOD_iter).m_fcLim - last_fcLim;
		uint32_t len = (c_len > p_len)? p_len : c_len;

		last_fcLim += len;
		if (len == c_len) FODs_iter++;
		if (len == p_len) PFOD_iter++;
	}

	/* should never happen? */
	if (last_fcLim > start_fcLim) last_fcLim = start_fcLim;

	uint16_t specialCode=0;
	int fieldType = 0;
	for (; last_fcLim < total_fcLim; FODs_iter++)
	{
		WPSFOD fod = *(FODs_iter);
		uint32_t c_len = (*FODs_iter).m_fcLim - last_fcLim;
		uint32_t p_len = (*PFOD_iter).m_fcLim - last_fcLim;

		if (last_fcLim + c_len > total_fcLim) c_len = total_fcLim - last_fcLim;
		uint32_t len = (c_len > p_len)? p_len : c_len;


		if (len % 2 != 0)
		{
			WPS_DEBUG_MSG(("Works: error: len %i is odd\n", len));
			throw libwps::ParseException();
		}
		len /= 2;

		/* print rgchProp as hex bytes */
		WPS_DEBUG_MSG(("rgch="));
		for (unsigned int blah=0; blah < (*FODs_iter).m_fprop.m_rgchProp.length(); blah++)
		{
			WPS_DEBUG_MSG(("%02X ", (uint8_t) (*FODs_iter).m_fprop.m_rgchProp[blah]));
		}
		WPS_DEBUG_MSG(("\n"));

		/* process character formatting */
		if ((*FODs_iter).m_fprop.m_cch > 0)
			propertyChange((*FODs_iter).m_fprop.m_rgchProp, specialCode, fieldType);

		/* loop until character format not exhausted */
		do
		{
			/* paragraph format may change here*/

			if ((*PFOD_iter).m_fprop.m_cch > 0)
				propertyChangePara((*PFOD_iter).m_fprop.m_rgchProp);

			/* plain text */
			input->seek(last_fcLim, WPX_SEEK_SET);
			for (uint32_t i = len; i>0; i--)
			{
				uint16_t readVal = libwps::readU16(input);

				if (0x00 == readVal)
					break;

				switch (readVal)
				{
				case 0x0A:
					break;

				case 0x0C:
					//fixme: add a page to list of pages
					//m_listener->insertBreak(WPS_PAGE_BREAK);
					break;

				case 0x0D:
					m_listener->insertEOL();
					break;

				case 0x0E:
					m_listener->insertBreak(WPS_COLUMN_BREAK);
					break;

				case 0x1E:
					//fixme: non-breaking hyphen
					break;

				case 0x1F:
					//fixme: optional breaking hyphen
					break;

				case 0x23:
					if (specialCode)
					{
						//	TODO: fields, pictures, etc.
						switch (specialCode)
						{
						case 3:
						{
							if (stream != Stream::Z_Body) break;
							shared_ptr<WPSSubDocument> doc
							(new WPS8ParserInternal::SubDocument(input, *this, WPS8ParserInternal::SubDocument::Footnote, m_actualFootnote++));
							m_listener->insertNote(WPSContentListener::FOOTNOTE, doc);
							break;
						}
						case 4:
						{
							if (stream != Stream::Z_Body) break;
							shared_ptr<WPSSubDocument> doc
							(new WPS8ParserInternal::SubDocument(input, *this, WPS8ParserInternal::SubDocument::Footnote, m_actualEndnote++));
							m_listener->insertNote(WPSContentListener::ENDNOTE, doc);
							break;
						}
						case 5:
							switch (fieldType)
							{
							case -1:
								m_listener->insertField(WPSContentListener::PageNumber);
								break;
							case -4:
								m_listener->insertField(WPSContentListener::Date);
								break;
							case -5:
								m_listener->insertField(WPSContentListener::Time);
								break;
							default:
								break;
							}
							break;
						default:
							m_listener->insertCharacter(0xE2/*0x263B*/);
							m_listener->insertCharacter(0x98);
							m_listener->insertCharacter(0xBB);
						}
						break;
					}
					// ! fallback to default

				case 0xfffc:
					// ! fallback to default

				default:
					if (readVal < 28 && readVal != 9)
					{
						// do not add unprintable control which can create invalid odt file
						WPS_DEBUG_MSG(("WPS8Parser::readTextRange(find unprintable character: ignored)\n"));
						break;
					}
					// fixme: convert UTF-16LE to UTF-8
					input->seek(-2, WPX_SEEK_CUR);
					this->appendUTF16LE(input);
					break;
				}
			}

			len *= 2;
			c_len -= len;
			p_len -= len;
			last_fcLim += len;

			if (p_len == 0)
			{
				PFOD_iter++;
				if (c_len > 0)   /* otherwise will be set by outside loop */
				{
					p_len = (*PFOD_iter).m_fcLim - last_fcLim;
					len = (c_len > p_len)? p_len : c_len;
					len /= 2;
				}
			}
		}
		while (c_len > 0);

	}
}

void WPS8Parser::sendNote(WPXInputStreamPtr &input, int id, bool is_endnote)
{
	std::vector<Note> const &notes = is_endnote ? m_endnotes : m_footnotes;
	if (id < 0 || id >= int(notes.size()))
	{
		WPS_DEBUG_MSG(("WPS8Parser::sendNote: can not find footnote\n"));
		if (m_listener) m_listener->insertCharacter(' ');
		return;
	}
	Note const &note =notes[id];
	Stream stream;
	Stream::Type streamkey = is_endnote ? Stream::Z_Endnotes : Stream::Z_Footnotes;
	for (unsigned i=0; i<m_streams.size(); i++)
	{
		if (m_streams[i].m_type == streamkey)
		{
			stream = m_streams[i];
			break;
		}
	}

	WPS_DEBUG_MSG(("Reading footnote [%d;%d)\n",note.begin(),note.end()));

	uint32_t pos = input->tell();
	uint32_t beginPos = stream.begin()+note.begin();
	uint32_t endPos = stream.begin()+note.end();
	// try to remove the end of lines which can appear after the footnote
	while (endPos-1 > beginPos)
	{
		input->seek(0x200+2*(endPos-1),WPX_SEEK_SET);
		uint16_t readVal =libwps::readU16(input);
		if (readVal != 0xd) break;
		endPos -= 1;
	}
	readTextRange(input,beginPos,endPos,streamkey);
	input->seek(pos,WPX_SEEK_SET);
}


/**
 * Read a single page (of size page_size bytes) that contains formatting descriptors
 * for either characters OR paragraphs.  Starts reading at current position in stream.
 *
 * Return: true if more pages of this type exist, otherwise false
 *
 */

//fixme: this readFODPage is mostly the same as in WPS4

bool WPS8Parser::readFODPage(WPXInputStreamPtr &input, std::vector<WPSFOD> &FODs, uint16_t page_size)
{
	uint32_t page_offset = input->tell();
	uint16_t cfod = libwps::readU16(input); /* number of FODs on this page */

	//fixme: what is the largest possible cfod?
	if (cfod > 0x54)
	{
		WPS_DEBUG_MSG(("Works8: error: cfod = %i (0x%X)\n", cfod, cfod));
		throw libwps::ParseException();
	}

	input->seek(page_offset + 2 + 6, WPX_SEEK_SET);	// fixme: unknown

	int first_fod = FODs.size();

	/* Read array of m_fcLim of FODs.  The m_fcLim refers to the offset of the
	       last character covered by the formatting. */
	for (int i = 0; i < cfod; i++)
	{
		WPSFOD fod;
		fod.m_fcLim = libwps::readU32(input);
//		WPS_DEBUG_MSG(("Works: info: m_fcLim = %i (0x%X)\n", fod.m_fcLim, fod.m_fcLim));

		/* check that m_fcLim is not too large */
		if (fod.m_fcLim > m_offset_eot)
		{
			WPS_DEBUG_MSG(("Works: error: length of 'text selection' %i > "
			               "total text length %i\n", fod.m_fcLim, m_offset_eot));
			throw libwps::ParseException();
		}

		/* check that m_fcLim is monotonic */
		if (FODs.size() > 0 && FODs.back().m_fcLim > fod.m_fcLim)
		{
			WPS_DEBUG_MSG(("Works: error: character position list must "
			               "be monotonic, but found %i, %i\n", FODs.back().m_fcLim, fod.m_fcLim));
			throw libwps::ParseException();
		}
		FODs.push_back(fod);
	}

	/* Read array of m_bfprop of FODs.  The m_bfprop is the offset where
	   the FPROP is located. */
	std::vector<WPSFOD>::iterator FODs_iter;
	for (FODs_iter = FODs.begin() + first_fod; FODs_iter!= FODs.end(); FODs_iter++)
	{
		if ((*FODs_iter).m_fcLim == m_offset_eot)
			break;

		(*FODs_iter).m_bfprop = libwps::readU16(input);

		/* check size of m_bfprop  */
		if (((*FODs_iter).m_bfprop < (8 + (6*cfod)) && (*FODs_iter).m_bfprop > 0) ||
		        (*FODs_iter).m_bfprop  > (page_size - 1))
		{
			WPS_DEBUG_MSG(("Works: error: size of m_bfprop is bad "
			               "%i (0x%X)\n", (*FODs_iter).m_bfprop, (*FODs_iter).m_bfprop));
			throw libwps::ParseException();
		}

		(*FODs_iter).m_bfpropAbs = (*FODs_iter).m_bfprop + page_offset;
//		WPS_DEBUG_MSG(("Works: debug: m_bfprop = 0x%03X, m_bfpropAbs = 0x%03X\n",
//                       (*FODs_iter).m_bfprop, (*FODs_iter).m_bfpropAbs));
	}


	/* Read array of FPROPs.  These contain the actual formatting
	   codes (bold, alignment, etc.) */
	for (FODs_iter = FODs.begin()+first_fod; FODs_iter!= FODs.end(); FODs_iter++)
	{
		if ((*FODs_iter).m_fcLim == m_offset_eot)
			break;

		if (0 == (*FODs_iter).m_bfprop)
		{
			(*FODs_iter).m_fprop.m_cch = 0;
			continue;
		}

		input->seek((*FODs_iter).m_bfpropAbs, WPX_SEEK_SET);
		(*FODs_iter).m_fprop.m_cch = libwps::readU8(input);
		if (0 == (*FODs_iter).m_fprop.m_cch)
		{
			WPS_DEBUG_MSG(("Works: error: 0 == cch at file offset 0x%lx", (input->tell())-1));
			throw libwps::ParseException();
		}
		// check that the property remains in the FOD zone
		if ((*FODs_iter).m_bfpropAbs+(*FODs_iter).m_fprop.m_cch > page_offset+page_size)
		{
			WPS_DEBUG_MSG(("Works: error: cch = %i, too large ", (*FODs_iter).m_fprop.m_cch));
			throw libwps::ParseException();
		}

		(*FODs_iter).m_fprop.m_cch--;

		for (int i = 0; (*FODs_iter).m_fprop.m_cch > i; i++)
			(*FODs_iter).m_fprop.m_rgchProp.append(1, (uint8_t)libwps::readU8(input));
	}

	/* go to end of page */
	input->seek(page_offset	+ page_size, WPX_SEEK_SET);

	return (m_offset_eot > FODs.back().m_fcLim);
}

/**
 * Parse an index entry in the file format's header.  For example,
 * this function may be called multiple times to parse several FDPP
 * entries.  This functions begins at the current position of the
 * input stream, which will be advanced.
 *
 */

void WPS8Parser::parseHeaderIndexEntry(WPXInputStreamPtr &input)
{
	WPS_DEBUG_MSG(("Works8: debug: parseHeaderIndexEntry() at file pos 0x%lX\n", input->tell()));

	uint16_t cch = libwps::readU16(input);
	if (0x18 != cch)
	{
		WPS_DEBUG_MSG(("Works8: error: parseHeaderIndexEntry cch = %i (0x%X)\n", cch, cch));
		/* Osnola: normally, this size must be >= 0x18
		   In my code, I throw an exception when this size is less than 10
		   to try to continue the parsing ( but I do not accept this entry)
		 */
		if (cch < 10)
			throw libwps::ParseException();
	}

	std::string name;

	// sanity check
	int i;
	for (i =0; i < 4; i++)
	{
		name.append(1, libwps::readU8(input));

		if ((uint8_t)name[i] != 0 && (uint8_t)name[i] != 0x20 &&
		        (41 > (uint8_t)name[i] || (uint8_t)name[i] > 90))
		{
			WPS_DEBUG_MSG(("Works8: error: bad character=%u (0x%02x) in name in header index\n",
			               (uint8_t)name[i], (uint8_t)name[i]));
			throw libwps::ParseException();
		}
	}
	name.append(1, (char)0);

	std::string unknown1;
	for (i = 0; i < 6; i ++)
		unknown1.append(1, libwps::readU8(input));

	std::string name2;
	for (i =0; i < 4; i++)
		name2.append(1, libwps::readU8(input));
	name2.append(1, (char)0);

	if (name != name2)
	{
		WPS_DEBUG_MSG(("Works8: error: name != name2, %s != %s\n",
		               name.c_str(), name2.c_str()));
		// fixme: what to do with this?
//		throw libwps::ParseException();
	}

	Zone hie;
	hie.setBegin(libwps::readU32(input));
	hie.setLength(libwps::readU32(input));

	WPS_DEBUG_MSG(("Works8: debug: header index entry %s with offset=0x%04X, length=0x%04X\n",
	               name.c_str(), hie.begin(), hie.end()));

	m_headerIndexTable.insert(std::multimap<std::string, Zone>::value_type(name, hie));
	// OSNOLA: cch is the length of the entry, so we must advance by cch-0x18
	input->seek(input->tell() + cch - 0x18, WPX_SEEK_SET);
}

/**
 * In the header, parse the index to the different sections of
 * the CONTENTS stream.
 *
 */
void WPS8Parser::parseHeaderIndex(WPXInputStreamPtr &input)
{
	input->seek(0x0C, WPX_SEEK_SET);
	uint16_t n_entries = libwps::readU16(input);
	// fixme: sanity check n_entries

	input->seek(0x18, WPX_SEEK_SET);
	do
	{
		uint16_t unknown1 = libwps::readU16(input);
		if (0x01F8 != unknown1)
		{
			WPS_DEBUG_MSG(("Works8: error: unknown1=0x%X\n", unknown1));
#if 0
			throw libwps::ParseException();
#endif
		}

		uint16_t n_entries_local = libwps::readU16(input);

		if (n_entries_local > 0x20)
		{
			WPS_DEBUG_MSG(("Works8: error: n_entries_local=%i\n", n_entries_local));
			throw libwps::ParseException();
		}

		uint32_t next_index_table = libwps::readU32(input);

		do
		{
			parseHeaderIndexEntry(input);
			n_entries--;
			n_entries_local--;
		}
		while (n_entries > 0 && n_entries_local);

		if (0xFFFFFFFF == next_index_table && n_entries > 0)
		{
			WPS_DEBUG_MSG(("Works8: error: expected more header index entries\n"));
			throw libwps::ParseException();
		}

		if (0xFFFFFFFF == next_index_table)
			break;

		WPS_DEBUG_MSG(("Works8: debug: seeking to position 0x%X\n", next_index_table));
		input->seek(next_index_table, WPX_SEEK_SET);
	}
	while (n_entries > 0);
}

/**
 * Read the page format from the file.  It seems that WPS8 files
 * can only have one page format throughout the whole document.
 *
 */
void WPS8Parser::parsePages(std::vector<WPSPageSpan> &pageList, WPXInputStreamPtr & /* input */)
{
	//fixme: this method doesn't do much

	/* record page format */
	WPSPageSpan ps;
	pageList.push_back(ps);
}

void WPS8Parser::parse(WPXInputStreamPtr &input)
{
	WPS_DEBUG_MSG(("WPS8Parser::parse()\n"));

	m_listener->startDocument();

	/* header index */
	parseHeaderIndex(input);

	IndexMultiMap::iterator pos;
	for (pos = m_headerIndexTable.begin(); pos != m_headerIndexTable.end(); ++pos)
	{
		WPS_DEBUG_MSG(("Works: debug: m_headerIndexTable: %s, offset=0x%X, length=0x%X, end=0x%X\n",
		               pos->first.c_str(), pos->second.begin(), pos->second.length(), pos->second.end()));
	}

	/* What is the total length of the text? */
	pos = m_headerIndexTable.lower_bound("TEXT");
	if (m_headerIndexTable.end() == pos)
	{
		WPS_DEBUG_MSG(("Works: error: no TEXT in header index table\n"));
	}
	m_offset_eot = pos->second.end();
	WPS_DEBUG_MSG(("Works: debug: TEXT m_offset_eot = 0x%04X\n", m_offset_eot));

	/* read character/para FODs (FOrmatting Descriptors) */
	for (int wh = 0; wh < 2; wh++)
	{
		char const *name = wh==0 ? "FDPC" : "FDPP";
		for (pos = m_headerIndexTable.begin(); pos != m_headerIndexTable.end(); ++pos)
		{
			if (0 != strcmp(name,pos->first.c_str()))
				continue;

			Zone const &entry = pos->second;
			WPS_DEBUG_MSG(("Works: debug: %s offset=0x%X, length=0x%X\n",
			               name, entry.begin(), entry.length()));

			input->seek(entry.begin(), WPX_SEEK_SET);
			if (entry.length() != 512)
			{
				WPS_DEBUG_MSG(("Works: warning: %s offset=0x%X, length=0x%X\n",
				               name,entry.begin(), entry.length()));
			}
			if (wh==0)
				readFODPage(input, m_CHFODs, entry.length());
			else
				readFODPage(input, m_PAFODs, entry.length());
		}
	}

	/* read streams table*/
	readStreams(input);

	/* read fonts table */
	readFontsTable(input);

	readNotes(m_footnotes,input,"FTN ");
	readNotes(m_endnotes,input,"EDN ");

	m_actualFootnote = m_actualEndnote = 0;

	/* process text file using previously-read character formatting */
	uint32_t doc_start = 0, doc_end = (m_offset_eot - 0x200) >> 1; // character offsets
	uint32_t doc_start2 = doc_start, doc_end2 = doc_end;
	for (unsigned i=0; i<m_streams.size(); i++)
	{
		/* skip to extract full document text for debug purposes */
		/*if (m_streams[i].m_type == Stream::Z_Body) {
			readTextRange(input,m_listener,m_streams[i].start,m_streams[i].limit,
				Stream::Z_Body);
		} else*/ if (m_streams[i].m_type == Stream::Z_Footnotes ||
		             m_streams[i].m_type == Stream::Z_Endnotes)
		{
			if (m_streams[i].begin() < doc_end) doc_end = m_streams[i].begin();
			if (m_streams[i].end() > doc_start2) doc_start2 = m_streams[i].end();
		}
	}
	if (doc_end > doc_start2) doc_start2 = doc_end;

	readTextRange(input,doc_start,doc_end,Stream::Z_Body);
	if (doc_end2 > doc_start2)
		readTextRange(input,doc_start2,doc_end2,Stream::Z_Body);

	m_listener->endDocument();
}

/**
 * @param newTextAttributeBits: all the new, current bits (will be compared against old, and old will be discarded).
 *
 */
void WPS8Parser::propertyChangeDelta(uint32_t newTextAttributeBits)
{
	if (newTextAttributeBits == m_oldTextAttributeBits)
		return;
#ifdef DEBUG
	static uint32_t const listAttributes[6] = { WPS_BOLD_BIT, WPS_ITALICS_BIT, WPS_UNDERLINE_BIT, WPS_STRIKEOUT_BIT, WPS_SUBSCRIPT_BIT, WPS_SUPERSCRIPT_BIT };
	uint32_t diffAttributes = (m_oldTextAttributeBits ^ newTextAttributeBits);
	for (int i = 0; i < 6; i++)
	{
		if (diffAttributes & listAttributes[i])
		{
			WPS_DEBUG_MSG(("WPS8Parser::propertyChangeDelta: attribute %i changed, now = %i\n", i, newTextAttributeBits & listAttributes[i]));
		}
	}
#endif
	m_listener->setFontAttributes(newTextAttributeBits);
	m_oldTextAttributeBits = newTextAttributeBits;
}



/**
 * Process a character property change.  The Works format supplies
 * all the character formatting each time there is any change (as
 * opposed to HTML, for example).  In Works 8, the position in
 * rgchProp is not significant because there are some kind of
 * codes.
 *
 */
void WPS8Parser::propertyChange(std::string rgchProp, uint16_t &specialCode, int &fieldType)
{
	//fixme: this method is immature

	/* set default properties */
	uint32_t textAttributeBits = 0;
	m_listener->setTextColor(0);
	propertyChangeDelta(0);
	m_listener->setFontSize(10);
	/* maybe other stuff */

	/* check */
	/* sometimes, the rgchProp is blank */
	if (0 == rgchProp.length())
	{
		return;
	}
	/* other than blank, the shortest should be 9 bytes */
	if (rgchProp.length() < 3)
	{
		WPS_DEBUG_MSG(("Works8: error: rgchProp.length() < 9\n"));
		throw libwps::ParseException();
	}
	if (0 == (rgchProp.length() % 2))
	{
		WPS_DEBUG_MSG(("Works8: error: rgchProp.length() is even\n"));
		throw libwps::ParseException();
	}
	if (0 != rgchProp[0] || 0 != rgchProp[1] || 0 != rgchProp[2])
	{
		WPS_DEBUG_MSG(("Works8: error: rgchProp does not begin 0x000000\n"));
		throw libwps::ParseException();
	}

	/* set difference from default properties */
	for (uint32_t x = 3; x < rgchProp.length(); x += 2)
	{
		if (0x0A == rgchProp[x+1])
		{
			switch(rgchProp[x])
			{
			case 0x02:
				textAttributeBits |= WPS_BOLD_BIT;
				break;
			case 0x03:
				textAttributeBits |= WPS_ITALICS_BIT;
				break;
			case 0x04:
				textAttributeBits |= WPS_OUTLINE_BIT;
				break;
			case 0x05:
				textAttributeBits |= WPS_SHADOW_BIT;
				break;
			case 0x10:
				textAttributeBits |= WPS_STRIKEOUT_BIT;
				break;
			case 0x13:
				textAttributeBits |= WPS_SMALL_CAPS_BIT;
				break;
			case 0x15:
				//fixme: unknown
				break;
			case 0x14:
				textAttributeBits |= WPS_ALL_CAPS_BIT;
				break;
			case 0x16:
				textAttributeBits |= WPS_EMBOSS_BIT;
				break;
			case 0x17:
				textAttributeBits |= WPS_ENGRAVE_BIT;
				break;
			default:
				WPS_DEBUG_MSG(("Works8: error: unknown 0x0A format code 0x%04X\n", rgchProp[x]));
				// OSNOLA: ok to continue, this is a bool field
				// ( without any data )
				//throw libwps::ParseException();
			}
			continue;
		}

		uint16_t format_code = rgchProp[x] | (rgchProp[x+1] << 8);
		int unparsedChar = rgchProp.length()-x-2;
		bool ok = true;
		switch (format_code)
		{
		case 0x0000:
			break;

		case 0x1200:
		{
			if (unparsedChar < 2)
			{
				ok = false;
				break;
			}
			// special code
			specialCode = WPS_LE_GET_GUINT16(rgchProp.substr(x+2,2).c_str());
			x += 2;
		}
		break;

		case 0x120F:
			if (unparsedChar < 2)
			{
				ok = false;
				break;
			}
			if (1 == rgchProp[x+2])
				textAttributeBits |= WPS_SUPERSCRIPT_BIT;
			if (2 == rgchProp[x+2])
				textAttributeBits |= WPS_SUBSCRIPT_BIT;
			x += 2;
			break;

		case 0x121E:
			// fixme: there are various styles of underline
			textAttributeBits |= WPS_UNDERLINE_BIT;
			x += 2;
			break;

		case 0x220C:
		{
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			uint32_t font_size = WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
			m_listener->setFontSize(font_size/12700);
			x += 4;
			break;
		}

		case 0x2218:
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			x += 4;
			break;

		case 0x2212:
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			m_listener->setTextLanguage(WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str()));
			x += 4;
			break;

		case 0x2222:
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			fieldType = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
			x += 4;
			break;

		case 0x2223:
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			//fixme: date and time field?
			x += 4;
			break;

		case 0x222E:
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			m_listener->setTextColor((((unsigned char)rgchProp[x+2]<<16)+((unsigned char)rgchProp[x+3]<<8)+
			                          (unsigned char)rgchProp[x+4])&0xFFFFFF);
			x += 4;
			break;

		case 0x8A24:
		{
			if (unparsedChar < 7)
			{
				ok = false;
				break;
			}
			/* font change */
			uint8_t font_n = (uint8_t)rgchProp[x+8];
			if (font_n > m_fontNames.size())
			{
				WPS_DEBUG_MSG(("Works: error: encountered font %i (0x%02x) which is not indexed\n",
				               font_n,font_n ));
				throw libwps::ParseException();
			}
			else
				m_listener->setTextFont(m_fontNames[font_n].c_str());

			//x++;
			x += rgchProp[x+2];
		}
		break;
		default:
			WPS_DEBUG_MSG(("Works8: error: unknown format code 0x%04X\n", format_code));
			switch ((format_code>>12)&0xF)
			{
			case 1:
				x+=2;
				break;
			case 2:
				x+=4;
				break;
			case 8:
				if (unparsedChar < 2)
				{
					ok = false;
					break;
				}
				x += rgchProp[x+2];
				break;
			}
			//throw libwps::ParseException();
			break;
		}
		if (!ok)
		{
			WPS_DEBUG_MSG(("WPS8Parser::propertyChange: problem with field size, stop\n"));
			break;
		}
	}

	propertyChangeDelta(textAttributeBits);
}

void WPS8Parser::propertyChangePara(std::string rgchProp)
{
	static const libwps::Justification _align[]=
	{
		libwps::JustificationLeft, libwps::JustificationRight,
		libwps::JustificationCenter, libwps::JustificationFull
	};
	libwps::Justification align = libwps::JustificationLeft;

	int iv = 0;
	std::vector<WPSTabStop> tabList;
	m_listener->setTabs(tabList);

	/* sometimes, the rgchProp is blank */
	if (0 == rgchProp.length())
	{
		m_listener->setCurrentListLevel(0);
		return;
	}
	float leftIndent=0.0, textIndent=0.0;
	int listLevel = 0;
	WPSList::Level level;
	for (uint32_t x = 3; x < rgchProp.length(); x += 2)
	{
		uint16_t format_code = rgchProp[x] | (rgchProp[x+1] << 8);

		int unparsedChar = rgchProp.length()-x-2;
		bool ok = true;
		switch (format_code)
		{
		case 0x1A03:
			// iv = WPS_LE_GET_GUINT16(rgchProp.substr(x+2,2).c_str()) & 0xF;
			/* paragraph has a bullet specified in num format*/
			level.m_type = libwps::BULLET;
			level.m_bullet = "*";
			listLevel = 1;
			x+=2;
			break;

		case 0x1204:
			if (unparsedChar < 2)
			{
				ok = false;
				break;
			}
			iv = WPS_LE_GET_GUINT16(rgchProp.substr(x+2,2).c_str()) & 0xF;
			if (iv >= 0 && iv < 4) align = _align[iv];
			x+=2;
			break;

		case 0x220C:
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			iv = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
			textIndent=iv/914400.0;
			x+=4;
			break;

		case 0x220D:
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			iv = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
			leftIndent=iv/914400.0;
			x+=4;
			break;

			/*case 0x220E:
				dw=*(int*)c;
				ps->cur_pf.right=dw/635;
				c+=4;
				break;

			case 0x2212:
				dw=*(int*)c;
				ps->cur_pf.before=dw/635;
				c+=4;
				break;

			case 0x2213:
				dw=*(int*)c;
				ps->cur_pf.after=dw/635;
				c+=4;
				break;
			*/
		case 0x2214:
		{
			if (unparsedChar < 4)
			{
				ok = false;
				break;
			}
			/* numbering style */
			iv = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
			int oldListLevel = listLevel;
			listLevel = 1;
			switch(iv & 0xFFFF)
			{
			case 0: // checkme
				WPS_DEBUG_MSG(("Find list flag=0\n"));
				level.m_type = libwps::NONE;
				listLevel = 0;
				break;
			case 2:
				level.m_type = libwps::ARABIC;
				break;
			case 3:
				level.m_type = libwps::LOWERCASE;
				break;
			case 4:
				level.m_type = libwps::UPPERCASE;
				break;
			case 5:
				level.m_type = libwps::LOWERCASE_ROMAN;
				break;
			case 6:
				level.m_type = libwps::UPPERCASE_ROMAN;
				break;
			default:
				listLevel=-1;
				WPS_DEBUG_MSG(("Unknown style %04x\n",(iv & 0xFFFF)));
				break;
			}
			if (listLevel == -1) listLevel = oldListLevel;
			else
			{
				switch(iv>>16)
				{
				case 2:
					level.m_suffix = ".";
					break;
				default:
					level.m_suffix = ")";
					break;
				}
			}
			x+=4;
		}
		break;

		case 0x8232:
		{
			if (unparsedChar < 2)
			{
				ok = false;
				break;
			}
			WPSTabStop tab;
			const char *ts = rgchProp.c_str();
			unsigned  t_count = 0;
			int  t_size = WPS_LE_GET_GUINT32(&ts[x+2]);
			if (unparsedChar < t_size)
			{
				ok = false;
				break;
			}
			int  tp_rem = 0;
			int  id = x+6;
			int  t_rem = t_size-4; // we skip 4 characters
			uint16_t prop;

			if (t_rem > 2)
			{
				prop = WPS_LE_GET_GUINT16(&ts[id]);
				if (prop == 0x1A27)
				{
					t_count = WPS_LE_GET_GUINT16(&ts[id+2]);
					id += 4;
					t_rem -= 4;

					if (t_count > 20) break; /* obviously wrong */
				}
				else break;   /* wrong format */
			}

			if (t_count > 0 && t_rem > 2)
			{
				prop = WPS_LE_GET_GUINT16(&ts[id]);
				if (prop == 0x8A28)
				{
					tp_rem = WPS_LE_GET_GUINT32(&ts[id+2]);
					id += 6;
					t_rem -= 6;
					tp_rem -= 4;

					if (tp_rem > t_rem) break; /* truncated? */
				}
				else break;   /* wrong format */
			}

			while (/*tabList.size() < t_count && */ tp_rem > 0)
			{
				prop = WPS_LE_GET_GUINT16(&ts[id]);
				id += 2;
				tp_rem -= 2;

				unsigned iid = (prop >> 3) & 0xF; /* TODO: verify*/
				int iprop = (prop & 0xFF87);

				if (iid >= t_count) break; /* sanity */
				while (iid >= tabList.size())
					tabList.resize(iid+1);
				switch (iprop)
				{
				case 0x2000:
				{
					float tabpos = WPS_LE_GET_GUINT32(&ts[id]) / 914400.0;
					tabList[iid].m_position = tabpos;
					id += 4;
					tp_rem -=4;
				}
				break;
				case 0x1001:
					switch (ts[id] & 0xF)
					{
					case 1:
						tabList[iid].m_alignment = WPSTabStop::RIGHT;
						break;
					case 2:
						tabList[iid].m_alignment = WPSTabStop::CENTER;
						break;
					case 3:
						tabList[iid].m_alignment = WPSTabStop::DECIMAL;
						break;
					};
					id += 2;
					tp_rem -= 2;
					break;
				case 0x1802:
					// TODO: leader
					id += 2;
					tp_rem -= 2;
					break;
				default:
					WPS_DEBUG_MSG(("Unknown tab prop %04x\n",iprop));
					break;// TODO: handle!
				};
			};

			/*while (t_rem > 4) {
				float tabpos = WPS_LE_GET_GUINT32(&ts[id]) / 914400.0;
				tab.m_pos = tabpos;
				tabList.push_back(tab);
				break;
				id += 4;
				t_rem -= 4;
			}*/

			m_listener->setTabs(tabList);

			x += t_size;
		}
		break;

		default:
			WPS_DEBUG_MSG(("Works8: error: unknown pformat code 0x%04X\n", format_code));
			switch ((format_code>>12)&0xF)
			{
			case 1:
				x+=2;
				break;
			case 2:
				x+=4;
				break;
			case 8:
				if (unparsedChar < 2)
				{
					ok = false;
					break;
				}
				x += rgchProp[x+2];
				break;
			}
			//throw libwps::ParseException();
			break;
		}
		if (!ok)
		{
			WPS_DEBUG_MSG(("WPS8Parser::propertyChangePara: problem with field size, stop\n"));
			break;
		}
	}

	if (listLevel != -1)
	{
		if (listLevel)
		{
			if (!m_listener->getCurrentList())
				m_listener->setCurrentList(shared_ptr<WPSList>(new WPSList));
			m_listener->getCurrentList()->set(1, level);
		}
		m_listener->setCurrentListLevel(listLevel);
	}
	m_listener->setParagraphJustification(align);
	m_listener->setParagraphTextIndent(textIndent);
	m_listener->setParagraphMargin(leftIndent, WPS_LEFT);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
