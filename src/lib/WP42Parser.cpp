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

#include "WP42Parser.h"
#include "WP42Part.h"
#include "WPXHeader.h"
#include "libwpd_internal.h"
#include "WPXTable.h"
#include "WP42FileStructure.h"
#include "WP42StylesListener.h"
#include "WP42ContentListener.h"

WP42Parser::WP42Parser(WPXInputStream *input) :
	WPXParser(input, NULL)
{
}

WP42Parser::~WP42Parser()
{
}

void WP42Parser::parse(WPXInputStream *input, WP42Listener *listener)
{
	listener->startDocument();

	input->seek(0, WPX_SEEK_SET);

	WPD_DEBUG_MSG(("WordPerfect: Starting document body parse (position = %ld)\n",(long)input->tell()));

	parseDocument(input, listener);

	listener->endDocument();
}

// parseDocument: parses a document body (may call itself recursively, on other streams, or itself)
void WP42Parser::parseDocument(WPXInputStream *input, WP42Listener *listener)
{
	while (!input->atEOS())
	{
		uint8_t readVal;
		readVal = readU8(input);

		if (readVal < (uint8_t)0x20)
		{
			WPD_DEBUG_MSG(("Offset: %i, Handling Control Character 0x%2x\n", input->tell(), readVal));			

			switch (readVal)
			{
				case 0x09: // tab
					listener->insertTab(0, 0.0f);
					break;
				case 0x0A: // hard new line
					listener->insertEOL();
					break;
				case 0x0B: // soft new page
					listener->insertBreak(WPX_SOFT_PAGE_BREAK);
					break;
				case 0x0C: // hard new page
					listener->insertBreak(WPX_PAGE_BREAK);
					break;
				case 0x0D: // soft new line
					listener->insertCharacter(' ');
					break;
				default:
					// unsupported or undocumented token, ignore
					break;
			}
		}
		else if (readVal >= (uint8_t)0x20 && readVal <= (uint8_t)0x7F)
		{
			WPD_DEBUG_MSG(("Offset: %i, Handling Ascii Character 0x%2x\n", input->tell(), readVal));			

			// normal ASCII characters
			listener->insertCharacter( readVal );
		}
		else if (readVal >= (uint8_t)0x80 && readVal <= (uint8_t)0xBF)
		{
			WPD_DEBUG_MSG(("Offset: %i, Handling Single Character Function 0x%2x\n", input->tell(), readVal));			

			// single character function codes
			switch (readVal)
			{
				case 0x92:
					listener->attributeChange(true, WP42_ATTRIBUTE_STRIKE_OUT);
					break;
				case 0x93:
					listener->attributeChange(false, WP42_ATTRIBUTE_STRIKE_OUT);
					break;
				case 0x94:
					listener->attributeChange(true, WP42_ATTRIBUTE_UNDERLINE);
					break;
				case 0x95:
					listener->attributeChange(false, WP42_ATTRIBUTE_UNDERLINE);
					break;

				case 0x90:
					listener->attributeChange(true, WP42_ATTRIBUTE_REDLINE);
					break;
				case 0x91:
					listener->attributeChange(false, WP42_ATTRIBUTE_REDLINE);
					break;

				case 0x9C:
					listener->attributeChange(false, WP42_ATTRIBUTE_BOLD);
					break;
				case 0x9D:
					listener->attributeChange(true, WP42_ATTRIBUTE_BOLD);
					break;

				case 0xB2:
					listener->attributeChange(true, WP42_ATTRIBUTE_ITALICS);
					break;
				case 0xB3:
					listener->attributeChange(false, WP42_ATTRIBUTE_ITALICS);
					break;
				case 0xB4:
					listener->attributeChange(true, WP42_ATTRIBUTE_SHADOW);
					break;
				case 0xB5:
					listener->attributeChange(false, WP42_ATTRIBUTE_SHADOW);
					break;

				default:
					// unsupported or undocumented token, ignore
					break;
			}
		}
		else if (readVal >= (uint8_t)0xC0 && readVal <= (uint8_t)0xF8)
		{
			WP42Part *part = WP42Part::constructPart(input, readVal);
			if (part != NULL)
			{
				part->parse(listener);
				DELETEP(part);
			}
		}
		// ignore the rest since they are not documented and at least 0xFF is a special character that
		// marks end of variable length part in variable length multi-byte functions
	}
}

void WP42Parser::parse(WPXHLListenerImpl *listenerImpl)
{
	WPXInputStream *input = getInput();
	std::list<WPXPageSpan> pageList;
	std::vector<WP42SubDocument *> subDocuments;	
	
	try
 	{
		// do a "first-pass" parse of the document
		// gather table border information, page properties (per-page)
		WP42StylesListener stylesListener(pageList, subDocuments);
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
		WP42ContentListener listener(pageList, subDocuments, listenerImpl);
		parse(input, &listener);

		// cleanup section: free the used resources
		for (std::vector<WP42SubDocument *>::iterator iterSubDoc = subDocuments.begin(); iterSubDoc != subDocuments.end(); iterSubDoc++)
		{
			if (*iterSubDoc)
				delete (*iterSubDoc);
		}
	}
	catch(FileException)
	{
		WPD_DEBUG_MSG(("WordPerfect: File Exception. Parse terminated prematurely."));

		for (std::vector<WP42SubDocument *>::iterator iterSubDoc = subDocuments.begin(); iterSubDoc != subDocuments.end(); iterSubDoc++)
		{
			if (*iterSubDoc)
				delete (*iterSubDoc);
		}

		throw FileException();
	}

}
