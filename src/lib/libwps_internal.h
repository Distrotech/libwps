/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
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

#ifndef LIBWPS_INTERNAL_H
#define LIBWPS_INTERNAL_H

#include <assert.h>

#include <iostream>
#include <map>
#include <string>

#include <libwpd-stream/libwpd-stream.h>
#include <libwpd/libwpd.h>

#if defined(_MSC_VER) || defined(__DJGPP__)
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
#else /* !_MSC_VER && !__DJGPP__*/
#include <inttypes.h>
#endif /* _MSC_VER || __DJGPP__*/

/* ---------- memory  --------------- */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(SHAREDPTR_TR1)
#include <tr1/memory>
using std::tr1::shared_ptr;
#elif defined(SHAREDPTR_STD)
#include <memory>
using std::shared_ptr;
#else
#include <boost/shared_ptr.hpp>
using boost::shared_ptr;
#endif

/** an noop deleter used to transform a libwpd pointer in a false shared_ptr */
template <class T>
struct WPS_shared_ptr_noop_deleter
{
	void operator() (T *) {}
};

typedef shared_ptr<WPXInputStream> WPXInputStreamPtr;

/* ---------- vec2/box2f ------------- */
/*! \class Vec2
 *   \brief small class which defines a vector with 2 elements
 */
template <class T> class Vec2
{
public:
	//! constructor
	Vec2(T x=0,T y=0) : m_x(x), m_y(y) { }
	//! generic copy constructor
	template <class U> Vec2(Vec2<U> const &p) : m_x(T(p.x())), m_y(T(p.y())) {}

	//! first element
	T x() const
	{
		return m_x;
	}
	//! second element
	T y() const
	{
		return m_y;
	}
	//! operator[]
	T operator[](int c) const
	{
		assert(c >= 0 && c <= 1);
		return (c==0) ? m_x : m_y;
	}
	//! operator[]
	T &operator[](int c)
	{
		assert(c >= 0 && c <= 1);
		return (c==0) ? m_x : m_y;
	}

	//! resets the two elements
	void set(T x, T y)
	{
		m_x = x;
		m_y = y;
	}
	//! resets the first element
	void setX(T x)
	{
		m_x = x;
	}
	//! resets the second element
	void setY(T y)
	{
		m_y = y;
	}

	//! increases the actuals values by \a dx and \a dy
	void add(T dx, T dy)
	{
		m_x += dx;
		m_y += dy;
	}
	//! adds \a dx
	void addX(T dx)
	{
		m_x += dx;
	}
	//! adds \a dy
	void addY(T dy)
	{
		m_y += dy;
	}

	//! operator+=
	Vec2<T> &operator+=(Vec2<T> const &p)
	{
		m_x += p.m_x;
		m_y += p.m_y;
		return *this;
	}
	//! operator-=
	Vec2<T> &operator-=(Vec2<T> const &p)
	{
		m_x -= p.m_x;
		m_y -= p.m_y;
		return *this;
	}
	//! generic operator*=
	template <class U>
	Vec2<T> &operator*=(U scale)
	{
		m_x = T(m_x*scale);
		m_y = T(m_y*scale);
		return *this;
	}

	//! operator+
	friend Vec2<T> operator+(Vec2<T> const &p1, Vec2<T> const &p2)
	{
		Vec2<T> p(p1);
		return p+=p2;
	}
	//! operator-
	friend Vec2<T> operator-(Vec2<T> const &p1, Vec2<T> const &p2)
	{
		Vec2<T> p(p1);
		return p-=p2;
	}
	//! generic operator*
	template <class U>
	friend Vec2<T> operator*(U scale, Vec2<T> const &p1)
	{
		Vec2<T> p(p1);
		return p *= scale;
	}

	//! comparison==
	bool operator==(Vec2<T> const &p) const
	{
		return cmpY(p) == 0;
	}
	//! comparison!=
	bool operator!=(Vec2<T> const &p) const
	{
		return cmpY(p) != 0;
	}
	//! comparison<: sort by y
	bool operator<(Vec2<T> const &p) const
	{
		return cmpY(p) < 0;
	}
	//! a comparison function: which first compares x then y
	int cmp(Vec2<T> const &p) const
	{
		T diff  = m_x-p.m_x;
		if (diff) return (diff < 0) ? -1 : 1;
		diff = m_y-p.m_y;
		if (diff) return (diff < 0) ? -1 : 1;
		return 0;
	}
	//! a comparison function: which first compares y then x
	int cmpY(Vec2<T> const &p) const
	{
		T diff  = m_y-p.m_y;
		if (diff) return (diff < 0) ? -1 : 1;
		diff = m_x-p.m_x;
		if (diff) return (diff < 0) ? -1 : 1;
		return 0;
	}

	//! Debug: prints data in form "(x,y)"
	std::ostream &debugP(std::ostream &o) const
	{
		o << "(" << m_x << "," << m_y << ")";
		return o;
	}
	//! operator<<: prints data in form "XxY"
	friend std::ostream &operator<< (std::ostream &o, Vec2<T> const &f)
	{
		o << f.m_x << "x" << f.m_y;
		return o;
	}

	/*! \struct PosSizeLtX
	 * \brief internal struct used to create sorted map, sorted by X
	 */
	struct PosSizeLtX
	{
		//! comparaison function
		bool operator()(Vec2<T> const &s1, Vec2<T> const &s2) const
		{
			return s1.cmp(s2) < 0;
		}
	};
	/*! \typedef MapX
	 *  \brief map of Vec2
	 */
	typedef std::map<Vec2<T>, T,struct PosSizeLtX> MapX;

	/*! \struct PosSizeLtY
	 * \brief internal struct used to create sorted map, sorted by Y
	 */
	struct PosSizeLtY
	{
		//! comparaison function
		bool operator()(Vec2<T> const &s1, Vec2<T> const &s2) const
		{
			return s1.cmpY(s2) < 0;
		}
	};
	/*! \typedef MapY
	 *  \brief map of Vec2
	 */
	typedef std::map<Vec2<T>, T,struct PosSizeLtY> MapY;
protected:
	T m_x/*! \brief first element */, m_y/*! \brief second element */;
};

/*! \brief Vec2 of bool */
typedef Vec2<bool> Vec2b;
/*! \brief Vec2 of int */
typedef Vec2<int> Vec2i;
/*! \brief Vec2 of float */
typedef Vec2<float> Vec2f;

/*! \class Box2
 *   \brief small class which defines a 2D Box
 */
template <class T> class Box2
{
public:
	//! constructor
	Box2(Vec2<T> minPt=Vec2<T>(), Vec2<T> maxPt=Vec2<T>())
	{
		m_pt[0] = minPt;
		m_pt[1] = maxPt;
	}
	//! generic constructor
	template <class U> Box2(Box2<U> const &p)
	{
		for (int c=0; c < 2; c++) m_pt[c] = p[c];
	}

	//! the minimum 2D point (in x and in y)
	Vec2<T> const &min() const
	{
		return m_pt[0];
	}
	//! the maximum 2D point (in x and in y)
	Vec2<T> const &max() const
	{
		return m_pt[1];
	}
	//! the minimum 2D point (in x and in y)
	Vec2<T> &min()
	{
		return m_pt[0];
	}
	//! the maximum 2D point (in x and in y)
	Vec2<T> &max()
	{
		return m_pt[1];
	}

	/*! \brief the two extremum points which defined the box
	 * \param c value 0 means the minimum
	 * \param c value 1 means the maximum
	 */
	Vec2<T> const &operator[](int c) const
	{
		assert(c >= 0 && c <= 1);
		return m_pt[c];
	}
	//! the box size
	Vec2<T> size() const
	{
		return m_pt[1]-m_pt[0];
	}
	//! the box center
	Vec2<T> center() const
	{
		return 0.5*(m_pt[0]+m_pt[1]);
	}

	//! resets the data to minimum \a x and maximum \a y
	void set(Vec2<T> const &x, Vec2<T> const &y)
	{
		m_pt[0] = x;
		m_pt[1] = y;
	}
	//! resets the minimum point
	void setMin(Vec2<T> const &x)
	{
		m_pt[0] = x;
	}
	//! resets the maximum point
	void setMax(Vec2<T> const &y)
	{
		m_pt[1] = y;
	}

	//!  resize the box keeping the minimum
	void resizeFromMin(Vec2<T> const &sz)
	{
		m_pt[1] = m_pt[0]+sz;
	}
	//!  resize the box keeping the maximum
	void resizeFromMax(Vec2<T> const &sz)
	{
		m_pt[0] = m_pt[1]-sz;
	}
	//!  resize the box keeping the center
	void resizeFromCenter(Vec2<T> const &sz)
	{
		Vec2<T> center = 0.5*(m_pt[0]+m_pt[1]);
		m_pt[0] = center - 0.5*sz;
		m_pt[1] = center + (sz - 0.5*sz);
	}

	//! scales all points of the box by \a factor
	template <class U> void scale(U factor)
	{
		m_pt[0] *= factor;
		m_pt[1] *= factor;
	}

	//! extends the bdbox by (\a val, \a val) keeping the center
	void extend(T val)
	{
		m_pt[0] -= Vec2<T>(val/2,val/2);
		m_pt[1] += Vec2<T>(val-(val/2),val-(val/2));
	}

	//! comparison operator==
	bool operator==(Box2<T> const &p) const
	{
		return cmp(p) == 0;
	}
	//! comparison operator!=
	bool operator!=(Box2<T> const &p) const
	{
		return cmp(p) != 0;
	}
	//! comparison operator< : fist sorts min by Y,X values then max extremity
	bool operator<(Box2<T> const &p) const
	{
		return cmp(p) < 0;
	}

	//! comparison function : fist sorts min by Y,X values then max extremity
	int cmp(Box2<T> const &p) const
	{
		int diff  = m_pt[0].cmpY(p.m_pt[0]);
		if (diff) return diff;
		diff  = m_pt[1].cmpY(p.m_pt[1]);
		if (diff) return diff;
		return 0;
	}

	//! print data in form X0xY0<->X1xY1
	friend std::ostream &operator<< (std::ostream &o, Box2<T> const &f)
	{
		o << "(" << f.m_pt[0] << "<->" << f.m_pt[1] << ")";
		return o;
	}

	/*! \struct PosSizeLt
	 * \brief internal struct used to create sorted map, sorted first min then max
	 */
	struct PosSizeLt
	{
		//! comparaison function
		bool operator()(Box2<T> const &s1, Box2<T> const &s2) const
		{
			return s1.cmp(s2) < 0;
		}
	};
	/*! \typedef Map
	 *  \brief map of Box2
	 */
	typedef std::map<Box2<T>, T,struct PosSizeLt> Map;

protected:
	//! the two extremities
	Vec2<T> m_pt[2];
};

/*! \brief Box2 of int */
typedef Box2<int> Box2i;
/*! \brief Box2 of float */
typedef Box2<float> Box2f;

/* ---------- debug  --------------- */
#ifdef DEBUG
#define WPS_DEBUG_MSG(M) printf M
#else
#define WPS_DEBUG_MSG(M)
#endif

/* ---------- exception  ------------ */
namespace libwps
{
// Various exceptions
class VersionException
{
	// needless to say, we could flesh this class out a bit
};

class FileException
{
	// needless to say, we could flesh this class out a bit
};

class ParseException
{
	// needless to say, we could flesh this class out a bit
};

class GenericException
{
	// needless to say, we could flesh this class out a bit
};
}

/* ---------- input ----------------- */
namespace libwps
{
uint8_t readU8(WPXInputStream *input);
uint16_t readU16(WPXInputStream *input);
uint32_t readU32(WPXInputStream *input);

int8_t read8(WPXInputStream *input);
int16_t read16(WPXInputStream *input);
int32_t read32(WPXInputStream *input);

inline uint8_t readU8(WPXInputStreamPtr &input)
{
	return readU8(input.get());
}
inline uint16_t readU16(WPXInputStreamPtr &input)
{
	return readU16(input.get());
}
inline uint32_t readU32(WPXInputStreamPtr &input)
{
	return readU32(input.get());
}

inline int8_t read8(WPXInputStreamPtr &input)
{
	return read8(input.get());
}
inline int16_t read16(WPXInputStreamPtr &input)
{
	return read16(input.get());
}
inline int32_t read32(WPXInputStreamPtr &input)
{
	return read32(input.get());
}
}

#define WPS_LE_GET_GUINT16(p)				  \
        (uint16_t)((((uint8_t const *)(p))[0] << 0)  |    \
                  (((uint8_t const *)(p))[1] << 8))
#define WPS_LE_GET_GUINT32(p) \
        (uint32_t)((((uint8_t const *)(p))[0] << 0)  |    \
                  (((uint8_t const *)(p))[1] << 8)  |    \
                  (((uint8_t const *)(p))[2] << 16) |    \
                  (((uint8_t const *)(p))[3] << 24))

// Various helper structures for the parser..
namespace libwps
{
enum NumberingType { NONE, BULLET, ARABIC, LOWERCASE, UPPERCASE, LOWERCASE_ROMAN, UPPERCASE_ROMAN };
}

// ATTRIBUTE bits
#define WPS_EXTRA_LARGE_BIT 1
#define WPS_VERY_LARGE_BIT 2
#define WPS_LARGE_BIT 4
#define WPS_SMALL_PRINT_BIT 8
#define WPS_FINE_PRINT_BIT 0x10
#define WPS_SUPERSCRIPT_BIT 0x20
#define WPS_SUBSCRIPT_BIT 0x40
#define WPS_OUTLINE_BIT 0x80
#define WPS_ITALICS_BIT 0x100
#define WPS_SHADOW_BIT 0x200
#define WPS_REDLINE_BIT 0x400
#define WPS_DOUBLE_UNDERLINE_BIT 0x800
#define WPS_BOLD_BIT 0x1000
#define WPS_STRIKEOUT_BIT 0x2000
#define WPS_UNDERLINE_BIT 0x4000
#define WPS_SMALL_CAPS_BIT 0x8000
#define WPS_BLINK_BIT 0x10000L
#define WPS_REVERSEVIDEO_BIT 0x20000L
#define WPS_ALL_CAPS_BIT 0x40000L
#define WPS_EMBOSS_BIT 0x80000L
#define WPS_ENGRAVE_BIT 0x100000L
#define WPS_SPECIAL_BIT 0x200000L

// JUSTIFICATION bits
#define WPS_PARAGRAPH_JUSTIFICATION_LEFT 0x00
#define WPS_PARAGRAPH_JUSTIFICATION_FULL 0x01
#define WPS_PARAGRAPH_JUSTIFICATION_CENTER 0x02
#define WPS_PARAGRAPH_JUSTIFICATION_RIGHT 0x03
#define WPS_PARAGRAPH_JUSTIFICATION_FULL_ALL_LINES 0x04
#define WPS_PARAGRAPH_JUSTIFICATION_DECIMAL_ALIGNED 0x05

#define WPS_PARAGRAPH_LAYOUT_NO_BREAK  0x01
#define WPS_PARAGRAPH_LAYOUT_WITH_NEXT 0x02

#define WPS_TAB_LEFT 0x00
#define WPS_TAB_CENTER 0x01
#define WPS_TAB_RIGHT 0x02
#define WPS_TAB_DECIMAL 0x03
#define WPS_TAB_BAR 0x04

// BREAK bits
#define WPS_PAGE_BREAK 0x00
#define WPS_SOFT_PAGE_BREAK 0x01
#define WPS_COLUMN_BREAK 0x02

// Generic bits
#define WPS_LEFT 0x00
#define WPS_RIGHT 0x01
#define WPS_CENTER 0x02
#define WPS_TOP 0x03
#define WPS_BOTTOM 0x04

// Field codes
#define WPS_FIELD_PAGE 1
#define WPS_FIELD_DATE 2
#define WPS_FIELD_TIME 3
#define WPS_FIELD_FILE 4

// Bullets and numbering

#define WPS_NUMBERING_NONE   0
#define WPS_NUMBERING_BULLET 1
#define WPS_NUMBERING_NUMBER 2

#endif /* LIBWPS_INTERNAL_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
