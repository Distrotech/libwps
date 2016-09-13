/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
* Version: MPL 2.0 / LGPLv2+
*
* The contents of this file are subject to the Mozilla Public License Version
* 2.0 (the "License"); you may not use this file except in compliance with
* the License or as specified alternatively below. You may obtain a copy of
* the License at http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* Major Contributor(s):
* Copyright (C) 2002 William Lachance (wrlach@gmail.com)
* Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
* Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
* Copyright (C) 2006, 2007 Andrew Ziem
* Copyright (C) 2011, 2012 Alonso Laurent (alonso@loria.fr)
*
*
* All Rights Reserved.
*
* For minor contributions see the git repository.
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
* in which case the provisions of the LGPLv2+ are applicable
* instead of those above.
*/

#ifndef HELPER_H
#  define HELPER_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if defined(SHAREDPTR_TR1)
#  include <tr1/memory>
using std::tr1::shared_ptr;
#elif defined(SHAREDPTR_STD)
#  include <memory>
using std::shared_ptr;
#else
#  include <boost/shared_ptr.hpp>
using boost::shared_ptr;
#endif

#  include <librevenge/librevenge.h>
#  include <libwps/libwps.h>

namespace libwpsHelper
{
/** check if a file is supported, if so returns the input stream
 the confidence, ... If not, returns an empty input stream.
*/
shared_ptr<librevenge::RVNGInputStream> isSupported
(char const *filename, libwps::WPSConfidence &confidence, libwps::WPSKind &kind, bool &needCharEncoding);
/** check for error, if yes, print an error message and returns
    true. If not return false */
bool checkErrorAndPrintMessage(libwps::WPSResult result);

/* ---------- debug  --------------- */
#if defined(__clang__) || defined(__GNUC__)
#  define WPS_ATTRIBUTE_PRINTF(fmt, arg) __attribute__((__format__(__printf__, fmt, arg)))
#else
#  define WPS_ATTRIBUTE_PRINTF(fmt, arg)
#endif

#ifdef DEBUG
void printDebugMsg(const char *format, ...) WPS_ATTRIBUTE_PRINTF(1, 2);
#  define WPS_DEBUG_MSG(M) libwpsHelper::printDebugMsg M
#else
#  define WPS_DEBUG_MSG(M)
#endif
}
#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
