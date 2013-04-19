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
		return int(m_cellsList.size());
	}
	//! returns the i^th cell
	WPSCellPtr getCell(int id);

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
