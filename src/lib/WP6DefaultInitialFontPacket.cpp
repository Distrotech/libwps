/* libwpd
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
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

#include "WP6Listener.h"
#include "WP6DefaultInitialFontPacket.h"
#include "libwpd_internal.h"

WP6DefaultInitialFontPacket::WP6DefaultInitialFontPacket(WPXInputStream *input, int id, uint32_t dataOffset, uint32_t dataSize)
	: WP6PrefixDataPacket(input)
{
	_read(input, dataOffset, dataSize);
}

void WP6DefaultInitialFontPacket::parse(WP6Listener *listener) const
{
	listener->fontChange(getPointSize(), getInitialFontDescriptorPID());
}

void WP6DefaultInitialFontPacket::_readContents(WPXInputStream *input)
{
   m_numPrefixIDs = readU16(input);
   m_initialFontDescriptorPID = readU16(input);
   m_pointSize = readU16(input);
   WPD_DEBUG_MSG(("WordPerfect: Read default initial font packet (initial font descriptor pid: %i, point size: %i)\n", 
		  (int) m_initialFontDescriptorPID, (int) m_pointSize));
}
