/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2009, 2011 Alonso Laurent (alonso@loria.fr)
 * Copyright (C) 2006, 2007 Andrew Ziem
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 * Copyright (C) 2004 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
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

/*
 */

#ifndef WPS_OLE1_PARSER_H
#define WPS_OLE1_PARSER_H

#include <string>

#include <librevenge-stream/librevenge-stream.h>

#include "libwps_internal.h"
#include "libwps_tools_win.h"

namespace WPSOLE1ParserInternal
{
struct State;
struct OLEZone;
}

/** \brief a class used to parse a container which is used by Lotus123 (and also by RagTime).

	\note I suppose that this is related to some OLE1 format, but I am not sure.
 */
class WPSOLE1Parser
{
public:
	/** constructor knowing the file stream */
	explicit WPSOLE1Parser(shared_ptr<WPSStream> fileStream);

	/** destructor */
	~WPSOLE1Parser();

	//! try to find the different zones
	bool createZones();

	/** try to return a string corresponding to a name:
		- WK3, FM3, lotus 123 v5 main file part
		- 123, lotus 123 v6+ main file part
		- CR3, maybe some database ?
		- Doc Info Object (its children defines author, ...)
		- WCHChart some Chart
		- Lotus:TOOLS:ByteStream some object
	 */
	shared_ptr<WPSStream> getStreamForName(std::string const &name) const;
	/** try to return a string corresponding to some id */
	shared_ptr<WPSStream> getStreamForId(int id) const;
	/** try to retrieve the meta data */
	bool updateMetaData(librevenge::RVNGPropertyList &list, libwps_tools_win::Font::Type encoding) const;
	/** try to retrieve the content of a graphic, knowing it local id */
	bool updateEmbeddedObject(int id, WPSEmbeddedObject &object) const;

protected:
	/** try to update the zone name */
	bool updateZoneNames(WPSOLE1ParserInternal::OLEZone &zone) const;
	/// try to return a stream correponding to a zone
	shared_ptr<WPSStream> getStream(WPSOLE1ParserInternal::OLEZone const &zone) const;
	/// check for unparsed zone
	void checkIfParsed(WPSOLE1ParserInternal::OLEZone const &zone) const;

	//! try to read a picture's frame: 0105 local zone
	bool readPicture(shared_ptr<WPSStream> stream, WPSEmbeddedObject &object) const;

private:
	//! a smart ptr used to stored the file data
	shared_ptr<WPSOLE1ParserInternal::State> m_state;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
