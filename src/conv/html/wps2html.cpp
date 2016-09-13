/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002-2004 Marc Maurer (uwog@uwog.net)
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

#include <stdio.h>
#include <unistd.h>

#include <cstring>

#include <librevenge/librevenge.h>
#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>

#include <libwps/libwps.h>

#include "helper.h"

using namespace libwps;

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef VERSION
#define VERSION "UNKNOWN VERSION"
#endif

static int printUsage()
{
	printf("`wps2html' converts various Microsoft text document formats to HTML.\n");
	printf("Supported formats are Microsoft Works, Write and Word for DOS.\n");
	printf("\n");
	printf("Usage: wps2html [OPTION] FILE\n");
	printf("\n");
	printf("Options:\n");
	printf("\t-e ENCODING       define the file encoding, where ENCODING can be\n");
	printf("\t\t CP037, CP424, CP437, CP737, CP500, CP775, CP850, CP852, CP855, CP856, CP857,\n");
	printf("\t\t CP860, CP861, CP862, CP863, CP864, CP865, CP866, CP869, CP874, CP875, CP932,\n");
	printf("\t\t CP950, CP1006, CP1026, CP1250, CP1251, CP1252, CP1253, CP1254, CP1255, CP1256,\n");
	printf("\t\t CP1257, CP1258, MacArabic, MacCeltic, MacCEurope, MacCroation, MacCyrillic,\n");
	printf("\t\t MacDevanage, MacFarsi, MacGaelic, MacGreek, MacGujarati, MacGurmukhi, MacHebrew,\n");
	printf("\t\t MacIceland, MacInuit, MacRoman, MacRomanian, MacThai, MacTurkish.\n");
	printf("\t-h                show this help message\n");
	printf("\t-p PASSWORD       set password to open the file\n");
	printf("\t-v                show version information\n");
	printf("\n");
	printf("Report bugs to <https://sourceforge.net/p/libwps/bugs/> or <https://bugs.documentfoundation.org/>.\n");
	return -1;
}

static int printVersion()
{
	printf("wps2html %s\n", VERSION);
	return 0;
}

int main(int argc, char *argv[])
{
	bool printHelp=false;
	int ch;
	char const *encoding="";
	char const *password=0;

	while ((ch = getopt(argc, argv, "e:hp:v")) != -1)
	{
		switch (ch)
		{
		case 'e':
			encoding=optarg;
			break;
		case 'p':
			password=optarg;
			break;
		case 'v':
			printVersion();
			return 0;
		default:
		case 'h':
			printHelp = true;
			break;
		}
	}
	if (argc != 1+optind || printHelp)
	{
		printUsage();
		return -1;
	}

	librevenge::RVNGFileStream input(argv[optind]);

	WPSCreator creator;
	WPSKind kind;
	bool needCharEncoding;
	WPSConfidence confidence = WPSDocument::isFileFormatSupported(&input,kind,creator,needCharEncoding);
	if (confidence == WPS_CONFIDENCE_NONE || kind != WPS_TEXT)
	{
		printf("ERROR: Unsupported file format!\n");
		return 1;
	}

	librevenge::RVNGString document;
	librevenge::RVNGHTMLTextGenerator listenerImpl(document);
	WPSResult error = WPSDocument::parse(&input, &listenerImpl, password, encoding);

	if (libwpsHelper::checkErrorAndPrintMessage(error))
		return 1;

	printf("%s", document.cstr());

	return 0;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
