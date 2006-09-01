/* libwpd
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef WP6PREFIXDATA_H
#define WP6PREFIXDATA_H
#include "WP6PrefixDataPacket.h"
#include <map>
#include <vector>

typedef std::map<int, WP6PrefixDataPacket *> DPH;	
typedef std::multimap<int, WP6PrefixDataPacket *> MPDP;
typedef MPDP::const_iterator MPDP_CIter;
typedef MPDP::iterator MPDP_Iter;

class WP6PrefixData
{
 public:
	WP6PrefixData(WPXInputStream *input, const int numPrefixIndices);
	virtual ~WP6PrefixData();
	const WP6PrefixDataPacket *getPrefixDataPacket(const int prefixID) const;
	std::pair< MPDP_CIter, MPDP_CIter > * getPrefixDataPacketsOfType(const int type) const;

	const uint16_t getDefaultInitialFontPID() const { return m_defaultInitialFontPID; }

 private:
	DPH m_prefixDataPacketHash;
	MPDP m_prefixDataPacketTypeHash;
	int m_defaultInitialFontPID;
};

#endif /* WP6PREFIXDATA_H */
