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

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "WPS8.h"
#include "WPSDocument.h"
#include "libwps_internal.h"


#define WPS8_PAGES_HEADER_OFFSET 0x22

/*
WPS8Parser public
*/


WPS8Parser::WPS8Parser(WPXInputStream *input, WPSHeader * header) :
	WPSParser(input, header),
	offset_eot(0),
	oldTextAttributeBits(0)
{
}

WPS8Parser::~WPS8Parser ()
{
}

void WPS8Parser::parse(WPXDocumentInterface *documentInterface)
{
	std::list<WPSPageSpan> pageList;
	
	WPS_DEBUG_MSG(("WPS8Parser::parse()\n"));		

	WPXInputStream *input = getInput();		

	/* parse pages */
	WPSPageSpan m_currentPage;
	parsePages(pageList, input);		
	
	/* parse document */
	WPS8ContentListener listener(pageList, documentInterface);
	parse(input, &listener);
}


/*
WPS8Parser private
*/


/**
 * Reads fonts table into memory.
 *
 */
void WPS8Parser::readFontsTable(WPXInputStream * input)
{
	/* find the fonts page offset, fonts array offset, and ending offset */
	HeaderIndexMultiMap::iterator pos;
	pos = headerIndexTable.lower_bound("FONT");
	if (headerIndexTable.end() == pos)
	{
		WPS_DEBUG_MSG(("Works8: error: no FONT in header index table\n"));
		throw ParseException();
	}
	input->seek(pos->second.offset + 0x04, WPX_SEEK_SET);
	uint32_t n_fonts = readU32(input);
	input->seek(pos->second.offset + 0x10 + (4*n_fonts), WPX_SEEK_SET);
	uint32_t offset_end_FFNTB = pos->second.offset + pos->second.length;
	
	/* read each font in the table */	
	while (input->tell() > 0 && (unsigned long)(input->tell()+8) < offset_end_FFNTB && fonts.size() < n_fonts)
	{
#ifdef DEBUG
		uint32_t unknown = readU32(input);
#else
		input->seek(4, WPX_SEEK_CUR);
#endif
		uint16_t string_size = readU16(input);
		
		std::string s;
		for (; string_size>0; string_size--)
			s.append(1, (uint16_t)readU16(input));
		s.append(1, (char)0);
		if (s.empty())
			continue;
		WPS_DEBUG_MSG(("Works: debug: unknown={0x%08X}, name=%s\n",
			 unknown, s.c_str()));
		fonts.push_back(s);
	}

	if (fonts.size() != n_fonts)
	{
		WPS_DEBUG_MSG(("Works: warning: expected %i fonts but only found %i\n",
			n_fonts, int(fonts.size())));
	}
}

/**
 * Reads streams (subdocuments) information
 */

void WPS8Parser::readStreams(WPXInputStream * input)
{
	HeaderIndexMultiMap::iterator pos;
	pos = headerIndexTable.lower_bound("STRS");
	if (headerIndexTable.end() == pos)
	{
		WPS_DEBUG_MSG(("Works8: error: no STRS in header index table\n"));
		throw ParseException();
	}

	uint32_t last_pos = 0;

	uint32_t n_streams;
	input->seek(pos->second.offset, WPX_SEEK_SET);
	n_streams = readU32(input);

	if (n_streams > 100) { WPS_DEBUG_MSG(("Probably garbled STRS: count = %u\n",n_streams)); }

	/* skip mysterious header*/
	input->seek(8, WPX_SEEK_CUR);

	WPSStream s;
	uint32_t offset;
	//uint32_t *offsets = (uint32_t*) malloc(4 * n_streams + 4);
	for (unsigned i=0; i < n_streams; i++) {
		offset = readU32(input);
		// TODO: assert index in within text
		s.span.start = last_pos;
		s.span.limit = last_pos + offset;
		s.type = WPS_STREAM_DUMMY;
		streams.push_back(s);

		last_pos += offset;
	}
	offset = readU32(input);
	if (offset) { WPS_DEBUG_MSG(("Offset table is not 0-terminated!\n")); }

	for (unsigned i=0; i < n_streams; i++) {
		uint16_t len;
		uint32_t type = 0;

		len = readU16(input);
		if (len > 10) {
			WPS_DEBUG_MSG(("Rogue strm[%d] def len (%d)\n",i,len));
			input->seek(len-2,WPX_SEEK_CUR);
		}

		if (len > 4) {
			readU32(input); // assume == 0x22000000
			type = readU32(input);
		} else input->seek(len-2,WPX_SEEK_CUR);

		streams[i].type = type;
	}

#ifdef DEBUG
	int bodypos = -1;
	for (unsigned i=0; i < n_streams; i++) {
		int z = streams[i].type;
		if (z == WPS_STREAM_DUMMY) WPS_DEBUG_MSG(("Default strm[%d] type\n",i));
		if (z == WPS_STREAM_BODY) {
			if (bodypos < 0) bodypos = i;
			else WPS_DEBUG_MSG(("Duplicating body (strm[%d])\n",i));
		}
	}
	if (bodypos < 0) WPS_DEBUG_MSG(("Doc body not found!\n"));
#endif
}

void WPS8Parser::readNotes(std::vector<WPSNote> &dest, WPXInputStream *input, const char *key)
{
	HeaderIndexMultiMap::iterator pos;
	pos = headerIndexTable.lower_bound(key);
	if (headerIndexTable.end() == pos)
		return;

	pos->second.length;

	uint32_t unk1;
	uint32_t count;
	uint32_t boff;
	uint32_t last;

	WPSNote n;

	do {
		input->seek(pos->second.offset,WPX_SEEK_SET);
		unk1 = readU32(input);
		count = readU32(input);
		input->seek(8, WPX_SEEK_CUR);
		last = 0;

		for (unsigned i=0; i < count; i++) {
			boff = readU32(input);
			if (unk1) {
				/* first table with body positions */
				n.offset = boff; // to characters!
				if (dest.size() <= i) dest.push_back(n);
			} else {
				if (i>0) dest[i-1].contents.limit=boff;
				n=dest[i];
				n.contents.start = boff;
				n.contents.limit = last;
				dest[i]=n;
				last = n.contents.start;
			}
		}
		boff = readU32(input);
		if (!unk1 && dest.size()>0) dest[dest.size()-1].contents.limit=boff;

		while (++pos != headerIndexTable.end()) {
			if (!strcmp(pos->first.c_str(),key)) break;
		}
	} while (pos != headerIndexTable.end());

	/* some kind of loop needed*/
}

/**
 * Read an UTF16 character in LE byte ordering, convert it
 * and append it to the text buffer as UTF8. Courtesy of glib2
 *
 */

#define SURROGATE_VALUE(h,l) (((h) - 0xd800) * 0x400 + (l) - 0xdc00 + 0x10000)

void WPS8Parser::appendUTF16LE(WPXInputStream *input, WPS8ContentListener *listener)
{
	uint16_t high_surrogate = 0;
	bool fail = false;
	uint16_t readVal;
	uint32_t ucs4Character;
	while (true) {
		if (input->atEOS()) {
			fail = true;
			break;
		}
		readVal = readU16(input);
		if (readVal >= 0xdc00 && readVal < 0xe000) /* low surrogate */ {
			if (high_surrogate) {
				ucs4Character = SURROGATE_VALUE(high_surrogate, readVal);
				high_surrogate = 0;
				break;
			}
			else {
				fail = true;
				break;
			}
		}
		else {
			if (high_surrogate) {
				fail = true;
				break;
			}
			if (readVal >= 0xd800 && readVal < 0xdc00) /* high surrogate */ {
				high_surrogate = readVal;
			}
			else {
				ucs4Character = readVal;
				break;
			}
		}
	}
	if (fail)
		throw GenericException();

	uint8_t first;
	int len;
	if (ucs4Character < 0x80) {
		first = 0; len = 1;
	}
	else if (ucs4Character < 0x800) {
		first = 0xc0; len = 2;
	}
	else if (ucs4Character < 0x10000) {
		first = 0xe0; len = 3;
	}
	else if (ucs4Character < 0x200000) {
		first = 0xf0; len = 4;
	}
	else if (ucs4Character < 0x4000000) {
		first = 0xf8; len = 5;
	}
	else {
		first = 0xfc; len = 6;
	}

	uint8_t outbuf[6] = { 0, 0, 0, 0, 0, 0 };
	int i;
	for (i = len - 1; i > 0; --i) {
		outbuf[i] = (ucs4Character & 0x3f) | 0x80;
		ucs4Character >>= 6;
	}
	outbuf[0] = ucs4Character | first;

	for (i = 0; i < len; i++)
		listener->insertCharacter(outbuf[i]);
}

/**
 * Read the text of the document using previously-read
 * formatting information.
 *
 */

void WPS8Parser::readText(WPXInputStream * /* input */, WPS8ContentListener * /* listener */)
{
#if (0)
	WPS_DEBUG_MSG(("WPS8Parser::readText()\n"));

	std::vector<FOD>::iterator FODs_iter;
	std::vector<FOD>::iterator PFOD_iter;

	uint32_t last_fcLim = 0x200; //fixme: start of text might vary according to header index table?
	PFOD_iter = PAFODs.begin();
	for (FODs_iter = CHFODs.begin(); FODs_iter!= CHFODs.end(); FODs_iter++)
	{
		FOD fod = *(FODs_iter);
		uint32_t c_len = (*FODs_iter).fcLim - last_fcLim;
		uint32_t p_len = (*PFOD_iter).fcLim - last_fcLim;

		uint32_t len = (c_len > p_len)? p_len : c_len;

		if (len % 2 != 0)
		{
			WPS_DEBUG_MSG(("Works: error: len %i is odd\n", len));
			throw ParseException();
		}
		len /= 2;

		/* print rgchProp as hex bytes */
		WPS_DEBUG_MSG(("rgch="));
		for (unsigned int blah=0; blah < (*FODs_iter).fprop.rgchProp.length(); blah++)
		{
			WPS_DEBUG_MSG(("%02X ", (uint8_t) (*FODs_iter).fprop.rgchProp[blah]));
		}
		WPS_DEBUG_MSG(("\n"));

		/* process character formatting */
		if ((*FODs_iter).fprop.cch > 0)
			propertyChange((*FODs_iter).fprop.rgchProp, listener);

		/* loop until character format not exhausted */
		do {
			/* paragraph format may change here*/

			if ((*PFOD_iter).fprop.cch > 0)
				propertyChangePara((*PFOD_iter).fprop.rgchProp, listener);

			/* plain text */
			input->seek(last_fcLim, WPX_SEEK_SET);
			for (uint32_t i = len; i>0; i--)
			{
				uint16_t readVal = readU16(input);

				if (0x00 == readVal)
					break;
					
				switch (readVal)
				{
					case 0x0A:
						break;

					case 0x0C:
						//fixme: add a page to list of pages
						//listener->insertBreak(WPS_PAGE_BREAK);
						break;

					case 0x0D:
						listener->insertEOL();
						break;

					case 0x0E:
						listener->insertBreak(WPS_COLUMN_BREAK);
						break;

					case 0x1E:
						//fixme: non-breaking hyphen
						break;

					case 0x1F:
						//fixme: optional breaking hyphen
						break;

					case 0x23:
						if (uint16_t spec = listener->getSpec()) {
						//	TODO: fields, pictures, etc.
							switch (spec) {
								case 3:
#if 0
								  /* REMOVED in this version
								     because footnote/endnote tends to make
								     the resulting file inconsistent */
									listener->openFootnote();
									propertyChange("",listener);
									listener->insertCharacter('F');
									listener->insertCharacter('!');
									listener->closeFootnote();
#endif
									break;
								case 4:
#if 0
								  /* REMOVED in this version
								     because footnote/endnote tends to make
								     the resulting file inconsistent */
									listener->openEndnote();
									propertyChange("",listener);
									listener->insertCharacter(0xE2/*0x263B*/);
									listener->insertCharacter(0x98);
									listener->insertCharacter(0xBA);
									listener->closeEndnote();
#endif
									break;
								case 5:
									/*switch (<field code>) {
									case -1:
										listener->insertField(WPS_FIELD_PAGE);
										break;
									default:*/
										listener->insertField();
									/*}*/
									break;
								default:
									listener->insertCharacter(0xE2/*0x263B*/);
									listener->insertCharacter(0x98);
									listener->insertCharacter(0xBB);
							}
							// listener->insertSpecial();
							break;
						}
						// ! fallback to default

					case 0xfffc:
						// ! fallback to default

					default:
						// fixme: convert UTF-16LE to UTF-8
						input->seek(-2, WPX_SEEK_CUR);
						this->appendUTF16LE(input, listener);
						break;
				}
			}

			len *= 2;
			c_len -= len;
			p_len -= len;
			last_fcLim += len;

			if (p_len == 0) {
				PFOD_iter++;
				if (c_len > 0) { /* otherwise will be set by outside loop */
					p_len = (*PFOD_iter).fcLim - last_fcLim;
					len = (c_len > p_len)? p_len : c_len;
					len /= 2;
				}
			}
		} while (c_len > 0);

	}
#endif
}

/**
 * Read the range of the document text using previously-read
 * formatting information, up to but excluding endpos.
 *
 */

void WPS8Parser::readTextRange(WPXInputStream * input, WPS8ContentListener *listener,
							   uint32_t startpos, uint32_t endpos, uint16_t stream)
{
	WPS_DEBUG_MSG(("WPS8Parser::readTextRange(stream=%d)\n",stream));

	std::vector<FOD>::iterator FODs_iter;
	std::vector<FOD>::iterator PFOD_iter;

	uint32_t last_fcLim = 0x200;
	uint32_t start_fcLim = 0x200 + startpos*2;
	uint32_t total_fcLim = start_fcLim + (endpos - startpos)*2;
	PFOD_iter = PAFODs.begin();
	FODs_iter = CHFODs.begin();

	while (last_fcLim < start_fcLim) {
		uint32_t c_len = (*FODs_iter).fcLim - last_fcLim;
		uint32_t p_len = (*PFOD_iter).fcLim - last_fcLim;
		uint32_t len = (c_len > p_len)? p_len : c_len;

		last_fcLim += len;
		if (len == c_len) FODs_iter++;
		if (len == p_len) PFOD_iter++;
	}

	/* should never happen? */
	if (last_fcLim > start_fcLim) last_fcLim = start_fcLim;

	for (; last_fcLim < total_fcLim; FODs_iter++)
	{
		FOD fod = *(FODs_iter);
		uint32_t c_len = (*FODs_iter).fcLim - last_fcLim;
		uint32_t p_len = (*PFOD_iter).fcLim - last_fcLim;

		if (last_fcLim + c_len > total_fcLim) c_len = total_fcLim - last_fcLim;
		uint32_t len = (c_len > p_len)? p_len : c_len;


		if (len % 2 != 0)
		{
			WPS_DEBUG_MSG(("Works: error: len %i is odd\n", len));
			throw ParseException();
		}
		len /= 2;

		/* print rgchProp as hex bytes */
		WPS_DEBUG_MSG(("rgch="));
		for (unsigned int blah=0; blah < (*FODs_iter).fprop.rgchProp.length(); blah++)
		{
			WPS_DEBUG_MSG(("%02X ", (uint8_t) (*FODs_iter).fprop.rgchProp[blah]));
		}
		WPS_DEBUG_MSG(("\n"));

		/* process character formatting */
		if ((*FODs_iter).fprop.cch > 0)
			propertyChange((*FODs_iter).fprop.rgchProp, listener);

		/* loop until character format not exhausted */
		do {
			/* paragraph format may change here*/

			if ((*PFOD_iter).fprop.cch > 0)
				propertyChangePara((*PFOD_iter).fprop.rgchProp, listener);

			/* plain text */
			input->seek(last_fcLim, WPX_SEEK_SET);
			for (uint32_t i = len; i>0; i--)
			{
				uint16_t readVal = readU16(input);

				if (0x00 == readVal)
					break;
					
				switch (readVal)
				{
					case 0x0A:
						break;

					case 0x0C:
						//fixme: add a page to list of pages
						//listener->insertBreak(WPS_PAGE_BREAK);
						break;

					case 0x0D:
						listener->insertEOL();
						break;

					case 0x0E:
						listener->insertBreak(WPS_COLUMN_BREAK);
						break;

					case 0x1E:
						//fixme: non-breaking hyphen
						break;

					case 0x1F:
						//fixme: optional breaking hyphen
						break;

					case 0x23:
						if (uint16_t spec = listener->getSpec()) {
						//	TODO: fields, pictures, etc.
							switch (spec) {
								case 3:
									if (stream != WPS_STREAM_BODY) break;
								  /* REMOVED in this version
								     because footnote/endnote tends to make
								     the resulting file inconsistent */
									//listener->openFootnote();
									listener->insertEOL();
									readNote(input,listener,false);
									break;
								case 4:
									if (stream != WPS_STREAM_BODY) break;
								  /* REMOVED in this version
								     because footnote/endnote tends to make
								     the resulting file inconsistent */
									//listener->openEndnote();
									listener->insertEOL();
									readNote(input,listener,true);
									//listener->closeEndnote();
									break;
								case 5:
									listener->insertField();
									break;
								default:
									listener->insertCharacter(0xE2/*0x263B*/);
									listener->insertCharacter(0x98);
									listener->insertCharacter(0xBB);
							}
							// listener->insertSpecial();
							break;
						}
						// ! fallback to default

					case 0xfffc:
						// ! fallback to default

					default:
						// fixme: convert UTF-16LE to UTF-8
						input->seek(-2, WPX_SEEK_CUR);
						this->appendUTF16LE(input, listener);
						break;
				}
			}

			len *= 2;
			c_len -= len;
			p_len -= len;
			last_fcLim += len;

			if (p_len == 0) {
				PFOD_iter++;
				if (c_len > 0) { /* otherwise will be set by outside loop */
					p_len = (*PFOD_iter).fcLim - last_fcLim;
					len = (c_len > p_len)? p_len : c_len;
					len /= 2;
				}
			}
		} while (c_len > 0);

	}
}

void WPS8Parser::readNote(WPXInputStream * input, WPS8ContentListener *listener, bool is_endnote)
{
	WPSNote note;
	WPSStream stream;
	uint32_t  streamkey = WPS_STREAM_FOOTNOTES;

	if (!is_endnote) {
		if (fn_iter != footnotes.end()) {
			note = *fn_iter++;
		}
	} else {
		if (en_iter != endnotes.end()) {
			note = *en_iter++;
		}
		streamkey = WPS_STREAM_ENDNOTES;
	}

	for (unsigned i=0; i<streams.size(); i++) {
		if (streams[i].type == streamkey) {
			stream = streams[i];
			break;
		}
	}

	WPS_DEBUG_MSG(("Reading footnote [%d;%d)\n",note.contents.start,note.contents.limit));

	uint32_t pos = input->tell();
	readTextRange(input,listener,stream.span.start+note.contents.start,
		stream.span.start+note.contents.limit,streamkey);
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

bool WPS8Parser::readFODPage(WPXInputStream * input, std::vector<FOD> * FODs, uint16_t page_size)
{
	uint32_t page_offset = input->tell();
	uint16_t cfod = readU16(input); /* number of FODs on this page */

	//fixme: what is the largest possible cfod?
	if (cfod > 0x54)
	{
		WPS_DEBUG_MSG(("Works8: error: cfod = %i (0x%X)\n", cfod, cfod));
		throw ParseException();
	}

	input->seek(page_offset + 2 + 6, WPX_SEEK_SET);	// fixme: unknown

	int first_fod = FODs->size();

	/* Read array of fcLim of FODs.  The fcLim refers to the offset of the
           last character covered by the formatting. */
	for (int i = 0; i < cfod; i++)
	{
		FOD fod;
		fod.fcLim = readU32(input);
//		WPS_DEBUG_MSG(("Works: info: fcLim = %i (0x%X)\n", fod.fcLim, fod.fcLim));			
		
		/* check that fcLim is not too large */
		if (fod.fcLim > offset_eot)
		{
			WPS_DEBUG_MSG(("Works: error: length of 'text selection' %i > "
				"total text length %i\n", fod.fcLim, offset_eot));					
			throw ParseException();	
		}

		/* check that fcLim is monotonic */
		if (FODs->size() > 0 && FODs->back().fcLim > fod.fcLim)
		{
			WPS_DEBUG_MSG(("Works: error: character position list must "
				"be monotonic, but found %i, %i\n", FODs->back().fcLim, fod.fcLim));
			throw ParseException();
		}
		FODs->push_back(fod);
	} 	

        /* Read array of bfprop of FODs.  The bfprop is the offset where
           the FPROP is located. */
	std::vector<FOD>::iterator FODs_iter;
	for (FODs_iter = FODs->begin() + first_fod; FODs_iter!= FODs->end(); FODs_iter++)
	{
		if ((*FODs_iter).fcLim == offset_eot)
			break;

		(*FODs_iter).bfprop = readU16(input);

		/* check size of bfprop  */
		if (((*FODs_iter).bfprop < (8 + (6*cfod)) && (*FODs_iter).bfprop > 0) ||
			(*FODs_iter).bfprop  > (page_size - 1))
		{
			WPS_DEBUG_MSG(("Works: error: size of bfprop is bad "
				"%i (0x%X)\n", (*FODs_iter).bfprop, (*FODs_iter).bfprop));
			throw ParseException();
		}

		(*FODs_iter).bfprop_abs = (*FODs_iter).bfprop + page_offset;
//		WPS_DEBUG_MSG(("Works: debug: bfprop = 0x%03X, bfprop_abs = 0x%03X\n",
//                       (*FODs_iter).bfprop, (*FODs_iter).bfprop_abs));
	}

	
	/* Read array of FPROPs.  These contain the actual formatting
	   codes (bold, alignment, etc.) */
	for (FODs_iter = FODs->begin()+first_fod; FODs_iter!= FODs->end(); FODs_iter++)
	{
		if ((*FODs_iter).fcLim == offset_eot)
			break;

		if (0 == (*FODs_iter).bfprop)
		{
			(*FODs_iter).fprop.cch = 0;
			continue;
		}

		input->seek((*FODs_iter).bfprop_abs, WPX_SEEK_SET);
		(*FODs_iter).fprop.cch = readU8(input);
		if (0 == (*FODs_iter).fprop.cch)
		{
			WPS_DEBUG_MSG(("Works: error: 0 == cch at file offset 0x%lx", (input->tell())-1));
			throw ParseException();
		}
		// check that the property remains in the FOD zone
		if ((*FODs_iter).bfprop_abs+(*FODs_iter).fprop.cch > page_offset+page_size)
		{
			WPS_DEBUG_MSG(("Works: error: cch = %i, too large ", (*FODs_iter).fprop.cch));
			throw ParseException();
		}

		(*FODs_iter).fprop.cch--;

		for (int i = 0; (*FODs_iter).fprop.cch > i; i++)
			(*FODs_iter).fprop.rgchProp.append(1, (uint8_t)readU8(input));
	}
	
	/* go to end of page */
	input->seek(page_offset	+ page_size, WPX_SEEK_SET);

	return (offset_eot > FODs->back().fcLim);

	
}

/**
 * Parse an index entry in the file format's header.  For example,
 * this function may be called multiple times to parse several FDPP
 * entries.  This functions begins at the current position of the
 * input stream, which will be advanced.
 *
 */

void WPS8Parser::parseHeaderIndexEntry(WPXInputStream * input)
{
	WPS_DEBUG_MSG(("Works8: debug: parseHeaderIndexEntry() at file pos 0x%lX\n", input->tell()));

	uint16_t cch = readU16(input);
	if (0x18 != cch)
	{
		WPS_DEBUG_MSG(("Works8: error: parseHeaderIndexEntry cch = %i (0x%X)\n", cch, cch));
               /* Osnola: normally, this size must be >= 0x18
                  In my code, I throw an exception when this size is less than 10
                  to try to continue the parsing ( but I do not accept this entry)
                */
               if (cch < 10)
                 throw ParseException();
	}

	std::string name;

	// sanity check
    int i;
	for (i =0; i < 4; i++)
	{
		name.append(1, readU8(input));

		if ((uint8_t)name[i] != 0 && (uint8_t)name[i] != 0x20 &&
			(41 > (uint8_t)name[i] || (uint8_t)name[i] > 90))
		{
			WPS_DEBUG_MSG(("Works8: error: bad character=%u (0x%02x) in name in header index\n", 
				(uint8_t)name[i], (uint8_t)name[i]));
			throw ParseException();
		}
	}
	name.append(1, (char)0);

	std::string unknown1;
	for (i = 0; i < 6; i ++)
		unknown1.append(1, readU8(input));

	std::string name2;
	for (i =0; i < 4; i++)
		name2.append(1, readU8(input));
	name2.append(1, (char)0);

	if (name != name2)
	{
		WPS_DEBUG_MSG(("Works8: error: name != name2, %s != %s\n", 
			name.c_str(), name2.c_str()));
		// fixme: what to do with this?
//		throw ParseException();
	}

	HeaderIndexEntries hie;
	hie.offset = readU32(input);
	hie.length = readU32(input);

	WPS_DEBUG_MSG(("Works8: debug: header index entry %s with offset=0x%04X, length=0x%04X\n", 
		name.c_str(), hie.offset, hie.length));

	headerIndexTable.insert(std::multimap<std::string, HeaderIndexEntries>::value_type(name, hie));
       // OSNOLA: cch is the length of the entry, so we must advance by cch-0x18
       input->seek(input->tell() + cch - 0x18, WPX_SEEK_SET);
}

/**
 * In the header, parse the index to the different sections of
 * the CONTENTS stream.
 * 
 */
void WPS8Parser::parseHeaderIndex(WPXInputStream * input)
{
	input->seek(0x0C, WPX_SEEK_SET);		
	uint16_t n_entries = readU16(input);
	// fixme: sanity check n_entries

	input->seek(0x18, WPX_SEEK_SET);		
	do
	{
		uint16_t unknown1 = readU16(input);
		if (0x01F8 != unknown1)
		{
			WPS_DEBUG_MSG(("Works8: error: unknown1=0x%X\n", unknown1));
#if 0
			throw ParseException();
#endif
		}

		uint16_t n_entries_local = readU16(input);

		if (n_entries_local > 0x20)
		{
			WPS_DEBUG_MSG(("Works8: error: n_entries_local=%i\n", n_entries_local));
			throw ParseException();	
		}

		uint32_t next_index_table = readU32(input);

		do
		{
			parseHeaderIndexEntry(input);
			n_entries--;
			n_entries_local--;
		} while (n_entries > 0 && n_entries_local);
		
		if (0xFFFFFFFF == next_index_table && n_entries > 0)
		{
			WPS_DEBUG_MSG(("Works8: error: expected more header index entries\n"));
			throw ParseException();
		}

		if (0xFFFFFFFF == next_index_table)
			break;

		WPS_DEBUG_MSG(("Works8: debug: seeking to position 0x%X\n", next_index_table));
		input->seek(next_index_table, WPX_SEEK_SET);
	} while (n_entries > 0);
}

/**
 * Read the page format from the file.  It seems that WPS8 files
 * can only have one page format throughout the whole document.
 *
 */
void WPS8Parser::parsePages(std::list<WPSPageSpan> &pageList, WPXInputStream * /* input */)
{
	//fixme: this method doesn't do much

	/* record page format */
	WPSPageSpan ps;
	pageList.push_back(ps);
}

void WPS8Parser::parse(WPXInputStream *input, WPS8ContentListener *listener)
{
	WPS_DEBUG_MSG(("WPS8Parser::parse()\n"));	

	listener->startDocument();

	/* header index */
	parseHeaderIndex(input);

	HeaderIndexMultiMap::iterator pos;
	for (pos = headerIndexTable.begin(); pos != headerIndexTable.end(); ++pos)
	{
		WPS_DEBUG_MSG(("Works: debug: headerIndexTable: %s, offset=0x%X, length=0x%X, end=0x%X\n",
			pos->first.c_str(), pos->second.offset, pos->second.length, pos->second.offset +
			pos->second.length));
	}

	/* What is the total length of the text? */
	pos = headerIndexTable.lower_bound("TEXT");
	if (headerIndexTable.end() == pos)
	{
		WPS_DEBUG_MSG(("Works: error: no TEXT in header index table\n"));
	}
	offset_eot = pos->second.offset + pos->second.length;
	WPS_DEBUG_MSG(("Works: debug: TEXT offset_eot = 0x%04X\n", offset_eot));

	/* read character FODs (FOrmatting Descriptors) */
	for (pos = headerIndexTable.begin(); pos != headerIndexTable.end(); ++pos)
	{
		if (0 != strcmp("FDPC",pos->first.c_str()))
			continue;

		WPS_DEBUG_MSG(("Works: debug: FDPC (%s) offset=0x%X, length=0x%X\n",
				pos->first.c_str(), pos->second.offset, pos->second.length));

		input->seek(pos->second.offset, WPX_SEEK_SET);
		if (pos->second.length != 512)
		{
			WPS_DEBUG_MSG(("Works: warning: FDPC offset=0x%X, length=0x%X\n", 
				pos->second.offset, pos->second.length));
		}
		readFODPage(input, &CHFODs, pos->second.length);
	}

	/* read para FODs (FOrmatting Descriptors) */
	for (pos = headerIndexTable.begin(); pos != headerIndexTable.end(); ++pos)
	{
		if (0 != strcmp("FDPP",pos->first.c_str()))
			continue;

		WPS_DEBUG_MSG(("Works: debug: FDPP (%s) offset=0x%X, length=0x%X\n",
				pos->first.c_str(), pos->second.offset, pos->second.length));

		input->seek(pos->second.offset, WPX_SEEK_SET);
		if (pos->second.length != 512)
		{
			WPS_DEBUG_MSG(("Works: warning: FDPP offset=0x%X, length=0x%X\n", 
				pos->second.offset, pos->second.length));
		}
		readFODPage(input, &PAFODs, pos->second.length);
	}

	/* read streams table*/
	readStreams(input);

	/* read fonts table */
	readFontsTable(input);

	readNotes(footnotes,input,"FTN ");
	readNotes(endnotes,input,"EDN ");

	fn_iter = footnotes.begin();
	en_iter = endnotes.begin();

	/* process text file using previously-read character formatting */
	uint32_t doc_start = 0, doc_end = (offset_eot - 0x200) >> 1; // character offsets
	uint32_t doc_start2 = doc_start, doc_end2 = doc_end;
	for (unsigned i=0; i<streams.size(); i++) {
		/* skip to extract full document text for debug purposes */
		/*if (streams[i].type == WPS_STREAM_BODY) {
			readTextRange(input,listener,streams[i].span.start,streams[i].span.limit,
				WPS_STREAM_BODY);
		} else*/ if (streams[i].type == WPS_STREAM_FOOTNOTES ||
					streams[i].type == WPS_STREAM_ENDNOTES) {
			if (streams[i].span.start < doc_end) doc_end = streams[i].span.start;
			if (streams[i].span.limit > doc_start2) doc_start2 = streams[i].span.limit;
		}
	}
	if (doc_end > doc_start2) doc_start2 = doc_end;

	readTextRange(input,listener,doc_start,doc_end,WPS_STREAM_BODY);
	if (doc_end2 > doc_start2) 
		readTextRange(input,listener,doc_start2,doc_end2,WPS_STREAM_BODY);
	//readText(input, listener);

	listener->endDocument();		
}



#define WPS8_ATTRIBUTE_BOLD 0
#define WPS8_ATTRIBUTE_ITALICS 1
#define WPS8_ATTRIBUTE_UNDERLINE 2
#define WPS8_ATTRIBUTE_STRIKEOUT 3
#define WPS8_ATTRIBUTE_SUBSCRIPT 4
#define WPS8_ATTRIBUTE_SUPERSCRIPT 5
#define WPS8_ATTRIBUTE_SPECIAL 6

void WPS8Parser::propertyChangeTextAttribute(const uint32_t newTextAttributeBits, const uint8_t attribute, const uint32_t bit, WPS8ContentListener *listener)
{
	if ((oldTextAttributeBits ^ newTextAttributeBits) & bit)
		listener->attributeChange(newTextAttributeBits & bit, attribute);
}

/**
 * @param newTextAttributeBits: all the new, current bits (will be compared against old, and old will be discarded).
 * @param listener: 
 *
 */
void WPS8Parser::propertyChangeDelta(uint32_t newTextAttributeBits, WPS8ContentListener *listener)
{
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_BOLD, WPS_BOLD_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_ITALICS, WPS_ITALICS_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_UNDERLINE, WPS_UNDERLINE_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_STRIKEOUT, WPS_STRIKEOUT_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_SUBSCRIPT, WPS_SUBSCRIPT_BIT, listener);
	propertyChangeTextAttribute(newTextAttributeBits, WPS8_ATTRIBUTE_SUPERSCRIPT, WPS_SUPERSCRIPT_BIT, listener);
	oldTextAttributeBits = newTextAttributeBits;
}



/**
 * Process a character property change.  The Works format supplies
 * all the character formatting each time there is any change (as
 * opposed to HTML, for example).  In Works 8, the position in
 * rgchProp is not significant because there are some kind of
 * codes.
 *
 */
void WPS8Parser::propertyChange(std::string rgchProp, WPS8ContentListener *listener)
{
	//fixme: this method is immature

	int iv;

	/* set default properties */
	uint32_t textAttributeBits = 0;
	listener->setColor(0);
	propertyChangeDelta(0,listener);
	listener->setFontSize(10);
	/* maybe other stuff */

	/* check */
	/* sometimes, the rgchProp is blank */
	if (0 == rgchProp.length()) {
		return;
	}
	/* other than blank, the shortest should be 9 bytes */
	if (rgchProp.length() < 3)
	{
		WPS_DEBUG_MSG(("Works8: error: rgchProp.length() < 9\n"));
		throw ParseException();
	}
	if (0 == (rgchProp.length() % 2))
	{
		WPS_DEBUG_MSG(("Works8: error: rgchProp.length() is even\n"));
		throw ParseException();
	}
        if (0 != rgchProp[0] || 0 != rgchProp[1] || 0 != rgchProp[2])
	{
		WPS_DEBUG_MSG(("Works8: error: rgchProp does not begin 0x000000\n"));		
		throw ParseException();
	}


	//oldTextAttributeBits=0;

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
                                       //throw ParseException();
			}
			continue;
		}

		uint16_t format_code = rgchProp[x] | (rgchProp[x+1] << 8);
		
		switch (format_code)
		{
			case 0x0000:
				break;

			case 0x1200:
				{
					// special code
					uint16_t spec = WPS_LE_GET_GUINT16(rgchProp.substr(x+2,2).c_str());
					listener->setSpec(spec);
					x += 2;
				}
				break;

			case 0x120F:
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
				/* font size */
				uint32_t font_size = WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
				listener->setFontSize(font_size/12700);
				x += 4;
				break;
			}

			case 0x2218:
				//fixme: unknown
				x += 4;
				break;

			case 0x2212:
				listener->setLCID(WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str()));
				x += 4;
				break;

			case 0x2222:
				// field.
				iv = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
				switch (iv) {
					case -1:
						listener->setFieldType(WPS_FIELD_PAGE);
						break;
					case -4:
						listener->setFieldType(WPS_FIELD_DATE);
						break;
					case -5:
						listener->setFieldType(WPS_FIELD_TIME);
						break;
				}
				x += 4;
				break;

			case 0x2223:
				//fixme: date and time field?
				iv = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
				x += 4;
				break;

			case 0x222E:
				listener->setColor((((unsigned char)rgchProp[x+2]<<16)+((unsigned char)rgchProp[x+3]<<8)+
					(unsigned char)rgchProp[x+4])&0xFFFFFF);
				x += 4;
				break;
	
			case 0x8A24:
			{
				/* font change */
				uint8_t font_n = (uint8_t)rgchProp[x+8];
				if (font_n > fonts.size())
				{
					WPS_DEBUG_MSG(("Works: error: encountered font %i (0x%02x) which is not indexed\n", 
						font_n,font_n ));			
					throw ParseException();
				}
				else
					listener->setTextFont(fonts[font_n].c_str());

				//x++;
				x += rgchProp[x+2];
			}
				break;		
			default:
				WPS_DEBUG_MSG(("Works8: error: unknown format code 0x%04X\n", format_code));
				switch ((format_code>>12)&0xF) {
					case 1:
						x+=2;
						break;
					case 2:
						x+=4;
						break;
					case 8:
						x += rgchProp[x+2];
						break;
				}
				//throw ParseException();
				break;


		}
	}

	propertyChangeDelta(textAttributeBits, listener);
}

void WPS8Parser::propertyChangePara(std::string rgchProp, WPS8ContentListener *listener)
{
	static const uint8_t _align[]={WPS_PARAGRAPH_JUSTIFICATION_LEFT,
		WPS_PARAGRAPH_JUSTIFICATION_RIGHT,WPS_PARAGRAPH_JUSTIFICATION_CENTER,
		WPS_PARAGRAPH_JUSTIFICATION_FULL};

	int iv, align = 0;
	float pleft=0.0, prigh=0.0, pfirst=0.0, pbefore=0.0, pafter=0.0;

	std::vector<TabPos> l_pos;

	listener->setNumberingType(WPS_NUMBERING_NONE);
	listener->setTabs(l_pos);

	/* sometimes, the rgchProp is blank */
	if (0 == rgchProp.length())
		return;

	for (uint32_t x = 3; x < rgchProp.length(); x += 2) {
		uint16_t format_code = rgchProp[x] | (rgchProp[x+1] << 8);
		
		switch (format_code)
		{
			case 0x1A03:
				iv = WPS_LE_GET_GUINT16(rgchProp.substr(x+2,2).c_str()) & 0xF;
				/* paragraph has a bullet specified in num format*/
				listener->setNumberingType(WPS_NUMBERING_BULLET);
				x+=2;
				break;

			case 0x1204:
				iv = WPS_LE_GET_GUINT16(rgchProp.substr(x+2,2).c_str()) & 0xF;
				if (iv >= 0 && iv < 4) align = _align[iv];
				x+=2;
				break;

			case 0x220C:
				iv = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
				pfirst=iv/914400.0;
				x+=4;
				break;

			case 0x220D:
				iv = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
				pleft=iv/914400.0;
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
					/* numbering style */
					iv = (int) WPS_LE_GET_GUINT32(rgchProp.substr(x+2,4).c_str());
					int separator = iv >> 16;
					int num_style = iv & 0xFFFF; // also Windings character for mark.
					listener->setNumberingProp(num_style,separator);
					x+=4;
				}
				break;

			case 0x8232:
				{
					TabPos s_pos;
					const char *ts = rgchProp.c_str();
					unsigned  t_count = 0;
					int  t_size = WPS_LE_GET_GUINT32(&ts[x+2]);
					int  t_rem = t_size;
					int  tp_rem = 0;
					int  id = x+6;
					uint16_t prop;

					if (t_rem > 2) {
						prop = WPS_LE_GET_GUINT16(&ts[id]);
						if (prop == 0x1A27) {
							t_count = WPS_LE_GET_GUINT16(&ts[id+2]);
							id += 4;
							t_rem -= 4;

							if (t_count > 20) break; /* obviously wrong */
						} else break; /* wrong format */
					}

					if (t_count > 0 && t_rem > 2) {
						prop = WPS_LE_GET_GUINT16(&ts[id]);
						if (prop == 0x8A28) {
							tp_rem = WPS_LE_GET_GUINT32(&ts[id+2]);
							id += 6;
							t_rem -= 6;
							tp_rem -= 4;

							if (tp_rem > t_rem) break; /* truncated? */
						} else break; /* wrong format */
					}

					while (/*l_pos.size() < t_count && */ tp_rem > 0) {
						prop = WPS_LE_GET_GUINT16(&ts[id]);
						id += 2;
						tp_rem -= 2;

						unsigned iid = (prop >> 3) & 0xF; /* TODO: verify*/
						int iprop = (prop & 0xFF87);

						if (iid >= t_count) break; /* sanity */
						while (iid >= l_pos.size()) {
							s_pos.pos = 0;
							s_pos.align = 0;
							s_pos.leader = 0;
							l_pos.push_back(s_pos);
						}
						switch (iprop) {
							case 0x2000:
								{
									float tabpos = WPS_LE_GET_GUINT32(&ts[id]) / 914400.0;
									l_pos[iid].pos = tabpos;
									id += 4;
									tp_rem -=4;
								}
								break;
							case 0x1001:
								switch (ts[id] & 0xF) {
									case 1: l_pos[iid].align = WPS_TAB_RIGHT; break;
									case 2: l_pos[iid].align = WPS_TAB_CENTER; break;
									case 3: l_pos[iid].align = WPS_TAB_DECIMAL; break;
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
						s_pos.pos = tabpos;
						l_pos.push_back(s_pos);
						break;
						id += 4;
						t_rem -= 4;
					}*/

					listener->setTabs(l_pos);

					x += t_size;
				}
				break;

			default:
				WPS_DEBUG_MSG(("Works8: error: unknown pformat code 0x%04X\n", format_code));
				switch ((format_code>>12)&0xF) {
					case 1:
						x+=2;
						break;
					case 2:
						x+=4;
						break;
					case 8:
						x += rgchProp[x+2];
						break;
				}
				//throw ParseException();
				break;
		}
	}

	listener->setAlign(align);
	listener->setMargins(pfirst,pleft,prigh,pbefore,pafter);
}

/*
WPS8ContentListener public
*/

WPS8ContentListener::WPS8ContentListener(std::list<WPSPageSpan> &pageList, WPXDocumentInterface *documentInterface) :
	WPSContentListener(pageList, documentInterface)
{
}

WPS8ContentListener::~WPS8ContentListener()
{
}

void WPS8ContentListener::attributeChange(const bool isOn, const uint8_t attribute)
{
	WPS_DEBUG_MSG(("WPS8ContentListener::attributeChange(%i, %i)\n", isOn, attribute));		
	_closeSpan();
	
	uint32_t textAttributeBit = 0;
	switch (attribute)
	{
		case WPS8_ATTRIBUTE_BOLD:
			textAttributeBit = WPS_BOLD_BIT;
			break;
		case WPS8_ATTRIBUTE_ITALICS:
			textAttributeBit = WPS_ITALICS_BIT;		
			break;
		case WPS8_ATTRIBUTE_UNDERLINE:
			textAttributeBit = WPS_UNDERLINE_BIT;		
			break;	
		case WPS8_ATTRIBUTE_STRIKEOUT:
			textAttributeBit = WPS_STRIKEOUT_BIT;		
			break;				
		case WPS8_ATTRIBUTE_SUBSCRIPT:
			textAttributeBit = WPS_SUBSCRIPT_BIT;		
			break;							
		case WPS8_ATTRIBUTE_SUPERSCRIPT:
			textAttributeBit = WPS_SUPERSCRIPT_BIT;		
			break;							
	}
	if (isOn)
		m_ps->m_textAttributeBits |= textAttributeBit;
	else
		m_ps->m_textAttributeBits &= ~textAttributeBit;
}
