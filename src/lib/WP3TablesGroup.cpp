/* libwpd
 * Copyright (C) 2005 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
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

#include <math.h>
#include "WP3TablesGroup.h"
#include "WP3FileStructure.h"
#include "WP3Listener.h"
#include "WPXFileStructure.h"
#include "libwpd_internal.h"
#include "libwpd_math.h"

WP3TablesGroup::WP3TablesGroup(WPXInputStream *input) :
	m_tableMode(0),
	m_offsetFromLeftEdge(0),
	m_topGutterSpacing(0),
	m_leftGutterSpacing(0),
	m_bottomGutterSpacing(0),
	m_rightGutterSpacing(0),
	m_numColumns(0)
{
	_read(input);
}

WP3TablesGroup::~WP3TablesGroup()
{
}

void WP3TablesGroup::_readContents(WPXInputStream *input)
{
	// this group can contain different kinds of data, thus we need to read
	// the contents accordingly
	uint8_t i;
	switch (getSubGroup())
	{
	case WP3_TABLES_GROUP_TABLE_FUNCTION:
		input->seek(71, WPX_SEEK_CUR);
		m_tableMode = readU8(input);
		m_offsetFromLeftEdge = readU32(input, true);
		m_topGutterSpacing = readU32(input, true);
		m_leftGutterSpacing = readU32(input, true);
		m_bottomGutterSpacing = readU32(input, true);
		m_rightGutterSpacing = readU32(input, true);
		input->seek(3, WPX_SEEK_CUR);
		m_numColumns = readU8(input);
		for (i=0; i<m_numColumns; i++)
		{
			m_columnMode[i] = readU8(input);
			m_numberFormat[i] = readU8(input);
			m_columnWidth[i] = readU32(input, true);
			m_rightOffsetForDecimalAlign[i] = readU32(input, true);
		}		
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_SPAN:
		m_colSpan = readU16(input, true);
		m_rowSpan = readU16(input, true);
		m_colSpan++; m_rowSpan++;
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_TOP_LINE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_LEFT_LINE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_BOTTOM_LINE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_RIGHT_LINE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_TOP_LINE_COLOR:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_LEFT_LINE_COLOR:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_BOTTOM_LINE_COLOR:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_RIGHT_LINE_COLOR:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_FILL_COLOR_PATTERN:
		{
			uint16_t tmpRed = readU16(input, true);
			uint16_t tmpGreen = readU16(input, true);
			uint16_t tmpBlue = readU16(input, true);
			m_cellFillColor = RGBSColor(tmpRed, tmpGreen, tmpBlue);
		}
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_VERTICAL_ALIGNMENT:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_MODE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_ROW_MODE:
		break;
	default: /* something else we don't support, since it isn't in the docs */
		break;
	}
}

void WP3TablesGroup::parse(WP3Listener *listener)
{
	WPD_DEBUG_MSG(("WordPerfect: handling a Tables group\n"));

	uint8_t i;
	switch (getSubGroup())
	{
	case WP3_TABLES_GROUP_TABLE_FUNCTION:
		listener->defineTable(m_tableMode, fixedPointToWPUs(m_offsetFromLeftEdge));
		for (i=0; i<m_numColumns; i++)
			listener->addTableColumnDefinition(fixedPointToWPUs(m_columnWidth[i]), fixedPointToWPUs(m_leftGutterSpacing),
								fixedPointToWPUs(m_rightGutterSpacing), 0, LEFT);
		listener->startTable();
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_SPAN:
		listener->setTableCellSpan(m_colSpan, m_rowSpan);
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_TOP_LINE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_LEFT_LINE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_BOTTOM_LINE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_RIGHT_LINE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_TOP_LINE_COLOR:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_LEFT_LINE_COLOR:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_BOTTOM_LINE_COLOR:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_RIGHT_LINE_COLOR:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_FILL_COLOR_PATTERN:
		listener->setTableCellFillColor(&m_cellFillColor);
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_VERTICAL_ALIGNMENT:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_CELL_MODE:
		break;
	case WP3_TABLES_GROUP_SET_TABLE_ROW_MODE:
		break;
	default: // something else we don't support, since it isn't in the docs
		break;
	}
}
