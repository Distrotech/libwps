/* libwpd
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include <stdlib.h>
#include <string.h>
#include "WPXHeader.h"
#include "WP3Header.h"
#include "WP5Header.h"
#include "WP6Header.h"
#include "WP60Header.h"
#include "WP61Header.h"
#include "WPXFileStructure.h"
#include "libwpd.h"
#include "libwpd_internal.h"
	
WPXHeader::WPXHeader(WPXInputStream *input, uint32_t documentOffset, uint8_t productType, uint8_t fileType, uint8_t majorVersion, uint8_t minorVersion, uint16_t documentEncryption) :
	m_documentOffset(documentOffset),
	m_productType(productType),
	m_fileType(fileType),
	m_majorVersion(majorVersion),
	m_minorVersion(minorVersion),
	m_documentEncryption(documentEncryption)
{
}

WPXHeader::~WPXHeader()
{
}

WPXHeader * WPXHeader::constructHeader(WPXInputStream *input)
{
	WPD_DEBUG_MSG(("WPXHeader::constructHeader()\n"));
	
	char fileMagic[4];
	/* check the magic */
	input->seek(WPX_HEADER_MAGIC_OFFSET - input->tell(), WPX_SEEK_CUR);
	for (int i=0; i<3 /* FIXME: && not EOF */; i++)
		fileMagic[i] = readU8(input);
	fileMagic[3] = '\0';
	
	if ( strcmp(fileMagic, "WPC") )
	{
		WPD_DEBUG_MSG(("WordPerfect: File magic is not equal to \"WPC\"!\n"));
		return NULL;
	}
	
	/* get the document pointer */
	input->seek(WPX_HEADER_DOCUMENT_POINTER_OFFSET - input->tell(), WPX_SEEK_CUR);
	uint32_t documentOffset = readU32(input);

	/* get information on product types, file types, versions */
	input->seek(WPX_HEADER_PRODUCT_TYPE_OFFSET - input->tell(), WPX_SEEK_CUR);
	uint8_t productType = readU8(input);
	uint8_t fileType = readU8(input);
	uint8_t majorVersion = readU8(input);
	uint8_t minorVersion = readU8(input);
	
	input->seek(WPX_HEADER_ENCRYPTION_OFFSET, WPX_SEEK_SET);
	uint8_t documentEncryption = (uint8_t)readU16(input);		
	
	WPD_DEBUG_MSG(("WordPerfect: Product Type: %i File Type: %i Major Version: %i Minor Version: %i\n", 
					productType, fileType, 
					majorVersion, minorVersion));	
	WPD_DEBUG_MSG(("WordPerfect: Document Encryption = 0x%x \n",(int)documentEncryption));		


	switch (fileType)
	{
		case 0x0a:
			WPD_DEBUG_MSG(("WordPerfect: Supported file type: \"WordPerfect Document\"\n"));
			switch (majorVersion)
			{
				case 0x00: // WP5 
					return new WP5Header(input, documentOffset, productType, fileType,
								majorVersion, minorVersion, documentEncryption);
				case 0x02: // WP6
					switch (minorVersion)
					{
						case 0x00:
							return new WP60Header(input, documentOffset, productType, fileType,
										majorVersion, minorVersion, documentEncryption);
						default: // assume this header can be parsed by a WP61 header parser
							return new WP61Header(input, documentOffset, productType, fileType,
										majorVersion, minorVersion, documentEncryption);
					}
				default:
					// unhandled file format
					WPD_DEBUG_MSG(("WordPerfect: Unsupported major number: %d\n", majorVersion));
					return NULL;
			}
		case 0x2c:
			WPD_DEBUG_MSG(("WordPerfect: Supported file type: \"MAC WP 2.0 document\"\n"));
			switch (majorVersion)
			{
				case 0x02: // WP Mac 2.x
				case 0x03: // WP Mac 3.0-3.5
				case 0x04: // WP Mac 3.5e
					return new WP3Header(input, documentOffset, productType, fileType,
								majorVersion, minorVersion, documentEncryption);
				default:
					// unhandled file format
					WPD_DEBUG_MSG(("WordPerfect: Unsupported major number: %d\n", majorVersion));
					return NULL;
			}
		default:
			WPD_DEBUG_MSG(("WordPerfect: Unsupported file type: %d\n", fileType));
			return NULL;
	}
	

	return NULL;
}
