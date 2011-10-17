/* libwps
 * Copyright (C) 2002 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2002 Marc Maurer (uwog@uwog.net)
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

#include <math.h>
#include <algorithm>
#include "WPSPageSpan.h"
#include "libwps_internal.h"

const float WPS_DEFAULT_PAGE_MARGIN_TOP = 1.0f;
const float WPS_DEFAULT_PAGE_MARGIN_BOTTOM = 1.0f;
const uint8_t DUMMY_INTERNAL_HEADER_FOOTER = 16;

WPSHeaderFooter::WPSHeaderFooter(const WPSHeaderFooter &headerFooter) :
	m_type(headerFooter.getType()),
	m_occurence(headerFooter.getOccurence()),
	m_internalType(headerFooter.getInternalType())
{
}

WPSHeaderFooter::~WPSHeaderFooter()
{
}

WPSPageSpan::WPSPageSpan() :
	m_formLength(11.0f),
	m_formWidth(8.5f),
	m_formOrientation(PORTRAIT),
	m_marginLeft(1.0f),
	m_marginRight(1.0f),
	m_marginTop(WPS_DEFAULT_PAGE_MARGIN_TOP),
	m_marginBottom(WPS_DEFAULT_PAGE_MARGIN_BOTTOM),
	m_headerFooterList(),
	m_pageSpan(1)
{
	for (int i=0; i<WPS_NUM_HEADER_FOOTER_TYPES; i++)
		m_isHeaderFooterSuppressed[i]=false;
}

WPSPageSpan::WPSPageSpan(const WPSPageSpan &page) :
	m_formLength(page.getFormLength()),
	m_formWidth(page.getFormWidth()),
	m_formOrientation(page.getFormOrientation()),
	m_marginLeft(page.getMarginLeft()),
	m_marginRight(page.getMarginRight()),
	m_marginTop(page.getMarginTop()),
	m_marginBottom(page.getMarginBottom()),
	m_headerFooterList(page.getHeaderFooterList()),
	m_pageSpan(page.getPageSpan())
{
	for (int i=0; i<WPS_NUM_HEADER_FOOTER_TYPES; i++)
		m_isHeaderFooterSuppressed[i] = page.getHeaderFooterSuppression(i);	
}

WPSPageSpan::~WPSPageSpan()
{
}

// makeConsistent: post-process page spans (i.e.: save this until all page spans are fully parsed)
// since this is a span, not an individuated page, we have to swap header/footer odd/even paramaters
// if we're not starting on an odd page
// ALSO: add a left/right footer to the page, if we have one but not the other (post-processing step)
void WPSPageSpan::makeConsistent(int startingPageNumber)
{
	if (!(startingPageNumber % 2))
	{
	// not sure whether this has any use (Fridrich) ?
	}
}

inline bool operator==(const WPSHeaderFooter &headerFooter1, const WPSHeaderFooter &headerFooter2)
{
	return ((headerFooter1.getType() == headerFooter2.getType()) && 
		(headerFooter1.getOccurence() == headerFooter2.getOccurence()) &&
		(headerFooter1.getInternalType() == headerFooter2.getInternalType()) );
}

bool operator==(const WPSPageSpan &page1, const WPSPageSpan &page2)
{
	if ((page1.getMarginLeft() != page2.getMarginLeft()) || (page1.getMarginRight() != page2.getMarginRight()) ||
	    (page1.getMarginTop() != page2.getMarginTop())|| (page1.getMarginBottom() != page2.getMarginBottom()))
		return false;


	for (int i=0; i<WPS_NUM_HEADER_FOOTER_TYPES; i++) {
		if (page1.getHeaderFooterSuppression(i) != page2.getHeaderFooterSuppression(i))
			return false;
	}

	// NOTE: yes this is O(n^2): so what? n=4 at most
	const std::vector<WPSHeaderFooter> headerFooterList1 = page1.getHeaderFooterList();
	const std::vector<WPSHeaderFooter> headerFooterList2 = page2.getHeaderFooterList();
	std::vector<WPSHeaderFooter>::const_iterator iter1;		
	std::vector<WPSHeaderFooter>::const_iterator iter2;		

	for (iter1 = headerFooterList1.begin(); iter1 != headerFooterList1.end(); iter1++)
	{
		if (std::find(headerFooterList2.begin(), headerFooterList2.end(), (*iter1)) == headerFooterList2.end())
			return false;
	}
	
	// If we came here, we know that every header/footer that is found in the first page span is in the second too.
	// But this is not enought for us to know whether the page spans are equal. Now we have to check in addition
	// whether every header/footer that is in the second one is in the first too. If someone wants to optimize this,
	// (s)he is most welcome :-)
	
	for (iter2 = headerFooterList2.begin(); iter2 != headerFooterList2.end(); iter2++)
	{
		if (std::find(headerFooterList1.begin(), headerFooterList1.end(), (*iter2)) == headerFooterList1.end())
			return false;
	}

	
	WPS_DEBUG_MSG(("MS Works: WPSPageSpan == comparison finished, found no differences\n"));

	return true;
}
