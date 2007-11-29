/* 
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2004-2007 Fridrich Strba (fridrich.strba@bluewin.ch)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
  *
 * For further information visit http://libwpd.sourceforge.net
 *
 */

#include "DiskDocumentHandler.hxx"

#include <string.h>

DiskDocumentHandler::DiskDocumentHandler(std::ostringstream &contentStream) :
        mContentStream(contentStream),
	mbIsTagOpened(false)
{
}

void DiskDocumentHandler::startElement(const char *psName, const WPXPropertyList &xPropList)
{
	if (mbIsTagOpened)
	{
		mContentStream << ">";
		mbIsTagOpened = false;
	}
	mContentStream << "<" << psName;

        WPXPropertyList::Iter i(xPropList);
        for (i.rewind(); i.next(); )
        {
                // filter out libwpd elements
                if (strncmp(i.key(), "libwpd", 6) != 0)
			mContentStream << " " <<  i.key() << "=\"" << i()->getStr().cstr() << "\"";
	}
	mbIsTagOpened = true;
	msOpenedTagName.sprintf("%s", psName);
}

void DiskDocumentHandler::endElement(const char *psName)
{
	if (mbIsTagOpened)
	{
		if( msOpenedTagName == psName )
		{
			mContentStream << "/>";
			mbIsTagOpened = false;
		}
		else // should not happen, but handle it
		{
			mContentStream << ">";
			mContentStream << "</" << psName << ">";
			mbIsTagOpened = false;
		}
	}
	else
	{
		mContentStream << "</" << psName << ">";
		mbIsTagOpened = false;
	}
}

void DiskDocumentHandler::characters(const WPXString &sCharacters)
{
	if (mbIsTagOpened)
	{
		mContentStream << ">";
		mbIsTagOpened = false;
	}
        WPXString sEscapedCharacters(sCharacters, true);
	mContentStream << sEscapedCharacters.cstr();
}

void DiskDocumentHandler::endDocument()
{
	if (mbIsTagOpened)
	{
		mContentStream << ">";
		mbIsTagOpened = false;
	}
}
