/* libwpd
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by 
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef WPXPAGE_H
#define WPXPAGE_H
#include "libwps.h"
#include "WPXFileStructure.h"
#include <vector>
#include "libwpd_internal.h"

// intermediate page representation class: for internal use only (by the high-level content/styles listeners). should not be exported.

class WPXHeaderFooter
{
public:
	WPXHeaderFooter(const WPXHeaderFooterType headerFooterType, const WPXHeaderFooterOccurence occurence, 
			const uint8_t internalType);
	WPXHeaderFooter(const WPXHeaderFooter &headerFooter);
	~WPXHeaderFooter();
	const WPXHeaderFooterType getType() const { return m_type; }
	const WPXHeaderFooterOccurence getOccurence() const { return m_occurence; }
	const uint8_t getInternalType() const { return m_internalType; }

private:
	WPXHeaderFooterType m_type;
	WPXHeaderFooterOccurence m_occurence;
	uint8_t m_internalType; // for suppression
};

class WPXPageSpan
{
public:
	WPXPageSpan();
	WPXPageSpan(const WPXPageSpan &page, float paragraphMarginLeft, float paragraphMarginRight);
	WPXPageSpan(const WPXPageSpan &page);
	virtual ~WPXPageSpan();

	const bool getHeaderFooterSuppression(const uint8_t headerFooterType) const { if (headerFooterType <= WPX_FOOTER_B) return m_isHeaderFooterSuppressed[headerFooterType]; return false; }
	const float getFormLength() const { return m_formLength; }
	const float getFormWidth() const { return m_formWidth; }
	const WPXFormOrientation getFormOrientation() const { return m_formOrientation; }
	const float getMarginLeft() const { return m_marginLeft; }
 	const float getMarginRight() const { return m_marginRight; }
 	const float getMarginTop() const { return m_marginTop; }
 	const float getMarginBottom() const { return m_marginBottom; }
	const int getPageSpan() const { return m_pageSpan; }
	const std::vector<WPXHeaderFooter> & getHeaderFooterList() const { return m_headerFooterList; }

	void setHeaderFooter(const WPXHeaderFooterType type, const uint8_t headerFooterType, const WPXHeaderFooterOccurence occurence);
	void setHeadFooterSuppression(const uint8_t headerFooterType, const bool suppress) { m_isHeaderFooterSuppressed[headerFooterType] = suppress; }
	void setFormLength(const float formLength) { m_formLength = formLength; }
	void setFormWidth(const float formWidth) { m_formWidth = formWidth; }
	void setFormOrientation(const WPXFormOrientation formOrientation) { m_formOrientation = formOrientation; }
	void setMarginLeft(const float marginLeft) { m_marginLeft = marginLeft; }
 	void setMarginRight(const float marginRight) { m_marginRight = marginRight; }
 	void setMarginTop(const float marginTop) { m_marginTop = marginTop; }
 	void setMarginBottom(const float marginBottom) { m_marginBottom = marginBottom; }
	void setPageSpan(const int pageSpan) { m_pageSpan = pageSpan; }
	
	void makeConsistent(int startingPageNumber);
	
protected:
	void _removeHeaderFooter(WPXHeaderFooterType type, WPXHeaderFooterOccurence occurence);
	bool _containsHeaderFooter(WPXHeaderFooterType type, WPXHeaderFooterOccurence occurence);

private:
	bool m_isHeaderFooterSuppressed[WPX_NUM_HEADER_FOOTER_TYPES];
	float m_formLength, m_formWidth;
	WPXFormOrientation m_formOrientation;
	float m_marginLeft, m_marginRight;
	float m_marginTop, m_marginBottom;
	std::vector<WPXHeaderFooter> m_headerFooterList;

	int m_pageSpan;
};

bool operator==(const WPXPageSpan &, const WPXPageSpan &);
#endif /* WPXPAGE_H */
