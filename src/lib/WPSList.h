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

#ifndef WPS_LIST_H
#  define WPS_LIST_H

#include <iostream>
#include <vector>

#include <libwpd/libwpd.h>
#include "libwps_internal.h"

class WPXPropertyList;
class WPXDocumentInterface;

/** a small structure used to store the informations about a list */
class WPSList
{
public:
	/** small structure to keep information about a level */
	struct Level
	{

		/** basic constructor */
		Level() : m_labelIndent(0.0), m_labelWidth(0.0), m_startValue(-1), m_type(libwps::NONE),
			m_prefix(""), m_suffix(""), m_bullet(""), m_sendToInterface(false) { }

		/** returns true if the level type was not set */
		bool isDefault() const
		{
			return m_type ==libwps::NONE;
		}
		/** returns true if the list is decimal, alpha or roman */
		bool isNumeric() const
		{
			return m_type !=libwps::NONE && m_type != libwps::BULLET;
		}
		/** add the information of this level in the propList */
		void addTo(WPXPropertyList &propList, int startVal) const;

		/** returns true, if addTo has been called */
		bool isSendToInterface() const
		{
			return m_sendToInterface;
		}
		/** reset the sendToInterface flag */
		void resetSendToInterface() const
		{
			m_sendToInterface = false;
		}

		/** returns the start value (if set) or 1 */
		int getStartValue() const
		{
			return m_startValue <= -1 ? 1 : m_startValue;
		}

		//! full comparison function
		int cmp(Level const &levl) const;

		//! type comparison function
		int cmpType(Level const &levl) const;

		//! operator<<
		friend std::ostream &operator<<(std::ostream &o, Level const &ft);

		double m_labelIndent /** the list indent*/;
		double m_labelWidth /** the list width */;
		/** the actual value (if this is an ordered level ) */
		int m_startValue;
		/** the type of the level */
		libwps::NumberingType m_type;
		WPXString m_prefix /** string which preceedes the number if we have an ordered level*/,
		          m_suffix/** string which follows the number if we have an ordered level*/,
		          m_bullet /** the bullet if we have an bullet level */;

	protected:
		/** true if it is already send to WPXDocumentInterface */
		mutable bool m_sendToInterface;
	};

	/** default constructor */
	WPSList() : m_levels(), m_actLevel(-1), m_actualIndices(), m_nextIndices(),
		m_id(-1), m_previousId (-1) {}

	/** returns the list id */
	int getId() const
	{
		return m_id;
	}

	/** returns the previous list id

	\note a cheat because writerperfect imposes to get a new id if the level 1 changes
	*/
	int getPreviousId() const
	{
		return m_previousId;
	}

	/** set the list id */
	void setId(int newId);

	/** returns the number of level */
	int numLevels() const
	{
		return int(m_levels.size());
	}
	/** sets a level */
	void set(int levl, Level const &level);

	/** set the list level */
	void setLevel(int levl) const;
	/** open the list element */
	void openElement() const;
	/** close the list element */
	void closeElement() const {}

	/** returns true is a level is numeric */
	bool isNumeric(int levl) const;

	/** returns true of the level must be send to the document interface */
	bool mustSendLevel(int level) const;

	/** send the list information to the document interface */
	void sendTo(WPXDocumentInterface &docInterface, int level) const;

protected:
	std::vector<Level> m_levels;

	mutable int m_actLevel;
	mutable std::vector<int> m_actualIndices, m_nextIndices;
	mutable int m_id, m_previousId;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
