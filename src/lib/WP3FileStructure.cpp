/* libwpd
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
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

#include "WP3FileStructure.h"

// size of the fixed length functiongroups 0xC0 to 0xCF
int WP3_FIXED_LENGTH_FUNCTION_GROUP_SIZE[16] = 
{
	5,	// 0xC0
	8,
	7,	
	4,	
	4,	
	7,	
	10,	
	7,	
	4,	
	5,	
	6,	
	6,	
	7,	
	9,	
	7,	
	4	// 0xCF
};
