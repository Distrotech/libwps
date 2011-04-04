/* libwps
 * Copyright (C) 2002, 2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002, 2004 Marc Maurer (uwog@uwog.net)
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

#include "libwps_internal.h"
#include <string>
#include <stdlib.h>
#include <locale.h>

uint8_t readU8(WPXInputStream *input)
{
	unsigned long numBytesRead;
	unsigned char const * p = input->read(sizeof(uint8_t), numBytesRead);
	
  	if (!p || numBytesRead != sizeof(uint8_t))
  		throw FileException();

	return *(uint8_t const *)(p);
}

uint16_t readU16(WPXInputStream *input)
{
	unsigned short p0 = (unsigned short)readU8(input);
	unsigned short p1 = (unsigned short)readU8(input);
	return p0|(p1<<8);
}

uint32_t readU32(WPXInputStream *input)
{
	uint8_t p0 = readU8(input);
	uint8_t p1 = readU8(input);
	uint8_t p2 = readU8(input);
	uint8_t p3 = readU8(input);
        return (uint32_t) ((p0<<0)|(p1<<8)|(p2<<16)|(p3<<24));
}

#ifdef DEBUG
std::string to_bits(std::string s)
{
	std::string r;
	for (unsigned int i = 0; i < s.length(); i++)
	{
		std::bitset<8> b(s[i]);	
		r.append(b.to_string<char,std::char_traits<char>,std::allocator<char> >());
		char buf[20];
		sprintf(buf, "(%02u,0x%02x)  ", (uint8_t)s[i],(uint8_t)s[i]);
		r.append(buf);
	}
	return r;
}
#endif

/* Not troubling ourselves with extra dependencies */
/* TODO: extend */

static const struct _lange
{
	uint32_t id;
	const char *name;
} s_lang_table[] = {
	{0x409,"en-US"},
	{0x419,"ru-RU"}
};

static int _ltcomp(const void *k1, const void *k2)
{
	int r = (int)((ssize_t)k1) - ((_lange*)k2)->id;
	return r;
}

const char *getLangFromLCID(uint32_t lcid)
{
	_lange *c = (_lange*) bsearch((const void*)lcid,s_lang_table,
		sizeof(s_lang_table)/sizeof(_lange),
		sizeof(_lange),_ltcomp);
	if (c) return c->name;
	return "-none-";
}


