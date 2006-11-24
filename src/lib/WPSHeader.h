/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002-2003 Marc Maurer (uwog@uwog.net)
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

#ifndef WPSHEADER_H
#define WPSHEADER_H

#include <stdlib.h>
#include "libwps_types.h"
#include "WPSStream.h"

class WPSHeader
{
 public:	
	WPSHeader(libwps::WPSInputStream *input, uint32_t documentOffset, uint8_t productType, uint8_t fileType, uint8_t majorVersion, uint8_t minorVersion);
	virtual ~WPSHeader();

	static WPSHeader * constructHeader(libwps::WPSInputStream *input);
		
	const uint32_t getDocumentOffset() const { return m_documentOffset; }
	const uint8_t getProductType() const { return m_productType; }
	const uint8_t getFileType() const { return m_fileType; }
	const uint8_t getMajorVersion() const { return m_majorVersion; }
	const uint8_t getMinorVersion() const { return m_minorVersion; }

 private:	
	uint32_t m_documentOffset;
	uint8_t m_productType;
	uint8_t m_fileType;
	uint8_t m_majorVersion;
	uint8_t m_minorVersion;
};

#endif /* WPSHEADER_H */
