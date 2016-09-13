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
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 */

#ifndef LOTUS_STYLE_MANAGER_H
#define LOTUS_STYLE_MANAGER_H

#include <ostream>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"

#include "WPSDebug.h"

namespace LotusParserInternal
{
struct LotusStream;
}

namespace LotusStyleManagerInternal
{
struct State;
}

class LotusParser;

/**
 * This class parses the Lotus style
 *
 */
class LotusStyleManager
{
public:
	friend class LotusParser;

	//! constructor
	explicit LotusStyleManager(LotusParser &parser);
	//! destructor
	~LotusStyleManager();
	//! clean internal state
	void cleanState();
	//! update the state (need to be called before asking for style)
	void updateState();

	//! returns if possible the color
	bool getColor(int cId, WPSColor &color) const;

	//! update a cell format using the cell id
	bool updateCellStyle(int cellId, WPSCellFormat &format,
	                     WPSFont &font, libwps_tools_win::Font::Type &fontType);
	//! update a font using the font id
	bool updateFontStyle(int fontId, WPSFont &font, libwps_tools_win::Font::Type &fontType);
	//! update style using line id
	bool updateLineStyle(int lineId, WPSGraphicStyle &style) const;
	//! update style using color id
	bool updateSurfaceStyle(int colorId, WPSGraphicStyle &style) const;
	//! update style using graphic id
	bool updateGraphicStyle(int graphicId, WPSGraphicStyle &style) const;
protected:
	//! return the file version
	int version() const;

	//
	// low level
	//

	//! reads a cell style
	bool readCellStyle(LotusParserInternal::LotusStream &stream, long endPos);
	//! reads a color style
	bool readColorStyle(LotusParserInternal::LotusStream &stream, long endPos);
	//! reads a font style
	bool readFontStyle(LotusParserInternal::LotusStream &stream, long endPos);
	//! reads a format style
	bool readFormatStyle(LotusParserInternal::LotusStream &stream, long endPos);
	//! reads a line style
	bool readLineStyle(LotusParserInternal::LotusStream &stream, long endPos, int vers);
	//! reads a graphic style
	bool readGraphicStyle(LotusParserInternal::LotusStream &stream, long endPos);

	// old fmt style

	//! reads a format font name: zones 0xae
	bool readFMTFontName(LotusParserInternal::LotusStream &stream);
	//! reads a format font sizes zones 0xaf and 0xb1
	bool readFMTFontSize(LotusParserInternal::LotusStream &stream);
	//! reads a format font id zone: 0xb0
	bool readFMTFontId(LotusParserInternal::LotusStream &stream);

	//! update style using color id for defining shadow
	bool updateShadowStyle(int colorId, WPSGraphicStyle &style) const;

private:
	LotusStyleManager(LotusStyleManager const &orig);
	LotusStyleManager &operator=(LotusStyleManager const &orig);
	//! the main parser
	LotusParser &m_mainParser;
	//! the internal state
	shared_ptr<LotusStyleManagerInternal::State> m_state;
};

#endif /* LOTUS_STYLE_MANAGER_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
