/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */
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
	enum Type { DOS_850,
	            WIN3_ARABIC, WIN3_BALTIC, WIN3_CEUROPE, WIN3_CYRILLIC,
	            WIN3_GREEK, WIN3_HEBREW, WIN3_TURKISH,
	            WIN3_VIETNAMESE, WIN3_WEUROPE
	          };

	//! converts a character in unicode, knowing the character and the font type
	static unsigned long unicode(unsigned char c, Type type)
	{
		switch(type)
		{
		case DOS_850:
			return unicodeFromCP850(c);
		case WIN3_ARABIC:
			return unicodeFromCP1256(c);
		case WIN3_BALTIC:
			return unicodeFromCP1257(c);
		case WIN3_CEUROPE:
			return unicodeFromCP1250(c);
		case WIN3_CYRILLIC:
			return unicodeFromCP1251(c);
		case WIN3_GREEK:
			return unicodeFromCP1253(c);
		case WIN3_HEBREW:
			return unicodeFromCP1255(c);
		case WIN3_TURKISH:
			return unicodeFromCP1254(c);
		case WIN3_VIETNAMESE:
			return unicodeFromCP1258(c);
		case WIN3_WEUROPE:
			return unicodeFromCP1252(c);
		default:
			assert(0);
			return c;
		}
	}

	/*! \brief returns the type of the font using the fontName
	 * \param name the font name.
	 *
	 * \note \a name can be modified to suppress an extension
	 */
	static Type getWin3Type(std::string &name);

protected:
	//
	// DOS FONTS
	//

	//! dos850 to unicode
	static unsigned long unicodeFromCP850(unsigned char c);

	//! Windows3© central europe to unicode
	static unsigned long unicodeFromCP1250(unsigned char c);
	//! Windows3© cyrillic to unicode
	static unsigned long unicodeFromCP1251(unsigned char c);
	//! Windows3© western europe (the default) to unicode
	static unsigned long unicodeFromCP1252(unsigned char c);
	//! Windows3© greek to unicode
	static unsigned long unicodeFromCP1253(unsigned char c);
	//! Windows3© turkish to unicode
	static unsigned long unicodeFromCP1254(unsigned char c);
	//! Windows3© hebrew to unicode
	static unsigned long unicodeFromCP1255(unsigned char c);
	//! Windows3© arabic to unicode
	static unsigned long unicodeFromCP1256(unsigned char c);
	//! Windows3© baltic to unicode
	static unsigned long unicodeFromCP1257(unsigned char c);
	//! Windows3© vietnamese to unicode
	static unsigned long unicodeFromCP1258(unsigned char c);
};

// see http://msdn.microsoft.com/en-us/library/bb213877.aspx (Community Content)
/** an enum used to define the language in Windows© for spell check */
namespace Language
{
//! returns the name given Windows© id
std::string name(long id);
//! returns the simplified locale name
std::string localeName(long id);
//! returns the default language: 0x409 : english(US)
long getDefault();
}

}


#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
