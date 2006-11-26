/* libwpd
 * Copyright (C) 2006 Andrew Ziem
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
 */

#ifndef WPS_H
#define WPS_H


/** 
 * CHaracter Properties (CHP) or
 * PAragraph Properties (PAP)
 *
 **/
class FPROP
{
public:
	FPROP() { cch = 0; };
	uint8_t	cch; /* number of bytes in this FPROP */
	std::string rgchProp; /* Prefix for a CHP or PAP sufficient to handle differing bits from default */
};

/**
 * FOrmatting Descriptor (FOD)
 */
class FOD
{
public:
	FOD() { fcLim = bfprop = bfprop_abs = 0; }
	uint32_t	fcLim; /* byte number of last character covered by this FOD */
	uint16_t	bfprop; /* byte offset from beginning of FOD array to corresponding FPROP */
	uint32_t        bfprop_abs; /* bfprop from beginning of stream offset */
	FPROP		fprop;	/* character or paragraph formatting */
};

#endif


