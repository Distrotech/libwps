/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libwps
 * Copyright (C) 2006, 2007 Andrew Ziem
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

#include "WPS8Struct.h"

/**
 * FOrmatting Descriptor (FOD)
 */
class WPSFOD
{
public:
	WPSFOD() : m_fcLim(0), m_bfprop(0), m_bfpropAbs(0), m_fprop() {}
	uint32_t	m_fcLim; /* byte number of last character covered by this FOD */
	uint16_t	m_bfprop; /* byte offset from beginning of FOD array to corresponding FPROP */
	uint32_t        m_bfpropAbs; /* bfprop from beginning of stream offset */
	WPS8Struct::FileData		m_fprop;	/* character or paragraph formatting */
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
