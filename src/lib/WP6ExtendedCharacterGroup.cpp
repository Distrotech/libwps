/* libwpd
 * Copyright (C) 2003 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2002 Ariya Hidayat <ariyahidayat@yahoo.de>
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

#include "WP6ExtendedCharacterGroup.h"
#include "WP6FileStructure.h"
#include "libwpd_internal.h"

WP6ExtendedCharacterGroup::WP6ExtendedCharacterGroup(WPXInputStream *input, uint8_t groupID) :
	WP6FixedLengthGroup(groupID),
	m_character(0),
	m_characterSet(0)
{
	_read(input);
}

void WP6ExtendedCharacterGroup::_readContents(WPXInputStream *input)
{
	m_character = readU8(input);
	m_characterSet = readU8(input);
}

void WP6ExtendedCharacterGroup::parse(WP6Listener *listener)
{
	const uint16_t *chars;
	int len = extendedCharacterWP6ToUCS2(m_character,
					  m_characterSet, &chars);
	int i;

	for (i = 0; i < len; i++)
		listener->insertCharacter(chars[i]);

}
