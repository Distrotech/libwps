/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <iostream>
#include <sstream>

#include <libwpd/libwpd.h>

#include "TableStyle.h"

void TableStyleManager::openTable(WPXPropertyListVector const &colList)
{
	std::vector<double> colWidths;
	for (unsigned long i = 0; i < colList.count(); i++)
	{
		WPXPropertyList const &prop=colList[i];
		double tmp;
		if (prop["style:column-width"] &&
		        wps2html::getPointValue(*prop["style:column-width"], tmp))
			colWidths.push_back(tmp/72.);
		else
			colWidths.push_back(0.);
	}
	m_columWitdhsStack.push_back(colWidths);
}

void TableStyleManager::closeTable()
{
	if (!m_columWitdhsStack.size())
	{
		WPS_DEBUG_MSG(("TableStyleManager::closeTable: can not find the columns witdh\n"));
		return;
	}
	m_columWitdhsStack.pop_back();
}

bool TableStyleManager::getColumnsWidth(int col, int numSpanned, double &w) const
{
	if (!m_columWitdhsStack.size())
		return false;
	std::vector<double> const &widths=m_columWitdhsStack.back();
	if (col < 0 || size_t(col+numSpanned-1) >= widths.size())
	{
		WPS_DEBUG_MSG(("TableStyleManager::getColumnsWidth: can not compute the columns witdh\n"));
		return false;
	}
	bool fixed = true;
	w = 0;
	for (size_t i=size_t(col); i < size_t(col+numSpanned); i++)
	{
		if (widths[i] < 0)
		{
			w += -widths[i];
			fixed = false;
		}
		else if (widths[i] > 0)
			w += widths[i];
		else
		{
			w = 0;
			return true;
		}
	}
	if (!fixed) w = -w;
	return true;
}

std::string TableStyleManager::getCellClass(WPXPropertyList const &pList)
{
	std::string content=getCellContent(pList);
	std::map<std::string, std::string>::iterator it=m_cellContentNameMap.find(content);
	if (it != m_cellContentNameMap.end())
		return it->second;
	std::stringstream s;
	s << "cellTable" << m_cellContentNameMap.size();
	m_cellContentNameMap[content]=s.str();
	return s.str();
}

std::string TableStyleManager::getRowClass(WPXPropertyList const &pList)
{
	std::string content=getRowContent(pList);
	std::map<std::string, std::string>::iterator it=m_rowContentNameMap.find(content);
	if (it != m_rowContentNameMap.end())
		return it->second;
	std::stringstream s;
	s << "rowTable" << m_rowContentNameMap.size();
	m_rowContentNameMap[content]=s.str();
	return s.str();
}

void TableStyleManager::send(std::ostream &out)
{
	std::map<std::string, std::string>::iterator it=m_cellContentNameMap.begin();
	while (it != m_cellContentNameMap.end())
	{
		out << "." << it->second << " " << it->first << "\n";
		++it;
	}
	it=m_rowContentNameMap.begin();
	while (it != m_rowContentNameMap.end())
	{
		out << "." << it->second << " " << it->first << "\n";
		++it;
	}
}

std::string TableStyleManager::getCellContent(WPXPropertyList const &pList) const
{
	std::stringstream s;
	s << "{\n";
	// try to get the cell width
	if (pList["libwpd:column"])
	{
		int c=pList["libwpd:column"]->getInt();
		int span=1;
		if (pList["table:number-columns-spanned"])
			span = pList["table:number-columns-spanned"]->getInt();
		double w;
		if (!getColumnsWidth(c,span,w))
		{
			WPS_DEBUG_MSG(("TableStyleManager::getCellContent: can not find columns witdth for %d[%d]\n", c, span));
		}
		else if (w > 0)
			s << "\twidth:" << w << "in;\n";
		else if (w < 0)
			s << "\tmin-width:" << -w << "in;\n";
	}
	if (pList["fo:text-align"])
	{
		if (pList["fo:text-align"]->getStr() == WPXString("end")) // stupid OOo convention..
			s << "\ttext-align:right;\n";
		else
			s << "\ttext-align:" << pList["fo:text-align"]->getStr().cstr() << ";\n";
	}
	if (pList["style:vertical-align"])
		s << "\tvertical-align:" << pList["style:vertical-align"]->getStr().cstr() << ";\n";
	else
		s << "\tvertical-align:top;\n";
	if (pList["fo:background-color"])
		s << "\tbackground-color:" << pList["fo:background-color"]->getStr().cstr() << ";\n";

	static char const *(type[]) = {"border", "border-left", "border-top", "border-right", "border-bottom" };
	for (int i = 0; i < 5; i++)
	{
		std::string field("fo:");
		field+=type[i];
		if (!pList[field.c_str()])
			continue;
		s << "\t" << type[i] << ": " << pList[field.c_str()]->getStr().cstr() << ";\n";
	}

	s << "}";
	return s.str();
}

std::string TableStyleManager::getRowContent(WPXPropertyList const &pList) const
{
	std::stringstream s;
	s << "{\n";
	if (pList["style:min-row-height"])
		s << "\tmin-height:" << pList["style:min-row-height"]->getStr().cstr() << ";\n";
	else if (pList["style:row-height"])
		s << "\theight:" << pList["style:row-height"]->getStr().cstr() << ";\n";
	s << "}";
	return s.str();
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
