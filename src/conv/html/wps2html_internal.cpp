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

#include <string.h>
#include <cstring>

#include <libwpd/libwpd.h>

#include "wps2html_internal.h"

namespace wps2html
{
bool getPointValue(WPXProperty const &prop, double &res)
{
	res = prop.getDouble();

	// try to guess the type
	WPXString str = prop.getStr();

	// point->pt, twip->*, inch -> in
	char c = str.len() ? str.cstr()[str.len()-1] : ' ';
	if (c == '*') res /= 1440.;
	else if (c == 't') res /= 72.;
	else if (c == 'n') ;
	else if (c == '%')
		return false;
	res *= 72.;
	return true;
}
}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
