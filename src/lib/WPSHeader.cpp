/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
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

#include <stdlib.h>
#include <string.h>
#include "WPSHeader.h"
#include "WPSFileStructure.h"
#include "libwps.h"
#include "libwps_internal.h"
	
WPSHeader::WPSHeader(libwps::WPSInputStream *input, uint32_t documentOffset, uint8_t productType, uint8_t fileType, uint8_t majorVersion, uint8_t minorVersion) :
	m_documentOffset(documentOffset),
	m_productType(productType),
	m_fileType(fileType),
	m_majorVersion(majorVersion),
	m_minorVersion(minorVersion)
{
}

WPSHeader::~WPSHeader()
{
}

WPSHeader * WPSHeader::constructHeader(libwps::WPSInputStream *input)
{
	WPS_DEBUG_MSG(("WPSHeader::constructHeader()\n"));
	
	return NULL;
}
