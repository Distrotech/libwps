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
#include <iomanip>
#include <sstream>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"

#include "WPSContentListener.h"
#include "WPSList.h"

#include "WPSParagraph.h"

void WPSTabStop::addTo(librevenge::RVNGPropertyListVector &propList, double decalX) const
{
	librevenge::RVNGPropertyList tab;

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
		librevenge::RVNGString sLeader;
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

	switch (pp.m_justify)
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
		WPS_DEBUG_MSG(("WPSParagraph:operator<<: called with unknown justification\n"));
		o << "just=" << static_cast<unsigned>(pp.m_justify) << ", ";
		break;
	}

	if (pp.m_tabs.size())
	{
		o << "tabs=(";
		for (size_t i = 0; i < pp.m_tabs.size(); i++)
			o << pp.m_tabs[i] << ",";
		o << "),";
	}
	if (!pp.m_backgroundColor.isWhite())
		o << "backgroundColor=" << pp.m_backgroundColor << ",";
	if (pp.m_listLevelIndex >= 1)
		o << pp.m_listLevel << ":" << pp.m_listLevelIndex <<",";

	if (pp.m_border)
	{
		o << "bord(" << pp.m_borderStyle << ")";
		o << "=";
		if (pp.m_border&WPSBorder::TopBit) o << "T";
		if (pp.m_border&WPSBorder::BottomBit) o << "B";
		if (pp.m_border&WPSBorder::LeftBit) o << "L";
		if (pp.m_border&WPSBorder::RightBit) o << "R";
		o << ",";
	}

	if (!pp.m_extra.empty()) o << "extras=(" << pp.m_extra << ")";
	return o;
}

void WPSParagraph::addTo(librevenge::RVNGPropertyList &propList, bool inTable) const
{
	switch (m_justify)
	{
	case libwps::JustificationLeft:
		// doesn't require a paragraph prop - it is the default
		propList.insert("fo:text-align", "left");
		break;
	case libwps::JustificationCenter:
		propList.insert("fo:text-align", "center");
		break;
	case libwps::JustificationRight:
		propList.insert("fo:text-align", "end");
		break;
	case libwps::JustificationFull:
		propList.insert("fo:text-align", "justify");
		break;
	case libwps::JustificationFullAllLines:
		propList.insert("fo:text-align", "justify");
		propList.insert("fo:text-align-last", "justify");
		break;
	default:
		break;
	}
	if (!inTable)
	{
		// these properties are not appropriate when a table is opened..
		propList.insert("fo:margin-left", m_listLevelIndex >= 1 ? m_listLevel.m_labelIndent : m_margins[1]);
		propList.insert("fo:text-indent", m_margins[0]);
		propList.insert("fo:margin-right", m_margins[2]);
		if (!m_backgroundColor.isWhite())
			propList.insert("fo:background-color", m_backgroundColor.str().c_str());
		if (m_border && m_borderStyle.m_style != WPSBorder::None)
		{
			int border = m_border;
			if (border == 0xF)
				m_borderStyle.addTo(propList);
			else
			{
				if (border & WPSBorder::LeftBit)
					m_borderStyle.addTo(propList, "left");
				if (border & WPSBorder::RightBit)
					m_borderStyle.addTo(propList, "right");
				if (border & WPSBorder::TopBit)
					m_borderStyle.addTo(propList, "top");
				if (border & WPSBorder::BottomBit)
					m_borderStyle.addTo(propList, "bottom");
			}
		}
	}
	// Note:
	// as we can not use percent, this may give a good approximation
	propList.insert("fo:margin-top", (10.*m_spacings[1])/72., librevenge::RVNG_INCH);
	propList.insert("fo:margin-bottom", (10.*m_spacings[2])/72., librevenge::RVNG_INCH);
	propList.insert("fo:line-height", m_spacings[0] <= 0 ? 1.0 : m_spacings[0], librevenge::RVNG_PERCENT);
	librevenge::RVNGPropertyListVector tabStops;
	for (size_t i=0; i< m_tabs.size(); i++)
		m_tabs[i].addTo(tabStops, 0);
	if (tabStops.count())
		propList.insert("style:tab-stops", tabStops);
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
