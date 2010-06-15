/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

/*
 * This file is in sync with CVS
 * /libwpd2/src/lib/WPXPageSpan.h 1.21
 */

#ifndef WPSPAGE_H
#define WPSPAGE_H
#include "WPSFileStructure.h"
#include <vector>
#include "libwps_internal.h"

// intermediate page representation class: for internal use only (by the high-level content/styles listeners). should not be exported.

class WPSHeaderFooter
{
public:
	WPSHeaderFooter(const WPSHeaderFooter &headerFooter);
	~WPSHeaderFooter();
	WPSHeaderFooterType getType() const { return m_type; }
	WPSHeaderFooterOccurence getOccurence() const { return m_occurence; }
	uint8_t getInternalType() const { return m_internalType; }

private:
	WPSHeaderFooterType m_type;
	WPSHeaderFooterOccurence m_occurence;
	uint8_t m_internalType; // for suppression
};

class WPSPageSpan
{
public:
	WPSPageSpan();
	WPSPageSpan(const WPSPageSpan &page);
	virtual ~WPSPageSpan();

	bool getHeaderFooterSuppression(const uint8_t headerFooterType) const { if (headerFooterType <= WPS_FOOTER_B) return m_isHeaderFooterSuppressed[headerFooterType]; return false; }
	float getFormLength() const { return m_formLength; }
	float getFormWidth() const { return m_formWidth; }
	WPSFormOrientation getFormOrientation() const { return m_formOrientation; }
	float getMarginLeft() const { return m_marginLeft; }
 	float getMarginRight() const { return m_marginRight; }
 	float getMarginTop() const { return m_marginTop; }
 	float getMarginBottom() const { return m_marginBottom; }
	int getPageSpan() const { return m_pageSpan; }
	const std::vector<WPSHeaderFooter> & getHeaderFooterList() const { return m_headerFooterList; }

	void setHeadFooterSuppression(const uint8_t headerFooterType, const bool suppress) { m_isHeaderFooterSuppressed[headerFooterType] = suppress; }
	void setFormLength(const float formLength) { m_formLength = formLength; }
	void setFormWidth(const float formWidth) { m_formWidth = formWidth; }
	void setFormOrientation(const WPSFormOrientation formOrientation) { m_formOrientation = formOrientation; }
	void setMarginLeft(const float marginLeft) { m_marginLeft = marginLeft; }
 	void setMarginRight(const float marginRight) { m_marginRight = marginRight; }
 	void setMarginTop(const float marginTop) { m_marginTop = marginTop; }
 	void setMarginBottom(const float marginBottom) { m_marginBottom = marginBottom; }
	void setPageSpan(const int pageSpan) { m_pageSpan = pageSpan; }
	
	void makeConsistent(int startingPageNumber);
	
protected:
	void _removeHeaderFooter(WPSHeaderFooterType type, WPSHeaderFooterOccurence occurence);

private:
	bool m_isHeaderFooterSuppressed[WPS_NUM_HEADER_FOOTER_TYPES];
	float m_formLength, m_formWidth;
	WPSFormOrientation m_formOrientation;
	float m_marginLeft, m_marginRight;
	float m_marginTop, m_marginBottom;
	std::vector<WPSHeaderFooter> m_headerFooterList;

	int m_pageSpan;
};

bool operator==(const WPSPageSpan &, const WPSPageSpan &);
#endif /* WPSPAGE_H */
