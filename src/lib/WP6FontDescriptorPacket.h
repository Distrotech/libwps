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

#ifndef WP6FONTDESCRIPTORPACKET_H
#define WP6FONTDESCRIPTORPACKET_H
#include "WP6PrefixDataPacket.h"

class WP6FontDescriptorPacket : public WP6PrefixDataPacket
{
 public:
	WP6FontDescriptorPacket(WPXInputStream *input, int id, uint32_t dataOffset, uint32_t dataSize);
	~WP6FontDescriptorPacket();
	void _readContents(WPXInputStream *input);
	const char *getFontName() const { return m_fontName; }

 private:
	uint16_t m_characterWidth;
	uint16_t m_ascenderHeight;
	uint16_t m_xHeight;
	uint16_t m_descenderHeight;
	uint16_t m_italicsAdjust;
	uint8_t m_primaryFamilyId; // family id's are supposed to be one unified element, but I split them up to ease parsing
	uint8_t m_primaryFamilyMemberId;
	
	uint8_t m_scriptingSystem;
	uint8_t m_primaryCharacterSet;
	uint8_t m_width;
	uint8_t m_weight; 
	uint8_t m_attributes;
	uint8_t m_generalCharacteristics;
	uint8_t m_classification;
	uint8_t m_fill; // fill byte
	uint8_t m_fontType;
	uint8_t m_fontSourceFileType;

	uint16_t m_fontNameLength;

	char *m_fontName; 
};
#endif
