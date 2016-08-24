/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002, 2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002, 2004 Marc Maurer (uwog@uwog.net)
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
#ifndef WPS_GRAPHIC_SHAPE
#  define WPS_GRAPHIC_SHAPE
#  include <ostream>
#  include <string>
#  include <vector>

#  include "librevenge/librevenge.h"

#  include "libwps_internal.h"

/** a structure used to define a picture shape */
class WPSGraphicShape
{
public:
	//! an enum used to define the shape type
	enum Type { Arc, Circle, Line, Rectangle, Path, Pie, Polygon, ShapeUnknown };
	//! an enum used to define the interface command
	enum Command { C_Ellipse, C_Polyline, C_Rectangle, C_Path, C_Polygon, C_Bad };
	//! a simple path component
	struct PathData
	{
		//! constructor
		PathData(char type, Vec2f const &x=Vec2f(), Vec2f const &x1=Vec2f(), Vec2f const &x2=Vec2f()):
			m_type(type), m_x(x), m_x1(x1), m_x2(x2), m_r(), m_rotate(0), m_largeAngle(false), m_sweep(false)
		{
		}
		//! translate all the coordinate by delta
		void translate(Vec2f const &delta);
		//! scale all the coordinate by a factor
		void scale(Vec2f const &factor);
		//! rotate all the coordinate by angle (origin rotation) then translate coordinate
		void rotate(float angle, Vec2f const &delta);
		//! update the property list to correspond to a command
		bool get(librevenge::RVNGPropertyList &pList, Vec2f const &orig) const;
		//! a print operator
		friend std::ostream &operator<<(std::ostream &o, PathData const &path);
		//! comparison function
		int cmp(PathData const &a) const;
		//! the type: M, L, ...
		char m_type;
		//! the main x value
		Vec2f m_x;
		//! x1 value
		Vec2f m_x1;
		//! x2 value
		Vec2f m_x2;
		//! the radius ( A command)
		Vec2f m_r;
		//! the rotate ( A command)
		float m_rotate;
		//! large angle ( A command)
		bool m_largeAngle;
		//! sweep value ( A command)
		bool m_sweep;
	};

	//! constructor
	WPSGraphicShape() : m_type(ShapeUnknown), m_bdBox(), m_formBox(), m_cornerWidth(0,0), m_arcAngles(0,0),
		m_vertices(), m_path(), m_extra("")
	{
	}
	//! destructor
	~WPSGraphicShape() { }
	//! static constructor to create a line
	static WPSGraphicShape line(Vec2f const &orign, Vec2f const &dest);
	//! static constructor to create a rectangle
	static WPSGraphicShape rectangle(Box2f const &box, Vec2f const &corners=Vec2f(0,0))
	{
		WPSGraphicShape res;
		res.m_type=Rectangle;
		res.m_bdBox=res.m_formBox=box;
		res.m_cornerWidth=corners;
		return res;
	}
	//! static constructor to create a circle
	static WPSGraphicShape circle(Box2f const &box)
	{
		WPSGraphicShape res;
		res.m_type=Circle;
		res.m_bdBox=res.m_formBox=box;
		return res;
	}
	//! static constructor to create a arc
	static WPSGraphicShape arc(Box2f const &box, Box2f const &circleBox, Vec2f const &angles)
	{
		WPSGraphicShape res;
		res.m_type=Arc;
		res.m_bdBox=box;
		res.m_formBox=circleBox;
		res.m_arcAngles=angles;
		return res;
	}
	//! static constructor to create a pie
	static WPSGraphicShape pie(Box2f const &box, Box2f const &circleBox, Vec2f const &angles)
	{
		WPSGraphicShape res;
		res.m_type=Pie;
		res.m_bdBox=box;
		res.m_formBox=circleBox;
		res.m_arcAngles=angles;
		return res;
	}
	//! static constructor to create a polygon
	static WPSGraphicShape polygon(Box2f const &box)
	{
		WPSGraphicShape res;
		res.m_type=Polygon;
		res.m_bdBox=box;
		return res;
	}
	//! static constructor to create a path
	static WPSGraphicShape path(Box2f const &box)
	{
		WPSGraphicShape res;
		res.m_type=Path;
		res.m_bdBox=box;
		return res;
	}

	//! translate all the coordinate by delta
	void translate(Vec2f const &delta);
	//! rescale all the coordinate
	void scale(Vec2f const &factor);
	/** return a new shape corresponding to a rotation from center.

	 \note the final bdbox is not tight */
	WPSGraphicShape rotate(float angle, Vec2f const &center) const;
	//! returns the type corresponding to a shape
	Type getType() const
	{
		return m_type;
	}
	//! returns the basic bdbox
	Box2f getBdBox() const
	{
		return m_bdBox;
	}
	//! updates the propList to send to an interface
	Command addTo(Vec2f const &orig, bool asSurface, librevenge::RVNGPropertyList &propList) const;
	//! a print operator
	friend std::ostream &operator<<(std::ostream &o, WPSGraphicShape const &sh);
	/** compare two shapes */
	int cmp(WPSGraphicShape const &a) const;
protected:
	//! return a path corresponding to the shape
	std::vector<PathData> getPath() const;
public:
	//! the type
	Type m_type;
	//! the shape bdbox
	Box2f m_bdBox;
	//! the internal shape bdbox ( used for arc, circle to store the circle bdbox )
	Box2f m_formBox;
	//! the rectangle round corner
	Vec2f m_cornerWidth;
	//! the start and end value which defines an arc
	Vec2f m_arcAngles;
	//! the list of vertices for lines or polygons
	std::vector<Vec2f> m_vertices;
	//! the list of path component
	std::vector<PathData> m_path;
	//! extra data
	std::string m_extra;
};
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
