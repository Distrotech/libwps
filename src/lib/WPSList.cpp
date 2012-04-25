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

#include <libwpd/WPXDocumentInterface.h>
#include <libwpd/WPXPropertyList.h>

#include "WPSList.h"

#include <iostream>

void WPSList::Level::addTo(WPXPropertyList &propList, int startVal) const
{
#ifdef NEW_VERSION
	propList.insert("text:min-label-width", m_labelWidth);
	propList.insert("text:space-before", m_labelIndent);
#endif
	switch(m_type)
	{
	case libwps::BULLET:
		propList.insert("text:bullet-char", m_bullet.cstr());
		break;
	case libwps::ARABIC:
	case libwps::LOWERCASE:
	case libwps::UPPERCASE:
	case libwps::LOWERCASE_ROMAN:
	case libwps::UPPERCASE_ROMAN:
		if (m_prefix.len()) propList.insert("style:num-prefix",m_prefix);
		if (m_suffix.len()) propList.insert("style:num-suffix", m_suffix);
		propList.insert("style:num-format", libwps::numberingTypeToString(m_type).c_str());
		propList.insert("text:start-value", startVal);
		break;
	case libwps::NONE:
		break;
	default:
		WPS_DEBUG_MSG(("WPSList::Level::addTo: the level type is not set\n"));
	}

	m_sendToInterface = true;
}

int WPSList::Level::cmp(WPSList::Level::Level const &levl) const
{
	int diff = int(m_type)-int(levl.m_type);
	if (diff) return diff;
	double fDiff = m_labelIndent-levl.m_labelIndent;
	if (fDiff) return fDiff < 0.0 ? -1 : 1;
	fDiff = m_labelWidth-levl.m_labelWidth;
	if (fDiff) return fDiff < 0.0 ? -1 : 1;
	diff = strcmp(m_prefix.cstr(),levl.m_prefix.cstr());
	if (diff) return diff;
	diff = strcmp(m_suffix.cstr(),levl.m_suffix.cstr());
	if (diff) return diff;
	diff = strcmp(m_bullet.cstr(),levl.m_bullet.cstr());
	if (diff) return diff;
	return 0;
}

std::ostream &operator<<(std::ostream &o, WPSList::Level::Level const &ft)
{
	o << "ListLevel[";
	switch(ft.m_type)
	{
	case libwps::BULLET:
		o << "bullet='" << ft.m_bullet.cstr() <<"'";
		break;
	case libwps::ARABIC:
		o << "decimal";
		break;
	case libwps::LOWERCASE:
		o << "alpha";
		break;
	case libwps::UPPERCASE:
		o << "ALPHA";
		break;
	case libwps::LOWERCASE_ROMAN:
		o << "roman";
		break;
	case libwps::UPPERCASE_ROMAN:
		o << "ROMAN";
		break;
	default:
		o << "####";
	}
	if (ft.m_type != libwps::BULLET && ft.m_startValue)
		o << ",startVal= " << ft.m_startValue;
	if (ft.m_prefix.len()) o << ", prefix='" << ft.m_prefix.cstr()<<"'";
	if (ft.m_suffix.len()) o << ", suffix='" << ft.m_suffix.cstr()<<"'";
	if (ft.m_labelIndent) o << ", indent=" << ft.m_labelIndent;
	if (ft.m_labelWidth) o << ", width=" << ft.m_labelWidth;
	o << "]";
	return o;
}

void WPSList::setId(int newId)
{
	if (m_id == newId) return;
	m_previousId = m_id;
	m_id = newId;
	for (int i = 0; i < int(m_levels.size()); i++)
		m_levels[i].resetSendToInterface();
}

bool WPSList::mustSendLevel(int level) const
{
	if (level <= 0 || level > int(m_levels.size()) ||
	        m_levels[level-1].isDefault())
	{
		WPS_DEBUG_MSG(("WPSList::mustResentLevel: level %d is not defined\n",level));
		return false;
	}
	if (!m_levels[level-1].isSendToInterface()) return true;

	return false;
}


void WPSList::sendTo(WPXDocumentInterface &docInterface, int level) const
{
	if (level <= 0 || level > int(m_levels.size()) ||
	        m_levels[level-1].isDefault())
	{
		WPS_DEBUG_MSG(("WPSList::sendTo: level %d is not defined\n",level));
		return;
	}

	if (m_id==-1)
	{
		WPS_DEBUG_MSG(("WPSList::sendTo: the list id is not set\n"));
		static int falseId = 1000;
		m_id = falseId++;
	}

	if (m_levels[level-1].isSendToInterface()) return;

	WPXPropertyList propList;
	propList.insert("libwpd:id", m_id);
	propList.insert("libwpd:level", level);
	m_levels[level-1].addTo(propList,m_actualIndices[level-1]);
	if (!m_levels[level-1].isNumeric())
		docInterface.defineUnorderedListLevel(propList);
	else
		docInterface.defineOrderedListLevel(propList);
}

void WPSList::set(int levl, Level const &level)
{
	if (levl < 1)
	{
		WPS_DEBUG_MSG(("WPSList::set: can not set level %d\n",levl));
		return;
	}
	if (levl > int(m_levels.size()))
	{
		m_levels.resize(levl);
		m_actualIndices.resize(levl, 0);
		m_nextIndices.resize(levl, 1);
	}
	int needReplace = m_levels[levl-1].cmp(level) != 0 ||
	                  (level.m_startValue && m_nextIndices[levl-1] !=level.getStartValue());
	if (level.m_startValue > 0 || level.m_type != m_levels[levl-1].m_type)
	{
		m_nextIndices[levl-1]=level.getStartValue();
	}
	if (needReplace) m_levels[levl-1] = level;
}

void WPSList::setLevel(int levl) const
{
	if (levl < 1 || levl > int(m_levels.size()))
	{
		WPS_DEBUG_MSG(("WPSList::setLevel: can not set level %d\n",levl));
		return;
	}

	if (levl < int(m_levels.size()))
		m_actualIndices[levl]=
		    (m_nextIndices[levl]=m_levels[levl].getStartValue())-1;

	m_actLevel=levl-1;
}

void WPSList::openElement() const
{
	if (m_actLevel < 0 || m_actLevel >= int(m_levels.size()))
	{
		WPS_DEBUG_MSG(("WPSList::openElement: can not set level %d\n",m_actLevel));
		return;
	}
	if (m_levels[m_actLevel].isNumeric())
		m_actualIndices[m_actLevel]=m_nextIndices[m_actLevel]++;
}

bool WPSList::isNumeric(int levl) const
{
	if (levl < 1 || levl > int(m_levels.size()))
	{
		WPS_DEBUG_MSG(("WPSList::isNumeric: the level does not exist\n"));
		return false;
	}

	return m_levels[levl-1].isNumeric();
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
