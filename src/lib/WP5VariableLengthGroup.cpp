/* libwpd
 * Copyright (C) 2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003-2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include "WP5PageFormatGroup.h"
#include "WP5VariableLengthGroup.h"
#include "WP5UnsupportedVariableLengthGroup.h"
#include "libwpd_internal.h"
#include "WP5HeaderFooterGroup.h"
#include "WP5FontGroup.h"
#include "WP5FootnoteEndnoteGroup.h"
#include "WP5DefinitionGroup.h"
#include "WP5TableEOLGroup.h"
#include "WP5TableEOPGroup.h"

WP5VariableLengthGroup::WP5VariableLengthGroup()
{
}

WP5VariableLengthGroup * WP5VariableLengthGroup::constructVariableLengthGroup(WPXInputStream *input, const uint8_t group)
{
	WPD_DEBUG_MSG(("WordPerfect: handling a variable length group Ox%x\n", group));	
	switch (group)
	{
		case WP5_TOP_DEFINITION_GROUP:
			return new WP5DefinitionGroup(input);
		case WP5_TOP_PAGE_FORMAT_GROUP:
			return new WP5PageFormatGroup(input);
		case WP5_TOP_HEADER_FOOTER_GROUP:
			return new WP5HeaderFooterGroup(input);
		case WP5_TOP_FONT_GROUP:
			return new WP5FontGroup(input);
		case WP5_TOP_FOOTNOTE_ENDNOTE_GROUP:
			return new WP5FootnoteEndnoteGroup(input);
		case WP5_TOP_TABLE_EOL_GROUP:
			return new WP5TableEOLGroup(input);
		case WP5_TOP_TABLE_EOP_GROUP:
			return new WP5TableEOPGroup(input);
		default:
			// this is an unhandled group, just skip it
			return new WP5UnsupportedVariableLengthGroup(input);
	}
}

bool WP5VariableLengthGroup::isGroupConsistent(WPXInputStream *input, const uint8_t group)
{
	uint32_t startPosition = input->tell();

	try
	{
		uint8_t subGroup = readU8(input);
		uint16_t size = readU16(input);

		if (input->seek((startPosition + size - 1 - input->tell()), WPX_SEEK_CUR) || input->atEOS())
		{
			input->seek(startPosition, WPX_SEEK_SET);
			return false;
		}
		if (size != readU16(input))
		{
			input->seek(startPosition, WPX_SEEK_SET);
			return false;
		}
		if (subGroup != readU8(input))
		{
			input->seek(startPosition, WPX_SEEK_SET);
			return false;
		}
		if (group != readU8(input))
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

void WP5VariableLengthGroup::_read(WPXInputStream *input)
{
	uint32_t startPosition = input->tell();

	m_subGroup = readU8(input);
	m_size = readU16(input) + 4; // the length is the number of data bytes minus 4 (ie. the function codes)
	
	WPD_DEBUG_MSG(("WordPerfect: Read variable group header (start_position: %i, sub_group: 0x%2x, size: %i)\n", startPosition, m_subGroup, m_size));

	_readContents(input);

	input->seek((startPosition + m_size - 5 - input->tell()), WPX_SEEK_CUR);

	if (m_size != (readU16(input) + 4))
	{
		WPD_DEBUG_MSG(("WordPerfect: Possible corruption detected. Bailing out!\n"));
		throw FileException();
	}
	if (m_subGroup != readU8(input))
	{
		WPD_DEBUG_MSG(("WordPerfect: Possible corruption detected. Bailing out!\n"));
		throw FileException();
	}

	input->seek((startPosition + m_size - 1 - input->tell()), WPX_SEEK_CUR);
}
