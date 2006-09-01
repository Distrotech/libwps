/* libwpd
 * Copyright (C) 2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
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

#include "WP5FixedLengthGroup.h"
#include "WP5FileStructure.h"
#include "WP5AttributeGroup.h"
#include "WP5ExtendedCharacterGroup.h"
#include "WP5UnsupportedFixedLengthGroup.h"
#include "libwpd_internal.h"

WP5FixedLengthGroup::WP5FixedLengthGroup(const uint8_t groupID):
	m_group(groupID)
{
}

WP5FixedLengthGroup * WP5FixedLengthGroup::constructFixedLengthGroup(WPXInputStream *input, const uint8_t groupID)
{
	switch (groupID) 
	{
		case WP5_TOP_EXTENDED_CHARACTER:
			return new WP5ExtendedCharacterGroup(input, groupID);
			
		case WP5_TOP_ATTRIBUTE_ON:
			return new WP5AttributeOnGroup(input, groupID);
			
		case WP5_TOP_ATTRIBUTE_OFF:
			return new WP5AttributeOffGroup(input, groupID);
		
		// Add the remaining cases here
		default:
			return new WP5UnsupportedFixedLengthGroup(input, groupID);
	}
}

bool WP5FixedLengthGroup::isGroupConsistent(WPXInputStream *input, const uint8_t groupID)
{
	uint32_t startPosition = input->tell();

	try
	{
		int size = WP5_FIXED_LENGTH_FUNCTION_GROUP_SIZE[groupID-0xC0];
		if (input->seek((startPosition + size - 2 - input->tell()), WPX_SEEK_CUR) || input->atEOS())
		{
			input->seek(startPosition, WPX_SEEK_SET);
			return false;
		}
		if (groupID != readU8(input))
		{
			input->seek(startPosition, WPX_SEEK_SET);
			return false;
		}
		
		input->seek(startPosition, WPX_SEEK_SET);
		return true;
	}
	catch(...)
	{
		input->seek(startPosition, WPX_SEEK_SET);
		return false;
	}
}

void WP5FixedLengthGroup::_read(WPXInputStream *input)
{
	uint32_t startPosition = input->tell();
	_readContents(input);
	
	if (m_group >= 0xC0 && m_group <= 0xCF) // just an extra safety check
	{
		int size = WP5_FIXED_LENGTH_FUNCTION_GROUP_SIZE[m_group-0xC0];
		input->seek((startPosition + size - 2 - input->tell()), WPX_SEEK_CUR);
		if (m_group != readU8(input))
		{
			WPD_DEBUG_MSG(("WordPerfect: Possible corruption detected. Bailing out!\n"));
			throw FileException();
		}
	}
	else
		throw FileException();
}
