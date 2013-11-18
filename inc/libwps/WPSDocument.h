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

#ifdef DLL_EXPORT
#ifdef BUILD_WPS
#define WPSLIB _declspec(dllexport)
#else
#define WPSLIB _declspec(dllimport)
#endif
#else
#define WPSLIB
#endif

enum WPSConfidence { WPS_CONFIDENCE_NONE=0, WPS_CONFIDENCE_EXCELLENT };
enum WPSKind { WPS_TEXT=0, WPS_SPREADSHEET, WPS_DATABASE };
enum WPSResult { WPS_OK, WPS_FILE_ACCESS_ERROR, WPS_PARSE_ERROR, WPS_OLE_ERROR, WPS_UNKNOWN_ERROR };

namespace librevenge
{
class RVNGInputStream;
class RVNGTextInterface;
class RVNGSpreadsheetInterface;
}

/**
This class provides all the functions an application would need to parse
Works documents.
*/

class WPSDocument
{
public:
	static WPSLIB WPSConfidence isFileFormatSupported(librevenge::RVNGInputStream *input, WPSKind &kind);
	static WPSLIB WPSResult parse(librevenge::RVNGInputStream *input, librevenge::RVNGTextInterface *documentInterface);
	static WPSLIB WPSResult parse(librevenge::RVNGInputStream *input, librevenge::RVNGSpreadsheetInterface *documentInterface);
};

#endif /* WPSDOCUMENT_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
