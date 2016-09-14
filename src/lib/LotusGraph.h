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

#ifndef LOTUS_GRAPH_H
#define LOTUS_GRAPH_H

#include <ostream>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"

#include "WPSDebug.h"
#include "WKSContentListener.h"

namespace LotusGraphInternal
{
struct Zone;
struct State;

class SubDocument;
}

class LotusParser;
class LotusStyleManager;

/**
 * This class parses Microsoft Works graph file
 *
 */
class LotusGraph
{
public:
	friend class LotusParser;
	friend class LotusGraphInternal::SubDocument;

	//! constructor
	explicit LotusGraph(LotusParser &parser);
	//! destructor
	~LotusGraph();
	//! clean internal state
	void cleanState();
	//! sets the listener
	void setListener(WKSContentListenerPtr &listen)
	{
		m_listener = listen;
	}
protected:
	//! return the file version
	int version() const;

	//! return true if the sheet sheetId has some graphic
	bool hasGraphics(int sheetId) const;
	//! send the graphics corresponding to a sheetId
	void sendGraphics(int sheetId);
	//! try to send a picture
	void sendPicture(LotusGraphInternal::Zone const &zone);
	//! try to send a textbox content's
	void sendTextBox(RVNGInputStreamPtr &input, libwps::DebugFile &ascFile, WPSEntry const &entry);

	//
	// low level
	//

	// ////////////////////// zone //////////////////////////////

	//! reads a begin graphic zone
	bool readZoneBegin(WPSStream &stream, long endPos);
	//! reads a graphic zone
	bool readZoneData(WPSStream &stream, long endPos, int type);
	//! reads a graphic textbox data
	bool readTextBoxData(WPSStream &stream, long endPos);
	//! reads a picture definition
	bool readPictureDefinition(WPSStream &stream, long endPos);
	//! reads a picture data
	bool readPictureData(WPSStream &stream, long endPos);

private:
	LotusGraph(LotusGraph const &orig);
	LotusGraph &operator=(LotusGraph const &orig);
	shared_ptr<WKSContentListener> m_listener; /** the listener (if set)*/
	//! the main parser
	LotusParser &m_mainParser;
	//! the style manager
	shared_ptr<LotusStyleManager> m_styleManager;
	//! the internal state
	shared_ptr<LotusGraphInternal::State> m_state;
};

#endif /* LOTUS_GRAPH_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
