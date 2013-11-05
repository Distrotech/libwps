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
#include <librevenge-stream/librevenge-stream.h>
#include <libwps/libwps.h>
#include "RawDocumentGenerator.h"

int main(int argc, char *argv[])
{
	bool printIndentLevel = false;
	char *file = 0;

	if (argc < 2)
	{
		printf("Usage: wps2raw [OPTION] <Works Document>\n");
		return -1;
	}

	if (!strcmp(argv[1], "--callgraph"))
	{
		if (argc == 2)
		{
			printf("Usage: wps2raw [OPTION] <Works Document>\n");
			return -1;
		}

		printIndentLevel = true;
		file = argv[2];
	}
	else if (!strcmp(argv[1], "--help"))
	{
		printf("Usage: wps2raw [OPTION] <Works Document>\n");
		printf("\n");
		printf("Options:\n");
		printf("--callgraph		    Display the call graph nesting level\n");
		printf("--help              Shows this help message\n");
		return 0;
	}
	else
		file = argv[1];

	librevenge::RVNGFileStream input(file);

	WPSConfidence confidence = WPSDocument::isFileFormatSupported(&input);
	if (confidence == WPS_CONFIDENCE_NONE || confidence == WPS_CONFIDENCE_POOR)
	{
		printf("ERROR: Unsupported file format!\n");
		return 1;
	}

	RawDocumentGenerator listenerImpl(printIndentLevel);
	WPSResult error = WPSDocument::parse(&input, &listenerImpl);

	if (error == WPS_FILE_ACCESS_ERROR)
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
