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

#include <libwpd/libwpd.h>

#include "libwps_internal.h"

#include "WPSFont.h"

std::ostream &operator<<(std::ostream &o, WPSFont const &ft)
{
	uint32_t flags = ft.m_attributes;
	if (!ft.m_name.empty())
		o << "nam='" << ft.m_name << "',";
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

	if (ft.m_color) o << "col=" << std::hex << ft.m_color << std::dec << ",";
	if (ft.m_extra.length()) o << "extra=" << ft.m_extra << ",";
	return o;
}

bool WPSFont::operator==(WPSFont const &ft) const
{
	if (m_size != ft.m_size || m_attributes != ft.m_attributes
	        || m_color != ft.m_color || m_languageId != ft.m_languageId)
		return false;
	if (m_name.compare(ft.m_name) || m_extra.compare(ft.m_extra))
		return false;
	return true;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
