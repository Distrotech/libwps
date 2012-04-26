/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
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
 * For further information visit http://libwps.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include <time.h>

#include <iomanip>
#include <sstream>

#include <libwpd/WPXPropertyList.h>

#include "WPSCell.h"

int WPSCellFormat::compare(WPSCellFormat const &cell) const
{
	int diff = int(m_hAlign) - int(cell.m_hAlign);
	if (diff) return diff;
	diff = m_bordersList - cell.m_bordersList;
	if (diff) return diff;
	return 0;
}

std::ostream &operator<<(std::ostream &o, WPSCellFormat const &cell)
{
	switch(cell.m_hAlign)
	{
	case WPSCellFormat::HALIGN_LEFT:
		o << ",left";
		break;
	case WPSCellFormat::HALIGN_CENTER:
		o << ",centered";
		break;
	case WPSCellFormat::HALIGN_RIGHT:
		o << ",right";
		break;
	case WPSCellFormat::HALIGN_FULL:
		o << ",full";
		break;
	default:
		break; // default
	}
	int border = cell.m_bordersList;
	if (border)
	{
		o << ",bord=[";
		if (border&libwps::LeftBorderBit) o << "Lef";
		if (border&libwps::RightBorderBit) o << "Rig";
		if (border&libwps::TopBorderBit) o << "Top";
		if (border&libwps::BottomBorderBit) o << "Bot";
		o << "]";
	}
	return o;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &o, WPSCell const &cell)
{
	o << "C" << cell.m_position << ":";
	if (cell.numSpannedCells()[0] != 1 || cell.numSpannedCells()[1] != 1)
		o << "span=[" << cell.numSpannedCells()[0] << "," << cell.numSpannedCells()[1] << "],";
	o << "box=" << cell.m_box << ",";
	o << static_cast<WPSCellFormat const &>(cell);

	return o;
}


/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
