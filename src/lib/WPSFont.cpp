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

#include <librevenge/librevenge.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WPSListener.h"
#include "WPSFont.h"

std::ostream &operator<<(std::ostream &o, WPSFont const &ft)
{
	uint32_t flags = ft.m_attributes;
	if (!ft.m_name.empty())
		o << "nam='" << ft.m_name.cstr() << "',";
	if (ft.m_size > 0) o << "sz=" << ft.m_size << ",";

	if (flags) o << "fl=";
	if (flags&WPS_BOLD_BIT) o << "b:";
	if (flags&WPS_ITALICS_BIT) o << "it:";
	if (flags&WPS_UNDERLINE_BIT) o << "underL:";
	if (flags&WPS_OVERLINE_BIT) o << "overL:";
	if (flags&WPS_EMBOSS_BIT) o << "emboss:";
	if (flags&WPS_SHADOW_BIT) o << "shadow:";
	if (flags&WPS_OUTLINE_BIT) o << "outline:";
	if (flags&WPS_DOUBLE_UNDERLINE_BIT) o << "2underL:";
	if (flags&WPS_STRIKEOUT_BIT) o << "strikeout:";
	if (flags&WPS_SMALL_CAPS_BIT) o << "smallCaps:";
	if (flags&WPS_ALL_CAPS_BIT) o << "allCaps:";
	if (flags&WPS_HIDDEN_BIT) o << "hidden:";
	if (flags&WPS_SMALL_PRINT_BIT) o << "consended:";
	if (flags&WPS_LARGE_BIT) o << "extended:";
	if (flags&WPS_SUPERSCRIPT_BIT) o << "superS:";
	if (flags&WPS_SUBSCRIPT_BIT) o << "subS:";
	if (flags) o << ",";

	if (ft.m_spacing<0)
		o << "condensed=" << -ft.m_spacing << "pt,";
	else if (ft.m_spacing>0)
		o << "extended=" << ft.m_spacing << "pt,";
	if (!ft.m_color.isBlack()) o << "col=" << ft.m_color << ",";
	if (ft.m_extra.length()) o << "extra=" << ft.m_extra << ",";
	return o;
}

bool WPSFont::operator==(WPSFont const &ft) const
{
	if (m_size < ft.m_size || m_size > ft.m_size ||
	        m_attributes != ft.m_attributes || m_color != ft.m_color ||
	        m_spacing < ft.m_spacing || m_spacing > ft.m_spacing ||
	        m_languageId != ft.m_languageId)
		return false;
	if (m_name != ft.m_name || m_extra.compare(ft.m_extra))
		return false;
	return true;
}

void WPSFont::addTo(librevenge::RVNGPropertyList &propList) const
{
	double fontSizeChange = 1.0;
	switch (m_attributes& 0x0000001f)
	{
	case 0x01:  // Extra large
		fontSizeChange = 2.0;
		break;
	case 0x02: // Very large
		fontSizeChange = 1.5;
		break;
	case 0x04: // Large
		fontSizeChange = 1.2;
		break;
	case 0x08: // Small print
		fontSizeChange = 0.8;
		break;
	case 0x10: // Fine print
		fontSizeChange = 0.6;
		break;
	default: // Normal
		fontSizeChange = 1.0;
		break;
	}

	if (m_attributes & WPS_SUPERSCRIPT_BIT)
		propList.insert("style:text-position", "super 58%");
	else if (m_attributes & WPS_SUBSCRIPT_BIT)
		propList.insert("style:text-position", "sub 58%");
	if (m_attributes & WPS_ITALICS_BIT)
		propList.insert("fo:font-style", "italic");
	if (m_attributes & WPS_BOLD_BIT)
		propList.insert("fo:font-weight", "bold");
	if (m_attributes & WPS_STRIKEOUT_BIT)
		propList.insert("style:text-line-through-type", "single");
	if (m_attributes & WPS_DOUBLE_UNDERLINE_BIT)
		propList.insert("style:text-underline-type", "double");
	else if (m_attributes & WPS_UNDERLINE_BIT)
		propList.insert("style:text-underline-type", "single");
	if (m_attributes & WPS_OVERLINE_BIT)
		propList.insert("style:text-overline-type", "single");
	if (m_attributes & WPS_OUTLINE_BIT)
		propList.insert("style:text-outline", "true");
	if (m_attributes & WPS_SMALL_CAPS_BIT)
		propList.insert("fo:font-variant", "small-caps");
	if (m_attributes & WPS_BLINK_BIT)
		propList.insert("style:text-blinking", "true");
	if (m_attributes & WPS_SHADOW_BIT)
		propList.insert("fo:text-shadow", "1pt 1pt");
	if (m_attributes & WPS_HIDDEN_BIT)
		propList.insert("text:display", "none");
	if (m_attributes & WPS_ALL_CAPS_BIT)
		propList.insert("fo:text-transform", "uppercase");
	if (m_attributes & WPS_EMBOSS_BIT)
		propList.insert("style:font-relief", "embossed");
	else if (m_attributes & WPS_ENGRAVE_BIT)
		propList.insert("style:font-relief", "engraved");

	if (!m_name.empty())
		propList.insert("style:font-name", m_name);
	if (m_size>0)
		propList.insert("fo:font-size", fontSizeChange*m_size, librevenge::RVNG_POINT);
	if (m_spacing < 0 || m_spacing > 0)
		propList.insert("fo:letter-spacing", m_spacing, librevenge::RVNG_POINT);

	propList.insert("fo:color", m_color.str().c_str());

	if (m_languageId < 0)
		libwps_tools_win::Language::addLocaleName(0x409, propList);
	if (m_languageId > 0)
		libwps_tools_win::Language::addLocaleName(m_languageId, propList);
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
