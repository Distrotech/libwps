/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 */

#include <stdlib.h>
#include <string.h>

#include <cmath>
#include <sstream>
#include <limits>
#include <stack>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

#include "WKSContentListener.h"
#include "WPSCell.h"
#include "WPSEntry.h"
#include "WPSFont.h"
#include "WPSGraphicShape.h"
#include "WPSGraphicStyle.h"
#include "WPSStream.h"

#include "Lotus.h"

#include "LotusStyleManager.h"

namespace LotusStyleManagerInternal
{
//! small struct used to defined a font name
struct FontName
{
	//! constructor
	FontName() : m_name(), m_id(-2)
	{
		for (int i=0; i<2; ++i) m_size[i]=0;
	}
	//! the font name
	std::string m_name;
	//! the font id
	int m_id;
	//! the font height, font size
	int m_size[2];
};
//! small struct used to defined color style
struct ColorStyle
{
	//! constructor
	ColorStyle() : m_patternId(0), m_pattern(), m_extra("")
	{
		m_colors[0]=m_colors[1]=m_colors[3]=WPSColor::white();
		m_colors[2]=WPSColor::black();
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, ColorStyle const &color)
	{
		for (int i=0; i<4; ++i)
		{
			if ((i==2 && color.m_colors[i].isBlack()) || (i!=2 && color.m_colors[i].isWhite()))
				continue;
			static char const *(wh[])= {"unkn0", "unkn1", "line", "surf" };
			o << "color[" << wh[i] << "]=" << color.m_colors[i] << ",";
		}
		if (color.m_patternId) // 0: none, 2:full
			o << "id[pattern]=" << color.m_patternId;
		if (!color.m_pattern.empty())
			o << "[" << color.m_pattern << "%],";
		o << color.m_extra;
		return o;
	}
	//! the color id : unknown0, unknown1, line, surface
	WPSColor m_colors[4];
	//! the pattern id
	int m_patternId;
	//! the pattern
	WPSGraphicStyle::Pattern m_pattern;
	//! extra data
	std::string m_extra;
};

//! small struct used to defined font style
struct FontStyle
{
	//! constructor
	explicit FontStyle(libwps_tools_win::Font::Type fontType) : m_font(), m_fontType(fontType), m_fontId(0), m_extra("")
	{
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, FontStyle const &font)
	{
		o << font.m_font;
		if (font.m_fontId) o << "FN" << font.m_fontId << ",";
		o << font.m_extra;
		return o;
	}
	//! the font
	WPSFont m_font;
	//! the font type
	libwps_tools_win::Font::Type m_fontType;
	//! the font id
	int m_fontId;
	//! extra data
	std::string m_extra;
};

//! small struct used to defined format style
struct FormatStyle
{
	//! constructor
	FormatStyle() : m_prefix(""), m_suffix(""), m_extra("")
	{
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, FormatStyle const &format)
	{
		if (!format.m_prefix.empty())
			o << "prefix=" << format.m_prefix << ",";
		if (!format.m_suffix.empty())
			o << "suffix=" << format.m_suffix << ",";
		o << format.m_extra;
		return o;
	}
	//! the prefix
	std::string m_prefix;
	//! the suffix
	std::string m_suffix;
	//! extra data
	std::string m_extra;
};

//! small struct used to defined line style
struct LineStyle
{
	//! constructor
	LineStyle() : m_width(1), m_color(WPSColor::black()), m_dashId(0), m_extra("")
	{
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, LineStyle const &line)
	{
		if (line.m_width<1 || line.m_width>1)
			o << "w=" << line.m_width << ",";
		if (!line.m_color.isBlack())
			o << "color=" << line.m_color << ",";
		if (line.m_dashId)
			o << "dashId=" << line.m_dashId << ",";
		o << line.m_extra;
		return o;
	}
	//! the line width
	float m_width;
	//! the line color
	WPSColor m_color;
	//! the dash id
	int m_dashId;
	//! extra data
	std::string m_extra;
};

//! small struct used to defined cell style
struct CellStyle
{
	//! constructor
	explicit CellStyle(libwps_tools_win::Font::Type fontType) :
		m_borders(0), m_fontId(0), m_formatId(0),
		m_colorStyle(), m_fontStyle(fontType), m_extra("")
	{
		for (int i=0; i<4; ++i)
		{
			m_bordersId[i]=0;
			m_bordersStyle[i].m_style=WPSBorder::None;
		}
		for (int i=0; i<2; ++i) m_colorsId[i]=0;
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, CellStyle const &cell)
	{
		if (cell.m_fontId)
			o << "id[font]=FS" << cell.m_fontId << ",";
		for (int i=0; i<4; ++i)
		{
			if (cell.m_bordersId[i])
				o << "bord" << i << "=Li" << cell.m_bordersId[i] << ",";
		}
		for (int i=0; i<2; ++i)
		{
			if (cell.m_colorsId[i])
				o << (i==0 ? "color" : "color[shadow]") << "=Co" << cell.m_colorsId[i] << ",";
		}
		if (cell.m_borders)
		{
			o << "bord=";
			for (int i=0,depl=1; i<4; ++i, depl*=2)
			{
				static char const *(wh[])= {"T","L","B","R"};
				if (cell.m_borders&depl)
					o << wh[i];
			}
			o << ",";
		}
		if (cell.m_formatId)
			o << "id[format]=Fo" << cell.m_formatId << ",";
		o << cell.m_extra;
		return o;
	}
	//! the borders
	int m_borders;
	//! the border line id
	int m_bordersId[4];
	//! the color id : surface, shadow ?
	int m_colorsId[2];
	//! the font id
	int m_fontId;
	//! the format id
	int m_formatId;
	// wk5
	//! the color style
	ColorStyle m_colorStyle;
	//! the font style
	FontStyle m_fontStyle;
	//! the cell border
	WPSBorder m_bordersStyle[4];
	//! extra data
	std::string m_extra;
};

//! small struct used to defined graphic style
struct GraphicStyle
{
	//! constructor
	GraphicStyle() : m_lineId(0), m_extra("")
	{
		for (int i=0; i<2; ++i) m_colorsId[i]=0;
	}
	//! operator<<
	friend std::ostream &operator<<(std::ostream &o, GraphicStyle const &graphic)
	{
		if (graphic.m_lineId)
			o << "L" << graphic.m_lineId << ",";
		if (graphic.m_colorsId[0])
			o << "Co" << graphic.m_colorsId[0] << ",";
		if (graphic.m_colorsId[1])
			o << "shadow[color]=Co" << graphic.m_colorsId[1] << ",";
		o << graphic.m_extra;
		return o;
	}
	//! the surface and shadow color id
	int m_colorsId[2];
	//! the border line id
	int m_lineId;
	//! extra data
	std::string m_extra;
};

//! the state of LotusStyleManager
struct State
{
	//! constructor
	State() :  m_version(-1), m_isUpdated(false), m_idCellStyleMap(), m_idColorStyleMap(),
		m_idFontStyleMap(), m_idFormatStyleMap(), m_idGraphicStyleMap(), m_idLineStyleMap(),
		m_idFontNameMap()

	{
	}
	//! returns a color corresponding to an id between 0 and 7
	bool getColor8(int id, WPSColor &color) const;
	//! returns a color corresponding to an id between 0 and 15
	bool getColor16(int id, WPSColor &color) const;
	//! returns a color corresponding to an id
	bool getColor256(int id, WPSColor &color) const;
	//! returns the pattern corresponding to a pattern id between 1 and 48
	bool getPattern48(int id, WPSGraphicStyle::Pattern &pattern) const;
	//! returns the pattern corresponding to a pattern id between 1 and 64
	bool getPattern64(int id, WPSGraphicStyle::Pattern &pattern) const;

	//! the file version
	int m_version;
	//! a flag to know if updateState was launched
	bool m_isUpdated;
	//! a map id to cell style
	std::map<int, CellStyle> m_idCellStyleMap;
	//! a map id to color style
	std::map<int, ColorStyle> m_idColorStyleMap;
	//! a map id to font style
	std::map<int, FontStyle> m_idFontStyleMap;
	//! a map id to format style
	std::map<int, FormatStyle> m_idFormatStyleMap;
	//! a map id to graphic style
	std::map<int, GraphicStyle> m_idGraphicStyleMap;
	//! a map id to line style
	std::map<int, LineStyle> m_idLineStyleMap;

	//! a map id to font name style
	std::map<int, FontName> m_idFontNameMap;
};

bool State::getColor8(int id, WPSColor &color) const
{
	if (id<0||id>=8)
	{
		WPS_DEBUG_MSG(("LotusStyleManagerInteranl::State::getColor8(): unknown color id: %d\n",id));
		return false;
	}
	static uint32_t colorMap[]=
	{
		// black, dark blue, green, cyan/gray, red, magenta, yellow, white?
		0, 0xFF, 0xFF00, 0x7F7F7F, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF
	};
	color=WPSColor(colorMap[id]);
	return true;
}

bool State::getColor16(int id, WPSColor &color) const
{
	if (id<0||id>=16)
	{
		WPS_DEBUG_MSG(("LotusStyleManagerInteranl::State::getColor16(): unknown color id: %d\n",id));
		return false;
	}
	static uint32_t colorMap[]=
	{
		0, 0xFFFFFF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFF00FF, 0x00FFFF,
		0x7F0000, 0x007F00, 0x00007F, 0x7F7F00, 0x7F007F, 0x007F7F, 0x7F7F7F, 0x3F3F3F
	};
	color=WPSColor(colorMap[id]);
	return true;
}

bool State::getColor256(int id, WPSColor &color) const
{
	if (id<0||id>=256)
	{
		WPS_DEBUG_MSG(("LotusStyleManagerInteranl::State::getColor256(): unknown color id: %d\n",id));
		return false;
	}
	// in one file, find 0xEF as current...
	static uint32_t colorMap[]=
	{
		0xffffff, 0xffcc99, 0xffffcc, 0xccff99, 0x99ff33, 0x99ff66, 0x99ff99, 0xccffcc, // 0
		0xccffff, 0x99ccff, 0x6699ff, 0xccccff, 0xcc99ff, 0xffccff, 0xff99cc, 0xffffff,
		0xffcccc, 0xffcc66, 0xffff99, 0xccff66, 0x99ff00, 0x66ff33, 0x66ff99, 0x99ffcc, // 10
		0x99ffff, 0x3399ff, 0x6666ff, 0x9999ff, 0xcc66ff, 0xff99ff, 0xff66cc, 0xeeeeee,
		0xff9999, 0xff9966, 0xffff66, 0xccff33, 0x66ff00, 0x66ff66, 0x33ff99, 0x66ffcc, // 20
		0x66ffff, 0x0099ff, 0x3366ff, 0x9966ff, 0xcc66cc, 0xff66ff, 0xff6699, 0xdddddd,
		0xff6666, 0xff9933, 0xffff33, 0xccff00, 0x33ff00, 0x33ff66, 0x00ff99, 0x33ffcc, // 30
		0x33ffff, 0x0066ff, 0x0066cc, 0x9966cc, 0xcc33ff, 0xff33ff, 0xff3399, 0xcccccc,
		0xff3333, 0xff6633, 0xffff00, 0xcccc33, 0x00ff00, 0x00ff66, 0x66cc99, 0x00ffcc, // 40
		0x00ffff, 0x0033ff, 0x3366cc, 0x9933ff, 0xcc00ff, 0xff33cc, 0xff3366, 0xbbbbbb,
		0xff0000, 0xff6600, 0xffcc33, 0xcccc00, 0x00ee00, 0x33ff33, 0x33cc99, 0x66cccc, // 50
		0x66ccff, 0x0000ee, 0x3333ff, 0x9900ff, 0xcc00cc, 0xff00cc, 0xff0066, 0xaaaaaa,
		0xcc0000, 0xff3300, 0xffcc00, 0x99cc33, 0x00dd00, 0x00ff33, 0x00cc99, 0x33cccc, // 60
		0x33ccff, 0x0000dd, 0x3300ff, 0x6666cc, 0x9933cc, 0xcc33cc, 0xff0033, 0x999999,
		0xbb0000, 0xee0000, 0xff9900, 0x99cc00, 0x00bb00, 0x33cc00, 0x33cc66, 0x00cccc, // 70
		0x00ccff, 0x0000bb, 0x0000ff, 0x6633ff, 0x993399, 0xcc3399, 0xcc0033, 0x888888,
		0xaa0000, 0xdd0000, 0xcc9933, 0x999933, 0x00aa00, 0x33cc33, 0x00cc66, 0x009999, // 80
		0x0099cc, 0x0000aa, 0x0033cc, 0x6633cc, 0x9900cc, 0xcc0099, 0xcc0066, 0x777777,
		0x990000, 0xcc3333, 0xcc9900, 0x999900, 0x008800, 0x00cc00, 0x339966, 0x339999, // 90
		0x3399cc, 0x000088, 0x0000cc, 0x6600ff, 0x663399, 0x993366, 0xcc3366, 0x666666,
		0x660000, 0xcc3300, 0xcc6633, 0x669900, 0x007700, 0x339933, 0x009966, 0x336666, // a0
		0x336699, 0x000077, 0x3300cc, 0x3333cc, 0x663366, 0x990066, 0x990033, 0x555555,
		0x550000, 0x993300, 0xcc6600, 0x669933, 0x005500, 0x339900, 0x336633, 0x006666, // b0
		0x006699, 0x000055, 0x000099, 0x333399, 0x6600cc, 0x990099, 0x880000, 0x444444,
		0x330000, 0x663300, 0x996633, 0x336600, 0x004400, 0x009900, 0x006633, 0x333333, // c0
		0x003399, 0x000044, 0x000066, 0x330099, 0x660099, 0x660066, 0x770000, 0x333333,
		0x220000, 0x440000, 0x996600, 0x333300, 0x002200, 0x006600, 0x003300, 0x003333, // d0
		0x003366, 0x000022, 0x000033, 0x330066, 0x330033, 0x660033, 0x440000, 0x222222,
		0xcc9966, 0xcc6666, 0xcccc99, 0xcccc66, 0x99cc66, 0x66cc66, 0x99cc99, 0x99ffcc, // e0
		0x99cccc, 0x999999, 0x6699cc, 0x9999cc, 0xcc99cc, 0xcc9999, 0xcc6699, 0x111111,
		0x996666, 0x993333, 0x999966, 0x666633, 0x66cc33, 0x009933, 0x669966, 0x66cc99, // f0
		0x669999, 0x666666, 0x666699, 0x333366, 0x996699, 0x663333, 0x663366, 0x000000
	};
	color=WPSColor(colorMap[id]);
	return true;
}

bool State::getPattern48(int id, WPSGraphicStyle::Pattern &pat) const
{
	if (id<=0 || id>=49)
	{
		WPS_DEBUG_MSG(("LotusStyleManagerInternal::State::getPattern48(): unknown pattern id: %d\n",id));
		return false;
	}
	if (id==47 || id==48)
	{
		// the gradient
		static const uint16_t (patterns[])=
		{
			0x8814, 0x2241, 0x8800, 0xaa00, 0x2050, 0x8888, 0x8888, 0x502
		};
		pat.m_dim=Vec2i(8,8);
		uint16_t const *ptr=&patterns[4*(id-47)];
		pat.m_data.resize(8);
		for (size_t i=0; i < 8; i+=2)
		{
			uint16_t val=*(ptr++);
			pat.m_data[i]=(unsigned char)((val>>8) & 0xFF);
			pat.m_data[i+1]=(unsigned char)(val & 0xFF);
		}
	}
	return getPattern64(id, pat);
}

bool State::getPattern64(int id, WPSGraphicStyle::Pattern &pat) const
{
	if (id<=0 || id>=64)
	{
		WPS_DEBUG_MSG(("LotusStyleManagerInternal::State::getPattern64(): unknown pattern id: %d\n",id));
		return false;
	}
	static const uint16_t (patterns[])=
	{
		0xffff, 0xffff, 0xffff, 0xffff, 0x0, 0x0, 0x0, 0x0, 0x50a, 0x1428, 0x50a0, 0x4182, 0xa851, 0xa245, 0x8a15, 0x2a54, // 1-4
		0x2142, 0x8409, 0x1224, 0x4890, 0x102, 0x408, 0x1020, 0x4080, 0x1122, 0x4488, 0x1122, 0x4488, 0xeedd, 0xbb77, 0xeedd, 0xbb77,
		0xaaaa, 0xaaaa, 0xaaaa, 0xaaaa, 0x8888, 0x8888, 0x8888, 0x8888, 0xa050, 0x2814, 0xa05, 0x8241, 0x158a, 0x45a2, 0x51a8, 0x542a, // 9-12
		0x9048, 0x2412, 0x984, 0x4221, 0x8040, 0x2010, 0x804, 0x201, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0x0, 0xff00, 0x0,
		0x44aa, 0x11aa, 0x44aa, 0x11aa, 0x182, 0x4428, 0x1028, 0x4482, 0xf874, 0x2247, 0x8f17, 0x2271, 0xaa00, 0xaa00, 0xaa00, 0xaa00, // 17-20
		0xff88, 0x8888, 0xff88, 0x8888, 0xff80, 0x8080, 0x8080, 0x8080, 0xaa00, 0x8000, 0x8800, 0x8000, 0xbf00, 0xbfbf, 0xb0b0, 0xb0b0,
		0xddff, 0x77ff, 0xddff, 0x77ff, 0xdd77, 0xdd77, 0xdd77, 0xdd77, 0xaa55, 0xaa55, 0xaa55, 0xaa55, 0x8822, 0x8822, 0x8822, 0x8822, // 25-28
		0x8010, 0x220, 0x108, 0x4004, 0x8800, 0x2200, 0x8800, 0x2200, 0x8000, 0x800, 0x8000, 0x800, 0x40a0, 0x0, 0x40a, 0x0,
		0x8040, 0x2000, 0x204, 0x800, 0x8000, 0x0, 0x0, 0x0, 0xb130, 0x31b, 0xd8c0, 0xc8d, 0xff80, 0x8080, 0xff08, 0x808, // 33-36
		0x81c, 0x22c1, 0x8001, 0x204, 0x8244, 0x3944, 0x8201, 0x101, 0x55a0, 0x4040, 0x550a, 0x404, 0x384, 0x4830, 0xc02, 0x101,
		0x8080, 0x413e, 0x808, 0x14e3, 0x1020, 0x54aa, 0xff02, 0x408, 0x7789, 0x8f8f, 0x7798, 0xf8f8, 0x8, 0x142a, 0x552a, 0x1408, // 41-44
		0xf0f0, 0xf0f0, 0xf0f, 0xf0f, 0x9966, 0x6699, 0x9966, 0x6699, 0x4188, 0x00cc, 0x8008, 0x1422, 0x8888, 0x8805, 0x220, 0x4184,
		0xff, 0xff00, 0xff, 0xff00, 0x55aa, 0x55aa, 0x55aa, 0x55aa, 0xff55, 0xff55, 0xff55, 0xff55, 0x8142, 0x2418, 0x1824, 0x4281, // 49-52
		0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0, 0x3399, 0xcc66, 0x3399, 0xcc66, 0x3366, 0xcc99, 0x3366, 0xcc99, 0x1188, 0x4422, 0x1188, 0x4422,
		0xffcc, 0xff33, 0xffcc, 0xff33, 0xf0f0, 0x0f0f, 0xf0f0, 0x0f0f, 0xcc33, 0x3333, 0x33cc, 0xcccc, 0xf0f, 0xf0f, 0xf0f, 0xf0f,// 57-60
		0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0, 0, 0, 0xffff, 0xffff, 0xffff, 0xffff, 0, 0
	};
	pat.m_dim=Vec2i(8,8);
	uint16_t const *ptr=&patterns[4*(id-1)];
	pat.m_data.resize(8);
	for (size_t i=0; i < 8; i+=2)
	{
		uint16_t val=*(ptr++);
		pat.m_data[i]=(unsigned char)((val>>8) & 0xFF);
		pat.m_data[i+1]=(unsigned char)(val & 0xFF);
	}
	return true;
}

}

// constructor, destructor
LotusStyleManager::LotusStyleManager(LotusParser &parser) :
	m_mainParser(parser), m_state(new LotusStyleManagerInternal::State)
{
}

LotusStyleManager::~LotusStyleManager()
{
}

void LotusStyleManager::cleanState()
{
	m_state.reset(new LotusStyleManagerInternal::State);
}

void LotusStyleManager::updateState()
{
	if (m_state->m_isUpdated)
		return;
	m_state->m_isUpdated=true;
	// try to update the font styles
	std::map<int, LotusStyleManagerInternal::FontStyle>::iterator fIt;
	for (fIt=m_state->m_idFontStyleMap.begin(); fIt!=m_state->m_idFontStyleMap.end(); ++fIt)
	{
		LotusStyleManagerInternal::FontStyle &font=fIt->second;
		if (!font.m_fontId) continue;
		WPSFont defFont;
		if (!m_mainParser.getFont(font.m_fontId, defFont, font.m_fontType))
			continue;
		font.m_font.m_name = defFont.m_name;
	}
}

int LotusStyleManager::version() const
{
	if (m_state->m_version<0)
		m_state->m_version=m_mainParser.version();
	return m_state->m_version;
}

bool LotusStyleManager::getColor8(int cId, WPSColor &color) const
{
	return m_state->getColor8(cId, color);
}

bool LotusStyleManager::getColor16(int cId, WPSColor &color) const
{
	return m_state->getColor16(cId, color);
}

bool LotusStyleManager::getColor256(int cId, WPSColor &color) const
{
	return m_state->getColor256(cId, color);
}

bool LotusStyleManager::getPattern64(int id, WPSGraphicStyle::Pattern &pattern) const
{
	return m_state->getPattern64(id, pattern);
}

////////////////////////////////////////////////////////////
// styles
////////////////////////////////////////////////////////////
bool LotusStyleManager::readLineStyle(shared_ptr<WPSStream> stream, long endPos, int vers)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	const int expectedSize=vers==0 ? 8 : vers==1 ? 14 : 0;
	if (endPos-pos!=expectedSize)   // only find in a WK3 mac file
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readLineStyle: the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(LineStyle):###");
		return true;
	}
	LotusStyleManagerInternal::LineStyle line;
	int id=(int) libwps::readU8(input);
	int val=(int) libwps::readU8(input); // always 10?
	if (val!=0x10)
		f << "fl=" << std::hex << val << std::dec << ",";
	val=(int) libwps::readU16(input); // 0 or small number
	if (val) f << "f0=" << val << ",";
	WPSColor color[2]= {WPSColor::black(), WPSColor::white()};
	for (int i=0; i<2; ++i)
	{
		int col=vers==1 ? (int) libwps::readU16(input) : (int) libwps::readU8(input);
		if (col!=0xEF && !m_state->getColor256(col, color[i]))
		{
			f << "###col" << i << "=" << col << ",";
			continue;
		}
		if ((i==0 && color[0].isBlack()) || (i==1 && color[1].isWhite()))
			continue;
		f << "col" << i << "=" << color[i] << ",";
	}
	WPSColor finalColor=color[0];
	int patId;
	if (vers==0)
	{
		val=(int) libwps::readU16(input);
		patId=(val&0x3f);
		line.m_width=float((val>>6)&0xF);
		line.m_dashId=(val>>11);
	}
	else // checkme
	{
		patId=(int) libwps::readU16(input);
		line.m_width=float(libwps::readU16(input))/256.f;
		line.m_dashId=(int) libwps::readU16(input);
	}
	if (patId!=1)
	{
		f << "pattern=" << patId << ",";
		WPSGraphicStyle::Pattern pattern;
		if (patId==0) // no pattern
			line.m_width=0;
		else if (patId==2)
			finalColor=color[1];
		else if (m_state->getPattern48(patId, pattern))
		{
			pattern.m_colors[0]=color[1];
			pattern.m_colors[1]=color[0];
			pattern.getAverageColor(finalColor);
		}
	}
	if (line.m_dashId) // no plain, so ...
		finalColor=WPSColor::barycenter(0.5f, finalColor, 0.5f, WPSColor::white());
	line.m_color=finalColor;
	line.m_extra=f.str();

	f.str("");
	f << "Entries(LineStyle):L" << id << "," << line;
	if (m_state->m_idLineStyleMap.find(id)!=m_state->m_idLineStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readLineStyle: the line style %d already exists\n", id));
		f << "###";
	}
	else
		m_state->m_idLineStyleMap[id]=line;
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::updateLineStyle(int lineId, WPSGraphicStyle &style) const
{
	if (lineId==0)
		return true;
	if (m_state->m_idLineStyleMap.find(lineId)==m_state->m_idLineStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::updateLineStyle: the line style %d does not exist\n", lineId));
		return false;
	}
	LotusStyleManagerInternal::LineStyle const &line=m_state->m_idLineStyleMap.find(lineId)->second;
	style.m_lineWidth=line.m_width;
	style.m_lineColor=line.m_color;
	return true;
}

bool LotusStyleManager::readColorStyle(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	int colorSz=1;
	if (endPos-pos==7)
		colorSz=1;
	else if (endPos-pos==11)
		colorSz=2;
	else
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readColorStyle: the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(ColorStyle):###");
		return true;
	}
	int id=(int) libwps::readU8(input);
	int val=(int) libwps::readU8(input); // always 20?
	if (val!=0x20)
		f << "fl=" << std::hex << val << std::dec << ",";
	LotusStyleManagerInternal::ColorStyle color;
	for (int i=0; i<4; ++i)
	{
		val=(colorSz==1) ? (int) libwps::readU8(input) : (int) libwps::readU16(input);
		if (val!=0xEF && !m_state->getColor256(val,color.m_colors[i]))
		{
			WPS_DEBUG_MSG(("LotusStyleManager::readColorStyle: can not read a color\n"));
			f << "##colId=" << val << ",";
		}
	}
	color.m_patternId=(int) libwps::readU8(input);
	if (color.m_patternId && !m_state->getPattern48(color.m_patternId, color.m_pattern))
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readColorStyle: can not read a pattern\n"));
		f << "##patId=" << color.m_patternId << ",";
	}

	color.m_extra=f.str();

	f.str("");
	f << "Entries(ColorStyle):Co" << id << "," << color;
	if (m_state->m_idColorStyleMap.find(id)!=m_state->m_idColorStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readColorStyle: the color style %d already exists\n", id));
		f << "###";
	}
	else
		m_state->m_idColorStyleMap[id]=color;

	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::updateSurfaceStyle(int colorId, WPSGraphicStyle &style) const
{
	if (colorId==0)
		return true;
	if (m_state->m_idColorStyleMap.find(colorId)==m_state->m_idColorStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::updateSurfaceStyle: the color style %d does not exist\n", colorId));
		return false;
	}
	LotusStyleManagerInternal::ColorStyle const &color=m_state->m_idColorStyleMap.find(colorId)->second;
	if (color.m_patternId==0) // empty
		return true;
	WPSColor finalColor=color.m_colors[2];
	WPSGraphicStyle::Pattern pattern;
	if (color.m_patternId==2)
		finalColor=color.m_colors[3];
	else if (color.m_patternId==47 || color.m_patternId==48)
	{
		style.m_gradientType=WPSGraphicStyle::G_Linear;
		style.m_gradientStopList.clear();
		style.m_gradientStopList.push_back(WPSGraphicStyle::GradientStop(0.0, color.m_patternId==47 ? color.m_colors[2] : WPSColor::black()));
		style.m_gradientStopList.push_back(WPSGraphicStyle::GradientStop(1.0, color.m_patternId==47 ? WPSColor::black() : color.m_colors[2]));
	}
	else if (color.m_patternId!=1 && m_state->getPattern48(color.m_patternId, pattern))
	{
		pattern.m_colors[0]=color.m_colors[3];
		pattern.m_colors[1]=color.m_colors[2];
		if (!pattern.getUniqueColor(finalColor))
			style.setPattern(pattern);
	}

	if (!style.hasPattern() && !style.hasGradient())
		style.setSurfaceColor(finalColor);
	return true;
}

bool LotusStyleManager::updateSurfaceStyle(int fColorId, int bColorId, int patternId, WPSGraphicStyle &style) const
{
	if (patternId==0)
		return true;
	WPSGraphicStyle::Pattern pattern;
	if (!getColor256(fColorId, pattern.m_colors[0])||!getColor256(bColorId, pattern.m_colors[1]))
	{
		WPS_DEBUG_MSG(("LotusStyleManager::updateSurfaceStyle: can not find some colors\n"));
		return false;
	}
	if (patternId>=60 && patternId<=63)
	{
		style.m_gradientType=WPSGraphicStyle::G_Linear;
		style.m_gradientStopList.clear();
		style.m_gradientStopList.push_back(WPSGraphicStyle::GradientStop(0.0, pattern.m_colors[0]));
		style.m_gradientStopList.push_back(WPSGraphicStyle::GradientStop(1.0, pattern.m_colors[1]));
		float const(angles[])= {90, 270, 0, 180};
		style.m_gradientAngle=angles[patternId-60];
		return true;
	}
	if (!getPattern64(patternId, pattern))
	{
		WPS_DEBUG_MSG(("LotusStyleManager::updateSurfaceStyle: can not find the pattern\n"));
		return false;
	}
	WPSColor color;
	if (pattern.getUniqueColor(color))
		style.setSurfaceColor(color);
	else
		style.m_pattern=pattern;
	return true;
}

bool LotusStyleManager::updateShadowStyle(int colorId, WPSGraphicStyle &style) const
{
	if (colorId==0)
		return true;
	if (m_state->m_idColorStyleMap.find(colorId)==m_state->m_idColorStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::updateShadowStyle: the color style %d does not exist\n", colorId));
		return false;
	}
	LotusStyleManagerInternal::ColorStyle const &color=m_state->m_idColorStyleMap.find(colorId)->second;
	if (color.m_patternId==0) // empty
		return true;
	WPSColor finalColor=color.m_colors[2];
	WPSGraphicStyle::Pattern pattern;
	if (color.m_patternId==2)
		finalColor=color.m_colors[3];
	else if (color.m_patternId!=1 && m_state->getPattern48(color.m_patternId, pattern))
	{
		pattern.m_colors[0]=color.m_colors[3];
		pattern.m_colors[1]=color.m_colors[2];
		pattern.getAverageColor(finalColor);
	}
	style.setShadowColor(finalColor);
	style.m_shadowOffset=Vec2f(3,3);
	return true;
}

bool LotusStyleManager::readGraphicStyle(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	if (endPos-pos!=13)   // only find in a WK3 mac file
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readGraphicStyle: the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(GraphicStyle):###");
		return true;
	}
	int id=(int) libwps::readU8(input);
	LotusStyleManagerInternal::GraphicStyle style;
	int val=(int) libwps::readU8(input); // always 40?
	if (val!=0x40)
		f << "fl=" << std::hex << val << std::dec << ",";
	for (int i=0; i<4; ++i)
	{
		val=(int) libwps::readU8(input);
		int fl=(int) libwps::readU8(input);
		if (!val) continue;
		if (i==0) f << "unknId=" << val << "[" << std::hex << fl << std::dec << ",";
		else if (i==1)
		{
			if (fl!=0x10) f << "#fl[line]=" << std::hex << fl << std::dec << ",";
			if (m_state->m_idLineStyleMap.find(val)==m_state->m_idLineStyleMap.end())
			{
				WPS_DEBUG_MSG(("LotusStyleManager::readGraphicStyle: the line style %d does not exists\n", val));
				f << "###lineId=" << val << ",";
			}
			else
				style.m_lineId=val;
		}
		else
		{
			if (fl!=0x20) f << "#fl[color" << i-2 << "]=" << std::hex << fl << std::dec << ",";
			if (m_state->m_idColorStyleMap.find(val)==m_state->m_idColorStyleMap.end())
			{
				WPS_DEBUG_MSG(("LotusStyleManager::readGraphicStyle: the color style %d does not exists\n", val));
				f << "###colorId[" << i-2 << "]=" << val << ",";
			}
			else
				style.m_colorsId[i-2]=val;
		}
	}
	for (int i=0; i<3; ++i)   //f0=f1=0|1|3|4 : a size?, f2=2|3|22
	{
		val=(int) libwps::readU8(input);
		if (val)
			f << "f" << i << "=" << std::hex << val << std::dec << ",";
	}
	style.m_extra=f.str();
	f.str("");
	f << "Entries(GraphicStyle):G" << id << "," << style;

	if (m_state->m_idGraphicStyleMap.find(id)!=m_state->m_idGraphicStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readGraphicStyle: the graphic style %d already exists\n", id));
		f << "###";
	}
	else
		m_state->m_idGraphicStyleMap[id]=style;

	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::updateGraphicStyle(int graphicId, WPSGraphicStyle &style) const
{
	if (graphicId==0)
		return true;
	if (m_state->m_idGraphicStyleMap.find(graphicId)==m_state->m_idGraphicStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::updateGraphicStyle: the graphic style %d does not exist\n", graphicId));
		return false;
	}
	LotusStyleManagerInternal::GraphicStyle const &graphic=m_state->m_idGraphicStyleMap.find(graphicId)->second;
	if (graphic.m_lineId)
		updateLineStyle(graphic.m_lineId, style);
	if (graphic.m_colorsId[0])
		updateSurfaceStyle(graphic.m_colorsId[0], style);
	if (graphic.m_colorsId[1])
		updateShadowStyle(graphic.m_colorsId[1], style);
	return true;
}

bool LotusStyleManager::readFontStyleA0(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	if (endPos-pos!=12)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFontStyleA0: the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(FontStyle):###");
		return true;
	}
	int id=(int) libwps::readU8(input);
	LotusStyleManagerInternal::FontStyle font(m_mainParser.getDefaultFontType());
	int val=(int) libwps::readU8(input); // always 0?
	if (val)
		f << "fl=" << std::hex << val << std::dec << ",";
	for (int i=0; i<2; ++i)   // always 0?
	{
		val=(int) libwps::readU8(input);
		if (val)
			f << "f" << i << "=" << val << ",";
	}
	val=(int) libwps::readU8(input);
	if (val!=0xFF)
		f << "g0=" << std::hex << val << std::dec << ",";
	// we can not read the font name here, because the font is defined after...
	font.m_fontId=(int) libwps::readU8(input);
	val=(int) libwps::readU16(input);
	if (val)
		font.m_font.m_size=val/32.;
	for (int i=0; i<2; ++i)
	{
		val=(int) libwps::readU8(input);
		if (val==0xEF) continue;
		WPSColor color;
		if (!getColor256(val, color))
			f << "#col" << i << "=" << std::hex << val << std::dec << ",";
		else if (i==0)
			font.m_font.m_color=color;
		else if (color!=font.m_font.m_color) //unsured
			f << "col[def]=" << color << ",";
	}
	val=(int) libwps::readU8(input);
	if (val)
	{
		if (val&1) font.m_font.m_attributes |= WPS_BOLD_BIT;
		if (val&2) font.m_font.m_attributes |= WPS_ITALICS_BIT;
		if (val&4) font.m_font.m_attributes |= WPS_UNDERLINE_BIT;
		if (val&8) font.m_font.m_attributes |= WPS_OUTLINE_BIT;
		if (val&0x10) font.m_font.m_attributes |= WPS_SHADOW_BIT;
		if (val&0x20) font.m_font.m_spacing=-2;
		if (val&0x40) font.m_font.m_spacing=2;
		if (val&0x80) f << "flags[#80],";
	}
	val=(int) libwps::readU8(input);
	if (val) // 0|18|20|24
		f << "h0=" << std::hex << val << std::dec << ",";
	font.m_extra=f.str();
	if (m_state->m_idFontStyleMap.find(id)!=m_state->m_idFontStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFontStyleA0: the font style %d already exists\n", id));
		f << "###";
	}
	else
		m_state->m_idFontStyleMap.insert
		(std::map<int,LotusStyleManagerInternal::FontStyle>::value_type(id,font));

	f.str("");
	f << "Entries(FontStyle):FS" << id << "," << font;
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::readFontStyleF0(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	long sz=endPos-pos;
	if (sz<20)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFontStyleF0: the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(FontStyle):###");
		return true;
	}
	int id=(int) libwps::readU8(input);
	LotusStyleManagerInternal::FontStyle font(m_mainParser.getDefaultFontType());
	if (id&8)
	{
		font.m_font.m_attributes |= WPS_BOLD_BIT;
		f << "bold,";
	}
	if (id&0x10)
	{
		font.m_font.m_attributes |= WPS_ITALICS_BIT;
		f << "italic,";
	}
	int fSz=int(libwps::readU16(input));
	font.m_font.m_size=double(fSz)/256;
	int val=(int) libwps::readU16(input);
	if (val!=fSz)
		f << "sz2=" << double(val)/256 << ",";
	for (int i=0; i<5; ++i)   // always 0?
	{
		val=(int) libwps::readU8(input);
		if (val)
			f << "f" << i << "=" << val << ",";
	}
	for (int i=0; i<8; ++i)   // fl2=background?, fl3=pattern?
	{
		// fl0=0|c0, fl1=0|1, fl2=0|80, fl3=[0-5]0, fl4=0-4, fl5=0|50, fl6=0
		val=(int) libwps::readU8(input);
		if (!val)
			continue;
		if (i==7)
			f << "font[id]=" << val << ",";
		else
			f << "fl" << i << "=" << std::hex << val << std::dec << ",";
	}
	val=(int) libwps::readU8(input);
	WPSColor color;
	if (!getColor256(val, color)) f << "#colorId=" << val << ",";
	else
	{
		font.m_font.m_color=color;
		if (!color.isBlack()) f << "color=" << color << ",";
	}

	librevenge::RVNGString name("");
	for (long i=19; i<sz; ++i)
	{
		unsigned char c=static_cast<unsigned char>(libwps::readU8(input));
		if (!c) break;
		libwps::appendUnicode((uint32_t)libwps_tools_win::Font::unicode(c,font.m_fontType), name);
	}
	font.m_font.m_name=name;
	libwps_tools_win::Font::Type fType=libwps_tools_win::Font::getFontType(name);
	if (fType!=libwps_tools_win::Font::UNKNOWN) font.m_fontType=fType;
	f << name.cstr() << ",";
	if (input->tell()!=endPos) ascFile.addDelimiter(input->tell(),'|');
	font.m_extra=f.str();

	if (m_state->m_idFontStyleMap.find(id)!=m_state->m_idFontStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFontStyleA0: the font style %d already exists\n", id));
		f << "###";
	}
	else
		m_state->m_idFontStyleMap.insert
		(std::map<int,LotusStyleManagerInternal::FontStyle>::value_type(id,font));
	f.str("");
	f << "Entries(FontStyle):FS" << id << "," << font;
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::updateFontStyle(int fontId, WPSFont &font, libwps_tools_win::Font::Type &fontType)
{
	if (fontId==0)
		return true;
	if (m_state->m_idFontStyleMap.find(fontId)==m_state->m_idFontStyleMap.end())
	{
		static bool first=true;
		if (first)
		{
			WPS_DEBUG_MSG(("LotusStyleManager::updateFontStyle: the font style %d does not exist\n", fontId));
			first=false;
		}
		return false;
	}
	LotusStyleManagerInternal::FontStyle const &fontStyle=m_state->m_idFontStyleMap.find(fontId)->second;
	font=fontStyle.m_font;
	fontType=fontStyle.m_fontType;
	return true;
}

bool LotusStyleManager::readFormatStyle(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	if (endPos-pos<23)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFormatStyle: the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(FormatStyle):###");
		return true;
	}
	int id=(int) libwps::readU8(input);
	LotusStyleManagerInternal::FormatStyle format;
	int val=(int) libwps::readU8(input); // always 30?
	if (val!=0x30)
		f << "fl=" << std::hex << val << std::dec << ",";

	for (int i=0; i<10; ++i) // always f1=100, other 0?
	{
		val=(int) libwps::readU16(input);
		if (val) f << "f" << i << "=" << val << ",";
	}
	bool ok=true;
	for (int i=0; i<2; ++i)
	{
		val=(int) libwps::readU8(input);
		if (val==0xf) continue;
		if (val!=0x3c)
		{
			WPS_DEBUG_MSG(("LotusStyleManager::readFormatStyle: find unknown type\n"));
			f << "###type=" << std::hex << val << std::dec << ",";
			ok=false;
			break;
		}
		int dSz=(int) libwps::readU8(input);
		if (input->tell()+dSz+1>endPos)
		{
			WPS_DEBUG_MSG(("LotusStyleManager::readFormatStyle: bad string size\n"));
			f << "###size=" << std::hex << dSz << std::dec << ",";
			ok=false;
			break;
		}
		std::string name("");
		for (int j=0; j<dSz; ++j)
			name += (char) libwps::readU8(input);
		if (i==0)
			format.m_prefix=name;
		else
			format.m_suffix=name;
	}
	if (ok && input->tell()+1<=endPos)
	{
		val=(int) libwps::readU8(input);
		if (val!=0xc)
			f << "g0=" << val << ",";
	}
	format.m_extra=f.str();

	if (m_state->m_idFormatStyleMap.find(id)!=m_state->m_idFormatStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFormatStyle: the format style %d already exists\n", id));
		f << "###";
	}
	else
		m_state->m_idFormatStyleMap[id]=format;

	f.str("");
	f << "Entries(FormatStyle):Fo" << id << "," << format;
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::readCellStyleD2(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	if (endPos-pos!=21 && endPos-pos!=33)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleD2 the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(CellStyle):###");
		return true;
	}
	int id=(int) libwps::readU8(input);
	int val=(int) libwps::readU8(input); // always 50?
	if (endPos-pos==33)   // wk6...wk9
	{
		static bool first=true;
		if (first)
		{
			first=false;
			WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleD2 sorry, reading cell style is not implemented\n"));
		}
		f << "Entries(CellStyle):Ce" << id << ",";
		if (val!=0x50)
			f << "fl=" << std::hex << val << std::dec << ",";
		// OP_CreatePattern123 in op.cxx contains some data
		ascFile.addPos(pos-6);
		ascFile.addNote(f.str().c_str());
		return true;
	}

	LotusStyleManagerInternal::CellStyle cell(m_mainParser.getDefaultFontType());
	if (val!=0x50)
		f << "fl=" << std::hex << val << std::dec << ",";
	for (int i=0; i<2; ++i)   // always 0?
	{
		val=(int) libwps::readU8(input);
		if (val)
			f << "f" << i << "=" << val << ",";
	}
	for (int i=0; i<8; ++i)
	{
		val=(int) libwps::readU8(input);
		int fl=(int) libwps::readU8(input);
		if (!val) continue;
		if (i<4)
		{
			if (fl!=0x10) f << "#fl[border" << i << "]=" << std::hex << fl << std::dec << ",";
			if (m_state->m_idLineStyleMap.find(val)==m_state->m_idLineStyleMap.end())
			{
				WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleD2: the line style %d does not exists\n", val));
				f << "###borderId" << i << "=" << val << ",";
			}
			else
				cell.m_bordersId[i]=val;
		}
		else if (i==4 || i==7)
		{
			int wh=i==4 ? 0 : 1;
			if (fl!=0x20) f << "#fl[color" << wh << "]=" << std::hex << fl << std::dec << ",";
			if (m_state->m_idColorStyleMap.find(val)==m_state->m_idColorStyleMap.end())
			{
				WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleD2: the color style %d does not exists\n", val));
				f << "###colorId[" << wh << "]=" << val << ",";
			}
			else
				cell.m_colorsId[wh]=val;
		}
		else if (i==5)
		{
			if (fl) f << "#fl[font]=" << std::hex << fl << std::dec << ",";
			if (m_state->m_idFontStyleMap.find(val)==m_state->m_idFontStyleMap.end())
			{
				WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleD2: the font style %d does not exists\n", val));
				f << "###fontId=" << val << ",";
			}
			else
				cell.m_fontId=val;
		}
		else
		{
			if (fl!=0x30) f << "#fl[format]=" << std::hex << fl << std::dec << ",";
			if (m_state->m_idFormatStyleMap.find(val)==m_state->m_idFormatStyleMap.end())
			{
				WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleD2: the format style %d does not exists\n", val));
				f << "###formatId=" << val << ",";
			}
			else
				cell.m_formatId=val;
		}
	}
	val=(int) libwps::readU8(input);
	cell.m_borders=(val&0xF);
	val >>=4;
	// small number 0|2
	if (val) f << "f2=" << val << ",";
	cell.m_extra=f.str();
	f.str("");
	f << "Entries(CellStyle):Ce" << id << "," << cell;

	if (m_state->m_idCellStyleMap.find(id)!=m_state->m_idCellStyleMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleD2: the cell style %d already exists\n", id));
		f << "###";
	}
	else
		m_state->m_idCellStyleMap.insert(std::map<int, LotusStyleManagerInternal::CellStyle>::value_type(id,cell));
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::readCellStyleE6(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	long sz=endPos-pos;
	if ((sz%15)!=10)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleE6: the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(CellStyle):###");
		return true;
	}
	f << "Entries(CellStyle):";
	for (int i=0; i<5; ++i)
	{
		int val=int(libwps::readU16(input));
		int const(expected[])= {0x10, 0x100, 0x10, 0xe, 0};
		if (val!=expected[i]) f << "f0=" << val << ",";
	}
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	int N=int(sz/15);
	libwps_tools_win::Font::Type fontType=m_mainParser.getDefaultFontType();
	for (int i=0; i<N; ++i)
	{
		pos=input->tell();
		f.str("");
		f << "CellStyle-" << i << ":";
		int id=int(libwps::readU8(input));
		f << "Ce" << id << ",";
		LotusStyleManagerInternal::CellStyle cell(fontType);
		int fId=int(libwps::readU8(input));
		LotusStyleManagerInternal::FontStyle &font=cell.m_fontStyle;
		if (fId)
		{
			if (!updateFontStyle(fId, font.m_font, font.m_fontType))
				f << "#";
			f << "FS" << fId << ",";
		}
		for (int j=0; j<5; ++j)   // fl0=0|20|40, fl1=0|f|32|36|76|a8, fl2=XX, fl3=0-b|7c, fl4=0|2
		{
			int val=int(libwps::readU8(input));
			if (j==0)
			{
				if (val&0x60)
				{
					int wh=((val>>5)&3);
					if (wh==1)
					{
						font.m_font.m_attributes |= WPS_UNDERLINE_BIT;
						f << "underline,";
					}
					else if (wh==2)
					{
						f << "underline[double],";
						font.m_font.m_attributes |= WPS_DOUBLE_UNDERLINE_BIT;
					}
					else
					{
						font.m_font.m_attributes |= WPS_UNDERLINE_BIT;
						f << "underline[w=2],"; // FIXME
					}
				}
				val&=0x9F;
			}
			if (!val) continue;
			if (j==1 || j==2)
			{
				WPSColor color;
				if (!getColor256(val, color)) f << "#colorId=" << val << ",";
				else
				{
					cell.m_colorStyle.m_colors[j==2 ? 3 : 2]=color;
					if (!color.isWhite()) f << "color" << j << "=" << color << ",";
				}
			}
			else if (j==3)
			{
				// checkme: something is not right here...
				if (val==1 || val==3)
				{
					cell.m_colorStyle.m_patternId=2;
					f << "pat[low]=" << val << ",";
				}
				else
				{
					cell.m_colorStyle.m_patternId=(val>>2);
					f << "pat=" << (val>>2) << ",";
					if (val&3) f << "pat[low]=" << (val&3) << ",";
				}
			}
			else
				f << "fl" << j << "=" << std::hex << val << std::dec << ",";
		}
		int colors[4]; // TBLR
		int borders[4];
		unsigned long lVal=libwps::readU16(input);
		colors[0]=(lVal>>10)&0x1f;
		colors[1]=(lVal>>5)&0x1f;
		lVal&=0xC21F;
		if (lVal) f << "col[h0]=" << std::hex << lVal << std::dec << ",";
		lVal=libwps::readU16(input);
		colors[2]=(lVal>>5)&0x1f;
		colors[3]=(lVal>>0)&0x1f;
		borders[1]=(lVal>>10)&0xf;
		lVal&=0xC210;
		if (lVal) f << "col[h1]=" << std::hex << lVal << std::dec << ",";
		lVal=libwps::readU16(input);
		borders[0]=(lVal&0xf);
		borders[3]=((lVal>>4)&0xf);
		borders[2]=((lVal>>8)&0xf);
		lVal&=0xF000;
		if (lVal) f << "col[h2]=" << std::hex << lVal << std::dec << ",";
		for (int j=0; j<4; ++j)
		{
			if (!borders[j]) continue;
			WPSBorder border;
			switch (borders[j])
			{
			case 1:
				border.m_style=WPSBorder::Simple;
				break;
			case 2:
				border.m_style=WPSBorder::Simple;
				border.m_type=WPSBorder::Double;
				break;
			case 3:
				border.m_style=WPSBorder::Simple;
				border.m_width=2;
				break;
			case 4:
				border.m_style=WPSBorder::Dot; // 1x1
				break;
			case 5:
				border.m_style=WPSBorder::LargeDot; // 2x2
				break;
			case 6:
				border.m_style=WPSBorder::Dash; // 3x1
				break;
			case 7:
				border.m_style=WPSBorder::Dash; // 1x1 2x1
				break;
			case 8:
				border.m_style=WPSBorder::Dash; // 1x1 1x1 2x1
				break;
			default:
				WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleE6: can not read some border format\n"));
				f << "##border" << j << "=" << borders[j] << ",";
				border.m_style=WPSBorder::Simple;
				break;
			}
			if (!getColor16(colors[j], border.m_color))
			{
				WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleE6: can not read some color\n"));
				f << "##col" << j << "=" << colors[j] << ",";
			}
			char const *(wh[])= {"T", "B", "L", "R"};
			f << "bord" << wh[j] << "=" << border << ",";
			cell.m_bordersStyle[j]=border;
		}
		f << "],";
		ascFile.addDelimiter(input->tell(),'|');
		if (m_state->m_idCellStyleMap.find(id)!=m_state->m_idCellStyleMap.end())
		{
			WPS_DEBUG_MSG(("LotusStyleManager::readCellStyleE6: the cell style %d already exists\n", id));
			f << "###id";
		}
		else
			m_state->m_idCellStyleMap.insert(std::map<int, LotusStyleManagerInternal::CellStyle>::value_type(id,cell));

		input->seek(pos+15,librevenge::RVNG_SEEK_SET);
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
	}
	return true;
}

bool LotusStyleManager::updateCellStyle(int cellId, WPSCellFormat &format,
                                        WPSFont &font, libwps_tools_win::Font::Type &fontType)
{
	if (cellId==0)
		return true;
	if (m_state->m_idCellStyleMap.find(cellId)==m_state->m_idCellStyleMap.end())
	{
		static bool first=true;
		if (first)
		{
			WPS_DEBUG_MSG(("LotusStyleManager::updateCellStyle: the cell style %d does not exist\n", cellId));
			first=false;
		}
		return false;
	}
	int const vers=version();
	LotusStyleManagerInternal::CellStyle const &cellStyle=m_state->m_idCellStyleMap.find(cellId)->second;
	if (vers==3)
	{
		font=cellStyle.m_fontStyle.m_font;
		fontType=cellStyle.m_fontStyle.m_fontType;
		format.setFont(font);
		for (int i=0; i<4; ++i)
		{
			static int const(wh[])= {WPSBorder::TopBit,WPSBorder::BottomBit,WPSBorder::LeftBit,WPSBorder::RightBit};
			format.setBorders(wh[i],cellStyle.m_bordersStyle[i]);
		}
	}
	// wk1 or wk3
	if ((vers!=3 && cellStyle.m_colorsId[0]) || vers==3)
	{
		if (vers!=3 && m_state->m_idColorStyleMap.find(cellStyle.m_colorsId[0])==m_state->m_idColorStyleMap.end())
		{
			WPS_DEBUG_MSG(("LotusStyleManager::updateCellStyle: the color style %d does not exist\n", cellStyle.m_colorsId[0]));
		}
		else
		{
			LotusStyleManagerInternal::ColorStyle const &color=
			    vers==3 ? cellStyle.m_colorStyle : m_state->m_idColorStyleMap.find(cellStyle.m_colorsId[0])->second;
			if (color.m_patternId)   // not empty
			{
				WPSColor finalColor=color.m_colors[2];
				WPSGraphicStyle::Pattern pattern;
				if (color.m_patternId==2)
					finalColor=color.m_colors[3];
				else if (color.m_patternId!=1 &&
				         ((vers<3 && m_state->getPattern48(color.m_patternId, pattern)) ||
				          (vers>=3 && m_state->getPattern64(color.m_patternId, pattern))))
				{
					pattern.m_colors[0]=color.m_colors[3];
					pattern.m_colors[1]=color.m_colors[2];
					pattern.getAverageColor(finalColor);
				}
				format.setBackgroundColor(finalColor);
			}
		}
	}
	if (vers==3) return true;
	// wk3
	if (cellStyle.m_fontId)
	{
		if (updateFontStyle(cellStyle.m_fontId, font, fontType))
			format.setFont(font);
	}
	if (!cellStyle.m_borders)
		return true;
	for (int i=0,depl=1; i<4; ++i, depl*=2)
	{
		if ((cellStyle.m_borders&depl)==0) continue;
		static int const(wh[])= {WPSBorder::TopBit,WPSBorder::LeftBit,WPSBorder::BottomBit,WPSBorder::RightBit};
		WPSBorder border;
		format.setBorders(wh[i],border);
	}
	return true;
}

bool LotusStyleManager::readFMTFontName(shared_ptr<WPSStream> stream)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	int type = (int) libwps::read16(input);
	if (type!=0xae)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontName: not a font name definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	long endPos=pos+4+sz;
	f << "Entries(FMTFont)[name]:";
	if (sz < 2)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontName: the zone is too short\n"));
		f << "###";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	int id=(int) libwps::readU8(input);
	f << "id=" << id << ",";
	bool nameOk=true;
	std::string name("");
	for (long i=1; i<sz; ++i)
	{
		char c=(char) libwps::readU8(input);
		if (!c) break;
		if (nameOk && !(c==' ' || (c>='0'&&c<='9') || (c>='a'&&c<='z') || (c>='A'&&c<='Z')))
		{
			nameOk=false;
			WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontName: find odd character in name\n"));
			f << "#";
		}
		name += c;
	}
	f << name << ",";
	if (m_state->m_idFontNameMap.find(id)!=m_state->m_idFontNameMap.end())
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontName: can not update font map for id=%d\n", id));
	}
	else
	{
		LotusStyleManagerInternal::FontName font;
		font.m_name=name;
		m_state->m_idFontNameMap[id]=font;
	}
	if (input->tell()!=endPos)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontName: find extra data\n"));
		f << "###extra";
		input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::readFMTFontId(shared_ptr<WPSStream> stream)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	int type = (int) libwps::read16(input);
	if (type!=0xb0)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontId: not a font id definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	long endPos=pos+4+sz;
	f << "Entries(FMTFont)[ids]:";
	if ((sz%2)!=0)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontId: the zone size is odd\n"));
		f << "###";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	f << "ids=[";
	bool isFirstError=true;
	for (int i=0; i<int(sz)/2; ++i)
	{
		int id =int(libwps::readU16(input));
		f << id << ",";
		if (m_state->m_idFontNameMap.find(i)!=m_state->m_idFontNameMap.end())
			m_state->m_idFontNameMap.find(i)->second.m_id=id;
		else if (isFirstError)
		{
			isFirstError=false;
			WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontId: can not update some font map for id=%d\n", id));
		}
	}
	f << "],";
	if (input->tell()!=endPos)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontId: find extra data\n"));
		f << "###extra";
		input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::readFMTFontSize(shared_ptr<WPSStream> stream)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input=stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	int type = (int) libwps::read16(input);
	if (type!=0xaf && type!=0xb1)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontSize: not a font size definition\n"));
		return false;
	}
	long sz = (long) libwps::readU16(input);
	long endPos=pos+4+sz;
	int const wh=type==0xaf ? 0 : 1;
	f << "Entries(FMTFont)[size" << wh << "]:";
	if ((sz%2)!=0)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontSize: the zone size is odd\n"));
		f << "###";
		ascFile.addPos(pos);
		ascFile.addNote(f.str().c_str());
		return true;
	}
	f << "size=[";
	bool isFirstError=true;
	for (int i=0; i<int(sz)/2; ++i)
	{
		int size =int(libwps::readU16(input));
		f << size << ",";
		if (m_state->m_idFontNameMap.find(i)!=m_state->m_idFontNameMap.end())
			m_state->m_idFontNameMap.find(i)->second.m_size[wh]=size;
		else if (isFirstError)
		{
			isFirstError=false;
			WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontSize: can not update some font map for size=%d\n", size));
		}
	}
	f << "],";
	if (input->tell()!=endPos)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readFMTFontSize: find extra data\n"));
		f << "###extra";
		input->seek(endPos, librevenge::RVNG_SEEK_SET);
	}
	ascFile.addPos(pos);
	ascFile.addNote(f.str().c_str());
	return true;
}

bool LotusStyleManager::readMenuStyleE7(shared_ptr<WPSStream> stream, long endPos)
{
	if (!stream) return false;
	RVNGInputStreamPtr &input = stream->m_input;
	libwps::DebugFile &ascFile=stream->m_ascii;
	libwps::DebugStream f;

	long pos = input->tell();
	if (endPos-pos<23)
	{
		WPS_DEBUG_MSG(("LotusStyleManager::readMenuStyleE7 the zone size seems bad\n"));
		ascFile.addPos(pos-6);
		ascFile.addNote("Entries(MenuStyle):###");
		return true;
	}
	f << "Entries(MenuStyle):";
	f << "id=" << libwps::readU16(input) << ",";
	for (int i=0; i<2; ++i)   // fl0=0|[048c]0FF, fl1=[04]00[0-3]
	{
		int val=int(libwps::readU16(input));
		if (val) f << "fl" << i << "=" << std::hex << val << std::dec << ",";
	}
	std::string name;
	for (int i=0; i<16; ++i)
	{
		char c=char(libwps::readU8(input));
		if (!c) break;
		name += c;
	}
	f << name << ",";
	input->seek(pos+22, librevenge::RVNG_SEEK_SET);
	name="";
	int maxN=int(endPos-input->tell());
	for (int i=0; i<maxN; ++i)
	{
		char c=char(libwps::readU8(input));
		if (!c) break;
		name += c;
	}
	f << name;
	if (input->tell() != endPos)
		ascFile.addDelimiter(input->tell(),'|');
	ascFile.addPos(pos-6);
	ascFile.addNote(f.str().c_str());
	return true;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
