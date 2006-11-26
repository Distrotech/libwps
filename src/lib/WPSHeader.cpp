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

/*
 * This file is in sync with CVS
 * /libwpd2/src/lib/WPXHeader.cpp 1.29
 */

#include "WPSHeader.h"
#include "WPSFileStructure.h"
#include "libwps_internal.h"
	
WPSHeader::WPSHeader(libwps::WPSInputStream *input, uint8_t majorVersion) :
	m_input(input),
	m_majorVersion(majorVersion)
{
}

WPSHeader::~WPSHeader()
{
}

WPSHeader * WPSHeader::constructHeader(libwps::WPSInputStream *input)
{
	WPS_DEBUG_MSG(("WPSHeader::constructHeader()\n"));

	libwps::WPSInputStream * document_mn0 = input->getWPSOleStream("MN0");	
	if (document_mn0)
	{
		WPS_DEBUG_MSG(("Microsoft Works v4 format detected\n"));	
		return new WPSHeader(document_mn0, 4);
	}	
	else
		DELETEP(document_mn0);
	
	libwps::WPSInputStream * document_contents = input->getWPSOleStream("CONTENTS");	
	if (document_contents)
	{
		/* check the Works 2000/7/8 format magic */
		document_contents->seek(0);
		
		char fileMagic[8];
		for (int i=0; i<7 && !document_contents->atEnd(); i++)
			fileMagic[i] = readU8(document_contents);		
		fileMagic[7] = '\0';
		
	
		/* Works 7/8 */
		if (0 == strcmp(fileMagic, "CHNKWKS"))
		{
			WPS_DEBUG_MSG(("Microsoft Works v8 (maybe 7) format detected\n"));
			return new WPSHeader(document_contents, 8);
		}
		
		/* Works 2000 */		
		if (0 == strcmp(fileMagic, "CHNKINK"))
		{
			return new WPSHeader(document_contents, 5);
		}	

		DELETEP(document_contents);
	}

	input->seek(0);
	if (readU8(input) < 6 && 0xFE == readU8(input))
	{
		WPS_DEBUG_MSG(("Microsoft Works v2 format detected\n"));
		return new WPSHeader(input, 2);
	}	
	
	return NULL;
}
