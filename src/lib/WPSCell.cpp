/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwps.sourceforge.net
 */

#include <time.h>

#include <sstream>

#include <librevenge/librevenge.h>

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

void WPSCellFormat::addTo(librevenge::RVNGPropertyList &propList) const
{

	switch(m_hAlign)
	{
	case HALIGN_LEFT:
		propList.insert("fo:text-align", "first");
		propList.insert("style:text-align-source", "fix");
		break;
	case HALIGN_CENTER:
		propList.insert("fo:text-align", "center");
		propList.insert("style:text-align-source", "fix");
		break;
	case HALIGN_RIGHT:
		propList.insert("fo:text-align", "end");
		propList.insert("style:text-align-source", "fix");
		break;
	case HALIGN_FULL:
	case HALIGN_DEFAULT:
	default:
		break;
	}

	for (size_t c = 0; c < m_bordersList.size(); c++)
	{
		std::string property = m_bordersList[c].getPropertyValue();
		if (property.length() == 0) continue;
		switch(c)
		{
		case WPSBorder::Left:
			propList.insert("fo:border-left", property.c_str());
			break;
		case WPSBorder::Right:
			propList.insert("fo:border-right", property.c_str());
			break;
		case WPSBorder::Top:
			propList.insert("fo:border-top", property.c_str());
			break;
		case WPSBorder::Bottom:
			propList.insert("fo:border-bottom", property.c_str());
			break;
		default:
			WPS_DEBUG_MSG(("WPSContentListener::openTableCell: can not send %d border\n",int(c)));
			break;
		}
	}
	if (backgroundColor() != 0xFFFFFF)
	{
		char color[20];
		sprintf(color,"#%06x",backgroundColor());
		propList.insert("fo:background-color", color);
	}
	if (m_protected)
		propList.insert("style:cell-protect","protected");
}

int WPSCellFormat::getUniqueIdForNumberingStyle() const
{
	if (m_subFormat < 0 || m_subFormat >= 8) return -1;
	switch(m_format)
	{
	case F_NUMBER:
		return m_subFormat;
	case F_DATE:
		return 8+m_subFormat;
	case F_TIME:
		return 16+m_subFormat;
	case F_TEXT:
	case F_UNKNOWN:
	default:
		break;
	}
	return -1;
}

bool WPSCellFormat::getNumberingProperties(librevenge::RVNGPropertyList &propList, librevenge::RVNGPropertyListVector &pVect) const
{
	pVect=librevenge::RVNGPropertyListVector();
	switch(m_format)
	{
	case F_NUMBER:
		if (m_digits>-1000)
			propList.insert("number:decimal-places", m_digits);
		switch(m_subFormat)
		{
		case 5: // thousand
			propList.insert("number:grouping", true);
		case 0: // default
		case 1: // decimal
			propList.insert("librevenge:value-type", "number");
			break;
		case 2:
			propList.insert("librevenge:value-type", "scientific");
			break;
		case 6:
			propList.insert("number:grouping", true);
		case 3:
			propList.insert("librevenge:value-type", "percentage");
			break;
		case 7:
		case 4:
		{
			// DOME: implement non us currency
			propList.clear();
			propList.insert("librevenge:value-type", "currency");
			librevenge::RVNGPropertyList list;
			list.insert("librevenge:value-type", "currency-symbol");
			list.insert("number:language","en");
			list.insert("number:country","US");
			list.insert("librevenge:currency","$");
			pVect.append(list);

			list.clear();
			list.insert("librevenge:value-type", "number");
			if (m_digits>-1000)
				list.insert("number:decimal-places", m_digits);
			if (m_subFormat==7)
				list.insert("number:grouping", true);
			pVect.append(list);
			break;
		}
		default:
			return false;
		}
		return true;
	case F_DATE:
	{
		propList.insert("librevenge:value-type", "date");
		propList.insert("number:automatic-order", "true");
		librevenge::RVNGPropertyList list;
		switch (m_subFormat)
		{
		case 0:
		case 1:
			list.insert("librevenge:value-type", "month");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", "/");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "day");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", "/");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "year");
			pVect.append(list);
			break;
		case 5:
			list.clear();
			list.insert("librevenge:value-type", "day-of-week");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", ", ");
			pVect.append(list);
		case 2:
			list.clear();
			list.insert("librevenge:value-type", "day");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", " ");
			pVect.append(list);
		case 4:
			list.clear();
			list.insert("librevenge:value-type", "month");
			list.insert("number:textual", true);
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", ", ");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "year");
			list.insert("number:style", "long");
			pVect.append(list);
			break;
		case 3:
			list.clear();
			list.insert("librevenge:value-type", "day");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", ", ");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "month");
			list.insert("number:textual", true);
			pVect.append(list);
			break;
		case 7:
			list.clear();
			list.insert("librevenge:value-type", "day-of-week");
			list.insert("number:style", "long");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", ", ");
			pVect.append(list);
		case 6:
			if (m_subFormat==6)
			{
				list.clear();
				list.insert("librevenge:value-type", "day");
				pVect.append(list);
				list.clear();
				list.insert("librevenge:value-type", "text");
				list.insert("librevenge:text", " ");
				pVect.append(list);
			}
			list.clear();
			list.insert("librevenge:value-type", "month");
			list.insert("number:textual", true);
			list.insert("number:style", "long");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", " ");
			pVect.append(list);
			if (m_subFormat==7)
			{
				list.clear();
				list.insert("librevenge:value-type", "day");
				pVect.append(list);
				list.clear();
				list.insert("librevenge:value-type", "text");
				list.insert("librevenge:text", ", ");
				pVect.append(list);
			}
			list.clear();
			list.insert("librevenge:value-type", "year");
			list.insert("number:style", "long");
			pVect.append(list);
			break;
		default:
			return false;
		}
		return true;
	}
	case F_TIME:
	{
		if (m_subFormat < 0 || m_subFormat > 5)
			break;
		propList.insert("librevenge:value-type", "time");
		propList.insert("number:automatic-order", "true");
		librevenge::RVNGPropertyList list;
		list.insert("librevenge:value-type", "hours");
		if (m_subFormat <= 3)
			list.insert("number:style", "long");
		pVect.append(list);
		list.clear();
		list.insert("librevenge:value-type", "text");
		list.insert("librevenge:text", ":");
		pVect.append(list);
		list.clear();
		list.insert("librevenge:value-type", "minutes");
		list.insert("number:style", "long");
		pVect.append(list);
		if (m_subFormat != 2 && m_subFormat != 4)
		{
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", ":");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "seconds");
			list.insert("number:style", "long");
			pVect.append(list);
		}
		if (m_subFormat <= 3)
		{
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", " ");
			pVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "am-pm");
			pVect.append(list);
		}
		return true;
	}
	case F_TEXT:
	case F_UNKNOWN:
	default:
		break;
	}
	return false;
}

int WPSCellFormat::compare(WPSCellFormat const &cell) const
{
	int diff = int(m_hAlign) - int(cell.m_hAlign);
	if (diff) return diff;
	diff = int(m_backgroundColor) - int(cell.m_backgroundColor);
	if (diff) return diff;
	if (m_format<cell.m_format) return 1;
	if (m_format<cell.m_format) return -1;
	if (m_subFormat<cell.m_subFormat) return 1;
	if (m_subFormat<cell.m_subFormat) return -1;
	if (m_digits<cell.m_digits) return 1;
	if (m_digits<cell.m_digits) return -1;
	if (m_protected !=cell.m_protected) return m_protected ? 1 : -1;
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
		o << "left,";
		break;
	case WPSCellFormat::HALIGN_CENTER:
		o << "centered,";
		break;
	case WPSCellFormat::HALIGN_RIGHT:
		o << "right,";
		break;
	case WPSCellFormat::HALIGN_FULL:
		o << "full,";
		break;
	case WPSCellFormat::HALIGN_DEFAULT:
	default:
		break; // default
	}
	int subForm = cell.m_subFormat;
	switch(cell.m_format)
	{
	case WPSCellFormat::F_TEXT:
		o << "text";
		if (subForm)
		{
			o << "[Fo" << subForm << "]";
			subForm = 0;
		}
		break;
	case WPSCellFormat::F_NUMBER:
		o << "number";
		switch(subForm)
		{
		case 1:
			o << "[decimal]";
			subForm = 0;
			break;
		case 2:
			o << "[exp]";
			subForm = 0;
			break;
		case 3:
			o << "[percent]";
			subForm = 0;
			break;
		case 4:
			o << "[money]";
			subForm = 0;
			break;
		case 5:
			o << "[thousand]";
			subForm = 0;
			break;
		case 6:
			o << "[percent,thousand]";
			subForm = 0;
			break;
		case 7:
			o << "[money,thousand]";
			subForm = 0;
			break;
		default:
			break;
		}
		break;
	case WPSCellFormat::F_DATE:
		o << "date";
		switch(subForm)
		{
		case 1:
			o << "[mm/dd/yy]";
			subForm = 0;
			break;
		case 2:
			o << "[dd Mon, yyyy]";
			subForm = 0;
			break;
		case 3:
			o << "[dd, Mon]";
			subForm = 0;
			break;
		case 4:
			o << "[Mon, yyyy]";
			subForm = 0;
			break;
		case 5:
			o << "[Da, Mon dd, yyyy]";
			subForm = 0;
			break;
		case 6:
			o << "[Month dd yyyy]";
			subForm = 0;
			break;
		case 7:
			o << "[Day, Month dd, yyyy]";
			subForm = 0;
			break;
		default:
			break;
		}
		break;
	case WPSCellFormat::F_TIME:
		o << "time";
		switch(subForm)
		{
		case 1:
			o << "[hh:mm:ss AM]";
			subForm = 0;
			break;
		case 2:
			o << "[hh:mm AM]";
			subForm = 0;
			break;
		case 3:
			o << "[hh:mm:ss]";
			subForm = 0;
			break;
		case 4:
			o << "[hh:mm]";
			subForm = 0;
			break;
		default:
			break;
		}
		break;
	case WPSCellFormat::F_UNKNOWN:
	default:
		break; // default
	}
	if (subForm) o << "[format=#" << subForm << "]";
	o << ",";

	if (cell.m_digits>-1000) o << "digits=" << cell.m_digits << ",";
	if (cell.m_protected) o << "protected,";
	if (cell.m_backgroundColor != 0xFFFFFF)
	{
		std::ios::fmtflags oldflags = o.setf(std::ios::hex, std::ios::basefield);
		o << "backColor=" << cell.m_backgroundColor << ",";
		o.setf(oldflags, std::ios::basefield);
	}
	for (size_t i = 0; i < cell.m_bordersList.size(); i++)
	{
		if (cell.m_bordersList[i].m_style == WPSBorder::None)
			continue;
		o << "bord";
		if (i < 6)
		{
			char const *wh[] = { "L", "R", "T", "B", "MiddleH", "MiddleV" };
			o << wh[i];
		}
		else o << "[#wh=" << i << "]";
		o << "=" << cell.m_bordersList[i] << ",";
	}
	return o;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
void WPSCell::addTo(librevenge::RVNGPropertyList &propList) const
{
	propList.insert("librevenge:column", position()[0]);
	propList.insert("librevenge:row", position()[1]);

	propList.insert("table:number-columns-spanned", numSpannedCells()[0]);
	propList.insert("table:number-rows-spanned", numSpannedCells()[1]);

	WPSCellFormat::addTo(propList);
}

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
