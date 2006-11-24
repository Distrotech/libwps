/* libwps
 * Copyright (C) 2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003-2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2006 Andrew Ziem (andrewziem users sourceforge net)
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
 * For further information visit http://libwps.sourceforge.net
 */


#include <stdlib.h>
#include <string.h>
#include "WPSDocument.h"
#include "WPS4.h"
#include "WPS8.h"
#include "WPSHeader.h"
#include "WPSParser.h"
#include "libwps_internal.h"

/**
\mainpage libwps documentation
This document contains both the libwps API specification and the normal libwps
documentation.
\section api_docs libwps API documentation
The external libwps API is provided by the WPSDocument class. This class, combined
with the WPXHLListenerImpl class, are the only two classes that will be of interest
for the application programmer using libwps.
\section lib_docs libwps documentation
If you are interrested in the structure of libwps itself, this whole document
would be a good starting point for exploring the interals of libwps. Mind that
this document is a work-in-progress, and will most likely not cover libwps for
the full 100%.
*/

/**
Check for Microsoft Works support.
**/
WPDConfidence WPSDocument::isFileFormatSupportedWPS(libwps::WPSInputStream *input, bool partialContent)
{
	libwps::WPSInputStream * document_mn0 = input->getWPSOleStream("MN0");	
	
	//fixme: catch exceptions
	if (document_mn0)
	{
		WPS_DEBUG_MSG(("Microsoft Works v4 format detected\n"));	
		DELETEP(document_mn0);		
		return WPD_CONFIDENCE_EXCELLENT;
	}	
	
	libwps::WPSInputStream * document_contents = input->getWPSOleStream("CONTENTS");	
	
	if (document_contents)
	{
		/* check the Works 2000/7/8 format magic */
		document_contents->seek(0);
		
		char fileMagic[8];
		for (int i=0; i<7 && !document_contents->atEnd(); i++)
			fileMagic[i] = readU8(document_contents);		
		fileMagic[7] = '\0';
		
		DELETEP(document_contents);
	
		/* Works 7/8 */
		if (0 == strcmp(fileMagic, "CHNKWKS"))
		{
			WPS_DEBUG_MSG(("Microsoft Works v8 (maybe 7) format detected\n"));
			return WPD_CONFIDENCE_EXCELLENT;
		}
		
		/* Works 2000 */		
		if (0 == strcmp(fileMagic, "CHNKINK"))
		{
			WPS_DEBUG_MSG(("Microsoft Works 2000 (v5) format detected\n"));
		}	
	}

	input->seek(0);
	if (readU8(input) < 6 && 0xFE == readU8(input))
	{
		WPS_DEBUG_MSG(("Microsoft Works v2 format detected\n"));
		return WPD_CONFIDENCE_EXCELLENT;
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
WPDConfidence WPSDocument::isFileFormatSupported(libwps::WPSInputStream *input, bool partialContent)
{
	WPDConfidence confidence = WPD_CONFIDENCE_NONE;

	WPS_DEBUG_MSG(("WPSDocument::isFileFormatSupported()\n"));
	
	confidence = isFileFormatSupportedWPS(input, partialContent);

	return confidence;
}


/**
Parse a Works document.
*/
WPDResult WPSDocument::parseWPS(libwps::WPSInputStream *input, WPXHLListenerImpl *listenerImpl)
{
	WPDResult error = WPD_OK;

	libwps::WPSInputStream * document_mn0 = input->getWPSOleStream("MN0");		
	libwps::WPSInputStream * document_contents = input->getWPSOleStream("CONTENTS");
	input->seek(0);
	uint8_t byte1 = readU8(input);
	uint8_t byte2 = readU8(input);
	
	//fixme: catch exceptions
	if (document_mn0)
	{
		WPS_DEBUG_MSG(("Microsoft Works v4 format detected\n"));	
		WPS4Parser *parser = new WPS4Parser(document_mn0, NULL);
		parser->parse(listenerImpl);
		DELETEP(parser);	
	}
	else if (0xFE == byte2 && byte1 < 7)
	{
		WPS_DEBUG_MSG(("Microsoft Works v2 format detected\n"));	
		WPS4Parser *parser = new WPS4Parser(input, NULL);
		parser->parse(listenerImpl);
		DELETEP(parser);	
	}
	else if (document_contents)
	{
		/* check the Works 2000/7/8 format magic */
		document_contents->seek(0);
		
		char fileMagic[8];
		for (int i=0; i<7 && !document_contents->atEnd(); i++)
			fileMagic[i] = readU8(document_contents);		
		fileMagic[7] = '\0';
			
		/* Works 7/8 */
		if (0 == strcmp(fileMagic, "CHNKWKS"))
		{
			WPS_DEBUG_MSG(("Microsoft Works v8 (maybe 7) format detected\n"));
			WPS8Parser *parser = new WPS8Parser(document_contents, NULL);
			parser->parse(listenerImpl);
			DELETEP(parser);		
		}
		else
			error = WPD_UNKNOWN_ERROR; //fixme too generic

	}
	else
		error = WPD_UNKNOWN_ERROR; //fixme too generic

	DELETEP(document_mn0);				
	DELETEP(document_contents);		
		
	return error;
}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
WPXHLListenerImpl class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param listenerImpl A WPSListener implementation
*/
WPDResult WPSDocument::parse(libwps::WPSInputStream *input, WPXHLListenerImpl *listenerImpl)
{
	WPDResult error = parseWPS(input, listenerImpl);
	
	return error;
}
