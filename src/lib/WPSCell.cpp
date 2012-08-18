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

void WPSCellFormat::setBorders(int wh, WPSBorder const &border)
{
	int const allBits = WPSBorder::LeftBit|WPSBorder::RightBit|WPSBorder::TopBit|WPSBorder::BottomBit;
	if (wh & (~allBits))
	{
		WPS_DEBUG_MSG(("WPSCellFormat::setBorders: unknown borders\n"));
		return;
	}
	if (m_bordersList.size() < 4)
	{
		WPSBorder emptyBorder;
		emptyBorder.m_style = WPSBorder::None;
		m_bordersList.resize(4, emptyBorder);
	}
	if (wh & WPSBorder::LeftBit) m_bordersList[WPSBorder::Left] = border;
	if (wh & WPSBorder::RightBit) m_bordersList[WPSBorder::Right] = border;
	if (wh & WPSBorder::TopBit) m_bordersList[WPSBorder::Top] = border;
	if (wh & WPSBorder::BottomBit) m_bordersList[WPSBorder::Bottom] = border;
}

int WPSCellFormat::compare(WPSCellFormat const &cell) const
{
	int diff = int(m_hAlign) - int(cell.m_hAlign);
	if (diff) return diff;
	diff = int(m_backgroundColor) - int(cell.m_backgroundColor);
	if (diff) return diff;
	diff = int(m_bordersList.size()) - int(cell.m_bordersList.size());
	if (diff) return diff;
	for (size_t c = 0; c < m_bordersList.size(); c++)
	{
		diff = m_bordersList[c].compare(cell.m_bordersList[c]);
		if (diff) return diff;
	}
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
	case WPSCellFormat::HALIGN_DEFAULT:
	default:
		break; // default
	}
	if (cell.m_backgroundColor != 0xFFFFFF)
		o << ",backColor=" << std::hex << cell.m_backgroundColor << ",";
	for (size_t i = 0; i < cell.m_bordersList.size(); i++)
	{
		if (cell.m_bordersList[i].m_style == WPSBorder::None)
			continue;
		o << "bord";
		char const *wh[] = { "L", "R", "T", "B", "MiddleH", "MiddleV" };
		if (i < 4) o << wh[i];
		else o << "[#wh=" << i << "]";
		o << "=" << cell.m_bordersList[i] << ",";
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
	o << "box=" << cell.m_box;
	if (cell.m_verticalSet) o << "[ySet]";
	o << ",";
	o << static_cast<WPSCellFormat const &>(cell);

	return o;
}


/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
