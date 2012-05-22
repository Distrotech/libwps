/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2005 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include "WPSSubDocument.h"

WPSSubDocument::WPSSubDocument(WPXInputStreamPtr &input, WPSParser *parser, int id)  : m_input(input), m_parser(parser), m_id(id)
{
}

WPSSubDocument::~WPSSubDocument()
{
}

bool WPSSubDocument::operator==(shared_ptr<WPSSubDocument> const &doc) const
{
	if (!doc) return false;
	if (doc.get() == this) return true;
	if (m_input.get() != doc.get()->m_input.get()) return false;
	if (m_parser != doc.get()->m_parser) return false;
	if (m_id != doc.get()->m_id) return false;
	return true;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */

