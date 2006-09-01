/* libwpd
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
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

#include "WPXPart.h"
#include "WP6Part.h"
#include "WP6Header.h"
#include "WP6VariableLengthGroup.h"
#include "WP6FixedLengthGroup.h"
#include "WP6SingleByteFunction.h"
#include "libwpd_internal.h"

// constructPart: constructs a parseable low-level representation of part of the document
// returns the part if it successfully creates the part, returns NULL if it can't
// throws an exception if there is an error
// precondition: readVal us between 0x80 and 0xFF
WP6Part * WP6Part::constructPart(WPXInputStream *input, const uint8_t readVal)
{	
	WPD_DEBUG_MSG(("WordPerfect: ConstructPart\n"));
		
	if (readVal >= (uint8_t)0x80 && readVal <= (uint8_t)0xCF)
	{
		WPD_DEBUG_MSG(("WordPerfect: constructSingleByteFunction(input, val=0x%.2x)\n", readVal));
		return WP6SingleByteFunction::constructSingleByteFunction(input, readVal);
	}
	else if (readVal >= (uint8_t)0xD0 && readVal <= (uint8_t)0xEF)
	{
		if (!WP6VariableLengthGroup::isGroupConsistent(input, readVal))
		{
			WPD_DEBUG_MSG(("WordPerfect: Consistency Check (variable length) failed; ignoring this byte\n"));
			return NULL;
		}
		WPD_DEBUG_MSG(("WordPerfect: constructVariableLengthGroup(input, val=0x%.2x)\n", readVal));
		return WP6VariableLengthGroup::constructVariableLengthGroup(input, readVal);
	}      

	else if (readVal >= (uint8_t)0xF0)
	{
		if (!WP6FixedLengthGroup::isGroupConsistent(input, readVal))
		{
			WPD_DEBUG_MSG(("WordPerfect: Consistency Check (fixed length) failed; ignoring this byte\n"));
			return NULL;
		}
		WPD_DEBUG_MSG(("WordPerfect: constructFixedLengthGroup(input, val=0x%.2x)\n", readVal));
		return WP6FixedLengthGroup::constructFixedLengthGroup(input, readVal);
	}

	WPD_DEBUG_MSG(("WordPerfect: Returning NULL from constructPart\n"));
	return NULL;
}
