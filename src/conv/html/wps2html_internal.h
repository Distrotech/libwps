/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#ifndef WPS2HTML_INTERNAL_H
#define WPS2HTML_INTERNAL_H

#ifdef DEBUG
#include <stdio.h>
#endif

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
	void operator()(T *) {}
};
/* ---------- debug  --------------- */
#ifdef DEBUG
#define WPS_DEBUG_MSG(M) printf M
#else
#define WPS_DEBUG_MSG(M)
#endif

class WPXProperty;
namespace wps2html
{
bool getPointValue(WPXProperty const &prop, double &res);
}
#endif /* WPS2HTML_INTERNAL_H */
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
