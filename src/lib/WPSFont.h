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

#ifndef WPS_FONT
#  define WPS_FONT

#include <assert.h>
#include <iostream>

#include <string>

#include "libwps_internal.h"

/** define the font properties */
class WPSFont
{
public:
	//! constructor
	WPSFont() : m_name(""), m_size(0), m_attributes(0), m_color(0), m_languageId(-1), m_extra("") {}
	//! returns the default font ( Courier 12pt)
	static WPSFont getDefault()
	{
		WPSFont res;
		res.m_name = "Courier";
		res.m_size = 12;
		return res;
	}
	//! destructor
	virtual ~WPSFont() {}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, WPSFont const &ft);

	//! add to the propList
	void addTo(librevenge::RVNGPropertyList &propList) const;

	//! accessor
	bool isSet() const
	{
		return !m_name.empty();
	}

	//! operator==
	bool operator==(WPSFont const &ft) const;
	//! operator!=
	bool operator!=(WPSFont const &ft) const
	{
		return !operator==(ft);
	}

	//! font name
	std::string m_name;
	//! font size
	double m_size;
	//! the font attributes defined as a set of bits
	uint32_t m_attributes;
	//! the font color
	uint32_t m_color;
	//! the language (simplified locale name id) if known
	int m_languageId;

	//! public field use to add a message when the font is printed
	std::string m_extra;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
