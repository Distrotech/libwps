/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2005-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

/*
 * This file is in sync with CVS
 * /libwpd2/src/lib/WPXContentListener.h 1.16
 */

#ifndef WPSLISTENER_H
#define WPSLISTENER_H

#include "WPSPageSpan.h"
#include <list>


class WPSListener
{
protected:
	WPSListener(std::list<WPSPageSpan> &pageList);
	virtual ~WPSListener();

	bool isUndoOn() { return m_isUndoOn; }
	void setUndoOn(bool isUndoOn) { m_isUndoOn = isUndoOn; }

	std::list<WPSPageSpan> &m_pageList;
	
private:
	bool m_isUndoOn;
};

#endif /* WPSLISTENER_H */
