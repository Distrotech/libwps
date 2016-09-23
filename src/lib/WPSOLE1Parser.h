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

namespace WPSOLE1ParserInternal
{
struct State;
}

/** \brief a class used to parse a container which is used by Lotus123 (and also by RagTime).

	\note I suppose that this is related to some OLE1 format, but I am not sure.
 */
class WPSOLE1Parser
{
public:
	/** constructor knowing the file stream

	 \note the file stream is supposed to exist while the parser is not destructed */
	explicit WPSOLE1Parser(shared_ptr<WPSStream> fileStream);

	/** destructor */
	~WPSOLE1Parser();

	//! try to find the different zones
	bool createZones();

	/** try to return a string corresponding to a name: WK3, FM3, 123 */
	shared_ptr<WPSStream> getStringForName(std::string const &name) const;
	/** try to return a string corresponding to some id */
	shared_ptr<WPSStream> getStringForId(int id) const;
	/** try to retrieve the content of a graphic, knowing it id */
	bool updateEmbedded(int id, WPSEmbeddedObject &object) const;

private:
	//! a smart ptr used to stored the file data
	shared_ptr<WPSOLE1ParserInternal::State> m_state;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
