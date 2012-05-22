/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 *
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

#ifndef WPS_POSITION_H
#define WPS_POSITION_H

#include <ostream>

#include <libwpd/WPXProperty.h>

#include "libwps_internal.h"

/** Class to define the position of an object (textbox, picture, ..) in the document
 *
 * Stores the page, object position, object size, anchor, wrapping, ...
 */
class WPSPosition
{
public:
	//! a list of enum used to defined the anchor
	enum AnchorTo { Char, CharBaseLine, Paragraph, Page };
	//! an enum used to define the wrapping
	enum Wrapping { WNone, WDynamic, WRunThrough }; // Add something for background ?
	//! an enum used to define the relative X position
	enum XPos { XRight, XLeft, XCenter, XFull };
	//! an enum used to define the relative Y position
	enum YPos { YTop, YBottom, YCenter, YFull };

public:
	//! constructor
	WPSPosition(Vec2f const &origin=Vec2f(), Vec2f const &size=Vec2f(), WPXUnit unit=WPX_INCH):
		m_anchorTo(Char), m_xPos(XLeft), m_yPos(YTop), m_wrapping(WNone),
		m_page(0), m_orig(origin), m_size(size), m_naturalSize(), m_unit(unit), m_order(0) {}

	virtual ~WPSPosition() {}
	//! operator<<
	friend  std::ostream &operator<<(std::ostream &o, WPSPosition const &pos)
	{
		Vec2f dest(pos.m_orig+pos.m_size);
		o << "Pos=" << pos.m_orig << "x" << dest;
		switch(pos.m_unit)
		{
		case WPX_INCH:
			o << "(inch)";
			break;
		case WPX_POINT:
			o << "(pt)";
			break;
		case WPX_TWIP:
			o << "(tw)";
			break;
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
	WPXUnit unit() const
	{
		return m_unit;
	}
	//! returns a float which can be used to convert between to unit
	static float getScaleFactor(WPXUnit orig, WPXUnit dest)
	{
		float actSc = 1.0, newSc = 1.0;
		switch(orig)
		{
		case WPX_TWIP:
			break;
		case WPX_POINT:
			actSc=20;
			break;
		case WPX_INCH:
			actSc = 1440;
			break;
		default:
			WPS_DEBUG_MSG(("WPSPosition::getScaleFactor %d unit must not appear\n", int(orig)));
		}
		switch(dest)
		{
		case WPX_TWIP:
			break;
		case WPX_POINT:
			newSc=20;
			break;
		case WPX_INCH:
			newSc = 1440;
			break;
		default:
			WPS_DEBUG_MSG(("WPSPosition::getScaleFactor %d unit must not appear\n", int(dest)));
		}
		return actSc/newSc;
	}
	//! returns a float which can be used to scale some data in object unit
	float getInvUnitScale(WPXUnit unit) const
	{
		return getScaleFactor(unit, m_unit);
	}

	//! sets the page
	void setPage(int page) const
	{
		const_cast<WPSPosition *>(this)->m_page = page;
	}
	//! sets the frame origin
	void setOrigin(Vec2f const &origin)
	{
		m_orig = origin;
	}
	//! sets the frame size
	void setSize(Vec2f const &size)
	{
		m_size = size;
	}
	//! sets the natural size (if known)
	void setNaturalSize(Vec2f const &naturalSize)
	{
		m_naturalSize = naturalSize;
	}
	//! sets the dimension unit
	void setUnit(WPXUnit unit)
	{
		m_unit = unit;
	}
	//! sets/resets the page and the origin
	void setPagePos(int page, Vec2f const &newOrig) const
	{
		const_cast<WPSPosition *>(this)->m_page = page;
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
	void setOrder(int order) const
	{
		m_order = order;
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
	WPXUnit m_unit;
	//! background/foward order
	mutable int m_order;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
