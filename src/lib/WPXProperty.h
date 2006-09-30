/* libwpd
 * Copyright (C) 2004 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2005 Net Integration Technologies (http://www.net-itech.com)
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef WPXPROPERTY_H
#define WPXPROPERTY_H
#include <libwpd/WPXString.h>

enum WPXUnit { INCH, PERCENT, POINT, TWIP };

class WPXProperty
{
public:
	virtual ~WPXProperty();
	virtual int getInt() const = 0;
	virtual float getFloat() const = 0;
	virtual WPXString getStr() const = 0;
	virtual WPXProperty * clone() const = 0;
};

class WPXPropertyFactory
{
public:
	static WPXProperty * newStringProp(const WPXString &str);
	static WPXProperty * newStringProp(const char *str);
	static WPXProperty * newIntProp(const int val);
	static WPXProperty * newBoolProp(const bool val);
	static WPXProperty * newInchProp(const float val);
	static WPXProperty * newPercentProp(const float val);
	static WPXProperty * newPointProp(const float val);
	static WPXProperty * newTwipProp(const float val);
};
#endif /* WPXPROPERTY_H */
