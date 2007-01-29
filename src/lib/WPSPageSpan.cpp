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

// precondition: 0 <= headerFooterType <= 3 (i.e.: we don't handle watermarks here)
WPSHeaderFooter::WPSHeaderFooter(const WPSHeaderFooterType headerFooterType, const WPSHeaderFooterOccurence occurence, 
				 const uint8_t internalType) :
	m_type(headerFooterType),
	m_occurence(occurence),
	m_internalType(internalType)
{
}

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

// NB: this is not a literal "clone" function: it is contingent on the side margins that are passed,
// and suppression variables are not copied
WPSPageSpan::WPSPageSpan(const WPSPageSpan &page, float paragraphMarginLeft, float paragraphMarginRight) :
	m_formLength(page.getFormLength()),
	m_formWidth(page.getFormWidth()),
	m_formOrientation(page.getFormOrientation()),
	m_marginLeft(page.getMarginLeft()+paragraphMarginLeft),
	m_marginRight(page.getMarginRight()+paragraphMarginRight),
	m_marginTop(page.getMarginTop()),
	m_marginBottom(page.getMarginBottom()),
	m_headerFooterList(page.getHeaderFooterList()),
	m_pageSpan(page.getPageSpan())
{
	for (int i=0; i<WPS_NUM_HEADER_FOOTER_TYPES; i++)
		m_isHeaderFooterSuppressed[i] = false;	
}

WPSPageSpan::~WPSPageSpan()
{
}


void WPSPageSpan::setHeaderFooter(const WPSHeaderFooterType type, const uint8_t headerFooterType, const WPSHeaderFooterOccurence occurence)
{
	WPSHeaderFooter headerFooter(type, occurence, headerFooterType);
	switch (occurence) 
	{
	case ALL:
	case NEVER:
		_removeHeaderFooter(type, ODD); _removeHeaderFooter(type, EVEN); _removeHeaderFooter(type, ALL);
		break;
	case ODD:
		_removeHeaderFooter(type, ODD); _removeHeaderFooter(type, ALL);
		break;
	case EVEN:
		_removeHeaderFooter(type, EVEN); _removeHeaderFooter(type, ALL);
		break;		
	}

	if (occurence != NEVER)
		m_headerFooterList.push_back(headerFooter);

	bool containsHFLeft = _containsHeaderFooter(type, ODD);
	bool containsHFRight = _containsHeaderFooter(type, EVEN);

	WPS_DEBUG_MSG(("Contains HFL: %i HFR: %i\n", containsHFLeft, containsHFRight));
	if (containsHFLeft && !containsHFRight)
	{
		WPS_DEBUG_MSG(("Inserting dummy header right\n"));
		WPSHeaderFooter dummyHeader(type, EVEN, DUMMY_INTERNAL_HEADER_FOOTER);
		m_headerFooterList.push_back(dummyHeader);
	}
	else if (!containsHFLeft && containsHFRight)
	{
		WPS_DEBUG_MSG(("Inserting dummy header left\n"));
		WPSHeaderFooter dummyHeader(type, ODD, DUMMY_INTERNAL_HEADER_FOOTER);
		m_headerFooterList.push_back(dummyHeader);
	}
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

void WPSPageSpan::_removeHeaderFooter(WPSHeaderFooterType type, WPSHeaderFooterOccurence occurence)
{
	for (std::vector<WPSHeaderFooter>::iterator iter = m_headerFooterList.begin(); iter != m_headerFooterList.end(); iter++) 
	{
		if ((*iter).getType() == type && (*iter).getOccurence() == occurence) {
			WPS_DEBUG_MSG(("MS Works: Removing header/footer element of type: %i since it is identical to %i\n",(*iter).getType(), type));
			m_headerFooterList.erase(iter);
			return;
		}
	}
}

bool WPSPageSpan::_containsHeaderFooter(WPSHeaderFooterType type, WPSHeaderFooterOccurence occurence)
{
	for (std::vector<WPSHeaderFooter>::iterator iter = m_headerFooterList.begin(); iter != m_headerFooterList.end(); iter++) 
	{
		if ((*iter).getType()==type && (*iter).getOccurence()==occurence)
			return true;
	}

	return false;
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
