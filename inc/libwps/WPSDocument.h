/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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


#ifndef WPSDOCUMENT_H
#define WPSDOCUMENT_H

#include <librevenge/librevenge.h>

#ifdef DLL_EXPORT
#ifdef BUILD_WPS
#define WPSLIB __declspec(dllexport)
#else
#define WPSLIB __declspec(dllimport)
#endif
#else // !DLL_EXPORT
#ifdef LIBWPS_VISIBILITY
#define WPSLIB __attribute__((visibility("default")))
#else
#define WPSLIB
#endif
#endif


namespace libwps
{

enum WPSConfidence { WPS_CONFIDENCE_NONE=0, WPS_CONFIDENCE_EXCELLENT, WPS_CONFIDENCE_SUPPORTED_ENCRYPTION };
enum WPSCreator { WPS_MSWORKS=0 /**< Microsoft Works documents (all wps, wks and wdb) */,
                  WPS_LOTUS /**Lotus DOS(Wk1), Apple(Lotus 123 v1), Windows(Wk3,Wk4) spreadsheets*/,
                  WPS_QUATTRO_PRO /**Quattro Pro Wq1 and Wq2 spreadsheets(minimum filter)*/,
                  WPS_SYMPHONY /**Lotus Symphony files(untested, probably DOS documents)*/,
                  WPS_RESERVED_0, WPS_RESERVED_1, WPS_RESERVED_2,
                  WPS_RESERVED_3, WPS_RESERVED_4, WPS_RESERVED_5,
                  WPS_RESERVED_6, WPS_RESERVED_7, WPS_RESERVED_8,
                  WPS_MSWRITE=WPS_RESERVED_0 /** Microsoft Write documents (wri) */
                };
enum WPSKind { WPS_TEXT=0, WPS_SPREADSHEET, WPS_DATABASE };
enum WPSResult { WPS_OK, WPS_ENCRYPTION_ERROR, WPS_FILE_ACCESS_ERROR, WPS_PARSE_ERROR, WPS_OLE_ERROR, WPS_UNKNOWN_ERROR };

/**
This class provides all the functions an application would need to parse Works documents.
*/
class WPSDocument
{
public:
	/** Analyzes the content of an input stream to see if it can be parsed.
		\param input The input stream
		\param kind The document kind
		\param creator The document creator
		\param needCharSetEncoding A flag set to true if we need the character set encoding

		\return A confidence value which represents the likelyhood that the content from
		the input stream can be parsed:
		- WPS_CONFIDENCE_NONE if the file is not supported,
		- WPS_CONFIDENCE_EXCELLENT if the file is supported,
		- WPS_CONFIDENCE_SUPPORTED_ENCRYTION if the file is supported but
		we need to have the password

		\note
		- as Dos/Windows 3 File does not seems to contain the char set encoding,
		the function can accept the following hint encoding CP424, CP437,
		CP737, CP775, CP850, CP852, CP855, CP856, CP857, CP860, CP861,
		CP862, CP863, CP864, CP865, CP866, CP869, CP874, CP932, CP950, CP1006,
		CP1250, CP1251, CP1252, CP1253, CP1254, CP1255, CP1256, CP1257, CP1258.
		- If no encoding is given, CP850 or CP1250 will be used.
	*/
	static WPSLIB WPSConfidence isFileFormatSupported(librevenge::RVNGInputStream *input, WPSKind &kind, WPSCreator &creator, bool &needCharSetEncoding);

	/**
	   Parses the input stream content. It will make callbacks to the functions provided by a
	   librevenge::RVNGTextInterface class implementation when needed. This is often commonly called the
	   'main parsing routine'.
	   \param input The input stream
	   \param documentInterface A librevenge::RVNGTextInterface implementation
	   \param password the file password
	   \param encoding the encoding
	*/
	static WPSLIB WPSResult parse(librevenge::RVNGInputStream *input, librevenge::RVNGTextInterface *documentInterface,
	                              char const *password="", char const *encoding="");
	/**
	   Parses the input stream content. It will make callbacks to the functions provided by a
	   librevenge::RVNGSpreadsheetInterface class implementation when needed. This is often commonly called the
	   'main parsing routine'.
	   \param input The input stream
	   \param documentInterface A librevenge::RVNGSpreadsheetInterface implementation
	   \param password the file password
	   \param encoding the encoding
	*/
	static WPSLIB WPSResult parse(librevenge::RVNGInputStream *input, librevenge::RVNGSpreadsheetInterface *documentInterface,
	                              char const *password="", char const *encoding="");
};

} // namespace libwps

#endif /* WPSDOCUMENT_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
