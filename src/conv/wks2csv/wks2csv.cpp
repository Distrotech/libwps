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

using namespace libwps;

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef VERSION
#define VERSION "UNKNOWN VERSION"
#endif

static int printUsage()
{
	printf("Usage: wks2csv [OPTION] <Works Spreadsheet Document>\n");
	printf("\n");
	printf("Options:\n");
	printf("\t-h:          Shows this help message\n");
	printf("\t-dc:         Sets the decimal commas to character c: default .\n");
	printf("\t-e \"encoding\":    Define the file encoding where encoding can be\n");
	printf("\t\t CP424, CP437, CP737, CP775, CP850, CP852, CP855, CP856, CP857,\n");
	printf("\t\t CP860, CP861, CP862, CP863, CP864, CP865, CP866, CP869, CP874, CP1006,\n");
	printf("\t\t CP1250, CP1251, CP1252, CP1253, CP1254, CP1255, CP1256, CP1257, CP1258,\n");
	printf("\t\t MacRoman, MacSymbol.\n");
	printf("\t-fc:         Sets the field separator to character c: default ,\n");
	printf("\t-tc:         Sets the text separator to character c: default \"\n");
	printf("\t-o file.csv: Defines the ouput file\n");
	printf("\t-p password: Password to open the file\n");
	printf("\t-v:          Output wks2csv version\n");
	printf("\t-F:          Sets to output the formula which exists in the file\n");
	printf("\t-Dformat:    Sets the date format: default \"%%m/%%d/%%y\"\n");
	printf("\t-Tformat:    Sets the time format: default \"%%H:%%M:%%S\"\n");
	printf("\n");
	printf("Example:\n");
	printf("\twks2cvs -d, -D\"%%d/%%m/%%y\" file : Converts a file using french locale\n");
	printf("\n");
	printf("Note:\n");
	printf("\t If -F is present, the formula are generated which english names\n");
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
	bool generateFormula=false;
	char const *encoding="";
	char const *password=0;
	char const *output = 0;
	int ch;
	char decSeparator='.', fieldSeparator=',', textSeparator='"';
	std::string dateFormat("%m/%d/%y"), timeFormat("%H:%M:%S");

	while ((ch = getopt(argc, argv, "e:hvo:d:f:p:t:D:FT:")) != -1)
	{
		switch (ch)
		{
		case 'D':
			dateFormat=optarg;
			break;
		case 'F':
			generateFormula=true;
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
		case 't':
			textSeparator=optarg[0];
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
	librevenge::RVNGFileStream input(file);

	WPSCreator creator;
	WPSKind kind;
	bool needCharEncoding;
	WPSConfidence confidence = WPSDocument::isFileFormatSupported(&input,kind,creator,needCharEncoding);
	if (confidence == WPS_CONFIDENCE_NONE || (kind != WPS_SPREADSHEET && kind != WPS_DATABASE))
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
		error= WPSDocument::parse(&input, &listenerImpl, password, encoding);
	}
	catch (...)
	{
		error=WPS_PARSE_ERROR;
	}

	if (error == WPS_ENCRYPTION_ERROR)
		fprintf(stderr, "ERROR: Encrypted file, bad Password!\n");
	else if (error == WPS_FILE_ACCESS_ERROR)
		fprintf(stderr, "ERROR: File Exception!\n");
	else if (error == WPS_PARSE_ERROR)
		fprintf(stderr, "ERROR: Parse Exception!\n");
	else if (error == WPS_OLE_ERROR)
		fprintf(stderr, "ERROR: File is an OLE document, but does not contain a Works stream!\n");
	else if (error != WPS_OK)
		fprintf(stderr, "ERROR: Unknown Error!\n");
	else if (vec.size()!=1)
	{
		fprintf(stderr, "ERROR: bad output!\n");
		error = WPS_PARSE_ERROR;
	}
	if (error != WPS_OK)
		return 1;
	if (!output)
		std::cout << vec[0].cstr() << std::endl;
	else
	{
		std::ofstream out(output);
		out << vec[0].cstr() << std::endl;
	}
	return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
