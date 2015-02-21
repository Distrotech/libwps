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

#include <stdio.h>
#include <time.h>

#include <sstream>

#include <librevenge/librevenge.h>

#include "WPSCell.h"

bool WPSCellFormat::convertDTFormat(std::string const &dtFormat, librevenge::RVNGPropertyListVector &propVect)
{
	propVect.clear();
	size_t len=dtFormat.size();
	std::string text("");
	librevenge::RVNGPropertyList list;
	for (size_t c=0; c < len; ++c)
	{
		if (dtFormat[c]!='%' || c+1==len)
		{
			text+=dtFormat[c];
			continue;
		}
		char ch=dtFormat[++c];
		if (ch=='%')
		{
			text += '%';
			continue;
		}
		if (!text.empty())
		{
			list.clear();
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", text.c_str());
			propVect.append(list);
			text.clear();
		}
		list.clear();
		switch (ch)
		{
		case 'Y':
			list.insert("number:style", "long");
		case 'y':
			list.insert("librevenge:value-type", "year");
			propVect.append(list);
			break;
		case 'B':
			list.insert("number:style", "long");
		case 'b':
		case 'h':
			list.insert("librevenge:value-type", "month");
			list.insert("number:textual", true);
			propVect.append(list);
			break;
		case 'm':
			list.insert("librevenge:value-type", "month");
			propVect.append(list);
			break;
		case 'e':
			list.insert("number:style", "long");
		// fall-through intended
		case 'd':
			list.insert("librevenge:value-type", "day");
			propVect.append(list);
			break;
		case 'A':
			list.insert("number:style", "long");
		case 'a':
			list.insert("librevenge:value-type", "day-of-week");
			propVect.append(list);
			break;

		case 'H':
			list.insert("number:style", "long");
		// fall-through intended
		case 'I':
			list.insert("librevenge:value-type", "hours");
			propVect.append(list);
			break;
		case 'M':
			list.insert("librevenge:value-type", "minutes");
			list.insert("number:style", "long");
			propVect.append(list);
			break;
		case 'S':
			list.insert("librevenge:value-type", "seconds");
			list.insert("number:style", "long");
			propVect.append(list);
			break;
		case 'p':
			list.insert("librevenge:value-type", "text");
			list.insert("librevenge:text", " ");
			propVect.append(list);
			list.clear();
			list.insert("librevenge:value-type", "am-pm");
			propVect.append(list);
			break;
		default:
			WPS_DEBUG_MSG(("WPSCellFormat::convertDTFormat: find unimplement command %c(ignored)\n", ch));
		}
	}
	if (!text.empty())
	{
		list.clear();
		list.insert("librevenge:value-type", "text");
		list.insert("librevenge:text", text.c_str());
		propVect.append(list);
	}
	return propVect.count()!=0;
}

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

	switch (m_hAlign)
	{
	case HALIGN_LEFT:
		propList.insert("fo:text-align", "start");
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
	switch (vAlignement())
	{
	case VALIGN_TOP:
		propList.insert("style:vertical-align", "top");
		break;
	case VALIGN_CENTER:
		propList.insert("style:vertical-align", "middle");
		break;
	case VALIGN_BOTTOM:
		propList.insert("style:vertical-align", "bottom");
		break;
	case VALIGN_DEFAULT:
		break; // default
	default:
		WPS_DEBUG_MSG(("MWAWCell::addTo: called with unknown valign=%d\n", vAlignement()));
	}

	for (size_t c = 0; c < m_bordersList.size(); c++)
	{
		if (m_bordersList[c].isEmpty()) continue;
		switch (c)
		{
		case WPSBorder::Left:
			m_bordersList[c].addTo(propList, "left");
			break;
		case WPSBorder::Right:
			m_bordersList[c].addTo(propList, "right");
			break;
		case WPSBorder::Top:
			m_bordersList[c].addTo(propList, "top");
			break;
		case WPSBorder::Bottom:
			m_bordersList[c].addTo(propList, "bottom");
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

std::string WPSCellFormat::getValueType() const
{
	switch (m_format)
	{
	case F_NUMBER:
		switch (m_subFormat)
		{
		case 0: // default
		case 1: // decimal
		case 5: // thousand
		case 6: // fixed
		case 7: // fraction
		default:
			return "float";
		case 2:
			return "scientific";
		case 3:
			return "percentage";
		case 4:
			return "currency";
		}
	case F_BOOLEAN:
		return "boolean";
	case F_DATE:
		return "date";
	case F_TIME:
		return "time";
	case F_TEXT:
	case F_UNKNOWN:
	default:
		break;
	}
	return "float";
}

bool WPSCellFormat::getNumberingProperties(librevenge::RVNGPropertyList &propList) const
{
	librevenge::RVNGPropertyListVector pVect;
	switch (m_format)
	{
	case F_BOOLEAN:
		propList.insert("librevenge:value-type", "boolean");
		break;
	case F_NUMBER:
		if (m_digits>-1000)
			propList.insert("number:decimal-places", m_digits);
		switch (m_subFormat)
		{
		case 5: // thousand
			propList.insert("number:grouping", true);
		// fall-through intended
		case 0: // default
			if (m_subFormat==0)
				propList.remove("number:decimal-places");
		// fall-through intended
		case 1: // decimal
			propList.insert("librevenge:value-type", "number");
			break;
		case 2:
			propList.insert("librevenge:value-type", "scientific");
			break;
		case 3:
			propList.insert("librevenge:value-type", "percentage");
			break;
		case 6: // fixed
			propList.insert("librevenge:value-type", "number");
			propList.insert("number:min-integer-digits", m_digits+1);
			propList.insert("number:decimal-places", 0);
			break;
		case 7:
			propList.insert("librevenge:value-type", "fraction");
			propList.insert("number:min-integer-digits", 0);
			propList.insert("number:min-numerator-digits", 1);
			propList.insert("number:min-denominator-digits", 1);
			propList.remove("number:decimal-places");
			break;
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
			pVect.append(list);
			break;
		}
		default:
			return false;
		}
		break;
	case F_DATE:
		propList.insert("librevenge:value-type", "date");
		propList.insert("number:automatic-order", "true");
		if (!convertDTFormat(m_DTFormat.empty() ? "%m/%d/%Y" : m_DTFormat, pVect))
			return false;
		break;
	case F_TIME:
		propList.insert("librevenge:value-type", "time");
		propList.insert("number:automatic-order", "true");
		if (!convertDTFormat(m_DTFormat.empty() ? "%H:%M:%S" : m_DTFormat, pVect))
			return false;
		break;
	case F_TEXT:
	case F_UNKNOWN:
	default:
		return false;
	}
	propList.insert("librevenge:format", pVect);
	return true;
}

int WPSCellFormat::compare(WPSCellFormat const &cell, bool onlyNumbering) const
{
	if (m_format<cell.m_format) return 1;
	if (m_format>cell.m_format) return -1;
	if (m_subFormat<cell.m_subFormat) return 1;
	if (m_subFormat>cell.m_subFormat) return -1;
	if (m_DTFormat<cell.m_DTFormat) return 1;
	if (m_DTFormat>cell.m_DTFormat) return -1;
	if (m_digits<cell.m_digits) return 1;
	if (m_digits>cell.m_digits) return -1;
	if (onlyNumbering) return 0;
	int diff = int(m_hAlign) - int(cell.m_hAlign);
	if (diff) return diff;
	diff = int(m_vAlign) - int(cell.m_vAlign);
	if (diff) return diff;
	diff = int(m_backgroundColor) - int(cell.m_backgroundColor);
	if (diff) return diff;
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
	o << "font=[" << cell.m_font << "],";
	switch (cell.m_hAlign)
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
	switch (cell.m_vAlign)
	{
	case WPSCellFormat::VALIGN_TOP:
		o << "yTop,";
		break;
	case WPSCellFormat::VALIGN_CENTER:
		o << "yCenter,";
		break;
	case WPSCellFormat::VALIGN_BOTTOM:
		o << "yBottom,";
		break;
	case WPSCellFormat::VALIGN_DEFAULT:
	default:
		break; // default
	}
	int subForm = cell.m_subFormat;
	switch (cell.m_format)
	{
	case WPSCellFormat::F_BOOLEAN:
		o << "boolean";
		break;
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
		switch (subForm)
		{
		case 1:
			o << "[decimal]";
			break;
		case 2:
			o << "[exp]";
			break;
		case 3:
			o << "[percent]";
			break;
		case 4:
			o << "[money]";
			break;
		case 5:
			o << "[thousand]";
			break;
		case 6:
			o << "[fixed]";
			break;
		case 7:
			o << "[fraction]";
			break;
		default:
			o << "[Fo" << subForm << "]";
		case 0:
			break;
		}
		subForm=0;
		break;
	case WPSCellFormat::F_DATE:
		o << "date[" << cell.getDTFormat() << "]";
		break;
	case WPSCellFormat::F_TIME:
		o << "time[" << cell.getDTFormat() << "]";
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
	if (cell.m_box!=Box2f())
		o << "box=" << cell.m_box << ",";
	if (cell.m_verticalSet) o << "ySet,";
	o << static_cast<WPSCellFormat const &>(cell);

	return o;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
