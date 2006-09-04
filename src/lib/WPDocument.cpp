/* libwpd
 * Copyright (C) 2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003-2004 Marc Maurer (uwog@uwog.net)
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

#include <stdlib.h>
#include <string.h>
#include "WPDocument.h"
#include "WPXHeader.h"
#include "WPXParser.h"
#include "WPS4.h"
#include "WPXStream.h"
#include "libwpd_internal.h"

/**
\mainpage libwpd documentation
This document contains both the libwpd API specification and the normal libwpd
documentation.
\section api_docs libwpd API documentation
The external libwpd API is provided by the WPDocument class. This class, combined
with the WPXHLListenerImpl class, are the only two classes that will be of interest
for the application programmer using libwpd.
\section lib_docs libwpd documentation
If you are interrested in the structure of libwpd itself, this whole document
would be a good starting point for exploring the interals of libwpd. Mind that
this document is a work-in-progress, and will most likely not cover libwpd for
the full 100%.
*/

/**
Check for Microsoft Works support.
**/
WPDConfidence WPDocument::isFileFormatSupportedWPS(WPXInputStream *input, bool partialContent)
{
	WPXInputStream * document_mn0 = input->getDocumentOLEStream("MN0");	
	
	//fixme: catch exceptions
	if (document_mn0)
	{
		WPD_DEBUG_MSG(("Microsoft Works 4 format detected\n"));	
		DELETEP(document_mn0);		
		return WPD_CONFIDENCE_EXCELLENT;
	}	
	
	WPXInputStream * document_contents = input->getDocumentOLEStream("CONTENTS");	
	
	if (document_contents)
	{
		/* check the Works 2000/7/8 format magic */
		document_contents->seek(0, WPX_SEEK_SET);
		
		char fileMagic[8];
		size_t numBytesRead;
		for (int i=0; i<7 && !document_contents->atEOS(); i++)
			fileMagic[i] = readU8(document_contents);		
		fileMagic[7] = '\0';
	
		/* Works 7/8 */
		if (0 == strcmp(fileMagic, "CHNKWKS"))
		{
			WPD_DEBUG_MSG(("Microsoft Works 8 (maybe 7) format detected\n"));
		}
		
		/* Works 2000 */		
		if (0 == strcmp(fileMagic, "CHNKINK"))
		{
			WPD_DEBUG_MSG(("Microsoft Works 2000 (v5) format detected\n"));
		}	
		DELETEP(document_contents);
	}
	
	return WPD_CONFIDENCE_NONE;
}

/**
Analyzes the content of an input stream to see if it can be parsed
\param input The input stream
\param partialContent A boolean which states if the content from the input stream
represents the full contents of a WordPerfect file, or just the first X bytes
\return A confidence value which represents the likelyhood that the content from
the input stream can be parsed
*/
WPDConfidence WPDocument::isFileFormatSupported(WPXInputStream *input, bool partialContent)
{
	WPDConfidence confidence = WPD_CONFIDENCE_NONE;

	WPD_DEBUG_MSG(("WPDocument::isFileFormatSupported()\n"));
	
	confidence = isFileFormatSupportedWPS(input, partialContent);

	return confidence;
}


/**
Parse a Works document.
*/
WPDResult WPDocument::parseWPS(WPXInputStream *input, WPXHLListenerImpl *listenerImpl)
{
	WPXInputStream * document_mn0 = input->getDocumentOLEStream("MN0");	
	
	WPDResult error = WPD_OK;
	
	//fixme: catch exceptions
	if (document_mn0)
	{
		WPD_DEBUG_MSG(("Microsoft Works 4 format detected\n"));	
		WPS4Parser *parser = new WPS4Parser(document_mn0, NULL);
		parser->parse(listenerImpl);
		DELETEP(parser);		
		DELETEP(document_mn0);		
	}	
	else
		error = WPD_UNKNOWN_ERROR; //fixme too generic
		
	return error;
}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
WPXHLListenerImpl class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param listenerImpl A WPXListener implementation
*/
WPDResult WPDocument::parse(WPXInputStream *input, WPXHLListenerImpl *listenerImpl)
{
	WPDResult error = parseWPS(input, listenerImpl);
	
	return error;
}
