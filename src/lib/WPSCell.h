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

/* Define some classes used to store a Cell
 */

#ifndef WPS_CELL_H
#  define WPS_CELL_H

#include <iostream>
#include <vector>

#include "libwps_internal.h"

class WPXPropertyList;
class WPSContentListener;
typedef shared_ptr<WPSContentListener> WPSContentListenerPtr;

/** a structure used to defined the cell format */
class WPSCellFormat
{
public:
	/** the default horizontal alignement.

	\note actually mainly used for table/spreadsheet cell, FULL is not yet implemented */
	enum HorizontalAlignment { HALIGN_LEFT, HALIGN_RIGHT, HALIGN_CENTER,
	                           HALIGN_FULL, HALIGN_DEFAULT
	                         };
	//! constructor
	WPSCellFormat() :
		m_hAlign(HALIGN_DEFAULT), m_bordersList(), m_backgroundColor(0xFFFFFF) { }

	virtual ~WPSCellFormat() {}


	//! returns the horizontal alignement
	HorizontalAlignment hAlignement() const
	{
		return m_hAlign;
	}
	//! sets the horizontal alignement
	void setHAlignement(HorizontalAlignment align)
	{
		m_hAlign = align;
	}

	//! return true if the cell has some border
	bool hasBorders() const
	{
		return m_bordersList.size() != 0;
	}

	//! return the cell border: libwps::LeftBit | ...
	std::vector<WPSBorder> const &borders() const
	{
		return m_bordersList;
	}

	//! reset the border
	void resetBorders()
	{
		m_bordersList.resize(0);
	}

	//! sets the cell border: wh=WPSBorder::LeftBit|...
	void setBorders(int wh, WPSBorder const &border);

	//! returns the background color
	uint32_t backgroundColor() const
	{
		return m_backgroundColor;
	}
	//! set the background color
	void setBackgroundColor(uint32_t color)
	{
		m_backgroundColor = color;
	}

	//! a comparison  function
	int compare(WPSCellFormat const &cell) const;

	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, WPSCellFormat const &cell);

protected:
	//! the cell alignement : by default nothing
	HorizontalAlignment m_hAlign;
	//! the cell border WPSBorder::Pos
	std::vector<WPSBorder> m_bordersList;
	//! the backgroung color
	uint32_t m_backgroundColor;
};

class WPSTable;

/** a structure used to defined the cell position, and a format */
class WPSCell : public WPSCellFormat
{
	friend class WPSTable;
public:
	//! constructor
	WPSCell() : WPSCellFormat(), m_box(), m_position(0,0), m_numberCellSpanned(1,1) {}

	//! set the bounding box (units in point)
	void setBox(Box2f const &b)
	{
		m_box = b;
	}
	//! return the bounding box
	Box2f const &box() const
	{
		return m_box;
	}
	//! position  accessor
	Vec2i &position()
	{
		return m_position;
	}
	//! position  accessor
	Vec2i const &position() const
	{
		return m_position;
	}
	//! set the cell positions :  0,0 -> A1, 0,1 -> A2
	void setPosition(Vec2i posi)
	{
		m_position = posi;
	}

	//! returns the number of spanned cells
	Vec2i const &numSpannedCells() const
	{
		return m_numberCellSpanned;
	}
	//! sets the number of spanned cells : Vec2i(1,1) means 1 cellule
	void setNumSpannedCells(Vec2i numSpanned)
	{
		m_numberCellSpanned=numSpanned;
	}

	//! call when a cell must be send
	virtual bool send(WPSContentListenerPtr &listener) = 0;

	//! call when the content of a cell must be send
	virtual bool sendContent(WPSContentListenerPtr &listener) = 0;

	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, WPSCell const &cell);

protected:
	//! a comparaison structure used retrieve the rows and the columns
	struct Compare
	{
		Compare(int dim) : m_coord(dim) {}
		//! small structure to define a cell point
		struct Point
		{
			Point(int wh, WPSCell const *cell) : m_which(wh), m_cell(cell) {}
			float getPos(int coord) const
			{
				if (m_which)
					return m_cell->box().max()[coord];
				return m_cell->box().min()[coord];
			}
			float getSize(int coord) const
			{
				return m_cell->box().size()[coord];
			}
			int m_which;
			WPSCell const *m_cell;
		};

		//! comparaison function
		bool operator()(Point const &c1, Point const &c2) const
		{
			float diffF = c1.getPos(m_coord)-c2.getPos(m_coord);
			if (diffF < 0) return true;
			if (diffF > 0) return false;
			int diff = c2.m_which - c1.m_which;
			if (diff) return (diff < 0);
			diffF = c1.m_cell->box().size()[m_coord]
			        - c2.m_cell->box().size()[m_coord];
			if (diffF < 0) return true;
			if (diffF > 0) return false;
			return long(c1.m_cell) < long(c2.m_cell);
		}

		//! the coord to compare
		int m_coord;
	};

	/** the cell bounding box (unit in point)*/
	Box2f m_box;
	//! the cell row and column : 0,0 -> A1, 0,1 -> A2
	Vec2i m_position;
	//! the cell spanned : by default (1,1)
	Vec2i m_numberCellSpanned;
};

typedef shared_ptr<WPSCell> WPSCellPtr;

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
