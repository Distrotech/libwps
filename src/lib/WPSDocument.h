/* libwps
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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


#ifndef WPSDOCUMENT_H
#define WPSDOCUMENT_H

#include "WPSStream.h"

enum WPDConfidence { WPD_CONFIDENCE_NONE=0, WPD_CONFIDENCE_POOR, WPD_CONFIDENCE_LIKELY, WPD_CONFIDENCE_GOOD, WPD_CONFIDENCE_EXCELLENT };
enum WPDResult { WPD_OK, WPD_FILE_ACCESS_ERROR, WPD_PARSE_ERROR, WPD_UNSUPPORTED_ENCRYPTION_ERROR, WPD_OLE_ERROR, WPD_UNKNOWN_ERROR };

class WPXHLListenerImpl;

/**
This class provides all the functions an application would need to parse 
Works documents.
*/

class WPSDocument
{
public:
	static WPDConfidence isFileFormatSupported(libwps::WPSInputStream *input, bool partialContent);
	static WPDResult parse(libwps::WPSInputStream *input, WPXHLListenerImpl *listenerImpl);
	//static void parse(libwps::WPSInputStream *input, WPXHLListenerImpl *listenerImpl, WPSFileType fileType);
	//WPSFileType getFileType(libwps::WPSInputStream *input)
private:
	static WPDConfidence isFileFormatSupportedWPS(libwps::WPSInputStream *input, bool partialContent);
	static WPDResult parseWPS(libwps::WPSInputStream *input, WPXHLListenerImpl *listenerImpl);	
	
};

#endif /* WPSDOCUMENT_H */
