/* libwpd
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

#include "WP42MarginResetGroup.h"
#include "libwpd_internal.h"
#include <string>

WP42MarginResetGroup::WP42MarginResetGroup(WPXInputStream *input, uint8_t group) :
	WP42MultiByteFunctionGroup(group),
	m_leftMargin(0),
	m_rightMargin(0)
{
	_read(input);
}

WP42MarginResetGroup::~WP42MarginResetGroup()
{
}

void WP42MarginResetGroup::_readContents(WPXInputStream *input)
{
	input->seek(2, WPX_SEEK_CUR);
	m_leftMargin = readU8(input);
	m_rightMargin = readU8(input);
}

void WP42MarginResetGroup::parse(WP42Listener *listener)
{
	WPD_DEBUG_MSG(("WordPerfect: handling the Margin Reset group\n"));
	listener->marginReset(m_leftMargin, m_rightMargin);
}
