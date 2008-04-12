/* libwps
 * Copyright (C) 2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003-2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2006, 2007 Andrew Ziem (andrewziem users sourceforge net)
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

/*
 * This file is in sync with CVS
 * /libwpd2/src/lib/WPDocument.cpp 1.34
 */

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
with the libwpd's WPXDocumentInterface class, are the only two classes that will be
of interest for the application programmer using libwps.
\section lib_docs libwps documentation
If you are interrested in the structure of libwps itself, this whole document
would be a good starting point for exploring the interals of libwps. Mind that
this document is a work-in-progress, and will most likely not cover libwps for
the full 100%.
*/

/**
Analyzes the content of an input stream to see if it can be parsed
\param input The input stream
\param partialContent A boolean which states if the content from the input stream
represents the full contents of a MS Works file, or just the first X bytes
\return A confidence value which represents the likelyhood that the content from
the input stream can be parsed
*/
WPSConfidence WPSDocument::isFileFormatSupported(WPXInputStream *input)
{
	WPSConfidence confidence = WPS_CONFIDENCE_NONE;

	WPS_DEBUG_MSG(("WPSDocument::isFileFormatSupported()\n"));
	
	try
	{
		WPSHeader *header = WPSHeader::constructHeader(input);

		if (!header)
			return WPS_CONFIDENCE_NONE;

		switch (header->getMajorVersion())
		{

			case 8:
			case 7:
			case 4:
				confidence = WPS_CONFIDENCE_EXCELLENT;
				break;

			case 5:
			case 2:
				confidence = WPS_CONFIDENCE_GOOD;
				break;
		}
		DELETEP(header);

		return confidence;
	}
	catch (FileException)
	{
		WPS_DEBUG_MSG(("File exception trapped\n"));
        }
	catch (ParseException)
	{
		WPS_DEBUG_MSG(("Parse exception trapped\n"));
	}
	catch (...)
	{
		//fixme: too generic
		WPS_DEBUG_MSG(("Unknown Exception trapped\n"));
	}

	return WPS_CONFIDENCE_NONE;
}

/**
Parses the input stream content. It will make callbacks to the functions provided by a
WPXDocumentInterface class implementation when needed. This is often commonly called the
'main parsing routine'.
\param input The input stream
\param documentInterface A WPSListener implementation
*/
WPSResult WPSDocument::parse(WPXInputStream *input, WPXDocumentInterface *documentInterface)
{
	WPSResult error = WPS_OK;

	try
	{
		WPSHeader *header = WPSHeader::constructHeader(input);

		if (!header)
			return WPS_UNKNOWN_ERROR;

		switch (header->getMajorVersion())
		{

			case 8:
			case 7:
			case 6:
			case 5:
			{
				WPS8Parser *parser = new WPS8Parser(header->getInput(), header);
				parser->parse(documentInterface);
				DELETEP(parser);		
				break;
			}

			case 4:
			case 3:
			case 2:
			{
				WPS4Parser *parser = new WPS4Parser(header->getInput(), header);
				parser->parse(documentInterface);
				DELETEP(parser);	
				break;
			}
			DELETEP(header);
		}
	}
	catch (FileException)
	{
		WPS_DEBUG_MSG(("File exception trapped\n"));
		error = WPS_FILE_ACCESS_ERROR;
        }
	catch (ParseException)
	{
		WPS_DEBUG_MSG(("Parse exception trapped\n"));
                error = WPS_PARSE_ERROR;
	}
	catch (...)
	{
		//fixme: too generic
		WPS_DEBUG_MSG(("Unknown exception trapped\n"));
		error = WPS_UNKNOWN_ERROR;
	}

	return error;
}
