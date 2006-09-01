/* libwpd
 * Copyright (C) 2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */
 
#include "WP5Parser.h"
#include "WPXHeader.h"
#include "WP5Part.h"
#include "WP5ContentListener.h"
#include "WP5StylesListener.h"
#include "libwpd_internal.h"
#include "WPXTable.h"
#include "WP5PrefixData.h"
#include "WP5ListFontsUsedPacket.h"
#include "WP5FontNameStringPoolPacket.h"

WP5Parser::WP5Parser(WPXInputStream *input, WPXHeader *header) :
	WPXParser(input, header)
{
}

WP5Parser::~WP5Parser()
{
}

WP5PrefixData * WP5Parser::getPrefixData(WPXInputStream *input)
{
	WP5PrefixData *prefixData = NULL;
	try
	{
		prefixData = new WP5PrefixData(input);
		return prefixData;
	}
	catch(FileException)
	{
		DELETEP(prefixData);
		throw FileException();
	}
}

void WP5Parser::parse(WPXInputStream *input, WP5Listener *listener)
{
	listener->startDocument();
	
	input->seek(getHeader()->getDocumentOffset(), WPX_SEEK_SET);	
	
	WPD_DEBUG_MSG(("WordPerfect: Starting document body parse (position = %ld)\n",(long)input->tell()));
	
	parseDocument(input, listener);
	
	listener->endDocument();		
}

// parseDocument: parses a document body (may call itself recursively, on other streams, or itself)
void WP5Parser::parseDocument(WPXInputStream *input, WP5Listener *listener)
{
	while (!input->atEOS())
	{
		uint8_t readVal;
		readVal = readU8(input);
		
		if (readVal == 0 || readVal == 0x7F || readVal == 0xFF)
		{
			// do nothing: this token is meaningless and is likely just corruption
		}
		else if (readVal >= (uint8_t)0x01 && readVal <= (uint8_t)0x1F)
		{
			// control characters
			
			switch (readVal)
			{
				case 0x0A: // hard new line
					listener->insertEOL();
					break;
				case 0x0B: // soft new page (convert like space)
					listener->insertCharacter((uint16_t) ' ');
					listener->insertBreak(WPX_SOFT_PAGE_BREAK);
					break;
				case 0x0C: // hard new page
					listener->insertBreak(WPX_PAGE_BREAK);
					break;
				case 0x0D: // soft new line (convert like space)
					listener->insertCharacter((uint16_t) ' ');
					break;
				default:
					// unsupported or undocumented token, ignore
					break;
			}
		}
		else if (readVal >= (uint8_t)0x20 && readVal <= (uint8_t)0x7E)
		{
			listener->insertCharacter( readVal );
		}
		else 
		{
			WP5Part *part = WP5Part::constructPart(input, readVal);
			if (part != NULL)
			{
				part->parse(listener);
				DELETEP(part);
			}
		}	
	}
}

void WP5Parser::parse(WPXHLListenerImpl *listenerImpl)
{
	WPXInputStream *input = getInput();
	std::list<WPXPageSpan> pageList;
	WPXTableList tableList;	
	WP5PrefixData * prefixData = NULL;
	std::vector<WP5SubDocument *> subDocuments;
	
	try
 	{
		prefixData = getPrefixData(input);

		// do a "first-pass" parse of the document
		// gather table border information, page properties (per-page)
		WP5StylesListener stylesListener(pageList, tableList, subDocuments);
		parse(input, &stylesListener);

		// postprocess the pageList == remove duplicate page spans due to the page breaks
		std::list<WPXPageSpan>::iterator previousPage = pageList.begin();
		for (std::list<WPXPageSpan>::iterator Iter=pageList.begin(); Iter != pageList.end();)
		{
			if ((Iter != previousPage) && ((*previousPage)==(*Iter)))
			{
				(*previousPage).setPageSpan((*previousPage).getPageSpan() + (*Iter).getPageSpan());
				Iter = pageList.erase(Iter);
			}
			else
			{
				previousPage = Iter;
				Iter++;
			}
		}

		// second pass: here is where we actually send the messages to the target app
		// that are necessary to emit the body of the target document
		WP5ContentListener listener(pageList, subDocuments, listenerImpl); // FIXME: SHOULD BE PASSED TABLE DATA!
		listener.setPrefixData(prefixData);


		// According the documentation, first font in the font list is the default font, so use it as such
		// This is the quick and dirty way and maybe one could do it differently
		// FIXME: UGLY, UGLY, UGLY!!! FIND A BETTER WAY TO ACHIEVE THE SAME
		uint16_t tmpFontNameOffset;
		float tmpFontSize = 12.0f;
		WPXString tmpFontName("Times New Roman");
		bool tmpHasFontsUsedPacket = true;

		if (listener.getGeneralPacketData(15))
		{
			tmpFontSize = static_cast<const WP5ListFontsUsedPacket*>(listener.getGeneralPacketData(15))->getFontSize(0);
			tmpFontNameOffset = static_cast<const WP5ListFontsUsedPacket*>(listener.getGeneralPacketData(15))->getFontNameOffset(0);
		}
		else if (listener.getGeneralPacketData(2))
		{
			tmpFontSize = static_cast<const WP5ListFontsUsedPacket*>(listener.getGeneralPacketData(2))->getFontSize(0);
			tmpFontNameOffset = static_cast<const WP5ListFontsUsedPacket*>(listener.getGeneralPacketData(2))->getFontNameOffset(0);
		}
		else
			tmpHasFontsUsedPacket = false;
		
		if (tmpHasFontsUsedPacket && (listener.getGeneralPacketData(7)))
			tmpFontName = static_cast<const WP5FontNameStringPoolPacket*>(listener.getGeneralPacketData(7))->getFontName(tmpFontNameOffset);
			
		listener.setFont(tmpFontName, tmpFontSize);
		listener.setDefaultFont(tmpFontName, tmpFontSize);
		// FIXME: UGLY, UGLY, UGLY!!! FIND A BETTER WAY TO ACHIEVE THE SAME
		

		parse(input, &listener);
		
		// cleanup section: free the used resources
		delete prefixData;
		for (std::vector<WP5SubDocument *>::iterator iterSubDoc = subDocuments.begin(); iterSubDoc != subDocuments.end(); iterSubDoc++)
		{
			if (*iterSubDoc)
				delete (*iterSubDoc);
		}
	}
	catch(FileException)
	{
		WPD_DEBUG_MSG(("WordPerfect: File Exception. Parse terminated prematurely."));

		delete prefixData;
		for (std::vector<WP5SubDocument *>::iterator iterSubDoc = subDocuments.begin(); iterSubDoc != subDocuments.end(); iterSubDoc++)
		{
			if (*iterSubDoc)
				delete (*iterSubDoc);
		}

		throw FileException();
	}	
}
