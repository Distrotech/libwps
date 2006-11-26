/* libwps
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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
 * /libwpd2/src/lib/WPXStylesListener.h 1.5
 */

#ifndef WPSSTYLESLISTENER_H
#define WPSSTYLESLISTENER_H

#include "WPSPageSpan.h"
#include "WPSListener.h"
#include <list>

class WPSStylesListener : protected WPSListener
{
protected:
	WPSStylesListener(std::list<WPSPageSpan> &pageList);
	virtual ~WPSStylesListener();
};

#endif /* WPSSTYLESLISTENER_H */
