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

#include <libwpd/WPXPropertyListVector.h>

#include "libwps_internal.h"

#include "WPSContentListener.h"
#include "WPSList.h"

#include "WPSParagraph.h"

void WPSTabStop::addTo(WPXPropertyListVector &propList, double decalX)
{
	WPXPropertyList tab;

	// type
	switch (m_alignment)
	{
	case RIGHT:
		tab.insert("style:type", "right");
		break;
	case CENTER:
		tab.insert("style:type", "center");
		break;
	case DECIMAL:
		tab.insert("style:type", "char");
		tab.insert("style:char", "."); // Assume a decimal point for now
		break;
	case BAR: // BAR is not handled in OOo
	case LEFT:
	default:
		break;
	}

	// leader character
	if (m_leaderCharacter != 0x0000)
	{
		WPXString sLeader;
		sLeader.sprintf("%c", m_leaderCharacter);
		tab.insert("style:leader-text", sLeader);
		tab.insert("style:leader-style", "solid");
	}

	// position
	double position = m_position+decalX;
	if (position < 0.00005f && position > -0.00005f)
		position = 0.0;
	tab.insert("style:position", position);

	propList.append(tab);
}

//! operator<<
std::ostream &operator<<(std::ostream &o, WPSTabStop const &tab)
{
	o << tab.m_position;

	switch (tab.m_alignment)
	{
	case WPSTabStop::LEFT:
		o << "L";
		break;
	case WPSTabStop::CENTER:
		o << "C";
		break;
	case WPSTabStop::RIGHT:
		o << "R";
		break;
	case WPSTabStop::DECIMAL:
		o << ":decimal";
		break;
	case WPSTabStop::BAR:
		o << ":bar";
		break;
	default:
		o << ":#type=" << int(tab.m_alignment);
		break;
	}
	if (tab.m_leaderCharacter != '\0')
		o << ":sep='"<< (char) tab.m_leaderCharacter << "'";
	return o;
}

//! operator<<
std::ostream &operator<<(std::ostream &o, WPSParagraph const &pp)
{
	if (pp.m_margins[0]<0||pp.m_margins[0]>0)
		o << "textIndent=" << pp.m_margins[0] << ",";
	if (pp.m_margins[1]<0||pp.m_margins[1]>0)
		o << "leftMarg=" << pp.m_margins[1] << ",";
	if (pp.m_margins[2]<0||pp.m_margins[2]>0)
		o << "rightMarg=" << pp.m_margins[2] << ",";

	if (pp.m_spacings[0] < 1.0 || pp.m_spacings[0] > 1.0)
		o << "interLineSpacing=" << pp.m_spacings[0] << ",";
	if (pp.m_spacings[1]<0||pp.m_spacings[1]>0)
		o << "befSpacing=" << pp.m_spacings[1] << ",";
	if (pp.m_spacings[2]<0||pp.m_spacings[2]>0)
		o << "aftSpacing=" << pp.m_spacings[2] << ",";

	if (pp.m_breakStatus & libwps::NoBreakBit) o << "dontbreak,";
	if (pp.m_breakStatus & libwps::NoBreakWithNextBit) o << "dontbreakafter,";

	switch(pp.m_justify)
	{
	case libwps::JustificationLeft:
		break;
	case libwps::JustificationCenter:
		o << "just=centered, ";
		break;
	case libwps::JustificationRight:
		o << "just=right, ";
		break;
	case libwps::JustificationFull:
		o << "just=full, ";
		break;
	case libwps::JustificationFullAllLines:
		o << "just=fullAllLines, ";
		break;
	default:
		o << "just=" << pp.m_justify << ", ";
		break;
	}

	if (pp.m_tabs.size())
	{
		o << "tabs=(";
		for (size_t i = 0; i < pp.m_tabs.size(); i++)
			o << pp.m_tabs[i] << ",";
		o << "),";
	}
	if (pp.m_backgroundColor != 0xFFFFFF)
		o << "backgroundColor=" << std::hex << pp.m_backgroundColor << std::dec << ",";
	if (pp.m_listLevelIndex >= 1)
		o << pp.m_listLevel << ":" << pp.m_listLevelIndex <<",";

	if (pp.m_border)
	{
		o << "bord";
		switch (pp.m_borderStyle)
		{
		case libwps::BorderSingle:
			break;
		case libwps::BorderDot:
			o << "(dot)";
			break;
		case libwps::BorderLargeDot:
			o << "(large dot)";
			break;
		case libwps::BorderDash:
			o << "(dash)";
			break;
		case libwps::BorderDouble:
			o << "(double)";
			break;
		default:
			WPS_DEBUG_MSG(("WPSParagraph::operator<<: find unknown style\n"));
			o << "(#style=" << int(pp.m_borderStyle) << "),";
			break;
		}
		o << "=";
		if (pp.m_border&libwps::TopBorderBit) o << "T";
		if (pp.m_border&libwps::BottomBorderBit) o << "B";
		if (pp.m_border&libwps::LeftBorderBit) o << "L";
		if (pp.m_border&libwps::RightBorderBit) o << "R";
		if (pp.m_borderWidth > 1) o << "(w=" << pp.m_borderWidth << ")";
		if (pp.m_borderColor)
			o << "(col=" << std::hex << pp.m_borderColor << std::dec << "),";
		o << ",";
	}

	if (!pp.m_extra.empty()) o << "extras=(" << pp.m_extra << ")";
	return o;
}

void WPSParagraph::send(shared_ptr<WPSContentListener> listener) const
{
	if (!listener)
		return;
	listener->setParagraphJustification(m_justify);
	listener->setTabs(m_tabs);

	double leftMargin = m_margins[1];
	WPSList::Level level;
	if (m_listLevelIndex >= 1)
	{
		level = m_listLevel;
		level.m_labelWidth = (m_margins[1]-level.m_labelIndent);
		if (level.m_labelWidth<0.1)
			level.m_labelWidth = 0.1;
		leftMargin=level.m_labelIndent;
		level.m_labelIndent = 0;
	}
	listener->setParagraphMargin(leftMargin, WPS_LEFT);
	listener->setParagraphMargin(m_margins[2], WPS_RIGHT);
	listener->setParagraphTextIndent(m_margins[0]);

	double interline = m_spacings[0];
	listener->setParagraphLineSpacing(interline>0.0 ? interline : 1.0);

	// Note:
	// as we can not use percent, this may give a good approximation
	listener->setParagraphMargin((10.*m_spacings[1])/72.,WPS_TOP);
	listener->setParagraphMargin((10.*m_spacings[2])/72.,WPS_BOTTOM);

	if (m_listLevelIndex >= 1)
	{
		if (!listener->getCurrentList())
			listener->setCurrentList(shared_ptr<WPSList>(new WPSList));
		listener->getCurrentList()->set(m_listLevelIndex, level);
		listener->setCurrentListLevel(m_listLevelIndex);
	}
	else
		listener->setCurrentListLevel(0);

	listener->setParagraphBackgroundColor(m_backgroundColor);
	listener->setParagraphBorders(m_border, m_borderStyle, m_borderWidth, m_borderColor);
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
