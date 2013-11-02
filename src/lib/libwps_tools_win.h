/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */
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

/* This header contains code specific to windows file :
 *     - a class used to convert DOS/Win3 font characters in unicode
 *     - a class used to defined the text Language
 */

#ifndef WPS_WIN
#  define WPS_WIN

#  include <assert.h>
#  include <string>

#  include "libwps_internal.h"

/** some Windows© classes and tools */
namespace libwps_tools_win
{
//!\brief a class to convert a DOS© or Windows3© character in unicode
class Font
{
public:
	//! enum Type \brief the knowned DOS© and Windows3© fonts
	enum Type { CP_424, CP_437, CP_737, CP_775,
	            DOS_850, CP_852, CP_855, CP_856, CP_857,
	            CP_860, CP_861, CP_862, CP_863, CP_864, CP_865,
	            CP_866, CP_869, CP_874, CP_1006,
	            WIN3_ARABIC, WIN3_BALTIC, WIN3_CEUROPE, WIN3_CYRILLIC,
	            WIN3_GREEK, WIN3_HEBREW, WIN3_TURKISH,
	            WIN3_VIETNAMESE, WIN3_WEUROPE,
	            UNKNOWN
	          };

	//! converts a character in unicode, knowing the character and the font type
	static unsigned long unicode(unsigned char c, Type type);
	/*! returns the type corresponding to Windows OEM */
	static Type getTypeForOEM(int oem);
	/*! \brief returns the type of the font using the fontName
	 * \param name the font name.
	 *
	 * \note \a name can be modified to suppress an extension
	 */
	static Type getFontType(std::string &name);
	//! return the type name
	static std::string getTypeName(Type type);
protected:
};

// see http://msdn.microsoft.com/en-us/library/bb213877.aspx (Community Content)
/** an enum used to define the language in Windows© for spell check */
namespace Language
{
//! returns the name given Windows© id
std::string name(long id);
//! returns the simplified locale name
std::string localeName(long id);
//! add locale name (ie. fo:language,fo:country ) to propList
void addLocaleName(long id, RVNGPropertyList &propList);
//! returns the default language: 0x409 : english(US)
long getDefault();
}

}


#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
