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

#ifndef WPS_POSITION_H
#define WPS_POSITION_H

#include <ostream>

#include <librevenge/librevenge.h>

#include "libwps_internal.h"

/** Class to define the position of an object (textbox, picture, ..) in the document
 *
 * Stores the page, object position, object size, anchor, wrapping, ...
 */
class WPSPosition
{
public:
	//! a list of enum used to defined the anchor
	enum AnchorTo { Char, CharBaseLine, Paragraph, ParagraphContent, Page, PageContent };
	//! an enum used to define the wrapping
	enum Wrapping { WNone, WDynamic, WRunThrough }; // Add something for background ?
	//! an enum used to define the relative X position
	enum XPos { XRight, XLeft, XCenter, XFull };
	//! an enum used to define the relative Y position
	enum YPos { YTop, YBottom, YCenter, YFull };

public:
	//! constructor
	WPSPosition(Vec2f const &orig=Vec2f(), Vec2f const &sz=Vec2f(), librevenge::RVNGUnit unt=librevenge::RVNG_INCH):
		m_anchorTo(Char), m_xPos(XLeft), m_yPos(YTop), m_wrapping(WNone),
		m_page(0), m_orig(orig), m_size(sz), m_naturalSize(), m_unit(unt), m_order(0) {}

	virtual ~WPSPosition() {}
	//! operator<<
	friend  std::ostream &operator<<(std::ostream &o, WPSPosition const &pos)
	{
		Vec2f dest(pos.m_orig+pos.m_size);
		o << "Pos=" << pos.m_orig << "x" << dest;
		switch (pos.m_unit)
		{
		case librevenge::RVNG_INCH:
			o << "(inch)";
			break;
		case librevenge::RVNG_POINT:
			o << "(pt)";
			break;
		case librevenge::RVNG_TWIP:
			o << "(tw)";
			break;
		case librevenge::RVNG_PERCENT:
		case librevenge::RVNG_GENERIC:
		case librevenge::RVNG_UNIT_ERROR:
		default:
			break;
		}
		if (pos.page()>0) o << ", page=" << pos.page();
		return o;
	}
	//! basic operator==
	bool operator==(WPSPosition const &f) const
	{
		return cmp(f) == 0;
	}
	//! basic operator!=
	bool operator!=(WPSPosition const &f) const
	{
		return cmp(f) != 0;
	}
	//! basic operator<
	bool operator<(WPSPosition const &f) const
	{
		return cmp(f) < 0;
	}

	//! returns the frame page
	int page() const
	{
		return m_page;
	}
	//! return the frame origin
	Vec2f const &origin() const
	{
		return m_orig;
	}
	//! returns the frame size
	Vec2f const &size() const
	{
		return m_size;
	}
	//! returns the natural size (if known)
	Vec2f const &naturalSize() const
	{
		return m_naturalSize;
	}
	//! returns the unit
	librevenge::RVNGUnit unit() const
	{
		return m_unit;
	}
	//! returns a float which can be used to convert between to unit
	static float getScaleFactor(librevenge::RVNGUnit orig, librevenge::RVNGUnit dest)
	{
		float actSc = 1.0, newSc = 1.0;
		switch (orig)
		{
		case librevenge::RVNG_TWIP:
			break;
		case librevenge::RVNG_POINT:
			actSc=20;
			break;
		case librevenge::RVNG_INCH:
			actSc = 1440;
			break;
		case librevenge::RVNG_PERCENT:
		case librevenge::RVNG_GENERIC:
		case librevenge::RVNG_UNIT_ERROR:
		default:
			WPS_DEBUG_MSG(("WPSPosition::getScaleFactor %d unit must not appear\n", int(orig)));
		}
		switch (dest)
		{
		case librevenge::RVNG_TWIP:
			break;
		case librevenge::RVNG_POINT:
			newSc=20;
			break;
		case librevenge::RVNG_INCH:
			newSc = 1440;
			break;
		case librevenge::RVNG_PERCENT:
		case librevenge::RVNG_GENERIC:
		case librevenge::RVNG_UNIT_ERROR:
		default:
			WPS_DEBUG_MSG(("WPSPosition::getScaleFactor %d unit must not appear\n", int(dest)));
		}
		return actSc/newSc;
	}
	//! returns a float which can be used to scale some data in object unit
	float getInvUnitScale(librevenge::RVNGUnit unt) const
	{
		return getScaleFactor(unt, m_unit);
	}

	//! sets the page
	void setPage(int pg) const
	{
		const_cast<WPSPosition *>(this)->m_page = pg;
	}
	//! sets the frame origin
	void setOrigin(Vec2f const &orig)
	{
		m_orig = orig;
	}
	//! sets the frame size
	void setSize(Vec2f const &sz)
	{
		m_size = sz;
	}
	//! sets the natural size (if known)
	void setNaturalSize(Vec2f const &natSize)
	{
		m_naturalSize = natSize;
	}
	//! sets the dimension unit
	void setUnit(librevenge::RVNGUnit unt)
	{
		m_unit = unt;
	}
	//! sets/resets the page and the origin
	void setPagePos(int pg, Vec2f const &newOrig) const
	{
		const_cast<WPSPosition *>(this)->m_page = pg;
		const_cast<WPSPosition *>(this)->m_orig = newOrig;
	}

	//! sets the relative position
	void setRelativePosition(AnchorTo anchor, XPos X = XLeft, YPos Y=YTop)
	{
		m_anchorTo = anchor;
		m_xPos = X;
		m_yPos = Y;
	}

	//! returns background/foward order
	int order() const
	{
		return m_order;
	}
	//! set background/foward order
	void setOrder(int ord) const
	{
		m_order = ord;
	}

	//! anchor position
	AnchorTo m_anchorTo;
	//! X relative position
	XPos m_xPos;
	//! Y relative position
	YPos m_yPos;
	//! Wrapping
	Wrapping m_wrapping;

protected:
	//! basic function to compare two positions
	int cmp(WPSPosition const &f) const
	{
		int diff = int(m_anchorTo) - int(f.m_anchorTo);
		if (diff) return diff < 0 ? -1 : 1;
		diff = int(m_xPos) - int(f.m_xPos);
		if (diff) return diff < 0 ? -1 : 1;
		diff = int(m_yPos) - int(f.m_yPos);
		if (diff) return diff < 0 ? -1 : 1;
		diff = page() - f.page();
		if (diff) return diff < 0 ? -1 : 1;
		diff = int(m_unit) - int(f.m_unit);
		if (diff) return diff < 0 ? -1 : 1;
		diff = m_orig.cmpY(f.m_orig);
		if (diff) return diff;
		diff = m_size.cmpY(f.m_size);
		if (diff) return diff;
		diff = m_naturalSize.cmpY(f.m_naturalSize);
		if (diff) return diff;

		return 0;
	}

	//! the page
	int m_page;
	Vec2f m_orig /** the origin position in a page */, m_size /* the size of the data*/, m_naturalSize /** the natural size of the data (if known) */;
	//! the unit used in \a orig and in \a m_size. Default: in inches
	librevenge::RVNGUnit m_unit;
	//! background/foward order
	mutable int m_order;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
