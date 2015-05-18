/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003-2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2006, 2007 Andrew Ziem (andrewziem users sourceforge net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwps.sourceforge.net
 */

#include <libwps/libwps.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "Lotus.h"
#include "Quattro.h"
#include "WKS4.h"
#include "WPS4.h"
#include "WPS8.h"
#include "WPSHeader.h"
#include "WPSParser.h"

using namespace libwps;

/**
\mainpage libwps documentation
This document contains both the libwps API specification and the normal libwps
documentation.
\section api_docs libwps API documentation
The external libwps API is provided by the WPSDocument class. This class, combined
with the librevenge's librevenge::RVNGTextInterface class, are the only two classes that will be
of interest for the application programmer using libwps.
\section lib_docs libwps documentation
If you are interrested in the structure of libwps itself, this whole document
would be a good starting point for exploring the interals of libwps. Mind that
this document is a work-in-progress, and will most likely not cover libwps for
the full 100%.

 \warning When compiled with -DDEBUG_WITH__FILES, code is added to store the results of the parsing in different files: one file by Ole parts and some files to store the read pictures. These files are created in the current repository, therefore it is recommended to launch the tests in an empty repository...*/

WPSLIB WPSConfidence WPSDocument::isFileFormatSupported(librevenge::RVNGInputStream *ip, WPSKind &kind, WPSCreator &creator, bool &needEncoding)
{
	WPS_DEBUG_MSG(("WPSDocument::isFileFormatSupported()\n"));

	if (!ip)
		return WPS_CONFIDENCE_NONE;

	kind=WPS_TEXT;
	WPSHeaderPtr header;
	shared_ptr<librevenge::RVNGInputStream > input(ip, WPS_shared_ptr_noop_deleter<librevenge::RVNGInputStream>());
	try
	{
		header.reset(WPSHeader::constructHeader(input));

		if (!header)
			return WPS_CONFIDENCE_NONE;
		creator = header->getCreator();
		kind = header->getKind();

		WPSConfidence confidence = WPS_CONFIDENCE_NONE;
		if (kind==WPS_TEXT && header->getMajorVersion()<=4)
		{
			// create a WPS4Parser to check the header validity
			WPS4Parser parser(header->getInput(), header);
			if (!parser.checkHeader(header.get(), true))
				return WPS_CONFIDENCE_NONE;
			needEncoding=header->getNeedEncoding();
			return WPS_CONFIDENCE_EXCELLENT;
		}
		else if (kind==WPS_SPREADSHEET && creator==WPS_LOTUS && header->getMajorVersion()>=100)
		{
			// create a Lotus to check the header validity
			LotusParser parser(header->getInput(), header);
			if (!parser.checkHeader(header.get(), true))
				return WPS_CONFIDENCE_NONE;
			needEncoding=header->getNeedEncoding();
			return WPS_CONFIDENCE_EXCELLENT;
		}
		else if (kind==WPS_SPREADSHEET && creator==WPS_QUATTRO_PRO && header->getMajorVersion()<=2)
		{
			// create a Quattro to check the header validity
			QuattroParser parser(header->getInput(), header);
			if (!parser.checkHeader(header.get(), true))
				return WPS_CONFIDENCE_NONE;
			needEncoding=header->getNeedEncoding();
			return WPS_CONFIDENCE_EXCELLENT;
		}
		else if (kind==WPS_SPREADSHEET || kind==WPS_DATABASE)
		{
			// create a WKS4Parser to check the header validity
			WKS4Parser parser(header->getInput(), header);
			if (!parser.checkHeader(header.get(), true))
				return WPS_CONFIDENCE_NONE;
			needEncoding=header->getNeedEncoding();
			return WPS_CONFIDENCE_EXCELLENT;
		}

		/* A word document: as WPS8Parser does not have a checkHeader
		   function, only rely on the version
		 */
		switch (header->getMajorVersion())
		{
		case 8:
		case 7:
		case 5:
			confidence = WPS_CONFIDENCE_EXCELLENT;
			break;
		default:
			break;
		}
		return confidence;
	}
	catch (libwps::FileException)
	{
		WPS_DEBUG_MSG(("File exception trapped\n"));
	}
	catch (libwps::ParseException)
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

WPSLIB WPSResult WPSDocument::parse(librevenge::RVNGInputStream *ip, librevenge::RVNGTextInterface *documentInterface,
                                    char const * /*password*/, char const *encoding)
{
	if (!ip || !documentInterface)
		return WPS_UNKNOWN_ERROR;

	WPSResult error = WPS_OK;

	WPSHeaderPtr header;
	shared_ptr<WPSParser> parser;
	shared_ptr<librevenge::RVNGInputStream > input(ip, WPS_shared_ptr_noop_deleter<librevenge::RVNGInputStream>());
	try
	{
		header.reset(WPSHeader::constructHeader(input));

		if (!header || header->getKind() != WPS_TEXT)
			return WPS_UNKNOWN_ERROR;

		switch (header->getMajorVersion())
		{
		case 8:
		case 7:
		case 6:
		case 5:
		{
			parser.reset(new WPS8Parser(header->getInput(), header));
			if (!parser) return WPS_UNKNOWN_ERROR;
			parser->parse(documentInterface);
			break;
		}

		case 4:
		case 3:
		case 2:
		case 1:
		{
			parser.reset(new WPS4Parser(header->getInput(), header,
			                            libwps_tools_win::Font::getTypeForString(encoding)));
			if (!parser) return WPS_UNKNOWN_ERROR;
			parser->parse(documentInterface);
			break;
		}
		default:
			break;
		}
	}
	catch (libwps::FileException)
	{
		WPS_DEBUG_MSG(("File exception trapped\n"));
		error = WPS_FILE_ACCESS_ERROR;
	}
	catch (libwps::ParseException)
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

WPSLIB WPSResult WPSDocument::parse(librevenge::RVNGInputStream *ip, librevenge::RVNGSpreadsheetInterface *documentInterface,
                                    char const * /*password*/, char const *encoding)
{
	if (!ip || !documentInterface)
		return WPS_UNKNOWN_ERROR;

	WPSResult error = WPS_OK;

	WPSHeaderPtr header;
	shared_ptr<WKSParser> parser;
	shared_ptr<librevenge::RVNGInputStream > input(ip, WPS_shared_ptr_noop_deleter<librevenge::RVNGInputStream>());
	try
	{
		header.reset(WPSHeader::constructHeader(input));

		if (!header || (header->getKind() != WPS_SPREADSHEET && header->getKind() != WPS_DATABASE))
			return WPS_UNKNOWN_ERROR;

		if (header->getKind() == WPS_SPREADSHEET && header->getCreator() == WPS_LOTUS &&
		        header->getMajorVersion()>=100)
		{
			parser.reset(new LotusParser(header->getInput(), header,
			                             libwps_tools_win::Font::getTypeForString(encoding)));
			if (!parser) return WPS_UNKNOWN_ERROR;
			parser->parse(documentInterface);
		}
		else if (header->getKind() == WPS_SPREADSHEET && header->getCreator() == WPS_QUATTRO_PRO &&
		         header->getMajorVersion()<=2)
		{
			parser.reset(new QuattroParser(header->getInput(), header,
			                               libwps_tools_win::Font::getTypeForString(encoding)));
			if (!parser) return WPS_UNKNOWN_ERROR;
			parser->parse(documentInterface);
		}
		else
		{
			switch (header->getMajorVersion())
			{
			case 4:
			case 3:
			case 2:
			case 1:
			{
				parser.reset(new WKS4Parser(header->getInput(), header,
				                            libwps_tools_win::Font::getTypeForString(encoding)));
				if (!parser) return WPS_UNKNOWN_ERROR;
				parser->parse(documentInterface);
				break;
			}
			default:
				WPS_DEBUG_MSG(("WPSDocument::parse: find unknown version number\n"));
				error=WPS_UNKNOWN_ERROR;
				break;
			}
		}
	}
	catch (libwps::FileException)
	{
		WPS_DEBUG_MSG(("File exception trapped\n"));
		error = WPS_FILE_ACCESS_ERROR;
	}
	catch (libwps::ParseException)
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
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
