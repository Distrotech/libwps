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

#include <libwpd/WPDocument.h>
#include "WPSStream.h"

enum WPSConfidence { WPS_CONFIDENCE_NONE=0, WPS_CONFIDENCE_POOR, WPS_CONFIDENCE_LIKELY, WPS_CONFIDENCE_GOOD, WPS_CONFIDENCE_EXCELLENT };
enum WPSResult { WPS_OK, WPS_FILE_ACCESS_ERROR, WPS_PARSE_ERROR, WPS_OLE_ERROR, WPS_UNKNOWN_ERROR };

class WPXHLListenerImpl;

/**
This class provides all the functions an application would need to parse 
Works documents.
*/

class WPSDocument
{
public:
	static WPSConfidence isFileFormatSupported(WPSInputStream *input, bool partialContent);
	static WPSResult parse(WPSInputStream *input, WPXHLListenerImpl *listenerImpl);
};

#endif /* WPSDOCUMENT_H */
