/* libwpd
 * Copyright (C) 2005 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#ifndef WP5FONTNAMESTRINGPACKET_H
#define WP5FONTNAMESTRINGPACKET_H
#include "WP5GeneralPacketData.h"
#include "WPXMemoryStream.h"
#include "WPXString.h"
#include <map>

class WP5FontNameStringPoolPacket : public WP5GeneralPacketData
{
public:
	WP5FontNameStringPoolPacket(WPXInputStream *input, int id, uint32_t dataOffset, uint32_t dataSize);
	~WP5FontNameStringPoolPacket();
	void _readContents(WPXInputStream *input, uint32_t dataSize);
	WPXString getFontName(const unsigned int offset) const;

private:
	std::map<unsigned int, WPXString> m_fontNameString;
};
#endif /* WP5FONTNAMESTRINGPACKET_H */
