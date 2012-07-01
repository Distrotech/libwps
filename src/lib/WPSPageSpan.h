/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
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
 * For further information visit http://libwpd.sourceforge.net
 */

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#ifndef WPSPAGESPAN_H
#define WPSPAGESPAN_H
#include <vector>
#include "libwps_internal.h"

class WPXPropertyList;
class WPXDocumentProperty;
class WPSContentListener;

class WPSSubDocument;
typedef shared_ptr<WPSSubDocument> WPSSubDocumentPtr;

namespace WPSPageSpanInternal
{
class HeaderFooter;
typedef shared_ptr<HeaderFooter> HeaderFooterPtr;
}

class WPSPageSpan
{
	friend class WPSContentListener;
public:
	enum FormOrientation { PORTRAIT, LANDSCAPE };

	enum HeaderFooterType { HEADER, FOOTER };
	enum HeaderFooterOccurence { ODD, EVEN, ALL, NEVER };

	enum PageNumberPosition { None = 0, TopLeft, TopCenter, TopRight, TopLeftAndRight, TopInsideLeftAndRight,
	                          BottomLeft, BottomCenter, BottomRight, BottomLeftAndRight, BottomInsideLeftAndRight
	                        };
public:
	WPSPageSpan();
	virtual ~WPSPageSpan();

	double getFormLength() const
	{
		return m_formLength;
	}
	double getFormWidth() const
	{
		return m_formWidth;
	}
	FormOrientation getFormOrientation() const
	{
		return m_formOrientation;
	}
	double getMarginLeft() const
	{
		return m_marginLeft;
	}
	double getMarginRight() const
	{
		return m_marginRight;
	}
	double getMarginTop() const
	{
		return m_marginTop;
	}
	double getMarginBottom() const
	{
		return m_marginBottom;
	}
	PageNumberPosition getPageNumberPosition() const
	{
		return m_pageNumberPosition;
	}
	int getPageNumber() const
	{
		return m_pageNumber;
	}
	libwps::NumberingType getPageNumberingType() const
	{
		return m_pageNumberingType;
	}
	double getPageNumberingFontSize() const
	{
		return m_pageNumberingFontSize;
	}
	WPXString getPageNumberingFontName() const
	{
		return m_pageNumberingFontName;
	}
	int getPageSpan() const
	{
		return m_pageSpan;
	}
	const std::vector<WPSPageSpanInternal::HeaderFooterPtr> & getHeaderFooterList() const
	{
		return m_headerFooterList;
	}

	void setHeaderFooter(const HeaderFooterType type, const HeaderFooterOccurence occurence,
	                     WPSSubDocumentPtr &subDocument);
	void setFormLength(const double formLength)
	{
		m_formLength = formLength;
	}
	void setFormWidth(const double formWidth)
	{
		m_formWidth = formWidth;
	}
	void setFormOrientation(const FormOrientation formOrientation)
	{
		m_formOrientation = formOrientation;
	}
	void setMarginLeft(const double marginLeft)
	{
		m_marginLeft = marginLeft;
	}
	void setMarginRight(const double marginRight)
	{
		m_marginRight = marginRight;
	}
	void setMarginTop(const double marginTop)
	{
		m_marginTop = marginTop;
	}
	void setMarginBottom(const double marginBottom)
	{
		m_marginBottom = marginBottom;
	}
	void setPageNumberPosition(const PageNumberPosition pageNumberPosition)
	{
		m_pageNumberPosition = pageNumberPosition;
	}
	void setPageNumber(const int pageNumber)
	{
		m_pageNumber = pageNumber;
	}
	void setPageNumberingType(const libwps::NumberingType pageNumberingType)
	{
		m_pageNumberingType = pageNumberingType;
	}
	void setPageNumberingFontSize(const double pageNumberingFontSize)
	{
		m_pageNumberingFontSize = pageNumberingFontSize;
	}
	void setPageNumberingFontName(const WPXString &pageNumberingFontName)
	{
		m_pageNumberingFontName = pageNumberingFontName;
	}
	void setPageSpan(const int pageSpan)
	{
		m_pageSpan = pageSpan;
	}

	bool operator==(shared_ptr<WPSPageSpan> const &pageSpan) const;
	bool operator!=(shared_ptr<WPSPageSpan> const &pageSpan) const
	{
		return !operator==(pageSpan);
	}
protected:
	// interface with WPSContentListener
	void getPageProperty(WPXPropertyList &pList) const;
	void sendHeaderFooters(WPSContentListener *listener,
	                       WPXDocumentInterface *documentInterface);

protected:

	int _getHeaderFooterPosition(HeaderFooterType type, HeaderFooterOccurence occurence);
	void _setHeaderFooter(HeaderFooterType type, HeaderFooterOccurence occurence, WPSSubDocumentPtr &doc);
	void _removeHeaderFooter(HeaderFooterType type, HeaderFooterOccurence occurence);
	bool _containsHeaderFooter(HeaderFooterType type, HeaderFooterOccurence occurence);

	void _insertPageNumberParagraph(WPXDocumentInterface *documentInterface);
private:
	double m_formLength, m_formWidth;
	FormOrientation m_formOrientation;
	double m_marginLeft, m_marginRight;
	double m_marginTop, m_marginBottom;
	PageNumberPosition m_pageNumberPosition;
	int m_pageNumber;
	libwps::NumberingType m_pageNumberingType;
	WPXString m_pageNumberingFontName;
	double m_pageNumberingFontSize;
	std::vector<WPSPageSpanInternal::HeaderFooterPtr> m_headerFooterList;

	int m_pageSpan;
};

#endif
/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
