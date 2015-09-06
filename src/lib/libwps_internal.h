/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
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

#ifndef LIBWPS_INTERNAL_H
#define LIBWPS_INTERNAL_H

#include <assert.h>
#ifdef DEBUG
#include <stdio.h>
#endif

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>
#include <librevenge/librevenge.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(_MSC_VER) || defined(__DJGPP__)
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
#else /* !_MSC_VER && !__DJGPP__*/
#  include <inttypes.h>
#endif /* _MSC_VER || __DJGPP__*/

/* ---------- memory  --------------- */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

// define localtime_r on Windows, so that can use
// thread-safe functions on other environments
#ifdef _WIN32
#  define gmtime_r(tp,tmp) (gmtime(tp)?(*(tmp)=*gmtime(tp),(tmp)):0)
#  define localtime_r(tp,tmp) (localtime(tp)?(*(tmp)=*localtime(tp),(tmp)):0)
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

/** a noop deleter used to transform a librevenge pointer in a false shared_ptr */
template <class T>
struct WPS_shared_ptr_noop_deleter
{
	void operator()(T *) {}
};

// basic classes and autoptr
/** shared pointer to librevenge::RVNGInputStream */
typedef shared_ptr<librevenge::RVNGInputStream> RVNGInputStreamPtr;

class WPSCell;
class WPSListener;
class WPSContentListener;
class WPSEntry;
class WPSFont;
class WPSHeader;
class WPSPosition;
class WPSSubDocument;

class WKSContentListener;
class WKSSubDocument;

/** shared pointer to WPSCell */
typedef shared_ptr<WPSCell> WPSCellPtr;
/** shared pointer to WPSListener */
typedef shared_ptr<WPSListener> WPSListenerPtr;
/** shared pointer to WPSContentListener */
typedef shared_ptr<WPSContentListener> WPSContentListenerPtr;
/** shared pointer to WPSHeader */
typedef shared_ptr<WPSHeader> WPSHeaderPtr;
/** shared pointer to WPSSubDocument */
typedef shared_ptr<WPSSubDocument> WPSSubDocumentPtr;

/** shared pointer to WKSContentListener */
typedef shared_ptr<WKSContentListener> WKSContentListenerPtr;
/** shared pointer to WKSSubDocument */
typedef shared_ptr<WKSSubDocument> WKSSubDocumentPtr;

#if defined(__clang__) || defined(__GNUC__)
#  define WPS_ATTRIBUTE_PRINTF(fmt, arg) __attribute__((__format__(__printf__, fmt, arg)))
#else
#  define WPS_ATTRIBUTE_PRINTF(fmt, arg)
#endif
/* ---------- debug  --------------- */
#ifdef DEBUG
namespace libwps
{
void printDebugMsg(const char *format, ...) WPS_ATTRIBUTE_PRINTF(1, 2);
}
#define WPS_DEBUG_MSG(M) libwps::printDebugMsg M
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
uint8_t readU8(librevenge::RVNGInputStream *input);
uint16_t readU16(librevenge::RVNGInputStream *input);
uint32_t readU32(librevenge::RVNGInputStream *input);

int8_t read8(librevenge::RVNGInputStream *input);
int16_t read16(librevenge::RVNGInputStream *input);
int32_t read32(librevenge::RVNGInputStream *input);

inline uint8_t readU8(RVNGInputStreamPtr &input)
{
	return readU8(input.get());
}
inline uint16_t readU16(RVNGInputStreamPtr &input)
{
	return readU16(input.get());
}
inline uint32_t readU32(RVNGInputStreamPtr &input)
{
	return readU32(input.get());
}

inline int8_t read8(RVNGInputStreamPtr &input)
{
	return read8(input.get());
}
inline int16_t read16(RVNGInputStreamPtr &input)
{
	return read16(input.get());
}
inline int32_t read32(RVNGInputStreamPtr &input)
{
	return read32(input.get());
}

//! read a double store with 4 bytes: mantisse 2.5 bytes, exponent 1.5 bytes
bool readDouble4(RVNGInputStreamPtr &input, double &res, bool &isNaN);
//! read a double store with 8 bytes: mantisse 6.5 bytes, exponent 1.5 bytes
bool readDouble8(RVNGInputStreamPtr &input, double &res, bool &isNaN);
//! read a double store with 10 bytes: mantisse 8 bytes, exponent 2 bytes
bool readDouble10(RVNGInputStreamPtr &input, double &res, bool &isNaN);
//! read a double store with 2 bytes: exponent 1.5 bytes, kind of mantisse 0.5 bytes
bool readDouble2Inv(RVNGInputStreamPtr &input, double &res, bool &isNaN);
//! read a double store with 4 bytes: exponent 3.5 bytes, mantisse 0.5 bytes
bool readDouble4Inv(RVNGInputStreamPtr &input, double &res, bool &isNaN);

//! try to read sz bytes from input and store them in a librevenge::RVNGBinaryData
bool readData(RVNGInputStreamPtr &input, unsigned long sz, librevenge::RVNGBinaryData &data);
//! try to read the last bytes from input and store them in a librevenge::RVNGBinaryData
bool readDataToEnd(RVNGInputStreamPtr &input, librevenge::RVNGBinaryData &data);
//! adds an unicode character to a string ( with correct encoding ).
void appendUnicode(uint32_t val, librevenge::RVNGString &buffer);
}

#define WPS_LE_GET_GUINT16(p)				  			\
        (uint16_t)((((uint8_t const *)(p))[0] << 0)  |	\
                  (((uint8_t const *)(p))[1] << 8))
#define WPS_LE_GET_GUINT32(p)				  			\
        (uint32_t)((((uint8_t const *)(p))[0] << 0) |	\
                  (((uint8_t const *)(p))[1] << 8)  |	\
                  (((uint8_t const *)(p))[2] << 16) |	\
                  (((uint8_t const *)(p))[3] << 24))

#define WPS_LE_PUT_GUINT16(p, v)				  		\
	*((uint8_t*)(p)) = uint8_t(v);				  		\
	*(((uint8_t*)(p)) + 1) = uint8_t((v) >> 8)

#define WPS_LE_PUT_GUINT32(p, v)				  		\
	*((uint8_t*)(p)) = uint8_t(v);				  		\
	*(((uint8_t*)(p)) + 1) = uint8_t((v) >> 8);			\
	*(((uint8_t*)(p)) + 2) = uint8_t((v) >> 16);		\
	*(((uint8_t*)(p)) + 3) = uint8_t((v) >> 24)

// Various helper structures for the parser..
/* ---------- small enum/class ------------- */

struct WPSColumnDefinition
{
	WPSColumnDefinition() : m_width(0), m_leftGutter(0), m_rightGutter(0)
	{
	}
	double m_width;
	double m_leftGutter;
	double m_rightGutter;
};

struct WPSColumnProperties
{
	WPSColumnProperties() : m_attributes(0), m_alignment(0)
	{
	}
	uint32_t m_attributes;
	uint8_t m_alignment;
};

//! the class to store a color
struct WPSColor
{
	//! constructor
	WPSColor(uint32_t argb=0) : m_value(argb)
	{
	}
	//! constructor from color
	WPSColor(unsigned char r, unsigned char g,  unsigned char b, unsigned char a=255) :
		m_value(uint32_t((a<<24)+(r<<16)+(g<<8)+b))
	{
	}
	//! operator=
	WPSColor &operator=(uint32_t argb)
	{
		m_value = argb;
		return *this;
	}
	//! return the back color
	static WPSColor black()
	{
		return WPSColor(0,0,0);
	}
	//! return the white color
	static WPSColor white()
	{
		return WPSColor(255,255,255);
	}

	//! return alpha*colA+beta*colB
	static WPSColor barycenter(float alpha, WPSColor const &colA,
	                           float beta, WPSColor const &colB);
	//! return the rgba value
	uint32_t value() const
	{
		return m_value;
	}
	//! returns the alpha value
	unsigned char getAlpha() const
	{
		return (unsigned char)((m_value>>24)&0xFF);
	}
	//! returns the green value
	unsigned char getBlue() const
	{
		return (unsigned char)(m_value&0xFF);
	}
	//! returns the red value
	unsigned char getRed() const
	{
		return (unsigned char)((m_value>>16)&0xFF);
	}
	//! returns the green value
	unsigned char getGreen() const
	{
		return (unsigned char)((m_value>>8)&0xFF);
	}
	//! return true if the color is black
	bool isBlack() const
	{
		return (m_value&0xFFFFFF)==0;
	}
	//! return true if the color is white
	bool isWhite() const
	{
		return (m_value&0xFFFFFF)==0xFFFFFF;
	}
	//! operator==
	bool operator==(WPSColor const &c) const
	{
		return (c.m_value&0xFFFFFF)==(m_value&0xFFFFFF);
	}
	//! operator!=
	bool operator!=(WPSColor const &c) const
	{
		return !operator==(c);
	}
	//! operator<
	bool operator<(WPSColor const &c) const
	{
		return (c.m_value&0xFFFFFF)<(m_value&0xFFFFFF);
	}
	//! operator<=
	bool operator<=(WPSColor const &c) const
	{
		return (c.m_value&0xFFFFFF)<=(m_value&0xFFFFFF);
	}
	//! operator>
	bool operator>(WPSColor const &c) const
	{
		return !operator<=(c);
	}
	//! operator>=
	bool operator>=(WPSColor const &c) const
	{
		return !operator<(c);
	}
	//! operator<< in the form \#rrggbb
	friend std::ostream &operator<< (std::ostream &o, WPSColor const &c);
	//! print the color in the form \#rrggbb
	std::string str() const;
protected:
	//! the argb color
	uint32_t m_value;
};

//! a border list
struct WPSBorder
{
	/** the line style */
	enum Style { None, Simple, Dot, LargeDot, Dash };
	/** the line repetition */
	enum Type { Single, Double, Triple };
	enum Pos { Left = 0, Right = 1, Top = 2, Bottom = 3 };
	enum { LeftBit = 0x01,  RightBit = 0x02, TopBit=0x4, BottomBit = 0x08 };

	//! constructor
	WPSBorder() : m_style(Simple), m_type(Single), m_width(1), m_widthsList(), m_color(WPSColor::black()), m_extra("") { }
	/** add the border property to proplist (if needed )

		\note if set which must be equal to "left", "top", ... */
	bool addTo(librevenge::RVNGPropertyList &propList, std::string which="") const;
	//! returns true if the border is empty
	bool isEmpty() const
	{
		return m_style==None || m_width <= 0;
	}

	//! operator==
	bool operator==(WPSBorder const &orig) const
	{
		return m_style == orig.m_style && m_type == orig.m_type && m_width == orig.m_width
		       && m_color == orig.m_color;
	}
	//! operator!=
	bool operator!=(WPSBorder const &orig) const
	{
		return !operator==(orig);
	}
	//! compare two cell
	int compare(WPSBorder const &orig) const;

	//! operator<<
	friend std::ostream &operator<< (std::ostream &o, WPSBorder const &border);
	//! operator<<: prints data in form "none|dot|..."
	friend std::ostream &operator<< (std::ostream &o, WPSBorder::Style const &style);
	//! the border style
	Style m_style;
	//! the border repetition
	Type m_type;
	//! the border width
	int m_width;
	/** the different length used for each line/sep (if defined)

		\note when defined, the size of this list must be equal to 2*Type-1*/
	std::vector<double> m_widthsList;
	//! the border color
	WPSColor m_color;
	//! extra data ( if needed)
	std::string m_extra;
};

/** small class use to define a embedded object

    \note mainly used to store picture
 */
struct WPSEmbeddedObject
{
	//! empty constructor
	WPSEmbeddedObject() : m_dataList(), m_typeList()
	{
	}
	//! constructor
	WPSEmbeddedObject(librevenge::RVNGBinaryData const &binaryData,
	                  std::string type="image/pict") : m_dataList(), m_typeList()
	{
		add(binaryData, type);
	}
	//! destructor
	virtual ~WPSEmbeddedObject()
	{
	}
	//! return true if the picture contains no data
	bool isEmpty() const
	{
		for (size_t i=0; i<m_dataList.size(); ++i)
		{
			if (!m_dataList[i].empty())
				return false;
		}
		return true;
	}
	//! add a picture
	void add(librevenge::RVNGBinaryData const &binaryData, std::string type="image/pict")
	{
		size_t pos=m_dataList.size();
		if (pos<m_typeList.size()) pos=m_typeList.size();
		m_dataList.resize(pos+1);
		m_dataList[pos]=binaryData;
		m_typeList.resize(pos+1);
		m_typeList[pos]=type;
	}
	/** add the link property to proplist */
	bool addTo(librevenge::RVNGPropertyList &propList) const;
	/** operator<<*/
	friend std::ostream &operator<<(std::ostream &o, WPSEmbeddedObject const &pict);

	//! the picture content: one data by representation
	std::vector<librevenge::RVNGBinaryData> m_dataList;
	//! the picture type: one type by representation
	std::vector<std::string> m_typeList;
};

namespace libwps
{
enum NumberingType { NONE, BULLET, ARABIC, LOWERCASE, UPPERCASE, LOWERCASE_ROMAN, UPPERCASE_ROMAN };
std::string numberingTypeToString(NumberingType type);
enum SubDocumentType { DOC_NONE, DOC_HEADER_FOOTER, DOC_NOTE, DOC_TABLE, DOC_TEXT_BOX, DOC_COMMENT_ANNOTATION };
enum Justification { JustificationLeft, JustificationFull, JustificationCenter,
                     JustificationRight, JustificationFullAllLines
                   };
enum { NoBreakBit = 0x1, NoBreakWithNextBit=0x2};
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
#define WPS_OVERLINE_BIT 0x400000L
#define WPS_HIDDEN_BIT 0x800000L

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

/* ---------- vec2/box2f ------------- */
/*! \class Vec2
 *   \brief small class which defines a vector with 2 elements
 */
template <class T> class Vec2
{
public:
	//! constructor
	Vec2(T xx=0,T yy=0) : m_x(xx), m_y(yy) { }
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
		if (c<0 || c>1) throw libwps::GenericException();
		return (c==0) ? m_x : m_y;
	}
	//! operator[]
	T &operator[](int c)
	{
		if (c<0 || c>1) throw libwps::GenericException();
		return (c==0) ? m_x : m_y;
	}

	//! resets the two elements
	void set(T xx, T yy)
	{
		m_x = xx;
		m_y = yy;
	}
	//! resets the first element
	void setX(T xx)
	{
		m_x = xx;
	}
	//! resets the second element
	void setY(T yy)
	{
		m_y = yy;
	}

	//! increases the actuals values by \a dx and \a dy
	void add(T dx, T dy)
	{
		m_x += dx;
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
		if (m_x<p.m_x) return -1;
		if (m_x>p.m_x) return 1;
		if (m_y<p.m_y) return -1;
		if (m_y>p.m_y) return 1;
		return 0;
	}
	//! a comparison function: which first compares y then x
	int cmpY(Vec2<T> const &p) const
	{
		if (m_y<p.m_y) return -1;
		if (m_y>p.m_y) return 1;
		if (m_x<p.m_x) return -1;
		if (m_x>p.m_x) return 1;
		return 0;
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
		if (c<0 || c>1) throw libwps::GenericException();
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
		Vec2<T> ctr = 0.5*(m_pt[0]+m_pt[1]);
		m_pt[0] = ctr - 0.5*sz;
		m_pt[1] = ctr + (sz - 0.5*sz);
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
	//! returns the union between this and box
	Box2<T> getUnion(Box2<T> const &box) const
	{
		Box2<T> res;
		res.m_pt[0]=Vec2<T>(m_pt[0][0]<box.m_pt[0][0]?m_pt[0][0] : box.m_pt[0][0],
		                    m_pt[0][1]<box.m_pt[0][1]?m_pt[0][1] : box.m_pt[0][1]);
		res.m_pt[1]=Vec2<T>(m_pt[1][0]>box.m_pt[1][0]?m_pt[1][0] : box.m_pt[1][0],
		                    m_pt[1][1]>box.m_pt[1][1]?m_pt[1][1] : box.m_pt[1][1]);
		return res;
	}
	//! returns the intersection between this and box
	Box2<T> getIntersection(Box2<T> const &box) const
	{
		Box2<T> res;
		res.m_pt[0]=Vec2<T>(m_pt[0][0]>box.m_pt[0][0]?m_pt[0][0] : box.m_pt[0][0],
		                    m_pt[0][1]>box.m_pt[0][1]?m_pt[0][1] : box.m_pt[0][1]);
		res.m_pt[1]=Vec2<T>(m_pt[1][0]<box.m_pt[1][0]?m_pt[1][0] : box.m_pt[1][0],
		                    m_pt[1][1]<box.m_pt[1][1]?m_pt[1][1] : box.m_pt[1][1]);
		return res;
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

#endif /* LIBWPS_INTERNAL_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
