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

#ifndef WPS_TABLE
#  define WPS_TABLE

#include <iostream>
#include <vector>

#include "libwps_internal.h"

class WPSContentListener;
typedef shared_ptr<WPSContentListener> WPSContentListenerPtr;

class WPSCell;
typedef shared_ptr<WPSCell> WPSCellPtr;

/*
 * Structure to store and construct a table from an unstructured list
 * of cell
 *
 */
class WPSTable
{
public:
	//! the constructor
	WPSTable() : m_cellsList(), m_rowsSize(), m_colsSize() {}

	//! the destructor
	virtual ~WPSTable();

	//! add a new cells
	void add(WPSCellPtr &cell);

	//! returns the number of cell
	int numCells() const
	{
		return m_cellsList.size();
	}
	//! returns the i^th cell
	WPSCellPtr get(int id);

	/** try to send the table

	Note: either send the table ( and returns true ) or do nothing.
	 */
	bool sendTable(WPSContentListenerPtr listener);

	/** try to send the table as basic text */
	bool sendAsText(WPSContentListenerPtr listener);

protected:
	//! create the correspondance list, ...
	bool buildStructures();

	/** the list of cells */
	std::vector<WPSCellPtr> m_cellsList;
	/** the final row and col size (in point) */
	std::vector<float> m_rowsSize, m_colsSize;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
