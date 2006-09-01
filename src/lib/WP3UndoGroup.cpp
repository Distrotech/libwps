/* libwpd
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include "WP3UndoGroup.h"
#include "libwpd_internal.h"

WP3UndoGroup::WP3UndoGroup(WPXInputStream *input, uint8_t groupID)
	: WP3FixedLengthGroup(groupID)
{
	_read(input);
}

void WP3UndoGroup::_readContents(WPXInputStream *input)
{
	m_undoType = readU8(input);
	m_undoLevel = readU16(input, true);
}

void WP3UndoGroup::parse(WP3Listener *listener)
{
	listener->undoChange(m_undoType, m_undoLevel);
}
