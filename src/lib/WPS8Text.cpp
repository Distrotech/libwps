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

#include <map>

#include "libwps_internal.h"

#include "WPSContentListener.h"
#include "WPSList.h"
#include "WPSPageSpan.h"
#include "WPSParagraph.h"
#include "WPSSubDocument.h"

#include "WPS.h"

#include "WPS8.h"
#include "WPS8Struct.h"

#include "WPS8Text.h"

namespace WPS8TextInternal
{
//! Internal: the subdocument of a WPS8Text
class SubDocument : public WPSSubDocument
{
public:
	//! type of an entry stored in textId
	enum Type { Unknown, Footnote, Endnote };

	//! constructor for a text entry
	SubDocument(WPXInputStreamPtr input, WPS8Text &pars, Type type, int i) :
		WPSSubDocument (input, 0, i), m_type(type), m_textParser(&pars) {}
	//! destructor
	~SubDocument() {}

	//! operator==
	virtual bool operator==(WPSSubDocumentPtr const &doc) const
	{
		if (!WPSSubDocument::operator==(doc))
			return false;
		SubDocument const *sDoc = dynamic_cast<SubDocument const *>(doc.get());
		if (!sDoc) return false;
		if (m_type != sDoc->m_type) return false;
		if (m_textParser != sDoc->m_textParser) return false;
		return true;
	}

	//! the parser function
	void parse(WPSContentListenerPtr &listener, libwps::SubDocumentType type);

	Type m_type;
	WPS8Text *m_textParser;
private:
	SubDocument(SubDocument const &orig);
	SubDocument &operator=(SubDocument const &orig);
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

	if (!m_textParser)
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
	if (type == libwps::DOC_NOTE)
		m_textParser->sendNote(m_input, m_id, m_type == Endnote);
	else
	{
		WPS_DEBUG_MSG(("SubDocument::parse: find unknown type of document...\n"));
	}
	m_input->seek(actPos, WPX_SEEK_SET);
}

}

/*
WPS8Text public
*/

WPS8Text::WPS8Text(WPS8Parser &parser) : WPSTextParser(parser, parser.getInput()),
	m_listener(),
	m_offset_eot(0),
	m_oldTextAttributeBits(0),
	m_CHFODs(),
	m_PAFODs(),
	m_fontNames(),
	m_streams(),
	m_footnotes(), m_actualFootnote(0),
	m_endnotes(), m_actualEndnote(0)
{
}

WPS8Text::~WPS8Text ()
{
}

void WPS8Text::parse(WPXDocumentInterface *documentInterface)
{

	WPS_DEBUG_MSG(("WPS8Text::parse()\n"));
	if (!m_input)
	{
		WPS_DEBUG_MSG(("WPS8Text::parse: does not find main ole\n"));
		throw(libwps::ParseException());
	}

	try
	{
		/* parse pages */
		std::vector<WPSPageSpan> pageList;
		parsePages(pageList, m_input);

		/* parse document */
		m_listener.reset(new WPS8ContentListener(pageList, documentInterface));
		parse(m_input);
		m_listener.reset();
	}
	catch (...)
	{
		WPS_DEBUG_MSG(("WPS8Text::parse: exception catched when parsing CONTENTS\n"));
		throw(libwps::ParseException());
	}
}


/*
WPS8Text private
*/


////////////////////////////////////////////////////////////
// the fontname:
////////////////////////////////////////////////////////////
bool WPS8Text::readFontNames(WPSEntry const &entry)
{
	WPXInputStreamPtr input = getInput();
	if (!entry.hasType(entry.name()))
	{
		WPS_DEBUG_MSG(("WPS8Text::readFonts: FONT name=%s, type=%s\n",
		               entry.name().c_str(), entry.type().c_str()));
		return false;
	}

	if (entry.length() < 20)
	{
		WPS_DEBUG_MSG(("WPS8Text::readFonts: FONT length=0x%ld\n", entry.length()));
		return false;
	}

	long debPos = entry.begin();
	input->seek(debPos, WPX_SEEK_SET);

	long len = libwps::readU32(input); // len + 0x14 = size
	size_t n_fonts = (size_t) libwps::readU32(input);

	if (long(4*n_fonts) > len)
	{
		WPS_DEBUG_MSG(("WPS8Text::readFonts: FONT number=%d\n", int(n_fonts)));
		return false;
	}
	libwps::DebugStream f;

	entry.setParsed();
	f << "N=" << n_fonts;
	if (len+20 != entry.length()) f << ", ###L=" << std::hex << len+0x14;

	f << ", unkn=(" << std::hex;
	for (int i = 0; i < 3; i++) f << libwps::readU32(input) << ", ";
	f << "), dec=[";
	for (int i = 0; i < int(n_fonts); i++) f << ", " << libwps::read32(input);
	f << "]" << std::dec;

	ascii().addPos(debPos);
	ascii().addNote(f.str().c_str());

	long pageEnd = entry.end();

	/* read each font in the table */
	while (input->tell() > 0 && m_fontNames.size() < n_fonts)
	{
		debPos = input->tell();
		if (debPos+6 > long(pageEnd)) break;

		int string_size = (int) libwps::readU16(input);
		if (debPos+2*string_size+6 > long(pageEnd)) break;

		std::string s;
		for (; string_size>0; string_size--)
			s.append(1, (char) libwps::readU16(input));

		f.str("");
		f << "FONT("<<m_fontNames.size()<<"): " << s;
		f << ", unkn=(";
		for (int i = 0; i < 4; i++) f << (int) libwps::read8(input) << ", ";
		f << ")";
		ascii().addPos(debPos);
		ascii().addNote(f.str().c_str());

		m_fontNames.push_back(s);
	}

	if (m_fontNames.size() != n_fonts)
	{
		WPS_DEBUG_MSG(("WPS8Text::readFonts: expected %i fonts but only found %i\n",
		               int(n_fonts), int(m_fontNames.size())));
	}
	return true;
}

/**
 * Reads streams (subdocuments) information
 */

void WPS8Text::readStreams(WPXInputStreamPtr &input)
{
	WPS8Parser::NameMultiMap::iterator pos;
	pos = mainParser().getNameEntryMap().lower_bound("STRS");
	if (mainParser().getNameEntryMap().end() == pos)
	{
		WPS_DEBUG_MSG(("Works8: error: no STRS in header index table\n"));
		throw libwps::ParseException();
	}
	WPSEntry const &entry = pos->second;
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
			if (bodypos < 0) bodypos = int(k);
			else WPS_DEBUG_MSG(("Duplicating body (strm[%d])\n",k));
		}
	}
	if (bodypos < 0) WPS_DEBUG_MSG(("Doc body not found!\n"));
#endif
}

void WPS8Text::readNotes(std::vector<Note> &dest, WPXInputStreamPtr &input, const char *key)
{
	WPS8Parser::NameMultiMap::iterator pos;
	pos = mainParser().getNameEntryMap().lower_bound(key);
	if (mainParser().getNameEntryMap().end() == pos)
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

		while (++pos != mainParser().getNameEntryMap().end())
		{
			if (!strcmp(pos->first.c_str(),key)) break;
		}
	}
	while (pos != mainParser().getNameEntryMap().end());
	/* some kind of loop needed */
}

/**
 * Read an UTF16 character in LE byte ordering, convert it
 * and append it to the text buffer as UTF8. Courtesy of glib2
 *
 */

#define SURROGATE_VALUE(h,l) (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)

void WPS8Text::appendUTF16LE(WPXInputStreamPtr &input)
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

void WPS8Text::readTextRange(WPXInputStreamPtr &input,
                             uint32_t startpos, uint32_t endpos, uint16_t stream)
{
	WPS_DEBUG_MSG(("WPS8Text::readTextRange(stream=%d)\n",stream));

	std::vector<WPSFOD>::iterator FODs_iter;
	std::vector<WPSFOD>::iterator PFOD_iter;

	// save old text attribute
	uint32_t oldTextAttributes = m_oldTextAttributeBits;

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


		/* process character formatting */
		if ((*FODs_iter).m_bfprop!=0)
			propertyChange((*FODs_iter).m_fprop, specialCode, fieldType);

		/* loop until character format not exhausted */
		do
		{
			/* paragraph format may change here*/
			if ((*PFOD_iter).m_bfprop!=0)
				propertyChangePara((*PFOD_iter).m_fprop);

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
							(new WPS8TextInternal::SubDocument(input, *this, WPS8TextInternal::SubDocument::Footnote, m_actualFootnote++));
							m_listener->insertNote(WPSContentListener::FOOTNOTE, doc);
							break;
						}
						case 4:
						{
							if (stream != Stream::Z_Body) break;
							shared_ptr<WPSSubDocument> doc
							(new WPS8TextInternal::SubDocument(input, *this, WPS8TextInternal::SubDocument::Endnote, m_actualEndnote++));
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
						WPS_DEBUG_MSG(("WPS8Text::readTextRange(find unprintable character: ignored)\n"));
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
	m_oldTextAttributeBits = oldTextAttributes;
}

void WPS8Text::sendNote(WPXInputStreamPtr &input, int id, bool is_endnote)
{
	std::vector<Note> const &notes = is_endnote ? m_endnotes : m_footnotes;
	if (id < 0 || id >= int(notes.size()))
	{
		WPS_DEBUG_MSG(("WPS8Text::sendNote: can not find footnote\n"));
		if (m_listener) m_listener->insertCharacter(' ');
		return;
	}
	Note const &note =notes[size_t(id)];
	Stream stream;
	Stream::Type streamkey = is_endnote ? Stream::Z_Endnotes : Stream::Z_Footnotes;
	for (size_t i=0; i<m_streams.size(); i++)
	{
		if (m_streams[i].m_type == streamkey)
		{
			stream = m_streams[i];
			break;
		}
	}

	WPS_DEBUG_MSG(("Reading footnote [%ld;%ld)\n",note.begin(),note.end()));

	long pos = input->tell();
	long beginPos = stream.begin()+note.begin();
	long endPos = stream.begin()+note.end();
	// try to remove the end of lines which can appear after the footnote
	while (endPos-1 > beginPos)
	{
		input->seek(0x200+2*(endPos-1),WPX_SEEK_SET);
		uint16_t readVal =libwps::readU16(input);
		if (readVal != 0xd) break;
		endPos -= 1;
	}
	readTextRange(input,(uint32_t)beginPos,(uint32_t)endPos,streamkey);
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

bool WPS8Text::readFODPage(WPXInputStreamPtr &input, std::vector<WPSFOD> &FODs, uint16_t page_size)
{
	uint32_t page_offset = (uint32_t) input->tell();
	uint16_t cfod = libwps::readU16(input); /* number of FODs on this page */

	//fixme: what is the largest possible cfod?
	if (cfod > 0x54)
	{
		WPS_DEBUG_MSG(("Works8: error: cfod = %i (0x%X)\n", cfod, cfod));
		throw libwps::ParseException();
	}

	input->seek(page_offset + 2 + 6, WPX_SEEK_SET);	// fixme: unknown

	int first_fod = int(FODs.size());

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
			continue;
		long actPos = (*FODs_iter).m_bfpropAbs;
		input->seek(actPos, WPX_SEEK_SET);

		long size = (long) libwps::readU16(input);
		if (actPos+size > long(page_offset+page_size) || size < 2)
		{
			WPS_DEBUG_MSG(("WPS8Text::readFODPage: error: size = %ld is very odd\n", size));
			continue;
		}

		std::string error;
		readBlockData(input, actPos+size, (*FODs_iter).m_fprop, error);

		libwps::DebugStream f;
		f << "PLC=" << (*FODs_iter).m_fprop;
		ascii().addPos(actPos);
		ascii().addNote(f.str().c_str());
		ascii().addPos(actPos+size);
		ascii().addNote("_");
	}

	/* go to end of page */
	input->seek(page_offset	+ page_size, WPX_SEEK_SET);

	return (m_offset_eot > FODs.back().m_fcLim);
}

/**
 * Read the page format from the file.  It seems that WPS8Text files
 * can only have one page format throughout the whole document.
 *
 */
void WPS8Text::parsePages(std::vector<WPSPageSpan> &pageList, WPXInputStreamPtr & /* input */)
{
	//fixme: this method doesn't do much

	/* record page format */
	WPSPageSpan ps;
	pageList.push_back(ps);
}

void WPS8Text::parse(WPXInputStreamPtr &input)
{
	WPS_DEBUG_MSG(("WPS8Text::parse()\n"));

	m_listener->startDocument();
	WPS8Parser::NameMultiMap &nameTable = mainParser().getNameEntryMap();
	WPS8Parser::NameMultiMap::iterator pos;
	/* What is the total length of the text? */
	pos = nameTable.lower_bound("TEXT");
	if (nameTable.end() == pos)
	{
		WPS_DEBUG_MSG(("Works: error: no TEXT in header index table\n"));
	}
	else
	{
		m_offset_eot = (u_int32_t) pos->second.end();
		WPS_DEBUG_MSG(("Works: debug: TEXT m_offset_eot = 0x%04X\n", m_offset_eot));
	}

	/* read character/para FODs (FOrmatting Descriptors) */
	for (int wh = 0; wh < 2; wh++)
	{
		char const *name = wh==0 ? "FDPC" : "FDPP";
		for (pos = nameTable.begin(); pos != nameTable.end(); ++pos)
		{
			if (0 != strcmp(name,pos->first.c_str()))
				continue;

			WPSEntry const &entry = pos->second;
			input->seek(entry.begin(), WPX_SEEK_SET);
			if (entry.length() != 512)
			{
				WPS_DEBUG_MSG(("Works: warning: %s offset=0x%lX, length=0x%lX\n",
				               name,entry.begin(), entry.length()));
			}
			if (wh==0)
				readFODPage(input, m_CHFODs, (uint16_t)entry.length());
			else
				readFODPage(input, m_PAFODs, (uint16_t)entry.length());
		}
	}

	/* read streams table*/
	readStreams(input);

	/* read fonts table */
	pos = nameTable.find("FONT");
	if (nameTable.end() == pos)
	{
		WPS_DEBUG_MSG(("WPS8Text::parse: error: no FONT in header index table\n"));
		throw libwps::ParseException();
	}
	readFontNames(pos->second);

	readNotes(m_footnotes,input,"FTN ");
	readNotes(m_endnotes,input,"EDN ");

	m_actualFootnote = m_actualEndnote = 0;

	if (m_offset_eot < 0x200)
		m_offset_eot = 0x200;

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
			if ((uint32_t)m_streams[i].begin() < doc_end)
				doc_end = (uint32_t) m_streams[i].begin();
			if ((uint32_t)m_streams[i].end() > doc_start2)
				doc_start2 = (uint32_t) m_streams[i].end();
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
void WPS8Text::propertyChangeDelta(uint32_t newTextAttributeBits)
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
			WPS_DEBUG_MSG(("WPS8Text::propertyChangeDelta: attribute %i changed, now = %i\n", i, newTextAttributeBits & listAttributes[i]));
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
void WPS8Text::propertyChange(WPS8Struct::FileData const &mainData, uint16_t &specialCode, int &fieldType)
{
	/* set default properties */
	uint32_t textAttributeBits = 0;
	m_listener->setTextColor(0);
	propertyChangeDelta(0);
	m_listener->setFontSize(10);

	libwps::DebugStream f;
	if (mainData.m_value) f << "unk=" << mainData.m_value << ",";

	/* move the map in state to be ok */
	static const int expextedTypes[] =
	{
		0, 0x12, 0x2, 0xA, 0x3, 0xA, 0x4, 0xA, 0x5, 0xA,
		0xc, 0x22, 0xf, 0x12,
		0x10, 0xA, 0x12, 0x22, 0x13, 0xA, 0x14, 0xA, /*0x15, 0xA,*/ 0x16, 0xA, 0x17, 0xA,
		/*0x18, 0x22,*/ 0x1e, 0x12,
		0x22, 0x22, /*0x23, 0x22,*/ 0x24, 0x8A,
		0x2e, 0x22,
	};
	static bool mapInit = false;
	static std::map<int, int> expextedTypesMap; // map id->type
	if (!mapInit)
	{
		int numData = sizeof(expextedTypes)/sizeof(int);
		for (int i = 0; i+1 < numData; i+=2)
			expextedTypesMap[expextedTypes[i]] = expextedTypes[i+1];
		mapInit = true;
	}
	for (size_t c = 0; c < mainData.m_recursData.size(); c++)
	{
		WPS8Struct::FileData const &data = mainData.m_recursData[c];
		if (data.isBad()) continue;
		if (expextedTypesMap.find(data.id())==expextedTypesMap.end())
		{
			f << data << ",";
			continue;
		}
		if (expextedTypesMap.find(data.id())->second != data.type())
		{
			WPS_DEBUG_MSG(("WPS8Text::propertyChange: unexpected type for %d\n", data.id()));
			f << "###" << data << ",";
			continue;
		}
		switch(data.id())
		{
		case 0x0:
			specialCode = (uint16_t) data.m_value;
			break;
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
		case 0x0c:
			m_listener->setFontSize(uint16_t(data.m_value/12700));
			break;
		case 0x0F:
			if ((data.m_value&0xFF) == 1) textAttributeBits |= WPS_SUPERSCRIPT_BIT;
			else if ((data.m_value&0xFF) == 2) textAttributeBits |= WPS_SUBSCRIPT_BIT;
			else f << "###ff=" << std::hex << data.m_value << std::dec << ",";
			break;
		case 0x10:
			textAttributeBits |= WPS_STRIKEOUT_BIT;
			break;
		case 0x12:
			m_listener->setTextLanguage(int(data.m_value));
			break;
		case 0x13:
			textAttributeBits |= WPS_SMALL_CAPS_BIT;
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
			// fixme: there are various styles of underline
		case 0x1e:
			textAttributeBits |= WPS_UNDERLINE_BIT;
			break;
		case 0x22:
			fieldType = int(data.m_value);
			break;
		case 0x24:
		{
			if ((!data.isRead() && !data.readArrayBlock() && data.m_recursData.size() == 0) ||
			        !data.isArray())
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChange: can not read font array\n"));
				f << "###fontPb";
				break;
			}

			size_t nChild = data.m_recursData.size();
			if (!nChild || data.m_recursData[0].isBad() || data.m_recursData[0].type() != 0x18)
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChange: can not read font id\n"));
				f << "###fontPb";
				break;
			}
			uint8_t fId = (uint8_t)data.m_recursData[0].m_value;
			if (fId < m_fontNames.size())
				m_listener->setTextFont(m_fontNames[fId].c_str());
			else
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChange: can not read find font %d\n", int(fId)));
			}
			std::vector<int> formats;
			for (size_t i = 0; i < nChild; i++)
			{
				WPS8Struct::FileData const &subD = data.m_recursData[i];
				if (subD.isBad()) continue;
				int formId = subD.id() >> 3;
				int sId = subD.id() & 0x7;
				if (sId == 0)
				{
					formats.resize(size_t(formId)+1,-1);
					formats[size_t(formId)] = int(subD.m_value);
				}
				else
					f << "###formats"<<formId<<"." << sId << "=" << data.m_recursData[i] << ",";
			}
			f << "formats=[" << std::hex;
			for (size_t i = 0; i < formats.size(); i++)
			{
				if (formats[i] != -1)
					f << "f" << i << "=" << formats[i] << ",";
			}
			f << "],";
			break;
		}
		case 0x2e:
		{
			uint32_t col = (uint32_t) (data.m_value&0xFFFFFF);
			m_listener->setTextColor((col>>16)|(col&0xFF00)|((col&0xFF)<<16));;
			if (data.m_value &0xFF000000) f << "#f2e=" << std::hex << data.m_value << std::dec;
			break;
		}
		default:
			WPS_DEBUG_MSG(("WPS8Text::propertyChange: unexpected %d\n", data.id()));
			f << "###" << data << ",";
			break;
		}
	}

	propertyChangeDelta(textAttributeBits);
}

void WPS8Text::propertyChangePara(WPS8Struct::FileData const &mainData)
{
	static const libwps::Justification _align[]=
	{
		libwps::JustificationLeft, libwps::JustificationRight,
		libwps::JustificationCenter, libwps::JustificationFull
	};
	libwps::Justification align = libwps::JustificationLeft;

	std::vector<WPSTabStop> tabList;
	m_listener->setTabs(tabList);

	libwps::DebugStream f;
	if (mainData.m_value) f << "unk=" << mainData.m_value << ",";

	/* move the map in state to be ok */
	static const int expextedTypes[] =
	{
		0x3, 0x1A, 0x4, 0x12,
		0xc, 0x22, 0xd, 0x22, /* 0xe, 0x22, */
		/* 0x12, 0x22, 0x13, 0x22, */ 0x14, 0x22,
		0x32, 0x82,
	};
	static bool mapInit = false;
	static std::map<int, int> expextedTypesMap; // map id->type
	if (!mapInit)
	{
		int numData = sizeof(expextedTypes)/sizeof(int);
		for (int i = 0; i+1 < numData; i+=2)
			expextedTypesMap[expextedTypes[i]] = expextedTypes[i+1];
		mapInit = true;
	}

	float leftIndent=0.0, textIndent=0.0;
	int listLevel = 0;
	WPSList::Level level;
	for (size_t c = 0; c < mainData.m_recursData.size(); c++)
	{
		WPS8Struct::FileData const &data = mainData.m_recursData[c];
		if (data.isBad()) continue;
		if (expextedTypesMap.find(data.id())==expextedTypesMap.end())
		{
			f << data << ",";
			continue;
		}
		if (expextedTypesMap.find(data.id())->second != data.type())
		{
			WPS_DEBUG_MSG(("WPS8Text::propertyChangePara: unexpected type for %d\n", data.id()));
			f << "###" << data << ",";
			continue;
		}

		switch(data.id())
		{
			/* paragraph has a bullet specified in num format*/
		case 0x3:
			level.m_type = libwps::BULLET;
			level.m_bullet = "*";
			listLevel = 1;
			break;
		case 0x04:
			if (data.m_value >= 0 && data.m_value < 4) align = _align[data.m_value];
			else f << "###align=" << data.m_value << ",";
			break;
		case 0x0C:
			textIndent=float(data.m_value)/914400.0f;
			break;
		case 0x0D:
			leftIndent=float(data.m_value)/914400.0f;
			break;
			/* 0x0E: right/635, 0x12: before/635, 0x13: after/635 */
		case 0x14:
		{
			/* numbering style */
			int oldListLevel = listLevel;
			listLevel = 1;
			switch(data.m_value & 0xFFFF)
			{
			case 0: // checkme
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara: Find list flag=0\n"));
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
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara style %04lx\n",(data.m_value & 0xFFFF)));
				break;
			}
			if (listLevel == -1)
				listLevel = oldListLevel;
			else
				level.m_suffix=((data.m_value>>16) == 2) ? "." : ")";
		}
		break;
		case 0x32:
		{
			if (!data.isRead() && !data.readArrayBlock() && data.m_recursData.size() == 0)
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara can not find tabs array\n"));
				f << "###tabs,";
				break;
			}
			size_t nChild = data.m_recursData.size();
			if (nChild < 1 ||
			        data.m_recursData[0].isBad() || data.m_recursData[0].id() != 0x27)
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara can not find first child\n"));
				f << "###tabs,";
				break;
			}
			if (nChild == 1) break;

			int numTabs = int(data.m_recursData[0].m_value);
			if (numTabs == 0 || nChild < 2 ||
			        data.m_recursData[1].isBad() || data.m_recursData[1].id() != 0x28)
			{
				WPS_DEBUG_MSG(("WPS8Text::propertyChangePara can not find second child\n"));
				f << "###tabs,";
				break;
			}

			WPS8Struct::FileData const &mData = data.m_recursData[1];
			size_t lastParsed = 0;
			if (mData.id() == 0x28 && mData.isArray() &&
			        (mData.isRead() || mData.readArrayBlock() || mData.m_recursData.size() != 0))
			{
				lastParsed = 1;
				size_t nTabsChilds = mData.m_recursData.size();
				int actTab = 0;
				tabList.resize(size_t(numTabs));

				for (size_t i = 0; i < nTabsChilds; i++)
				{
					if (mData.m_recursData[i].isBad()) continue;
					int value = mData.m_recursData[i].id();
					int wTab = value/8;
					int what = value%8;

					// the first tab can be skipped
					// so this may happens only one time
					if (wTab > actTab && actTab < numTabs)
					{
						tabList[size_t(actTab)].m_alignment = WPSTabStop::LEFT;
						tabList[size_t(actTab)].m_position =  0.;

						actTab++;
					}

					if (mData.m_recursData[i].isNumber() && wTab==actTab && what == 0
					        && actTab < numTabs)
					{
						tabList[size_t(actTab)].m_alignment = WPSTabStop::LEFT;
						tabList[size_t(actTab)].m_position =  float(mData.m_recursData[i].m_value)/914400.f;

						actTab++;
						continue;
					}
					if (mData.m_recursData[i].isNumber() && wTab == actTab-1 && what == 1)
					{
						int actVal = int(mData.m_recursData[i].m_value);
						switch((actVal & 0x3))
						{
						case 0:
							tabList[size_t(actTab-1)].m_alignment = WPSTabStop::LEFT;
							break;
						case 1:
							tabList[size_t(actTab-1)].m_alignment = WPSTabStop::RIGHT;
							break;
						case 2:
							tabList[size_t(actTab-1)].m_alignment = WPSTabStop::CENTER;
							break;
						case 3:
							tabList[size_t(actTab-1)].m_alignment = WPSTabStop::DECIMAL;
							break;
						default:
							break;
						}
						if (actVal&0xC)
							f << "###tabFl" << actTab<<":low=" << (actVal&0xC) << ",";
						actVal = (actVal>>8);
						/* not frequent:
						   but fl1:high=db[C], fl2:high=b7[R] appear relatively often*/
						if (actVal)
							f << "###tabFl" << actTab<<":high=" << actVal << ",";
						continue;
					}
					if (mData.m_recursData[i].isNumber() && wTab == actTab-1 && what == 2)
					{
						// tabList[actTab-1].m_leaderCharacter = mData.m_recursData[i].m_value;
						continue;
					}
					f << "###tabData:fl" << actTab << "=" << mData.m_recursData[i] << ",";
				}
				if (actTab != numTabs)
				{
					f << "NTabs[###founds]="<<actTab << ",";
					tabList.resize(size_t(actTab));
				}
			}
			for (size_t ch =lastParsed+1; ch < nChild; ch++)
			{
				if (data.m_recursData[ch].isBad()) continue;
				f << "extra[tabs]=[" << data.m_recursData[ch] << "],";
			}
			m_listener->setTabs(tabList);
		}
		break;
		default:
			WPS_DEBUG_MSG(("WPS8Text::propertyChangePara: unexpected %d\n", data.id()));
			f << "###" << data << ",";
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
