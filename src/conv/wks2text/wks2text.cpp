/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
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
#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include <librevenge/librevenge.h>
#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>

#include <libwps/libwps.h>

#include "helper.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef VERSION
#define VERSION "UNKNOWN VERSION"
#endif

using namespace libwps;

static int printUsage()
{
	printf("`wks2text' converts various spreadsheets and database formats to plain text.\n");
	printf("Supported formats are Microsoft Works Spreadsheet and Database, Lotus Wk1-Wk4 and Quattro Pro Wq1-Wq2.\n");
	printf("\n");
	printf("Usage: wks2text [OPTION] FILE\n");
	printf("\n");
	printf("Options:\n");
	printf("\t-e ENCODING         define the file encoding, where encoding can be\n");
	printf("\t\t CP037, CP424, CP437, CP737, CP500, CP775, CP850, CP852, CP855, CP856, CP857,\n");
	printf("\t\t CP860, CP861, CP862, CP863, CP864, CP865, CP866, CP869, CP874, CP875, CP1006, CP1026,\n");
	printf("\t\t CP1250, CP1251, CP1252, CP1253, CP1254, CP1255, CP1256, CP1257, CP1258,\n");
	printf("\t\t MacArabic, MacCeltic, MacCEurope, MacCroation, MacCyrillic, MacDevanage,\n");
	printf("\t\t MacFarsi, MacGaelic, MacGreek, MacGujarati, MacGurmukhi, MacHebrew, MacIceland,\n");
	printf("\t\t MacInuit, MacRoman, MacRomanian, MacThai, MacTurkish.\n");
	printf("\t-h                  show this help message\n");
	printf("\t-o FILE             set the output file\n");
	printf("\t-p PASSWORD         set password to open the file\n");
	printf("\t-v                  show version information\n");
	printf("\n");
	printf("Report bugs to <https://sourceforge.net/p/libwps/bugs/> or <https://bugs.documentfoundation.org/>.\n");
	return -1;
}

static int printVersion()
{
	printf("wks2text %s\n", VERSION);
	return 0;
}

int main(int argc, char *argv[])
{
	bool printHelp=false;
	char const *encoding="";
	char const *password=0;
	char const *output = 0;
	int ch;

	while ((ch = getopt(argc, argv, "hvo:p:")) != -1)
	{
		switch (ch)
		{
		case 'e':
			encoding=optarg;
			break;
		case 'o':
			output=optarg;
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
	char const *file=argv[optind];
	WPSConfidence confidence;
	WPSKind kind;
	bool needCharEncoding;
	shared_ptr<librevenge::RVNGInputStream> input=libwpsHelper::isSupported(file,confidence, kind,needCharEncoding);
	if (!input || confidence == WPS_CONFIDENCE_NONE || (kind != WPS_SPREADSHEET && kind != WPS_DATABASE))
	{
		printf("ERROR: Unsupported file format!\n");
		return 1;
	}

	librevenge::RVNGStringVector vec;
	WPSResult error=WPS_OK;
	try
	{
		librevenge::RVNGTextSpreadsheetGenerator listenerImpl(vec);
		error= WPSDocument::parse(input.get(), &listenerImpl, password, encoding);
	}
	catch (...)
	{
		error=WPS_PARSE_ERROR;
	}

	if (libwpsHelper::checkErrorAndPrintMessage(error))
		return 1;
	else if (vec.empty())
	{
		fprintf(stderr, "ERROR: bad output!\n");
		return 1;
	}

	if (!output)
	{
		for (unsigned i=0; i<vec.size(); ++i)
		{
			if (i)
				std::cout << "\n\t############# Sheet " << i+1 << " ################\n\n";
			std::cout << vec[i].cstr() << std::endl;
		}
	}
	else
	{
		std::ofstream out(output);
		for (unsigned i=0; i<vec.size(); ++i)
		{
			if (i)
				out << "\n\t############# Sheet " << i+1 << " ################\n\n";
			out << vec[i].cstr() << std::endl;
		}
	}
	return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
