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

#include <librevenge/librevenge.h>
#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>

#include <libwps/libwps.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef VERSION
#define VERSION "UNKNOWN VERSION"
#endif

static int printUsage()
{
	printf("`wks2raw' is used to test spreadsheet import in libwps.\n");
	printf("Supported formats are Microsoft Works Spreadsheet and Database, Lotus Wk1-Wk4 and Quattro Pro Wq1-Wq2.\n");
	printf("\n");
	printf("Usage: wks2raw [OPTION] FILE\n");
	printf("\n");
	printf("Options:\n");
	printf("\t-h, --help                 show this help message\n");
	printf("\t-v, --version              show version information\n");
	printf("\t--callgraph                display the call graph nesting level\n");
	printf("\t--password PASSWORD        set password to open the file\n");
	printf("\n");
	printf("Report bugs to <https://sourceforge.net/p/libwps/bugs/> or <https://bugs.documentfoundation.org/>.\n");
	return -1;
}

static int printVersion()
{
	printf("wks2raw %s\n", VERSION);
	return 0;
}

using namespace libwps;

int main(int argc, char *argv[])
{
	bool printIndentLevel = false;
	char *file = 0;
	char const *password=0;

	for (int arg=1; arg<argc; ++arg)
	{
		if (!strcmp(argv[arg], "-h") || !strcmp(argv[arg], "--help"))
			return printUsage();
		if (!strcmp(argv[arg], "-v") || !strcmp(argv[arg], "--version"))
			return printVersion();
		if (!strcmp(argv[arg], "--callgraph"))
		{
			printIndentLevel = true;
			continue;
		}
		if (!strcmp(argv[arg], "--password"))
		{
			if (arg+1>=argc)
				return printUsage();
			password=argv[++arg];
			continue;
		}
		if (file)
			return printUsage();
		file = argv[arg];
	}
	if (!file)
		return printUsage();

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

	librevenge::RVNGRawSpreadsheetGenerator listenerImpl(printIndentLevel);
	WPSResult error= WPSDocument::parse(&input, &listenerImpl, password);

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

	if (error != WPS_OK)
		return 1;

	return 0;
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
