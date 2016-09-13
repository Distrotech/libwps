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

#include <cstdlib>
#include <fstream>
#include <iostream>

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
	printf("`wks2csv' converts various spreadsheets and database formats to CSV.\n");
	printf("Supported formats are Microsoft Works Spreadsheet and Database, Lotus Wk1-Wk4 and Quattro Pro Wq1-Wq2.\n");
	printf("\n");
	printf("Usage: wks2csv [OPTION] FILE\n");
	printf("\n");
	printf("Options:\n");
	printf("\t-h           show this help message\n");
	printf("\t-d CHAR      set the decimal commas to character CHAR: default .\n");
	printf("\t-e ENCODING         Define the file encoding where encoding can be\n");
	printf("\t\t CP037, CP424, CP437, CP737, CP500, CP775, CP850, CP852, CP855, CP856, CP857,\n");
	printf("\t\t CP860, CP861, CP862, CP863, CP864, CP865, CP866, CP869, CP874, CP875, CP932,\n");
	printf("\t\t CP950, CP1006, CP1026, CP1250, CP1251, CP1252, CP1253, CP1254, CP1255, CP1256,\n");
	printf("\t\t CP1257, CP1258, MacArabic, MacCeltic, MacCEurope, MacCroation, MacCyrillic,\n");
	printf("\t\t MacDevanage, MacFarsi, MacGaelic, MacGreek, MacGujarati, MacGurmukhi, MacHebrew,\n");
	printf("\t\t MacIceland, MacInuit, MacRoman, MacRomanian, MacThai, MacTurkish.\n");
	printf("\t-f CHAR      Sets the field separator to character CHAR: default ,\n");
	printf("\t-t CHAR      set the text separator to character CHAR: default \"\n");
	printf("\t-o FILE      set the ouput file\n");
	printf("\t-p PASSWORD  set password to open the file\n");
	printf("\t-v           show version information\n");
	printf("\t-N           show the number of sheets\n");
	printf("\t-n NUM       choose the sheet to convert (1 means first sheet) \n");
	printf("\t-F           set to output the formula which exists in the file\n");
	printf("\t-D FORMAT    set date format: default \"%%m/%%d/%%y\"\n");
	printf("\t-T FORMAT    set time format: default \"%%H:%%M:%%S\"\n");
	printf("\n");
	printf("Examples:\n");
	printf("\twks2cvs -d, -D\"%%d/%%m/%%y\" file	convert a file using french locale\n");
	printf("\n");
	printf("Note:\n");
	printf("\tIf -F is present, the formula are generated which english names\n");
	printf("\n");
	printf("Report bugs to <https://sourceforge.net/p/libwps/bugs/> or <https://bugs.documentfoundation.org/>.\n");
	return -1;
}

static int printVersion()
{
	printf("wks2csv %s\n", VERSION);
	return 0;
}

int main(int argc, char *argv[])
{
	bool printHelp=false;
	bool printNumberOfSheet=false;
	bool generateFormula=false;
	char const *encoding="";
	char const *password=0;
	char const *output = 0;
	int sheetToConvert=0;
	int ch;
	char decSeparator='.', fieldSeparator=',', textSeparator='"';
	std::string dateFormat("%m/%d/%y"), timeFormat("%H:%M:%S");

	while ((ch = getopt(argc, argv, "e:hvo:d:f:p:t:D:Nn:FT:")) != -1)
	{
		switch (ch)
		{
		case 'D':
			dateFormat=optarg;
			break;
		case 'F':
			generateFormula=true;
			break;
		case 'N':
			printNumberOfSheet=true;
			break;
		case 'T':
			timeFormat=optarg;
			break;
		case 'd':
			decSeparator=optarg[0];
			break;
		case 'e':
			encoding=optarg;
			break;
		case 'f':
			fieldSeparator=optarg[0];
			break;
		case 'n':
			sheetToConvert=std::atoi(optarg);
			break;
		case 'o':
			output=optarg;
			break;
		case 'p':
			password=optarg;
			break;
		case 't':
			textSeparator=optarg[0];
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

	WPSResult error=WPS_OK;
	librevenge::RVNGStringVector vec;

	try
	{
		librevenge::RVNGCSVSpreadsheetGenerator listenerImpl(vec, generateFormula);
		listenerImpl.setSeparators(fieldSeparator, textSeparator, decSeparator);
		listenerImpl.setDTFormats(dateFormat.c_str(),timeFormat.c_str());
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
	else if (sheetToConvert>0 && sheetToConvert>(int) vec.size())
	{
		fprintf(stderr, "ERROR: Can not find sheet %d\n", sheetToConvert);
		return 1;
	}

	if (printNumberOfSheet)
	{
		std::cout << vec.size() << "\n";
		return 0;
	}
	if (!output)
		std::cout << vec[sheetToConvert>0 ? unsigned(sheetToConvert-1) : 0].cstr() << std::endl;
	else
	{
		std::ofstream out(output);
		out << vec[sheetToConvert>0 ? unsigned(sheetToConvert-1) : 0].cstr() << std::endl;
	}
	return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
