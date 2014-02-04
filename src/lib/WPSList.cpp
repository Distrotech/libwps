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

#include <librevenge/librevenge.h>

#include "WPSList.h"

#include <iostream>
#include <cstring>

void WPSList::Level::addTo(librevenge::RVNGPropertyList &propList, int startVal) const
{
	propList.insert("text:min-label-width", m_labelWidth);
	propList.insert("text:space-before", m_labelIndent);
	switch (m_type)
	{
	case libwps::BULLET:
		if (m_bullet.len())
			propList.insert("text:bullet-char", m_bullet.cstr());
		else
		{
			WPS_DEBUG_MSG(("WPSList::Level::addTo: the bullet char is not defined\n"));
			propList.insert("text:bullet-char", "*");
		}
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

int WPSList::Level::cmpType(WPSList::Level const &levl) const
{
	int diff = int(m_type)-int(levl.m_type);
	if (diff) return diff;
	diff = strcmp(m_prefix.cstr(),levl.m_prefix.cstr());
	if (diff) return diff;
	diff = strcmp(m_suffix.cstr(),levl.m_suffix.cstr());
	if (diff) return diff;
	diff = strcmp(m_bullet.cstr(),levl.m_bullet.cstr());
	if (diff) return diff;
	return 0;
}

int WPSList::Level::cmp(WPSList::Level const &levl) const
{
	int diff = cmpType(levl);
	if (diff) return diff;
	double fDiff = m_labelIndent-levl.m_labelIndent;
	if (fDiff < 0) return -1;
	if (fDiff > 0) return 1;
	fDiff = m_labelWidth-levl.m_labelWidth;
	if (fDiff < 0) return -1;
	if (fDiff > 0) return 1;
	diff = strcmp(m_bullet.cstr(),levl.m_bullet.cstr());
	if (diff) return diff;
	return 0;
}

std::ostream &operator<<(std::ostream &o, WPSList::Level const &ft)
{
	o << "ListLevel[";
	switch (ft.m_type)
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
	case libwps::NONE:
	default:
		o << "####";
	}
	if (ft.m_type != libwps::BULLET && ft.m_startValue >= 0)
		o << ",startVal= " << ft.m_startValue;
	if (ft.m_prefix.len()) o << ", prefix='" << ft.m_prefix.cstr()<<"'";
	if (ft.m_suffix.len()) o << ", suffix='" << ft.m_suffix.cstr()<<"'";
	if (ft.m_labelIndent<0||ft.m_labelIndent>0)
		o << ", indent=" << ft.m_labelIndent;
	if (ft.m_labelWidth<0||ft.m_labelWidth>0)
		o << ", width=" << ft.m_labelWidth;
	o << "]";
	return o;
}

void WPSList::setId(int newId)
{
	if (m_id == newId) return;
	m_previousId = m_id;
	m_id = newId;
	for (size_t i = 0; i < m_levels.size(); i++)
		m_levels[i].resetSendToInterface();
}

bool WPSList::mustSendLevel(int level) const
{
	if (level <= 0 || level > int(m_levels.size()) ||
	        m_levels[size_t(level-1)].isDefault())
	{
		WPS_DEBUG_MSG(("WPSList::mustResentLevel: level %d is not defined\n",level));
		return false;
	}
	if (!m_levels[size_t(level-1)].isSendToInterface()) return true;

	return false;
}

void WPSList::addLevelTo(int level, librevenge::RVNGPropertyList &propList) const
{
	if (level <= 0 || level > int(m_levels.size()) ||
	        m_levels[size_t(level-1)].isDefault())
	{
		WPS_DEBUG_MSG(("WPSList::addLevelTo: level %d is not defined\n",level));
		return;
	}

	if (m_id==-1)
	{
		WPS_DEBUG_MSG(("WPSList::addLevelTo: the list id is not set\n"));
		static int falseId = 1000;
		m_id = falseId++;
	}
	propList.insert("librevenge:list-id", m_id);
	propList.insert("librevenge:level", level);
	m_levels[size_t(level-1)].addTo(propList,m_actualIndices[size_t(level-1)]);
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
		m_levels.resize(size_t(levl));
		m_actualIndices.resize(size_t(levl), 0);
		m_nextIndices.resize(size_t(levl), 1);
	}
	int needReplace = m_levels[size_t(levl-1)].cmp(level) != 0 ||
	                  (level.m_startValue>=0 && m_nextIndices[size_t(levl-1)] !=level.getStartValue());
	if (level.m_startValue >= 0 || level.cmpType(m_levels[size_t(levl-1)]))
	{
		m_nextIndices[size_t(levl-1)]=level.getStartValue();
	}
	if (needReplace) m_levels[size_t(levl-1)] = level;
}

void WPSList::setLevel(int levl) const
{
	if (levl < 1 || levl > int(m_levels.size()))
	{
		WPS_DEBUG_MSG(("WPSList::setLevel: can not set level %d\n",levl));
		return;
	}

	if (levl < int(m_levels.size()))
		m_actualIndices[size_t(levl)]=
		    (m_nextIndices[size_t(levl)]=m_levels[size_t(levl)].getStartValue())-1;

	m_actLevel=levl-1;
}

void WPSList::openElement() const
{
	if (m_actLevel < 0 || m_actLevel >= int(m_levels.size()))
	{
		WPS_DEBUG_MSG(("WPSList::openElement: can not set level %d\n",m_actLevel));
		return;
	}
	if (m_levels[size_t(m_actLevel)].isNumeric())
		m_actualIndices[size_t(m_actLevel)]=m_nextIndices[size_t(m_actLevel)]++;
}

bool WPSList::isNumeric(int levl) const
{
	if (levl < 1 || levl > int(m_levels.size()))
	{
		WPS_DEBUG_MSG(("WPSList::isNumeric: the level does not exist\n"));
		return false;
	}

	return m_levels[size_t(levl-1)].isNumeric();
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
