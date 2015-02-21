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
#ifndef WPS_GRAPHIC_STYLE
#  define WPS_GRAPHIC_STYLE
#  include <ostream>
#  include <string>
#  include <vector>

#  include "librevenge/librevenge.h"
#  include "libwps_internal.h"

/** a structure used to define a picture style

 \note in order to define the internal surface style, first it looks for
 a gradient, if so it uses it. Then it looks for a pattern. Finally if
 it found nothing, it uses surfaceColor and surfaceOpacity.*/
class WPSGraphicStyle
{
public:
	//! an enum used to define the basic line cap
	enum LineCap { C_Butt, C_Square, C_Round };
	//! an enum used to define the basic line join
	enum LineJoin { J_Miter, J_Round, J_Bevel };
	//! an enum used to define the gradient type
	enum GradientType { G_None, G_Axial, G_Linear, G_Radial, G_Rectangular, G_Square, G_Ellipsoid };

	//! a structure used to define the gradient limit
	struct GradientStop
	{
		//! constructor
		GradientStop(float offset=0.0, WPSColor const &col=WPSColor::black(), float opacity=1.0) :
			m_offset(offset), m_color(col), m_opacity(opacity)
		{
		}
		/** compare two gradient */
		int cmp(GradientStop const &a) const
		{
			if (m_offset < a.m_offset) return -1;
			if (m_offset > a.m_offset) return 1;
			if (m_color < a.m_color) return -1;
			if (m_color > a.m_color) return 1;
			if (m_opacity < a.m_opacity) return -1;
			if (m_opacity > a.m_opacity) return 1;
			return 0;
		}
		//! a print operator
		friend std::ostream &operator<<(std::ostream &o, GradientStop const &st)
		{
			o << "offset=" << st.m_offset << ",";
			o << "color=" << st.m_color << ",";
			if (st.m_opacity<1.0)
				o << "opacity=" << st.m_opacity*100.f << "%,";
			return o;
		}
		//! the offset
		float m_offset;
		//! the color
		WPSColor m_color;
		//! the opacity
		float m_opacity;
	};
	/** a basic pattern used in a WPSGraphicStyle:
	    - either given a list of 8x8, 16x16, 32x32 bytes with two colors
	    - or with a picture ( and an average color)
	 */
	struct Pattern
	{
		//! constructor
		Pattern() : m_dim(0,0), m_data(), m_picture(), m_pictureMime(""), m_pictureAverageColor(WPSColor::white())
		{
			m_colors[0]=WPSColor::black();
			m_colors[1]=WPSColor::white();
		}
		//!  constructor from a binary data
		Pattern(Vec2i dim, librevenge::RVNGBinaryData const &picture, std::string const &mime, WPSColor const &avColor) :
			m_dim(dim), m_data(), m_picture(picture), m_pictureMime(mime), m_pictureAverageColor(avColor)
		{
			m_colors[0]=WPSColor::black();
			m_colors[1]=WPSColor::white();
		}
		//! virtual destructor
		virtual ~Pattern() {}
		//! return true if we does not have a pattern
		bool empty() const
		{
			if (m_dim[0]==0 || m_dim[1]==0) return true;
			if (m_picture.size()) return false;
			if (m_dim[0]!=8 && m_dim[0]!=16 && m_dim[0]!=32) return true;
			return m_data.size()!=size_t((m_dim[0]/8)*m_dim[1]);
		}
		//! return the average color
		bool getAverageColor(WPSColor &col) const;
		//! check if the pattern has only one color; if so returns true...
		bool getUniqueColor(WPSColor &col) const;
		/** tries to convert the picture in a binary data ( ppm) */
		bool getBinary(librevenge::RVNGBinaryData &data, std::string &type) const;

		/** compare two patterns */
		int cmp(Pattern const &a) const
		{
			int diff = m_dim.cmp(a.m_dim);
			if (diff) return diff;
			if (m_data.size() < a.m_data.size()) return -1;
			if (m_data.size() > a.m_data.size()) return 1;
			for (size_t h=0; h < m_data.size(); ++h)
			{
				if (m_data[h]<a.m_data[h]) return 1;
				if (m_data[h]>a.m_data[h]) return -1;
			}
			for (int i=0; i<2; ++i)
			{
				if (m_colors[i] < a.m_colors[i]) return 1;
				if (m_colors[i] > a.m_colors[i]) return -1;
			}
			if (m_pictureAverageColor < a.m_pictureAverageColor) return 1;
			if (m_pictureAverageColor > a.m_pictureAverageColor) return -1;
			if (m_pictureMime < a.m_pictureMime) return 1;
			if (m_pictureMime > a.m_pictureMime) return -1;
			if (m_picture.size() < a.m_picture.size()) return 1;
			if (m_picture.size() > a.m_picture.size()) return -1;
			const unsigned char *ptr=m_picture.getDataBuffer();
			const unsigned char *aPtr=a.m_picture.getDataBuffer();
			if (!ptr || !aPtr) return 0; // must only appear if the two buffers are empty
			for (unsigned long h=0; h < m_picture.size(); ++h, ++ptr, ++aPtr)
			{
				if (*ptr < *aPtr) return 1;
				if (*ptr > *aPtr) return -1;
			}
			return 0;
		}
		//! a print operator
		friend std::ostream &operator<<(std::ostream &o, Pattern const &pat)
		{
			o << "dim=" << pat.m_dim << ",";
			if (pat.m_picture.size())
			{
				o << "type=" << pat.m_pictureMime << ",";
				o << "col[average]=" << pat.m_pictureAverageColor << ",";
			}
			else
			{
				if (!pat.m_colors[0].isBlack()) o << "col0=" << pat.m_colors[0] << ",";
				if (!pat.m_colors[1].isWhite()) o << "col1=" << pat.m_colors[1] << ",";
				o << "[";
				for (size_t h=0; h < pat.m_data.size(); ++h)
					o << std::hex << (int) pat.m_data[h] << std::dec << ",";
				o << "],";
			}
			return o;
		}
		//! the dimension width x height
		Vec2i m_dim;

		//! the two indexed colors
		WPSColor m_colors[2];
		//! the pattern data: a sequence of data: p[0..7,0],p[8..15,0]...p[0..7,1],p[8..15,1], ...
		std::vector<unsigned char> m_data;
	protected:
		//! a picture
		librevenge::RVNGBinaryData m_picture;
		//! the picture type
		std::string m_pictureMime;
		//! the picture average color
		WPSColor m_pictureAverageColor;
	};
	//! constructor
	WPSGraphicStyle() :  m_lineWidth(1), m_lineDashWidth(), m_lineCap(C_Butt), m_lineJoin(J_Miter), m_lineOpacity(1), m_lineColor(WPSColor::black()),
		m_fillRuleEvenOdd(false), m_surfaceColor(WPSColor::white()), m_surfaceOpacity(0),
		m_shadowColor(WPSColor::black()), m_shadowOpacity(0), m_shadowOffset(1,1),
		m_pattern(),
		m_gradientType(G_None), m_gradientStopList(), m_gradientAngle(0), m_gradientBorder(0), m_gradientPercentCenter(0.5f,0.5f), m_gradientRadius(1),
		m_backgroundColor(WPSColor::white()), m_backgroundOpacity(-1), m_bordersList(), m_frameName(""), m_frameNextName(""),
		m_rotate(0), m_extra("")
	{
		m_arrows[0]=m_arrows[1]=false;
		m_flip[0]=m_flip[1]=false;
		m_gradientStopList.push_back(GradientStop(0.0, WPSColor::white()));
		m_gradientStopList.push_back(GradientStop(1.0, WPSColor::black()));
	}
	/** returns an empty style. Can be used to initialize a default frame style...*/
	static WPSGraphicStyle emptyStyle()
	{
		WPSGraphicStyle res;
		res.m_lineWidth=0;
		return res;
	}
	//! virtual destructor
	virtual ~WPSGraphicStyle() { }
	//! returns true if the border is defined
	bool hasLine() const
	{
		return m_lineWidth>0 && m_lineOpacity>0;
	}
	//! set the surface color
	void setSurfaceColor(WPSColor const &col, float opacity = 1)
	{
		m_surfaceColor = col;
		m_surfaceOpacity = opacity;
	}
	//! returns true if the surface is defined
	bool hasSurfaceColor() const
	{
		return m_surfaceOpacity > 0;
	}
	//! set the pattern
	void setPattern(Pattern const &pat)
	{
		m_pattern=pat;
	}
	//! returns true if the pattern is defined
	bool hasPattern() const
	{
		return !m_pattern.empty();
	}
	//! returns true if the gradient is defined
	bool hasGradient(bool complex=false) const
	{
		return m_gradientType != G_None && (int) m_gradientStopList.size() >= (complex ? 3 : 2);
	}
	//! returns true if the interior surface is defined
	bool hasSurface() const
	{
		return hasSurfaceColor() || hasPattern() || hasGradient();
	}
	//! set the background color
	void setBackgroundColor(WPSColor const &col, float opacity = 1)
	{
		m_backgroundColor = col;
		m_backgroundOpacity = opacity;
	}
	//! returns true if the background is defined
	bool hasBackgroundColor() const
	{
		return m_backgroundOpacity > 0;
	}
	//! set the shadow color
	void setShadowColor(WPSColor const &col, float opacity = 1)
	{
		m_shadowColor = col;
		m_shadowOpacity = opacity;
	}
	//! returns true if the shadow is defined
	bool hasShadow() const
	{
		return m_shadowOpacity > 0;
	}
	//! return true if the frame has some border
	bool hasBorders() const
	{
		return !m_bordersList.empty();
	}
	//! return true if the frame has some border
	bool hasSameBorders() const
	{
		if (m_bordersList.empty()) return true;
		if (m_bordersList.size()!=4) return false;
		for (size_t i=1; i<m_bordersList.size(); ++i)
		{
			if (m_bordersList[i]!=m_bordersList[0])
				return false;
		}
		return true;
	}
	//! return the frame border: libwps::Left | ...
	std::vector<WPSBorder> const &borders() const
	{
		return m_bordersList;
	}
	//! reset the border
	void resetBorders()
	{
		m_bordersList.resize(0);
	}
	//! sets the cell border: wh=libwps::LeftBit|...
	void setBorders(int wh, WPSBorder const &border);
	//! a print operator
	friend std::ostream &operator<<(std::ostream &o, WPSGraphicStyle const &st);
	//! add all the parameters to the propList excepted the frame parameter: the background and the borders
	void addTo(librevenge::RVNGPropertyList &pList, bool only1d=false) const;
	//! add all the frame parameters to propList: the background and the borders
	void addFrameTo(librevenge::RVNGPropertyList &pList) const;
	/** compare two styles */
	int cmp(WPSGraphicStyle const &a) const;

	//! the linewidth
	float m_lineWidth;
	//! the dash array: a sequence of (fullsize, emptysize)
	std::vector<float> m_lineDashWidth;
	//! the line cap
	LineCap m_lineCap;
	//! the line join
	LineJoin m_lineJoin;
	//! the line opacity: 0=transparent
	float m_lineOpacity;
	//! the line color
	WPSColor m_lineColor;
	//! true if the fill rule is evenod
	bool m_fillRuleEvenOdd;
	//! the surface color
	WPSColor m_surfaceColor;
	//! true if the surface has some color
	float m_surfaceOpacity;

	//! the shadow color
	WPSColor m_shadowColor;
	//! true if the shadow has some color
	float m_shadowOpacity;
	//! the shadow offset
	Vec2f m_shadowOffset;

	//! the pattern if it exists
	Pattern m_pattern;

	//! the gradient type
	GradientType m_gradientType;
	//! the list of gradient limits
	std::vector<GradientStop> m_gradientStopList;
	//! the gradient angle
	float m_gradientAngle;
	//! the gradient border opacity
	float m_gradientBorder;
	//! the gradient center
	Vec2f m_gradientPercentCenter;
	//! the gradient radius
	float m_gradientRadius;

	//! two bool to indicated if extremity has arrow or not
	bool m_arrows[2];

	//
	// related to the frame
	//

	//! the background color
	WPSColor m_backgroundColor;
	//! true if the background has some color
	float m_backgroundOpacity;
	//! the borders WPSBorder::Pos (for a frame)
	std::vector<WPSBorder> m_bordersList;
	//! the frame name
	std::string m_frameName;
	//! the frame next name (if there is a link)
	std::string m_frameNextName;

	//
	// some transformation: must probably be somewhere else
	//

	//! the rotation
	float m_rotate;
	//! two bool to indicated we need to flip the shape or not
	bool m_flip[2];

	//! extra data
	std::string m_extra;
};
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
