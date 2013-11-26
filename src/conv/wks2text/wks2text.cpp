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

static int printUsage()
{
	printf("Usage: wks2cvs [-h] [-o file.txt] <Works Spreadsheet Document>\n");
	printf("\t-h:                 Shows this help message\n");
	printf("\t-o file.text:       Defines the ouput file\n");
	return -1;
}

int main(int argc, char *argv[])
{
	bool printHelp=false;
	char const *output = 0;
	int ch;

	while ((ch = getopt(argc, argv, "ho:")) != -1)
	{
		switch (ch)
		{
		case 'o':
			output=optarg;
			break;
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

	WPSKind kind;
	WPSConfidence confidence = WPSDocument::isFileFormatSupported(&input,kind);
	if (confidence == WPS_CONFIDENCE_NONE)
	{
		printf("ERROR: Unsupported file format!\n");
		return 1;
	}

	librevenge::RVNGStringVector vec;
	if (kind != WPS_SPREADSHEET)
	{
		printf("ERROR: Unsupported file format!\n");
		return 1;
	}

	WPSResult error=WPS_OK;
	try
	{
		librevenge::RVNGTextSpreadsheetGenerator listenerImpl(vec);
		error= WPSDocument::parse(&input, &listenerImpl);
	}
	catch (...)
	{
		error=WPS_PARSE_ERROR;
	}

	if (error == WPS_FILE_ACCESS_ERROR)
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
