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

#include <string>
#include <sstream>

#include "libwps_tools_win.h"

namespace libwps_tools_win
{
////////////////////////////////////////////////////////////
//
// FONT
//
////////////////////////////////////////////////////////////

Font::Type Font::getDosWin2Type(std::string &name)
{
	Font::Type res=getWin3Type(name);
	return res==WIN3_WEUROPE ? DOS_850 : res;
}

Font::Type Font::getWin3Type(std::string &fName)
{
	size_t len = fName.length();
	if (len && fName[len-1] != ')')
	{
		if (fName == "Baltica" || fName == "Pragmatica")
			return WIN3_CYRILLIC;
		if (len > 4 && (fName.find(" CYR", len-4) != std::string::npos ||
		                fName.find(" Cyr", len-4) != std::string::npos ||
		                fName.find(" cyr", len-4) != std::string::npos))
		{
			fName.resize(len-4);
			return WIN3_CYRILLIC;
		}
		if (len > 3 && (fName.find(" CE", len-3) != std::string::npos ||
		                fName.find(" Ce", len-3) != std::string::npos ||
		                fName.find(" ce", len-3) != std::string::npos))
		{
			fName.resize(len-3);
			return WIN3_CEUROPE;
		}
		if (len > 6 && (fName.find(" GREEK", len-6) != std::string::npos ||
		                fName.find(" Greek", len-6) != std::string::npos ||
		                fName.find(" greek", len-6) != std::string::npos))
		{
			fName.resize(len-6);
			return WIN3_GREEK;
		}
		if (len > 4 && (fName.find(" TUR", len-4) != std::string::npos ||
		                fName.find(" Tur", len-4) != std::string::npos ||
		                fName.find(" tur", len-4) != std::string::npos))
		{
			fName.resize(len-4);
			return WIN3_TURKISH;
		}
		if (len > 7 && (fName.find(" BALTIC", len-7) != std::string::npos ||
		                fName.find(" Baltic", len-7) != std::string::npos ||
		                fName.find(" baltic", len-7) != std::string::npos))
		{
			fName.resize(len-7);
			return WIN3_BALTIC;
		}
	}
	else if (len)
	{
		if (len > 9 && (fName.find(" (HEBREW)", len-9) != std::string::npos ||
		                fName.find(" (Hebrew)", len-9) != std::string::npos ||
		                fName.find(" (Hebrew)", len-9) != std::string::npos))
		{
			fName.resize(len-9);
			return WIN3_HEBREW;
		}
		if (len > 9 && (fName.find(" (ARABIC)", len-9) != std::string::npos ||
		                fName.find(" (Arabic)", len-9) != std::string::npos ||
		                fName.find(" (arabic)", len-9) != std::string::npos))
		{
			fName.resize(len-9);
			return WIN3_ARABIC;
		}
		if (len > 13 && (fName.find(" (VIETNAMESE)", len-13) != std::string::npos ||
		                 fName.find(" (Vietnamese)", len-13) != std::string::npos ||
		                 fName.find(" (vietnamese)", len-13) != std::string::npos))
		{
			fName.resize(len-13);
			return WIN3_VIETNAMESE;
		}
	}
	return WIN3_WEUROPE;
}

#ifdef UNDEF
#  undef UNDEF
#endif
#define UNDEF 0xfffd
/**
 * Take a character in CP850 encoding, convert it
 * Courtesy of glib2 and iconv
 **/
unsigned long Font::unicodeFromCP850(unsigned char c)
{
	// Fridrich: I see some MS Works files with IBM850 encoding ???
	static const unsigned int cp850[128] =
	{
		0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
		0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
		0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
		0x00ff, 0x00d6, 0x00dc, 0x00f8, 0x00a3, 0x00d8, 0x00d7, 0x0192,
		0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
		0x00bf, 0x00ae, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
		0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00c1, 0x00c2, 0x00c0,
		0x00a9, 0x2563, 0x2551, 0x2557, 0x255d, 0x00a2, 0x00a5, 0x2510,
		0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x00e3, 0x00c3,
		0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x00a4,
		0x00f0, 0x00d0, 0x00ca, 0x00cb, 0x00c8, 0x0131, 0x00cd, 0x00ce,
		0x00cf, 0x2518, 0x250c, 0x2588, 0x2584, 0x00a6, 0x00cc, 0x2580,
		0x00d3, 0x00df, 0x00d4, 0x00d2, 0x00f5, 0x00d5, 0x00b5, 0x00fe,
		0x00de, 0x00da, 0x00db, 0x00d9, 0x00fd, 0x00dd, 0x00af, 0x00b4,
		0x00ad, 0x00b1, 0x2017, 0x00be, 0x00b6, 0x00a7, 0x00f7, 0x00b8,
		0x00b0, 0x00a8, 0x00b7, 0x00b9, 0x00b3, 0x00b2, 0x25a0, 0x00a0
	};

	if (c < 0x80) return c;
	return cp850[c - 0x80];
}

/**
 * Take a character in CP1250 encoding, convert it
 * Courtesy of unicode.org: http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/
 **/
unsigned long Font::unicodeFromCP1250(unsigned char c)
{
	static unsigned int const cp1250[] =
	{
		0x20AC, UNDEF /* 0x81 */, 0x201A, UNDEF /* 0x83 */, 0x201E, 0x2026, 0x2020, 0x2021,
		UNDEF /* 0x88 */, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,
		UNDEF /* 0x90 */, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		UNDEF /* 0x98 */, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A,
		0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,
		0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,
		0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
		0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,
		0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
		0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
		0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
		0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
		0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
		0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
		0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
		0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9
	};
	if (c < 0x80) return c;
	return cp1250[c - 0x80];
}

/**
 * Take a character in CP1251 encoding, convert it
 * Courtesy of unicode.org: http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/
 **/
unsigned long Font::unicodeFromCP1251(unsigned char c)
{
	static unsigned int const cp1251[] =
	{
		0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
		0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
		0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		UNDEF /* 0x98 */, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
		0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
		0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
		0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
		0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
		0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
		0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
		0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
		0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
		0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
		0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
		0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
		0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
	};
	if (c < 0x80) return c;
	return cp1251[c - 0x80];
}

/**
 * Take a character in CP1252 encoding, convert it
 * Courtesy of glib2 and iconv
 *
 */
unsigned long Font::unicodeFromCP1252(unsigned char c)
{
	static const unsigned int cp1252[32] =
	{
		0x20ac, UNDEF, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
		0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, UNDEF, 0x017d, UNDEF,
		UNDEF, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
		0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, UNDEF, 0x017e, 0x0178
	};

	if (c < 0x80 || c >= 0xa0) return c;
	return cp1252[c - 0x80];
}

/**
 * Take a character in CP1253 encoding, convert it
 * Courtesy of unicode.org: http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/
 *
 */
unsigned long Font::unicodeFromCP1253(unsigned char c)
{
	static unsigned int const cp1253[] =
	{
		0x20AC, UNDEF /* 0x81 */, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
		UNDEF /* 0x88 */, 0x2030, UNDEF /* 0x8a */, 0x2039, UNDEF /* 0x8c */, UNDEF /* 0x8d */, UNDEF /* 0x8e */, UNDEF /* 0x8f */,
		UNDEF /* 0x90 */, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		UNDEF /* 0x98 */, 0x2122, UNDEF /* 0x9a */, 0x203A, UNDEF /* 0x9c */, UNDEF /* 0x9d */, UNDEF /* 0x9e */, UNDEF /* 0x9f */,
		0x00A0, 0x0385, 0x0386, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
		0x00A8, 0x00A9, UNDEF /* 0xaa */, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x2015,
		0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0384, 0x00B5, 0x00B6, 0x00B7,
		0x0388, 0x0389, 0x038A, 0x00BB, 0x038C, 0x00BD, 0x038E, 0x038F,
		0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
		0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
		0x03A0, 0x03A1, UNDEF /* 0xd2 */, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7,
		0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
		0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7,
		0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
		0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7,
		0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, UNDEF /* 0xff */,
	};
	if (c < 0x80) return c;
	return cp1253[c - 0x80];
}

/**
 * Take a character in CP1254 encoding, convert it
 * Courtesy of unicode.org: http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/
 *
 */
unsigned long Font::unicodeFromCP1254(unsigned char c)
{
	static unsigned int const cp1254[] =
	{
		0x20AC, UNDEF /* 0x81 */, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
		0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, UNDEF /* 0x8d */, UNDEF /* 0x8e */, UNDEF /* 0x8f */,
		UNDEF /* 0x90 */, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, UNDEF /* 0x9d */, UNDEF /* 0x9e */, 0x0178,
		0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
		0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
		0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
		0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
		0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
		0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
		0x011E, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
		0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0130, 0x015E, 0x00DF,
		0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
		0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
		0x011F, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
		0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0131, 0x015F, 0x00FF,
	};
	if (c < 0x80) return c;
	return cp1254[c - 0x80];
}

/**
 * Take a character in CP1255 encoding, convert it
 * Courtesy of unicode.org: http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/
 **/
unsigned long Font::unicodeFromCP1255(unsigned char c)
{
	static int const cp1255[] =
	{
		0x20AC, UNDEF /* 0x81 */, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
		0x02C6, 0x2030, UNDEF /* 0x8a */, 0x2039, UNDEF /* 0x8c */, UNDEF /* 0x8d */, UNDEF /* 0x8e */, UNDEF /* 0x8f */,
		UNDEF /* 0x90 */, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		0x02DC, 0x2122, UNDEF /* 0x9a */, 0x203A, UNDEF /* 0x9c */, UNDEF /* 0x9d */, UNDEF /* 0x9e */, UNDEF /* 0x9f */,
		0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AA, 0x00A5, 0x00A6, 0x00A7,
		0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
		0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
		0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
		0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7,
		0x05B8, 0x05B9, UNDEF /* 0xca */, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
		0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05F0, 0x05F1, 0x05F2, 0x05F3,
		0x05F4, UNDEF /* 0xd9 */, UNDEF /* 0xda */, UNDEF /* 0xdb */, UNDEF /* 0xdc */, UNDEF /* 0xdd */, UNDEF /* 0xde */, UNDEF /* 0xdf */,
		0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7,
		0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
		0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7,
		0x05E8, 0x05E9, 0x05EA, UNDEF /* 0xfb */, UNDEF /* 0xfc */, 0x200E, 0x200F, UNDEF /* 0xff */,
	};
	if (c < 0x80) return c;
	return (unsigned long)cp1255[c - 0x80];
}

/**
 * Take a character in CP1256 encoding, convert it
 * Courtesy of unicode.org: http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/
 **/
unsigned long Font::unicodeFromCP1256(unsigned char c)
{
	static int const cp1256[] =
	{
		0x20AC, 0x067E, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
		0x02C6, 0x2030, 0x0679, 0x2039, 0x0152, 0x0686, 0x0698, 0x0688,
		0x06AF, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		0x06A9, 0x2122, 0x0691, 0x203A, 0x0153, 0x200C, 0x200D, 0x06BA,
		0x00A0, 0x060C, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
		0x00A8, 0x00A9, 0x06BE, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
		0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
		0x00B8, 0x00B9, 0x061B, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x061F,
		0x06C1, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627,
		0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F,
		0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x00D7,
		0x0637, 0x0638, 0x0639, 0x063A, 0x0640, 0x0641, 0x0642, 0x0643,
		0x00E0, 0x0644, 0x00E2, 0x0645, 0x0646, 0x0647, 0x0648, 0x00E7,
		0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0649, 0x064A, 0x00EE, 0x00EF,
		0x064B, 0x064C, 0x064D, 0x064E, 0x00F4, 0x064F, 0x0650, 0x00F7,
		0x0651, 0x00F9, 0x0652, 0x00FB, 0x00FC, 0x200E, 0x200F, 0x06D2,
	};
	if (c < 0x80) return c;
	return (unsigned long) cp1256[c - 0x80];
}


/**
 * Take a character in CP1257 encoding, convert it
 * Courtesy of unicode.org: http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/
 **/
unsigned long Font::unicodeFromCP1257(unsigned char c)
{
	static unsigned int const cp1257[] =
	{
		0x20AC, UNDEF /* 0x81 */, 0x201A, UNDEF /* 0x83 */, 0x201E, 0x2026, 0x2020, 0x2021,
		UNDEF /* 0x88 */, 0x2030, UNDEF /* 0x8a */, 0x2039, UNDEF /* 0x8c */, 0x00A8, 0x02C7, 0x00B8,
		UNDEF /* 0x90 */, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		UNDEF /* 0x98 */, 0x2122, UNDEF /* 0x9a */, 0x203A, UNDEF /* 0x9c */, 0x00AF, 0x02DB, UNDEF /* 0x9f */,
		0x00A0, UNDEF /* 0xa1 */, 0x00A2, 0x00A3, 0x00A4, UNDEF /* 0xa5 */, 0x00A6, 0x00A7,
		0x00D8, 0x00A9, 0x0156, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00C6,
		0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
		0x00F8, 0x00B9, 0x0157, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00E6,
		0x0104, 0x012E, 0x0100, 0x0106, 0x00C4, 0x00C5, 0x0118, 0x0112,
		0x010C, 0x00C9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012A, 0x013B,
		0x0160, 0x0143, 0x0145, 0x00D3, 0x014C, 0x00D5, 0x00D6, 0x00D7,
		0x0172, 0x0141, 0x015A, 0x016A, 0x00DC, 0x017B, 0x017D, 0x00DF,
		0x0105, 0x012F, 0x0101, 0x0107, 0x00E4, 0x00E5, 0x0119, 0x0113,
		0x010D, 0x00E9, 0x017A, 0x0117, 0x0123, 0x0137, 0x012B, 0x013C,
		0x0161, 0x0144, 0x0146, 0x00F3, 0x014D, 0x00F5, 0x00F6, 0x00F7,
		0x0173, 0x0142, 0x015B, 0x016B, 0x00FC, 0x017C, 0x017E, 0x02D9,
	};
	if (c < 0x80) return c;
	return  cp1257[c - 0x80];
}

/**
 * Take a character in CP1258 encoding, convert it
 * Courtesy of unicode.org: http://unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/
 **/
unsigned long Font::unicodeFromCP1258(unsigned char c)
{
	static unsigned int const cp1258[] =
	{
		0x20AC, UNDEF /* 0x81 */, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
		0x02C6, 0x2030, UNDEF /* 0x8a */, 0x2039, 0x0152, UNDEF /* 0x8d */, UNDEF /* 0x8e */, UNDEF /* 0x8f */,
		UNDEF /* 0x90 */, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		0x02DC, 0x2122, UNDEF /* 0x9a */, 0x203A, 0x0153, UNDEF /* 0x9d */, UNDEF /* 0x9e */, 0x0178,
		0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
		0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
		0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
		0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
		0x00C0, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
		0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x0300, 0x00CD, 0x00CE, 0x00CF,
		0x0110, 0x00D1, 0x0309, 0x00D3, 0x00D4, 0x01A0, 0x00D6, 0x00D7,
		0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x01AF, 0x0303, 0x00DF,
		0x00E0, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
		0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0301, 0x00ED, 0x00EE, 0x00EF,
		0x0111, 0x00F1, 0x0323, 0x00F3, 0x00F4, 0x01A1, 0x00F6, 0x00F7,
		0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x01B0, 0x20AB, 0x00FF,
	};
	if (c < 0x80) return c;
	return cp1258[c - 0x80];
}

////////////////////////////////////////////////////////////
//
// Other
//
////////////////////////////////////////////////////////////

long Language::getDefault()
{
	return 0x409;
}

std::string Language::name(long id)
{
	switch(id)
	{
	case 0x400:
		return "none";
	case 0x401:
		return "arabic";
	case 0x402:
		return "bulgarian";
	case 0x403:
		return "catalan";
	case 0x404:
		return "chinese(Trad)";
	case 0x405:
		return "czech";
	case 0x406:
		return "danish";
	case 0x407:
		return "german";
	case 0x408:
		return "greek";
	case 0x409:
		return "english(US)";
	case 0x40A:
		return "spanish";
	case 0x40B:
		return "finish";
	case 0x40C:
		return "french";
	case 0x40D:
		return "hebrew";
	case 0x40E:
		return "hungarian";
	case 0x40F:
		return "islandic";
	case 0x410:
		return "italian";
	case 0x411:
		return "japonese";
	case 0x412:
		return "korean";
	case 0x413:
		return "dutch";
	case 0x414:
		return "norvegian";
	case 0x415:
		return "polish";
	case 0x416:
		return "portuguese(Brazil)";
	case 0x417:
		return "rhaeto(Romanic)";
	case 0x418:
		return "romania";
	case 0x419:
		return "russian";
	case 0x41D:
		return "swedish";
	case 0x420:
		return "croatian";
	case 0x809:
		return "english(UK)";
	case 0x80a:
		return "spanish(Mexican)";
	case 0x816:
		return "portuguese";
	case 0xC09:
		return "englAUS";
	case 0xC0A:
		return "spanish(Modern)";
	case 0xC0C:
		return "french(Canadian)";
	case 0x1009:
		return "englCan";
	case 0x100c:
		return "french(Swiss)";
	case 0x2C0A:
		return "spanish(Argentina)";
	case 0x3409:
		return "english(Philippines)";
	case 0x480A:
		return "spanish(Honduras)";
	default:
		break;
	}

	std::stringstream f;
	f << "###unkn=" << std::hex << id;
	return f.str();
}

std::string Language::localeName(long id)
{
	switch(id)
	{
	case 0x400:
		return "";
	case 0x401:
		return "ar_DZ"; // checkme
	case 0x402:
		return "bg_BG";
	case 0x403:
		return "ca_ES";
	case 0x404:
		return "zh_TW";
	case 0x405:
		return "cs_CZ";
	case 0x406:
		return "da_DK";
	case 0x407:
		return "de_DE";
	case 0x408:
		return "el_GR";
	case 0x409:
		return "en_US";
	case 0x40A:
		return "es_ES";
	case 0x40B:
		return "fi_FI";
	case 0x40C:
		return "fr_FR";
	case 0x40D:
		return "iw_IL";
	case 0x40E:
		return "hu_HU";
	case 0x40F:
		return "is_IS";
	case 0x410:
		return "it_IT";
	case 0x411:
		return "ja_JP";
	case 0x412:
		return "ko_KR";
	case 0x413:
		return "nl_NL";
	case 0x414:
		return "no_NO";
	case 0x415:
		return "pl_PL";
	case 0x416:
		return "pt_BR";
	case 0x417:
		return "rm_CH";
	case 0x418:
		return "ro_RO";
	case 0x419:
		return "ru_RU";
	case 0x41d:
		return "sv_SE";
	case 0x420:
		return "hr_HR";
	case 0x809:
		return "en_GB";
	case 0x80a:
		return "es_MX";
	case 0x816:
		return "pt_PT";
	case 0xC09:
		return "en_AU";
	case 0xC0a:
		return "es_ES";
	case 0xC0c:
		return "fr_CA";
	case 0x1009:
		return "en_CA";
	case 0x100c:
		return "fr_CH";
	case 0x2c0a:
		return "es_AR";
	case 0x3409:
		return "en_PH"; // english Philippines
	case 0x480A:
		return "es_HN";
	default:
		return "";
	}
}

void Language::addLocaleName(long id, WPXPropertyList &propList)
{
	if (id<0) return;
	std::string lang = libwps_tools_win::Language::localeName(id);
	if (lang.length())
	{
		std::string language(lang);
		std::string country("none");
		if (lang.length() > 3 && lang[2]=='_')
		{
			country=lang.substr(3);
			language=lang.substr(0,2);
		}
		propList.insert("fo:language", language.c_str());
		propList.insert("fo:country", country.c_str());
	}
	else
	{
		propList.insert("fo:language", "none");
		propList.insert("fo:country", "none");
	}
}

}
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
