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

#ifndef WPS_PARAGRAPH
#  define WPS_PARAGRAPH

#include <assert.h>
#include <iostream>

#include <vector>

#include "libwps_internal.h"

#include "WPSList.h"

class WPSContentListener;
class WPXPropertyListVector;

struct WPSTabStop
{
	enum Alignment { LEFT, RIGHT, CENTER, DECIMAL, BAR };
	WPSTabStop(double position = 0.0, Alignment alignment = LEFT, uint16_t leaderCharacter='\0', uint8_t leaderNumSpaces = 0)  :
		m_position(position), m_alignment(alignment), m_leaderCharacter(leaderCharacter), m_leaderNumSpaces(leaderNumSpaces)
	{
	}
	void addTo(WPXPropertyListVector &propList, double decalX=0.0);
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, WPSTabStop const &ft);
	double m_position;
	Alignment m_alignment;
	uint16_t m_leaderCharacter;
	uint8_t m_leaderNumSpaces;
};

//! class to store the paragraph properties
struct WPSParagraph
{
	typedef WPSList::Level ListLevel;

	//! constructor
	WPSParagraph() : m_tabs(), m_justify(libwps::JustificationLeft),
		m_breakStatus(0), m_listLevelIndex(0), m_listLevel(), m_backgroundColor(0xFFFFFF),
		m_border(0), m_borderStyle(), m_extra("")
	{
		for(int i = 0; i < 3; i++) m_margins[i] = m_spacings[i] = 0.0;
		m_spacings[0] = 1.0; // interline normal
	}
	virtual ~WPSParagraph() {}
	//! send data to the listener
	void send(shared_ptr<WPSContentListener> listener) const;
	//! operator <<
	friend std::ostream &operator<<(std::ostream &o, WPSParagraph const &ft);

	/** the margins
	 *
	 * - 0: first line left margin
	 * - 1: left margin
	 * - 2: right margin*/
	double m_margins[3]; // 0: first line left, 1: left, 2: right
	/** the line spacing
	 *
	 * - 0: interline
	 * - 1: before
	 * - 2: after */
	double m_spacings[3]; // 0: interline, 1: before, 2: after
	//! the tabulations
	std::vector<WPSTabStop> m_tabs;

	/** the justification */
	libwps::Justification m_justify;
	/** a list of bits: 0x1 (unbreakable), 0x2 (do not break after) */
	int m_breakStatus; // BITS: 1: unbreakable, 2: dont break after

	/** the actual level index */
	int m_listLevelIndex;
	/** the actual level */
	ListLevel m_listLevel;

	//! the background color
	uint32_t m_backgroundColor;

	//! list of bits to indicated a border 1: LeftBorderBit, 2: RightBorderBit, ...
	int m_border;
	//! the border style
	WPSBorder m_borderStyle;

	//! a string to store some errors
	std::string m_extra;
};
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
