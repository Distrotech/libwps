/* libwps
 * Copyright (C) 2002-2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002-2004 Marc Maurer (uwog@uwog.net)
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * For further information visit http://libwps.sourceforge.net
 */

#include <stdio.h>
#include <string.h>
#include "libwps.h"
#include "TextDocumentGenerator.h"
#include <libwpd-stream/libwpd-stream.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: wps2text <Works Document>\n");
		return -1;
	}

	WPXFileStream input(argv[1]);

	WPSConfidence confidence = WPSDocument::isFileFormatSupported(&input);
	if (confidence == WPS_CONFIDENCE_NONE || confidence == WPS_CONFIDENCE_POOR)
	{
		printf("ERROR: Unsupported file format!\n");
		return 1;
	}
	
	TextDocumentGenerator listenerImpl;
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
